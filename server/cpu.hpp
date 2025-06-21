#pragma once

#include <vector>
#include <fstream>
#include "../common/gpu_metrics.hpp"

class CPU {
private:
    std::ifstream ifs_stat;
    std::ifstream ifs_cpuinfo;

    std::vector<uint64_t> prev_idle_times;
    std::vector<uint64_t> prev_total_times;

    void poll_load();
    void poll_frequency();

    std::vector<std::vector<uint64_t>> get_cpu_times();
    bool get_cpu_times(
        const std::vector<uint64_t>& cpu_times, uint64_t &idle_time, uint64_t &total_time
    );

protected:
    cpu_info_t info;
    std::vector<core_info_t> cores;

    std::chrono::time_point<std::chrono::steady_clock> previous_time;
    std::chrono::nanoseconds delta_time_ns;

public:
    CPU();

    void poll();
    virtual void pre_poll_overrides() {}
    cpu_info_t get_info();
    std::vector<core_info_t> get_core_info();
};

class CPUWithRAPL : public CPU {
private:
    std::ifstream ifs_rapl;
    uint64_t previous_usage = 0;

    float get_power_usage();

public:
    CPUWithRAPL();
    void pre_poll_overrides() override;
};
