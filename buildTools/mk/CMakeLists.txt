
# --------------------------------------------------------------------------------------------------
#  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

add_definitions(-include ${CMAKE_BINARY_DIR}/LegatoObjectModel.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ComponentModel)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Parser)


add_executable(mk
        mk.cpp
        mkcomp.cpp
        mkexe.cpp
        mkapp.cpp
        mksys.cpp
        Args.cpp
        InterfaceBuilder.cpp
        ComponentBuilder.cpp
        ComponentInstanceBuilder.cpp
        ExecutableBuilder.cpp
        ApplicationBuilder.cpp
        Utilities.cpp
        )

target_link_libraries(mk Parser ObjectModel)

add_dependencies(mk PrecompiledHeaders)
