/**
 * @file fileStreamClient.h
 *
 * Functions used to interface to client side apps
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_FILE_STREAM_CLIENT_INCLUDE_GUARD
#define LEGATO_FILE_STREAM_CLIENT_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Get the stream management object
 *
 * If a file is present, its related instance Id (for LwM2M object) is returned in instanceId
 * parameter.
 * This instance ID value range is [0 - LE_FILESTREAMSERVER_INSTANCE_ID_DOWNLOAD[ for any stored
 * files.
 * If the instance ID value is LE_FILESTREAMSERVER_INSTANCE_ID_DOWNLOAD, it indicates that this
 * file is transferring
 *
 * @return:
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
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the stream management object
 *
 * @return:
 *      - LE_OK on success
 *      - LE_FAULT on any other error
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t fileStreamClient_SetStreamMgmtObject
(
    le_fileStreamClient_StreamMgmt_t* streamMgmtObjPtr
);

#endif         //LEGATO_FILE_STREAM_CLIENT_INCLUDE_GUARD
