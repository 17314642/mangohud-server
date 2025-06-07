pkgname=mangohud-2
pkgver=v1
pkgrel=1
pkgdesc="MangoHud with server-client architecture"
arch=('x86_64')
url="https://github.com/17314642/MangoHud-2"
makedepends=('ninja' 'meson' 'python-mako' 'glslang' 'pkgconf')
license=('MIT')
source=(
    "mangohud-2"::"git+${url}"
)

sha256sums=('SKIP')

build() {
    cd "$srcdir/$pkgname"
    meson setup build
    ninja -C build
}

package() {
    cd "$srcdir/$pkgname"
    DESTDIR="$pkgdir/" ninja -C build install
}
