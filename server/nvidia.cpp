#include "nvidia.hpp"

Nvidia::Nvidia(
    const std::string &drm_node, const std::string &pci_dev,
    uint16_t vendor_id, uint16_t device_id)
    :
    GPU(drm_node, pci_dev, vendor_id, device_id, "gpu-nvidia"
) {
    nvml_available = init_nvml(pci_dev);
}

bool Nvidia::init_nvml(const std::string& pci_dev) {
    nvml = get_libnvml_loader();

    if (!nvml)
        return false;

    if (!nvml->IsLoaded())
        return false;

    nvmlReturn_t result = nvml->nvmlInit();
    
    if (NVML_SUCCESS != result) {
        SPDLOG_ERROR("Nvidia module initialization failed: {}", nvml->nvmlErrorString(result));
        return false;
    }

    result = nvml->nvmlDeviceGetHandleByPciBusId(pci_dev.c_str(), &device);
    
    if (NVML_SUCCESS != result) {
        SPDLOG_ERROR("Getting device handle by PCI bus ID failed: {}", nvml->nvmlErrorString(result));
        return false;
    }

    return true;
}

Nvidia::nvml_proc_info Nvidia::get_processes() {
    unsigned int info_count = 0;

    nvmlProcessInfo_t* cur_process_info = new nvmlProcessInfo_t[info_count];
    nvmlReturn_t ret = nvml->nvmlDeviceGetGraphicsRunningProcesses(device, &info_count, cur_process_info);

    delete[] cur_process_info;

    if (ret != NVML_ERROR_INSUFFICIENT_SIZE)
        return { nullptr, 0 };

    cur_process_info = new nvmlProcessInfo_t[info_count];
    ret = nvml->nvmlDeviceGetGraphicsRunningProcesses(device, &info_count, cur_process_info);

    if (ret != NVML_SUCCESS) {
        delete[] cur_process_info;
        return { nullptr, 0 };
    }

    return { cur_process_info, info_count };
}

int Nvidia::get_load() {
    nvmlUtilization_st nvml_utilization;

    nvmlReturn_t ret = nvml->nvmlDeviceGetUtilizationRates(device, &nvml_utilization);

    if (ret != NVML_SUCCESS)
        return 0;

    return nvml_utilization.gpu;
}

float Nvidia::get_vram_used() {
    nvmlMemory_st nvml_memory;
    
    nvmlReturn_t ret = nvml->nvmlDeviceGetMemoryInfo(device, &nvml_memory);

    if (ret != NVML_SUCCESS)
        return 0;
    
    return nvml_memory.used / 1024.f / 1024.f / 1024.f;
}

float Nvidia::get_memory_total() {
    nvmlMemory_st nvml_memory;
    
    nvmlReturn_t ret = nvml->nvmlDeviceGetMemoryInfo(device, &nvml_memory);

    if (ret != NVML_SUCCESS)
        return 0;
    
    return nvml_memory.total / 1024.f / 1024.f / 1024.f;
}

int Nvidia::get_memory_clock() {
    uint32_t memory_clock;
    
    nvmlReturn_t ret = nvml->nvmlDeviceGetClockInfo(device, NVML_CLOCK_MEM, &memory_clock);

    if (ret != NVML_SUCCESS)
        return 0;
    
    return memory_clock;
}

int Nvidia::get_temperature() {
    uint32_t temperature = 0;
    
    nvmlReturn_t ret = nvml->nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temperature);

    if (ret != NVML_SUCCESS)
        return 0;
    
    return temperature;
}

int Nvidia::get_core_clock() {
    uint32_t core_clock = 0;
    
    nvmlReturn_t ret = nvml->nvmlDeviceGetClockInfo(device, NVML_CLOCK_GRAPHICS, &core_clock);

    if (ret != NVML_SUCCESS)
        return 0;
    
    return core_clock;
}

float Nvidia::get_power_usage() {
    uint32_t power_usage = 0;
    
    nvmlReturn_t ret = nvml->nvmlDeviceGetPowerUsage(device, &power_usage);

    if (ret != NVML_SUCCESS)
        return 0;
    
    return power_usage / 1000.f;
}

float Nvidia::get_power_limit() {
    uint32_t power_limit = 0;
    
    nvmlReturn_t ret = nvml->nvmlDeviceGetPowerManagementLimit(device, &power_limit);

    if (ret != NVML_SUCCESS)
        return 0;
    
    return power_limit / 1000.f;
}

bool Nvidia::get_is_power_throttled() {
    unsigned long long throttle_reasons;
    nvmlReturn_t ret = nvml->nvmlDeviceGetCurrentClocksThrottleReasons(device, &throttle_reasons);

    if (ret != NVML_SUCCESS)
        return 0;

    return (throttle_reasons & 0x000000000000008CLL) != 0;
}

bool Nvidia::get_is_temp_throttled() {
    unsigned long long throttle_reasons;
    nvmlReturn_t ret = nvml->nvmlDeviceGetCurrentClocksThrottleReasons(device, &throttle_reasons);

    if (ret != NVML_SUCCESS)
        return 0;

    return (throttle_reasons & 0x0000000000000060LL) != 0;
}

bool Nvidia::get_is_other_throttled() {
    unsigned long long throttle_reasons;
    nvmlReturn_t ret = nvml->nvmlDeviceGetCurrentClocksThrottleReasons(device, &throttle_reasons);

    if (ret != NVML_SUCCESS)
        return 0;

    return (throttle_reasons & 0x0000000000000112LL) != 0;
}

int Nvidia::get_fan_speed() {
    uint32_t fan_speed = 0;
    
    nvmlReturn_t ret = nvml->nvmlDeviceGetFanSpeed(device, &fan_speed);

    if (ret != NVML_SUCCESS)
        return 0;
    
    return fan_speed;
}

bool Nvidia::get_fan_rpm() {
    return false;
}

float Nvidia::get_process_vram_used(pid_t pid) {
    Nvidia::nvml_proc_info info = get_processes();
    float used_memory = 0.f;

    for (size_t i = 0; i < info.second; i++) {
        if (static_cast<pid_t>(info.first[i].pid) != pid)
            continue;

        used_memory = info.first[i].usedGpuMemory / 1024.f / 1024.f / 1024.f;
        break;
    }

    delete[] info.first;

    return used_memory;
}
