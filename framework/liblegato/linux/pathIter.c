
//--------------------------------------------------------------------------------------------------
/** @file pathIter.c
 *
 * Implements the path iterator API.
 *
 * See @ref c_pathIter for details.
 *
 * Copyright (C) Sierra Wireless Inc.
 * license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum size of the various path components within the path object.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_COMPONENT_NAME_BYTES 32


//--------------------------------------------------------------------------------------------------
/**
 * Objects of this type are used to iterate and manipulate path strings.
 */
//--------------------------------------------------------------------------------------------------
typedef struct PathIterator_t
{
    char path[LIMIT_MAX_PATH_BYTES];             ///< The path to iterate and manuipulate.
    size_t pathSize;                             ///< Size of the path in bytes, not chars.

    char separator[MAX_COMPONENT_NAME_BYTES];    ///< String to represent the path separator.
    size_t separatorSize;                        ///< Size of the separator in bytes.

    char parentSpec[MAX_COMPONENT_NAME_BYTES];   ///< Name of the parent component of the path.
    size_t parentSpecSize;                       ///< Size of the parent name string in bytes.

    char currentSpec[MAX_COMPONENT_NAME_BYTES];  ///< Name of the "current" component of the path.
    size_t currentSpecSize;                      ///< Size of the current name string in bytes.

    ssize_t firstNodeIndex;                      ///< Index of the first node in the path, or -1 if
                                                 ///<   the path has no nodes.
    ssize_t lastNodeIndex;                       ///< Index of the last node in the path, or -1 if
                                                 ///<   the path has no nodes.
    ssize_t currNodeIndex;                       ///< Index of the current node in the path, or -1
                                                 ///<   if the path has no nodes.
}
PathIterator_t;

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for path iterators
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(PathIterator, LE_CONFIG_MAX_PATH_ITERATOR_POOL_SIZE,
    sizeof(PathIterator_t));


//--------------------------------------------------------------------------------------------------
/**
 * Pool of path iterators.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PathIteratorPool = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Static map of object refs to help validate external accesses to this API.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(PathIteratorMap, LE_CONFIG_MAX_PATH_ITERATOR_POOL_SIZE);

//--------------------------------------------------------------------------------------------------
/**
 * Map of object refs to help validate external accesses to this API.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t PathIteratorMap = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Given an iterator safe reference, find the original object pointer.  If this can not be done a
 * fatal error is issued.
 */
