#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************


mkapp(imaFileClient.adef)
mkapp(imaFileServer.adef)

# This is a C test
add_dependencies(tests_c
                 imaFileClient
                 imaFileServer
                 )
