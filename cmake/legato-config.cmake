#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

set(LEGATO_INCLUDE_DIRS)
set(LEGATO_INCLUDE_DIRS_PRIV) #TODO: Remove this.

if(NOT DEFINED LEGATO_ROOT)
    message(FATAL_ERROR "LEGATO_ROOT is not defined.  Set to path of Legato framework directory.")
endif()
if(NOT DEFINED LEGATO_TARGET)
    message(FATAL_ERROR "LEGATO_TARGET is not defined.  Set to target device type (e.g., wp85).")
endif()

# Defines
set(LEGATO_LIBLEGATO_DIR    "${LEGATO_ROOT}/framework/liblegato")
set(LEGATO_BUILD            "${LEGATO_ROOT}/build/${LEGATO_TARGET}")
set(LEGATO_BINARY_DIR       "${LEGATO_BUILD}")  # TODO: Remove this.

# Tools
set(LEGATO_TOOL_IFGEN       "${LEGATO_ROOT}/bin/ifgen")
set(LEGATO_TOOL_MKAPP       "${LEGATO_ROOT}/bin/mkapp")
set(LEGATO_TOOL_MKEXE       "${LEGATO_ROOT}/bin/mkexe")
set(LEGATO_TOOL_MKCOMP      "${LEGATO_ROOT}/bin/mkcomp")
set(LEGATO_TOOL_MKSYS       "${LEGATO_ROOT}/bin/mksys")
set(LEGATO_TOOL_MKDOC       "${LEGATO_ROOT}/bin/mkdoc")
set(LEGATO_TOOL_K2DOX       "${LEGATO_ROOT}/bin/kconfig2dox")

# C Framework
set(LEGATO_INCLUDE_DIRS ${LEGATO_INCLUDE_DIRS}
                        ${LEGATO_ROOT}/framework/include
                        ${LEGATO_ROOT}/build/${LEGATO_TARGET}/framework/include)
set(LEGATO_LIBRARY_PATH ${LEGATO_ROOT}/build/${LEGATO_TARGET}/framework/lib/liblegato.so)

set(JOBS_ARGS "")
if (NOT $ENV{LEGATO_JOBS} STREQUAL "")
    set(JOBS_ARGS -j $ENV{LEGATO_JOBS})
endif()

# Low-Level Interfaces
# TODO: Get rid of this.
set(LEGATO_INCLUDE_DIRS_PRIV ${LEGATO_INCLUDE_DIRS_PRIV}

    # Components
    ${LEGATO_COMPONENTS_MODEM_DIR}/interfaces/inc
    ${LEGATO_COMPONENTS_AT_MANAGER_INCLUDE_DIRS}
    ${LEGATO_COMPONENTS_GNSS_DIR}/interfaces/inc
    ${LEGATO_COMPONENTS_AUDIO_DIR}/interfaces/inc

    # Devices
    ${LEGATO_UART_TARGET_DIR}/inc

    # Apps
    ${LEGATO_SERVICES_AUDIO_DIR}/src
    ${LEGATO_SERVICES_MODEM_DIR}/MdmSvc
    ${LEGATO_SERVICES_POSITIONING_DIR}/src
    ${LEGATO_SERVICES_VOICECALL_DIR}/src

    # ConfigDB entries
    ${LEGATO_CONFIGTREE_ENTRIES_DIR}
)

# Function to add a compile flag for a given target.
# Put the flags in quotes.
# e.g., add_compile_flags(myExe "-DDEBUG")
function(add_compile_flags COMPILE_TARGET)

    set_target_properties(${COMPILE_TARGET} PROPERTIES COMPILE_FLAGS ${ARGN})

endfunction()

function(set_legato_component APP_COMPONENT)
    message("WARNING: (${APP_COMPONENT}) Deprecated CMake function used (set_legato_component).")
    # Log Configuration
    add_definitions(-DLE_COMPONENT_NAME=${APP_COMPONENT})
    add_definitions(-DLE_LOG_SESSION=${APP_COMPONENT}_LogSession)
    add_definitions(-DLE_LOG_LEVEL_FILTER_PTR=${APP_COMPONENT}_LogLevelFilterPtr)
endfunction()

function(clear_legato_component APP_COMPONENT)
    message("WARNING: (${APP_COMPONENT}) Deprecated CMake function used (clear_legato_component).")
    remove_definitions(-DLE_COMPONENT_NAME=${APP_COMPONENT})
    remove_definitions(-DLE_LOG_SESSION=${APP_COMPONENT}_LogSession)
    remove_definitions(-DLE_LOG_LEVEL_FILTER_PTR=${APP_COMPONENT}_LogLevelFilterPtr)
