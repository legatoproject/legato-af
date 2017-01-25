#*******************************************************************************
# Copyright (C) Sierra Wireless Inc.
#*******************************************************************************

mkapp(  secStoreTest1a.adef
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_ROOT}/interfaces/le_secStore.api
            secStoreTest1a/*
     )

mkapp(  secStoreTest1b.adef
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_ROOT}/interfaces/le_secStore.api
            secStoreTest1b/*
     )

mkapp(  secStoreTest2.adef
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_ROOT}/interfaces/le_secStore.api
            secStoreTest2/*
     )
