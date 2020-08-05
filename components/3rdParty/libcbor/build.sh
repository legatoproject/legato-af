#!/bin/bash
set -xe

# Set up directories.
LEGATO_BUILD="${LEGATO_ROOT}/build/${TARGET}"
BUILDDIR="${LEGATO_BUILD}/3rdParty/libcbor"
LIBDIR="${LEGATO_BUILD}/3rdParty/lib"
SRCDIR="${LEGATO_ROOT}/3rdParty/libcbor"

mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"

# Import build environment.
varname=${TARGET^^}_CFLAGS
export CMAKE_C_FLAGS="${CFLAGS} ${!varname} ${EXTRA_CFLAGS}"

# Build libcbor
cmake -DCMAKE_SYSTEM_NAME=$1 -DWITH_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release -DCBOR_CUSTOM_ALLOC=ON "${SRCDIR}"
make

# Copy the libraries to the final directory.
mkdir -p "${LIBDIR}"
cp src/libcbor.* "${LIBDIR}/"
cp cbor/configuration.h "${LEGATO_BUILD}/3rdParty/inc/cbor/"
