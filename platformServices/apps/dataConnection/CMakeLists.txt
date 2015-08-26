#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

#
# Build the Data Connection Service app
#



mkapp(  dataConnectionService.adef
            -i ${LEGATO_ROOT}/interfaces/dataConnectionService
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/interfaces/cellNetService
            -i ${LEGATO_ROOT}/components/cfgEntries
        DEPENDS
            ${LEGATO_ROOT}/components/dataConnectionService/Component.cdef
            ${LEGATO_ROOT}/components/dataConnectionService/dcsServer.c
            ${LEGATO_ROOT}/interfaces/dataConnectionService/le_data.api
            ${LEGATO_ROOT}/interfaces/cellNetService/le_cellnet.api
            ${LEGATO_ROOT}/interfaces/modemServices/le_mdc.api
            ${LEGATO_ROOT}/interfaces/modemServices/le_mrc.api
            ${LEGATO_ROOT}/interfaces/modemServices/le_sim.api
        )


