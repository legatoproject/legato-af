# --------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

find_package(Legato REQUIRED)

mkexe(  smackApiTest
            ../smackAPI
            -i ${LEGATO_ROOT}/framework/c/src/security
            -i ${LEGATO_ROOT}/framework/c/src/serviceDirectory
            -i ${LEGATO_ROOT}/framework/c/src
            -o ${EXECUTABLE_OUTPUT_PATH}/smackApiTest
            ${CFLAGS_TARGET_IMPORTS}
        DEPENDS
            smackApiTest.c
            ${LEGATO_ROOT}/framework/c/src/security/smack.c
        )
