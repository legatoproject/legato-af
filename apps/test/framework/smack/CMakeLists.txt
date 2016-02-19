#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

mkexe(  rogue
            rogue.c
            -o ${EXECUTABLE_OUTPUT_PATH}/rogue
            -i ${LEGATO_ROOT}/framework/c/src
     )
