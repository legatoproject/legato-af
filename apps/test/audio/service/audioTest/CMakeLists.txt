#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

if( (LEGATO_COMPONENTS_AUDIO MATCHES "ar7") )

    # Creates application from audioTest.adef
    mkapp(audioTest.adef
            -i ${LEGATO_ROOT}/interfaces/modemServices
    )

    # This is a C test
    add_dependencies(tests_c audioTest)

endif()
