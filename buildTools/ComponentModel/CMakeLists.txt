# --------------------------------------------------------------------------------------------------
#  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

add_definitions(-include ${CMAKE_BINARY_DIR}/LegatoObjectModel.h)

add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/LegatoObjectModel.h.gch
                    COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS}
                            -std=c++0x
                            -g
                            -o ${CMAKE_BINARY_DIR}/LegatoObjectModel.h.gch
                            ${CMAKE_CURRENT_SOURCE_DIR}/LegatoObjectModel.h
                    DEPENDS Api.h
                            App.h
                            BoolLimit.h
                            BuildParams.h
                            ProgrammingLanguage.h
                            Component.h
                            ComponentInstance.h
                            ConfigItem.h
                            Exception.h
                            Executable.h
                            FaultAction.h
                            FileMapping.h
                            FilePath.h
                            Interface.h
                            IpcBinding.h
                            LegatoObjectModel.h
                            Library.h
                            Limit.h
                            MemoryPool.h
                            NonNegativeIntLimit.h
                            Permissions.h
                            PositiveIntLimit.h
                            Priority.h
                            ProcessEnvironment.h
                            Process.h
                            System.h
                            WatchdogAction.h
                            WatchdogTimeout.h
                   )

add_custom_target(  PrecompiledHeaders
                    ALL
                    DEPENDS ${CMAKE_BINARY_DIR}/LegatoObjectModel.h.gch
                    )

add_library(ObjectModel
            Library.cpp
            Api.cpp
            Component.cpp
            ComponentInstance.cpp
            Executable.cpp
            App.cpp
            ProcessEnvironment.cpp
            Process.cpp
            Exception.cpp
            FilePath.cpp
            FileMapping.cpp
            ProgrammingLanguage.cpp
            BuildParams.cpp
            Interface.cpp
            Priority.cpp
            System.cpp
            WatchdogTimeout.cpp
            WatchdogAction.cpp
            FaultAction.cpp
            NonNegativeIntLimit.cpp
            PositiveIntLimit.cpp
        )

add_dependencies(ObjectModel PrecompiledHeaders)
