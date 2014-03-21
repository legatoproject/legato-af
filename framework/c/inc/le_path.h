/**
 * @page c_path Path API
 *
 * @ref le_path.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * @ref c_path_create <br>
 * @ref c_path_iterate <br>
 * @ref c_path_absoluteRelative <br>
 * @ref c_path_dirAndBasename <br>
 * @ref c_path_modifyPath <br>
 * @ref c_path_threads <br>
 *
 * Paths are text strings that contain nodes separated by character separators.  Paths are used in
 * many common applications like file system addressing, URLs, etc. so being able to parse them
 * is quite important.
 *
 * The Path API is intended for general purpose and supports UTF-8 null-terminated
 * strings and multi-character separators.
 *
 * This API can be used to iterate over paths, traversing the path node-by-node.
 *
 * @section c_path_create Create a Path Iterator
 *
 * Before iterating over a path, a path iterator must first be created by calling
 * @c le_path_iter_Create().  The path and separator strings passed when the path iterator is
 * created must not change during the lifetime of the iterator.  If the path and/or separator
 * strings were to change, operations on the path iterator are undefined.
 *
 * The separator can be one or more characters and must be formatted as a UTF-8 null-terminated
 * string.
 *
 * When the path iterator is no longer needed, it can be deleted by calling le_path_iter_Delete().
 *
 * @section c_path_iterate Iterating a Path
 *
 * Once an iterator is created, the nodes in it can be accessed using
 * le_path_iter_GetFirstNode() and le_path_iter_GetNextNode().
 *
 * le_path_iter_GetFirstNode() can be used to get the first node in the path.
 * le_path_iter_GetNextNode() is used to get the next node
 * relative to the most recently accessed node.  For example, after calling
 * le_path_iter_GetFirstNode() a subsequent call to le_path_iter_GetNextNode() will access the
 * second node in the path.  Calling le_path_iter_GetNextNode() again will access the third node in
 * the path.  If there are no more nodes in the path le_path_iter_GetNextNode() will return
 * LE_NOT_FOUND.
 *
 * Code sample, iterate over an entire path:
 *
 * @code
 * // Create the iterator.
 * le_path_IteratorRef_t pathIterator = le_path_iter_Create("a/sample/path", "/");
 *
 * // Print all nodes in the path.
 * char nodeStr[100];
 *
 * while (le_path_iter_GetNextNode(pathIterator, nodeStr, sizeof(nodeStr)) != LE_NOT_FOUND)
 * {
 *     printf("%s\n", nodeStr);
 * }
 * @endcode
 *
 * @note In the sample code, the first call to le_path_iter_GetNextNode() gets the first node.
 *
 * le_path_iter_GetFirstNode() and le_path_iter_GetNextNode() treats consecutive separators as a
 * single separator.
 *
 * @section c_path_absoluteRelative Absolute versus Relative Paths
 *
 * Absolute paths begin with one or more separators.  Relative paths do not begin
 * with a separator. @c le_path_iter_IsAbsolute() can be used to determine if the path is absolute or
 * relative.
 *
 * @section c_path_dirAndBasename Directory and Basename
 *
 * The function @c le_path_GetDir() is a convenient way to get the path's directory without having
 * to create an iterator.  The directory is the portion of the path up to and including the last
 * separator.  le_path_GetDir() does not modify the path in anyway (i.e., consecutive paths are left
 * as is), except to drop the node after the last separator.
 *
 * The function le_path_GetBasenamePtr() is an efficient and convenient function for accessing the
 * last node in the path without having to create an iterator.  The returned pointer points to the
 * character following the last separator in the path.  Because the basename is actually a portion
 * of the path string, not a copy, any changes to the returned basename will also affect the
 * original path string.
 *
 * @section c_path_modifyPath Modifying Paths
 *
 * Paths used in iterators cannot be modified, however, before an iterator is created for a path the
 * path can be modified.  The function le_path_Concat() can be used to easily concatenate multiple
 * path segments together.
 *
 * @section c_path_threads Thread Safety
 *
 * All the functions in this API are thread safe and reentrant unless of course the path iterators
 * or the buffers passed into the functions are shared between threads.  If the path iterators or
 * buffers are shared by multiple threads then some other mechanism must be used to ensure these
 * functions are thread safe.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

//--------------------------------------------------------------------------------------------------
/** @file le_path.h
 *
 * Legato @ref c_path include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_PATH_INCLUDE_GUARD
#define LEGATO_PATH_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Objects of this type are used to refer to a path iterator created by le_path_iter_Create().
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_path_Iterator* le_path_IteratorRef_t;


//--------------------------------------------------------------------------------------------------
/**
 * Creates a path iterator. Path and separator strings must be unchanged during the life time
 * of the interator.  Operations on the iterator are undefined if either the path or separator
 * strings change.
 *
 * The path and separator strings must be null-terminated UTF-8 strings.  The separator string must
 * be non empty.
 *
 * @return
 *      Reference to the path iterator.
 */
