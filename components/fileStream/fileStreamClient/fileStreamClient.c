/**
 * @file fileStreamClient.c
 *
 * Implementation of File Stream Client API
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#include "legato.h"
#include "interfaces.h"
#include "le_print.h"
#include "watchdogChain.h"
#include "fileStreamClient.h"
#include "fileStreamServer.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * Stream clients
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char appName[LIMIT_MAX_APP_NAME_BYTES];
    char topic[LE_FILESTREAMCLIENT_FILE_TOPIC_MAX_BYTES];
}
StreamClient_t;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for file stream clients.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t StreamClientPool;

//--------------------------------------------------------------------------------------------------
/**
 * Map containing safe refs of stream clients.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t StreamClientMap;

//--------------------------------------------------------------------------------------------------
/**
 * Boolean for 1st file info read
 */
//--------------------------------------------------------------------------------------------------
static bool IsInfoRead = false;

//--------------------------------------------------------------------------------------------------
/**
 * Single instance stream management object managed by File Stream Service.
 */
//--------------------------------------------------------------------------------------------------
static le_fileStreamClient_StreamMgmt_t StreamMgmtObject = {
                     "defaultPkgName",                          ///< Name
                     "defaultPkgTopic",                         ///< Class
                     LE_FILESTREAMCLIENT_DIRECTION_DOWNLOAD,    ///< Direction
                     LE_FILESTREAMCLIENT_ORIGIN_SERVER,         ///< Origin
                     "",                                        ///< Hash
                     0,                                         ///< Current download offset
                     0,                                         ///< Size of entire package
                     0,                                         ///< Status of current stream
                     0,                                         ///< Result of current stream
                     UINT16_MAX,                                ///< Instance Id for object 33407
                     0                                          ///< File transfer progress
                   };

//==================================================================================================
//                                       Local function
//==================================================================================================

//==================================================================================================
//                         Functions exposed to file stream server
//==================================================================================================


//--------------------------------------------------------------------------------------------------
/**
 * Write the stream management object
 *
 * @return:
 *      - LE_OK on success
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t fileStreamClient_SetStreamMgmtObject
(
    le_fileStreamClient_StreamMgmt_t* streamMgmtObjPtr      ///< [IN] Stream management object
)
{
    memcpy((le_fileStreamClient_StreamMgmt_t*)&StreamMgmtObject,
            (le_fileStreamClient_StreamMgmt_t*)streamMgmtObjPtr,
            sizeof(le_fileStreamClient_StreamMgmt_t));

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Read the stream management object
 *
 * If a file is present, its related instance Id (for LwM2M object) is returned in instanceId
 * parameter.
 * This instance ID value range is [0 - LE_FILESTREAMSERVER_INSTANCE_ID_DOWNLOAD[ for any stored
 * files.
 * If the instance ID value is LE_FILESTREAMSERVER_INSTANCE_ID_DOWNLOAD, it indicates that this
 * file is transferring
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER  Incorrect parameter provided
 *      - LE_FAULT          The function failed
 *      - LE_NOT_FOUND      The file is not present
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t fileStreamClient_GetStreamMgmtObject
(
    uint16_t                            instanceId,         ///< [IN] Intance Id of the file
    le_fileStreamClient_StreamMgmt_t*   streamMgmtObjPtr    ///< [OUT] Stream management object
)
{
    le_result_t result = LE_OK;
    if ((StreamMgmtObject.instanceId != instanceId) || (false == IsInfoRead))
    {
        result = fileStreamServer_GetFileInfoByInstance(instanceId,
                                                        StreamMgmtObject.pkgName,
                                                        LE_FILESTREAMCLIENT_FILE_NAME_MAX_BYTES,
                                                        StreamMgmtObject.pkgTopic,
                                                        LE_FILESTREAMCLIENT_FILE_TOPIC_MAX_BYTES,
                                                        StreamMgmtObject.hash,
                                                        LE_FILESTREAMCLIENT_HASH_MAX_BYTES,
                                                        &(StreamMgmtObject.pkgSize),
                                                        (uint8_t*)&(StreamMgmtObject.origin));

        IsInfoRead = true;
        StreamMgmtObject.instanceId = instanceId;
    }
    memcpy((le_fileStreamClient_StreamMgmt_t*)streamMgmtObjPtr,
           (le_fileStreamClient_StreamMgmt_t*)&StreamMgmtObject,
           sizeof(le_fileStreamClient_StreamMgmt_t));

    return result;
}


//==================================================================================================
//                                       Public API Functions
//==================================================================================================

//--------------------------------------------------------------------------------------------------
/**
 * Read the stream management object
 *
 * @return
 *      - LE_OK on success.
  *     - LE_BAD_PARAMETER  Incorrect parameter provided
 *      - LE_FAULT          The function failed
 *      - LE_NOT_FOUND      The file is not present
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fileStreamClient_GetStreamMgmtObject
(
    uint16_t                            instanceId,         ///< [IN] Intance Id of the file
    le_fileStreamClient_StreamMgmt_t*   streamMgmtObjPtr    ///< [OUT] Stream management object
)
{
    return fileStreamClient_GetStreamMgmtObject(instanceId, streamMgmtObjPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the stream management object
 *
 * @return
 *      - LE_OK on success.
 *      - LE_FAULT if failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fileStreamClient_SetStreamMgmtObject
(
    const le_fileStreamClient_StreamMgmt_t* streamMgmtObjPtr    ///< [IN] Stream management object
)
{
    return fileStreamClient_SetStreamMgmtObject((le_fileStreamClient_StreamMgmt_t*)streamMgmtObjPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function removes the registed handler
 */
