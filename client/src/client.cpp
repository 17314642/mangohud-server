#include <chrono>
#include <thread>
#include <spdlog/spdlog.h>

#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>

#include "../../common/socket.hpp"
#include "client.h"

using namespace std::chrono_literals;

std::mutex metrics_lock;
mangohud_message metrics = {};

bool create_socket(int& sock) {
    sock = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);

    if (sock < 0) {
        LOG_UNIX_ERRNO_ERROR("Couldn't create socket.");
        return false;
    }

    SPDLOG_DEBUG("Socket created.");
    return true;
}

bool connect_to_socket(const int sock, const sockaddr_un& addr) {
    int ret = connect(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(sockaddr_un));

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("Failed to connect to server socket.");
        return false;
    }

    SPDLOG_INFO("Connected to server");
    return true;
}

void poll_server(pollfd& server_poll, bool& connected, int sock) {
    mangohud_message msg = {};
    send_message(server_poll.fd, msg);

    int ret = poll(&server_poll, 1, 1000);

    if (ret < 0) {
        LOG_UNIX_ERRNO_ERROR("poll() failed.");
        return;
    }

    // no new data
    if (ret < 1)
        return;

    // Check events
    if (server_poll.revents == 0)
        return;

    SPDLOG_TRACE("fd {}: revents = {}", server_poll.fd, server_poll.revents);

    if (server_poll.revents & POLLHUP || server_poll.revents & POLLNVAL) {
        connected = false;
        
        {
            std::unique_lock lock(metrics_lock);
            metrics = {};
        }

        close(sock);
        return;
    }

    if (!receive_message(server_poll.fd, msg)) {
        SPDLOG_DEBUG("Failed to receive message from server.");
        return;
    }

    {
        std::unique_lock lock(metrics_lock);
        metrics = msg;
    }
}

void client_thread () {
    std::string socket_path = get_socket_path();

    if (socket_path.empty())
        return; 

    SPDLOG_INFO("Socket path: {}", socket_path);

    const sockaddr_un addr = {
        .sun_family = AF_UNIX
    };

    std::strncpy(const_cast<char*>(addr.sun_path), socket_path.c_str(), socket_path.size());

    int sock = 0;
    pollfd server_poll = { .fd = sock, .events = POLLIN };

    std::chrono::time_point<std::chrono::steady_clock> previous_time;

    bool connected = false;

    while (true) {
        if (!connected) {
            SPDLOG_WARN("Lost connection to server. Retrying...");

            if (!create_socket(sock)) {
                std::this_thread::sleep_for(1s);
                continue;
            }

            if (!connect_to_socket(sock, addr)) {
                close(sock);
                std::this_thread::sleep_for(1s);
                continue;
            }

            server_poll.fd = sock;
            connected = true;
        }

        poll_server(server_poll, connected, sock);
        std::this_thread::sleep_for(1s);
    }

    return;
}

void init_client() {
    spdlog::set_level(spdlog::level::level_enum::debug);

    SPDLOG_DEBUG("init_client()");

    std::thread t = std::thread(&client_thread);
    t.detach();
}
