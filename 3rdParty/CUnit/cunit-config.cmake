#*******************************************************************************
# Copyright (c) 2012-2013 Sierra Wireless and others.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Include CUnit as a third party shipped with project

set(CUNIT_ARCHIVE "CUnit-2.1-2-src.tar.bz2")
set(CUNIT_ARCHIVE_MD5 "31c62bd7a65007737ba28b7aafc44d3a")
set(CUNIT_INSTALL "${LEGATO_BINARY_DIR}/3rdParty/CUnit")
set(CUNIT_SOURCE "${LEGATO_SOURCE_DIR}/3rdParty/CUnit/src")
    
get_target_property(CUNIT_TARGET cunit TYPE)

# Create target 'cunit' to build the library
if(${CUNIT_TARGET} MATCHES "CUNIT_TARGET-NOTFOUND")

    include(ExternalProject)

    if(CMAKE_CROSSCOMPILING)
        include(GetCompilerTarget)
        get_compiler_target(gcc TARGET_HOST)
        set(CUNIT_CONFIGURE_ARGS    "--host=${TARGET_HOST}")
    endif()
    
    configure_file(${CUNIT_SOURCE}/configure.sh.in ${CUNIT_INSTALL}/configure.sh)

    ExternalProject_Add(
        cunit
        PREFIX ${CUNIT_INSTALL}
        URL ${CUNIT_SOURCE}/${CUNIT_ARCHIVE}
        URL_MD5 ${CUNIT_ARCHIVE_MD5}
        CONFIGURE_COMMAND sh <INSTALL_DIR>/configure.sh
        BUILD_COMMAND make
        BUILD_IN_SOURCE 1
        LOG_CONFIGURE 1
        LOG_BUILD 1
        LOG_INSTALL 1
    )
    
    ExternalProject_Add_Step(
        cunit comment
        COMMENT "CUnit: Building ${CUNIT_ARCHIVE}"
        DEPENDERS configure
    )
    
endif()

set(CUNIT_INCLUDE_DIRS
    ${CUNIT_INSTALL}/include
    ${CUNIT_INSTALL}/include/CUnit
)
set(CUNIT_LIBRARIES ${CUNIT_INSTALL}/lib/libcunit.a)
    
find_package_handle_standard_args(
    CUnit
    DEFAULT_MSG
    CUNIT_LIBRARIES
    CUNIT_INCLUDE_DIRS
)
