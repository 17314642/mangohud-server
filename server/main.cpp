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

#include "gpu.hpp"
#include "fdinfo.hpp"
#include "../common/socket.hpp"

int main() {
    spdlog::set_level(spdlog::level::level_enum::debug);

    std::string socket_path = get_socket_path();

    if (socket_path.empty())
        return -1;

    SPDLOG_INFO("Socket path: {}", socket_path);

    int sock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);

    if (sock < 0) {
        LOG_UNIX_ERRNO_ERROR("Couldn't create socket.");
        return -1;
    }

    SPDLOG_INFO("Socket created.");

    if (std::filesystem::exists(socket_path) && !std::filesystem::remove(socket_path)) {
        SPDLOG_ERROR("Failed to delete existing socket file");
        return -1;
    }

    const sockaddr_un addr = {
        .sun_family = AF_UNIX
    };

    std::strncpy(const_cast<char*>(addr.sun_path), socket_path.c_str(), socket_path.size());

    int ret = bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(sockaddr_un));

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("Failed to assign name to socket.");
        return -1;
    }

    // https://linux.die.net/man/2/setsockopt
    // Most socket-level options utilize an int argument for optval. For setsockopt(),
    // the argument should be nonzero to enable a boolean option, or zero if the option
    // is to be disabled.
    const int optval = 1;

    if (setsockopt(sock, SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval)) == -1) {
        LOG_UNIX_ERRNO_ERROR("Failed to enable credentials support for socket.");
        return -1;
    }

    ret = listen(sock, 0);

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("Failed to listen to socket.");
        return -1;
    }

    if (getuid() == 0) {
        ret = chmod(socket_path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (ret < 0) {
            LOG_UNIX_ERRNO_ERROR("Failed to make socket available to everyone.");
            return -1;
        }
    }

    std::vector<pollfd> poll_fds = {
        { .fd = sock, .events = POLLIN }
    };

    std::chrono::time_point<std::chrono::steady_clock> previous_time;

    GPUS gpus;

    while (true) {
        ret = poll(poll_fds.data(), poll_fds.size(), 1000);

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
                    // TODO: only add_pid if doesn't exist already
                    // TODO: add timeout after which pid should be deleted from GPU
                    for (auto& gpu : gpus.available_gpus) {
                        gpu->add_pid(pid);

                        if (FDInfo* ptr = dynamic_cast<FDInfo*>(gpu.get()))
                            ptr->fdinfo.add_pid(pid);
                    }

                    mangohud_message msg = {};

                    // TODO: move this somewhere else so it's not called bazillion times
                    for (auto& gpu : gpus.available_gpus) {
                        msg.gpus[msg.num_of_gpus].process_metrics = gpu->get_process_metrics(pid);
                        msg.gpus[msg.num_of_gpus].system_metrics = gpu->get_system_metrics();
                        msg.num_of_gpus++;
                    }

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

    return 0;
}
