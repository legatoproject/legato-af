#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

set(LEGATO_INCLUDE_DIRS)
set(LEGATO_INCLUDE_DIRS_PRIV) #TODO: Handle "Private" option

# Build targets & defines
set(LEGATO_FRAMEWORK_TARGET                 legato)
set(LEGATO_ROOT                             ${LEGATO_SOURCE_DIR})
set(LEGATO_FRAMEWORK_DIR                    ${LEGATO_SOURCE_DIR}/framework/c)
set(LEGATO_LIBRARIES                        ${LIBRARY_OUTPUT_PATH}/liblegato.so -lpthread -lrt)

# Tools
set(LEGATO_TOOL_IFGEN                       ${LEGATO_SOURCE_DIR}/bin/ifgen)
set(LEGATO_TOOL_MKAPP                       ${LEGATO_SOURCE_DIR}/bin/mkapp)
set(LEGATO_TOOL_MKEXE                       ${LEGATO_SOURCE_DIR}/bin/mkexe)
set(LEGATO_TOOL_MKCOMP                      ${LEGATO_SOURCE_DIR}/bin/mkcomp)
set(LEGATO_TOOL_MKSYS                       ${LEGATO_SOURCE_DIR}/bin/mksys)

# C Framework
set(LEGATO_INCLUDE_DIRS ${LEGATO_INCLUDE_DIRS}
    ${LEGATO_FRAMEWORK_DIR}/inc
)

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
    message("WARNING: Deprecated CMake function used (set_legato_component).")
    # Log Configuration
    add_definitions(-DLE_COMPONENT_NAME=${APP_COMPONENT})
    add_definitions(-DLE_LOG_SESSION=${APP_COMPONENT}_LogSession)
    add_definitions(-DLE_LOG_LEVEL_FILTER_PTR=${APP_COMPONENT}_LogLevelFilterPtr)
endfunction()

function(clear_legato_component APP_COMPONENT)
    message("WARNING: Deprecated CMake function used (clear_legato_component).")
    remove_definitions(-DLE_COMPONENT_NAME=${APP_COMPONENT})
    remove_definitions(-DLE_LOG_SESSION=${APP_COMPONENT}_LogSession)
    remove_definitions(-DLE_LOG_LEVEL_FILTER_PTR=${APP_COMPONENT}_LogLevelFilterPtr)
endfunction()


# If embedded, need to build C/C++ code with LEGATO_EMBEDDED defined.
if(LEGATO_EMBEDDED)
    set (LEGATO_EMBEDDED_OPTION --cflags=-DLEGATO_EMBEDDED)
endif()

if(AUTOMOTIVE_TARGET)
    set (LEGATO_AUTOMOTIVE_TARGET_OPTION --cflags=-DAUTOMOTIVE_TARGET)
endif()

