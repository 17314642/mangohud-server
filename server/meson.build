src = [
    'main.cpp',
    'api.cpp',

    '../common/helpers.cpp',
    '../common/socket.cpp',

    'cpu/cpu.cpp',
    'cpu/power/rapl.cpp',
    'cpu/power/zenpower.cpp',
    'cpu/power/zenergy.cpp',

    'memory.cpp',
    'fdinfo.cpp',
    'gpu.cpp',
    'hwmon.cpp',
    'iostats.cpp',

    'intel/i915/i915.cpp',
    'intel/i915/i915_drm.cpp',
    'intel/xe/xe.cpp',
    'intel/xe/xe_drm.cpp',

    'amdgpu/amdgpu.cpp',
    'amdgpu/gpu_metrics.cpp',

    'nvidia/nvidia.cpp',
    'nvidia/nvml_loader.cpp',

    'panfrost.cpp',

    'msm/dpu.cpp',
    'msm/kgsl.cpp'
]

libdrm_dep = dependency('libdrm')
libcap_dep = dependency('libcap')

executable(
    'mangohud-server', src,
    dependencies: [spdlog_dep, libdrm_dep, libcap_dep, json_dep], install: true
)
