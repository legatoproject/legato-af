#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

if( (LEGATO_COMPONENTS_AUDIO MATCHES "ar7") )

    # Creates application from audioMemoryTest.adef
    mkapp(audioMemoryTest.adef)

    # This is a C test
    add_dependencies(tests_c audioMemoryTest)

endif()
