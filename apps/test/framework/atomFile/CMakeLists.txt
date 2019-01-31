#***************************************************************************************************
# Copyright (C) Sierra Wireless Inc.
#***************************************************************************************************

mkapp(  fileAtomTest.adef
            -i ${LEGATO_ROOT}/framework/liblegato
            -i ${LEGATO_ROOT}/framework/liblegato/linux
        )

# This is a C test
add_dependencies(tests_c fileAtomTest)
