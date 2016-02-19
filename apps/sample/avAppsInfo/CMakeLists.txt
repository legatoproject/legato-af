#***************************************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#***************************************************************************************************

mkapp(  avAppsInfo.adef
            -i ${LEGATO_ROOT}/framework/c/src
            -i ${PROJECT_SOURCE_DIR}/interfaces/airVantage/
            -i ${PROJECT_SOURCE_DIR}/interfaces/supervisor/
            -i ${PROJECT_SOURCE_DIR}/framework/c/src/
        )

# This is a sample application
add_dependencies(samples avAppsInfo)
