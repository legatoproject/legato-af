#!/bin/sh -x

# tinycbor does not handle CFLAGS externally set, so pass them through CC instead.
CC="${CC} ${CFLAGS}"

make -f ${LEGATO_ROOT}/3rdParty/tinycbor/Makefile CC="${CC}" CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}"
