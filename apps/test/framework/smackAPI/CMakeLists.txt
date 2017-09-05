# --------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc.
# --------------------------------------------------------------------------------------------------

mkexe(  smackApiTest
            ../smackAPI
            -i ${LEGATO_ROOT}/framework/daemons/linux/serviceDirectory
            -i ${LEGATO_ROOT}/framework/liblegato
            -i ${LEGATO_ROOT}/framework/liblegato/linux
            -o ${EXECUTABLE_OUTPUT_PATH}/smackApiTest
        )

# This is a C test
add_dependencies(tests_c smackApiTest)
