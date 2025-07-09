#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <utility>
#include <chrono>
#include <filesystem>

#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>

#include "gpu.hpp"
#include "fdinfo.hpp"
#include "../common/socket.hpp"
#include "memory.hpp"
#include "cpu/cpu.hpp"
#include "iostats.hpp"
#include "api.hpp"

std::mutex current_metrics_lock;
metrics current_metrics;

std::atomic<bool> should_exit = false;

spdlog::level::level_enum get_log_level() {
    const char* ch_log_level = getenv("MANGOHUD_LOG_LEVEL");

    if (!ch_log_level)
        return spdlog::level::debug;

    const std::string log_level(ch_log_level);

    if (log_level == "trace")
        return spdlog::level::trace;
    else if (log_level == "debug")
        return spdlog::level::debug;
    else if (log_level == "info")
        return spdlog::level::info;
    else if (log_level == "warn")
        return spdlog::level::warn;
    else if (log_level == "err")
        return spdlog::level::critical;
    else if (log_level == "off")
        return spdlog::level::off;
    else
        return spdlog::level::debug;
}

void poll_metrics(CPU& cpu, GPUS& gpus, IOStats& iostats) {
    metrics m = {};

    {
        std::unique_lock lock(current_metrics_lock);
        m = current_metrics;
    }

    {
        std::set<pid_t> pids_to_delete;

        for (std::pair<const pid_t, process_metrics>& proc : m.pids) {
            pid_t pid = proc.first;

            if (!std::filesystem::exists("/proc/" + std::to_string(pid)))
                pids_to_delete.insert(pid);
        }

        for (const auto& p : pids_to_delete) {
            SPDLOG_TRACE("deleting pid {}", p);
            m.pids.erase(p);
        }
    }

    // ====START CPU INFO===========================================================
    cpu.poll();

    m.cpu = cpu.get_info();

    uint16_t num_of_cores = 0;
    for (core_info_t core : cpu.get_core_info()) {
        m.cores[num_of_cores] = core;
        num_of_cores++;
    }

    m.num_of_cores = num_of_cores;
    // ====END CPU INFO=============================================================

    // ====START GPU INFO===========================================================
    uint8_t num_of_gpus = 0;

    for (std::shared_ptr<GPU>& gpu : gpus.available_gpus) {
        m.gpus[num_of_gpus] = gpu->get_system_metrics();
        num_of_gpus++;

        if (m.gpus[num_of_gpus].is_apu) {
            float apu_power = m.gpus[num_of_gpus].apu_cpu_power;
            int apu_temp  = m.gpus[num_of_gpus].apu_cpu_temp;

            // maybe make it configurable
            if (apu_power > m.cpu.power)
                m.cpu.power = apu_power;

            if (apu_temp > m.cpu.temp)
                m.cpu.temp = apu_temp;
        }
    }

    m.num_of_gpus = num_of_gpus;

    for (std::pair<const pid_t, process_metrics>& proc : m.pids) {
        const pid_t pid = proc.first;
        num_of_gpus = 0;

        for (std::shared_ptr<GPU>& gpu : gpus.available_gpus)
            m.pids[pid].gpus[num_of_gpus++] = gpu->get_process_metrics(pid);
    }
    // ====END GPU INFO=============================================================

    // ====START MEMORY INFO========================================================
    for (std::pair<const pid_t, process_metrics>& proc : m.pids) {
        pid_t pid = proc.first;

        std::map<std::string, float> mem_stats = get_process_memory(pid);
        m.pids[pid].memory = {
            .resident = mem_stats["resident"],
            .shared = mem_stats["shared"],
            .virt = mem_stats["virtual"]
        };
    }

    std::map<std::string, float> ram_stats = get_ram_info();

    m.memory = {
        .used = ram_stats["used"],
        .total = ram_stats["total"],
        .swap_used = ram_stats["swap_used"],
    };
    // ====END MEMORY INFO==========================================================

    // ====START IO INFO============================================================
    iostats.poll();

    for (std::pair<const pid_t, process_metrics>& proc : m.pids) {
        pid_t pid = proc.first;
        m.pids[pid].io_stats = iostats.get_stats(pid);
    }
    // ====END IO INFO==============================================================

    {
        std::unique_lock lock(current_metrics_lock);
        current_metrics = m;
    }
}

mangohud_message form_mangohud_message(pid_t pid) {
    metrics m = {};
    mangohud_message msg = {};

    {
        std::unique_lock lock(current_metrics_lock);
        m = current_metrics;
    }

    process_metrics proc_metrics = m.pids[pid];

    msg.num_of_gpus = m.num_of_gpus;

    for (size_t i = 0; i < sizeof(m.gpus) / sizeof(m.gpus[0]); i++)
        msg.gpus[i] = {
            .process_metrics = proc_metrics.gpus[i],
            .system_metrics = m.gpus[i]
        };

    msg.memory = {
        .used = m.memory.used,
        .total = m.memory.total,
        .swap_used = m.memory.swap_used,

        .process_resident = proc_metrics.memory.resident,
        .process_shared = proc_metrics.memory.shared,
        .process_virtual = proc_metrics.memory.virt
    };

    msg.io_stats = proc_metrics.io_stats;
    msg.cpu = m.cpu;
    msg.num_of_cores = m.num_of_cores;
    std::memcpy(&msg.cores, &m.cores, sizeof(m.cores));

    return msg;
}

