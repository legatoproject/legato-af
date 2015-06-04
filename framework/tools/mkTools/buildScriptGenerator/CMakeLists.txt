# --------------------------------------------------------------------------------------------------
#  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

add_library(buildScriptGenerator
                buildScriptCommon.cpp
                appBuildScript.cpp
                exeBuildScript.cpp
                componentBuildScript.cpp
                toolChain.cpp
           )

add_dependencies(buildScriptGenerator
                    PrecompiledHeader
                )

