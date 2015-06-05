#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

if( (LEGATO_COMPONENTS_AUDIO MATCHES "wp7") OR (LEGATO_COMPONENTS_AUDIO MATCHES "ar7") )

    # Creates application from audioMemoryTest.adef
    mkapp(audioMemoryTest.adef
            -i ${LEGATO_ROOT}/interfaces/audio
        DEPENDS
            legato
    )

endif()
