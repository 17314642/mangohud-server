#include <set>

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>
#include <sys/stat.h>

#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include "api.hpp"
#include "../common/gpu_metrics.hpp"
#include "../common/log_errno.hpp"

std::string form_json_response() {
    using json = nlohmann::ordered_json;

    metrics m = {};

    {
        std::unique_lock lock(current_metrics_lock);
        m = current_metrics;
    }

    json j;

    // ====START CPU INFO===========================================================
    j["cpu"] = {
        { "num_of_cores" , m.num_of_cores   },
        { "load"         , m.cpu.load       },
        { "frequency"    , m.cpu.frequency  },
        { "temp"         , m.cpu.temp       },
        { "power"        , m.cpu.power      }
    };

    for (uint16_t i = 0; i < m.num_of_cores; i++) {
        j["cpu"]["cores"].push_back({
            { "load"         , m.cores[i].load      },
            { "frequency"    , m.cores[i].frequency }
        });
    }
    // ====END CPU INFO=============================================================

    // ====START GPU INFO===========================================================
    for (uint16_t i = 0; i < m.num_of_gpus; i++) {
        gpu_metrics_system_t& g = m.gpus[i];

#define METRIC(m) { #m, g.m }
        j["gpu"].push_back({
            METRIC(load),

            METRIC(vram_used),
            METRIC(gtt_used),
            METRIC(memory_total),
            METRIC(memory_clock),
            METRIC(memory_temp),

            METRIC(temperature),
            METRIC(junction_temperature),

            METRIC(core_clock),
            METRIC(voltage),

            METRIC(power_usage),
            METRIC(power_limit),

            METRIC(is_apu),
            METRIC(apu_cpu_power),
            METRIC(apu_cpu_temp),

            METRIC(is_power_throttled),
            METRIC(is_current_throttled),
            METRIC(is_temp_throttled),
            METRIC(is_other_throttled),

            METRIC(fan_speed),
            METRIC(fan_rpm)
        });
    }
    // ====END GPU INFO=============================================================

    // ====START MEMORY INFO========================================================
    j["memory"] = {
        { "used", m.memory.used },
        { "total", m.memory.total },
        { "swap_used", m.memory.swap_used },
    };
    // ====END MEMORY INFO==========================================================

    // ====START CLIENTS INFO=======================================================
    for (std::pair<const pid_t, process_metrics>& proc : m.pids) {
        pid_t pid = proc.first;
        std::string s_pid = std::to_string(pid);
        process_metrics& p = proc.second;

        for (uint16_t i = 0; i < m.num_of_gpus; i++) {
            gpu_metrics_process_t g = p.gpus[i];

            j["clients"][s_pid]["gpu"].push_back({
                METRIC(load),
                METRIC(vram_used),
                METRIC(gtt_used)
            });
        }

        j["clients"][s_pid]["memory"] = {
            { "resident", p.memory.resident },
            { "shared", p.memory.shared },
            { "virt", p.memory.virt },
        };

        j["clients"][s_pid]["io"] = {
            { "read_mb_per_sec", p.io_stats.read_mb_per_sec },
            { "write_mb_per_sec", p.io_stats.write_mb_per_sec },
        };
    }
    // ====END CLIENTS INFO=========================================================

    return j.dump(4);

#undef METRIC
}

void api_server_thread() {
    int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (sock < 0) {
        LOG_UNIX_ERRNO_ERROR("Couldn't create socket.");
        return;
    }

    SPDLOG_INFO("Socket created.");

    const sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(45050),
        .sin_addr = {
            .s_addr = htonl(INADDR_LOOPBACK)
        }
    };

    int ret = bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(sockaddr_in));

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("Failed to configure socket settings.");
        return;
    }

    ret = listen(sock, 0);

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("Failed to listen to socket.");
        return;
    }

    std::vector<pollfd> poll_fds = {
        { .fd = sock, .events = POLLIN }
    };

    while (!should_exit) {
        int ret = poll(poll_fds.data(), poll_fds.size(), 1000);

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

                fds_to_add.push_back({.fd = ret, .events = POLLOUT});
                SPDLOG_TRACE("Accepted new connection: fd={}", ret);
            } else {
                if (fd->revents & POLLHUP || fd->revents & POLLNVAL) {
                    fds_to_close.insert(fd);
                    continue;
                }

                std::string response = form_json_response() + "\n";
                send(fd->fd, response.data(), response.size(), 0);

                fds_to_close.insert(fd);
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
            SPDLOG_TRACE("Closed connection fd {}", fd->fd);
            poll_fds.erase(fd);
        }

        // Add new connections to poll()
        for (auto& fd : fds_to_add) {
            poll_fds.push_back(fd);
        }
    }
}
