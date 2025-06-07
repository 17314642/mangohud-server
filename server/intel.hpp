#pragma once

#include <cstdint>

#include "gpu.hpp"
#include "hwmon.hpp"
#include "fdinfo.hpp"

class Intel : public GPU, private Hwmon, public FDInfo {
private:
    const std::vector<hwmon_sensor> sensors = {
        { "voltage"     , "in0_input"       },
        { "fan_speed"   , "fan1_input"      },
        { "temp"        , "temp1_input"     },
        { "energy"      , "energy1_input"   },
        { "power_limit" , "power1_max"      },
    };

    uint64_t previous_power_usage = 0;
    std::map<pid_t, uint64_t> previous_gpu_times;

protected:
    void pre_poll_overrides() override;

public:
    Intel(
        const std::string& drm_node, const std::string& pci_dev,
        uint16_t vendor_id, uint16_t device_id
    );

    // System-related functions
    // int     get_load() override;                 // Not available

    // float   get_vram_used() override;            // Available only with root
    // float   get_gtt_used() override;             // Not available
    // float   get_memory_total() override;         // TODO via DRM uAPI
    // int     get_memory_clock() override;         // Not available
    // int     get_memory_temp() override;          // TODO

    int     get_temperature() override;
    // int     get_junction_temperature() override; // Not available

    // int     get_core_clock() override;           // TODO
    int     get_voltage() override;

    float   get_power_usage() override;
    float   get_power_limit() override;

    // float   get_apu_cpu_power() override;        // Investigate
    // int     get_apu_cpu_temp() override;         // Investigate

    // bool    get_is_power_throttled() override;   // TODO
    // bool    get_is_current_throttled() override; // TODO
    // bool    get_is_temp_throttled() override;    // TODO
    // bool    get_is_other_throttled() override;   // TODO

    int     get_fan_speed() override;

    // Process-related functions
    int     get_process_load(pid_t pid) override;
    float   get_process_vram_used(pid_t pid) override;
    float   get_process_gtt_used(pid_t pid) override;
};
