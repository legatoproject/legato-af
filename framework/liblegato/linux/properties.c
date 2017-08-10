/** @file properties.c
 *
 * This API is used to obtain the key-value pairs from an application configuration file in the Java
 * .properties format.
 *
 * The .properties file is assumed:
 *      1) Contain a single property per line.
 *      2) Only use the 'key=value' format for properties.
 *      3) Comment lines always start with a '#' character.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "properties.h"
#include "fileDescriptor.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of characters that a property line can be.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_PROPERTIES_BYTES            LIMIT_MAX_PATH_BYTES


//--------------------------------------------------------------------------------------------------
/**
 * Reference to a properties iterator.
 */
//--------------------------------------------------------------------------------------------------
typedef struct properties_Iter
{
    FILE* streamPtr;                            ///< Stream pointer for the .properties file.
    char propertyBuf[MAX_PROPERTIES_BYTES];     ///< Property string buffer.
    char* keyPtr;                               ///< Pointer to the key string in the property buffer.
    char* valuePtr;                             ///< Pointer to the value string in the property buffer.
    char fileName[LIMIT_MAX_PATH_BYTES];        ///< File name of the .properties file.
}
PropertiesIter_t;


//--------------------------------------------------------------------------------------------------
/**
 * Pool of properties iterators.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PropertiesIterPool = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Checks if an iterator has a valid key-value pair at its currently location.
 *
 * @return
 *      true if the iterator has a valid key-value pair.
 *      false if the iterator does not have a valid key-value pair.
 */
