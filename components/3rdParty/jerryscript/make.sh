#!/bin/sh -x
python ${LEGATO_ROOT}/3rdParty/jerryscript/tools/build.py \
       --jerry-libc=off                                   \
       --cmake-param="-DCMAKE_C_COMPILER=${CC}" \
       --compile-flag="-nostdlib -I${LEGATO_SYSROOT}"