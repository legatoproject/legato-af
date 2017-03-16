#***************************************************************************************************
# Copyright (C) Sierra Wireless Inc.
#***************************************************************************************************

mkapp(  fileAtomTest.adef
            -i ${LEGATO_ROOT}/framework/c/src
        )

# This is a C test
add_dependencies(tests_c fileAtomTest)
