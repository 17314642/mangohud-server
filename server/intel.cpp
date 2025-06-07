#include "intel.hpp"

Intel::Intel(
    const std::string& drm_node, const std::string& pci_dev,
    uint16_t vendor_id, uint16_t device_id
) : GPU(drm_node, pci_dev, vendor_id, device_id, "gpu-intel"), FDInfo(drm_node) {
    hwmon.setup(sensors, drm_node);
}

void Intel::pre_poll_overrides() {
    hwmon.poll_sensors();
    fdinfo.poll_all();
}

int Intel::get_temperature() {
    return hwmon.get_sensor_value("temp");
}

float Intel::get_power_usage() {
    uint64_t current_power_usage = hwmon.get_sensor_value("energy");

    if (!previous_power_usage) {
        previous_power_usage = current_power_usage;
        return 0;
    }

    float delta = current_power_usage - previous_power_usage;
    delta /= std::chrono::duration_cast<std::chrono::seconds>(delta_time_ns).count();

    previous_power_usage = current_power_usage;

    return delta / 1'000'000.f;
}

float Intel::get_power_limit() {
    float limit = hwmon.get_sensor_value("power_limit") / 1'000'000.f;
    return limit;
}

int Intel::get_voltage() {
    return hwmon.get_sensor_value("voltage");
}

int Intel::get_fan_speed() {
    return hwmon.get_sensor_value("fan_speed");
}

int Intel::get_process_load(pid_t pid) {
    uint64_t* previous_gpu_time = &previous_gpu_times[pid];
    uint64_t gpu_time_now = fdinfo.get_gpu_time(pid, "drm-engine-render");

    if (!*previous_gpu_time) {
        *previous_gpu_time = gpu_time_now;
        return 0;
    }

    float delta_gpu_time = gpu_time_now - *previous_gpu_time;
    float result = delta_gpu_time / delta_time_ns.count() * 100;

    if (result > 100.f)
        result = 100.f;

    *previous_gpu_time = gpu_time_now;

    return std::round(result);
}

float Intel::get_process_vram_used(pid_t pid) {
    return fdinfo.get_memory_used(pid, "drm-total-local0");
}

float Intel::get_process_gtt_used(pid_t pid) {
    return fdinfo.get_memory_used(pid, "drm-total-system0");
}