//--------------------------------------------------------------------------------------------------
void le_fileStreamClient_RemoveStreamEventHandler
(
    le_fileStreamClient_StreamEventHandlerRef_t handlerRef   ///< [IN] handler reference
)
{
    StreamClient_t* streamClient = le_ref_Lookup(StreamClientMap, handlerRef);

    if (streamClient == NULL)
    {
        LE_ERROR("Invalid reference (%p) provided.", handlerRef);
        return;
    }

    le_ref_DeleteRef(StreamClientMap, handlerRef);
    le_mem_Release(streamClient);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler to receive an fd to a cached stream or incoming stream on a selected topic.
 */
//--------------------------------------------------------------------------------------------------
le_fileStreamClient_StreamEventHandlerRef_t le_fileStreamClient_AddStreamEventHandler
(
    const char* topicPtr,                           ///< [IN] Subscribing to topic type
    le_fileStreamClient_StreamFunc_t handlerPtr,    ///< [IN] This callback will receive status
                                                    ///< events
    void* contextPtr                                ///< [IN] Context
)
{
    void* handlerRef = NULL;

    // Get the client's credentials.
    pid_t pid;
    uid_t uid;

    if (le_msg_GetClientUserCreds(le_fileStreamClient_GetClientSessionRef(), &uid, &pid) != LE_OK)
    {
        LE_CRIT("Could not get credentials for the client.");
        return handlerRef;
    }

    StreamClient_t* streamClient = le_mem_ForceAlloc(StreamClientPool);

    // Look up the process's application name.
    le_result_t result = le_appInfo_GetName(pid, streamClient->appName, LIMIT_MAX_APP_NAME_BYTES);

    if (result != LE_OK)
    {
        return handlerRef;
    }
    else
    {
        le_utf8_Copy(streamClient->topic, topicPtr, LE_FILESTREAMCLIENT_FILE_TOPIC_MAX_BYTES, NULL);
        handlerRef = le_ref_CreateRef(StreamClientMap, streamClient);
    }

    fileStreamServer_StoreDownloadHandler(topicPtr, handlerPtr, contextPtr);

    return handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_fileStreamClient_UploadState'
 */
//--------------------------------------------------------------------------------------------------
void le_fileStreamClient_RemoveUploadStateHandler
(
    le_fileStreamClient_UploadStateHandlerRef_t handlerRef
        ///< [IN]
)
{
    // ToDo:
    LE_ERROR("TO BE IMPLEMENTED");
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_fileStreamClient_UploadState'
 *
 * This event provides information on upload state changes
 */
//--------------------------------------------------------------------------------------------------
le_fileStreamClient_UploadStateHandlerRef_t le_fileStreamClient_AddUploadStateHandler
(
    const char* LE_NONNULL topic,
        ///< [IN] Subscribing to topic type
    int fd,
        ///< [IN] Client has to provide the read end of the pipe
    le_fileStreamClient_UploadFunc_t handlerPtr,
        ///< [IN] This callback will receive status events
    void* contextPtr
        ///< [IN]
)
{
    // ToDo:
    LE_ERROR("TO BE IMPLEMENTED");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure that the file stream/download is completed
 *
 * This API is supposed to be called by client side to tell server that client has completed its
 * stream/download task.
 */
//--------------------------------------------------------------------------------------------------
void le_fileStreamClient_StreamComplete
(
    void
)
{
    fileStreamServer_SetClientDownloadComplete();
}

//--------------------------------------------------------------------------------------------------
/**
 * Configure the resume position for following stream
 */
//--------------------------------------------------------------------------------------------------
void le_fileStreamClient_SetResumePosition
(
    size_t position                 ///< Stream resume position
)
{
    fileStreamServer_SetResumePosition(position);
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialization function for File Stream Service
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Starting File Stream Client Service");

    StreamClientPool = le_mem_CreatePool("Stream client pool", sizeof(StreamClient_t));
    StreamClientMap = le_ref_CreateMap("Stream client map", 20);
}
