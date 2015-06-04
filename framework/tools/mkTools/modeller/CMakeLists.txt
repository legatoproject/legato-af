# --------------------------------------------------------------------------------------------------
#  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

add_library(modeller
                componentModeller.cpp
                appModeller.cpp
                modellerCommon.cpp
           )

add_dependencies(modeller
                    PrecompiledHeader
                )

