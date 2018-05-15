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

mkapp(  secStoreTest2Global.adef
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_ROOT}/interfaces/le_secStore.api
            secStoreTest2/*
     )

mkapp(  secStoreTestGlobal.adef
        DEPENDS
            ## TODO: Remove all this when the mk tools do dependency checking.
            ${LEGATO_ROOT}/interfaces/le_secStore.api
            secStoreTestGlobal/*
     )

if ($ENV{TARGET} MATCHES "localhost")
    add_subdirectory(secStoreUnitTest)
endif()

# This is a C test
add_dependencies(tests_c secStoreTest1a
                         secStoreTest1b
                         secStoreTest2
                         secStoreTest2Global
                         secStoreTestGlobal)
