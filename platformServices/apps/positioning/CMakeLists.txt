#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

message(STATUS "GNSS Platform Adaptor (LEGATO_GNSS_PA): ${LEGATO_GNSS_PA}")

set(POSITIONING_COMPONENTS ${LEGATO_ROOT}/components/positioning)
set(MODEM_COMPONENTS   ${LEGATO_ROOT}/components/modemServices)
set(POS_INTERFACE_DIR   ${LEGATO_ROOT}/interfaces/positioning)


## TODO: Remove this when the mk tools do dependency checking.
set(POS_DAEMON_FILES    ${LEGATO_ROOT}/components/positioning/posDaemon/Component.cdef
                        ${LEGATO_ROOT}/components/positioning/posDaemon/le_gnss.c
                        ${LEGATO_ROOT}/components/positioning/posDaemon/le_pos.c
                        )

## TODO: Remove this when the mk tools do dependency checking.
set(APIS        ${POS_INTERFACE_DIR}/le_gnss.api
                ${POS_INTERFACE_DIR}/le_pos.api
                ${POS_INTERFACE_DIR}/le_posCtrl.api
                )


if (LEGATO_GNSS_PA MATCHES ".*/pre-built/.*")

    message(STATUS "Using pre-built GNSS PA.")

    set(LINK_DIRECTIVES     --ldflags=-lqmi_cci
                            --ldflags=-lqmiservices
                            --ldflags=-lloc_api
                            --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qapi.a
                            )

    file(COPY ${LEGATO_GNSS_PA}/libComponent_le_pa_gnss.so DESTINATION ${LIBRARY_OUTPUT_PATH})

elseif(NOT DEFINED LEGATO_COMPONENTS_GNSS)

    message(FATAL_ERROR "Legato GNSS Platform Adaptor (LEGATO_COMPONENTS_GNSS) not selected.")

elseif(LEGATO_COMPONENTS_GNSS MATCHES "AT")

    message(STATUS "Building AT GNSS PA from sources.")

    ## TODO: Remove this when the mk tools do dependency checking.
    set(PA_FILES    ${LEGATO_GNSS_PA}/gnss_at.c
                    ${LEGATO_GNSS_PA}/Component.cdef
                    )

# Simulation Implementation
elseif(LEGATO_COMPONENTS_GNSS MATCHES "simu")

    ## TODO: Remove this when the mk tools do dependency checking.
    set(PA_FILES    ${LEGATO_GNSS_PA}/*
                    )

    set(ADDITIONAL_INTERFACE_SEARCH_DIRECTIVES
        -i  ${LEGATO_ROOT}/components/modemServices/modemDaemon
        )

# QMI Implementation
elseif(LEGATO_COMPONENTS_GNSS MATCHES "QMI")

    message(STATUS "Building QMI GNSS PA from sources.")

    set(LINK_DIRECTIVES     --ldflags=-lqmi_cci
                            --ldflags=-lqmiservices
                            --ldflags=-lloc_api
                            --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qapi.a
                            --cflags="-DSIERRA"
                            )

    set(PA_SOURCE_DIR ${LEGATO_GNSS_PA})

    ## TODO: Remove this when the mk tools do dependency checking.
    set(PA_FILES    ${PA_SOURCE_DIR}/pa_gnss_qmi.c
                    ${PA_SOURCE_DIR}/Component.cdef
                    )

    set(LEGATO_PROPRIETARY_QCT_DIR    ${LEGATO_ROOT}/proprietary/qct)

    set(ADDITIONAL_INTERFACE_SEARCH_DIRECTIVES
        -i ${LEGATO_GNSS_PA}
        -i ${LEGATO_MODEM_PA}
        -i ${LEGATO_PROPRIETARY_QCT_DIR}/inc/framework
        -i ${LEGATO_PROPRIETARY_QCT_DIR}/inc/services
        )

else()

    message(FATAL_ERROR "Legato GNSS Platform Adaptor type '${LEGATO_COMPONENTS_GNSS}' not supported.")

endif()


mkapp(  positioningService.adef
            -i ${LEGATO_ROOT}/interfaces/positioning
            -i ${LEGATO_ROOT}/interfaces/audio
            -i ${LEGATO_ROOT}/interfaces/modemServices
            -i ${POSITIONING_COMPONENTS}/platformAdaptor/inc
            -i ${MODEM_COMPONENTS}/platformAdaptor/inc
            -i ${LEGATO_ROOT}/components/cfgEntries
            -c ${LEGATO_ROOT}/components
            -i ${LEGATO_ROOT}/components
            -c ${POSITIONING_COMPONENTS}
            ${ADDITIONAL_INTERFACE_SEARCH_DIRECTIVES}
            ${LINK_DIRECTIVES}
        DEPENDS
            ${APIS}
            ${GNSS_DAEMON_FILES}
            ${PA_FILES}
        )
