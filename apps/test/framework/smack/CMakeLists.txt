#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

mkexe(  rogue
            rogue.c
            -o ${EXECUTABLE_OUTPUT_PATH}/rogue
            -i ${LEGATO_ROOT}/framework/c/src
     )

# This is a C test
add_dependencies(tests_c rogue)
