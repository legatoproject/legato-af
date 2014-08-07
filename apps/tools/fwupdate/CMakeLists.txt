#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

mkexe(  fwupdate
            ../fwupdate
            -i ${PROJECT_SOURCE_DIR}/interfaces/modemServices
        DEPENDS
            legato
            ## TODO: Remove this when the mk tools do dependency checking.
            Component.cdef
            fwupdate.c
            ${PROJECT_SOURCE_DIR}/interfaces/modemServices/le_fwupdate.api
     )

