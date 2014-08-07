#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

mkexe(  appCfgInstall
            appConfigInstall.c
            -c ${CMAKE_CURRENT_SOURCE_DIR}/components
            appConfig
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            appConfigInstall.c
            components/appConfig/Component.cdef
            components/appConfig/inc/configInstaller.h
            components/appConfig/src/configInstaller.c
            )

mkexe(  appCfgRemove
            appConfigRemove.c
            -c ${CMAKE_CURRENT_SOURCE_DIR}/components
            appConfig
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            appConfigRemove.c
            components/appConfig/Component.cdef
            components/appConfig/inc/configInstaller.h
            components/appConfig/src/configInstaller.c
            )

mkexe(  appUserAdd
            appUserAddRemove.c
            --cflags=-DADD_USER
            -c ${CMAKE_CURRENT_SOURCE_DIR}/components
            userAdderRemover
            -i ../../c/src
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            appUserAddRemove.c
            components/userAdderRemover/Component.cdef
            components/userAdderRemover/userAdderRemover.h
            components/userAdderRemover/userAdderRemover.c
            )


mkexe(  appUserRemove
            appUserAddRemove.c
            --cflags=-DREMOVE_USER
            -c ${CMAKE_CURRENT_SOURCE_DIR}/components
            userAdderRemover
            -i ../../c/src
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            appUserAddRemove.c
            components/userAdderRemover/Component.cdef
            components/userAdderRemover/userAdderRemover.h
            components/userAdderRemover/userAdderRemover.c
            )
