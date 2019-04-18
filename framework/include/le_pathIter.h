
//--------------------------------------------------------------------------------------------------
/**
 * @page c_pathIter Path Iterator API
 *
 * @subpage le_pathIter.h "API Reference"
 *
 * <HR>
 *
 * Paths are text strings that contain nodes separated by character separators.  Paths are used in
 * many common applications like file system addressing, URLs, etc. so being able to parse them
 * is quite important.
 *
 * The Path Iterator API is intended for general purpose use and supports UTF-8 null-terminated
 * strings and multi-character separators.
 *
 * This API can be used to iterate over paths, traversing the path node-by-node.  Or creating and
 * combining paths together while ensuring that the resultant paths are properly normalized.  For
 * instance the following path:
 *
 * @verbatim
    /a//path/to/a///some/../place
   @endverbatim
 *
 * Would be normalized to the path:
 *
 * @verbatim
    /a/path/to/a/place
   @endverbatim
 *
 * @section c_pathIter_create Create a Path Iterator
 *
 * Before iterating over a path, a path object must first be created by calling either
 * @c le_pathIter_Create(), or @c le_pathIter_CreateForUnix().  @c le_pathIter_Create() will
 * allow you to create an iterator for one of many different path styles.  While
 * @c le_pathIter_CreateForUnix() will create an iterator preconfigured for Unix style paths.
 *
 * All strings to this API must be formatted as UTF-8 null-terminated strings.
 *
 * When the path object is no longer needed, it can be deleted by calling le_pathIter_Delete().
 *
 * @section c_pathIter_iterate Iterating a Path
 *
 * Once an object is created, the nodes in it can be accessed using @c le_pathIter_GoToNext(), or
 * @c le_pathIter_GoToPrev().  To start over at the beginning of the path call
 * @c le_pathIter_GoToStart().  To position the iterator at the end of the path, use
 * @c le_pathIter_GoToEnd().  On creation, the default position of the iterator is at the end of
 * the path.
 *
 * Code sample, iterate over an entire path:
 *
 * @code
 *
 * // Create an iterator object, and move it to the front of the path.
 * le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix(myPathPtr);
 *
 * if (le_pathIter_IsEmpty(iteratorRef))
 * {
 *     return;
 * }
 *
 * le_pathIter_GoToStart(iteratorRef);
 *
 * // Now go through all of the path nodes and print out each one.
 * do
 * {
 *     char buffer[BUFFER_SIZE] = { 0 };
 *
 *     if (le_pathIter_GetCurrentNode(iteratorRef, buffer, BUFFER_SIZE) == LE_OK)
 *     {
 *         printf("%s\n", buffer);
 *     }
 * }
 * while (le_pathIter_GoToNext(iteratorRef) != LE_NOT_FOUND);
 *
 * // All done with the iterator, so free it now.
 * le_pathIter_Delete(iteratorRef);
 *
 * @endcode
 *
 * @note @c le_pathIter_GetNextNode() and @c le_pathIter_GetPreviousNode() treat consecutive
 *       separators as a single separator.
 *
 * @section c_pathIter_absoluteRelative Absolute versus Relative Paths
 *
 * Absolute paths begin with one or more separators.  Relative paths do not begin with a separator.
 * @c le_pathIter_IsAbsolute() can be used to determine if the path is absolute or relative.
 *
 * @section c_pathIter_modifyPath Modifying Paths
 *
 * In addition to pure iteration, the path iterator can allow you to modify a path.  For instance,
 * you can iterate to a node in the path and use @c le_pathIter_Truncate() to truncate everything at
 * and after that point.  While you can use @c le_pathIter_Append() to add new path nodes at the end
 * of the path.
 *
 * Take the following code:
 *
 * @code
 *
 * le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/a/path/to/a/place");
 * char fullPath[PATH_SIZE] = { 0 };
 *
 * le_pathIter_GoToStart(iteratorRef);
 *
 * le_pathIter_GoToNext(iteratorRef);
 * le_pathIter_GoToNext(iteratorRef);
 * le_pathIter_GoToNext(iteratorRef);
 *
 * le_pathIter_Truncate(iteratorRef);
 *
 * le_pathIter_Append(iteratorRef, "nowhere");
 *
 * le_pathIter_GetPath(iteratorRef, fullPath, PATH_SIZE);
 *
 * LE_ASSERT(strcmp(fullPath, "/a/path/to/nowhere") == 0);
 *
 * @endcode
 *
 * Note that @c le_pathIter_Append() will also normalize paths as it appends.  So, the following
 * example has the same effect as the previous one.
 *
 * @code
 *
 * le_pathIter_Ref_t iteratorRef = le_pathIter_CreateForUnix("/a/path/to/a/place");
 * char fullPath[PATH_SIZE] = { 0 };
 *
 * le_pathIter_Append(iteratorRef, "../../nowhere");
 * le_pathIter_GetPath(iteratorRef, fullPath, PATH_SIZE);
 *
 * le_pathIter_GetPath(iteratorRef, fullPath, PATH_SIZE);
 *
 * LE_ASSERT(strcmp(fullPath, "/a/path/to/nowhere") == 0);
 *
 * @endcode
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 * license.
 */
