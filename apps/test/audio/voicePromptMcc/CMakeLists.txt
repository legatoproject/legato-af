#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

if ( (LEGATO_TARGET MATCHES "ar7") OR (LEGATO_TARGET MATCHES "ar86") )
    mkapp(voicePromptMcc.adef
        -i ${LEGATO_ROOT}/interfaces/modemServices
    )
endif()
