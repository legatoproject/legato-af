#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

find_package(Legato REQUIRED)

if(LEGATO_COMPONENTS_AUDIO MATCHES "ar7")

    mkapp(paAudioAr7Test.adef
          -i ${LEGATO_ROOT}/interfaces/audio
          -i ${LEGATO_ROOT}/components/audio/platformAdaptor
          -i ${LEGATO_ROOT}/proprietary/qct/inc/framework
          -i ${LEGATO_ROOT}/proprietary/qct/inc/services
          --ldflags=-lqmi_cci
          --ldflags=-lqmiservices
          --ldflags=${TARGET_SYSROOT}/usr/lib/libsierra_qcsi.a
        DEPENDS
            legato
            paAudioAr7TestComp/Component.cdef
            paAudioAr7TestComp/testPaAudioAr7.c
    )

endif()
