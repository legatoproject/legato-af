
# --------------------------------------------------------------------------------------------------
#  Copyright (C) 2013-2014, Sierra Wireless Inc.  Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ComponentModel)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Parser)


add_executable(mk
        mk.cpp
        mkcomp.cpp
        mkexe.cpp
        mkapp.cpp
        Args.cpp
        InterfaceBuilder.cpp
        ComponentBuilder.cpp
        ExecutableBuilder.cpp
        ApplicationBuilder.cpp
        Utilities.cpp
        )

target_link_libraries(mk Parser ObjectModel)
