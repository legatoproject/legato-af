/**
 * @file pa_ftStorage.c
 *
 * implementation of PA functions for fileStreamServer API with local file storage.
 *
 * This PA stores the downloaded file content to local storage file system (flash).
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_fileStream.h"
#include "fileStreamServer.h"
#include "fileStreamClient.h"
#include "jansson.h"
#include <sys/statvfs.h>

//--------------------------------------------------------------------------------------------------
/**
 * Static file descriptor for file transfer
 */
//--------------------------------------------------------------------------------------------------
static int StaticFd = -1;


//--------------------------------------------------------------------------------------------------
/**
 * Static file stream thread reference
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t FileStreamRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * File streaming thread stack size in words
 */
//--------------------------------------------------------------------------------------------------
#define THREAD_STACK_SIZE (5*1024)


//--------------------------------------------------------------------------------------------------
/**
 * Allocate stacks for both threads
 */
//--------------------------------------------------------------------------------------------------
LE_THREAD_DEFINE_STATIC_STACK(FileStreamThreadStrStack, THREAD_STACK_SIZE);


//--------------------------------------------------------------------------------------------------
/**
 * Stream Context
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int             readFd;                                             ///< File download fd
    le_fs_FileRef_t fileRef;                                            ///< File storage reference
    char            topic[LE_FILESTREAMCLIENT_FILE_TOPIC_MAX_BYTES];    ///< File class
    size_t          bytesReceived;                                      ///< Received bytes
}
StreamContext_t;


#define MAX_STREAM_OBJECT           20
#define READ_CHUNK_BYTES            4096
#define DEFAULT_TIMEOUT_MS          900000
#define MAX_EVENTS                  10
#define ROOT_PATH_STORAGE           "/data/le_fs"
#define FILESTREAM_LEFS_DIR         "/fileStream"
#define FILESTREAM_STORAGE_LEFS_DIR "/files"

//--------------------------------------------------------------------------------------------------
/**
 * Define values for the FILESTREAM_FILE_LIST JSON file
 */
//--------------------------------------------------------------------------------------------------
#define JSON_FILE_FIELD_FILES                       "files"
#define JSON_FILE_FIELD_NAME                        "name"
#define JSON_FILE_FIELD_SIZE                        "size"
#define JSON_FILE_FIELD_STATE                       "state"
#define JSON_FILE_FIELD_RESULT                      "result"
#define JSON_FILE_FIELD_CLASS                       "class"
#define JSON_FILE_FIELD_HASH                        "hash"
#define JSON_FILE_FIELD_DIRECTION                   "direction"
#define JSON_FILE_FIELD_ORIGIN                      "origin"
#define JSON_FILE_FIELD_INSTANCE                    "instance"

//--------------------------------------------------------------------------------------------------
/**
 * Define values for the FILESTREAM_FILE_DOWNLOAD JSON file
 */
//--------------------------------------------------------------------------------------------------
#define FILE_DOWNLOAD_NO_SIZE                       "no size"
#define FILE_DOWNLOAD_PENDING                       "waiting"
#define FILE_DOWNLOAD_ON_GOING                      "transferring"
#define FILE_DOWNLOAD_SUCCESS                       "success"
#define FILE_DOWNLOAD_FAILURE                       "failure"

//--------------------------------------------------------------------------------------------------
/**
 * Define value for the file list (JSON format)
 */
//--------------------------------------------------------------------------------------------------
#define FILESTREAM_FILE_LIST                        FILESTREAM_LEFS_DIR "/" "file_list.json"

//--------------------------------------------------------------------------------------------------
/**
 * Define value for the file which is downloaded (JSON format)
 */
//--------------------------------------------------------------------------------------------------
#define FILESTREAM_FILE_DOWNLOAD                    FILESTREAM_LEFS_DIR "/" "file_download.json"

//--------------------------------------------------------------------------------------------------
/**
 * Define value for an empty file list (JSON format)
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_FILE_LIST                           "{\"" JSON_FILE_FIELD_FILES "\":[]}"

//--------------------------------------------------------------------------------------------------
/**
 * Config path to the file stream content content.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_FILESTREAM                              "fileStreamService:/"


//--------------------------------------------------------------------------------------------------
/**
 * Static process stream thread reference.
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t ProcessStreamThreadRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Context of the current download stream
 */
//--------------------------------------------------------------------------------------------------
StreamContext_t StreamContext;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for file stream handlers.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t StreamObjPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Stream object table
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t StreamObjTable;

//--------------------------------------------------------------------------------------------------
/**
 * File instances availablibity
 */
//--------------------------------------------------------------------------------------------------
static bool IsFileInstanceUsed[LE_FILESTREAMSERVER_FILE_MAX_NUMBER];


//==================================================================================================
//                                       Local Functions
//==================================================================================================

//--------------------------------------------------------------------------------------------------
/**
 * Read from file using Legato le_fs API
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_BAD_PARAMETER  Incorrect parameter provided
 *  - LE_OVERFLOW       The file path is too long
 *  - LE_FAULT          The function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ReadFs
(
    const char* pathPtr,    ///< [IN] File path
    uint8_t*    bufPtr,     ///< [OUT] Data buffer
    size_t*     sizePtr     ///< [OUT] Buffer size
)
{
    le_fs_FileRef_t fileRef;
    le_result_t result;

    LE_FATAL_IF(!pathPtr, "Invalid parameter");
    LE_FATAL_IF(!bufPtr, "Invalid parameter");

    result = le_fs_Open(pathPtr, LE_FS_RDONLY, &fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("failed to open %s: %s", pathPtr, LE_RESULT_TXT(result));
        return result;
    }

    result = le_fs_Read(fileRef, bufPtr, sizePtr);
    if (LE_OK != result)
    {
        LE_ERROR("failed to read %s: %s", pathPtr, LE_RESULT_TXT(result));
        if (LE_OK != le_fs_Close(fileRef))
        {
            LE_ERROR("failed to close %s", pathPtr);
        }
        return result;
    }

    result = le_fs_Close(fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("failed to close %s: %s", pathPtr, LE_RESULT_TXT(result));
        return result;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write to file using Legato le_fs API
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_BAD_PARAMETER  Incorrect parameter provided
 *  - LE_OVERFLOW       The file path is too long
 *  - LE_FAULT          The function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteFs
(
    const char  *pathPtr,   ///< [IN] File path
    uint8_t     *bufPtr,    ///< [IN] Data buffer
    size_t      size        ///< [IN] Buffer size
)
{
    le_fs_FileRef_t fileRef;
    le_result_t result;

    LE_FATAL_IF(!pathPtr, "Invalid parameter");
    LE_FATAL_IF(!bufPtr, "Invalid parameter");

    result = le_fs_Open(pathPtr, LE_FS_WRONLY | LE_FS_CREAT | LE_FS_TRUNC, &fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("failed to open %s: %s", pathPtr, LE_RESULT_TXT(result));
        return result;
    }

    result = le_fs_Write(fileRef, bufPtr, size);
    if (LE_OK != result)
    {
        LE_ERROR("failed to write %s: %s", pathPtr, LE_RESULT_TXT(result));
        if (LE_OK != le_fs_Close(fileRef))
        {
            LE_ERROR("failed to close %s", pathPtr);
        }
        return result;
    }

    result = le_fs_Close(fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("failed to close %s: %s", pathPtr, LE_RESULT_TXT(result));
        return result;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create empty json file
 */
//--------------------------------------------------------------------------------------------------
static void CreateDefaultJsonFile
(
    const char* jsonFileNamePtr     ///< [IN] File name
)
{
    if (!jsonFileNamePtr)
    {
        return;
    }

    // Check the presence of the file list
    if (false  == le_fs_Exists(jsonFileNamePtr))
    {
        // Create default file
        char buffer[20] = {0};
        le_fs_FileRef_t fileRef;
        le_result_t result;

        snprintf(buffer, 20, DEFAULT_FILE_LIST);

        result = le_fs_Open(jsonFileNamePtr, LE_FS_WRONLY | LE_FS_CREAT | LE_FS_TRUNC, &fileRef);
        if (LE_OK != result)
        {
            LE_ERROR("failed to open %s: %s", jsonFileNamePtr, LE_RESULT_TXT(result));
        }
        else
        {
            result = le_fs_Write(fileRef, (uint8_t*)buffer, strlen(buffer));
            if (LE_OK != result)
            {
                LE_ERROR("failed to write %s: %s", jsonFileNamePtr, LE_RESULT_TXT(result));
            }

            result = le_fs_Close(fileRef);
            if (LE_OK != result)
            {
                LE_ERROR("failed to close %s: %s", jsonFileNamePtr, LE_RESULT_TXT(result));
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete file using Legato le_fs API
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_BAD_PARAMETER  A parameter is invalid
 *  - LE_OVERFLOW       The file path is too long
 *  - LE_NOT_FOUND      The file does not exist or a directory in the path does not exist
 *  - LE_NOT_PERMITTED  The access right fails to delete the file or access is not granted to a
 *                      a directory in the path
 *  - LE_UNSUPPORTED    The function is unusable
 *  - LE_FAULT          The function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t DeleteFs
(
    const char* pathPtr    ///< [IN] File path
)
{
    le_result_t result;

    LE_FATAL_IF(!pathPtr, "Invalid parameter");

    result = le_fs_Delete(pathPtr);
    if (LE_OK != result)
    {
        LE_ERROR("failed to delete %s: %s", pathPtr, LE_RESULT_TXT(result));
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read file details from a JSON file
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_BAD_PARAMETER  Incorrect parameter provided
 *  - LE_FAULT          The function failed
 *  - LE_NOT_FOUND      The file is not present
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FileInfoFromJsonFileRead
(
    const char* jsonFileNamePtr,            ///< [IN] File name of the file info need to be read
    uint16_t    instanceId,                 ///< [IN] File instance
    char*       fileNamePtr,                ///< [OUT] File name buffer
    size_t      fileNameNumElements,        ///< [IN]  File name buffer size (\0 included)
    char*       fileTopicPtr,               ///< [OUT] File topic buffer
    size_t      fileTopicNumElements,       ///< [IN]  File topic buffer size (\0 included)
    char*       fileHashPtr,                ///< [OUT] File hash buffer
    size_t      fileHashNumElements,        ///< [IN]  File hash buffer size (\0 included)
    uint64_t*   fileSizePtr,                ///< [OUT]  File size
    uint8_t*    fileOriginPtr               ///< [OUT] File origin
)
{
    char* bufferPtr = NULL;
    int instance = 0;
    size_t bufferSize = 0;
    int i = 0;
    bool doesFileExist = false;
    size_t length;

    json_error_t error;
    json_t* filePtr;
    json_t* root;

    if ((!jsonFileNamePtr))
    {
        return LE_BAD_PARAMETER;
    }

    if ((LE_FILESTREAMSERVER_FILE_MAX_NUMBER < instanceId)
     && (FILE_INSTANCE_ID_DOWNLOADING != instanceId))
    {
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != le_fs_GetSize(jsonFileNamePtr, &bufferSize))
    {
        LE_DEBUG("Error to get file %s size", jsonFileNamePtr);
        return LE_FAULT;
    }

    bufferPtr = calloc(bufferSize + 1, sizeof(char));

    if (LE_OK != ReadFs(jsonFileNamePtr, (uint8_t*)bufferPtr, &bufferSize))
    {
        LE_DEBUG("Error to read file %s", jsonFileNamePtr);
        return LE_FAULT;
    }

    /* Parse the file (JSON) */
    root = json_loads(bufferPtr, 0, &error);
    free(bufferPtr);

    if (!root)
    {
        LE_ERROR("Error: on loading json: %s\n", error.text);
        return LE_FAULT;
    }

    filePtr = json_object_get(root, JSON_FILE_FIELD_FILES);
    if (!filePtr)
    {
        LE_ERROR("Error to find files array");
        json_decref(root);
        return LE_FAULT;
    }

    if(!json_is_array(filePtr))
    {
        LE_ERROR("Files is not an array");
        json_decref(root);
        return LE_FAULT;
    }

    length = json_array_size(filePtr);
    LE_DEBUG("length %zd", length);

    // Check if the filename is not already present
    for (i = 0; i < length; i++)
    {
        json_t* fileObjectPtr = json_array_get(filePtr, i);
        json_t* jsonFieldPtr;

        jsonFieldPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_INSTANCE);
        if(jsonFieldPtr)
        {
            instance = json_integer_value(jsonFieldPtr);
        }

        if (instanceId == instance)
        {
            const char* tempPtr;

            doesFileExist = true;

            jsonFieldPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_NAME);
            if (jsonFieldPtr && fileNamePtr)
            {
                tempPtr = (char*)json_string_value(jsonFieldPtr);
                LE_DEBUG("file name: %s", tempPtr);
                if(fileNameNumElements >= strlen(tempPtr))
                {
                    snprintf(fileNamePtr, fileNameNumElements, "%s", tempPtr);
                }
            }

            jsonFieldPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_CLASS);
            if (jsonFieldPtr && fileTopicPtr)
            {
                tempPtr = (char*)json_string_value(jsonFieldPtr);
                LE_DEBUG("file class: %s", tempPtr);
                if(fileTopicNumElements >= strlen(tempPtr))
                {
                    snprintf(fileTopicPtr, fileTopicNumElements, "%s", tempPtr);
                }
            }

            jsonFieldPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_HASH);
            if (jsonFieldPtr && fileHashPtr)
            {
                tempPtr = (char*)json_string_value(jsonFieldPtr);
                LE_DEBUG("file hash: %s", tempPtr);
                if(fileHashNumElements >= strlen(tempPtr))
                {
                    snprintf(fileHashPtr, fileHashNumElements, "%s", tempPtr);
                }
            }

            jsonFieldPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_SIZE);
            if (jsonFieldPtr && fileSizePtr)
            {
                *fileSizePtr = json_integer_value(jsonFieldPtr);
                LE_DEBUG("file size: %"PRIu64, *fileSizePtr);
            }

            jsonFieldPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_ORIGIN);
            if (jsonFieldPtr && fileOriginPtr)
            {
                *fileOriginPtr = json_integer_value(jsonFieldPtr);
                LE_DEBUG("file origin: %d", *fileOriginPtr);
            }
            break;
        }
    }

    json_decref(root);
    if (false == doesFileExist)
    {
        return LE_NOT_FOUND;
    }

    return LE_OK;

}

