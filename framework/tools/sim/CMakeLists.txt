#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(APP_TARGET "sim")
set(APP_COMPONENT "sim")

set(INTERFACE_DIR "${PROJECT_SOURCE_DIR}/interfaces")
set(MODEM_GEN_DIR "${INTERFACE_DIR}/modemServices/c")
set(CONFIG_GEN_DIR "${INTERFACE_DIR}/config/c")

include_directories(
	   	${MODEM_GEN_DIR}
		${CONFIG_GEN_DIR}
		)


set_legato_component(${APP_COMPONENT})
add_legato_executable(${APP_TARGET}
                               sim.c)

target_link_libraries(sim
		      ${LEGATO_SERVICES_MODEM_CLIENT_TARGET}
		      le_cfg_api )