bool setup_socket(int& sock) {
    std::string socket_path = get_socket_path();

    if (socket_path.empty())
        return false;

    SPDLOG_INFO("Socket path: {}", socket_path);

    sock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);

    if (sock < 0) {
        LOG_UNIX_ERRNO_ERROR("Couldn't create socket.");
        return false;
    }

    SPDLOG_INFO("Socket created.");

    if (std::filesystem::exists(socket_path) && !std::filesystem::remove(socket_path)) {
        SPDLOG_ERROR("Failed to delete existing socket file");
        return false;
    }

    const sockaddr_un addr = {
        .sun_family = AF_UNIX
    };

    std::strncpy(const_cast<char*>(addr.sun_path), socket_path.c_str(), socket_path.size());

    int ret = bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(sockaddr_un));

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("Failed to assign name to socket.");
        return false;
    }

    // https://linux.die.net/man/2/setsockopt
    // Most socket-level options utilize an int argument for optval. For setsockopt(),
    // the argument should be nonzero to enable a boolean option, or zero if the option
    // is to be disabled.
    const int optval = 1;

    if (setsockopt(sock, SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval)) == -1) {
        LOG_UNIX_ERRNO_ERROR("Failed to enable credentials support for socket.");
        return false;
    }

    ret = listen(sock, 0);

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("Failed to listen to socket.");
        return false;
    }

    if (getuid() == 0) {
        ret = chmod(socket_path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (ret < 0) {
            LOG_UNIX_ERRNO_ERROR("Failed to make socket available to everyone.");
            return false;
        }
    }

    return true;
}

void sigint_handler(int signum) {
    should_exit = true;
}

int main() {
    spdlog::set_level(get_log_level());
    signal(SIGINT, sigint_handler);

    int sock = 0;

    if (!setup_socket(sock)) {
        SPDLOG_ERROR("Socket setup failed.");
        return -1;
    }

    std::vector<pollfd> poll_fds = {
        { .fd = sock, .events = POLLIN }
    };

    GPUS gpus;
    CPU cpu;
    IOStats iostats;

    std::chrono::time_point<std::chrono::steady_clock> last_stats_poll;

    std::thread api_thread(&api_server_thread);
    pthread_setname_np(api_thread.native_handle(), "api-server");

    while (!should_exit) {
        std::chrono::time_point<std::chrono::steady_clock> cur_time =
            std::chrono::steady_clock().now();

        if (cur_time - last_stats_poll > 500ms) {
            poll_metrics(cpu, gpus, iostats);
            last_stats_poll = cur_time;
        }

        int ret = poll(poll_fds.data(), poll_fds.size(), 100);

        if (ret < 0) {
            LOG_UNIX_ERRNO_ERROR("poll() failed.");
            continue;
        }

        // no new data
        if (ret < 1)
            continue;

        std::set<std::vector<pollfd>::iterator> fds_to_close;
        std::vector<pollfd> fds_to_add;

        // Check events
        for (auto fd = poll_fds.begin(); fd != poll_fds.end(); fd++) {
            if (fd->revents == 0)
                continue;

            // SPDLOG_TRACE("fd {}: revents = {}", fd->fd, fd->revents);

            if (fd->fd == sock) {
                ret = accept(fd->fd, nullptr, nullptr);

                if (ret < 0) {
                    LOG_UNIX_ERRNO_WARN("Failed to accept new connection.");
                    continue;
                }

                fds_to_add.push_back({.fd = ret, .events = POLLIN});
                SPDLOG_INFO("Accepted new connection: fd={}", ret);
            } else {
                if (fd->revents & POLLHUP || fd->revents & POLLNVAL) {
                    fds_to_close.insert(fd);
                    continue;
                }

                size_t pid = 0;
                if (receive_message_with_creds(fd->fd, pid)) {
                    bool pid_exists = false;

                    {
                        std::unique_lock lock(current_metrics_lock);
                        pid_exists = current_metrics.pids.find(pid) != current_metrics.pids.end();
                    }

                    if (!pid_exists) {
                        iostats.add_pid(pid);

                        for (auto& gpu : gpus.available_gpus) {
                            gpu->add_pid(pid);

                            if (FDInfo* ptr = dynamic_cast<FDInfo*>(gpu.get()))
                                ptr->fdinfo.add_pid(pid);
                        }

                        current_metrics.pids.try_emplace(pid, process_metrics());
                    }

                    mangohud_message msg = form_mangohud_message(pid);
                    send_message(fd->fd, msg);
                }
            }
        }

        // Close broken connections in reverse to not trigger vector reordering
        for (auto it = fds_to_close.rbegin(); it != fds_to_close.rend(); it++) {
            std::vector<pollfd>::iterator fd = *it;
            // SPDLOG_TRACE("fd = {}", fd->fd);

            ret = close(fd->fd);

            if (ret < 0) {
                LOG_UNIX_ERRNO_WARN("Failed to close connection fd {}.", fd->fd);
                continue;
            }

            // do not put spdlog_info() after poll_fds.erase(),
            // because fd will now point to next element.
            SPDLOG_INFO("Closed connection fd {}", fd->fd);
            poll_fds.erase(fd);
        }

        // Add new connections to poll()
        for (auto& fd : fds_to_add) {
            poll_fds.push_back(fd);
        }
    }

    api_thread.join();

    return 0;
}
