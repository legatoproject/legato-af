
#---------------------------------------------------------------------------------------------------
#  Build script for the configTree system service.
#
#  Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved.
#  Use of this work is subject to license.
#---------------------------------------------------------------------------------------------------

mkexe(  configTree
            ../configTree
            -i ..
            --cflags=-std=c99
            -v
        DEPENDS
            configTree.c
            configTreeApi.c
            configTreeAdminApi.c
            stringBuffer.c
            dynamicString.c
            requestQueue.c
            nodeIterator.c
            treeIterator.c
            treePath.c
            treeUser.c
            internalConfig.c
            treeDb.c
        )
