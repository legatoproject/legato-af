#!/bin/sh -xe

# In case this is a Yocto toolchain, declare OECORE_NATIVE_SYSROOT as to make perl (used by autoreconf)
# work properly.
export OECORE_NATIVE_SYSROOT="${TOOLCHAIN_DIR}/../../.."

cp -rf ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/tinydtls/* .
autoreconf -i
ac_cv_func_malloc_0_nonnull=yes  ./configure \
    --prefix="${LEGATO_BUILD}/3rdParty/tinydtls" \
    --host="$($CC -dumpmachine)" \
    CFLAGS="-w -std=gnu99 -DWITH_SHA256 -I${LEGATO_ROOT}/3rdParty/Lwm2mCore/include/platform-specific/linux -fPIC ${CFLAGS}" \
    LDFLAGS="${LDFLAGS}"
make
make install
