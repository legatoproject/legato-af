#**********************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#**********************************************************************************

project(SUPERVISOR C)

find_package(Legato REQUIRED)

mkexe(  supervisor
            ../supervisor
            -i ${LEGATO_ROOT}/interfaces/supervisor
            -i appSmack
            -i ..
            -i ../serviceDirectory
            --cflags=-DNO_LOG_CONTROL
            --cflags=-DLEGATO_FRAMEWORK_NICE_LEVEL=${LEGATO_FRAMEWORK_NICE_LEVEL}
            ${CFLAGS_TARGET_IMPORTS}
        DEPENDS
            ./*
            ${LEGATO_ROOT}/interfaces/supervisor/*
            ${LEGATO_ROOT}/interfaces/le_cfg.api
            appSmack/*
        )
