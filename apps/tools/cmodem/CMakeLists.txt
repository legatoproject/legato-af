#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# TODO: Convert this into a "command-line app" when that feature is available.

mkexe(  cm
            cm
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/interfaces/dataConnectionService
            -v
        DEPENDS
            cm/*
            ${LEGATO_ROOT}/interfaces/modemServices/*
            ${LEGATO_ROOT}/interfaces/dataConnectionService/le_data.api
     )
