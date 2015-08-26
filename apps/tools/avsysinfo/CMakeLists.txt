#***************************************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#***************************************************************************************************

find_package(Legato REQUIRED)

mkapp(  avsysinfo.adef
            -i ${LEGATO_ROOT}/framework/c/src
            -i ${PROJECT_SOURCE_DIR}/interfaces/airVantage/
        DEPENDS
            avSysInfo/sysInfo.c
            ${PROJECT_SOURCE_DIR}/interfaces/airVantage/le_avdata.api
            avSysInfo/Component.cdef
        )
