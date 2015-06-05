#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

mkapp(installStatusTest.adef
      -i ${LEGATO_ROOT}/interfaces/updateDaemon
      DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ../*
            installNotify/*
            ${LEGATO_ROOT}/interfaces/updateDaemon/*
            installStatusTest.adef )

mkapp(testApp1.adef)
mkapp(testApp2.adef)
