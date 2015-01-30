#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# Use of this work is subject to license.
#*******************************************************************************

find_package(Legato REQUIRED)

set(CELL_NET_COMPONENT_DIR ${LEGATO_ROOT}/components/cellNetService)

mkapp(  cellNetService.adef
            -i ${LEGATO_ROOT}/interfaces/cellNetService
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/components/cfgEntries
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${CELL_NET_COMPONENT_DIR}/Component.cdef
            ${CELL_NET_COMPONENT_DIR}/cellNetServiceServer.c
            ${LEGATO_ROOT}/interfaces/cellNetService/le_cellnet.api
            ${LEGATO_ROOT}/interfaces/modemServices/le_sim.api
            ${LEGATO_ROOT}/interfaces/modemServices/le_mrc.api
            ${LEGATO_ROOT}/interfaces/le_cfg.api
            ${LEGATO_ROOT}/components/cfgEntries/mdmCfgEntries.h
        )
