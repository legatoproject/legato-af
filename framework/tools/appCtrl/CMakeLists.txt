#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(INTERFACE_DIR "${PROJECT_SOURCE_DIR}/interfaces/supervisor")

include_directories(${INTERFACE_DIR}/c)

set_legato_component(appCtrl)
add_legato_internal_executable(appCtrl appCtrl.c)

target_link_libraries(appCtrl le_sup_api)
