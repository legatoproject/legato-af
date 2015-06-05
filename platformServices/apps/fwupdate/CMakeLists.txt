#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

message(STATUS "FwUpdate Platform Adaptor (LEGATO_FWUPDATE_PA): ${LEGATO_FWUPDATE_PA}")

if (LEGATO_FWUPDATE_PA MATCHES ".*/pre-built/.*")

    message(STATUS "Using pre-built FwUpdate PA.")

    set(PA_FLAGS            --ldflags=-lqmi_cci
                            --ldflags=-lqmiservices
                            --ldflags=-lloc_api
                            --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qapi.a
                            )

    file(COPY ${LEGATO_FWUPDATE_PA}/libComponent_le_pa_fwupdate.so DESTINATION ${LIBRARY_OUTPUT_PATH})


elseif(NOT DEFINED LEGATO_COMPONENTS_FWUPDATE)

    message(FATAL_ERROR "Legato FwUpdate Platform Adaptor (LEGATO_COMPONENTS_FWUPDATE) not selected.")


elseif(LEGATO_COMPONENTS_FWUPDATE MATCHES "AT")

    message(STATUS "Building AT FwUpdate PA from sources.")


# QMI Implementation
elseif(LEGATO_COMPONENTS_FWUPDATE MATCHES "QMI")

    message(STATUS "Building QMI FwUpdate PA from sources.")

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
elseif(LEGATO_COMPONENTS_FWUPDATE MATCHES "simu")

    message(STATUS "Building Simulated FwUpdate PA.")


else()

    message(FATAL_ERROR "Legato FwUpdate Platform Adaptor type '${LEGATO_COMPONENTS_FWUPDATE}' not supported.")

endif()


mkapp(  fwupdateService.adef
            -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
            -i ${LEGATO_MODEM_PA}
            -i ${LEGATO_ROOT}/components/fwupdate/platformAdaptor/inc
            -i ${LEGATO_ROOT}/framework/c/src
            -i ${LEGATO_ROOT}/interfaces/audio
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${LEGATO_ROOT}/interfaces/fwupdate
            ${PA_FLAGS}
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_MODEM_PA}/*
            ${LEGATO_FWUPDATE_PA}/*
            ${LEGATO_ROOT}/components/fwupdate/platformAdaptor/default/le_pa_fwupdate_default/*
            ${LEGATO_ROOT}/interfaces/fwupdate/*
            ${LEGATO_ROOT}/components/fwupdate/fwupdateDaemon/*
            fwupdateService.adef
        )

