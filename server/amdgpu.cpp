#include "amdgpu.hpp"

AMDGPU::AMDGPU(
    const std::string& drm_node, const std::string& pci_dev,
    uint16_t vendor_id, uint16_t device_id
) : GPU(drm_node, pci_dev, vendor_id, device_id), FDInfo(drm_node) {
    pthread_setname_np(worker_thread.native_handle(), "gpu-amdgpu");

    sysfs_hwmon.base_dir = "/sys/class/drm/" + drm_node + "/device";
    sysfs_hwmon.setup(sysfs_sensors);
}

void AMDGPU::poll_overrides() {
    hwmon.poll_sensors();
    sysfs_hwmon.poll_sensors();
    fdinfo.poll_all();
}

int AMDGPU::get_load() {
    return sysfs_hwmon.get_sensor_value("load");
}

float AMDGPU::get_vram_used() {
    float used = sysfs_hwmon.get_sensor_value("vram_used") / 1024.f / 1024.f;
    return used;
}

float AMDGPU::get_gtt_used() {
    float used = sysfs_hwmon.get_sensor_value("gtt_used") / 1024.f / 1024.f;
    return used;
}

float AMDGPU::get_memory_total() {
    float used = sysfs_hwmon.get_sensor_value("vram_total") / 1024.f / 1024.f;
    return used;
}

int AMDGPU::get_temperature() {
    float temp = hwmon.get_sensor_value("temp") / 1'000.f;
    return std::round(temp);
}

int AMDGPU::get_core_clock() {
    return hwmon.get_sensor_value("frequency") / 1'000'000.f;
}

int AMDGPU::get_process_load(pid_t pid) {
    uint64_t* previous_gpu_time = &previous_gpu_times[pid];
    uint64_t gpu_time_now = fdinfo.get_gpu_time(pid, "drm-engine-gfx");

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

float AMDGPU::get_process_vram_used(pid_t pid) {
    return fdinfo.get_memory_used(pid, "drm-memory-vram");
}

float AMDGPU::get_process_gtt_used(pid_t pid) {
    return fdinfo.get_memory_used(pid, "drm-memory-gtt");
}
