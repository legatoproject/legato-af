# --------------------------------------------------------------------------------------------------
#  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

add_library(parseTree
                defFile.cpp
                token.cpp
                content.cpp
                compoundItem.cpp
           )

add_dependencies(parseTree
                    PrecompiledHeader
                )
