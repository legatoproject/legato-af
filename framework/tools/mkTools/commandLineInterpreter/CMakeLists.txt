# --------------------------------------------------------------------------------------------------
#  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------


add_executable(mk
        mk.cpp
        mkCommon.cpp
        mkcomp.cpp
        mkexe.cpp
        mkapp.cpp
        mksys.cpp
        args.cpp
        )

target_link_libraries(mk
        modeller
        codeGenerator
        configGenerator
        buildScriptGenerator
        avManifestGenerator
        conceptualModel
        parser
        parseTree
        mkTools
        )

add_dependencies(mk PrecompiledHeader)
