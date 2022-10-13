/**
 * @file pa_fileStream.c
 *
 * Default implementation of PA functions for fileStreamServer API.
 * This default PA is only used to pass the compilation with the file transfer feature.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "fileStreamServer.h"


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
    LE_UNUSED(instanceId);
    LE_UNUSED(fileNamePtr);
    LE_UNUSED(fileNameNumElements);
    LE_UNUSED(fileTopicPtr);
    LE_UNUSED(fileTopicNumElements);
    LE_UNUSED(fileHashPtr);
    LE_UNUSED(fileHashNumElements);
    LE_UNUSED(fileSizePtr);
    LE_UNUSED(fileOriginPtr);

    LE_WARN("API not implemented");
    return LE_NOT_IMPLEMENTED;
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
    LE_WARN("API not implemented");
    return LE_NOT_IMPLEMENTED;
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

    LE_WARN("API not implemented");
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
    LE_UNUSED(positionPtr);

    LE_WARN("API not implemented");
    return LE_NOT_IMPLEMENTED;
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
    LE_UNUSED(streamMgmtObj);
    LE_UNUSED(readFd);

    LE_WARN("API not implemented");
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process download stream status events. Receive status and process it locally or pass to
 * interested applications.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_fileStream_DownloadStatus
(
    le_fileStreamClient_DownloadState_t status,     ///< [IN] Download status
    int32_t bytesLeft,                              ///< [IN] Number of bytes left to be downloaded
    int32_t progress                                ///< [IN] Download progress in %
)
{
    LE_UNUSED(status);
    LE_UNUSED(bytesLeft);
    LE_UNUSED(progress);

    LE_WARN("API not implemented");
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
    LE_UNUSED(streamObjPtr);

    LE_WARN("API not implemented");
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

    LE_WARN("API not implemented");
    return LE_NOT_IMPLEMENTED;
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

    LE_WARN("API not implemented");
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
    LE_UNUSED(availableSpacePtr);

    LE_WARN("API not implemented");
    return LE_NOT_IMPLEMENTED;
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

    LE_WARN("API not implemented");
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

    LE_WARN("API not implemented");
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

    LE_WARN("API not implemented");
    return LE_NOT_IMPLEMENTED;
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

    LE_WARN("API not implemented");
    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to abort a stream
 *
 * @return
 *      - LE_OK              On success
 *      - LE_NOT_IMPLEMENTED If feature is not supported
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fileStream_AbortStream
(
    void
)
{
    LE_WARN("API not implemented");
    return LE_NOT_IMPLEMENTED;
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
    LE_WARN("API not implemented");
}
