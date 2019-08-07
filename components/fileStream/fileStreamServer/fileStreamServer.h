/**
 * @file fileStreamServer.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_FILE_STREAM_SERVER_INCLUDE_GUARD
#define LEGATO_FILE_STREAM_SERVER_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Define value for instacne ID for the downloading file
 */
//--------------------------------------------------------------------------------------------------
#define FILE_INSTANCE_ID_DOWNLOADING                UINT16_MAX

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
);

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
LE_SHARED le_result_t fileStreamServer_GetFileInfoByInstance
(
    uint16_t    instanceId,                 ///< [IN]   File instance
    char*       fileNamePtr,                ///< [OUT]  File name buffer
    size_t      fileNameNumElements,        ///< [IN]   File name buffer size
    char*       fileTopicPtr,               ///< [OUT]  File topic buffer
    size_t      fileTopicNumElements,       ///< [IN]   File topic buffer size
    char*       fileHashPtr,                ///< [OUT]  File hash buffer
    size_t      fileHashNumElements,        ///< [IN]   File hash buffer size
    uint64_t*   sizePtr,                    ///< [OUT]  File size
    uint8_t*    originPtr                   ///< [OUT]  File origin
);

//--------------------------------------------------------------------------------------------------
/**
 * Configure that the file stream/download is completed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void fileStreamServer_SetClientDownloadComplete
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Configure the resume position for following stream
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void fileStreamServer_SetResumePosition
(
    size_t position                 ///< [IN] position or offset of a file
);

#endif //LEGATO_FILE_STREAM_SERVER_INCLUDE_GUARD