endfunction()

# Function to build a Legato executable using mkexe.
# The executable will be put in the appropriate target's bin directory.
# Supporting libraries will be put in the target's lib directory.
# The first parameter to the function must be the name of the exe file.
# Any subsequent parameters will be passed as-is to mkexe on its command line.
function(mkexe EXE_NAME)

    if (NOT DEFINED EXECUTABLE_OUTPUT_PATH)
        message(FATAL_ERROR "EXECUTABLE_OUTPUT_PATH not set when building exe '${EXE_NAME}'")
    elseif (EXECUTABLE_OUTPUT_PATH STREQUAL "")
        message(FATAL_ERROR "EXECUTABLE_OUTPUT_PATH empty when building exe '${EXE_NAME}'")
    endif()
    if (NOT DEFINED LIBRARY_OUTPUT_PATH)
        message(FATAL_ERROR "LIBRARY_OUTPUT_PATH not set when building exe '${EXECUTABLE_OUTPUT_PATH}'")
    elseif (LIBRARY_OUTPUT_PATH STREQUAL "")
        message(FATAL_ERROR "LIBRARY_OUTPUT_PATH empty when building exe '${EXECUTABLE_OUTPUT_PATH}'")
    endif()

    add_custom_target(${EXE_NAME} ALL
            COMMAND ${LEGATO_TOOL_MKEXE}
                          -o ${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME}
                          -w ${CMAKE_CURRENT_BINARY_DIR}/_build_${EXE_NAME}
                          -i ${CMAKE_CURRENT_SOURCE_DIR}
                          -l ${LIBRARY_OUTPUT_PATH}
                          -t ${LEGATO_TARGET}
                          ${JOBS_ARGS}
                          ${ARGN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "mkexe '${EXE_NAME}': ${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME}"
    )

endfunction()


# Function to build a Legato application using mkapp
# Any subsequent parameters will be passed as-is to mkapp on its command line.
function(mkapp ADEF)

    if (NOT DEFINED APP_OUTPUT_PATH)
        message(FATAL_ERROR "APP_OUTPUT_PATH not set when building '${ADEF}'")
    elseif (APP_OUTPUT_PATH STREQUAL "")
        message(FATAL_ERROR "APP_OUTPUT_PATH empty when building '${ADEF}'")
    endif()

    get_filename_component(APP_NAME ${ADEF} NAME_WE)
    set(APP_PKG "${APP_OUTPUT_PATH}/${APP_NAME}.${LEGATO_TARGET}")

    message("mkapp : ${APP_NAME} = ${APP_PKG}")

    add_custom_target(${APP_NAME} ALL
            COMMAND
                PATH=${LEGATO_ROOT}/bin:$ENV{PATH}
                ${LEGATO_TOOL_MKAPP}
                        ${ADEF}
                        -t ${LEGATO_TARGET}
                        -w ${CMAKE_CURRENT_BINARY_DIR}/_build_${APP_NAME}.${LEGATO_TARGET}
                        -i ${CMAKE_CURRENT_SOURCE_DIR}
                        -c ${CMAKE_CURRENT_SOURCE_DIR}
                        -o ${APP_OUTPUT_PATH}
                        ${JOBS_ARGS}
                        ${ARGN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "mkapp '${APP_NAME}': ${APP_PKG}"
    )

endfunction()


# Function to build a Legato component library using mkcomp
# Any subsequent parameters will be passed as-is to mkapp on its command line.
function(mkcomp COMP_PATH)

    if (NOT DEFINED LIBRARY_OUTPUT_PATH)
        message(FATAL_ERROR "LIBRARY_OUTPUT_PATH not set when building component '${COMP_PATH}'")
    elseif (LIBRARY_OUTPUT_PATH STREQUAL "")
        message(FATAL_ERROR "LIBRARY_OUTPUT_PATH empty when building component '${COMP_PATH}'")
    endif()

    get_filename_component(COMP_NAME ${COMP_PATH} NAME_WE)
    set(COMPONENT_LIB "${LIBRARY_OUTPUT_PATH}/lib${COMP_NAME}.so")

    message("mkcomp: ${COMP_NAME} = ${COMPONENT_LIB} (from ${COMP_PATH})")

    add_custom_target(${COMP_NAME} ALL
            COMMAND
                PATH=${LEGATO_ROOT}/bin:$ENV{PATH}
                ${LEGATO_TOOL_MKCOMP}
                        ${COMP_PATH}
                        -o ${COMPONENT_LIB}
                        -t ${LEGATO_TARGET}
                        -w ${CMAKE_CURRENT_BINARY_DIR}
                        -i ${CMAKE_CURRENT_SOURCE_DIR}
                        -c ${CMAKE_CURRENT_SOURCE_DIR}
                        -l ${LIBRARY_OUTPUT_PATH}
                        ${JOBS_ARGS}
                        ${ARGN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "mkcomp '${COMP_NAME}': ${COMPONENT_LIB}"
    )

endfunction()


# Function to compile applications
function(add_legato_executable EXE_TARGET)

    message("WARNING: (${EXE_TARGET}) Deprecated CMake function used (add_legato_executable).  Consider changing to mkexe.")

    # Other arguments are sources
    set(EXE_SOURCES ${ARGN})

    # Legato Includes
    include_directories(${LEGATO_INCLUDE_DIRS})

    # Compile
    add_executable(${EXE_TARGET}
        ${LEGATO_ROOT}/framework/liblegato/codegen/_le_main.c
        ${EXE_SOURCES}
   )

    # Link
    target_link_libraries(${EXE_TARGET} ${LEGATO_LIBRARY_PATH} -lpthread -lrt)

    # Compile flags
    add_compile_flags(${EXE_TARGET} "-DLE_EXECUTABLE_NAME=${EXE_TARGET}")

endfunction()

# Function to compile internal applications
function(add_legato_internal_executable EXE_TARGET)

    message("WARNING: (${EXE_TARGET}) Deprecated CMake function used (add_legato_internal_executable).  Consider changing to mkexe.")

    # Legato Private Includes
    include_directories(${LEGATO_INCLUDE_DIRS_PRIV})

    add_legato_executable(${EXE_TARGET} ${ARGN})

endfunction()

# Function to generate the interface header from the API file to make it
# available for documentation.
function(generate_header API_FILE)

    get_filename_component(API_NAME ${API_FILE} NAME_WE)

    set(API_PATH    "${CMAKE_CURRENT_SOURCE_DIR}/${API_FILE}")
    set(HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_interface.h")

    add_custom_command( OUTPUT ${HEADER_PATH}
                        COMMAND ${LEGATO_TOOL_IFGEN} --gen-interface ${API_PATH}
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}/audio
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}/modemServices
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}/atServices
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}
                        ${ARGN}
                        COMMENT "ifgen '${API_FILE}': ${HEADER_PATH}"
                        DEPENDS ${API_FILE}
                        )

    add_custom_target(  ${API_NAME}_if
                        DEPENDS ${HEADER_PATH}
                        )

    add_dependencies( api_headers ${API_NAME}_if)
