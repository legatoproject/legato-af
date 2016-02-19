#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

if( (LEGATO_COMPONENTS_AUDIO MATCHES "ar7") )

    # Creates application from audioTest.adef
    mkapp(audioTest.adef
            -i ${LEGATO_ROOT}/interfaces/modemServices
    )

endif()
