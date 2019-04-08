#!/bin/sh -xe

if [ x"$LE_CONFIG_LINUX" = "xy" ]; then
    PLATFORM_CFLAGS="-I${LEGATO_ROOT}/3rdParty/Lwm2mCore/include/platform-specific/linux -fPIC"

    # In case this is a Yocto toolchain, declare OECORE_NATIVE_SYSROOT as to make perl (used by autoreconf)
    # work properly.
    export OECORE_NATIVE_SYSROOT="${TOOLCHAIN_DIR}/../../.."
elif [ x"$LE_CONFIG_CUSTOM_OS" = "xy" ]; then
    PLATFORM_CFLAGS="-I${LEGATO_ROOT}/3rdParty/Lwm2mCore/include/platform-specific/rtos -D__RTOS__"
fi

cp -rf ${LEGATO_ROOT}/3rdParty/Lwm2mCore/3rdParty/tinydtls/* .
autoreconf -i
ac_cv_func_malloc_0_nonnull=yes  ./configure \
    --prefix="${LEGATO_BUILD}/3rdParty/tinydtls" \
    --host="$($CC -dumpmachine)" \
    CFLAGS="-w -std=gnu99 -DSIERRA -DWITH_SHA256 ${PLATFORM_CFLAGS} ${CFLAGS}" \
    LDFLAGS="${LDFLAGS}"
make -k
make -k install