//--------------------------------------------------------------------------------------------------
/**
 * Return all file instances of one JSON file
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_BAD_PARAMETER  Incorrect parameter provided
 *  - LE_FAULT          The function failed
 *  - LE_NOT_FOUND      The file is not present
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ListFileInstanceFromJsonFile
(
    const char* jsonFileNamePtr,    ///< [IN] File name in which the file info need to be deleted
    uint16_t*   instanceListPtr,    ///< [OUT] File instance number list buffer
    uint32_t*   instanceNbPtr       ///< [OUT] File instance number
)
{
    char* bufferPtr = NULL;
    size_t bufferSize = 0;
    uint32_t step = 0;
    int i = 0;
    size_t length;
    json_error_t error;
    json_t* filePtr;
    json_t* root;

    if ((!jsonFileNamePtr)
     || (!instanceListPtr)
     || (!instanceNbPtr))
    {
        return LE_BAD_PARAMETER;
    }

    *instanceNbPtr = 0;

    LE_DEBUG("ListFileInstanceFromJsonFile file %s", jsonFileNamePtr);

    if (LE_OK != le_fs_GetSize(jsonFileNamePtr, &bufferSize))
    {
        LE_DEBUG("Error to get file %s size", jsonFileNamePtr);
        return LE_FAULT;
    }

    bufferPtr = calloc(bufferSize + 1, sizeof(char));

    if (LE_OK != ReadFs(jsonFileNamePtr, (uint8_t*)bufferPtr, &bufferSize))
    {
        LE_DEBUG("Error to read file %s", jsonFileNamePtr);
        free(bufferPtr);
        return LE_FAULT;
    }

    /* Parse the file (JSON) */
    root = json_loads(bufferPtr, 0, &error);
    free(bufferPtr);

    filePtr = json_object_get(root, JSON_FILE_FIELD_FILES);
    if (!filePtr)
    {
        LE_ERROR("Error to find files array");
        return LE_FAULT;
    }

    if(!json_is_array(filePtr))
    {
        LE_ERROR("Files is not an array");
        json_decref(root);
        return LE_FAULT;
    }

    length = json_array_size(filePtr);
    LE_DEBUG("length %d", length);

    // Check if the filename is not already present
    for (i = 0; i < length; i++)
    {
        json_t* fileObjectPtr = json_array_get(filePtr, i);

        if(!fileObjectPtr)
        {
            break;
        }

        json_t* jsonInstance = json_object_get(fileObjectPtr, JSON_FILE_FIELD_INSTANCE);
        instanceListPtr[step] = json_integer_value(jsonInstance);
        step++;
        (*instanceNbPtr)++;

    }
    json_decref(root);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add one file in the JSON file list
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_BAD_PARAMETER  Incorrect parameter provided
 *  - LE_FAULT          The function failed
 *  - LE_DUPLICATE      The file already exists
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FileInfoFromJsonFileAdd
(
    const char* jsonFileNamePtr,    ///< [IN] File name in which the file info need to be added
    char*       fileNamePtr,        ///< [IN] File name
    char*       statePtr,           ///< [IN] File state
    char*       classPtr,           ///< [IN] File class
    char*       hashPtr,            ///< [IN] File hash
    uint64_t    fileSize,           ///< [IN] File size
    uint8_t     direction,          ///< [IN] File direction (0: download)
    uint8_t     origin,             ///< [IN] File origin (0: server)
    uint16_t    instanceId          ///< [IN] Instance Id
)
{
    char* bufferPtr = NULL;
    size_t bufferSize = 0;
    bool isFileAlreadyExists = false;
    bool isFileAdded = false;
    int i = 0;
    size_t length;
    json_error_t error;
    json_t* filePtr;
    json_t* root;

    if ((!jsonFileNamePtr) || (!fileNamePtr) || (!statePtr) || (!classPtr) || (!hashPtr))
    {
        return LE_BAD_PARAMETER;
    }

    if ((strlen(statePtr) == (strlen(FILE_DOWNLOAD_NO_SIZE)))
     && (!strncmp(statePtr, FILE_DOWNLOAD_NO_SIZE, strlen(statePtr))))
    {
        LE_INFO("New file transfer. File %s, class %s", fileNamePtr, classPtr);
    }

    LE_DEBUG("Add one file in %s", jsonFileNamePtr);
    LE_DEBUG("File name: %s", fileNamePtr);
    LE_DEBUG("File state: %s", statePtr);
    LE_DEBUG("File class: %s", classPtr);
    LE_DEBUG("File hash: %s", hashPtr);
    LE_DEBUG("File direction: %d", direction);
    LE_DEBUG("File origin: %d", origin);

    if (LE_OK != le_fs_GetSize(jsonFileNamePtr, &bufferSize))
    {
        LE_DEBUG("Error to get file %s size", jsonFileNamePtr);
        return LE_FAULT;
    }

    bufferPtr = calloc(bufferSize + 1, sizeof(char));

    if (LE_OK != ReadFs(jsonFileNamePtr, (uint8_t*)bufferPtr, &bufferSize))
    {
        free(bufferPtr);
        return LE_FAULT;
    }

    /* Parse the file (JSON) */
    root = json_loads(bufferPtr, 0, &error);
    free(bufferPtr);

    filePtr = json_object_get(root, JSON_FILE_FIELD_FILES);
    if (!filePtr)
    {
        LE_ERROR("Error to find files array");
        return LE_FAULT;
    }

    if(!json_is_array(filePtr))
    {
        LE_ERROR("Files is not an array");
        json_decref(root);
        return LE_FAULT;
    }

    length = json_array_size(filePtr);
    // Check if the filename is not already present
    for (i = 0; i < length; i++)
    {
        json_t* fileObjectPtr = json_array_get(filePtr, i);

        json_t* jsonName = json_object_get(fileObjectPtr, JSON_FILE_FIELD_NAME);
        const char *name = json_string_value(jsonName);

        if(name)
        {
            LE_DEBUG("file name: %s", name);

            if (!strncmp(name, fileNamePtr, strlen(fileNamePtr)))
            {
                // Same file name, check the hash
                json_t* jsonHash = json_object_get(fileObjectPtr, "hash");
                const char *hash = json_string_value(jsonHash);
                LE_DEBUG("file hash: %s", hash);

                if ((!strncmp(hash, hashPtr, strlen(hashPtr)))
                 && (strncmp(jsonFileNamePtr, FILESTREAM_FILE_DOWNLOAD, strlen(jsonFileNamePtr))))
                {
                    LE_DEBUG("File already exists in the list");
                    isFileAlreadyExists = true;
                }
                if ((!strlen(hash))
                 && (!strncmp(jsonFileNamePtr, FILESTREAM_FILE_DOWNLOAD, strlen(jsonFileNamePtr))))
                {
                    LE_DEBUG("File already exists in the list");
                    isFileAlreadyExists = true;
                }
            }
        }
    }

    if (!isFileAlreadyExists)
    {
        json_t* newFile = json_object();
        if(newFile)
        {
            json_object_set_new(newFile, JSON_FILE_FIELD_NAME,           json_string(fileNamePtr));
            json_object_set_new(newFile, JSON_FILE_FIELD_SIZE,           json_integer(fileSize));
            json_object_set_new(newFile, JSON_FILE_FIELD_STATE,          json_string(statePtr));
            json_object_set_new(newFile, JSON_FILE_FIELD_CLASS,          json_string(classPtr));
            json_object_set_new(newFile, JSON_FILE_FIELD_HASH,           json_string(hashPtr));
            json_object_set_new(newFile, JSON_FILE_FIELD_DIRECTION,      json_integer(direction));
            json_object_set_new(newFile, JSON_FILE_FIELD_ORIGIN,         json_integer(origin));
            json_object_set_new(newFile, JSON_FILE_FIELD_INSTANCE,       json_integer(instanceId));

            json_array_append_new(filePtr, newFile);
            isFileAdded = true;
        }
    }

    LE_DEBUG("%s", json_dumps(root, JSON_COMPACT));

    if (isFileAdded)
    {
        if (LE_OK != WriteFs(jsonFileNamePtr,
                             (uint8_t*)json_dumps(root, JSON_COMPACT),
                             strlen(json_dumps(root, JSON_COMPACT))))
        {
            json_decref(root);
            return LE_FAULT;
        }
    }

    json_decref(root);

    if(isFileAlreadyExists)
    {
        return LE_DUPLICATE;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Update some file details in the JSON file list
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_BAD_PARAMETER  Incorrect parameter provided
 *  - LE_FAULT          The function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FileInfoFromJsonFileUpdate
(
    const char* jsonFileNamePtr,    ///< [IN] File name in which the file info need to be added
    char*       fileNamePtr,        ///< [IN] File name
    char*       statePtr,           ///< [IN] File state
    int32_t     bytesLeft,          ///< [IN] Number of bytes left to be downloaded
    uint16_t    instanceId          ///< [IN] File instance Id (if not UINT16_MAX)
)
{
    char* bufferPtr = NULL;
    size_t bufferSize = 0;
    int i = 0;
    bool isFileAlreadyExists = false;
    size_t length;
    json_error_t error;
    json_t* filePtr;
    json_t* root;

    if ((!jsonFileNamePtr) || (!fileNamePtr) || (!statePtr))
    {
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != le_fs_GetSize(jsonFileNamePtr, &bufferSize))
    {
        LE_DEBUG("Error to get file %s size", jsonFileNamePtr);
        return LE_FAULT;
    }

    if ((strlen(statePtr) == (strlen(FILE_DOWNLOAD_PENDING)))
     && (!strncmp(statePtr, FILE_DOWNLOAD_PENDING, strlen(statePtr))))
    {
        LE_INFO("Pending transfer. File %s, bytes to be downloaded %d", fileNamePtr, bytesLeft);
    }

    bufferPtr = calloc(bufferSize + 1, sizeof(char));

    if (LE_OK != ReadFs(jsonFileNamePtr, (uint8_t*)bufferPtr, &bufferSize))
    {
        free(bufferPtr);
        return LE_FAULT;
    }

    /* Parse the file (JSON) */
    root = json_loads(bufferPtr, 0, &error);
    free(bufferPtr);

    filePtr = json_object_get(root, JSON_FILE_FIELD_FILES);
    if (!filePtr)
    {
        LE_ERROR("Error to find files array");
        return LE_FAULT;
    }

    if(!json_is_array(filePtr))
    {
        LE_ERROR("Files is not an array");
        json_decref(root);
        return LE_FAULT;
    }

    length = json_array_size(filePtr);
    // Check if the filename is not already present
    for (i = 0; i < length; i++)
    {
        json_t* fileObjectPtr = json_array_get(filePtr, i);

        json_t* tempPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_NAME);
        const char *name = json_string_value(tempPtr);
        LE_DEBUG("file name: %s", name);

        if (!strncmp(name, fileNamePtr, strlen(fileNamePtr)))
        {
            LE_DEBUG("File already exists in the list");
            isFileAlreadyExists = true;

            LE_DEBUG("Update download file json");

            // Only update the size if state = FILE_DOWNLOAD_PENDING
            if (!strncmp(statePtr, FILE_DOWNLOAD_PENDING, strlen(FILE_DOWNLOAD_PENDING)))
            {
                tempPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_SIZE);
                int size = 0;
                if (tempPtr)
                {
                    size = json_integer_value(tempPtr);
                }
                if (!size)
                {
                    json_object_set_new(fileObjectPtr, JSON_FILE_FIELD_SIZE, json_integer(bytesLeft));
                }
            }

            json_object_set_new(fileObjectPtr, JSON_FILE_FIELD_STATE, json_string(statePtr));

            if (instanceId != FILE_INSTANCE_ID_DOWNLOADING)
            {
                json_object_set_new(fileObjectPtr,
                                    JSON_FILE_FIELD_INSTANCE,
                                    json_integer(instanceId));
            }
        }
        else
        {
            LE_DEBUG("File name are different: %s - %s", name, fileNamePtr);
        }
    }

    LE_DEBUG("%s", json_dumps(root, JSON_COMPACT));

    if (isFileAlreadyExists)
    {
        if (LE_OK != WriteFs(jsonFileNamePtr, (uint8_t*)json_dumps(root, JSON_COMPACT), strlen(json_dumps(root, JSON_COMPACT))))
        {
            json_decref(root);
            return LE_FAULT;
        }
    }

    json_decref(root);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Update some file details in the JSON file list
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_BAD_PARAMETER  Incorrect parameter provided
 *  - LE_FAULT          The function failed
 *  - LE_NOT_FOUND      The file is not present.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FileInfoFromJsonFileCheckFileName
(
    const char* jsonFileNamePtr,    ///< [IN] File name in which the file info need to be added
    const char* fileNamePtr,        ///< [IN] File name
    const char* fileHashPtr,        ///< [IN] File hash
    uint16_t*   instanceIdPtr       ///< [IN] Instance Id if the file was found
)
{
    char* bufferPtr = NULL;
    size_t bufferSize = 0;
    int i = 0;
    size_t length;
    json_error_t error;
    json_t* filePtr;
    json_t* root;

    if ((!jsonFileNamePtr) || (!fileNamePtr))
    {
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != le_fs_GetSize(jsonFileNamePtr, &bufferSize))
    {
        LE_DEBUG("Error to get file %s size", jsonFileNamePtr);
        return LE_FAULT;
    }

    bufferPtr = calloc(bufferSize + 1, sizeof(char));

    if (LE_OK != ReadFs(jsonFileNamePtr, (uint8_t*)bufferPtr, &bufferSize))
    {
        free(bufferPtr);
        return LE_FAULT;
    }

    /* Parse the file (JSON) */
    root = json_loads(bufferPtr, 0, &error);
    free(bufferPtr);

    filePtr = json_object_get(root, JSON_FILE_FIELD_FILES);
    if (!filePtr)
    {
        LE_ERROR("Error to find files array");
        return LE_FAULT;
    }

    if(!json_is_array(filePtr))
    {
        LE_ERROR("Files is not an array");
        json_decref(root);
        return LE_FAULT;
    }

    length = json_array_size(filePtr);
    // Check if the filename is not already present
    for (i = 0; i < length; i++)
    {
        bool isNameFound = false;
        bool isHashFound = false;
        const char* field;
        json_t* fileObjectPtr = json_array_get(filePtr, i);

        json_t* tempPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_NAME);
        if (tempPtr)
        {
            field = json_string_value(tempPtr);
            if ((field)
             && (strlen(field) == strlen(fileNamePtr))
             && (!strncmp(field, fileNamePtr, strlen(fileNamePtr)))
             )
            {
                LE_DEBUG("File with same name already exists");
                isNameFound = true;
            }
        }

        if (fileHashPtr && strlen(fileHashPtr))
        {
            tempPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_HASH);
            if (tempPtr)
            {
                field = json_string_value(tempPtr);
                if (field && (!strncmp(field, fileHashPtr, strlen(fileHashPtr))))
                {
                    LE_DEBUG("File with same hash already exists");
                    isHashFound = true;
                }
            }
        }
        else
        {
            isHashFound = true;
        }

        if (isNameFound && isHashFound)
        {
            if (fileHashPtr)
            {
                LE_DEBUG("File with same name and same hash already exists");
            }
            else
            {
                LE_DEBUG("File with same name (hash not checked) already exists");
            }
            if (instanceIdPtr)
            {
                json_t* jsonInstancePtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_INSTANCE);
                if (jsonInstancePtr)
                {
                    int instance = json_integer_value(jsonInstancePtr);
                    *instanceIdPtr = (uint16_t)instance;
                }
            }
            json_decref(root);
            return LE_OK;
        }
    }

    json_decref(root);
    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 * Update some file details in the JSON file list
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_BAD_PARAMETER  Incorrect parameter provided
 *  - LE_FAULT          The function failed
 *  - LE_NOT_FOUND      The file is not present
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FileInfoFromJsonFileDelete
(
    const char* jsonFileNamePtr,    ///< [IN] File name in which the file info need to be deleted
    uint16_t    Id                  ///< [IN] File instance
)
{
    char* bufferPtr = NULL;
    size_t bufferSize = 0;
    bool isInstanceFound = false;
    json_error_t error;
    json_t* filePtr;
    json_t* root;
    int i = 0;
    size_t length;

    if (!jsonFileNamePtr)
    {
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != le_fs_GetSize(jsonFileNamePtr, &bufferSize))
    {
        LE_DEBUG("Error to get file %s size", jsonFileNamePtr);
        return LE_FAULT;
    }

    bufferPtr = calloc(bufferSize + 1, sizeof(char));

    if (LE_OK != ReadFs(jsonFileNamePtr, (uint8_t*)bufferPtr, &bufferSize))
    {
        free(bufferPtr);
        return LE_FAULT;
    }

    /* Parse the file (JSON) */
    root = json_loads(bufferPtr, 0, &error);
    free(bufferPtr);

    if (!root)
    {
        LE_ERROR("Error to parse the file list");
        return LE_FAULT;
    }

    filePtr = json_object_get(root, JSON_FILE_FIELD_FILES);
    if (!filePtr)
    {
        LE_ERROR("Error to find files array");
        json_decref(root);
        return LE_FAULT;
    }

    if(!json_is_array(filePtr))
    {
        LE_ERROR("Files is not an array");
        json_decref(root);
        return LE_FAULT;
    }

    length = json_array_size(filePtr);
    for (i = 0; i < length; i++)
    {
        json_t* fileObjectPtrPtr = json_array_get(filePtr, i);
        json_t* jsonInstancePtr = json_object_get(fileObjectPtrPtr, JSON_FILE_FIELD_INSTANCE);
        if (jsonInstancePtr)
        {
            int instance = json_integer_value(jsonInstancePtr);
            if(instance == Id)
            {
                // Delete
                isInstanceFound = true;

                json_t* jsonFileName = json_object_get(fileObjectPtrPtr, JSON_FILE_FIELD_NAME);
                const char* name = (char*)json_string_value(jsonFileName);
                if(!name)
                {
                    LE_ERROR("File name is NULL");
                }
                else
                {
                    le_result_t result;
                    size_t len = LE_FILESTREAMCLIENT_FILE_NAME_MAX_BYTES
                                 + strlen(FILESTREAM_LEFS_DIR)
                                 + strlen(FILESTREAM_STORAGE_LEFS_DIR) + 10;
                    char namePtr[len];

                    snprintf(namePtr,
                             len,
                             "%s%s/%s",
                             FILESTREAM_LEFS_DIR,
                             FILESTREAM_STORAGE_LEFS_DIR,
                             name);

                    result = le_fs_Delete(namePtr);
                    if (LE_OK != result)
                    {
                        LE_DEBUG("File %s was NOT deleted: %s", name, LE_RESULT_TXT(result));
                    }
                    else
                    {
                        LE_ERROR("File %s was deleted", name);
                    }

                    // Indicate that the instance Id is available
                    if (instance < LE_FILESTREAMSERVER_FILE_MAX_NUMBER)
                    {
                        IsFileInstanceUsed[instance] = false;
                    }

                    json_array_remove(filePtr, i);
                    break;
                }
            }
        }
    }

    LE_DEBUG("%s", json_dumps(root, JSON_COMPACT));

    if (!isInstanceFound)
    {
        json_decref(root);
        return LE_NOT_FOUND;
    }

    if (LE_OK != WriteFs(jsonFileNamePtr,
                         (uint8_t*)json_dumps(root, JSON_COMPACT),
                         strlen(json_dumps(root, JSON_COMPACT))))
    {
        json_decref(root);
        return LE_FAULT;
    }

    json_decref(root);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy the file from FILESTREAM_FILE_DOWNLOAD to FILESTREAM_FILE_LIST
 * The FILESTREAM_FILE_LIST file only includes successfully downloaded files = available files
 */
//--------------------------------------------------------------------------------------------------
static void MoveDownloadedFileToFileList
(
    void
)
{
    char* bufferPtr = NULL;
    size_t bufferSize = 0;
    int i = 0;
    json_error_t error;
    json_t* filePtr;
    json_t* root;

    const char *namePtr = "";
    const char *statePtr = "";
    const char *classPtr = "";
    const char *hashPtr = "";
    int size = 0;
    int direction = 0;
    int origin = 0;
    int instance = 0;
    json_t* tempPtr;

    LE_DEBUG("Copy the downloaded file from %s to %s",
             FILESTREAM_FILE_DOWNLOAD, FILESTREAM_FILE_LIST);

    if (LE_OK != le_fs_GetSize(FILESTREAM_FILE_DOWNLOAD, &bufferSize))
    {
        LE_DEBUG("Error to get file %s size", FILESTREAM_FILE_DOWNLOAD);
        return;
    }

    bufferPtr = calloc(bufferSize + 1, sizeof(char));

    if (LE_OK != ReadFs(FILESTREAM_FILE_DOWNLOAD, (uint8_t*)bufferPtr, &bufferSize))
    {
        LE_DEBUG("Can not read %s", FILESTREAM_FILE_DOWNLOAD);
        free(bufferPtr);
        return;
    }

    /* Parse the file (JSON) */
    root = json_loads(bufferPtr, 0, &error);
    free(bufferPtr);

    if (!root)
    {
        LE_ERROR("Error to parse the file list");
        return;
    }

    filePtr = json_object_get(root, JSON_FILE_FIELD_FILES);
    if (!filePtr)
    {
        LE_ERROR("Error to find files array");
        json_decref(root);
        return;
    }

    if(!json_is_array(filePtr))
    {
        LE_ERROR("Files is not an array");
        json_decref(root);
        return;
    }

    json_t* fileObjectPtr = json_array_get(filePtr, i);
    if (NULL == fileObjectPtr)
    {
        LE_DEBUG("JSON error: not an array");
    }

    tempPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_NAME);
    if (tempPtr)
    {
        namePtr = json_string_value(tempPtr);
        LE_DEBUG("file name: %s", namePtr);
    }

    tempPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_SIZE);
    if (tempPtr)
    {
        size = json_integer_value(tempPtr);
        LE_DEBUG("file size: %d", size);
    }

    tempPtr= json_object_get(fileObjectPtr, JSON_FILE_FIELD_STATE);
    if (tempPtr)
    {
        statePtr = json_string_value(tempPtr);
        LE_DEBUG("file state: %s", statePtr);
    }

    tempPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_CLASS);
    if (tempPtr)
    {
        classPtr = json_string_value(tempPtr);
        LE_DEBUG("file class: %s", classPtr);
    }

    tempPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_HASH);
    if (tempPtr)
    {
        hashPtr = json_string_value(tempPtr);
        LE_DEBUG("file hash: %s", hashPtr);
    }

    tempPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_DIRECTION);
    if (tempPtr)
    {
        int direction = json_integer_value(tempPtr);
        LE_DEBUG("file direction: %d", direction);
    }

    tempPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_ORIGIN);
    if (tempPtr)
    {
        origin = json_integer_value(tempPtr);
        LE_DEBUG("file origin: %d", origin);
    }

    tempPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_INSTANCE);
    if (tempPtr)
    {
        instance = json_integer_value(tempPtr);
        LE_DEBUG("file origin: %d", origin);
    }

    if (LE_OK == FileInfoFromJsonFileAdd(FILESTREAM_FILE_LIST,
                                         (char*)namePtr,
                                         (char*)statePtr,
                                         (char*)classPtr,
                                         (char*)hashPtr,
                                         size,
                                         direction,
                                         origin,
                                         instance))
    {
        DeleteFs(FILESTREAM_FILE_DOWNLOAD);
        CreateDefaultJsonFile(FILESTREAM_FILE_DOWNLOAD);
    }

    json_decref(root);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function which manage the instance list of available files
 */
