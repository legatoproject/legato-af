#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
