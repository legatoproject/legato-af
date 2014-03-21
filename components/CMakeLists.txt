#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

### Components
if(INCLUDE_AIRVANTAGE)
    add_subdirectory(legatoTreeHandler)
endif()
add_subdirectory(audio)
add_subdirectory(audioDaemon)
add_subdirectory(modemServices)
add_subdirectory(modemDaemon)
add_subdirectory(positioning)
add_subdirectory(posDaemon)
add_subdirectory(dataConnectionService)