//--------------------------------------------------------------------------------------------------
static bool HasValidKeyValue
(
    PropertiesIter_t* iterPtr       ///< [IN] Iterator to check.
)
{
    return !(strcmp(iterPtr->propertyBuf, "") == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initializes the properties subsystem's internal memory pools.  This function is to be called from
 * Legato's internal init.
 */
//--------------------------------------------------------------------------------------------------
void properties_Init
(
    void
)
{
    PropertiesIterPool = le_mem_CreatePool("PropertiesIterPool", sizeof(PropertiesIter_t));
}


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
)
{
    // Open the .properties file.
    FILE* filePtr;

    do
    {
        filePtr = fopen(fileNamePtr, "r");
    }
    while ( (filePtr == NULL) && (errno == EINTR) );

    if (filePtr == NULL)
    {
        LE_ERROR("File '%s' could not be opened.  %m.", fileNamePtr);
        return NULL;
    }

    // Create the iterator.
    PropertiesIter_t* iterPtr = le_mem_ForceAlloc(PropertiesIterPool);

    iterPtr->streamPtr = filePtr;
    iterPtr->propertyBuf[0] = '\0';
    iterPtr->keyPtr = NULL;
    iterPtr->valuePtr = NULL;

    // The stored file name is only used for debug log messages so it is fine if the file name gets
    // truncated.  No need to check the return value.
    le_utf8_Copy(iterPtr->fileName, fileNamePtr, LIMIT_MAX_PATH_BYTES, NULL);

    return iterPtr;
}


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
)
{
    // Search for the next key-value pair in the file.
    while (1)
    {
        if (fgets(iteratorRef->propertyBuf, MAX_PROPERTIES_BYTES, iteratorRef->streamPtr) != NULL)
        {
            // Skip comments.
            if (iteratorRef->propertyBuf[0] != '#')
            {
                // Remove the trailing line feed if it exists.
                char* lineFeedPtr = strchr(iteratorRef->propertyBuf, '\n');

                if (lineFeedPtr != NULL)
                {
                    *lineFeedPtr = '\0';
                }

                // Locate the equal sign in the property file.
                char* equalSignPtr = strchr(iteratorRef->propertyBuf, '=');

                if (equalSignPtr == NULL)
                {
                    LE_ERROR("'=' character not found in file %s", iteratorRef->fileName);

                    return LE_FAULT;
                }

                // Replace the equal sign with a NULL.
                *equalSignPtr =  '\0';

                // Set key pointer.
                iteratorRef->keyPtr = iteratorRef->propertyBuf;

                // Set the value pointer.
                iteratorRef->valuePtr = equalSignPtr + 1;

                return LE_OK;
            }
        }
        else
        {
            // End of file.
            return LE_NOT_FOUND;
        }
    }
}


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
)
{
    LE_FATAL_IF(!HasValidKeyValue(iteratorRef), "Iterator does not contain a valid key-value.");

    return (const char*)iteratorRef->keyPtr;
}


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
)
{
    LE_FATAL_IF(!HasValidKeyValue(iteratorRef), "Iterator does not contain a valid key-value.");

    return (const char*)iteratorRef->valuePtr;
}


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
)
{
    // Close the file.
    LE_CRIT_IF(fclose(iteratorRef->streamPtr) != 0,
               "Failed to close file %s.  %m.", iteratorRef->fileName);

    // Release the memory.
    le_mem_Release(iteratorRef);
}


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
)
{
    // Get an iterator to the app's info.properties file.
    properties_Iter_Ref_t iter = properties_CreateIter(fileNamePtr);

    if (NULL == iter)
    {
        return LE_FAULT;
    }

    // Look through the name value pairs to find the key.
    while (1)
    {
        le_result_t result = properties_NextNode(iter);

        if (result == LE_OK)
        {
            // Get the key.
            if (strcmp(properties_GetKey(iter), keyPtr) == 0)
            {
                // Get and return the value.
                le_result_t r = le_utf8_Copy(bufPtr, properties_GetValue(iter), bufSize, NULL);

                if (NULL != iter)
                {
                    properties_DeleteIter(iter);
                }

                return r;
            }
        }
        else
        {
            if (NULL != iter)
            {
                properties_DeleteIter(iter);
            }

            return result;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the value for a specified key in the specified .properties file.
 *
 * @return
 *      LE_OK if successful.
 *      LE_OVERFLOW if fileNamePtr was too long.
 *      LE_BAD_PARAMETER if fileNamePtr was formatted poorly.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t properties_SetValueForKey
(
    const char* fileNamePtr,                ///< [IN] File name of the .properties file.
    const char* keyPtr,                     ///< [IN] Key to get the value for.
    const char* valuePtr                    ///< [IN] New property value string to write.
)
{
    // Create a temporary file path for writing.
    char tempFileName[LIMIT_MAX_PATH_BYTES] = "";

    int count = snprintf(tempFileName, sizeof(tempFileName), "%s.tmp", fileNamePtr);

    if (count > sizeof(tempFileName))
    {
        return LE_NO_MEMORY;
    }
    else if (-1 == count)
    {
        return LE_BAD_PARAMETER;
    }

    // Try to open the original property file.
    properties_Iter_Ref_t iteratorRef = properties_CreateIter(fileNamePtr);

    // Now create our temp file for writing.
    FILE* outputFilePtr = NULL;
    do
    {
        outputFilePtr = fopen(tempFileName, "w");
    }
    while ( (NULL == outputFilePtr) && (EINTR == errno) );

    if (NULL == outputFilePtr)
    {
        if (NULL != iteratorRef)
        {
            properties_DeleteIter(iteratorRef);
        }

        LE_ERROR("File '%s' could not be opened.  %m.", tempFileName);
        return LE_FAULT;
    }

    // Now, iterate through the original property file and copy out the keys and values.  If our
    // key is found, then write out the new value.  Otherwise, simply write out the original value.
    size_t keySize = strlen(keyPtr);
    bool found = false;

    if (NULL != iteratorRef)
    {
        while (properties_NextNode(iteratorRef) == LE_OK)
        {
            const char* nextKey = properties_GetKey(iteratorRef);

            if (strncmp(nextKey, keyPtr, keySize) == 0)
            {
                found = true;
            }

            const char* nextValue = (found == true) ? valuePtr
                                                    : properties_GetValue(iteratorRef);

            fprintf(outputFilePtr, "%s=%s\n", nextKey, nextValue);
        }
    }

    // If the key in question was never found, then write it and it's new value to the end of the
    // file.
    if (false == found)
    {
        fprintf(outputFilePtr, "%s=%s\n", keyPtr, valuePtr);
    }

    if (NULL != iteratorRef)
    {
        properties_DeleteIter(iteratorRef);
    }

    fclose(outputFilePtr);

    // Finally, replace the original file with our new replacement.
    unlink(fileNamePtr);

    if (0 != rename(tempFileName, fileNamePtr))
    {
        LE_EMERG("Failed to rename temporary property file '%s' to '%s'.",
                 tempFileName,
                 fileNamePtr);
        return LE_FAULT;
    }

    return LE_OK;
}
