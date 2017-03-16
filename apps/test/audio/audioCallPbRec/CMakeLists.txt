#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

if( (LEGATO_COMPONENTS_AUDIO MATCHES "ar7") OR (LEGATO_COMPONENTS_AUDIO MATCHES "ar86") )

    # Creates application from audioCallPbRecApp.adef
    mkapp(audioCallPbRecApp.adef
        -i ${LEGATO_ROOT}/interfaces/modemServices
    )

    # This is a C test
    add_dependencies(tests_c audioCallPbRecApp)

endif()
