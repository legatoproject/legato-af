#***************************************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#***************************************************************************************************

find_package(Legato REQUIRED)

mkapp(  avappinfo.adef
            -i ${LEGATO_ROOT}/framework/c/src
            -i ${PROJECT_SOURCE_DIR}/interfaces/airVantage/
            -i ${PROJECT_SOURCE_DIR}/interfaces/supervisor/
            -i ${PROJECT_SOURCE_DIR}/framework/c/src/
        DEPENDS
            avAppInfo/appInfo.c
            ${PROJECT_SOURCE_DIR}/interfaces/airVantage/le_avdata.api
            ${PROJECT_SOURCE_DIR}/interfaces/supervisor/le_appInfo.api
            avAppInfo/Component.cdef
        )
