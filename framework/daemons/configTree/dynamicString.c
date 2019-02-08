
// -------------------------------------------------------------------------------------------------
/**
 *  @file dynamicString.c
 *
 *  A memory pool backed dynamic string API.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "dynamicString.h"




/// This value is stored in the string header block so that the access functions can make sure that
/// the string is valid.
#define HEADER_MAGIC 0xdca00acd




/// This macro will use the HEADER_MAGIC value to perform a sanity check on a string object supplied
/// to this API.
#define VALIDATE_HEADER(strPtr) \
    LE_FATAL_IF((strPtr) == NULL, "Trying to access a NULL dynamic string."); \
    LE_FATAL_IF((strPtr)->head.magic != HEADER_MAGIC, "Corrupted dynamic string detected.");




//--------------------------------------------------------------------------------------------------
/**
 *  Node that represents the head of a dynamic string.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int32_t magic;       ///< Safety value.  If this isn't set to HEADER_MAGIC then the string is
                         ///<   invalid.
    le_sls_List_t list;  ///< The list of the segments that this string is made up of.

    // TODO: Other quick access stats as needed.  Like byte and char counts.
}
HeadNode_t;




/// Define how big the text in a segment is.
#define SEGMENT_SIZE (size_t)32




//--------------------------------------------------------------------------------------------------
/**
 *  Strings are made up of pool-allocated segments.  The SEGMENT_SIZE should be tuned for optimal
 *  efficiency.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char value[SEGMENT_SIZE];  ///< Buffer to hold the actual text of the string.  Each buffer is
                               ///<   expected to be NULL terminated.
    le_sls_Link_t link;        ///< Link to the next node in the chain.
}
BodyNode_t;




//--------------------------------------------------------------------------------------------------
/**
 *  These blocks are either string headers or string segments.  This is used so that we can get away
 *  with using only one memory pool.
 *
 *  TODO: Decide if it's worth it staying with this approach, or if it's better to use two pools
 *        afterall.
 */
//--------------------------------------------------------------------------------------------------
typedef struct Dstr
{
    union
    {
        HeadNode_t head;
        BodyNode_t body;
    };
}
Dstr_t;




/// This pool is used to manage the memory used by the dynamic strings.
static le_mem_PoolRef_t DynamicStringPoolRef = NULL;


/// Name of the dynamic string memory pool.
LE_MEM_DEFINE_STATIC_POOL(dynamicStringPool,
                          LE_CONFIG_CFGTREE_MAX_DSTRING_POOL_SIZE,
                          sizeof(Dstr_t));


//--------------------------------------------------------------------------------------------------
/**
 *  Create a new, blank segment ready to be inserted into a string.
 *
 *  @return A pointer to the newly allocated and initialized segment.
 */
//--------------------------------------------------------------------------------------------------
static dstr_Ref_t NewSegment
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    dstr_Ref_t newSegmentRef = le_mem_ForceAlloc(DynamicStringPoolRef);

    memset(newSegmentRef->body.value, 0, SEGMENT_SIZE);
    newSegmentRef->body.link = LE_SLS_LINK_INIT;

    return newSegmentRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the first segment containing actual string data.  The usage pattern is to first call
 *  FirstSegmentRef on a string.  Then later as you are iterating call NextSegmentRef to return
 *  following segments.  The iteration is complete when NextSegmentRef returns NULL.
 *
 *  @return A pointer to the first segment chunk that can contain string data.  Or NULL if the
 *          string contains no data.
 */
