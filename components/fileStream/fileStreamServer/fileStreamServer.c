/**
 * @file fileStreamServer.c
 *
 * Implementation of File Stream Server API
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#include "legato.h"
#include "interfaces.h"
#include "fileStreamClient.h"
#include "fileStreamServer.h"
#include "watchdogChain.h"
#include "pa_fileStream.h"


//--------------------------------------------------------------------------------------------------
/**
 * Flag indicating if download is active
 */
//--------------------------------------------------------------------------------------------------
static volatile bool IsBusy = false;


//--------------------------------------------------------------------------------------------------
/**
 * Configure that the file stream/download is completed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void fileStreamServer_SetClientDownloadComplete
(
    void
)
{
    IsBusy = false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the resume position for following stream
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void fileStreamServer_SetResumePosition
(
    size_t position                 ///< [IN] position or offset of a file
)
{
    pa_fileStream_SetResumePosition(position);
}

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
le_result_t fileStreamServer_GetFileInfoByInstance
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
    return pa_fileStream_GetFileInfoByInstance(instanceId,
                                               fileNamePtr,
                                               fileNameNumElements,
                                               fileTopicPtr,
                                               fileTopicNumElements,
                                               fileHashPtr,
                                               fileHashNumElements,
                                               fileSizePtr,
                                               fileOriginPtr);
}

//==================================================================================================
//                                       Public API Functions
//==================================================================================================

//--------------------------------------------------------------------------------------------------
/**
 * Init Stream
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fileStreamServer_InitStream
(
    void
)
{
    return pa_fileStream_InitStream();
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
le_result_t le_fileStreamServer_GetResumePosition
(
    size_t* positionPtr                             ///< [OUT] Stream resume position
)
{
    return pa_fileStream_GetResumePosition(positionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Find if a stream is currently in progress
 *
 * @return
 *      - 0 if not busy
 *      - 1 if busy
 */
