
// -------------------------------------------------------------------------------------------------
/*
 *  Helper code for maintaining largish buffers of string memory.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "stringBuffer.h"




//--------------------------------------------------------------------------------------------------
/**
 *  Our pool for data strings.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t StringBufferPoolRef = NULL;

#define CFG_STRING_BUFFER_POOL "configTree.stringPool"




//--------------------------------------------------------------------------------------------------
/**
 *  Init the buffer pool this code depends on.
 */
//--------------------------------------------------------------------------------------------------
void sb_Init
(
)
{
    StringBufferPoolRef = le_mem_CreatePool(CFG_STRING_BUFFER_POOL, SB_SIZE);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Allocate a new string from our pool.
 */
//--------------------------------------------------------------------------------------------------
char* sb_Get
(
)
{
    char* buffer = le_mem_ForceAlloc(StringBufferPoolRef);
    memset(buffer, 0, SB_SIZE);

    return buffer;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Crate a new copy of an existing string buffer object.
 */
//--------------------------------------------------------------------------------------------------
char* sb_NewCopy
(
    const char* stringPtr  ///< The buffer to duplicate.
)
{
    char* bufferPtr = sb_Get();
    strncpy(bufferPtr, stringPtr, SB_SIZE);

    return bufferPtr;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Release back the string into the pool.
 */
//--------------------------------------------------------------------------------------------------
void sb_Release
(
    char* bufferPtr  ///< The string to release.
)
{
    le_mem_Release(bufferPtr);
}
