#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

# Creates application from audioCallPbRecApp.adef
mkapp(audioCallPbRecApp.adef
    -i ${LEGATO_SOURCE_DIR}/interfaces/modemServices
    -i ${LEGATO_SOURCE_DIR}/interfaces/audio
    DEPENDS
        legato
        audioCallPbRecApp.c
)
