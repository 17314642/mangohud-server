#pragma once

struct gpu_metrics_process_t {
    int     load;
    float   vram_used;
    float   gtt_used;
};

struct gpu_metrics_system_t {
    int     load;

    float   vram_used;
    float   gtt_used;
    float   memory_total;
    int     memory_clock;
    int     memory_temp;

    int     temperature;
    int     junction_temperature;

    int     core_clock;
    int     voltage;

    float   power_usage;
    float   power_limit;

    float   apu_cpu_power;
    int     apu_cpu_temp;

    bool    is_power_throttled;
    bool    is_current_throttled;
    bool    is_temp_throttled;
    bool    is_other_throttled;

    int     fan_speed;
    bool    fan_rpm;
};

struct gpu_t {
    bool is_active;

    gpu_metrics_process_t process_metrics;
    gpu_metrics_system_t system_metrics;
};

struct memory_t {
    float used      = 0;
    float total     = 0;
    float swap_used = 0;

    float process_resident  = 0;
    float process_shared    = 0;
    float process_virtual   = 0;
};

struct mangohud_message {
    uint8_t num_of_gpus;
    gpu_t gpus[8];
    memory_t memory;
};
