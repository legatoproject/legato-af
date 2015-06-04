# --------------------------------------------------------------------------------------------------
#  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

add_library(codeGenerator
                codeGenerator.cpp
           )

add_dependencies(codeGenerator
                    PrecompiledHeader
                )

