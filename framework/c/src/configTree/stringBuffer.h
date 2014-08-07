
// -------------------------------------------------------------------------------------------------
/**
 *  @file stringBuffer.h
 *
 *  Helper code for maintaining largish buffers of string memory.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved.
 *  Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_STRING_BUFFER_INCLUDE_GUARD
#define CFG_STRING_BUFFER_INCLUDE_GUARD




#define SB_SIZE ((size_t)512)




//--------------------------------------------------------------------------------------------------
/**
 *  Init the buffer pool this code depends on.
 */
//--------------------------------------------------------------------------------------------------
void sb_Init
(
    void
);




//--------------------------------------------------------------------------------------------------
/**
 *  Allocate a new string from our pool.
 *
 *  @return Pointer to the new string buffer.
 */
//--------------------------------------------------------------------------------------------------
char* sb_Get
(
    void
);




//--------------------------------------------------------------------------------------------------
/**
 *  Crate a new copy of an existing string buffer object.
 *
 *  @return a new copy of the given string.
 */
//--------------------------------------------------------------------------------------------------
char* sb_NewCopy
(
    const char* stringPtr  ///< [IN] The buffer to duplicate.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Release back the string into the pool.
 */
//--------------------------------------------------------------------------------------------------
void sb_Release
(
    char* bufferPtr  ///< [IN] The string to release.
);




#endif
