#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

mkapp(      fwupdateTest.adef
            -i ${LEGATO_ROOT}/interfaces/fwupdate
)

# This is a C test
add_dependencies(tests_c fwupdateTest)

# This test application can only run on dual system modules.
# Therefore, it can be currently run only on ar758x and ar759x targets.
if((LEGATO_TARGET MATCHES "ar758x") OR (LEGATO_TARGET MATCHES "ar759x"))
    mkapp(      fwupdateStatusTest.adef
                -i ${LEGATO_ROOT}/interfaces/fwupdate
    )

    add_dependencies(tests_c fwupdateStatusTest)

    mkapp(      flashTest.adef
                -i ${LEGATO_ROOT}/interfaces/flash
    )

    add_dependencies(tests_c flashTest)

    mkapp(      flashApiTest.adef
                -i ${LEGATO_ROOT}/interfaces/flash
    )

    add_dependencies(tests_c flashApiTest)
endif()

