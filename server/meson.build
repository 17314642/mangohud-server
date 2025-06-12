src = [
    'fdinfo.cpp',
    'gpu.cpp',
    'hwmon.cpp',
    'intel/i915/i915.cpp',
    'intel/i915/i915_drm.cpp',
    'intel/xe/xe.cpp',
    'intel/xe/xe_drm.cpp',
    'amdgpu/amdgpu.cpp',
    'amdgpu/gpu_metrics.cpp',
    'nvidia/nvidia.cpp',
    'nvidia/nvml_loader.cpp',
    'main.cpp',
    '../common/helpers.cpp',
    '../common/socket.cpp'
]

executable('mangohud-server', src, dependencies: spdlog_dep, install: true)
