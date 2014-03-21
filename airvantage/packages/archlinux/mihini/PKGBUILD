# Maintainer: Guilhem Saurel <gsaurel@sierrawireless.com>
pkgname=mihini-git
pkgver=20130626
pkgrel=1
pkgdesc="Open Source framework for M2M"
arch=('any')
url="http://www.eclipse.org/mihini/"
license=('EPL')
makedepends=('git' 'cmake')
optdepends=('telnet')
install='mihini.install'
source=("mihini.service")
md5sums=("c9256985ffbf751d76d86e3bf8b4f059")
_gitroot=http://git.eclipse.org/gitroot/mihini/org.eclipse.mihini.git
_gitname=mihini-repo

build() {
  cd "$srcdir"
  msg "Connecting to GIT server…"

  if [[ -d "$_gitname" ]]; then
    cd "$_gitname" && git pull origin
    msg "The local files are updated."
  else
    git clone "$_gitroot" "$_gitname"
  fi

  msg "Git checkout done or server timeout"
  msg "Starting build…"

  rm -rf "$srcdir/$_gitname-build"
  git clone "$srcdir/$_gitname" "$srcdir/$_gitname-build"
  cd "$srcdir/$_gitname-build"

  ./bin/build.sh
  cd build.default
  make lua luac modbus_serial
}

package() {
  msg "Creating systemd's .service file"
  sudo cp mihini.service /etc/systemd/system

  cd "$srcdir/$_gitname-build/build.default"
  sudo mv runtime /opt/mihini
}

# vim:set ts=2 sw=2 et:
