#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

message(STATUS "Modem Platform Adaptor (LEGATO_MODEM_PA): ${LEGATO_MODEM_PA}")

# Directory in which the modem components can be found.
set(MODEM_COMPONENTS_DIR    ${LEGATO_ROOT}/components/modemServices)

# TODO: Remove this when mk tools are doing dependency checking.
set(LEGATO_MODEM_DAEMON_FILES
        ${MODEM_COMPONENTS_DIR}/modemDaemon/Component.cdef
        ${MODEM_COMPONENTS_DIR}/modemDaemon/le_fwupdate.c
        ${MODEM_COMPONENTS_DIR}/modemDaemon/le_info.c
        ${MODEM_COMPONENTS_DIR}/modemDaemon/le_mcc.c
        ${MODEM_COMPONENTS_DIR}/modemDaemon/le_mcc_local.h
        ${MODEM_COMPONENTS_DIR}/modemDaemon/le_mdc.c
        ${MODEM_COMPONENTS_DIR}/modemDaemon/le_mdc_local.h
        ${MODEM_COMPONENTS_DIR}/modemDaemon/le_mrc.c
        ${MODEM_COMPONENTS_DIR}/modemDaemon/le_mrc_local.h
        ${MODEM_COMPONENTS_DIR}/modemDaemon/le_ms.c
        ${MODEM_COMPONENTS_DIR}/modemDaemon/le_sim.c
        ${MODEM_COMPONENTS_DIR}/modemDaemon/le_sim_local.h
        ${MODEM_COMPONENTS_DIR}/modemDaemon/le_sms.c
        ${MODEM_COMPONENTS_DIR}/modemDaemon/le_sms_local.h
        ${MODEM_COMPONENTS_DIR}/modemDaemon/smsPdu.c
        ${MODEM_COMPONENTS_DIR}/modemDaemon/smsPdu.h
    )

if(AUTOMOTIVE_TARGET)
    # TODO: Remove this when mk tools are doing dependency checking.
    set(LEGATO_MODEM_DAEMON_FILES
            ${MODEM_COMPONENTS_DIR}/modemDaemon/asn1Msd.h
            ${MODEM_COMPONENTS_DIR}/modemDaemon/asn1Msd.c
            ${MODEM_COMPONENTS_DIR}/modemDaemon/le_ecall_local.h
            ${MODEM_COMPONENTS_DIR}/modemDaemon/le_ecall.c
        )
endif()



if (LEGATO_MODEM_PA MATCHES ".*/pre-built/.*")

    message(STATUS "Using pre-built modem PA.")

    set(PA_FLAGS    --ldflags=-lqmi_cci
                    --ldflags=-lqmiservices
                    --ldflags=-lloc_api
                    --ldflags=/opt/swi/y14-ext/tmp/sysroots/swi-mdm9x15/usr/lib/libsierra_qapi.a
                    )

elseif(NOT DEFINED LEGATO_COMPONENTS_MODEM)

    message(FATAL_ERROR "Legato Modem Platform Adaptor (LEGATO_COMPONENTS_MODEM) not selected.")

elseif (LEGATO_COMPONENTS_MODEM MATCHES "AT")

    message(STATUS "Building AT modem PA from sources.")

    # TODO: Remove this when mk tools are doing dependency checking.
    set(LEGATO_MODEM_PA_FILES
            ${LEGATO_MODEM_PA}/Component.cdef
            ${LEGATO_MODEM_PA}/pa.c
            ${LEGATO_MODEM_PA}/pa_common.c
            ${LEGATO_MODEM_PA}/pa_common_local.h
            ${LEGATO_MODEM_PA}/pa_info.c
            ${LEGATO_MODEM_PA}/pa_mcc.c
            ${LEGATO_MODEM_PA}/pa_mcc_local.h
            ${LEGATO_MODEM_PA}/pa_mdc.c
            ${LEGATO_MODEM_PA}/pa_mdc_local.h
            ${LEGATO_MODEM_PA}/pa_mrc.c
            ${LEGATO_MODEM_PA}/pa_mrc_local.h
            ${LEGATO_MODEM_PA}/pa_sim.c
            ${LEGATO_MODEM_PA}/pa_sim_local.h
            ${LEGATO_MODEM_PA}/pa_sms.c
            ${LEGATO_MODEM_PA}/pa_sms_local.h
        )

    set(PA_FLAGS    -i ${LEGATO_ROOT}/components/
                    )

# QMI Implementation
elseif(LEGATO_COMPONENTS_MODEM MATCHES "QMI")

    message(STATUS "Building QMI modem PA from sources.")

    set(LEGATO_MODEM_PA_FILES
            ${LEGATO_MODEM_PA}/Component.cdef
            ${LEGATO_MODEM_PA}/info_qmi.h
            ${LEGATO_MODEM_PA}/mcc_qmi.h
            ${LEGATO_MODEM_PA}/mdc_qmi.h
            ${LEGATO_MODEM_PA}/mrc_qmi.h
            ${LEGATO_MODEM_PA}/pa_info_qmi.c
            ${LEGATO_MODEM_PA}/pa_mcc_qmi.c
            ${LEGATO_MODEM_PA}/pa_mdc_qmi.c
            ${LEGATO_MODEM_PA}/pa_mrc_qmi.c
            ${LEGATO_MODEM_PA}/pa_qmi.c
            ${LEGATO_MODEM_PA}/pa_sim_qmi.c
            ${LEGATO_MODEM_PA}/pa_sms_qmi.c
            ${LEGATO_MODEM_PA}/sim_qmi.h
            ${LEGATO_MODEM_PA}/sms_qmi.h
            ${LEGATO_MODEM_PA}/swiQmi.c
            ${LEGATO_MODEM_PA}/swiQmi.h
        )

    if(AUTOMOTIVE_TARGET)
            # TODO: Remove this when mk tools are doing dependency checking.
            set(LEGATO_MODEM_PA_FILES
                    ${LEGATO_MODEM_PA}/ecall_qmi.h
                    ${LEGATO_MODEM_PA}/pa_ecall_qmi.c
                )
    endif()

    set(LEGATO_PROPRIETARY_QCT_DIR    ${LEGATO_ROOT}/proprietary/qct)

    set(PA_FLAGS    -i ${LEGATO_PROPRIETARY_QCT_DIR}/inc/framework
                    -i ${LEGATO_PROPRIETARY_QCT_DIR}/inc/services
                    --ldflags=-lqmi_cci
                    --ldflags=-lqmiservices
                    --ldflags=-lloc_api
                    --ldflags=/opt/swi/y14-ext/tmp/sysroots/swi-mdm9x15/usr/lib/libsierra_qapi.a
                    )

else()

    message(FATAL_ERROR "Legato Modem Platform Adaptor '${LEGATO_COMPONENTS_MODEM}' not supported.")

endif()


mkapp(  modemService.adef
            -c ${LEGATO_ROOT}/components
            -c ${LEGATO_ROOT}/components/atManager
            -i ${CMAKE_CURRENT_BINARY_DIR}
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/interfaces/audio
            -i ${MODEM_COMPONENTS_DIR}/platformAdaptor/inc
            -i ${LEGATO_ROOT}/components/cfgEntries
            -i ${LEGATO_ROOT}/interfaces
            ${PA_FLAGS}
            -v
        DEPENDS
            ${LEGATO_MODEM_DAEMON_FILES}
            ${LEGATO_MODEM_PA_FILES}
            legato
        )

