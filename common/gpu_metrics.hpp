#pragma once

struct gpu_metrics_process {
    int     load;
    float   vram_used;
    float   gtt_used;
};

struct gpu_metrics_system {
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

struct gpu {
    bool is_active;

    gpu_metrics_process process_metrics;
    gpu_metrics_system system_metrics;
};

struct mangohud_message {
    uint8_t num_of_gpus;
    gpu gpus[8];
};
