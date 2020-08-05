#!/bin/bash -xe

mkdir -p iperf

cd iperf

INSTALL_DIR="${LEGATO_BUILD}/3rdParty/iperf/"

${LEGATO_ROOT}/3rdParty/iperf/configure --host=$(${CC} -dumpmachine) --prefix=$INSTALL_DIR \
              --with-sysroot=${TARGET_SYSROOT} CFLAGS="-O2"

make

make install
