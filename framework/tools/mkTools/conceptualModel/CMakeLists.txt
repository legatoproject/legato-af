# --------------------------------------------------------------------------------------------------
#  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# --------------------------------------------------------------------------------------------------

add_library(conceptualModel
                api.cpp
                component.cpp
                exe.cpp
                app.cpp
                boolLimit.cpp
                faultAction.cpp
                nonNegativeIntLimit.cpp
                positiveIntLimit.cpp
                priority.cpp
                process.cpp
                processEnvironment.cpp
                watchdogAction.cpp
                watchdogTimeout.cpp
           )

add_dependencies(conceptualModel
                    PrecompiledHeader
                )

