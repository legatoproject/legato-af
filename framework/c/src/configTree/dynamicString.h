
// -------------------------------------------------------------------------------------------------
/**
 *  @file dynamicString.h
 *
 *  A memory pool backed dynamic string API.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#ifndef CFG_DYNAMIC_STRING_INCLUDE_GUARD
#define CFG_DYNAMIC_STRING_INCLUDE_GUARD




//--------------------------------------------------------------------------------------------------
/**
 *  The dynamic string object pointer.
 */
//--------------------------------------------------------------------------------------------------
typedef struct Dstr* dstr_Ref_t;




//--------------------------------------------------------------------------------------------------
/**
 *  Init the dynamic string API and the internal memory resources it depends on.
 */
//--------------------------------------------------------------------------------------------------
void dstr_Init
(
    void
);




//--------------------------------------------------------------------------------------------------
/**
 *  Create a new and empty dynamic string.
 */
//--------------------------------------------------------------------------------------------------
dstr_Ref_t dstr_New
(
    void
);




//--------------------------------------------------------------------------------------------------
/**
 *  Create a new dynamic string, but make it a copy of the existing C-String.
 */
//--------------------------------------------------------------------------------------------------
dstr_Ref_t dstr_NewFromCstr
(
    const char* originalStrPtr  ///< [IN] The orignal C-String to copy.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Create a new dynamic string that is a copy of a pre-existing one.
 */
//--------------------------------------------------------------------------------------------------
dstr_Ref_t dstr_NewFromDstr
(
    const dstr_Ref_t originalStrPtr  ///< [IN] The original dynamic string to copy.
);



//--------------------------------------------------------------------------------------------------
/**
 *  Free a dynamic string and return it's memory to the pool from whence it came.
 */
//--------------------------------------------------------------------------------------------------
void dstr_Release
(
    dstr_Ref_t strRef  ///< [IN] The dynamic string to free.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Copy the contents of a dynamic string into a regular C-style string.
 *
 *  @return LE_OK if the string fit properly within the bounds of the supplied string buffer.
 *          LE_OVERFLOW if the string had to be truncated during the copy.
 */
//--------------------------------------------------------------------------------------------------
le_result_t dstr_CopyToCstr
(
    char* destStrPtr,               ///< [OUT] The destiniation string buffer.
    size_t destStrMax,              ///< [IN]  The maximum string the buffer can handle.
    const dstr_Ref_t sourceStrRef,  ///< [IN]  The dynamic string to copy to said buffer.
    size_t* totalCopied             ///< [IN]  If supplied, this is the total number of bytes copied
                                    ///<       to the target string buffer.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Copy the contents from a C-style string into a dynamic string.  The dynamic string will
 *  automatically grow or shrink as required.
 */
//--------------------------------------------------------------------------------------------------
void dstr_CopyFromCstr
(
    dstr_Ref_t destStrRef,    ///< [OUT] The dynamic string to copy to.
    const char* sourceStrPtr  ///< [IN]  The C-style string to copy from.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Copy the contents from one dynamic string to another.  The destination string will automatically
 *  grow or shrink as required.
 */
//--------------------------------------------------------------------------------------------------
void dstr_Copy
(
    dstr_Ref_t destStrPtr,         ///< [OUT] The string to copy into.
    const dstr_Ref_t sourceStrPtr  ///< [IN]  The string to copy from.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Call to check the dynamic string if it's effectively empty.
 *
 *  @return A value of true if the string pointer is NULL or the data is an empty string.  False is
 *          returned otherwise.
 */
//--------------------------------------------------------------------------------------------------
bool dstr_IsNullOrEmpty
(
    const dstr_Ref_t strRef  ///< [IN] The dynamic string object to check.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the length of a dynamic string in utf-8 characters.
 *
 *  @return The length of the dynamic string, excluding a terminating NULL.
 */
//--------------------------------------------------------------------------------------------------
size_t dstr_NumChars
(
    const dstr_Ref_t strRef  ///< [IN] The dynamic string object to read.
);




//--------------------------------------------------------------------------------------------------
/**
 *  Get the length of a dynamic string, in bytes.
 *
 *  @return The length of the dynamic string in bytes, excluding a terminating NULL.
 */
//--------------------------------------------------------------------------------------------------
size_t dstr_NumBytes
(
    const dstr_Ref_t strRef  ///< [IN] The dynamic string object to read.
);




#endif
