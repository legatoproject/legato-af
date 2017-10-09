#!/bin/sh -x
make -f ${LEGATO_ROOT}/3rdParty/tinycbor/Makefile CC=${CC} CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}"
