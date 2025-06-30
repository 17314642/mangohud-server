#pragma once

#include <map>
#include <fstream>
#include <chrono>
#include <cstdint>
#include <sys/types.h>

#include "../common/gpu_metrics.hpp"

class IOStats {
private:
    struct _io_stats {
        float read_mb_per_sec = 0.f;
        float write_mb_per_sec = 0.f;

        uint64_t previous_read_bytes = 0;
        uint64_t previous_write_bytes = 0;

        std::chrono::time_point<std::chrono::steady_clock> last_update;
        std::ifstream stream;
    };

    std::map<pid_t, _io_stats> pids;

    void poll_pid(pid_t pid);

public:
    void add_pid(pid_t pid);
    void poll();
    io_stats_t get_stats(pid_t pid);
};
