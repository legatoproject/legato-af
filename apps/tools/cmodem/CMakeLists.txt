#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

mkexe(  cm
            cm
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/interfaces/dataConnectionService
# -c ${CMAKE_CURRENT_SOURCE_DIR}/components
            -v
        DEPENDS
            cm/cm_common.c
            cm/cm_common.h
            cm/cm_data.c
            cm/cm_data.h
            cm/cm_mrc.c
            cm/cm_mrc.h
            cm/cmodem.c
            cm/cm_sim.c
            cm/cm_sim.h
            cm/Component.cdef
            ${LEGATO_ROOT}/interfaces/modemServices/le_mrc.api
            ${LEGATO_ROOT}/interfaces/modemServices/le_sim.api
            ${LEGATO_ROOT}/interfaces/dataConnectionService/le_data.api
     )
