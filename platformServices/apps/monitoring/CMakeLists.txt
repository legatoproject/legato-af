#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
# Use of this work is subject to license.
#*******************************************************************************

message(STATUS "Monitoring Platform Adaptor (LEGATO_MON_PA): ${LEGATO_MON_PA}")

# Directory in which the modem components can be found.
set(MON_COMPONENTS_DIR    ${LEGATO_ROOT}/components/monitoring)

# TODO: Remove this when mk tools are doing dependency checking.
set(LEGATO_MON_DAEMON_FILES
        ${MON_COMPONENTS_DIR}/monDaemon/Component.cdef
        ${MON_COMPONENTS_DIR}/monDaemon/le_mon.c
        ${MON_COMPONENTS_DIR}/monDaemon/le_antenna.c
    )


if (LEGATO_MON_PA MATCHES ".*/pre-built/.*")

    message(STATUS "Using pre-built platform monitoring PA.")

    set(PA_FLAGS    --ldflags=-lqmi_cci
                    --ldflags=-lqmiservices
                    --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qapi.a
                    )

    file(COPY ${LEGATO_MON_PA}/libComponent_le_pa_mon.so DESTINATION ${LIBRARY_OUTPUT_PATH})

elseif(NOT DEFINED LEGATO_COMPONENTS_MONITORING)

    message(FATAL_ERROR "Legato Modem Platform Adaptor (LEGATO_COMPONENTS_MONITORING) not selected.")

elseif (LEGATO_COMPONENTS_MONITORING MATCHES "AT")

    message(STATUS "Building AT modem PA from sources.")

    # TODO: Remove this when mk tools are doing dependency checking.
    set(LEGATO_MODEM_PA_FILES
            ${LEGATO_MON_PA}/Component.cdef
        )

    set(PA_FLAGS    -i ${LEGATO_ROOT}/components/
                    )

# QMI Implementation
elseif(LEGATO_COMPONENTS_MONITORING MATCHES "QMI")

    message(STATUS "Building QMI platform monitoring PA from sources.")

    set(LEGATO_MON_PA_FILES
            ${LEGATO_MON_PA}/Component.cdef
            ${LEGATO_MON_PA}/pa_mon_qmi.c
            ${LEGATO_MON_PA}/pa_antenna_qmi.c
            ${LEGATO_MON_PA}/swiQmi.c
            ${LEGATO_MON_PA}/swiQmi.h
        )

    set(LEGATO_PROPRIETARY_QCT_DIR    ${LEGATO_ROOT}/proprietary/qct)

    set(PA_FLAGS    -i ${LEGATO_PROPRIETARY_QCT_DIR}/inc/framework
                    -i ${LEGATO_PROPRIETARY_QCT_DIR}/inc/services
                    --ldflags=-lqmi_cci
                    --ldflags=-lqmiservices
                    --cflags=-DSIERRA
                    --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qapi.a
                    )

elseif (LEGATO_COMPONENTS_MONITORING MATCHES "simu")

    # TODO: Remove this when mk tools are doing dependency checking.
    set(LEGATO_MODEM_PA_FILES
            ${LEGATO_MON_PA}/Component.cdef
        )

    set(PA_FLAGS    -i ${LEGATO_ROOT}/components/
                    -i ${LEGATO_ROOT}/components/monitoring/monDaemon/
                    )

else()

    message(FATAL_ERROR "Legato platform monitoring Adaptor '${LEGATO_COMPONENTS_MODEM}' not supported.")

endif()

mkapp(  monService.adef
            -c ${MON_COMPONENTS_DIR}/platformAdaptor/qmi
            -i ${CMAKE_CURRENT_BINARY_DIR}
            -i ${MON_COMPONENTS_DIR}/platformAdaptor/inc
            -i ${LEGATO_MON_PA}
            -i ${LEGATO_ROOT}/components/cfgEntries
            -i ${LEGATO_ROOT}/interfaces/monitoring
            ${PA_FLAGS}
            ${C_FLAGS}
            -v
        DEPENDS
            ${LEGATO_MON_DAEMON_FILES}
            ${LEGATO_MON_PA_FILES}
            ${LEGATO_ROOT}/interfaces/monitoring/le_antenna.api
            legato
        )
