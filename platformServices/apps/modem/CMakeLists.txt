#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

message(STATUS "Modem Platform Adaptor (LEGATO_MODEM_PA): ${LEGATO_MODEM_PA}")
message(STATUS "eCall Platform Adaptor (LEGATO_MODEM_PA_ECALL): ${LEGATO_MODEM_PA_ECALL}")


# Directory in which the modem components can be found.
set(MODEM_COMPONENTS_DIR    ${LEGATO_ROOT}/components/modemServices)

# TODO: Remove this when mk tools are doing dependency checking.
set(LEGATO_MODEM_DAEMON_FILES
        ${MODEM_COMPONENTS_DIR}/modemDaemon/*
    )

set(JANSSON_INC_DIR "${CMAKE_BINARY_DIR}/3rdParty/jansson-2.6/include/")

if (LEGATO_MODEM_PA MATCHES ".*/pre-built/.*")

    message(STATUS "Using pre-built modem PA.")

    set(PA_FLAGS    --ldflags=-lqmi_cci
                    --ldflags=-lqmiservices
                    --ldflags=-lloc_api
                    --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qapi.a
                    )

    file(COPY ${LEGATO_MODEM_PA}/libComponent_le_pa.so DESTINATION ${LIBRARY_OUTPUT_PATH})

elseif(NOT DEFINED LEGATO_COMPONENTS_MODEM)

    message(FATAL_ERROR "Legato Modem Platform Adaptor (LEGATO_COMPONENTS_MODEM) not selected.")

# AT Implementation
elseif (LEGATO_COMPONENTS_MODEM MATCHES "AT")

    message(STATUS "Building AT modem PA from sources.")

    # TODO: Remove this when mk tools are doing dependency checking.
    set(LEGATO_MODEM_PA_FILES
            ${LEGATO_MODEM_PA}/*
        )

    set(PA_FLAGS    -i ${LEGATO_ROOT}/components/
                    )

# Simulation Implementation
elseif (LEGATO_COMPONENTS_MODEM MATCHES "simu")

    # TODO: Remove this when mk tools are doing dependency checking.
    set(LEGATO_MODEM_PA_FILES
            ${LEGATO_MODEM_PA}/*
        )

    set(PA_FLAGS    -i ${LEGATO_ROOT}/components/
                    -i ${LEGATO_ROOT}/components/modemServices/modemDaemon/
                    )

# QMI Implementation
elseif(LEGATO_COMPONENTS_MODEM MATCHES "QMI")

    message(STATUS "Building QMI modem PA from sources.")

    set(LEGATO_MODEM_PA_FILES
            ${LEGATO_MODEM_PA}/*
        )

    if(LEGATO_MODEM_PA_ECALL MATCHES ".*/qmi/.*")
            # TODO: Remove this when mk tools are doing dependency checking.
            list(APPEND LEGATO_MODEM_PA_FILES
                    ${LEGATO_MODEM_PA_ECALL}/*
                )
    endif()

    set(LEGATO_PROPRIETARY_QCT_DIR    ${LEGATO_ROOT}/proprietary/qct)

    set(PA_FLAGS    -i ${LEGATO_PROPRIETARY_QCT_DIR}/inc/framework
                    -i ${LEGATO_PROPRIETARY_QCT_DIR}/inc/services
                    --ldflags=-lqmi_cci
                    --ldflags=-lqmiservices
                    --ldflags=-lloc_api
                    --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qapi.a
                    --cflags="-DSIERRA"
                    )

else()

    message(FATAL_ERROR "Legato Modem Platform Adaptor '${LEGATO_COMPONENTS_MODEM}' not supported.")

endif()

mkapp(  modemService.adef
            -c ${MODEM_COMPONENTS_DIR}/platformAdaptor/qmi
            -c ${LEGATO_ROOT}/components
            -c ${LEGATO_ROOT}/components/atManager
            -i ${CMAKE_CURRENT_BINARY_DIR}
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/interfaces/powerMgr
            -i ${MODEM_COMPONENTS_DIR}/platformAdaptor/inc
            -i ${LEGATO_MODEM_PA}
            -i ${LEGATO_ROOT}/components/cfgEntries
            -i ${LEGATO_ROOT}/interfaces
            -i ${JANSSON_INC_DIR}
            --ldflags=-ljansson
            ${PA_FLAGS}
            ${C_FLAGS}
            -v
        DEPENDS
            ${LEGATO_MODEM_DAEMON_FILES}
            ${LEGATO_MODEM_PA_FILES}
            legato
        )

add_dependencies(modemService jansson)