endfunction()

# Function to generate the required files for a client
function(generate_client API_FILE)

    get_filename_component(API_NAME ${API_FILE} NAME_WE)

    set(API_PATH    "${API_FILE}")
    set(HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_interface.h")
    set(CLIENT_PATH "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_client.c")
    set(LOCAL_PATH  "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_service.h")
    set(COMMON_LOCAL_PATH  "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_messages.h")
    set(COMMON_HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_common.h")
    set(COMMON_CLIENT_PATH "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_commonclient.c")

    add_custom_command( OUTPUT ${HEADER_PATH} ${CLIENT_PATH} ${LOCAL_PATH}
                               ${COMMON_LOCAL_PATH} ${COMMON_HEADER_PATH} ${COMMON_CLIENT_PATH}
                        COMMAND ${LEGATO_TOOL_IFGEN} --gen-client --gen-interface --gen-local
                                --gen-messages --gen-common-interface --gen-common-client
                        ${API_PATH}
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}/audio
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}/modemServices
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}/atServices
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}
                        ${ARGN}
                        COMMENT "ifgen '${API_FILE}': ${HEADER_PATH}"
                        DEPENDS ${API_FILE}
                        )
endfunction()

# Function to generate the interface header from an API file that resides outside the framework
# source tree to make it  available for documentation.
# API_FILE is with full path
function(generate_header_extern API_FILE)

    get_filename_component(API_NAME ${API_FILE} NAME_WE)

    set(API_PATH    "${API_FILE}")
    set(HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/${API_NAME}_interface.h")

    add_custom_command( OUTPUT ${HEADER_PATH}
                        COMMAND ${LEGATO_TOOL_IFGEN} --gen-interface ${API_PATH}
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}/audio
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}/modemServices
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}/atServices
                        --import-dir ${CMAKE_CURRENT_SOURCE_DIR}
                        ${ARGN}
                        COMMENT "ifgen '${API_FILE}': ${HEADER_PATH}"
                        DEPENDS ${API_FILE}
                        )

    add_custom_target(  ${API_NAME}_if
                        DEPENDS ${HEADER_PATH}
                        )

    add_dependencies( api_headers ${API_NAME}_if)
endfunction()
