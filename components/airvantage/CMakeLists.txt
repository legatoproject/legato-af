
# Copyright (c) 2014, Sierra Wireless Inc.
# Use of this work is subject to license.

# This directory contains components that are used by the Air Vantage agent.
# These components need to be built before the Air Vantage agent is built.

# TODO: Delete these after better integrating the Air Vantage Agent build with
#       the Legato application framework.

mkcomp( le_data_client
            -i ${LEGATO_ROOT}/interfaces/dataConnectionService
        DEPENDS
            le_data_client/clientMain.c
            le_data_client/Component.cdef
            ${LEGATO_SOURCE_DIR}/interfaces/dataConnectionService/le_data.api
        )

mkcomp( le_tree_hdlr
            -i ${LEGATO_SOURCE_DIR}/airvantage/libs/common
            -i ${LEGATO_SOURCE_DIR}/airvantage/agent/agent/treemgr/handlers/extvars
            -i ${LEGATO_SOURCE_DIR}/interfaces/modemServices
            -i ${LEGATO_SOURCE_DIR}/interfaces/positioning
        DEPENDS
            le_tree_hdlr/Component.cdef
            le_tree_hdlr/extvars.c
            ${LEGATO_SOURCE_DIR}/interfaces/modemServices/le_info.api
            ${LEGATO_SOURCE_DIR}/interfaces/modemServices/le_mdc.api
            ${LEGATO_SOURCE_DIR}/interfaces/modemServices/le_mrc.api
            ${LEGATO_SOURCE_DIR}/interfaces/modemServices/le_sim.api
            ${LEGATO_SOURCE_DIR}/interfaces/positioning/le_pos.api
            ${LEGATO_SOURCE_DIR}/interfaces/positioning/le_posCtrl.api
        )
