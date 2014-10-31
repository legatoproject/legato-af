# --------------------------------------------------------------------------------------------------
# Copyright (C) 2013-2014, Sierra Wireless Inc., Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

project(SUPERVISOR C)

find_package(Legato REQUIRED)

mkexe(  supervisor
            ../supervisor
            -i ${LEGATO_ROOT}/interfaces/supervisor
            -i ..
            -i ../security
            -i ../serviceDirectory
            --cflags=-DNO_LOG_CONTROL
            --cflags=-DLEGATO_FRAMEWORK_NICE_LEVEL=${LEGATO_FRAMEWORK_NICE_LEVEL}
            --cflags=-DLE_RUNTIME_DIR='"${LE_RUNTIME_DIR}"'
        DEPENDS
            ../security/*
            ./*
            ${LEGATO_ROOT}/interfaces/supervisor/le_sup_ctrl.api
            ${LEGATO_ROOT}/interfaces/supervisor/le_sup_state.api
            ${LEGATO_ROOT}/interfaces/supervisor/le_sup_wdog.api
        )
