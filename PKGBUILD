pkgname=mangohud-server
pkgver=v1
pkgrel=1
pkgdesc="MangoHud with server-client architecture. Server package."
arch=('x86_64')
url="https://github.com/17314642/mangohud-server"
makedepends=('ninja' 'meson' 'python-mako' 'glslang' 'pkgconf' 'patch' 'gcc' 'libdrm' 'libcap')
license=('MIT')
source=(
    "mangohud-server"::"git+${url}"
)

sha256sums=(
    'SKIP'
)

build() {
    cd "$srcdir/$pkgname"
    meson setup builddir
    ninja -C builddir
}

package() {
    cd "$srcdir/$pkgname"
    DESTDIR="$pkgdir/" ninja -C builddir install
}
