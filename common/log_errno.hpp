#pragma once

#include <string>
#include <spdlog/spdlog.h>

#define LOG_UNIX_ERRNO_GENERIC(lvl, ...) log_unix_errno(lvl, __FILE__, __LINE__, SPDLOG_FUNCTION, __VA_ARGS__)
#define LOG_UNIX_ERRNO_DEBUG(...) LOG_UNIX_ERRNO_GENERIC(spdlog::level::debug, __VA_ARGS__)
#define LOG_UNIX_ERRNO_WARN(...)  LOG_UNIX_ERRNO_GENERIC(spdlog::level::warn , __VA_ARGS__)
#define LOG_UNIX_ERRNO_ERROR(...) LOG_UNIX_ERRNO_GENERIC(spdlog::level::err, __VA_ARGS__)

template <class... Args>
void log_unix_errno(
    spdlog::level::level_enum lvl,
    const char* filename_in, int line_in, const char* funcname_in,
    std::string fmt, Args&&... a
) {
    fmt += " Error code: {} ({})";

    spdlog::default_logger_raw()->log(
        spdlog::source_loc{filename_in, line_in, funcname_in},
        lvl,
        fmt,
        std::forward<Args>(a)...,
        errno,
        strerror(errno)
    );
}
