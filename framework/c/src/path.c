//--------------------------------------------------------------------------------------------------
/** @file path.c
 *
 * Implements the path API.  The path iterator stores the location of the path string and the
 * separator string and assumes that the user will not change them during the lifetime of the
 * iterator.
 *
 * Separators can be one or more characters.  Iterators treat consecutive separators in a path as a
 * single separator.  Paths that begin with one or more separators are considered absolute paths.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Path iterator type.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char* pathPtr;          // Pointer to the path string.
    char* separatorPtr;     // Pointer to the separator string.
    ssize_t firstNodeIndex; // Index of the first node in the path.
    ssize_t currNodeIndex;  // Index of the current node in the path.
}
PathIterator_t;


//--------------------------------------------------------------------------------------------------
/**
 * Path iterator memory pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t IteratorPool;


//--------------------------------------------------------------------------------------------------
/**
 * The expected maximum number of iterators.  This is not a hard limit and is used only for creating
 * the safe reference maps.
 */
//--------------------------------------------------------------------------------------------------
#define EXPECTED_MAX_NUM_ITERATORS      10


//--------------------------------------------------------------------------------------------------
/**
 * The safe reference maps for iterators.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t IteratorMap;


//--------------------------------------------------------------------------------------------------
/**
 * Finds the index of the next character that is not a separator in the str.
 *
 * @return
 *      The index of the first character that is not a separator.  This might be the null-terminator.
 */
