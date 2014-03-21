#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(CUnit REQUIRED)
find_package(Legato REQUIRED)

include_directories(
    ${CUNIT_INCLUDE_DIRS}
    ${LEGATO_COMPONENTS_AT_MANAGER_DIR}/src
)

add_subdirectory(atmachinecommand)
add_subdirectory(atmachinedevices)
add_subdirectory(atmachinestring)
add_subdirectory(atmachineunsolicited)
add_subdirectory(atmachineparser)
add_subdirectory(atmachinemgritf)
