#***************************************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#***************************************************************************************************

set(JANSSON_INC_DIR "${CMAKE_BINARY_DIR}/3rdParty/jansson-2.6/include/")

mkexe(  updateDaemon
            ../updateDaemon
            -i ..
            -i  ${LEGATO_ROOT}/interfaces/updateDaemon/
            -i ${JANSSON_INC_DIR}
            -i ${LEGATO_ROOT}/interfaces/fwupdate
            -i ${LEGATO_ROOT}/framework/c/src/appCfg
            -s ${LEGATO_ROOT}/framework/c/src
            -i instStat
            --ldflags=-ljansson
        DEPENDS
            updateDaemon.c
            manifest.c
            ${LEGATO_ROOT}/interfaces/updateDaemon/le_update.api
            ${LEGATO_ROOT}/interfaces/updateDaemon/le_instStat.api
            ${LEGATO_ROOT}/interfaces/fwupdate/le_fwupdate.api
            Component.cdef
            ${LEGATO_ROOT}/framework/c/src/appCfg/*
            instStat/*
        )