//--------------------------------------------------------------------------------------------------
/** @file le_pathIter.h
 *
 * Legato @ref c_pathIter include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 * license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_PATH_ITER_INCLUDE_GUARD
#define LEGATO_PATH_ITER_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Objects of this type are used to iterate and manipulate path strings.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_pathIter_t* le_pathIter_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Create a new path iterator object.  On creation, the default position of the iterator is at the
 * end of the path.
 *
 * @return A new path object setup with the given parameters.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_pathIter_Ref_t le_pathIter_Create
(
    const char* pathPtr,        ///< [IN] Optional.  Pointer to the inital path to use.
    const char* separatorPtr,   ///< [IN] Required.  Path separator to use.  The separator can not
                                ///<      be NULL or empty.
    const char* parentSpecPtr,  ///< [IN] Optional.  Used to traverse upwards in a path.  Leave as
                                ///<      NULL or empty to not use.  This acts like how ".." is used
                                ///<      in a filesystem path.
    const char* currentSpecPtr  ///< [IN] Optional.  Used to refer to a current node.  Much like how
                                ///<      a '.' is used in a filesystem path.
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a new path iterator object that is pre-configured for Unix styled paths.    On creation,
 * the default position of the iterator is at the end of the path.
 *
 * The parameters are configured as follows:
 *
 *  - separator:   "/"
 *  - parentSpec:  ".."
 *  - currentSpec: "."
 *
 * @return A new path iterator object that's ready for iterating on Unix style paths.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_pathIter_Ref_t le_pathIter_CreateForUnix
(
    const char* pathPtr  ///<  [IN] Optional.  Create an iterator for this path, or start with an
                         ///<       empty path.
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a clone of an existing path iterator object.
 *
 * @return A new path iterator object that is a duplicate of the original one.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_pathIter_Ref_t le_pathIter_Clone
(
    le_pathIter_Ref_t originalRef  ///< [IN] The path object to duplicate.
);


//--------------------------------------------------------------------------------------------------
/**
 * Delete an iterator object and free it's memory.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API void le_pathIter_Delete
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator object to destroy.
);


//--------------------------------------------------------------------------------------------------
/**
 * Read the string that is being used to represent path separators in this iterator object.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_result_t le_pathIter_GetSeparator
(
    le_pathIter_Ref_t iterRef,  ///< [IN] The iterator object to read.
    char* bufferPtr,            ///< [OUT] The string buffer to write to.
    size_t bufferSize           ///< [IN] The size of the buffer being written to.
);


//--------------------------------------------------------------------------------------------------
/**
 * Read the string that represents parent nodes in a path string.  By for Unix style paths this is
 * "..".  If an empty string is used, then it is ignored for the purposes of appending and
 * normalizing paths.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_result_t le_pathIter_GetParentSpecifier
(
    le_pathIter_Ref_t iterRef,  ///< [IN] The iterator object to read.
    char* bufferPtr,            ///< [OUT] The string buffer to write to.
    size_t bufferSize           ///< [IN] The size of the buffer being written to.
);


//--------------------------------------------------------------------------------------------------
/**
 * Read the iterators string for the current node specifier.  For Unix style paths for this is ".".
 * If an empty string is used, then this is ignored for the purposes of appending and normalizing
 * paths.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_result_t le_pathIter_GetCurrentSpecifier
(
    le_pathIter_Ref_t iterRef,  ///< [IN] The iterator object to read.
    char* bufferPtr,            ///< [OUT] The string buffer to write to.
    size_t bufferSize           ///< [IN] The size of the string buffer being written to.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get a copy of the path currently contained within the iterator.
 *
 * @return LE_OK if the copy is successful.
 *         LE_OVERFLOW if the buffer isn't big enough for the path string.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_result_t le_pathIter_GetPath
(
    le_pathIter_Ref_t iterRef,  ///< [IN] The iterator object to read.
    char* bufferPtr,            ///< [OUT] The string buffer to write to.
    size_t bufferSize           ///< [IN] The size of the buffer being written to.
);


//--------------------------------------------------------------------------------------------------
/**
 * Jump the iterator to the beginning of the path.
 *
 * @return LE_OK if the move was successful.
 *         LE_NOT_FOUND if the path is empty, or only contains a separator.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_result_t le_pathIter_GoToStart
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator object to update.
);


//--------------------------------------------------------------------------------------------------
/**
 * Jump the iterator to the end of the path.
 *
 * @return LE_OK if the move was successful.
 *         LE_NOT_FOUND if the path is empty, or only contains a separator.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_result_t le_pathIter_GoToEnd
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator object to update.
);


//--------------------------------------------------------------------------------------------------
/**
 * Move to the next node in the path.
 *
 * @return LE_OK if the itrator was successful in jumping to the next node.  LE_NOT_FOUND is
 *         returned if there are no more nodes to move to in the path.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_result_t le_pathIter_GoToNext
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator object to update.
);


//--------------------------------------------------------------------------------------------------
/**
 * Move to the previous node in the path.
 *
 * @return LE_OK if the iterator was successfuly moved, LE_NOT_FOUND if there are no prior nodes to
 *         move to.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_result_t le_pathIter_GoToPrev
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator object to update.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the text for the node the itrator is pointing at.
 *
 * @return
 *      LE_OK if succesful.
 *      LE_OVERFLOW if the bufferPtr is too small to hold the whole string.
 *      LE_NOT_FOUND if the iterator is at the end of the path.  Or if the path is empty, or simply
 *      consists of a separator.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API le_result_t le_pathIter_GetCurrentNode
(
    le_pathIter_Ref_t iterRef,  ///< [IN] The iterator object to read.
    char* bufferPtr,            ///< [OUT] The utf-8 formatted text buffer to write to.
    size_t bufferSize           ///< [IN] The size in bytes of the text buffer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Truncate the path at the current iterator node.  If the iterator is at the beginning of the
 * path, then the whole path is cleared.  If the iterator is at the end of the path, then nothing
 * happens.
 *
 * Once done, then the iterator will be pointing at the new end of the path.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API void le_pathIter_Truncate
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator to update.
);


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
LE_FULL_API le_result_t le_pathIter_Append
(
    le_pathIter_Ref_t iterRef,  ///< [IN] The path object to write to.
    const char* pathStr         ///< [IN] The new path segment to append.
);


//--------------------------------------------------------------------------------------------------
/**
 * Is this an absolute or relative path?
 *
 * @return True if the path is absolute, that is that it begins with a separator.  False if the path
 *         is considered relative.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API bool le_pathIter_IsAbsolute
(
    le_pathIter_Ref_t iterRef  ///< [IN] The iterator object to read.
);


//--------------------------------------------------------------------------------------------------
/**
 * Is the path object holding an empty string?
 *
 * @return True if the path is empty, false if not.
 */
//--------------------------------------------------------------------------------------------------
LE_FULL_API bool le_pathIter_IsEmpty
(
    le_pathIter_Ref_t iterRef  ///< The path object to read.
);


#endif  // LEGATO_PATH_ITER_INCLUDE_GUARD
