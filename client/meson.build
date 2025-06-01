inc_common = [
    include_directories('include'),
]

vkh_sp = subproject('vulkan-headers')
vk_api_xml = vkh_sp.get_variable('vulkan_api_xml')
dep_vulkan = vkh_sp.get_variable('vulkan_headers_dep')

prog_python = import('python').find_installation('python3', modules: ['mako'])

vk_enum_to_str = custom_target(
    'vk_enum_to_str',
    input : ['gen_enum_to_str.py', vk_api_xml],
    output : ['vk_enum_to_str.c', 'vk_enum_to_str.h'],
    command : [
        prog_python, '@INPUT0@', '--xml', '@INPUT1@',
        '--outdir', meson.current_build_dir()
    ],
)

imgui_options = [
    'default_library=static',
    'werror=false',
    # use 'auto_features=disabled' once available: https://github.com/mesonbuild/meson/issues/5320
    'dx9=disabled',
    'dx10=disabled',
    'dx11=disabled',
    'dx12=disabled',
    'metal=disabled',
    'opengl=disabled',
    'vulkan=disabled',
    'glfw=disabled',
    'sdl2=disabled',
    'osx=disabled',
    'win=disabled',
    'allegro5=disabled',
    'webgpu=disabled',
    'sdl_renderer=disabled'
]

dearimgui_dep = dependency('imgui', fallback: ['imgui'], required: true, default_options: imgui_options)

subdir('src')
