#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

mkapp(      fwupdateTest.adef
            -i ${LEGATO_ROOT}/interfaces/fwupdate
)

# This test application can only run on dual system modules.
# Therefore, it can be ran currently only on ar758x and ar759x targets.
if((LEGATO_TARGET MATCHES "ar758x") OR (LEGATO_TARGET MATCHES "ar759x"))
    mkapp(      fwupdateStatusTest.adef
                -i ${LEGATO_ROOT}/interfaces/fwupdate
    )
endif()

# This test application can only run on dual system modules.
# Therefore, it can be ran currently only on ar758x and ar759x targets.
if((LEGATO_TARGET MATCHES "ar758x") OR (LEGATO_TARGET MATCHES "ar759x"))
    mkapp(      flashTest.adef
                -i ${LEGATO_ROOT}/interfaces/flash
    )
endif()

# This is a C test
add_dependencies(tests_c fwupdateTest)
add_dependencies(tests_c flashTest)
