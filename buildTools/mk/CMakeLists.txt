
# --------------------------------------------------------------------------------------------------
#  Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
# --------------------------------------------------------------------------------------------------


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ComponentModel)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Parser)


add_executable(mk
        mk.cpp
        mkexe.cpp
        mkapp.cpp
        Args.cpp
        ComponentBuilder.cpp
        ExecutableBuilder.cpp
        Utilities.cpp
        )

target_link_libraries(mk Parser ObjectModel)