//--------------------------------------------------------------------------------------------------
static size_t FindNextPathCharIndex
(
    const char* str,        // The string to search.
    const char* sepPtr      // The separator string.
)
{
    size_t sepStrLen = le_utf8_NumBytes(sepPtr);
    size_t i = 0;

    while (1)
    {
        if (strncmp(str + i, sepPtr, sepStrLen) == 0)
        {
            i += sepStrLen;
        }
        else
        {
            return i;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches backwards to find the index of the next character that is not a separator in the str.
 *
 * @return
 *      The index of the first character that is not a separator.
 *      -1 if there are no characters that is not a separator.
 */
//--------------------------------------------------------------------------------------------------
static ssize_t ReverseFindNextPathCharIndex
(
    const char* str,        // The string to search.
    const char* sepPtr      // The separator string.
)
{
    size_t sepStrLen = le_utf8_NumBytes(sepPtr);
    ssize_t i = le_utf8_NumBytes(str) - sepStrLen;

    while (i > 0)
    {
        if (strncmp(str + i, sepPtr, sepStrLen) == 0)
        {
            i -= sepStrLen;
        }
        else
        {
            // Move back to the 1 minus the previous known separator.
            return i + sepStrLen - 1;
        }
    }

    return -1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches backwards in str starting at endIndex for subPtr and returns the index of the character
 * following the end of the substr.
 *
 * @return
 *      The index of character following the end of the subPtr.
 *      Zero is returned if the subPtr is not found.
 */
//--------------------------------------------------------------------------------------------------
static size_t GetEndOfLastSubstr
(
    const char* strPtr,     // The string to search.
    const char* subStrPtr,  // The sub-string to search for.
    size_t endIndex         // The index to start search for moving bacwards.
)
{
    size_t subStrLen = le_utf8_NumBytes(subStrPtr);
    ssize_t i = endIndex - (subStrLen - 1);

    while (i >= 0)
    {
        if (memcmp(strPtr + i, subStrPtr, subStrLen) == 0)
        {
            return i + subStrLen;
        }

        i--;
    }

    return 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next node in the path starting from a specified index.
 *
 * If successful the iterator's currNodeIndex is set to the next node after the one that was just
 * accessed.  If there is no next node then currNodeIndex is set to the null-termninator.
 *
 * If the node buffer is too small, the portion of the node that will fit is copied to the node
 * buffer and LE_OVERFLOW.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the node buffer was too small.
 *      LE_NOT_FOUND if there are no more nodes after the index.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetNextNodeFromIndex
(
    PathIterator_t* iteratorPtr,        ///< [IN] The path iterator.
    size_t pathIndex,                   ///< [IN] The index into the path to start from.
    char* nodePtr,                      ///< [OUT] The buffer to store the node string.
    size_t nodeBuffSize                 ///< [IN] The size of the node buffer in bytes.
)
{
    char* pathPtr = iteratorPtr->pathPtr + pathIndex;

    if (pathPtr[0] == '\0')
    {
        return LE_NOT_FOUND;
    }
    else
    {
        char* nextSepPtr = strstr(pathPtr, iteratorPtr->separatorPtr);

        if (nextSepPtr == NULL)
        {
            // No separators were found.  Set the current node index to the null-terminator.
            iteratorPtr->currNodeIndex = le_utf8_NumBytes(iteratorPtr->pathPtr);

            // Copy the entire string.
            return le_utf8_Copy(nodePtr, pathPtr, nodeBuffSize, NULL);
        }
        else
        {
            le_result_t result;
            size_t numBytesInNode = nextSepPtr - pathPtr;

            if (numBytesInNode >= nodeBuffSize)
            {
                // Copy up to the first separator.  Doesn't matter what the return value is we know
                // there was an overflow.
                le_utf8_Copy(nodePtr, pathPtr, nodeBuffSize, NULL);

                result = LE_OVERFLOW;
            }
            else
            {
                // Copy up to the first separator.  Doesn't matter what the return value is we know
                // the copy was OK.
                le_utf8_Copy(nodePtr, pathPtr, numBytesInNode + 1, NULL);

                result = LE_OK;
            }

            // Set the current node index to the beginning of the next node.
            iteratorPtr->currNodeIndex = pathIndex + numBytesInNode +
                                         FindNextPathCharIndex(pathPtr + numBytesInNode,
                                                               iteratorPtr->separatorPtr);

            return result;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the path system.
 */
//--------------------------------------------------------------------------------------------------
void path_Init
(
    void
)
{
    IteratorPool = le_mem_CreatePool("PathIteratorPool", sizeof(PathIterator_t));

    // Create a Safe Reference Map to use for iterators.
    IteratorMap = le_ref_CreateMap("PathIteratorMap", EXPECTED_MAX_NUM_ITERATORS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a path iterator.  The path and separator strings must be unchanged during the life time
 * of the interator.  Operations on the iterator are undefined if either the path or separator
 * strings change.
 *
 * The path and separator strings must be null-terminated UTF-8 strings.  The separator string must
 * be non empty.
 *
 * @return
 *      A reference to the path iterator.
 */
//--------------------------------------------------------------------------------------------------
le_path_IteratorRef_t le_path_iter_Create
(
    const char* pathPtr,        ///< [IN] The path string.
    const char* separatorPtr    ///< [IN] Separator string.
)
{
    // Check parameters.
    LE_ASSERT( (pathPtr != NULL) && (separatorPtr != NULL) && (le_utf8_NumBytes(separatorPtr) > 0) );

    // Create the iterator.
    PathIterator_t* iteratorPtr = le_mem_ForceAlloc(IteratorPool);

    // Initialize the iterator.
    iteratorPtr->pathPtr = (char*)pathPtr;
    iteratorPtr->separatorPtr = (char*)separatorPtr;
    iteratorPtr->firstNodeIndex = FindNextPathCharIndex(pathPtr, separatorPtr);
    iteratorPtr->currNodeIndex = iteratorPtr->firstNodeIndex;

    // Create and return a Safe Reference for this iterator.
    return le_ref_CreateRef(IteratorMap, iteratorPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the first node in the path.
 *
 * If the node buffer is too small, the portion of the node that will fit is copied to the node
 * buffer and LE_OVERFLOW is returned.
 *
 * If the path is empty then LE_NOT_FOUND is returned and nothing is copied to nodePtr.
 *
 * @return
 *      LE_OK if the entire node was copied.
 *      LE_OVERFLOW if the node buffer was too small.
 *      LE_NOT_FOUND if no nodes are available.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_path_iter_GetFirstNode
(
    le_path_IteratorRef_t iteratorRef,  ///< [IN] The path iterator reference.
    char* nodePtr,                      ///< [OUT] The buffer to store the node string.
    size_t nodeBuffSize                 ///< [IN] The size of the node buffer in bytes.
)
{
    // Check parameters.
    LE_ASSERT( (nodePtr != NULL) && (nodeBuffSize > 0) );

    PathIterator_t* iteratorPtr = le_ref_Lookup(IteratorMap, iteratorRef);
    LE_ASSERT(iteratorPtr != NULL);

    return GetNextNodeFromIndex(iteratorPtr, iteratorPtr->firstNodeIndex, nodePtr, nodeBuffSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next node in the path.  Gets the node after the node that was most recently accessed.
 * Consecutive separators are treated as a single separator.
 *
 * If no other nodes have been accessed then the first node is copied to nodePtr.
 *
 * If the node buffer is too small, the portion of the node that will fit is copied to the node
 * buffer and LE_OVERFLOW is returned.
 *
 * If there are no more nodes then LE_NOT_FOUND is returned nothing is copied to nodePtr.
 *
 * @return
 *      LE_OK if succesful.
 *      LE_OVERFLOW if the nodePtr buffer is too small.
 *      LE_NOT_FOUND if no more nodes are available.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_path_iter_GetNextNode
(
    le_path_IteratorRef_t iteratorRef,  ///< [IN] The path iterator reference.
    char* nodePtr,                      ///< [OUT] The buffer to store the node string.
    size_t nodeBuffSize                 ///< [IN] The size of the node buffer in bytes.
)
{
    // Check parameters.
    LE_ASSERT( (nodePtr != NULL) && (nodeBuffSize > 0) );

    PathIterator_t* iteratorPtr = le_ref_Lookup(IteratorMap, iteratorRef);
    LE_ASSERT(iteratorPtr != NULL);

    return GetNextNodeFromIndex(iteratorPtr, iteratorPtr->currNodeIndex, nodePtr, nodeBuffSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Determines if the path is absolute (begins with a separator) or relative (does not begin with a
 * separator.
 *
 * @return
 *      true if the path is absolute.
 *      false if the path is relative.
 */
//--------------------------------------------------------------------------------------------------
bool le_path_iter_IsAbsolute
(
    le_path_IteratorRef_t iteratorRef   ///< [IN] The path iterator reference.
)
{
    PathIterator_t* iteratorPtr = le_ref_Lookup(IteratorMap, iteratorRef);
    LE_ASSERT(iteratorPtr != NULL);

    return !(iteratorPtr->firstNodeIndex == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a path iterator.
 */
//--------------------------------------------------------------------------------------------------
void le_path_iter_Delete
(
    le_path_IteratorRef_t iteratorRef   ///< [IN] The path iterator reference.
)
{
    // Check parameters.
    PathIterator_t* iteratorPtr = le_ref_Lookup(IteratorMap, iteratorRef);
    LE_ASSERT(iteratorPtr != NULL);

    // Invalidate the Safe Reference.
    le_ref_DeleteRef(IteratorMap, iteratorRef);

    le_mem_Release(iteratorPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the directory, which is the entire path up to and including the last separator.
 *
 * @return
 *      LE_OK if succesful.
 *      LE_OVERFLOW if the dirPtr buffer is too small.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_path_GetDir
(
    const char* pathPtr,            ///< [IN] The path string.
    const char* separatorPtr,       ///< [IN] The separator string.
    char* dirPtr,                   ///< [OUT] The buffer to store the directory string.
    size_t dirBuffSize              ///< [IN] The size of the directory buffer in bytes.
)
{
    // Check parameters.
    LE_ASSERT( (pathPtr != NULL) && (separatorPtr != NULL) && (dirPtr != NULL) && (dirBuffSize > 0));

    size_t i = GetEndOfLastSubstr(pathPtr, separatorPtr, le_utf8_NumBytes(pathPtr));

    if (i == 0)
    {
        // Copy the entire path.
        return le_utf8_Copy(dirPtr, pathPtr, dirBuffSize, NULL);
    }
    if (i >= dirBuffSize)
    {
        // The return value does not matter because the overflow is intentional.
        le_utf8_Copy(dirPtr, pathPtr, dirBuffSize, NULL);

        return LE_OVERFLOW;
    }
    else
    {
        // The return value does not matter because the overflow is intentional.
        le_utf8_Copy(dirPtr, pathPtr, i + 1, NULL);

        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the basename (the last node in the path).  This function gets the basename by
 * simply returning a pointer to the character following the last separator.
 *
 * @return
 *      A pointer to the character following the last separator.
 */
//--------------------------------------------------------------------------------------------------
char* le_path_GetBasenamePtr
(
    const char* pathPtr,            ///< [IN] The path string.
    const char* separatorPtr        ///< [IN] The separator string.
)
{
    // Check parameters.
    LE_ASSERT( (pathPtr != NULL) && (separatorPtr != NULL) );

    return (char*)pathPtr + GetEndOfLastSubstr(pathPtr, separatorPtr, le_utf8_NumBytes(pathPtr));
}


//--------------------------------------------------------------------------------------------------
/**
 * Concatenates multiple path segments together.
 *
 * Copies all path segments into the provided buffer, ensuring that where segments are joined there
 * is only one separator between them.  Ending separators in the last path segments are also
 * dropped.
 *
 * @warning  The (char*)NULL at the end of the list of path segments is mandatory.  If this NULL is
 *           omitted the behaviour is undefined.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if there was not enough buffer space for all segments.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_path_Concat
(
    const char* separatorPtr,       ///< [IN] Separator string.
    char* bufPtr,                   ///< [OUT] Buffer to store the resultant path.
    size_t bufSize,                 ///< [IN] Buffer size.
    ...                             ///< [IN] Path segments to concatenate.  The list of segments
                                    ///       must be terminated by (char*)NULL.
)
{
    // Check parameters.
    LE_ASSERT( (bufPtr != NULL) && (separatorPtr != NULL) );

    // Initialize the buffer.
    if (bufSize > 0)
    {
        bufPtr[0] = '\0';
    }

    va_list varArgs;
    char* pathSegmentPtr;
    size_t bufIndex = 0;
    le_result_t result = LE_OK;
    size_t separatorSize = le_utf8_NumBytes(separatorPtr);

    // Go to the first var arg.
    va_start(varArgs, bufSize);

    while (1)
    {
        // Get the path segment from the next var arg.
        pathSegmentPtr = va_arg(varArgs, char*);

        if (pathSegmentPtr == NULL)
        {
            // No more segments.
            break;
        }

        // Get the part of the path segment not inlcuding starting and ending separators.
        size_t segStartIndex = FindNextPathCharIndex(pathSegmentPtr, separatorPtr);
        ssize_t segEndIndex = ReverseFindNextPathCharIndex(pathSegmentPtr, separatorPtr);

        // Add the separator unless the first segment does not have a leading separator.
        size_t numBytesWritten;
        if ((bufIndex != 0) || (segStartIndex != 0))
        {
            if (separatorSize >= bufSize - bufIndex)
            {
                // No more room to add the separator.
                result = LE_OVERFLOW;
                break;
            }

            // This should always succeed because we've checked the space above.
            LE_ASSERT(le_utf8_Copy(bufPtr + bufIndex,
                                   separatorPtr,
                                   bufSize - bufIndex,
                                   &numBytesWritten) == LE_OK);

            bufIndex += numBytesWritten;
        }

        // Copy the path segment.
        if ((segEndIndex - segStartIndex) + 2 < bufSize - bufIndex)
        {
            // This may overflow but is not an error so we don't check the return code here.
            le_utf8_Copy(bufPtr + bufIndex,
                         pathSegmentPtr + segStartIndex,
                         segEndIndex - segStartIndex + 2,
                         &numBytesWritten);
        }
        else
        {
            if (le_utf8_Copy(bufPtr + bufIndex,
                             pathSegmentPtr + segStartIndex,
                             bufSize - bufIndex,
                             &numBytesWritten) == LE_OVERFLOW)
            {
                // No more room in the buffer.
                result = LE_OVERFLOW;
                break;
            }
        }

        bufIndex += numBytesWritten;
    }

    va_end(varArgs);

    return result;
}