//--------------------------------------------------------------------------------------------------
le_path_IteratorRef_t le_path_iter_Create
(
    const char* pathPtr,        ///< [IN] Path string.
    const char* separatorPtr    ///< [IN] Separator string.
);


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
    le_path_IteratorRef_t iteratorRef,  ///< [IN] Path iterator reference.
    char* nodePtr,                      ///< [OUT] Buffer to store the node string.
    size_t nodeBuffSize                 ///< [IN] Size of the node buffer in bytes.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the next node in the path.  Gets the node after the node that was most recently accessed.
 * Consecutive separators are treated as a single separator.
 *
 * If no other nodes have been accessed, the first node is copied to nodePtr.
 *
 * If the node buffer is too small, the portion of the node that will fit is copied to the node
 * buffer and LE_OVERFLOW is returned.
 *
 * If there are no more nodes, LE_NOT_FOUND is returned nothing is copied to nodePtr.
 *
 * @return
 *      LE_OK if succesful.
 *      LE_OVERFLOW if the nodePtr buffer is too small.
 *      LE_NOT_FOUND if no more nodes are available.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_path_iter_GetNextNode
(
    le_path_IteratorRef_t iteratorRef,  ///< [IN] Path iterator reference.
    char* nodePtr,                      ///< [OUT] Buffer to store the node string.
    size_t nodeBuffSize                 ///< [IN] Size of the node buffer in bytes.
);


//--------------------------------------------------------------------------------------------------
/**
 * Determines if the path is absolute (begins with a separator) or relative (does not begin with a
 * separator).
 *
 * @return
 *      true if the path is absolute.
 *      false if the path is relative.
 */
//--------------------------------------------------------------------------------------------------
bool le_path_iter_IsAbsolute
(
    le_path_IteratorRef_t iteratorRef   ///< [IN] Path iterator reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a path iterator.
 */
//--------------------------------------------------------------------------------------------------
void le_path_iter_Delete
(
    le_path_IteratorRef_t iteratorRef   ///< [IN] Path iterator reference.
);


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
    const char* pathPtr,            ///< [IN] Path string.
    const char* separatorPtr,       ///< [IN] Separator string.
    char* dirPtr,                   ///< [OUT] Buffer to store the directory string.
    size_t dirBuffSize              ///< [IN] Size of the directory buffer in bytes.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the basename (the last node in the path).  This function gets the basename by
 * returning a pointer to the character following the last separator.
 *
 * @return
 *      Pointer to the character following the last separator.
 */
//--------------------------------------------------------------------------------------------------
char* le_path_GetBasenamePtr
(
    const char* pathPtr,            ///< [IN] Path string.
    const char* separatorPtr        ///< [IN] Separator string.
);


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
);


#endif // LEGATO_PATH_INCLUDE_GUARD
