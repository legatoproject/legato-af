#!/bin/bash
set -xe

# Set up directories.
LEGATO_BUILD="${LEGATO_ROOT}/build/${TARGET}"
BUILDDIR="${LEGATO_BUILD}/3rdParty/mbedtls"
LIBDIR="${LEGATO_BUILD}/3rdParty/lib"
SRCDIR="${LEGATO_ROOT}/3rdParty/mbedtls"

mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"

# Import build environment.
varname=${TARGET^^}_AR
export AR="${!varname}"
varname=${TARGET^^}_CC
export CC="${!varname}"
varname=${TARGET^^}_CFLAGS
export CFLAGS="${!varname} ${EXTRA_CFLAGS}"

# Build mbedTLS.
cmake -G Ninja -DENABLE_PROGRAMS=OFF -DENABLE_TESTING=OFF "${SRCDIR}"
ninja

# Copy the libraries to the final directory.
mkdir -p "${LIBDIR}"
for l in crypto x509 tls; do
    cp "library/libmbed${l}.a" "${LIBDIR}/"
done
