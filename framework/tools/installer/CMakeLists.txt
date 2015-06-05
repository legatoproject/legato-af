#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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


mkexe(  appSetFilePermissions
            appSetFilePermissions.c
            -c ${CMAKE_CURRENT_SOURCE_DIR}/components
            -i ${PROJECT_SOURCE_DIR}/framework/c/src
            -i ${PROJECT_SOURCE_DIR}/framework/c/src/serviceDirectory
            -i ${LEGATO_ROOT}/interfaces/supervisor
            ${CFLAGS_TARGET_IMPORTS}
            filePermissions
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            appSetFilePermissions.c
            components/filePermissions/Component.cdef
            components/filePermissions/filePermissions.h
            components/filePermissions/filePermissions.c
            ${LEGATO_ROOT}/interfaces/supervisor/*
            )
