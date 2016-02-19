#***************************************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#***************************************************************************************************

mkapp(  avSystemInfo.adef
            -i ${LEGATO_ROOT}/framework/c/src
            -i ${PROJECT_SOURCE_DIR}/interfaces/airVantage/
        )

# This is a sample application
add_dependencies(samples avSystemInfo)
