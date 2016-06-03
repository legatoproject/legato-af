#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

if(LEGATO_COMPONENTS_AUDIO EQUAL "ar7")

    mkapp(paAudioAr7Test.adef
          -i ${LEGATO_ROOT}/components/audio/platformAdaptor/inc
          -i ${LEGATO_ROOT}/components/audio
          -i ${LEGATO_PROPRIETARY_QCT_DIR}/inc/framework
          -i ${LEGATO_PROPRIETARY_QCT_DIR}/inc/services
    )

endif()
