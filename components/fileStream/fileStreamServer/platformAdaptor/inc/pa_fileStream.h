/**
 * @file pa_fileStream.h
 *
 * Header of default PA functions for fileStreamServer API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef _FILE_TRANSFER_PA_FILESTREAM_H_INCLUDE_GUARD_
#define _FILE_TRANSFER_PA_FILESTREAM_H_INCLUDE_GUARD_

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Structure containing streams
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char name[LE_FILESTREAMCLIENT_FILE_NAME_MAX_BYTES];
    char topic[LE_FILESTREAMCLIENT_FILE_TOPIC_MAX_BYTES];
    bool cached;
    le_fileStreamClient_StreamFunc_t streamCb;
    void* contextPtr;
}
StreamObject_t;


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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * Check if a file name is already present
 *
 * If a file is present, its related instance Id (for LwM2M object) is returned in instanceId
 * parameter.
 * This instance ID value range is [0 - LE_FILESTREAMSERVER_INSTANCE_ID_DOWNLOAD[ for any stored
 * files.
 * If the instance ID value is LE_FILESTREAMSERVER_INSTANCE_ID_DOWNLOAD, it indicates that this
 * file is transferring
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
);

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
);

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
);

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
);

#endif // _FILE_TRANSFER_PA_FILESTREAM_H_INCLUDE_GUARD_
