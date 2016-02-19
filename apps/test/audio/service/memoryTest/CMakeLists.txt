#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

if( (LEGATO_COMPONENTS_AUDIO MATCHES "ar7") )

    # Creates application from audioMemoryTest.adef
    mkapp(audioMemoryTest.adef)

endif()
