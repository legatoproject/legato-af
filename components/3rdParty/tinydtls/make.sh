#!/bin/sh -x
ac_cv_func_malloc_0_nonnull=yes  ./configure --prefix="${LEGATO_BUILD}/3rdParty/tinydtls" --host="$($CC -dumpmachine)" CFLAGS="-w -std=gnu99 -DWITH_SHA256 -I${LEGATO_ROOT}/3rdParty/Lwm2mCore/include/platform-specific/linux ${CFLAGS}" LDFLAGS="${LDFLAGS}"
make
make install
