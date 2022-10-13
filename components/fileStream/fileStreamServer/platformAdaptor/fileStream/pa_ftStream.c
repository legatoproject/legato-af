/**
 * @file pa_ftStream.c
 *
 * implementation of PA functions for fileStreamServer API with streaming file to client.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_fileStream.h"


//--------------------------------------------------------------------------------------------------
/**
 * File transfer default file information
 */
//--------------------------------------------------------------------------------------------------
#define FILE_TRANSFER_DEFAULT_TOPIC     "octave"
#define FILE_TRANSFER_DEFAULT_FILE_NAME "unknown"
#define FILE_TRANSFER_DEFAULT_HASH      ""
#define FILE_TRANSFER_DEFAULT_FILE_SIZE -1

//--------------------------------------------------------------------------------------------------
/**
 * Resume position
 */
//--------------------------------------------------------------------------------------------------
static size_t ResumePosition = 0;


//--------------------------------------------------------------------------------------------------
/**
 * Stream object
 * The current stream mode supports only one file streaming at a time, thus this one stream object
 * is sufficient to handle the process.
 */
//--------------------------------------------------------------------------------------------------
static StreamObject_t StreamObj =
{
    .name = "",
    .topic = "",
    .cached = false,
    .streamCb = NULL,
    .contextPtr = NULL
};


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
    LE_INFO("File instance Id = %d", instanceId);
    if (fileNamePtr)
    {
        snprintf(fileNamePtr, fileNameNumElements, "%s", FILE_TRANSFER_DEFAULT_FILE_NAME);
    }

    if (fileTopicPtr)
    {
        snprintf(fileTopicPtr, fileTopicNumElements, "%s", FILE_TRANSFER_DEFAULT_TOPIC);
    }

    if (fileHashPtr)
    {
        snprintf(fileHashPtr, fileHashNumElements, "%s", FILE_TRANSFER_DEFAULT_HASH);
    }

    if (fileSizePtr)
    {
        *fileSizePtr = FILE_TRANSFER_DEFAULT_FILE_SIZE;
    }

    if (fileOriginPtr)
    {
        *fileOriginPtr = LE_FILESTREAMCLIENT_ORIGIN_SERVER;
    }

    LE_WARN("Stream mode always returns fixed file instance info");
    return LE_OK;
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
    // Nothing to be initialized
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
    ResumePosition = position;
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
    size_t* positionPtr             ///< [OUT] Stream resume position
)
{
    LE_ASSERT(positionPtr);

    *positionPtr = ResumePosition;
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
    LE_INFO("This platform is treating download as stream from topic: %s", streamMgmtObj->pkgTopic);
    StreamObject_t *streamObjPtr = pa_fileStream_GetStreamObject(streamMgmtObj->pkgTopic);
    if (streamObjPtr == NULL)
    {
        LE_ERROR("No application registered for file streaming");
        return LE_UNAVAILABLE;
    }
    else
    {
        if (streamObjPtr->streamCb == NULL)
        {
            LE_ERROR("No callback registered on topic");
            return LE_UNAVAILABLE;
        }
        else
        {
            // Process received stream
            LE_INFO("Passing read fd of download to the client application.");
            streamObjPtr->streamCb(readFd, streamObjPtr->contextPtr);
        }
    }

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
    LE_INFO("Download status: %d, bytes left: %d, progress: %d", status, bytesLeft, progress);
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
    // Cached case is not applicable for streaming
    LE_UNUSED(streamObjPtr);

    return;
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
    LE_UNUSED(instanceId);

    LE_WARN("Stream mode always reports delete file success");
    return LE_OK;
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
    LE_UNUSED(fileNamePtr);

    LE_WARN("Stream mode doesn't support deleting a file");
    return LE_NOT_IMPLEMENTED;
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
    *availableSpacePtr = __INT64_MAX__;
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
    LE_UNUSED(fileInstancePtr);
    LE_UNUSED(fileInstancedNumElementsPtr);

    LE_WARN("Not support of this function in stream mode");
    return LE_NOT_IMPLEMENTED;
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
    LE_UNUSED(pathPtr);
    LE_UNUSED(pathNumElements);

    LE_WARN("Not support of this function in stream mode");
    return LE_NOT_IMPLEMENTED;
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
    uint16_t*   instanceIdPtr   ///< [OUT] Instance Id if the file is present
)
{
    LE_UNUSED(fileNamePtr);
    LE_UNUSED(fileHashPtr);
    LE_UNUSED(instanceIdPtr);

    LE_WARN("Stream mode always reports file not present");
    return LE_NOT_FOUND;
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
    LE_UNUSED(fileNamePtr);

    LE_WARN("Stream mode does not use this function");
    return LE_NOT_FOUND;
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
    LE_WARN("Stream mode does not use this function");
    return LE_NOT_FOUND;
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

    if (strncmp(StreamObj.topic, topic, LE_FILESTREAMCLIENT_FILE_TOPIC_MAX_BYTES) == 0)
    {
        return &StreamObj;
    }

    LE_ERROR("Couldn't find stream object has name: %s", topic);
    return NULL;
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
    LE_INFO("Adding new stream object with topic: %s", topic);

    memset(StreamObj.name, 0, sizeof(StreamObj.name));
    le_utf8_Copy(StreamObj.topic, topic, sizeof(StreamObj.topic), NULL);
    StreamObj.cached = false;
    StreamObj.streamCb = handlerPtr;
    StreamObj.contextPtr = contextPtr;
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
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization function for file stream PA
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    return;
}
