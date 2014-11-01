#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

set(AIRVANTAGE_INCLUDE_DIR              ${LEGATO_BINARY_DIR}/airvantage/runtime/itf)
set(AIRVANTAGE_LIBRARY_DIR              ${LEGATO_BINARY_DIR}/airvantage/runtime/lib)

# Creates application from the .adef
mkapp(airvantageDemo.adef
    -i ${AIRVANTAGE_INCLUDE_DIR}
    DEPENDS
        legato
        airvantageDemo.c
)

# This is a sample application
add_dependencies(samples airvantageDemo)
