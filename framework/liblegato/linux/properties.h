/** @file properties.h
 *
 * This API is used to obtain the key-value pairs from an application configuration file in the Java
 * .properties format.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SRC_PROPERTIES_INCLUDE_GUARD
#define LEGATO_SRC_PROPERTIES_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Reference to a properties iterator.
 */
//--------------------------------------------------------------------------------------------------
typedef struct properties_Iter* properties_Iter_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the properties subsystem's internal memory pools.  This function is to be called from
 * Legato's internal init.
 */
//--------------------------------------------------------------------------------------------------
void properties_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Opens the specified .properties file and returns an iterator that can be used to step through the
 * list of name-value pairs in the file.
 *
 * The iterator is not ready for data access until properties_NextNode() has been called
 * at least once.
 *
 * @note
 *      Reading from a file that is being written to at the same time can result in unexpected
 *      behaviour.  The caller must ensure that the file is not being written to while an iterator
 *      for the file exists.
 *
 * @return
 *      A reference to a properties iterator if successful.
 *      NULL if the the file could not be opened.
 */
//--------------------------------------------------------------------------------------------------
properties_Iter_Ref_t properties_CreateIter
(
    const char* fileNamePtr                 ///< [IN] File name of the .properties file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Moves the iterator to the next key-value pair in the file.  This function must be called at least
 * once before any key-value pairs can be read.  After the first time this function is called
 * successfully on an iterator the first key-value pair will be available.
 *
 * @return
 *      LE_OK if successful.
 *      LE_NOT_FOUND if there are no more key-value pairs in the file.
 *      LE_FAULT if there is a format error in the file.
 */
//--------------------------------------------------------------------------------------------------
le_result_t properties_NextNode
(
    properties_Iter_Ref_t iteratorRef       ///< [IN] Reference to the iterator.
);


//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the key where the iterator is currently pointing.
 *
 * @note
 *      This function should only be used if a previous call to properties_NextNode() returned
 *      successfully (returned LE_OK).  Otherwise the behaviour of this function is undefined.
 *
 * @return
 *      The key string.
 */
//--------------------------------------------------------------------------------------------------
const char* properties_GetKey
(
    properties_Iter_Ref_t iteratorRef       ///< [IN] Reference to the iterator.
);


//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the value where the iterator is currently pointing.
 *
 * @note
 *      This function should only be used if a previous call to properties_NextNode() returned
 *      successfully (returned LE_OK).  Otherwise the behaviour of this function is undefined.
 *
 * @return
 *      The value string.
 */
//--------------------------------------------------------------------------------------------------
const char* properties_GetValue
(
    properties_Iter_Ref_t iteratorRef       ///< [IN] Reference to the iterator.
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the iterator and closes the associated .properties file.
 *
 * @note
 *      If the iterator is invalid this function will kill the calling process.
 */
//--------------------------------------------------------------------------------------------------
void properties_DeleteIter
(
    properties_Iter_Ref_t iteratorRef       ///< [IN] Reference to the iterator.
);


//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the value for a specified key in the specified .properties file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if the buffer is too small to hold the entire value string.
 *      LE_NOT_FOUND if the key does not exist.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t properties_GetValueForKey
(
    const char* fileNamePtr,                ///< [IN] File name of the .properties file.
    const char* keyPtr,                     ///< [IN] Key to get the value for.
    char* bufPtr,                           ///< [OUT] Buffer to hold the value string.
    size_t bufSize                          ///< [IN] Size of the buffer.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the value for a specified key in the specified .properties file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t properties_SetValueForKey
(
    const char* fileNamePtr,                ///< [IN] File name of the .properties file.
    const char* keyPtr,                     ///< [IN] Key to get the value for.
    const char* valuePtr                    ///< [IN] New property value string to write.
);


#endif // LEGATO_SRC_PROPERTIES_INCLUDE_GUARD