//--------------------------------------------------------------------------------------------------
static PathIterator_t* GetPathIterPtr
(
    le_pathIter_Ref_t iterRef  ///< [IN] The ref to translate to a pointer.
)
{
    PathIterator_t* iterPtr = le_ref_Lookup(PathIteratorMap, iterRef);
    LE_FATAL_IF(iterPtr == NULL, "Iterator reference, <%p> was found to be invalid.", iterRef);

    return iterPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check the string at the current postion and see if we're currently sitting on a separator.
 *
 * @return True if there is a separator at the current position, false if not.
 */
//--------------------------------------------------------------------------------------------------
static bool IsAtSeperator
(
    const char* pathStr,    ///< [IN] The string to search.
    size_t pathSize,        ///< [IN] Size of that string, in bytes.
    const char* sepStr,     ///< [IN] Path separator string to use.
    size_t sepSize,         ///< [IN] Size of the path separator in bytes.
    size_t currentPosition  ///< [IN] The position to check.
)
{
    if (   (currentPosition >= pathSize)
        || ((pathSize - currentPosition) < sepSize))
    {
        return false;
    }

    return memcmp(pathStr + currentPosition, sepStr, sepSize) == 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the next start of node index in the given string, starting the search from the given
 * position.
 *
 * @return A position within the string if a next node is found.  Otherwise pathSize is returned.
 */
//--------------------------------------------------------------------------------------------------
static ssize_t FindNextNodeIndex
(
    const char* pathStr,  ///< [IN] The string to search.
    size_t pathSize,      ///< [IN] Size of that string, in bytes.
    const char* sepStr,   ///< [IN] Path separator string to use.
    size_t sepSize,       ///< [IN] Size of the path separator in bytes.
    size_t startPoint     ///< [IN] Where to start in that search.
)
{
    ssize_t i = startPoint;

    // If already in a separator... skip past it, and any adjacent ones if there.
    if (IsAtSeperator(pathStr, pathSize, sepStr, sepSize, i))
    {
        i += sepSize;

        while (i < pathSize)
        {
            if (memcmp(pathStr + i, sepStr, sepSize) == 0)
            {
                i += sepSize;
            }
            else
            {
                return i;
            }
        }

        return pathSize;
    }

    // Otherwise, we're not in a separator, so skip past the current node and its separator(s).
    bool foundSep = false;

    while (i < pathSize)
    {
        if (IsAtSeperator(pathStr, pathSize, sepStr, sepSize, i))
        {
            foundSep = true;
            i += sepSize;
        }
        else if (foundSep == true)
        {
            return i;
        }
        else
        {
            ++i;
        }
    }

    // Looks like there was no next node.  So just return the end of the string.
    return pathSize;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find the start of the previous node index in the given string.  Starting this search from the
 * given position.
 *
 * @return The index of the beginning of the previous node.  Or -1 if no node can be found.
 */
//--------------------------------------------------------------------------------------------------
static ssize_t FindPrevNodeIndex
(
    const PathIterator_t* iterPtr,  ///< [IN] The iterator to search.
    ssize_t startPoint              ///< [IN] Where to start in that search.
)
{
    ssize_t i = startPoint - iterPtr->separatorSize;

    // If there are one or more trailing separators, get past them now.
    bool foundNode = false;

    while (   (foundNode == false)
           && (i >= 0))
    {
        if (memcmp(iterPtr->path + i, iterPtr->separator, iterPtr->separatorSize) == 0)
        {
            i -= iterPtr->separatorSize;
        }
        else
        {
            foundNode = true;
        }
    }

    // Looks like we got to the begining of the string without finding a node.
    if (foundNode == false)
    {
        return -1;
    }

    // Ok, we got past the separators.  So start searching back through the string until we either
    // get to the beginning or another separator.
    while (i >= 0)
    {
        if (memcmp(iterPtr->path + i, iterPtr->separator, iterPtr->separatorSize) == 0)
        {
            return i + iterPtr->separatorSize;
        }

        --i;
    }

    // Looks like we got to the beginning of the string.
    return 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check to see if the current node string is equal to the comparison string.  Comparison stops
 * at either the end of the source string or at the beginning of the next separator.
 *
 * So, given the path string: "things/and/stuff" or simply "things", compared with the string:
 * "things" this function will return true.
 *
 * If path string was: "thingsand/stuff" then the function would return false.
 *
 * @return True if the node matches the compare string, false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool NodeEqual
(
    const char* pathStrPtr,    ///< [IN] Compare this string with compareStr.  However, ignore
    const char* comparePtr,    ///< [IN] The name we're comparing to the node.
    size_t compareSize,        ///< [IN] How big is the compare string?
    const char* separatorPtr,  ///< [IN] The separators in the path.
    size_t separatorSize       ///< [IN] The size of the separators
)
{
    // First make sure that the compare string starts the path str we were given.  Then check to see
    // if that's the end of the string or, what immediately follows the compare string is a
    // separator.
    if (   (memcmp(pathStrPtr, comparePtr, compareSize) == 0)
        && (   (*(pathStrPtr + compareSize) == 0)
            || (memcmp(pathStrPtr + compareSize, separatorPtr, separatorSize) == 0)))
    {
        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check the current path to see if it currently ends in a parent specification.
 *
 * @return True if it does, false if not.
 */
//--------------------------------------------------------------------------------------------------
static bool EndsInParentSpec
(
    const PathIterator_t* iterPtr  ///< The iterator to check.
)
{
    // If we don't have a parent specifier, or if the path isn't even big enough to have one, then
    // clearly there isn't a parent specifier in the path.
    if (   (iterPtr->parentSpecSize == 0)
        || (iterPtr->pathSize < iterPtr->parentSpecSize))
    {
        return false;
    }

    // Check to see if the path ends in the specifier.
    return strcmp(iterPtr->parentSpec,
                  iterPtr->path + iterPtr->pathSize - iterPtr->parentSpecSize) == 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check the current path to see if it currently ends in a separator.
 *
 * @return True if it does, false if not.
 */
//--------------------------------------------------------------------------------------------------
static bool EndsWithSeparator
(
    const PathIterator_t* iterPtr  ///< The iterator path to read.
)
{
    // If the path is shorter than a separator then it can't possibly be terminated with a sperator.
    if (iterPtr->pathSize < iterPtr->separatorSize)
    {
        return false;
    }

    // Otherwise, compare the end of the string with our separator string.
    ssize_t start = iterPtr->pathSize - iterPtr->separatorSize;

    if (strcmp(iterPtr->path + start, iterPtr->separator) == 0)
    {
        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reset the iterator indicies to their proper positions.  With start index at the beginning, last
 * and current at the end.
 */
//--------------------------------------------------------------------------------------------------
static void ResetIterator
(
    PathIterator_t* iterPtr  ///< The iterator to reset.
)
{
    if (iterPtr->pathSize > 0)
    {
        if (   (iterPtr->pathSize >= iterPtr->separatorSize)
            && (memcmp(iterPtr->path, iterPtr->separator, iterPtr->separatorSize) == 0))
        {
            iterPtr->firstNodeIndex = FindNextNodeIndex(iterPtr->path,
                                                        iterPtr->pathSize,
                                                        iterPtr->separator,
                                                        iterPtr->separatorSize,
                                                        0);

            if (iterPtr->firstNodeIndex == iterPtr->pathSize)
            {
                iterPtr->firstNodeIndex = -1;
            }
        }
        else
        {
            iterPtr->firstNodeIndex = 0;
        }

        iterPtr->lastNodeIndex = FindPrevNodeIndex(iterPtr, iterPtr->pathSize);

        iterPtr->currNodeIndex = iterPtr->lastNodeIndex;
    }
    else
    {
        iterPtr->firstNodeIndex = -1;
        iterPtr->lastNodeIndex = -1;
        iterPtr->currNodeIndex = -1;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Check the current path to see if it currently ends in a separator, and if not, append one.
 *
 * @return LE_OK if the trailing separator fit within the string.
 *         LE_OVERFLOW if not.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AppendSeparator
(
    PathIterator_t* iterPtr  ///< The iterator to update.
)
{
    le_result_t result = LE_OK;
    size_t bytesCopied = 0;

    if (EndsWithSeparator(iterPtr) == false)
    {
        result = le_utf8_Copy(iterPtr->path + iterPtr->pathSize,
                              iterPtr->separator,
                              LIMIT_MAX_PATH_BYTES - iterPtr->pathSize,
                              &bytesCopied);

        iterPtr->pathSize += bytesCopied;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Append a node onto the end of an iterator's path string.
 *
 * This function also deals with parent and current node specs.  For instance, if the new node to
 * append is a parent specifier then the last node on the iterator's path is removed.  If a
 * currentNode specifier is appended onto an empty path string then it is kept at the beginning of
 * the path, otherwise it's thrown away.
 *
 * This way you can end up with paths like:
 *
 *   ./a/path/to/somewhere
 *
 * But not the less sensible:
 *
 *   ./a/./path/to/./somewhere
 *
 * @return LE_OK if the append is fits within the path string.
 *         LE_OVERFLOW if the new stirng overflows the path buffer.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AppendNode
(
    PathIterator_t* iterPtr,   ///< [IN/OUT] The iterator to update.
    const char* newSegmentPtr  ///< [IN] The new segment we're writing to the path.
)
{
    le_result_t result = LE_OK;

    // If there's no path then we can append anything, including ".." and "." type segments.
    if (iterPtr->pathSize == 0)
    {
        result = le_utf8_CopyUpToSubStr(iterPtr->path,
                                        newSegmentPtr,
                                        iterPtr->separator,
                                        LIMIT_MAX_PATH_BYTES,
                                        &iterPtr->pathSize);

        iterPtr->firstNodeIndex = 0;
        iterPtr->lastNodeIndex = 0;
        iterPtr->currNodeIndex = 0;

        return result;
    }

    // There's text in the path, and this new segment is a "." type segment, so ignore it.
    if (   (iterPtr->currentSpecSize > 0)
        && (NodeEqual(newSegmentPtr,
                      iterPtr->currentSpec,
                      iterPtr->currentSpecSize,
                      iterPtr->separator,
                      iterPtr->separatorSize)))
    {
        return LE_OK;
    }

    // Is this segment a ".." type segment?
    if (   (iterPtr->parentSpecSize > 0)
        && (NodeEqual(newSegmentPtr,
                      iterPtr->parentSpec,
                      iterPtr->parentSpecSize,
                      iterPtr->separator,
                      iterPtr->separatorSize))
        && (EndsInParentSpec(iterPtr) == false))
    {
        // Remove the trailing segment, but only if the string doesn't conist entirely of a root
        // separator.
        if (   (iterPtr->pathSize > 0)
            && (iterPtr->firstNodeIndex == -1))
        {
            return LE_UNDERFLOW;
        }

        ssize_t position = FindPrevNodeIndex(iterPtr, iterPtr->pathSize);

        LE_ASSERT(position != -1);

        if (position > iterPtr->separatorSize)
        {
            position -= iterPtr->separatorSize;
        }

        iterPtr->path[position] = 0;
        iterPtr->pathSize = position;

        ResetIterator(iterPtr);
        return LE_OK;
    }

    // Make sure there's a separator in there.
    result = AppendSeparator(iterPtr);

    if (result != LE_OK)
    {
        return result;
    }

    // Update the indicies to point at the new end node.
    iterPtr->lastNodeIndex = iterPtr->pathSize;
    iterPtr->currNodeIndex = iterPtr->lastNodeIndex;

    if (iterPtr->firstNodeIndex == -1)
    {
        iterPtr->firstNodeIndex = iterPtr->lastNodeIndex;
    }

    // Copy the new segment upto any trailing separators.
    size_t bytesCopied = 0;
    result = le_utf8_CopyUpToSubStr(iterPtr->path + iterPtr->pathSize,
                                    newSegmentPtr,
                                    iterPtr->separator,
                                    LIMIT_MAX_PATH_BYTES - iterPtr->pathSize,
                                    &bytesCopied);

    iterPtr->pathSize += bytesCopied;

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the path subsystem's internal memory pools.  This function is ment to be called
 * from Legato's internal init.
 */
//--------------------------------------------------------------------------------------------------
void pathIter_Init
(
    void
)
{
    PathIteratorPool = le_mem_InitStaticPool(PathIterator,
                                             LE_CONFIG_MAX_PATH_ITERATOR_POOL_SIZE,
                                             sizeof(PathIterator_t));
    PathIteratorMap = le_ref_InitStaticMap(PathIteratorMap, LE_CONFIG_MAX_PATH_ITERATOR_POOL_SIZE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a new path iterator object.
 */
//--------------------------------------------------------------------------------------------------
le_pathIter_Ref_t le_pathIter_Create
(
    const char* pathPtr,        ///< [IN] Optional.  Pointer to the inital path to use.
    const char* separatorPtr,   ///< [IN] Required.  Path separator to use.  The separator can not
                                ///<      be NULL or empty.
    const char* parentSpecPtr,  ///< [IN] Optional.  Used to traverse upwards in a path.  Leave as
                                ///<      NULL or empty to not use.
    const char* currentSpecPtr  ///< [IN] Optional.  Used to refer to a current node.  Much like how
                                ///<      a '.' is use in a filesystem path.
)
{
    LE_ASSERT(separatorPtr != NULL);

    // Allocate the object and it's ref.
    PathIterator_t* iterPtr = le_mem_ForceAlloc(PathIteratorPool);
    le_pathIter_Ref_t iterRef = le_ref_CreateRef(PathIteratorMap, iterPtr);
    le_result_t result;

    memset(iterPtr, 0, sizeof(PathIterator_t));

    iterPtr->firstNodeIndex = -1;
    iterPtr->lastNodeIndex = -1;
    iterPtr->currNodeIndex = -1;

    // Set our parameters.
    result = le_utf8_Copy(iterPtr->separator,
                          separatorPtr,
                          MAX_COMPONENT_NAME_BYTES,
                          &iterPtr->separatorSize);

    LE_FATAL_IF(result != LE_OK,
                "Separator '%s' is too big for internal buffers.  Max size: %d.",
                separatorPtr,
                MAX_COMPONENT_NAME_BYTES);

    if (parentSpecPtr)
    {
        result = le_utf8_Copy(iterPtr->parentSpec,
                              parentSpecPtr,
                              MAX_COMPONENT_NAME_BYTES,
                              &iterPtr->parentSpecSize);

        LE_FATAL_IF(result != LE_OK,
                    "Parent node specifier '%s' is too big for internal buffers.  Max size: %d.",
                    parentSpecPtr,
                    MAX_COMPONENT_NAME_BYTES);
    }

    if (currentSpecPtr)
    {
        result = le_utf8_Copy(iterPtr->currentSpec,
                              currentSpecPtr,
                              MAX_COMPONENT_NAME_BYTES,
                              &iterPtr->currentSpecSize);

        LE_FATAL_IF(result != LE_OK,
                    "Current node specifier '%s' is too big for internal buffers.  Max size: %d.",
                    currentSpecPtr,
                    MAX_COMPONENT_NAME_BYTES);
    }

    // Setup the path, we call append so that it can take care of normalizing the path.  If we were
    // not given a new path to work with, then stay with a default path of nothing.
    if (pathPtr)
    {
        result = le_pathIter_Append(iterRef, pathPtr);

        LE_FATAL_IF(result != LE_OK,
                    "Path '%s' is too big for internal buffers.  Max size: %d.",
                    pathPtr,
                    LIMIT_MAX_PATH_BYTES);
    }

    // All done.
    return iterRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a new path iterator object that is pre-configured for Unix styled paths.
 *
 * @return A new path iterator object that's ready for iterating on Unix paths.
 */
//--------------------------------------------------------------------------------------------------
le_pathIter_Ref_t le_pathIter_CreateForUnix
(
    const char* pathPtr  ///<  [IN] Optional.  Create an iterator for this path, or start with an
                         ///<       empty path.
)
{
    return le_pathIter_Create(pathPtr, "/", "..", ".");
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a clone of an existing path iterator object.
 *
 * @return A new path iterator object that is a duplicate of the original one.
 */
//--------------------------------------------------------------------------------------------------
le_pathIter_Ref_t le_pathIter_Clone
(
    le_pathIter_Ref_t originalRef  ///< [IN] The path object to duplicate.
)
{
    PathIterator_t* originalPtr = GetPathIterPtr(originalRef);

    // Allocate the new object and it's ref, then copy over all of the data.
    PathIterator_t* iterPtr = le_mem_ForceAlloc(PathIteratorPool);
    le_pathIter_Ref_t iterRef = le_ref_CreateRef(PathIteratorMap, iterPtr);

    memcpy(iterPtr, originalPtr, sizeof(PathIterator_t));

    return iterRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete an iterator object and free it's memory.
 */
//--------------------------------------------------------------------------------------------------
void le_pathIter_Delete
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator object to destroy.
)
{
    PathIterator_t* iterPtr = GetPathIterPtr(iterRef);

    le_ref_DeleteRef(PathIteratorMap, iterRef);
    le_mem_Release(iterPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Read the string that is being used to represent path separators in this iterator object.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pathIter_GetSeparator
(
    le_pathIter_Ref_t iterRef,  ///< [IN] The iterator object to read.
    char* bufferPtr,            ///< [OUT] The string buffer to write to.
    size_t bufferSize           ///< [IN] The size of the buffer being written to.
)
{
    LE_ASSERT(bufferPtr != NULL);
    LE_ASSERT(bufferSize > 0);

    return le_utf8_Copy(bufferPtr, GetPathIterPtr(iterRef)->separator, bufferSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Read the string that represents parent nodes in a path string.  By for Unix style paths this is
 * "..".  If an empty string is used, then it is ignored for the purposes of appending and
 * normalizing paths.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pathIter_GetParentSpecifier
(
    le_pathIter_Ref_t iterRef,  ///< [IN] The iterator object to read.
    char* bufferPtr,            ///< [OUT] The string buffer to write to.
    size_t bufferSize           ///< [IN] The size of the buffer being written to.
)
{
    LE_ASSERT(bufferPtr != NULL);
    LE_ASSERT(bufferSize > 0);

    return le_utf8_Copy(bufferPtr, GetPathIterPtr(iterRef)->parentSpec, bufferSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Read the iterators string for the current node specifier.  The for Unix style paths for this is
 * ".".  If an empty string is used, then this is ignored for the purposes of appending and
 * normalizing paths.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pathIter_GetCurrentSpecifier
(
    le_pathIter_Ref_t iterRef,  ///< [IN] The iterator object to read.
    char* bufferPtr,            ///< [OUT] The string buffer to write to.
    size_t bufferSize           ///< [IN] The size of the string buffer being written to.
)
{
    LE_ASSERT(bufferPtr != NULL);
    LE_ASSERT(bufferSize > 0);

    return le_utf8_Copy(bufferPtr, GetPathIterPtr(iterRef)->currentSpec, bufferSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a copy of the path currently contained within the iterator.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pathIter_GetPath
(
    le_pathIter_Ref_t iterRef,  ///< [IN] The iterator object to read.
    char* bufferPtr,            ///< [OUT] The string buffer to write to.
    size_t bufferSize           ///< [IN] The size of the buffer being written to.
)
{
    LE_ASSERT(bufferPtr != NULL);
    LE_ASSERT(bufferSize > 0);

    return le_utf8_Copy(bufferPtr, GetPathIterPtr(iterRef)->path, bufferSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Jump the iterator to the beginning of the path.
 *
 * @return LE_OK if the move was successful.
 *         LE_NOT_FOUND if the path is empty, or only contains a separator.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pathIter_GoToStart
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator object to update.
)
{
    PathIterator_t* iterPtr = GetPathIterPtr(iterRef);

    if (iterPtr->pathSize == 0)
    {
        return LE_NOT_FOUND;
    }

    // Simply set the iterator to first node in the path.
    iterPtr->currNodeIndex = iterPtr->firstNodeIndex;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Jump the iterator to the end of the path.
 *
 * @return LE_OK if the move was successful.
 *         LE_NOT_FOUND if the path is empty, or only contains a separator.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pathIter_GoToEnd
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator object to update.
)
{
    PathIterator_t* iterPtr = GetPathIterPtr(iterRef);

    if (iterPtr->pathSize == 0)
    {
        return LE_NOT_FOUND;
    }

    // Simply point the iterator at the last node in the path.
    iterPtr->currNodeIndex = iterPtr->lastNodeIndex;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Move to the next node in the path.
 *
 * @return LE_OK if the itrator was successful in jumping to the next node.  LE_NOT_FOUND is
 *         returned if there are no more nodes to move to in the path.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pathIter_GoToNext
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator object to update.
)
{
    PathIterator_t* iterPtr = GetPathIterPtr(iterRef);

    // If there's no path, return not found.
    if (iterPtr->pathSize == 0)
    {
        return LE_NOT_FOUND;
    }

    // Ok, try to find the beginning of the next node.  If this returns the an index that's past the
    // last known index, then we're run out of nodes.
    ssize_t newIndex = FindNextNodeIndex(iterPtr->path,
                                         iterPtr->pathSize,
                                         iterPtr->separator,
                                         iterPtr->separatorSize,
                                         iterPtr->currNodeIndex);

    if (newIndex > iterPtr->lastNodeIndex)
    {
        return LE_NOT_FOUND;
    }

    // Looks like we've found a node, so update our index.
    iterPtr->currNodeIndex = newIndex;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Move to the previous node in the path.
 *
 * @return LE_OK if the iterator was successfuly moved, LE_NOT_FOUND if there are no prior nodes to
 *         move to.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pathIter_GoToPrev
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator object to update.
)
{
    PathIterator_t* iterPtr = GetPathIterPtr(iterRef);

    // If there's no path, then return not found as there's nowhere else to go.
    if (iterPtr->pathSize == 0)
    {
        return LE_NOT_FOUND;
    }

    // Looks like we're somewhere within the list.  So, attempt to search backwards for the next
    // node.  If we hit -1 then we're past the beginning of the list.
    ssize_t newIndex = FindPrevNodeIndex(iterPtr, iterPtr->currNodeIndex);

    if (newIndex < iterPtr->firstNodeIndex)
    {
        return LE_NOT_FOUND;
    }

    iterPtr->currNodeIndex = newIndex;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the text for the node the iterator is pointing at.
 *
 * @return
 *      LE_OK if succesful.
 *      LE_OVERFLOW if the bufferPtr is too small to hold the whole string.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pathIter_GetCurrentNode
(
    le_pathIter_Ref_t iterRef,  ///< [IN] The iterator object to read.
    char* bufferPtr,             ///< [OUT] The utf-8 formatted text buffer to write to.
    size_t bufferSize            ///< [IN] The size in bytes of the text buffer.
)
{
    LE_ASSERT(bufferPtr != NULL);
    LE_ASSERT(bufferSize > 0);

    PathIterator_t* iterPtr = GetPathIterPtr(iterRef);

    // Zero out the target buffer and check to see if there is any path to copy at all in the first
    // place.  If not, we just leave the result string as empty and return not found.
    bufferPtr[0] = 0;

    if (   (iterPtr->pathSize == 0)
        || (iterPtr->currNodeIndex == -1))
    {
        return LE_NOT_FOUND;
    }

    // Now copy up until the next separator, (if any.)
    return le_utf8_CopyUpToSubStr(bufferPtr,
                                  iterPtr->path + iterPtr->currNodeIndex,
                                  iterPtr->separator,
                                  bufferSize,
                                  NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Truncate the path at the current iterator node.  If the iterator is at the beginning of the
 * path, then the whole path is cleared.  If the iterator is at the end of the path, then nothing
 * happens.
 *
 * Once done, then the iterator will be pointing at the new end of the path.
 */
//--------------------------------------------------------------------------------------------------
void le_pathIter_Truncate
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator to update.
)
{
    PathIterator_t* iterPtr = GetPathIterPtr(iterRef);

    // If there's no path, then there's nothing to do.
    if (   (iterPtr->pathSize == 0)
        || (iterPtr->currNodeIndex == -1))
    {
        return;
    }

    LE_ASSERT(iterPtr->currNodeIndex <= iterPtr->pathSize);

    // Clear out the path at the current node index.
    iterPtr->path[iterPtr->currNodeIndex] = 0;
    iterPtr->pathSize = iterPtr->currNodeIndex;

    // If there is a seperator at the end of the path, remove it now.
    if (   (EndsWithSeparator(iterPtr) == true)
        && ((iterPtr->currNodeIndex - iterPtr->separatorSize) > iterPtr->firstNodeIndex))
    {
        iterPtr->currNodeIndex -= iterPtr->separatorSize;
        iterPtr->path[iterPtr->currNodeIndex] = 0;
        iterPtr->pathSize = iterPtr->currNodeIndex;
    }

    // Reset the iterator to match the new reality.
    ResetIterator(iterPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Take the new string path and combine it with the object's existing path.
 *
 * @note This function looks for the current and parent node strings and treats them specially.
 *       So, (assuming defaults,) combining the path "/a/b" with the path "../x" will give you the
 *       combined path of: "/a/x".
 *
 * @note Appending a non-relative path onto an existing path effectivly replaces the current path,
 *       for example, appending /a/rooted/path, onto the existing /a/seperate/path will given you
 *       the path: /a/rooted/path.
 *
 * @note This will automatically reset the internal iterator to point at the end of the newly formed
 *       path.  Also, this function always appends to the end of a path, ignoring the current
 *       position of the iterator.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the output buffer is too small for the new string.
 *      LE_UNDERFLOW if combining the path the new path tries to traverse past the root.  For
 *        example: "/a/b" + "../../../x" will result in LE_UNDERFLOW.  However if the base path is
 *        relative, "a/b", then the resulting string will be "../x" and a return code of LE_OK.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pathIter_Append
(
    le_pathIter_Ref_t iterRef,  ///< [IN] The path object to write to.
    const char* pathStr         ///< [IN] The new path segment to append.
)
{
    PathIterator_t* iterPtr = GetPathIterPtr(iterRef);
    size_t newPathSize = le_utf8_NumBytes(pathStr);
    size_t newPosition = 0;

    // Check to see if the new path is absolute.  If it is, then we're replacing the original path.
    if (memcmp(pathStr, iterPtr->separator, iterPtr->separatorSize) == 0)
    {
        le_utf8_Copy(iterPtr->path, iterPtr->separator, LIMIT_MAX_PATH_BYTES, &iterPtr->pathSize);
        newPosition += iterPtr->separatorSize;

        iterPtr->firstNodeIndex = -1;
        iterPtr->lastNodeIndex = -1;
        iterPtr->currNodeIndex = -1;
    }

    // Now, iterate through the new path segments and append them onto our existing path.
    le_result_t result = LE_OK;

    while (   (result == LE_OK)
           && (newPosition < newPathSize))
    {
        if (newPosition < newPathSize)
        {
            result = AppendNode(iterPtr, pathStr + newPosition);
        }

        // Advance to the next position in the new path.
        newPosition = FindNextNodeIndex(pathStr,
                                        newPathSize,
                                        iterPtr->separator,
                                        iterPtr->separatorSize,
                                        newPosition);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Is this an absolute or relative path?
 *
 * @return True if the path is rooted, that is that it begins with a separator.  False if the path
 *         is considered relative.
 */
//--------------------------------------------------------------------------------------------------
bool le_pathIter_IsAbsolute
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator object to read.
)
{
    PathIterator_t* iterPtr = GetPathIterPtr(iterRef);

    // If the path isn't even big enough for a separator then it can't be absolute.
    if (iterPtr->pathSize < iterPtr->separatorSize)
    {
        return false;
    }

    // If the path size is non-zero and the first node index is -1.  Then we have an absolute path.
    // Or, if the first node index is one or more seperator widths away from the beginning then we
    // also have a absolute path.
    if (   (   (iterPtr->pathSize > 0)
            && (iterPtr->firstNodeIndex == -1))
        || (iterPtr->firstNodeIndex >= iterPtr->separatorSize))
    {
        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Is the path object holding an empty string?
 *
 * @return True if the path is empty, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool le_pathIter_IsEmpty
(
    le_pathIter_Ref_t iterRef  ///< The path object to read.
)
{
    return GetPathIterPtr(iterRef)->pathSize == 0;
}
