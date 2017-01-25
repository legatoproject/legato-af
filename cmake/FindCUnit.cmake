#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

# Include CUnit as a third party shipped with project

set(CUNIT_ARCHIVE "CUnit-2.1-2-src.tar.bz2")
set(CUNIT_ARCHIVE_MD5 "31c62bd7a65007737ba28b7aafc44d3a")
#set(CUNIT_INSTALL "${PROJECT_SOURCE_DIR}/3rdParty/CUnit")
set(CUNIT_SOURCE "${PROJECT_SOURCE_DIR}/3rdParty/CUnit/src")

if(CMAKE_VERSION VERSION_GREATER 3.0)
    cmake_policy(SET CMP0045 OLD)
endif()

find_package(PackageHandleStandardArgs REQUIRED)

get_target_property(CUNIT_TARGET cunit TYPE)

# Create target 'cunit' to build the library
if(${CUNIT_TARGET} MATCHES "CUNIT_TARGET-NOTFOUND")

    include(ExternalProject)

    if(CMAKE_CROSSCOMPILING)
        include(GetCompilerOptions)
        get_compiler_target(${TARGET_CC} TARGET_HOST)
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

if(NOT DEFINED CUNIT_INSTALL)
    message(FATAL_ERROR "CUNIT_INSTALL not set.")
elseif("${CUNIT_INSTALL}" STREQUAL "")
    message(FATAL_ERROR "CUNIT_INSTALL is empty.")
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
