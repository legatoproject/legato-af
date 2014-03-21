#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)


set(INTERFACE_DIR "${PROJECT_SOURCE_DIR}/interfaces/")

include_directories("${INTERFACE_DIR}/config/c" "${INTERFACE_DIR}/configAdmin/c")


set_legato_component(config)
add_legato_internal_executable(config config.c)

target_link_libraries(config le_cfg_api le_cfgAdmin_api)
