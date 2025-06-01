#pragma once

#include <string>
#include <spdlog/spdlog.h>

#define LOG_UNIX_ERRNO_DEBUG(...) log_unix_errno(log_unix_errno_lvl::debug, __VA_ARGS__)
#define LOG_UNIX_ERRNO_WARN(...)  log_unix_errno(log_unix_errno_lvl::warn , __VA_ARGS__)
#define LOG_UNIX_ERRNO_ERROR(...) log_unix_errno(log_unix_errno_lvl::error, __VA_ARGS__)

enum log_unix_errno_lvl {
    debug = 0,
    warn,
    error
};

template <class... Args>
void log_unix_errno(log_unix_errno_lvl lvl, std::string fmt, Args&&... a) {
    fmt += " Error code: {} ({})";
    switch (lvl) {
        case debug:
            SPDLOG_DEBUG(fmt, std::forward<Args>(a)..., errno, strerror(errno));
            break;

        case warn:
            SPDLOG_WARN(fmt, std::forward<Args>(a)..., errno, strerror(errno));
            break;

        case error:
            SPDLOG_ERROR(fmt, std::forward<Args>(a)..., errno, strerror(errno));
            break;

    }
}
