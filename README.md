#### Dependencies
1. C and C++ compiler
2. `apt install python3-mako glslang-tools`

#### Compilation

`meson setup build && ninja -C build`

#### Running
1. Launch server: `./build/server/mangohud-server`
2. Copy layer file: `cp mangohud-2-0.x86_64.json ~/.local/share/vulkan/implicit_layer.d/`
3. Open layer file `~/.local/share/vulkan/implicit_layer.d/mangohud-2-0.x86_64.json` in text editor
4. Replace string `ENTER_PATH_TO_LIBRARY_HERE` to your library path. Example: `/home/user/Desktop/MangoHud-2/build/client/src/libvk-layer.so`
5. Launch any vulkan app: `MANGOHUD=1 vkcube`
