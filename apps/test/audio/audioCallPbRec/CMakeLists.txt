#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

if( (LEGATO_COMPONENTS_AUDIO MATCHES "ar7") OR (LEGATO_COMPONENTS_AUDIO MATCHES "ar86") )

    # Creates application from audioCallPbRecApp.adef
    mkapp(audioCallPbRecApp.adef
        -i ${LEGATO_ROOT}/interfaces/modemServices
    )

endif()
