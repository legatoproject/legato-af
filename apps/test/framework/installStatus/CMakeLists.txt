#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

mkapp(installStatusTest.adef
      DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ../*
            installNotify/*
            ${LEGATO_ROOT}/interfaces/*
            installStatusTest.adef )

mkapp(testApp1.adef)
mkapp(testApp2.adef)

# This is a C test
add_dependencies(tests_c installStatusTest testApp1 testApp2)
