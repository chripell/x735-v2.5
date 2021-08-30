# Maintainer: Christian Pellegrin <chripell@fsfe.org>
pkgname=supervisor
pkgver=20210830
pkgrel=1
pkgdesc="Supervisor daemon for Attimis"
arch=('aarch64')
url="https://github.com/chripell/x735-v2.5"
license=('Apache')

build() {
    cd ..
    make
}

package() {
    cd ..
    while read l; do
	FROM=`echo $l | cut -d \, -f 1`
	TO=`echo $l | cut -d \, -f 2`
	mkdir -p $pkgdir/$TO
	cp -a $FROM $pkgdir/$TO
    done < install_list
}

pkgver() {
    date '+%Y%m%d'
}