//--------------------------------------------------------------------------------------------------
static dstr_Ref_t FirstSegmentRef
(
    dstr_Ref_t headRef  ///< [IN] The head of the string.
)
//--------------------------------------------------------------------------------------------------
{
    VALIDATE_HEADER(headRef);

    le_sls_Link_t* linkPtr = le_sls_Peek(&headRef->head.list);

    if (linkPtr == NULL)
    {
        return NULL;
    }

    return CONTAINER_OF(linkPtr, Dstr_t, body.link);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Function used while iterating the contents of a dynamic string.  Called to get the next segment
 *  of a string.
 *
 *  @return A pointer to the next segment pointer in the chain.  NULL otherwise.
 */
//--------------------------------------------------------------------------------------------------
static dstr_Ref_t NextSegmentRef
(
    dstr_Ref_t headRef,    ///< [IN] The head of the string.
    dstr_Ref_t currentPtr  ///< [IN] The current sub-section of the string.
)
//--------------------------------------------------------------------------------------------------
{
    VALIDATE_HEADER(headRef);
    LE_FATAL_IF(currentPtr == NULL, "Fatal corrupted string iteration detected.");

    le_sls_Link_t* linkPtr = le_sls_PeekNext(&headRef->head.list, &currentPtr->body.link);

    if (linkPtr == NULL)
    {
        return NULL;
    }

    return CONTAINER_OF(linkPtr, Dstr_t, body.link);
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the first string segment of a string.  If there isn't one, create it and return that
 *  instead.
 *
 *  @return A pointer to the first segment in the string.
 */
//--------------------------------------------------------------------------------------------------
static dstr_Ref_t NewOrFirstSegmentRef
(
    dstr_Ref_t headRef  ///< [IN] The head of the string.
)
//--------------------------------------------------------------------------------------------------
{
    dstr_Ref_t segmentRef = FirstSegmentRef(headRef);

    if (segmentRef != NULL)
    {
        return segmentRef;
    }

    segmentRef = NewSegment();
    le_sls_Stack(&headRef->head.list, &segmentRef->body.link);

    return segmentRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Get the next segment in the string after currentPtr.  If one doesn't exist, one is created and
 *  appended to the string.
 */
//--------------------------------------------------------------------------------------------------
static dstr_Ref_t NewOrNextSegmentRef
(
    dstr_Ref_t headRef,    ///< [IN] The head of the string.
    dstr_Ref_t currentPtr  ///< [IN] The current sub-section of the string.
)
//--------------------------------------------------------------------------------------------------
{
    dstr_Ref_t segmentRef = NextSegmentRef(headRef, currentPtr);

    if (segmentRef != NULL)
    {
        return segmentRef;
    }

    segmentRef = NewSegment();
    le_sls_AddAfter(&headRef->head.list, &currentPtr->body.link, &segmentRef->body.link);

    return segmentRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Truncate any remaining string segments after currentPtr.
 */
//--------------------------------------------------------------------------------------------------
static void FreeAnyAfter
(
    dstr_Ref_t headRef,    ///< [IN] The head of the string.
    dstr_Ref_t currentPtr  ///< [IN] The current sub-section of the string.
)
//--------------------------------------------------------------------------------------------------
{
    VALIDATE_HEADER(headRef);
    LE_FATAL_IF(currentPtr == NULL, "Corrupted string free encountered.");

    le_sls_Link_t* linkPtr;

    while ((linkPtr = le_sls_PeekNext(&headRef->head.list, &currentPtr->body.link)) != NULL)
    {
        dstr_Ref_t segmentRef = CONTAINER_OF(linkPtr, Dstr_t, body.link);

        le_sls_RemoveAfter(&headRef->head.list, &currentPtr->body.link);
        le_mem_Release(segmentRef);
    }
}




//--------------------------------------------------------------------------------------------------
/**
 *  Init the dynamic string API and the internal memory resources it depends on.
 */
//--------------------------------------------------------------------------------------------------
void dstr_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    LE_DEBUG("** Initialize Dynamic String subsystem.");

    DynamicStringPoolRef = le_mem_InitStaticPool(dynamicStringPool,
                                                 LE_CONFIG_CFGTREE_MAX_DSTRING_POOL_SIZE,
                                                 sizeof(Dstr_t));
    le_mem_SetNumObjsToForce(DynamicStringPoolRef, 100);    // Grow in chunks of 100 blocks.
}




//--------------------------------------------------------------------------------------------------
/**
 *  Create a new and empty dynamic string.
 */
//--------------------------------------------------------------------------------------------------
dstr_Ref_t dstr_New
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    dstr_Ref_t newHeadRef = le_mem_ForceAlloc(DynamicStringPoolRef);

    newHeadRef->head.magic = HEADER_MAGIC;
    newHeadRef->head.list = LE_SLS_LIST_INIT;

    return newHeadRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Create a new dynamic string, but make it a copy of the existing C-String.
 */
//--------------------------------------------------------------------------------------------------
dstr_Ref_t dstr_NewFromCstr
(
    const char* originalStrPtr  ///< [IN] The orignal C-String to copy.
)
//--------------------------------------------------------------------------------------------------
{
    dstr_Ref_t newStringRef = dstr_New();

    dstr_CopyFromCstr(newStringRef, originalStrPtr);
    return newStringRef;
}




//--------------------------------------------------------------------------------------------------
/**
 *  Create a new dynamic string that is a copy of a pre-existing one.
 */
//--------------------------------------------------------------------------------------------------
dstr_Ref_t dstr_NewFromDstr
(
    const dstr_Ref_t originalStrPtr  ///< [IN] The original dynamic string to copy.
)
//--------------------------------------------------------------------------------------------------
{
    dstr_Ref_t newStringRef = dstr_New();

    dstr_Copy(newStringRef, originalStrPtr);
    return newStringRef;
}



//--------------------------------------------------------------------------------------------------
/**
 *  Free a dynamic string and return it's memory to the pool from whence it came.
 */
//--------------------------------------------------------------------------------------------------
void dstr_Release
(
    dstr_Ref_t strRef  ///< [IN] The dynamic string to free.
)
//--------------------------------------------------------------------------------------------------
{
    dstr_Ref_t segmentRef = FirstSegmentRef(strRef);

    while (segmentRef != NULL)
    {
        dstr_Ref_t nextSegmentRef = NextSegmentRef(strRef, segmentRef);

        le_mem_Release(segmentRef);
        segmentRef = nextSegmentRef;
    }

    le_mem_Release(strRef);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    dstr_Ref_t segmentRef = NULL;

    if (totalCopied)
    {
        *totalCopied = 0;
    }

    for (segmentRef = FirstSegmentRef(sourceStrRef);
         segmentRef != NULL;
         segmentRef = NextSegmentRef(sourceStrRef, segmentRef))
    {
        size_t bytesCopied = 0;
        le_result_t result = le_utf8_Copy(destStrPtr,
                                          segmentRef->body.value,
                                          destStrMax,
                                          &bytesCopied);

        if (totalCopied)
        {
            *totalCopied += bytesCopied;
        }

        if (result == LE_OVERFLOW)
        {
            return LE_OVERFLOW;
        }

        LE_FATAL_IF(result != LE_OK, "Unexpected result code returned, %s.", LE_RESULT_TXT(result));

        destStrMax -= bytesCopied;
        destStrPtr += bytesCopied;

    }

    return LE_OK;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;
    dstr_Ref_t destSegmentRef = NewOrFirstSegmentRef(destStrRef);

    do
    {
        size_t bytesCopied = 0;

        result = le_utf8_Copy(destSegmentRef->body.value,
                              sourceStrPtr,
                              SEGMENT_SIZE,
                              &bytesCopied);

        sourceStrPtr += bytesCopied;

        if (result == LE_OVERFLOW)
        {
            destSegmentRef = NewOrNextSegmentRef(destStrRef, destSegmentRef);
        }
    }
    while (result == LE_OVERFLOW);

    FreeAnyAfter(destStrRef, destSegmentRef);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    dstr_Ref_t sourceSegmentRef = FirstSegmentRef(sourceStrPtr);
    dstr_Ref_t destSegmentRef = NewOrFirstSegmentRef(destStrPtr);

    while (sourceSegmentRef != NULL)
    {
        memcpy(destSegmentRef->body.value, sourceSegmentRef->body.value, SEGMENT_SIZE);

        sourceSegmentRef = NextSegmentRef(sourceStrPtr, sourceSegmentRef);
        if (sourceSegmentRef != NULL)
        {
            destSegmentRef = NewOrNextSegmentRef(destStrPtr, destSegmentRef);
        }
    }

    FreeAnyAfter(destStrPtr, destSegmentRef);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    // If we're dealing with a NULL node string the consider it empty.
    if (strRef == NULL)
    {
        return true;
    }

    // Also, consider the string empty if the first character is NULL.
    dstr_Ref_t segmentRef = FirstSegmentRef(strRef);
    return (segmentRef == NULL) || (segmentRef->body.value[0] == '\0');
}




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
)
//--------------------------------------------------------------------------------------------------
{
    dstr_Ref_t segmentRef = NULL;
    size_t count = 0;

    for (segmentRef = FirstSegmentRef(strRef);
         segmentRef != NULL;
         segmentRef = NextSegmentRef(strRef, segmentRef))
    {
        ssize_t localCount = le_utf8_NumChars(segmentRef->body.value);

        if (localCount == LE_FORMAT_ERROR)
        {
            return 0;
        }

        count += localCount;
    }

    return count;
}




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
)
//--------------------------------------------------------------------------------------------------
{
    dstr_Ref_t segmentRef = NULL;
    size_t count = 0;

    for (segmentRef = FirstSegmentRef(strRef);
         segmentRef != NULL;
         segmentRef = NextSegmentRef(strRef, segmentRef))
    {
        count += le_utf8_NumBytes(segmentRef->body.value);
    }

    return count;
}
