#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

mkapp(testAppInfo.adef
      -i ${LEGATO_ROOT}/interfaces/supervisor
      DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ../*
            getAppInfo/*
            ${LEGATO_ROOT}/interfaces/supervisor/*
            testAppInfo.adef )

# This is a C test
add_dependencies(tests_c testAppInfo)
