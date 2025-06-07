glslang = find_program('glslang', 'glslangValidator')

overlay_spv = []
overlay_shaders = [
    'overlay.frag',
    'overlay.vert',
]

foreach s : ['overlay.frag', 'overlay.vert']
    overlay_spv += custom_target(
        s + '.spv.h', input : s, output : s + '.spv.h',
        command : [glslang, '-V', '-x', '-o', '@OUTPUT@', '@INPUT@']
    )
endforeach

link_args = cpp.get_supported_link_arguments(['-Wl,-Bsymbolic-functions', '-Wl,-z,relro', '-Wl,--exclude-libs,ALL', '-static-libstdc++'])

vk_layer_static_lib = static_library(
    'vk-layer',
    files('vulkan.cpp', 'overlay.cpp', 'client.cpp', '../../common/socket.cpp'),
    vk_enum_to_str,
    overlay_spv,
    gnu_symbol_visibility : 'hidden',
    dependencies : [
        dearimgui_dep,
        spdlog_dep,
        dep_vulkan,
    ],
    include_directories : [inc_common],
    link_args : link_args
)

shared_library(
  'vk-layer',
  objects: vk_layer_static_lib.extract_all_objects(recursive: true),
  link_with: vk_layer_static_lib,
  link_args : link_args,
  install: true
)
