#!/bin/bash -xe

set -o pipefail

rm -rf lighttpd

cp -R "${LEGATO_ROOT}/3rdParty/lighttpd/" lighttpd

INSTALL_DIR="${LEGATO_BUILD}/3rdParty/lighttpd/"

cd lighttpd
export CCFLAGS="-DDEBUG"

./autogen.sh

./configure --host="$($CC -dumpmachine)" \
            --target="${TOOLCHAIN_PREFIX%-}" \
            --prefix="${INSTALL_DIR}" \
            --enable-static \
            --enable-shared \
            --without-bzip2 \
            --without-pcre \
            --with-zlib \
            --with-openssl \
            --with-openssl-libs=/usr/lib
if ! make > build.log; then
    tail -30 build.log
fi

make install

