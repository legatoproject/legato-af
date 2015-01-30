#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

#
# Build the Data Connection Service app
#

mkapp(  powerMgr.adef
            -i ${LEGATO_ROOT}/interfaces/powerMgr
            -i ${LEGATO_ROOT}/interfaces/dataConnectionService
        DEPENDS
            ${LEGATO_ROOT}/components/powerMgr/Component.cdef
            ${LEGATO_ROOT}/components/powerMgr/le_pm.c
            ${LEGATO_ROOT}/interfaces/powerMgr/le_pm.api
            ${LEGATO_ROOT}/interfaces/dataConnectionService/le_data.api
        )

