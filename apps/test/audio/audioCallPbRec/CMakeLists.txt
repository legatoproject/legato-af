#*******************************************************************************
# Copyright (C) 2014, Sierra Wireless Inc.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

if( (LEGATO_COMPONENTS_AUDIO MATCHES "wp7") OR (LEGATO_COMPONENTS_AUDIO MATCHES "ar7") )

    # Creates application from audioCallPbRecApp.adef
    mkapp(audioCallPbRecApp.adef
        -i ${LEGATO_ROOT}/interfaces/modemServices
        -i ${LEGATO_ROOT}/interfaces/audio
        -i ${LEGATO_ROOT}/components/audio/platformAdaptor
        -i ${LEGATO_ROOT}/components/modemServices/platformAdaptor/inc
        -i ${LEGATO_ROOT}/components/cfgEntries
        -i ${LEGATO_ROOT}/proprietary/qct/inc/framework
        -i ${LEGATO_ROOT}/proprietary/qct/inc/services
        --ldflags=-lqmi_cci
        --ldflags=-lqmiservices
        --ldflags=-lloc_api
        --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qapi.a
        DEPENDS
            legato
    )

endif()
