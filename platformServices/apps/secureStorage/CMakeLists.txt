#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

message(STATUS "Secure Storage Platform Adaptor (LEGATO_SECSTORE_PA): ${LEGATO_SECSTORE_PA}")

if (LEGATO_SECSTORE_PA MATCHES ".*/pre-built/.*")

    message(STATUS "Using pre-built Secure Storage PA.")

    set(PA_FLAGS            --ldflags=-lqmi_cci
                            --ldflags=-lqmiservices
                            --ldflags=-lloc_api
                            --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qapi.a
                            )

    file(COPY ${LEGATO_SECSTORE_PA}/libComponent_le_pa_secStore.so DESTINATION ${LIBRARY_OUTPUT_PATH})


elseif(NOT DEFINED LEGATO_COMPONENTS_SECSTORE)

    message(FATAL_ERROR "Legato Secure Storage Platform Adaptor (LEGATO_COMPONENTS_SECSTORE) not selected.")


elseif(LEGATO_COMPONENTS_SECSTORE MATCHES "AT")

    message(STATUS "Building AT Secure Storage PA from sources.")


# QMI Implementation
elseif(LEGATO_COMPONENTS_SECSTORE MATCHES "QMI")

    message(STATUS "Building QMI Secure Storage PA from sources.")

    set(LEGATO_PROPRIETARY_QCT_DIR    ${LEGATO_ROOT}/proprietary/qct)

    set(PA_FLAGS            -i ${LEGATO_PROPRIETARY_QCT_DIR}/inc/framework
                            -i ${LEGATO_PROPRIETARY_QCT_DIR}/inc/services
                            --ldflags=-lqmi_cci
                            --ldflags=-lqmiservices
                            --ldflags=-lloc_api
                            --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qapi.a
                            --cflags="-DSIERRA"
                            )


# Simulation Implementation
elseif(LEGATO_COMPONENTS_SECSTORE MATCHES "simu")

    message(STATUS "Building Simulated Secure Storage PA.")


else()

    message(FATAL_ERROR "Legato Secure Storage Platform Adaptor type '${LEGATO_COMPONENTS_SECSTORE}' not supported.")

endif()

mkapp(  secStore.adef
            -i ${LEGATO_ROOT}/interfaces/secureStorage
            -i ${LEGATO_ROOT}/interfaces/supervisor
            -i ${LEGATO_ROOT}/interfaces/updateDaemon
            -i ${LEGATO_ROOT}/framework/c/src/appCfg
            -i ${LEGATO_ROOT}/framework/c/src
            -i ${LEGATO_ROOT}/components/secStore/platformAdaptor/inc
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
            -i ${LEGATO_ROOT}/interfaces/audio
            -i ${LEGATO_MODEM_PA}
            ${PA_FLAGS}
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_SECSTORE_PA}/*
            ${LEGATO_ROOT}/interfaces/secureStorage/*
            ${LEGATO_ROOT}/interfaces/supervisor/*
            ${LEGATO_ROOT}/interfaces/updateDaemon/*
            ${LEGATO_ROOT}/components/secStore/*
            ${LEGATO_ROOT}/interfaces/modemServices
            ${LEGATO_ROOT}/framework/c/src/*
            secStore.adef
        )
