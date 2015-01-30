#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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

