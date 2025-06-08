#### Dependencies
1. C and C++ compiler
2. Meson
3. Ninja
4. `apt install python3-mako glslang-tools libdrm-dev`

#### Compilation

`meson setup build && ninja -C build`

#### Installation
`sudo ninja -C build install`

#### Running
1. `mangohud_server`
2. `MANGOHUD_2=1 any_vulkan_app`
