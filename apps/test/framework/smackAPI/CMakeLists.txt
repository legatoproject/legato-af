# --------------------------------------------------------------------------------------------------
# Copyright (C) Sierra Wireless Inc.
# --------------------------------------------------------------------------------------------------

mkexe(  smackApiTest
            ../smackAPI
            -i ${LEGATO_ROOT}/framework/c/src/serviceDirectory
            -i ${LEGATO_ROOT}/framework/c/src
            -o ${EXECUTABLE_OUTPUT_PATH}/smackApiTest
        )

# This is a C test
add_dependencies(tests_c smackApiTest)
