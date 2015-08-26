#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

message(STATUS "AVC Platform Adaptor (LEGATO_AVC_PA): ${LEGATO_AVC_PA}")

if (LEGATO_AVC_PA MATCHES ".*/pre-built/.*")

    message(STATUS "Using pre-built AVC PA.")

    set(PA_FLAGS            --ldflags=-lqmi_cci
                            --ldflags=-lqmiservices
                            --ldflags=-lloc_api
                            --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qapi.a
                            )

    file(COPY ${LEGATO_AVC_PA}/libComponent_le_pa_avc.so DESTINATION ${LIBRARY_OUTPUT_PATH})


elseif(NOT DEFINED LEGATO_COMPONENTS_AVC)

    message(FATAL_ERROR "Legato AVC Platform Adaptor (LEGATO_COMPONENTS_AVC) not selected.")


elseif(LEGATO_COMPONENTS_AVC MATCHES "AT")

    message(STATUS "Building AT AVC PA from sources.")


# QMI Implementation
elseif(LEGATO_COMPONENTS_AVC MATCHES "QMI")

    message(STATUS "Building QMI AVC PA from sources.")

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
elseif(LEGATO_COMPONENTS_AVC MATCHES "simu")

    message(STATUS "Building Simulated AVC PA.")


else()

    message(FATAL_ERROR "Legato AVC Platform Adaptor type '${LEGATO_COMPONENTS_AVC}' not supported.")

endif()


mkapp(  avcService.adef
            -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
            -i ${LEGATO_MODEM_PA}
            -i ${LEGATO_ROOT}/components/fwupdate/platformAdaptor/inc
            -i ${LEGATO_ROOT}/components/airVantage/platformAdaptor/inc
            -i ${LEGATO_ROOT}/framework/c/src
            -i ${LEGATO_ROOT}/interfaces/audio
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/interfaces/fwupdate
            -i ${LEGATO_ROOT}/interfaces/airVantage
            -i ${LEGATO_ROOT}/interfaces/updateDaemon
            -i ${LEGATO_ROOT}/interfaces/supervisor
            -i ${LEGATO_ROOT}/framework/c/src/appCfg
            -s ${LEGATO_ROOT}/framework/c/src
            -C "-I${LEGATO_ROOT}/framework/c/src"
            ${PA_FLAGS}
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_MODEM_PA}/*
            ${LEGATO_FWUPDATE_PA}/*
            ${LEGATO_AVC_PA}/*
            ${LEGATO_ROOT}/components/airVantage/platformAdaptor/default/le_pa_avc_default/*
            ${LEGATO_ROOT}/interfaces/airVantage/*
            ${LEGATO_ROOT}/components/airVantage/avcDaemon/*
            ${LEGATO_ROOT}/components/airVantage/avcAppUpdate/*
            ${LEGATO_ROOT}/interfaces/updateDaemon/*
            ${LEGATO_ROOT}/interfaces/supervisor/*
            avcService.adef
        )

