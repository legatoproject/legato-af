#!/bin/bash
set -xe

# Set up directories.
LEGATO_BUILD="${LEGATO_ROOT}/build/${LEGATO_TARGET}"
BUILDDIR="${LEGATO_BUILD}/3rdParty/libcbor"
LIBDIR="${LEGATO_BUILD}/3rdParty/lib"
SRCDIR="${LEGATO_ROOT}/3rdParty/libcbor"

mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"

# Build libcbor
cmake -DCMAKE_SYSTEM_NAME=$1 -DCMAKE_TRY_COMPILE_TARGET_TYPE="STATIC_LIBRARY" -DWITH_EXAMPLES=OFF "${SRCDIR}"
make

# Copy the libraries to the final directory.
mkdir -p "${LIBDIR}"
cp src/libcbor.* "${LIBDIR}/"
cp cbor/configuration.h "${LEGATO_BUILD}/3rdParty/inc/cbor/"