//--------------------------------------------------------------------------------------------------
bool le_fileStreamServer_IsBusy
(
    void
)
{
    LE_DEBUG("Process stream is %s", IsBusy? "Busy": "Idle");
    return IsBusy;
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
le_result_t le_fileStreamServer_Download
(
    int readFd                      ///< [IN] File discriptor for reading file content
)
{
    le_fileStreamClient_StreamMgmt_t streamMgmtObj;
    uint16_t instanceId = FILE_INSTANCE_ID_DOWNLOADING;

    // Get the stream management object
    le_result_t res = fileStreamClient_GetStreamMgmtObject(instanceId,
                            (le_fileStreamClient_StreamMgmt_t*)&streamMgmtObj);

    if (LE_OK != res)
    {
        LE_ERROR("Cannot get stream management object");
        return LE_FAULT;
    }

    LE_DEBUG("Download topic '%s' ", streamMgmtObj.pkgTopic);

    // Caching file only if cached direction is specified
    if (streamMgmtObj.direction == LE_FILESTREAMCLIENT_DIRECTION_DOWNLOAD)
    {
        if (LE_OK != pa_fileStream_Download(&streamMgmtObj, readFd))
        {
            return LE_UNAVAILABLE;
        }

        IsBusy = true;
    }
    else
    {
        // Send readFd to client application if a handler has been registered for this topic
        // Client application manages what to do with the stream content
        StreamObject_t* streamObjPtr = pa_fileStream_GetStreamObject(streamMgmtObj.pkgTopic);

        if (streamObjPtr == NULL)
        {
            LE_ERROR("No application registered on topic: %s", streamMgmtObj.pkgTopic);
            return LE_UNAVAILABLE;
        }
        else
        {
            if (streamObjPtr->streamCb == NULL)
            {
                LE_ERROR("No application registered on topic: %s", streamMgmtObj.pkgTopic);
                return LE_UNAVAILABLE;
            }
            else
            {
                // TODO: SMACK permission related to readfd
                LE_INFO("Passing read fd of download to the client application.");
                streamObjPtr->streamCb(readFd, streamObjPtr->contextPtr);
            }
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Process download stream status events. Receive status and process it locally or pass to
 * interested applications.
 */
//--------------------------------------------------------------------------------------------------
void le_fileStreamServer_DownloadStatus
(
    le_fileStreamClient_DownloadState_t status,     ///< [IN] Download status
    int32_t bytesLeft,                              ///< [IN] Number of bytes left to be downloaded
    int32_t progress                                ///< [IN] Download progress in %
)
{
    pa_fileStream_DownloadStatus(status, bytesLeft, progress);
}


//--------------------------------------------------------------------------------------------------
/**
 * Start Upload
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fileStreamServer_Upload
(
    int fd                            ///< [IN] Read end of the client's pipe
)
{
    LE_ERROR("Upload: TO BE IMPLEMENTED");
    return LE_NOT_IMPLEMENTED;
}


//--------------------------------------------------------------------------------------------------
/**
 * Store client handlers registered on a specific topic. If client registers on a topic that is
 * already cached successfully, provide client with the fd to the cache file.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void fileStreamServer_StoreDownloadHandler
(
    const char* topicPtr,                            ///< [IN] Topic
    le_fileStreamClient_StreamFunc_t handlerPtr,     ///< [IN] Stream handler
    void* contextPtr                                 ///< [IN] Context pointer
)
{
    StreamObject_t* streamObjPtr = pa_fileStream_GetStreamObject(topicPtr);

    if (streamObjPtr != NULL)
    {
        streamObjPtr->streamCb = handlerPtr;
        streamObjPtr->contextPtr = contextPtr;

        pa_fileStream_ProcessCacheClient(streamObjPtr);
    }
    else
    {
        // If stream object does not exist, topic has not been operated on.
        pa_fileStream_AddStreamObject(topicPtr, handlerPtr, contextPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete a file
 *
 * @return
 *      - LE_OK on success.
 *      - LE_BAD_PARAMETER if instance does no exist
 *      - LE_FAULT if failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fileStreamServer_DeleteFileByInstance
(
    uint16_t    instanceId          ///< [IN] File instance
)
{
    return pa_fileStream_DeleteFileByInstance(instanceId);
}

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
le_result_t le_fileStreamServer_GetFileInfoByInstance
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
    if ((LE_FILESTREAMSERVER_FILE_MAX_NUMBER < instanceId)
     && (FILE_INSTANCE_ID_DOWNLOADING != instanceId))
    {
        return LE_BAD_PARAMETER;
    }

    return fileStreamServer_GetFileInfoByInstance(instanceId,
                                                  fileNamePtr,
                                                  fileNameNumElements,
                                                  fileTopicPtr,
                                                  fileTopicNumElements,
                                                  fileHashPtr,
                                                  fileHashNumElements,
                                                  fileSizePtr,
                                                  fileOriginPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete a file
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if failed.
 *      - LE_BAD_PARAMETER if a provided parameter is incorrect.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fileStreamServer_Delete
(
    const char* fileNamePtr      ///< [IN] File name
)
{
    return pa_fileStream_Delete(fileNamePtr);
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
le_result_t le_fileStreamServer_GetAvailableSpace
(
    uint64_t*   availableSpacePtr       ///< [OUT] Available space (in bytes)
)
{
    return pa_fileStream_GetAvailableSpace(availableSpacePtr);
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
le_result_t le_fileStreamServer_GetFileInstanceList
(
    uint16_t*   fileInstancePtr,                ///< [OUT] File instances list
    size_t*     fileInstancedNumElementsPtr     ///< [OUT] File instances list size
)
{
    return pa_fileStream_GetFileInstanceList(fileInstancePtr, fileInstancedNumElementsPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get path storage
 *
 * @return
 *      - LE_OK on success.
 *      - LE_BAD_PARAMETER if a provided parameter is incorrect.
 *      - LE_OVERFLOW in case of overflow
 *      - LE_FAULT if failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fileStreamServer_GetPathStorage
(
    char*       pathPtr,                ///< [OUT]  Path buffer
    size_t      pathNumElements         ///< [IN]   Path buffer size (\0 included)
)
{
    if (!pathPtr)
    {
        return LE_BAD_PARAMETER;
    }

    return pa_fileStream_GetPathStorage(pathPtr, pathNumElements);
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
le_result_t le_fileStreamServer_IsFilePresent
(
    const char* fileNamePtr,    ///< [IN] File name
    const char* fileHashPtr,    ///< [IN] File hash (optional)
    uint16_t*   instanceIdPtr   ///< [OUT] Instance Id if the file is present
)
{
    return pa_fileStream_IsFilePresent(fileNamePtr, fileHashPtr, instanceIdPtr);
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
le_result_t le_fileStreamServer_StartStream
(
    const char* fileNamePtr         ///< [IN] File name
)
{
    return pa_fileStream_StartStream(fileNamePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to abort a stream
 *
 * @return
 *      - LE_OK              On success
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fileStreamServer_AbortStream
(
    void
)
{
    return pa_fileStream_AbortStream();
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization function for File Stream Service
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Starting File Stream Service");

    pa_fileStream_Init();
}
