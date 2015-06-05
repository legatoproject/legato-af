#*******************************************************************************
# Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
#*******************************************************************************

mkapp(  secStoreTest1.adef
            -i ${LEGATO_ROOT}/interfaces/secureStorage
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_ROOT}/interfaces/secureStorage/*
            secStoreTest1/*
     )


mkapp(  secStoreTest2.adef
            -i ${LEGATO_ROOT}/interfaces/secureStorage
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_ROOT}/interfaces/secureStorage/*
            secStoreTest2/*
     )
