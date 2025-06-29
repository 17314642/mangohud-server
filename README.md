#### Building on Arch Linux
1. Download PKGBUILD
2. `makepkg -si`

#### Dependencies
1. C++ compiler
2. Meson
3. Ninja
4. For compiling: `apt install libdrm-dev libcap-dev`
5. For running: `apt install libdrm libcap`

#### Compilation

`meson setup builddir && ninja -C builddir`

#### Installation
`sudo ninja -C builddir install`

#### Running
1. `mangohud-server`
