#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

if ( (LEGATO_TARGET MATCHES "ar7") OR (LEGATO_TARGET MATCHES "ar86") )
    mkapp(voicePromptMcc.adef
        -i ${LEGATO_ROOT}/interfaces/modemServices
    )

    # This is a C test
    add_dependencies(tests_c voicePromptMcc)

endif()