//--------------------------------------------------------------------------------------------------
static void InitializeFileInstances
(
    void
)
{
    uint16_t fileInstanceList[LE_FILESTREAMSERVER_FILE_MAX_NUMBER];
    uint32_t loop = 0;
    uint32_t instanceNb = 0;

    memset(fileInstanceList, UINT16_MAX, sizeof(uint16_t)*(LE_FILESTREAMSERVER_FILE_MAX_NUMBER));

    for (loop = 0; loop < LE_FILESTREAMSERVER_FILE_MAX_NUMBER; loop++)
    {
        IsFileInstanceUsed[loop] = false;
    }

    // Read the FILESTREAM_FILE_LIST file in order to check which instance Ids are already used
    ListFileInstanceFromJsonFile(FILESTREAM_FILE_LIST, fileInstanceList, &instanceNb);

    for (loop = 0; loop < instanceNb; loop++)
    {
        if (fileInstanceList[loop] < LE_FILESTREAMSERVER_FILE_MAX_NUMBER)
        {
            IsFileInstanceUsed[fileInstanceList[loop]] = true;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function which manage the instance list of available files
 *
 * @return
 *      - UINT16_MAX if all instances were used
 *      - uint16 value else
 */
//--------------------------------------------------------------------------------------------------
static uint16_t FindNextAvailableFileInstanceId
(
    void
)
{
    uint16_t loop = 0;

    for (loop = 0; loop < LE_FILESTREAMSERVER_FILE_MAX_NUMBER; loop++)
    {
        if(IsFileInstanceUsed[loop] == false)
        {
            LE_DEBUG("Next file instance Id: %d", loop);
            return loop;
        }
    }
    return UINT16_MAX;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume file stream
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_BAD_PARAMETER  Incorrect parameter provided
 *  - LE_OVERFLOW       The file path is too long
 *  - LE_FAULT          The function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ResumeStreamInfo //ToDo: to remove?
(
    void
)
{
    le_fileStreamClient_StreamMgmt_t streamMgmtObj;
    uint16_t instanceId = FILE_INSTANCE_ID_DOWNLOADING;

    memset((char*)&streamMgmtObj, 0, sizeof(le_fileStreamClient_StreamMgmt_t));

    fileStreamClient_GetStreamMgmtObject(instanceId, &streamMgmtObj);

    // Resume Stream Management Object
    fileStreamClient_SetStreamMgmtObject((le_fileStreamClient_StreamMgmt_t*)&streamMgmtObj);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copy the downloaded bytes to Fd
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_FAULT          The function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteBytesToFd
(
    le_fs_FileRef_t     fileReference,      ///< [IN] File reference to write to
    uint8_t*            bufferPtr,          ///< [IN] Buffer pointer to Downloaded bytes
    size_t              readCount           ///< [IN] Number of bytes downloaded
)
{
    le_result_t result = le_fs_Write(fileReference, bufferPtr, readCount);
    LE_DEBUG("le_fs_Write return %d (%s)", result, LE_RESULT_TXT(result));

    if (LE_OK != result)
    {
        LE_WARN("Write data error %d (%s)", result, LE_RESULT_TXT(result));
    }

    if (LE_OK != result)
    {
        return LE_FAULT;
    }

    StreamContext.bytesReceived += readCount;

    LE_DEBUG("Bytes written: %zd. Total bytes streamed: %zd",
             readCount, StreamContext.bytesReceived);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Copy the downloaded bytes from read pipe to write pipe. This function should be called
 * only when data is available on pipe (example: EPOLLIN event triggered).
 *
 * @return
 *  - LE_OK if some bytes are copied.
 *  - LE_TERMINATED write end of update pipe is closed.
 *  - LE_WOULD_BLOCK if no data available on update pipe.
 *  - LE_FAULT if there is an error.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CopyBytesToFd
(
    le_fs_FileRef_t fileReference,  ///< [IN] File descriptor to store
    int             readFd,         ///< [IN] File descriptor to read
    ssize_t*        bytesCopied     ///< [OUT] Number of bytes copied on success. Undefined otherwise.
)
{
    uint8_t buffer[READ_CHUNK_BYTES];
    ssize_t readCount;

    if ((readFd < 0) || (!fileReference))
    {
        LE_CRIT("Bad file descriptor");
        return LE_FAULT;
    }

readRemainingBytes:
    LE_DEBUG("start reading bytes");

    // Read the bytes, retrying if interrupted by a signal.
    do
    {
        readCount = read(readFd, buffer, sizeof(buffer));
    }
    while ((-1 == readCount) && (EINTR == errno));

    LE_DEBUG("Received '%d' bytes", readCount);

    // Write the bytes read to disk.
    if (0 == readCount)
    {
        // Incurred end of file. Close the Read fd.
        LE_INFO("Update pipe closed, finished storing; %zd bytes stored",
                StreamContext.bytesReceived);
        return LE_TERMINATED;
    }
    else if (readCount > 0)
    {
        if (LE_OK != WriteBytesToFd(fileReference, buffer, readCount))
        {
            LE_ERROR("Failed to process downloaded data");
            return LE_FAULT;
        }

        *bytesCopied = readCount;

        if (readCount == sizeof(buffer))
        {
            goto readRemainingBytes;
        }

        LE_DEBUG("No more data, wait for fd event: %d", readFd);
        return LE_OK;
    }
    else
    {
        // readCount is negative here. Check errno.
        if ((EAGAIN == errno) || (EWOULDBLOCK == errno))
        {
            LE_DEBUG("No data available yet, wait for fd event: %d", readFd);
            return LE_OK;
        }

        LE_ERROR("Error while reading fd: %d. %m", readFd);
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Wait for event on fd
 *
 * @return
 *      - LE_OK             On success
 *      - LE_FAULT          On failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WaitForFdEvent
(
    int fd,
    int efd
)
{
    int n;
    struct epoll_event events[MAX_EVENTS];

    while (1)
    {
        if (-1 == efd)
        {
            // fd is a regular file, not compliant with epoll, simulate it
            n = 1;
            events[0].events = EPOLLIN;
            events[0].data.fd = fd;
        }
        else
        {
            LE_DEBUG("Start epoll wait");
            n = epoll_wait(efd, events, sizeof(events), DEFAULT_TIMEOUT_MS);
            LE_DEBUG("n=%d", n);
        }
        switch (n)
        {
            case -1:
                LE_ERROR("epoll_wait error %m");
                return LE_FAULT;
            case 0:
                LE_DEBUG("Timeout");
                return LE_TIMEOUT;
            default:
                for(;n--;)
                {
                    LE_DEBUG("events[%d] .data.fd=%d .events=0x%x",
                             n, events[n].data.fd, events[n].events);
                    if (events[n].data.fd == fd)
                    {
                        uint32_t evts = events[n].events;

                        if (evts & EPOLLERR)
                        {
                            LE_ERROR("Error on epoll wait");
                            return LE_FAULT;
                        }
                        else if (evts & EPOLLIN)
                        {
                            LE_DEBUG("Read bytes from package downloader");
                            return LE_OK;
                        }
                        else if ((evts & EPOLLRDHUP ) || (evts & EPOLLHUP))
                        {
                            // file descriptor has been closed
                            LE_INFO("file descriptor %d has been closed", fd);
                            return LE_CLOSED;
                        }
                        else
                        {
                            LE_WARN("unexpected event received 0x%x",
                                    evts & ~(EPOLLRDHUP|EPOLLHUP|EPOLLERR|EPOLLIN));
                            return LE_FAULT;
                        }
                    }
                }
                break;
        }
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create and configure an event notification
 *
 * @return
 *      - LE_OK             On success
 *      - LE_FAULT          On failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateAndConfEpoll
(
    int  fd,            ///< [IN] file descriptor
    int* efdPtr         ///< [OUT] event file descriptor
)
{
    struct epoll_event event;
    int efd = epoll_create1(0);
    if (efd == -1)
    {
        return LE_FAULT;
    }

    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    if (-1 == epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event))
    {
        LE_ERROR("epoll_ctl error %m");
        return LE_FAULT;
    }

    *efdPtr = efd;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure a file descriptor as NON-BLOCK
 *
 * @return
 *      - LE_OK             On success
 *      - LE_FAULT          On failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t MakeFdNonBlocking
(
    int fd      ///< [IN] file descriptor
)
{
    int flags;

    flags = fcntl(fd, F_GETFL);
    if (-1 == flags)
    {
        LE_ERROR("Fails to GETFL fd %d: %m", fd);
        return LE_FAULT;
    }
    else
    {
        if (-1 == fcntl(fd, F_SETFL, flags | O_NONBLOCK))
        {
            LE_ERROR("Fails to SETFL fd %d: %m", fd);
            return LE_FAULT;
        }
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Prepare the file descriptor to be used for download
 *
 * @return
 *      - LE_OK             On success
 *      - LE_FAULT          On failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PrepareFd
(
    int  fd,            ///< [IN] file descriptor
    bool isRegularFile, ///< [IN] flag to indicate if the file descriptor is related to a regular
                        ///<      file or not
    int* efdPtr         ///< [OUT] event file descriptor
)
{
    /* Like we use epoll(2), force the O_NONBLOCK flags in fd */
    if (LE_OK != MakeFdNonBlocking(fd))
    {
        return LE_FAULT;
    }

    if (!isRegularFile)
    {
        if (LE_OK != CreateAndConfEpoll(fd, efdPtr))
        {
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check the file descriptor type
 *
 * @return
 *      - LE_OK             If fd is socket, pipe or regular file
 *      - LE_FAULT          On other file descriptor type
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CheckFdType
(
    int fd,                     ///< [IN] file descriptor to test
    bool *isRegularFilePtr      ///< [OUT] true if fd is a regular file
)
{
    struct stat buf;

    if (-1 == fstat(fd, &buf))
    {
        LE_ERROR("fstat error %m");
        return LE_FAULT;
    }

    switch (buf.st_mode & S_IFMT)
    {
        case 0:       // unknown type
        case S_IFDIR: // directory
        case S_IFLNK: // link
            LE_ERROR("Bad file descriptor type 0x%x", buf.st_mode & S_IFMT);
            return LE_FAULT;

        case S_IFIFO:  // fifo or pipe
        case S_IFSOCK: // socket
            LE_DEBUG("Socket, fifo or pipe");
            *isRegularFilePtr = false;
            break;

        default:
            LE_DEBUG("Regular file");
            *isRegularFilePtr = true;
            break;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Cleanup thread function
 *
 */
//--------------------------------------------------------------------------------------------------
static void CleanUp
(
    void *contextPtr                                ///< [IN] Context from ProcessStream thread
)
{
    StreamContext_t* streamCtxtPtr = (StreamContext_t*)contextPtr;

    LE_INFO("Process Stream exited");

    if (streamCtxtPtr->readFd)
    {
        LE_DEBUG("Close the read fd");
        close(streamCtxtPtr->readFd);
    }

    if (streamCtxtPtr->fileRef)
    {
        le_fs_Close(streamCtxtPtr->fileRef);
    }

    fileStreamServer_SetClientDownloadComplete();
}


//--------------------------------------------------------------------------------------------------
/**
 *  Thread to process data received from the package downloader. Either stores the content to file
 *  system if received on topic 'local' or writes the content to a pipe owned by the client
 *  application
 *
 *  @return
 *      - LE_OK when process completed successfully
 *      - LE_BAD_PARAMETER if the file descriptor is bad
 *      - LE_ERROR for all other failures
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ProcessStream
(
    void* contextPtr                                ///< [IN] Process received stream
)
{
    le_result_t result;
    int efd = -1;
    StreamContext_t* StreamCtxtPtr = (StreamContext_t*)contextPtr;

    LE_DEBUG("Start processing the received stream");

    le_thread_AddDestructor(CleanUp, (void*)&StreamContext);

    StreamContext.bytesReceived = 0;

    // Do check whether all data are read from pipe.
    if (StreamCtxtPtr->readFd != -1)
    {
        bool isRegularFile;

        if (LE_OK != CheckFdType(StreamCtxtPtr->readFd, &isRegularFile))
        {
            LE_ERROR("Bad file descriptor: %d", StreamCtxtPtr->readFd);
            return LE_BAD_PARAMETER;
        }

        /* Like we use epoll(2), force the O_NONBLOCK flags in fd */
        result = PrepareFd(StreamCtxtPtr->readFd, isRegularFile, &efd);
        if (result != LE_OK)
        {
            LE_ERROR("Fail to prepare fd: %d", StreamCtxtPtr->readFd);

            // fd passed here should be closed as it is duped and sent here by messaging api.
            close(StreamCtxtPtr->readFd);
            return LE_FAULT;
        }

        // Still some data in the pipe. Finish reading it.
        while (true)
        {
            LE_DEBUG("Start waiting for an event");
            result = WaitForFdEvent(StreamCtxtPtr->readFd, efd);

            if (result == LE_CLOSED)
            {
                LE_DEBUG("Fd closed");
                return result;
            }
            else if (result != LE_OK)
            {
                LE_DEBUG("result = %s", LE_RESULT_TXT(result));
                return result;
            }

            ssize_t bytesCopied = 0;
            result = CopyBytesToFd(//StreamCtxtPtr->writeFd,
                                   StreamCtxtPtr->fileRef,
                                   StreamCtxtPtr->readFd,
                                   &bytesCopied);

            if (LE_TERMINATED == result)
            {
                LE_INFO("Finished reading update package. Package size: %zd bytes",
                        StreamContext.bytesReceived);
                return LE_OK;
            }
            else if (LE_OK == result)
            {
                LE_DEBUG("Bytes copied: %zd", bytesCopied);
            }
            else
            {
                LE_ERROR("Failure in storing update package: %s", LE_RESULT_TXT(result));
                return LE_FAULT;
            }
        }
    }
    else
    {
        // Update pipe is closed before calling this function. This means POLLHUP is already
        // received by the fdMonitor event handler function. In this case, there is a chance that
        // SOTA state may not be updated properly. So, set it properly.
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the number of files in the JSON file list
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_BAD_PARAMETER  Incorrect parameter provided
 *  - LE_FAULT          The function failed
 */
//--------------------------------------------------------------------------------------------------
/*
le_result_t pa_fileStream_GetFileNb
(
    uint32_t*   fileNumberPtr   ///< [INOUT] Number of files
)
{
    char* bufferPtr = NULL;
    size_t bufferSize = 0;
    json_error_t error;
    json_t* filePtr;
    json_t* root;

    if (!fileNumberPtr)
    {
        return LE_BAD_PARAMETER;
    }

    *fileNumberPtr = 0;

    if (LE_OK != le_fs_GetSize(FILESTREAM_FILE_LIST, &bufferSize))
    {
        LE_DEBUG("Error to get file %s size", FILESTREAM_FILE_LIST);
        return LE_FAULT;
    }

    bufferPtr = calloc(bufferSize + 1, sizeof(char));

    if (LE_OK != ReadFs(FILESTREAM_FILE_LIST, (uint8_t*)bufferPtr, &bufferSize))
    {
        free(bufferPtr);
        return LE_FAULT;
    }

    // Parse the file (JSON)
    root = json_loads(bufferPtr, 0, &error);
    free(bufferPtr);
    if (!root)
    {
        LE_ERROR("Error to parse the file list");
        return LE_FAULT;
    }

    filePtr = json_object_get(root, JSON_FILE_FIELD_FILES);
    if (!filePtr)
    {
        LE_ERROR("Error to find files array");
        json_decref(root);
        return LE_FAULT;
    }

    if(!json_is_array(filePtr))
    {
        LE_ERROR("Files is not an array");
        json_decref(root);
        return LE_FAULT;
    }

    size_t length = json_array_size(filePtr);
    *fileNumberPtr = (uint32_t)length;
    LE_INFO("File number in JSON file: %d", *fileNumberPtr);
    json_decref(root);
    return LE_OK;
}
*/

//--------------------------------------------------------------------------------------------------
/**
 * Get file info from instance Id
 *
 * @return
 *  - LE_OK             The function succeeded
 *  - LE_BAD_PARAMETER  Incorrect parameter provided
 *  - LE_FAULT          The function failed
 *  - LE_NOT_FOUND      The file is not present
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fileStream_GetFileInfoByInstance
(
    uint16_t    instanceId,                 ///< [IN]   File instance
    char*       fileNamePtr,                ///< [OUT]  File name buffer
    size_t      fileNameNumElements,        ///< [IN]   File name buffer size (\0 included)
    char*       fileTopicPtr,               ///< [OUT]  File topic buffer
    size_t      fileTopicNumElements,       ///< [IN]   File topic buffer size (\0 included)
    char*       fileHashPtr,                ///< [OUT]  File hash buffer
    size_t      fileHashNumElements,        ///< [IN]   File hash buffer size (\0 included)
    uint64_t*   fileSizePtr,                ///< [OUT]  File size
    uint8_t*    fileOriginPtr               ///< [OUT]  File origin
)
{
    le_result_t result;

    if ((LE_FILESTREAMSERVER_FILE_MAX_NUMBER < instanceId)
     && (FILE_INSTANCE_ID_DOWNLOADING != instanceId))
    {
        return LE_BAD_PARAMETER;
    }

    // Search in the download file
    result = FileInfoFromJsonFileRead(FILESTREAM_FILE_DOWNLOAD,
                                      instanceId,
                                      fileNamePtr,
                                      fileNameNumElements,
                                      fileTopicPtr,
                                      fileTopicNumElements,
                                      fileHashPtr,
                                      fileHashNumElements,
                                      fileSizePtr,
                                      fileOriginPtr);
    if (LE_OK != result)
    {
        // Search in the file list
        result = FileInfoFromJsonFileRead(FILESTREAM_FILE_LIST,
                                          instanceId,
                                          fileNamePtr,
                                          fileNameNumElements,
                                          fileTopicPtr,
                                          fileTopicNumElements,
                                          fileHashPtr,
                                          fileHashNumElements,
                                          fileSizePtr,
                                          fileOriginPtr);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Init Stream
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fileStream_InitStream
(
    void
)
{
    le_fs_FileRef_t fileReference;

    // Get the stream management object
    le_fileStreamClient_StreamMgmt_t streamMgmtObj;
    uint16_t instanceId = FILE_INSTANCE_ID_DOWNLOADING;

    fileStreamClient_GetStreamMgmtObject(instanceId, (le_fileStreamClient_StreamMgmt_t*)&streamMgmtObj);

    LE_DEBUG("Saving file to %s", streamMgmtObj.pkgName);

    // Create new download file
    size_t len = LE_FILESTREAMCLIENT_FILE_NAME_MAX_BYTES
                    + strlen(FILESTREAM_LEFS_DIR)
                    + strlen(FILESTREAM_STORAGE_LEFS_DIR)
                    + 10;
    char namePtr[len];
    snprintf(namePtr,
             len,
             "%s%s/%s",
             FILESTREAM_LEFS_DIR,
             FILESTREAM_STORAGE_LEFS_DIR,
             streamMgmtObj.pkgName);
    if (LE_OK != le_fs_Open(namePtr, LE_FS_WRONLY | LE_FS_TRUNC | LE_FS_CREAT, &fileReference))
    {
        LE_ERROR("Unable to open file '%s' for writing.", streamMgmtObj.pkgName);
        return LE_FAULT;
    }
    le_fs_Close(fileReference);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the resume position for following stream
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_fileStream_SetResumePosition
(
    size_t position                 ///< [IN] Offset of a file
)
{
    LE_UNUSED(position);

    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Find resume position of the stream currently in progress
 *
 * @return
 *      - LE_OK if able to retrieve resume position
 *      - LE_FAULT otherwise
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fileStream_GetResumePosition
(
    size_t* positionPtr                             ///< [OUT] Stream resume position
)
{
    le_result_t result = LE_FAULT;
    uint16_t instanceId = FILE_INSTANCE_ID_DOWNLOADING;

    // Set resume position to 0.
    *positionPtr = 0;

    le_fileStreamClient_StreamMgmt_t streamMgmtObj;
    fileStreamClient_GetStreamMgmtObject(instanceId, (le_fileStreamClient_StreamMgmt_t*)&streamMgmtObj);

    size_t len = LE_FILESTREAMCLIENT_FILE_NAME_MAX_BYTES
                    + strlen(FILESTREAM_LEFS_DIR)
                    + strlen(FILESTREAM_STORAGE_LEFS_DIR)
                    + 10;
    char namePtr[len];
    snprintf(namePtr,
             len,
             "%s%s/%s",
             FILESTREAM_LEFS_DIR,
             FILESTREAM_STORAGE_LEFS_DIR,
             streamMgmtObj.pkgName);
    result = le_fs_GetSize(namePtr, positionPtr);
    if (LE_OK != result)
    {
        if (LE_NOT_FOUND == result)
        {
            LE_DEBUG("No file to resume");
        }
        else
        {
            LE_ERROR("Error to get file %s size (%s)", namePtr, LE_RESULT_TXT(result));
        }
        return LE_FAULT;
    }

    LE_INFO("Size of downloaded file = %zd", *positionPtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start Download
 *
 * @return
 *      - LE_OK on success.
 *      - LE_UNAVAILABLE if no registered application for the class
 *      - LE_FAULT if failed.
 *
 * Note: Stream Sync Object should be set with all the parameters for a the stream job
 * before calling this function
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fileStream_Download
(
    const le_fileStreamClient_StreamMgmt_t* streamMgmtObj,  ///< [IN] Stream management object
    int readFd                                              ///< [IN] File discriptor for reading
                                                            ///<      file content
)
{
    le_fs_FileRef_t fileRef;
    LE_DEBUG("Caching file %s", streamMgmtObj->pkgName);

    // Create new download file
    size_t len = LE_FILESTREAMCLIENT_FILE_NAME_MAX_BYTES +
                 strlen(FILESTREAM_LEFS_DIR) +
                 strlen(FILESTREAM_STORAGE_LEFS_DIR) +
                 10;
    char namePtr[len];
    snprintf(namePtr,
             len,
             "%s%s/%s",
             FILESTREAM_LEFS_DIR,
             FILESTREAM_STORAGE_LEFS_DIR,
             streamMgmtObj->pkgName);
    if (LE_OK != le_fs_Open(namePtr, LE_FS_WRONLY | LE_FS_APPEND, &fileRef))
    {
        LE_ERROR("Failed to create file");
        return LE_FAULT;
    }

    // Set the stream context
    StreamContext.readFd = readFd;
    StreamContext.fileRef = fileRef;
    strncpy(StreamContext.topic,
            streamMgmtObj->pkgTopic,
            LE_FILESTREAMCLIENT_FILE_TOPIC_MAX_BYTES);

    // Process received stream
    ProcessStreamThreadRef = le_thread_Create("ProcessStream",
                                              (void *)ProcessStream,
                                              (void *)&StreamContext);
    le_thread_SetJoinable(ProcessStreamThreadRef);
    le_thread_Start(ProcessStreamThreadRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process download stream status events. Receive status and process it locally or pass to
 * interested applications.
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_fileStream_DownloadStatus
(
    le_fileStreamClient_DownloadState_t status,     ///< [IN] Download status
    int32_t bytesLeft,                              ///< [IN] Number of bytes left to be downloaded
    int32_t progress                                ///< [IN] Download progress in %
)
{
    le_fileStreamClient_StreamMgmt_t streamMgmtObj;
    uint16_t instanceId = 0;

    LE_INFO("Download Status %d, bytes left is %d, progress is %d%%", status, bytesLeft, progress);


    if (LE_FILESTREAMCLIENT_DOWNLOAD_IDLE == status)
    {
        le_result_t result;
         // Get the stream management object
        instanceId = FILE_INSTANCE_ID_DOWNLOADING;
        fileStreamClient_GetStreamMgmtObject(instanceId, (le_fileStreamClient_StreamMgmt_t*)&streamMgmtObj);
        result = FileInfoFromJsonFileAdd(FILESTREAM_FILE_DOWNLOAD,
                                         streamMgmtObj.pkgName,
                                         FILE_DOWNLOAD_NO_SIZE,
                                         streamMgmtObj.pkgTopic,
                                         streamMgmtObj.hash,
                                         streamMgmtObj.pkgSize,
                                         streamMgmtObj.direction,
                                         streamMgmtObj.origin,
                                         streamMgmtObj.instanceId);
        if (LE_DUPLICATE == result)
        {
            DeleteFs(FILESTREAM_FILE_DOWNLOAD);
            CreateDefaultJsonFile(FILESTREAM_FILE_DOWNLOAD);
            result = FileInfoFromJsonFileAdd(FILESTREAM_FILE_DOWNLOAD,
                                         streamMgmtObj.pkgName,
                                         FILE_DOWNLOAD_NO_SIZE,
                                         streamMgmtObj.pkgTopic,
                                         streamMgmtObj.hash,
                                         streamMgmtObj.pkgSize,
                                         streamMgmtObj.direction,
                                         streamMgmtObj.origin,
                                         streamMgmtObj.instanceId);
        }
        return;
    }

    if (LE_FILESTREAMCLIENT_DOWNLOAD_PENDING == status)
    {
        // First check if info are already available in case of suspended download
        le_result_t result = ResumeStreamInfo();
        LE_DEBUG("ResumeStreamInfo return %d", result);

        if (LE_OK == result)
        {
            instanceId = FILE_INSTANCE_ID_DOWNLOADING;
            fileStreamClient_GetStreamMgmtObject(instanceId, (le_fileStreamClient_StreamMgmt_t*)&streamMgmtObj);
        }
    }

    instanceId = FILE_INSTANCE_ID_DOWNLOADING;
    fileStreamClient_GetStreamMgmtObject(instanceId, (le_fileStreamClient_StreamMgmt_t*)&streamMgmtObj);

    char configPath[LE_CFG_STR_LEN_BYTES] = {0};
    snprintf(configPath, sizeof(configPath), "%s/%s", CFG_FILESTREAM, streamMgmtObj.pkgTopic);

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn(configPath);
    le_cfg_SetString(iteratorRef, "packageName", streamMgmtObj.pkgName);
    le_cfg_SetInt(iteratorRef, "packageOption", streamMgmtObj.direction); // TODO: to keep ?

    switch (status)
    {
        case LE_FILESTREAMCLIENT_DOWNLOAD_IDLE:
        break;

        case LE_FILESTREAMCLIENT_DOWNLOAD_PENDING:
        {
            FileInfoFromJsonFileUpdate(FILESTREAM_FILE_DOWNLOAD,
                                       streamMgmtObj.pkgName,
                                       FILE_DOWNLOAD_PENDING,
                                       bytesLeft,
                                       FILE_INSTANCE_ID_DOWNLOADING);
        }
        break;

        case LE_FILESTREAMCLIENT_DOWNLOAD_IN_PROGRESS:
            FileInfoFromJsonFileUpdate(FILESTREAM_FILE_DOWNLOAD,
                                       streamMgmtObj.pkgName,
                                       FILE_DOWNLOAD_ON_GOING,
                                       0,
                                       FILE_INSTANCE_ID_DOWNLOADING);
            break;

        case LE_FILESTREAMCLIENT_DOWNLOAD_COMPLETED:
        {
            uint16_t newInstanceId = UINT16_MAX;
            newInstanceId = FindNextAvailableFileInstanceId();
            if (newInstanceId == UINT16_MAX)
            {
                LE_CRIT("Max file number was already reached and a new file was downloaded");
                break;
            }
            else
            {
                LE_DEBUG("Set new file instance Id: %d", newInstanceId);
                IsFileInstanceUsed[newInstanceId] = true;
            }
            FileInfoFromJsonFileUpdate(FILESTREAM_FILE_DOWNLOAD,
                                       streamMgmtObj.pkgName,
                                       FILE_DOWNLOAD_SUCCESS,
                                       0,
                                       newInstanceId);
            // Copy the file from FILESTREAM_FILE_DOWNLOAD to FILESTREAM_FILE_LIST
            MoveDownloadedFileToFileList();
        }
        break;

        case LE_FILESTREAMCLIENT_DOWNLOAD_FAILED:
            // The download failed, so the FILESTREAM_FILE_DOWNLOAD file can be reset
            FileInfoFromJsonFileDelete(FILESTREAM_FILE_DOWNLOAD, FILE_INSTANCE_ID_DOWNLOADING);

            break;

        default:
        break;
    }

    // Provide fd to fully cached file
    if ((status == LE_FILESTREAMCLIENT_DOWNLOAD_COMPLETED) &&
        (streamMgmtObj.direction == LE_FILESTREAMCLIENT_DIRECTION_DOWNLOAD))
    {
        LE_INFO("File is cached successfully!");

        // This would not work in streaming because complete doesn't imply cached
        le_cfg_SetBool(iteratorRef, "packageDownloaded", true);

        StreamObject_t* streamObjPtr = le_hashmap_Get(StreamObjTable, streamMgmtObj.pkgTopic);
        if ((streamObjPtr != NULL) &&
            (streamObjPtr->streamCb != NULL))
        {
            le_utf8_Copy(streamObjPtr->name,
                         streamMgmtObj.pkgName,
                         sizeof(streamObjPtr->name),
                         NULL);
            streamObjPtr->cached = le_cfg_GetBool(iteratorRef, "packageDownloaded", false);

            int fd = open(streamMgmtObj.pkgName, O_RDONLY);

            if (fd == -1)
            {
                LE_ERROR("Unable to open: %s", streamMgmtObj.pkgName);
                return;
            }
            streamObjPtr->streamCb(fd, streamObjPtr->contextPtr);
            close(fd);
        }
        else
        {
            LE_DEBUG("streamCb NULL");
        }

    }

    le_cfg_CommitTxn(iteratorRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Call client with the fd to the cache file
 * Manage case where files are cached and new app registers to this service requesting this topic.
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_fileStream_ProcessCacheClient
(
    StreamObject_t* streamObjPtr    ///< [IN] Stream object
)
{
    LE_ASSERT(streamObjPtr);

    if (streamObjPtr->streamCb != NULL &&
        streamObjPtr->cached)
    {

        int fd = open(streamObjPtr->name, O_RDONLY);

        if (fd == -1)
        {
            LE_ERROR("Unable to open: %s", streamObjPtr->name);
            return;
        }

        streamObjPtr->streamCb(fd, streamObjPtr->contextPtr);
        close(fd);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete a file by instance id
 *
 * @return
 *      - LE_OK on success.
 *      - LE_BAD_PARAMETER if instance does no exist
 *      - LE_FAULT if failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fileStream_DeleteFileByInstance
(
    uint16_t    instanceId          ///< [IN] File instance
)
{
    le_result_t result;

    if ((LE_FILESTREAMSERVER_FILE_MAX_NUMBER < instanceId)
     && (FILE_INSTANCE_ID_DOWNLOADING != instanceId))
    {
        return LE_BAD_PARAMETER;
    }

    // Search in the download file
    result = FileInfoFromJsonFileDelete(FILESTREAM_FILE_DOWNLOAD, instanceId);
    if (LE_OK != result)
    {
        // Search in the file list
        result = FileInfoFromJsonFileDelete(FILESTREAM_FILE_LIST, instanceId);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete a file by file name
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if failed.
 *      - LE_BAD_PARAMETER if a provided parameter is incorrect.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fileStream_Delete
(
    const char* fileNamePtr      ///< [IN] File name
)
{
    char* bufferPtr = NULL;
    size_t bufferSize = 0;
    bool isInstanceFound = false;
    int i = 0;
    int length;
    json_error_t error;
    json_t* filePtr;
    json_t* root;

    if (!fileNamePtr)
    {
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != le_fs_GetSize(FILESTREAM_FILE_LIST, &bufferSize))
    {
        LE_DEBUG("Error to get file %s size", FILESTREAM_FILE_LIST);
        return LE_FAULT;
    }

    bufferPtr = calloc(bufferSize + 1, sizeof(char));

    if (LE_OK != ReadFs(FILESTREAM_FILE_LIST, (uint8_t*)bufferPtr, &bufferSize))
    {
        free(bufferPtr);
        return LE_FAULT;
    }

    /* Parse the file (JSON) */
    root = json_loads(bufferPtr, 0, &error);
    free(bufferPtr);

    if (!root)
    {
        LE_ERROR("Error to parse the file list");
        return LE_FAULT;
    }

    filePtr = json_object_get(root, JSON_FILE_FIELD_FILES);
    if (!filePtr)
    {
        LE_ERROR("Error to find files array");
        json_decref(root);
        return LE_FAULT;
    }

    if(!json_is_array(filePtr))
    {
        LE_ERROR("Files is not an array");
        json_decref(root);
        return LE_FAULT;
    }

    length = json_array_size(filePtr);
    for (i=(length-1); i>=0; i--)
    {
        json_t* fileObjectPtr = json_array_get(filePtr, i);
        json_t* jsonFieldPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_NAME);
        const char* name = (char*)json_string_value(jsonFieldPtr);

        if(!name)
        {
            LE_ERROR("File name is NULL");
        }
        else
        {
            size_t size = strlen(fileNamePtr) > strlen(name) ? strlen(fileNamePtr) : strlen(name);
            if(!strncmp(fileNamePtr, name, size))
            {
                le_result_t result;
                int instanceId = 0;
                size_t len = LE_FILESTREAMCLIENT_FILE_NAME_MAX_BYTES
                            + strlen(FILESTREAM_LEFS_DIR)
                            + strlen(FILESTREAM_STORAGE_LEFS_DIR)
                            + 10;
                char namePtr[len];

                // Delete
                isInstanceFound = true;

                snprintf(namePtr,
                         len,
                         "%s%s/%s",
                         FILESTREAM_LEFS_DIR,
                         FILESTREAM_STORAGE_LEFS_DIR,
                         fileNamePtr);
                result =  le_fs_Delete(namePtr);
                if (LE_OK != result)
                {
                    LE_DEBUG("File %s was NOT deleted: %s", name, LE_RESULT_TXT(result));
                }
                else
                {
                    LE_ERROR("File %s was deleted", name);
                }

                jsonFieldPtr = json_object_get(fileObjectPtr, JSON_FILE_FIELD_INSTANCE);
                if(jsonFieldPtr)
                {
                    instanceId = json_integer_value(jsonFieldPtr);
                    // Indicate that the instance Id is available
                    if (instanceId < LE_FILESTREAMSERVER_FILE_MAX_NUMBER)
                    {
                        IsFileInstanceUsed[instanceId] = false;
                    }
                }

                json_array_remove(filePtr, i);
            }
        }
    }

    LE_DEBUG("%s", json_dumps(root, JSON_COMPACT));

    if (!isInstanceFound)
    {
        json_decref(root);
        return LE_BAD_PARAMETER;
    }

    if (LE_OK != WriteFs(FILESTREAM_FILE_LIST, (uint8_t*)json_dumps(root, JSON_COMPACT), strlen(json_dumps(root, JSON_COMPACT))))
    {
        json_decref(root);
        return LE_FAULT;
    }

    json_decref(root);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the available space for file storage
 *
 * @return
 *      - LE_OK             The function succeeded
 *      - LE_BAD_PARAMETER  Incorrect parameter provided
 *      - LE_FAULT          The function failed
 *      - LE_NOT_FOUND      The file is not present
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fileStream_GetAvailableSpace
(
    uint64_t*   availableSpacePtr       ///< [OUT] Available space (in bytes)
)
{
    struct statvfs info;

    if (!availableSpacePtr)
    {
        return LE_BAD_PARAMETER;
    }

    statvfs("/data/le_fs", &info);
    LE_INFO("block size :%lu, free blocks for root: %ld, free blocks for user: %ld",
            info.f_bsize, info.f_bfree, info.f_bavail);
    *availableSpacePtr = info.f_bsize * info.f_bavail;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get file instance list
 *
 * @return
 *      - LE_OK             The function succeeded
 *      - LE_BAD_PARAMETER  Incorrect parameter provided
 *      - LE_FAULT          The function failed
 *      - LE_NOT_FOUND      The file is not present
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fileStream_GetFileInstanceList
(
    uint16_t*   fileInstancePtr,                ///< [OUT] File instances list
    size_t*     fileInstancedNumElementsPtr     ///< [OUT] File instances list size
)
{
    uint32_t step = 0;
    uint32_t instanceNb;

    if ((!fileInstancePtr) || (!fileInstancedNumElementsPtr))
    {
        return LE_BAD_PARAMETER;
    }

    // Only get the instance ID list from FILESTREAM_FILE_LIST file.
    // FILESTREAM_FILE_DOWNLOAD includes only one file which is downloading which instance ID
    // is DOWNLOADING_FILE_INSTANCE_ID
    ListFileInstanceFromJsonFile(FILESTREAM_FILE_LIST, fileInstancePtr, &instanceNb);
    step += instanceNb;
    *fileInstancedNumElementsPtr = (size_t)step;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get path storage
 *
 * @return
 *      - LE_OK on success.
 *      - LE_OVERFLOW in case of overflow
 *      - LE_FAULT if failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fileStream_GetPathStorage
(
    char*       pathPtr,                ///< [OUT]  Path buffer
    size_t      pathNumElements         ///< [IN]   Path buffer size (\0 included)
)
{
    LE_ASSERT(pathPtr && pathNumElements > 0);

    size_t len = strlen(ROOT_PATH_STORAGE \
                        FILESTREAM_LEFS_DIR \
                        FILESTREAM_STORAGE_LEFS_DIR);

    if (pathNumElements <= len)
    {
        return LE_OVERFLOW;
    }

    int n = snprintf(pathPtr,
                     pathNumElements,
                     "%s%s%s",
                     ROOT_PATH_STORAGE,
                     FILESTREAM_LEFS_DIR,
                     FILESTREAM_STORAGE_LEFS_DIR);

    if (n < 0)
    {
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a file name is already present
 *
 * If a file is present, its related instance Id (for LwM2M object) is returned in instanceId
 * parameter.
 * This instance ID value range is [0 - LE_FILESTREAMSERVER_INSTANCE_ID_DOWNLOAD[ for any stored
 * files.
 * If the instance ID value is LE_FILESTREAMSERVER_INSTANCE_ID_DOWNLOAD, it indicates that this
 * file is transferring.
 *
 * @return
 *      - LE_OK if the file is already present.
 *      - LE_FAULT if failed.
 *      - LE_BAD_PARAMETER if a provided parameter is incorrect.
 *      - LE_NOT_FOUND if the file is not present.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fileStream_IsFilePresent
(
    const char* fileNamePtr,    ///< [IN] File name
    const char* fileHashPtr,    ///< [IN] File hash (optional)
    uint16_t*   instanceIdPtr   ///< [OUT] Instance Id if the file is present (optional)
)
{
    le_result_t result;

    if ((!fileNamePtr) || (!instanceIdPtr))
    {
        return LE_BAD_PARAMETER;
    }

    result = FileInfoFromJsonFileCheckFileName(FILESTREAM_FILE_DOWNLOAD,
                                               fileNamePtr,
                                               fileHashPtr,
                                               instanceIdPtr);
    LE_DEBUG("Check file name %s in %s return %d (%s)",
             fileNamePtr, FILESTREAM_FILE_DOWNLOAD, result, LE_RESULT_TXT(result));
    if (LE_OK != result)
    {
        result = FileInfoFromJsonFileCheckFileName(FILESTREAM_FILE_LIST,
                                                   fileNamePtr,
                                                   fileHashPtr,
                                                   instanceIdPtr);
        LE_DEBUG("Check file name %s in %s return %d (%s)",
                 fileNamePtr, FILESTREAM_FILE_LIST, result, LE_RESULT_TXT(result));
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * File streaming thread
 */
//--------------------------------------------------------------------------------------------------
static void* FileStreamThread
(
    void* ctxPtr    ///< [IN] Context pointer
)
{
    static le_result_t result;
    size_t fileToReadLen = 0;
    char* fileNamePtr = (char*)ctxPtr;
    size_t len = LE_FILESTREAMCLIENT_FILE_NAME_MAX_BYTES
                    + strlen(FILESTREAM_LEFS_DIR)
                    + strlen(FILESTREAM_STORAGE_LEFS_DIR)
                    + 10;
    char namePtr[len];
    le_fs_FileRef_t fileRef;
    int fd = -1;
    size_t readDataLen = 0;

    #define DATA_LEN 500 // To be updated
    uint8_t data[DATA_LEN];
    size_t dataLen = DATA_LEN;

    // Initialize the return value at every start
    result = LE_OK;

    // Block SIGPIPE so we receive EPIPE
    le_sig_Block(SIGPIPE);

    // Open the pipe
    fd = le_fd_Open(LE_FILESTREAMSERVER_FIFO_PATH, O_WRONLY);
    if (-1 == fd)
    {
        LE_ERROR("Failed to open FIFO %d", errno);
        result =  LE_FAULT;
        goto errorEnd;
    }
    StaticFd = fd;
    // Open the file
    snprintf(namePtr,
             len,
             "%s%s/%s",
             FILESTREAM_LEFS_DIR,
             FILESTREAM_STORAGE_LEFS_DIR,
             fileNamePtr);

    LE_INFO("File name to read: %s", namePtr);

    result = le_fs_GetSize(namePtr, &fileToReadLen);
    if (LE_OK != result)
    {
        LE_ERROR("Failed to get the file len (%s)", LE_RESULT_TXT(result));
        goto errorCloseFd;
    }
    LE_INFO("File len %zd", fileToReadLen);

    dataLen = DATA_LEN;
    result = le_fs_Open(namePtr, LE_FS_RDONLY, &fileRef);
    if (LE_OK != result)
    {
        LE_ERROR("Failed to open the file (%s)", LE_RESULT_TXT(result));
        goto errorCloseFd;
    }

    readDataLen = 0;
    while(true)
    {
        result = le_fs_Read(fileRef, data, &dataLen);
        if (LE_OK != result)
        {
            LE_ERROR("Error to read data file %s", LE_RESULT_TXT(result));
            goto errorCloseFile;
        }
        if (!dataLen)
        {
            break;
        }
        readDataLen += dataLen;
        LE_DEBUG("readDataLen %zd - %zd fileToReadLen -  %zd", readDataLen, fileToReadLen, dataLen);

        ssize_t count = -1;
        do
        {
            count = le_fd_Write(fd, data, dataLen);
            if (-1 == count)
            {
                LE_ERROR("Failed to write to fifo: %s", strerror(errno));

                if ((-1 == count) && (EAGAIN == errno))
                {
                    count = 0;
                }
                else if ((-1 == count) && (EINTR != errno))
                {
                    LE_ERROR("Error during write: %m");
                    goto errorCloseFile;
                }
            }
            LE_DEBUG("Read from file: %d bytes", (uint32_t)count);
        }while ((-1 == count) && (EINTR == errno));


        if (dataLen > count)
        {
            LE_ERROR("Failed to write data: size %"PRIu32", count %zd", dataLen, count);
        }
    }

errorCloseFile:
    if (LE_OK != le_fs_Close(fileRef))
    {
        LE_ERROR("failed to close file %s", namePtr);
    }

errorCloseFd:
    if ((-1 != fd) && (0 != le_fd_Close(fd)))
    {
        LE_ERROR("failed to close fd");
    }
    else
    {
        LE_DEBUG("FD closed on file service");
    }

errorEnd:
    FileStreamRef = NULL;
    return (void*)&result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to request a stream
 *
 * @return
 *      - LE_OK              On success
 *      - LE_BUSY            If a stream is on-going
 *      - LE_BAD_PARAMETER   If an input parameter is not valid
 *      - LE_TIMEOUT         After 900 seconds without data received
 *      - LE_CLOSED          File descriptor has been closed before all data have been received
 *      - LE_OUT_OF_RANGE    Storage is too small
 *      - LE_NOT_FOUND       If the file is not present.
 *      - LE_FAULT           On failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fileStream_StartStream
(
    const char* fileNamePtr         ///< [IN] File name
)
{
    le_result_t result;
    static char fileToStreamPtr[LE_FILESTREAMSERVER_FILE_NAME_MAX_BYTES];

    if (FileStreamRef)
    {
        LE_ERROR("A streaming is still in progress, wait for its end");
        return LE_BUSY;
    }

    if ((!fileNamePtr) || (fileNamePtr && (!strlen(fileNamePtr))))
    {
        LE_ERROR("file name is not correct");
        return LE_BAD_PARAMETER;
    }

    result = FileInfoFromJsonFileCheckFileName(FILESTREAM_FILE_LIST,
                                               fileNamePtr,
                                               NULL,
                                               NULL);
    LE_DEBUG("Check file name %s in %s return %d (%s)",
             fileNamePtr, FILESTREAM_FILE_LIST, result, LE_RESULT_TXT(result));

    if (LE_OK != result)
    {
        LE_ERROR("Issue on file name check: %s", LE_RESULT_TXT(result));
        return result;
    }

    LE_INFO("File %s is present, stream will begin", fileNamePtr);
    // Start the File Streaming thread
    memset(fileToStreamPtr,0, sizeof(fileToStreamPtr));
    snprintf(fileToStreamPtr, sizeof(fileToStreamPtr), "%s", fileNamePtr);
    FileStreamRef = le_thread_Create("FileStreaming", FileStreamThread, (void*)fileToStreamPtr);
    le_thread_SetJoinable(FileStreamRef);
    LE_THREAD_SET_STATIC_STACK(FileStreamRef, FileStreamThreadStrStack);
    le_thread_Start(FileStreamRef);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to abort a stream
 *
 * @return
 *      - LE_OK              On success
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fileStream_AbortStream
(
    void
)
{
    le_result_t result;

    LE_DEBUG("le_fileStreamServer_AbortStream");
    if (!FileStreamRef)
    {
        LE_DEBUG("No on-going stream");
        return LE_OK;
    }

    result = le_thread_Cancel(FileStreamRef);
    if (LE_OK != result)
    {
        LE_ERROR("Can not cancel streaming file: %s", LE_RESULT_TXT(result));
    }
    else
    {
        LE_DEBUG("THREAD CANCELED");
    }

    if (-1 == le_fd_Close(StaticFd))
    {
        LE_ERROR("Error to close W FIFO %d", errno);
    }
    FileStreamRef = NULL;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Retrieve stream object by topic
 *
 * @return
 *      - stream object pointer     if successfully retrieved by the given topic
 *      - NULL                      otherwise
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED StreamObject_t* pa_fileStream_GetStreamObject
(
    const char* topic               ///< [IN] Topic name
)
{
    LE_ASSERT(topic);

    return le_hashmap_Get(StreamObjTable, topic);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add stream object by topic
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_fileStream_AddStreamObject
(
    const char* topic,                              ///< [IN] Topic name
    le_fileStreamClient_StreamFunc_t handlerPtr,    ///< [IN] Callback handler
    void* contextPtr                                ///< [IN] Context pointer
)
{
    StreamObject_t *streamObjPtr = le_mem_ForceAlloc(StreamObjPoolRef);

    memset(streamObjPtr->name, 0, sizeof(streamObjPtr->name));
    le_utf8_Copy(streamObjPtr->topic, topic, sizeof(streamObjPtr->topic), NULL);
    streamObjPtr->streamCb = handlerPtr;
    streamObjPtr->contextPtr = contextPtr;

    le_hashmap_Put(StreamObjTable, streamObjPtr->topic, streamObjPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize stream objects that have been cached.
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_fileStream_Init
(
    void
)
{
    StreamObjPoolRef = le_mem_CreatePool("Stream object pool", sizeof(StreamObject_t));

    LE_ASSERT(StreamObjPoolRef);

    CreateDefaultJsonFile(FILESTREAM_FILE_LIST);
    CreateDefaultJsonFile(FILESTREAM_FILE_DOWNLOAD);

    InitializeFileInstances();

    StreamObjTable = le_hashmap_Create("Stream object Table",
                                       MAX_STREAM_OBJECT,
                                       le_hashmap_HashString,
                                       le_hashmap_EqualsString);
    LE_ASSERT(StreamObjTable);

    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn(CFG_FILESTREAM);

    if (le_cfg_GoToFirstChild(iteratorRef) == LE_OK)
    {
        do
        {
            StreamObject_t* streamObjPtr = le_mem_ForceAlloc(StreamObjPoolRef);
            le_cfg_GetNodeName(iteratorRef, "", streamObjPtr->topic, sizeof(streamObjPtr->topic));
            le_cfg_GetString(iteratorRef,
                             "packageName",
                             streamObjPtr->name,
                             sizeof(streamObjPtr->name),
                             "");
            streamObjPtr->cached = le_cfg_GetBool(iteratorRef, "packageDownloaded", false);
            le_hashmap_Put(StreamObjTable, streamObjPtr->topic, streamObjPtr);
        }
        while (le_cfg_GoToNextSibling(iteratorRef) == LE_OK);

        le_cfg_GoToParent(iteratorRef);
    }

    le_cfg_CancelTxn(iteratorRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization function for file storage PA
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    return;
}