# Function to build a Legato executable using mkexe.
# The executable will be put in the appropriate target's bin directory.
# Supporting libraries will be put in the target's lib directory.
# The first parameter to the function must be the name of the exe file.
# Any subsequent parameters will be passed as-is to mkexe on its command line.
function(mkexe EXE_NAME)

    add_custom_command(
            OUTPUT ${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME}
            COMMAND ${LEGATO_TOOL_MKEXE}
                          -o ${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME}
                          -w ${CMAKE_CURRENT_BINARY_DIR}
                          -c ${CMAKE_CURRENT_BINARY_DIR}
                          -i ${CMAKE_CURRENT_SOURCE_DIR}
                          -l ${LIBRARY_OUTPUT_PATH}
                          -t ${LEGATO_TARGET}
                          ${LEGATO_EMBEDDED_OPTION}
                          ${LEGATO_AUTOMOTIVE_TARGET_OPTION}
                          -v
                          ${ARGN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "mkexe '${EXE_NAME}': ${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME}"
    )

    add_custom_target(${EXE_NAME}
            ALL
            DEPENDS ${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME}
    )

    # Dependencies
    add_dependencies(${EXE_NAME} ${LEGATO_FRAMEWORK_TARGET})

endfunction()


# Function to build a Legato application using mkapp
# Any subsequent parameters will be passed as-is to mkapp on its command line.
function(mkapp ADEF)

    get_filename_component(APP_NAME ${ADEF} NAME_WE)
    set(APP_PKG "${APP_OUTPUT_PATH}/${APP_NAME}.${LEGATO_TARGET}")

    message("mkapp : ${APP_NAME} = ${APP_PKG}")

    add_custom_command(
            OUTPUT ${APP_PKG}
            COMMAND
                PATH=${LEGATO_SOURCE_DIR}/bin:$ENV{PATH}
                ${LEGATO_TOOL_MKAPP}
                        ${ADEF}
                        -t ${LEGATO_TARGET}
                        -w ${CMAKE_CURRENT_BINARY_DIR}/_build_${APP_NAME}.${LEGATO_TARGET}
                        -i ${CMAKE_CURRENT_SOURCE_DIR}
                        -c ${CMAKE_CURRENT_SOURCE_DIR}
                        -o ${APP_OUTPUT_PATH}
                        ${LEGATO_EMBEDDED_OPTION}
                        ${LEGATO_AUTOMOTIVE_TARGET_OPTION}
                        -v
                        ${ARGN}
            COMMAND
            DEPENDS ${ADEF}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "mkapp '${APP_NAME}': ${APP_PKG}"
    )

    add_custom_target(${APP_NAME}
            ALL
            DEPENDS ${APP_PKG}
    )

    # Dependencies
    add_dependencies(${APP_NAME} ${LEGATO_FRAMEWORK_TARGET})

endfunction()


# Function to build a Legato component library using mkcomp
# Any subsequent parameters will be passed as-is to mkapp on its command line.
function(mkcomp COMP_PATH)

    get_filename_component(COMP_NAME ${COMP_PATH} NAME_WE)
    set(COMPONENT_LIB "${LIBRARY_OUTPUT_PATH}/lib${COMP_NAME}.so")

    message("mkcomp: ${COMP_NAME} = ${COMPONENT_LIB} (from ${COMP_PATH})")

    add_custom_command(
            OUTPUT ${COMPONENT_LIB}
            COMMAND
                PATH=${LEGATO_SOURCE_DIR}/bin:$ENV{PATH}
                ${LEGATO_TOOL_MKCOMP}
                        ${COMP_PATH}
                        -o ${COMPONENT_LIB}
                        -t ${LEGATO_TARGET}
                        -w ${CMAKE_CURRENT_BINARY_DIR}
                        -i ${CMAKE_CURRENT_SOURCE_DIR}
                        -c ${CMAKE_CURRENT_SOURCE_DIR}
                        -l ${LIBRARY_OUTPUT_PATH}
                        ${LEGATO_EMBEDDED_OPTION}
                        ${LEGATO_AUTOMOTIVE_TARGET_OPTION}
                        -v
                        ${ARGN}
            DEPENDS ${COMP_PATH}/Component.cdef
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "mkcomp '${COMP_NAME}': ${COMPONENT_LIB}"
    )

    add_custom_target(${COMP_NAME}
            ALL
            DEPENDS ${COMPONENT_LIB}
    )

    # Dependencies
    add_dependencies(${COMP_NAME} ${LEGATO_FRAMEWORK_TARGET})

endfunction()


# Function to compile applications
function(add_legato_executable EXE_TARGET)

    message("WARNING: Deprecated CMake function used (add_legato_executable).  Consider changing to mkexe.")

    # Other arguments are sources
    set(EXE_SOURCES ${ARGN})

    # Legato Includes
    include_directories(${LEGATO_INCLUDE_DIRS})

    # Compile
    add_executable(${EXE_TARGET}
        ${LEGATO_SOURCE_DIR}/framework/c/codegen/_le_main.c
        ${EXE_SOURCES}
   )

    # Link
    target_link_libraries(${EXE_TARGET}
        ${LEGATO_LIBRARIES}
    )

    # Dependencies
    add_dependencies(${EXE_TARGET} ${LEGATO_FRAMEWORK_TARGET} )

    # Compile flags
    add_compile_flags(${EXE_TARGET} "-DLE_EXECUTABLE_NAME=${EXE_TARGET}")

endfunction()

# Function to compile internal applications
function(add_legato_internal_executable)

    message("WARNING: Deprecated CMake function used (add_legato_internal_executable).  Consider changing to mkexe.")

    # Legato Private Includes
    include_directories(${LEGATO_INCLUDE_DIRS_PRIV})

    add_legato_executable(${ARGN})

endfunction()
