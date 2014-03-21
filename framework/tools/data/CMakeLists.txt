#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

# Set the output file name
set(APP_TARGET "data")
set(APP_COMPONENT "data")
# Compile
set_legato_component(${APP_COMPONENT})
add_legato_internal_executable(${APP_TARGET}
                               data.c)

# Link
target_link_libraries(${APP_TARGET}
                      ${LEGATO_SERVICES_DCS_CLIENT_TARGET})


