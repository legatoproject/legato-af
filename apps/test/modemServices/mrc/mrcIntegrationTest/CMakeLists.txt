#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# AR7 Component Implementation
if(LEGATO_COMPONENTS_AUDIO MATCHES "ar7")
    set(AR7_ENABLE_FLAG 1)
else()
    set(AR7_ENABLE_FLAG 0)
endif()

mkapp(mrcTest.adef
     -i ${LEGATO_ROOT}/interfaces/modemServices
     --cflags=-DAR7_DETECTED=${AR7_ENABLE_FLAG}
)

# This is a C test
add_dependencies(tests_c mrcTest)
