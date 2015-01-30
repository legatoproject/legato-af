
//--------------------------------------------------------------------------------------------------
/** @file path.c
 *
 * Implements the path API.
 *
 * Separators can be one or more characters.  Path objects treat consecutive separators in a path as
 * a single separator.  Paths that begin with one or more separators are considered absolute paths.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 * license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"


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
 * Finds the index of the trailing separators.
 *
 * @return
 *      The index of the first trailing separator.
 *      The index of the NULL-terminator if there are no trailing separators.
 */
//--------------------------------------------------------------------------------------------------
static size_t FindTrailingSeparatorIndex
(
    const char* str,        // The string to search.
    const char* sepPtr      // The separator string.
)
{
    size_t sepStrLen = le_utf8_NumBytes(sepPtr);

    // Start at the end of the string.
    ssize_t i = le_utf8_NumBytes(str) - sepStrLen;

    while (i >= 0)
    {
        if (strncmp(str + i, sepPtr, sepStrLen) == 0)
        {
            i -= sepStrLen;
        }
        else
        {
            break;
        }
    }

    // Move back to the previous known separator.
    return i + sepStrLen;
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
    LE_ASSERT(   (pathPtr != NULL)
              && (separatorPtr != NULL)
              && (dirPtr != NULL)
              && (dirBuffSize > 0));

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
 * Remove duplicate trailing separators from the path.  If there are multiple trailing separators
 * then all trailing separators except one are removed.  If there are no trailing separators then
 * nothing is changed.
 *
 * @return
 *      true if duplicate trailing separators were successfully removed or there was already only a
 *           single trailing separator.
 *      false if there are no trailing separators in the path.
 */
//--------------------------------------------------------------------------------------------------
static bool RemoveDuplicateTrailingSep
(
    char* pathPtr,                  ///< [IN] The path string.
    const char* separatorPtr,       ///< [IN] The separator string.
    size_t* lenPtr                  ///< [OUT] The resultant strings length.
)
{
    size_t index = FindTrailingSeparatorIndex(pathPtr, separatorPtr);

    if (pathPtr[index] == '\0')
    {
        // There are no trailing separators.
        if (lenPtr != NULL)
        {
            *lenPtr = index;
        }

        return false;
    }

    // Move past the first trailing separator.
    index += le_utf8_NumBytes(separatorPtr);

    // Remove everything after the first trailing separator.
    pathPtr[index] = '\0';

    if (lenPtr != NULL)
    {
        *lenPtr = index;
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Concatenates multiple path segments together.
 *
 * Concatenates the path in the pathPtr buffer with all path segments and stores the result in the
 * pathPtr.  Ensures that where path segments are joined there is only one separator between them.
 * Duplicate trailing separators in the resultant path are also dropped.
 *
 * If there is not enough space in pathPtr for all segments, as many characters from the segments
 * that will fit in the buffer will be copied and LE_OVERFLOW will be returned.  Partial UTF-8
 * characters and partial separators will never be copied.
 *
 * @warning  The (char*)NULL at the end of the list of path segments is mandatory.  If this NULL is
 *           omitted the behaviour is undefined.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if there was not enough buffer space in pathPtr for all segments.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_path_Concat
(
    const char* separatorPtr,       ///< [IN] Separator string.
    char* pathPtr,                  ///< [IN/OUT] Buffer containing the first segment and where
                                    ///           the resultant path will be stored.
    size_t pathSize,                ///< [IN] Buffer size.
    ...                             ///< [IN] Path segments to concatenate.  The list of segments
                                    ///       must be terminated by (char*)NULL.
)
{
    // Check parameters.
    LE_ASSERT( (pathPtr != NULL) && (separatorPtr != NULL) && (pathSize > 0) );

    va_list varArgs;
    le_result_t result = LE_OK;
    size_t separatorSize = le_utf8_NumBytes(separatorPtr);
    size_t pathIndex;

    // Go to the first var arg.
    va_start(varArgs, pathSize);

    while (1)
    {
        bool hasSep = RemoveDuplicateTrailingSep(pathPtr, separatorPtr, &pathIndex);

        // Get the path segment from the next var arg.
        char* segmentPtr = va_arg(varArgs, char*);

        if (segmentPtr == NULL)
        {
            // No more segments.
            break;
        }

        // Get the start of the segment skipping over all starting separators.
        size_t segStartIndex = FindNextPathCharIndex(segmentPtr, separatorPtr);

        size_t numBytesWritten;

        // Add a separator.
        if ( (!hasSep) &&  // The path does not already have a separator.
             ( ((pathIndex == 0) && (segStartIndex != 0)) || // Path is empty and segment starts with a separator.
               ((pathIndex != 0) && (segmentPtr[segStartIndex] != '\0')) ) ) // Path is not empty and segment is not empty.
        {
            if (separatorSize >= pathSize - pathIndex)
            {
                // No more room to add the separator.
                result = LE_OVERFLOW;
                break;
            }

            // This should always succeed because we've checked the space above.
            LE_ASSERT(le_utf8_Copy(pathPtr + pathIndex,
                                   separatorPtr,
                                   pathSize - pathIndex,
                                   &numBytesWritten) == LE_OK);

            pathIndex += numBytesWritten;
        }

        if (segmentPtr[segStartIndex] == '\0')
        {
            // Nothing in segment except for separators so skip it.  Do this check here after adding
            // the separator so that if the path is empty and the separator only has a separators
            // a separator is added to the path.
            continue;
        }

        // Copy the path segment skipping over all starting separators in the segment.
        if (le_utf8_Copy(pathPtr + pathIndex,
                         segmentPtr + segStartIndex,
                         pathSize - pathIndex,
                         &numBytesWritten) == LE_OVERFLOW)
        {
            // No more room in the buffer.
            result = LE_OVERFLOW;
            break;
        }

        pathIndex += numBytesWritten;
    }

    va_end(varArgs);

    return result;
}
