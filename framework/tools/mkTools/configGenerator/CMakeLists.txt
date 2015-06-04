# --------------------------------------------------------------------------------------------------
#  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

add_library(configGenerator
                configGenerator.cpp
           )

add_dependencies(configGenerator
                    PrecompiledHeader
                )

