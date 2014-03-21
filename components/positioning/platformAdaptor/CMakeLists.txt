#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

project(LEGATO_GNSS C)

find_package(Legato REQUIRED)

###
### GNSS
###

message(STATUS "GNSS Component (LEGATO_COMPONENTS_GNSS): ${LEGATO_COMPONENTS_GNSS}")

include_directories(
    ${LEGATO_INCLUDE_DIRS}
    ${LEGATO_INCLUDE_DIRS_PRIV}
)

if(NOT DEFINED LEGATO_COMPONENTS_GNSS)
    message(FATAL_ERROR "Legato Gnss Component to build is not selected.")

# Stub Implementation
elseif(LEGATO_COMPONENTS_GNSS MATCHES "STUB")

    aux_source_directory(${LEGATO_COMPONENTS_GNSS_DIR}/stub/src LEGATO_GNSS_STUB_SRC)

    add_library(${LEGATO_COMPONENTS_GNSS_TARGET} SHARED
        ${LEGATO_GNSS_STUB_SRC}
    )

# AT Implementation
elseif(LEGATO_COMPONENTS_GNSS MATCHES "AT")

    include_directories(
        ${LEGATO_COMPONENTS_AT_MANAGER_INCLUDE_DIRS}
    )

    aux_source_directory(${LEGATO_COMPONENTS_GNSS_DIR}/at/src LEGATO_GNSS_AT_SRC)

    add_library(${LEGATO_COMPONENTS_GNSS_TARGET} SHARED
        ${LEGATO_GNSS_AT_SRC}
    )

    target_link_libraries(${LEGATO_COMPONENTS_GNSS_TARGET}
        ${LEGATO_COMPONENTS_AT_MANAGER_TARGET}
        ${LEGATO_FRAMEWORK_TARGET}
    )

# QMI Implementation
elseif(LEGATO_COMPONENTS_GNSS MATCHES "QMI")

    # Build from sources
    if(EXISTS ${LEGATO_COMPONENTS_GNSS_DIR}/qmi/src)
        add_subdirectory(${LEGATO_COMPONENTS_GNSS_DIR}/qmi/src)

    # Use precompiled library
    else()
        message("Using pre-compiled library for QMI PA (${LEGATO_COMPONENTS_GNSS_TARGET})")

        set(QMI_LIB_NAME lib${LEGATO_COMPONENTS_GNSS_TARGET}.${LEGATO_TARGET}.so)
        set(QMI_LIB      ${LEGATO_COMPONENTS_GNSS_DIR}/qmi/lib/${QMI_LIB_NAME})

        message("Located @ ${QMI_LIB}")

        set_property(TARGET ${LEGATO_COMPONENTS_GNSS_TARGET} PROPERTY
            IMPORTED_LOCATION ${QMI_LIB}
        )

        file(COPY ${QMI_LIB} DESTINATION ${LIBRARY_OUTPUT_PATH})
        file(RENAME
            ${LIBRARY_OUTPUT_PATH}/${QMI_LIB_NAME}
            ${LIBRARY_OUTPUT_PATH}/lib${LEGATO_COMPONENTS_GNSS_TARGET}.so
        )
    endif()

else()
    message(FATAL_ERROR "Legato GNSS Component to build is invalid.")
endif()
