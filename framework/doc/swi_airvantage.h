/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *     Romain Perier      for Sierra Wireless - initial API and implementation
 *******************************************************************************/


/**
 * @file
 * @brief This API enables interaction with Sierra Wireless AirVantage Services Platform.
 *
 * Using this API, an application can work with Sierra Wireless AirVantage Services Platform to:
 *  - request connection to AirVantage server
 *  - send data
 *  - receive data
 *  - receive asset update request
 *
 *  This module relies on Agent process, which is responsible for queuing data,
 *  managing the flush timers and sending the data to the remote AirVantage server.
 *  Many of the APIs in this module relay the data to the Agent; the Agent then manages the data as described.
 *
 *  Two methods are supported for sending data to the AirVantage servers:
 *   - The swi_av_asset_Push* functions: this is a simple API for managing how to send data, this is the recommended method for most use cases.
 *   - Tables API (via swi_av_table_Create): this allows for more advanced control of the transfer of data.
 *
 * <HR>
 */

#ifndef SWI_AIRVANTAGE_INCLUDE_GUARD
#define SWI_AIRVANTAGE_INCLUDE_GUARD

#include <stdlib.h>
#include "returncodes.h"
#include "swi_dset.h"

/**
* @name General functionalities.
* @{
*/


/**
* Defines whether the library displays error traces (on stdout) to help debugging errors.
* Using 0 for this setting will ensure no printing is done by the library.
*
*/
#define SWI_AV_ERROR 1
#define SWI_AV_INFO 1

/**
* Initializes the AirVantage library.
* A call to swi_av_Init is mandatory to enable AirVantage library APIs.
*
* @return RC_OK on success
* @return RC_NOT_AVAILABLE if the Agent cannot be accessed.
*/
rc_ReturnCode_t swi_av_Init();

/**
* Destroys the AirVantage library.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_av_Destroy();


#define SWI_AV_CX_SYNC UINT_MAX ///< Value to be used to request synchronous connection to server using #swi_av_ConnectToServer

/**
* Forces a  connection to the server.
*  This connection will not flush outgoing data handled through policies,
*  but it will poll the server for new messages addressed to assets on this gateway device.
*
*  If using #SWI_AV_CX_SYNC, the connection is synchronous, i.e. once this function returns, the requested connection to the server is closed.
*  Otherwise the connection will happen after this call returns.
*
*  Notes:
*  - valid values for latency are 0 to INT_MAX.
*  - 0 value means the connection will be asynchronous, but will be done as soon as possible.
*
*  @return RC_OK on success
*  @return RC_NOT_AVAILABLE if the Agent cannot be accessed.
*/
rc_ReturnCode_t swi_av_ConnectToServer
(
    unsigned int latency ///< [IN] latency in seconds before initiating the connection to the server,
                         ///<  use SWI_AV_CX_SYNC to specify synchronous connection.
);



/**
* Forces data attached to a given policy to be sent or consolidated immediately.
* This only applies to data sent using simple or advanced Data Sending APIs.
* Data Reception and Asset Update exchanges are @b not modified by this function.
*
* A connection to the server is done only if data needs to be sent as the result
* to this trigger operation. Put another way, if no data is attached to the
* triggered policy(ies), then no connection to the server is done.
* See #swi_av_ConnectToServer for complementary function.
*
* For a description of how policies allow to manage data reporting from the assets to the server,
* see Agent product documentation.
*
* @return RC_OK on success
* @return RC_BAD_PARAMETER if the requested policy name is not found.
* @return RC_NOT_AVAILABLE if the Agent cannot be accessed.
*/
rc_ReturnCode_t swi_av_TriggerPolicy
(
    const char* policyPtr ///< [IN] name of the policy queue to be flushed. Flush all policies if policy=='*';
                          ///<      only flush the default policy if policy is omitted.
);


/**
*   An Asset is the AirVantage Application Services object used to send data to the AirVantage Application Services server.
*   Instances of this object must be created through #swi_av_asset_Create.
*
*/
typedef struct swi_av_Asset swi_av_Asset_t;


/**
* Creates and returns a new asset instance.
* The newly created asset is not started when returned,
* it can therefore neither send nor receive messages at this point.
* This intermediate, non-started asset allows to configure message/update handlers
* before any message/update is actually transferred to the asset.
*
* See #swi_av_asset_Start to start the newly created instance.
*
* @return RC_OK on success
* @return RC_NOT_AVAILABLE if the Agent cannot be accessed.
* @return RC_BAD_PARAMETER if the supplied parameters are invalid
*
*/
rc_ReturnCode_t swi_av_asset_Create
(
    swi_av_Asset_t** asset, ///< [OUT] pointer to hold the newly created asset.
                            ///<       The AirVantage library is responsible for allocating the resources of this asset.
                            ///<       The user is responsible of releasing resources using #swi_av_asset_Destroy.
    const char* assetIdPtr  ///< [IN] string defining the assetId identifying the instance of this new asset.
                            ///<      NULL and empty string values are forbidden.
                            ///<      The assetId must be unique on the same device, otherwise asset starting will fail.
);






/**
* Starts a newly created asset.
* Allows the asset instance to send and receive messages to/from the servers.
*
* @return RC_OK on success
* @return RC_NOT_AVAILABLE if the Agent cannot be accessed.
*/
rc_ReturnCode_t swi_av_asset_Start
(
    swi_av_Asset_t* asset ///< [IN] the asset to start
);


/**
* Closes an asset instance, releasing the associated resources.
* Once this destructor method has been called, no more message can be sent
* nor received by the instance and update for this asset will be automatically rejected.
*
* @return RC_OK on success
* @return RC_BAD_FORMAT if asset parameter is invalid
*/
rc_ReturnCode_t swi_av_asset_Destroy
(
    swi_av_Asset_t* asset ///< [IN] the asset to stop
);


// end General
///@}
/**
* @name Data Sending Simple API
* @{
*/



/**
* Specific values for timestamps to be used with swi_av_asset_Push* functions.
* (Those values are not meant to be used with advanced swi_av_table_Push* functions).
* Timestamps values 0 and 1 (in second since Unix Epoch) are reserved
* for those special timestamp requests.
*/
typedef enum swi_av_timestamp{
  SWI_AV_TSTAMP_NO = 0,   ///< Explicitly request no timestamp to send alongside the data
  SWI_AV_TSTAMP_AUTO = 1, ///< Timestamp will be automatically generated when the data is added.
}swi_av_timestamp_t;

/**
* Pushes a string value to the agent.
* The data are not necessarily moved forward from the agent to the server immediately:
* agent-to-server data transfers are managed through policies, as described in the Agent product documentation.
* This API is optimized for ease of use: it will internally try to reformat data in the most sensible,
* server-compatible way. Applications requiring a tight control over how data are structured, buffered,
* consolidated and reported should consider the more advanced Table API,
* especially it is not possible to send correlated data using this API.
*
* String parameters can be released by user once the call has returned.
*
* @return RC_OK on success
* @return RC_NOT_AVAILABLE if the Agent cannot be accessed.
*/
rc_ReturnCode_t swi_av_asset_PushString
(
    swi_av_Asset_t* asset, ///< [IN] the asset used to send the data
    const char *pathPtr,   ///< [IN] the datastore path under which data will be stored relative to the asset node,
                           ///<      the last path segment will be used as a datastore key.
                           ///<      NULL and empty string values are forbidden.
    const char* policyPtr, ///< [IN] optional name of the policy controlling when the data must be sent to the server.
                           ///<      If omitted, the default policy is used.
    uint32_t timestamp,    ///< [IN] optional timestamp, in second since Unix Epoch,  #swi_av_timestamp_t values can be used
                           ///<      to request automatic or no timestamp.
    const char* valuePtr   ///< [IN] string value to push
);

/**
* Pushes an integer value to the agent.
* The data are not necessarily moved forward from the agent to the server immediately:
* agent-to-server data transfers are managed through policies, as described in the Agent product documentation.
* This API is optimized for ease of use: it will internally try to reformat data in the most sensible,
* server-compatible way. Applications requiring a tight control over how data are structured, buffered,
* consolidated and reported should consider the more advanced Table API,
* especially it is not possible to send correlated data using this API.
*
* String parameters can be released by user once the call has returned.
*
* @return RC_OK on success
* @return RC_NOT_AVAILABLE if the Agent cannot be accessed.
*/
rc_ReturnCode_t swi_av_asset_PushInteger
(
    swi_av_Asset_t* asset, ///< [IN] the asset used to send the data
    const char *pathPtr,   ///< [IN] the datastore path under which data will be stored relative to the asset node,
                           ///<      the last path segment will be used as a datastore key.
    const char* policyPtr, ///< [IN] optional, name of the policy controlling when the data must be sent to the server.
                           ///<      If omitted, the default policy is used.
    uint32_t timestamp,    ///< [IN] optional timestamp, in second since Unix Epoch, #swi_av_timestamp_t values can be used
                           ///<      to request automatic or no timestamp.
    int64_t value          ///< [IN] integer value to push
);

/**
* Pushes a float value to the agent.
* The data are not necessarily moved forward from the agent to the server immediately:
* agent-to-server data transfers are managed through policies, as described in the Agent product documentation.
* This API is optimized for ease of use: it will internally try to reformat data in the most sensible,
* server-compatible way. Applications requiring a tight control over how data are structured, buffered,
* consolidated and reported should consider the more advanced Table API,
* especially it is not possible to send correlated data using this API.
*
* String parameters can be released by user once the call has returned.
*
* @return RC_OK on success
* @return RC_NOT_AVAILABLE if the Agent cannot be accessed.
*/
rc_ReturnCode_t swi_av_asset_PushFloat
(
    swi_av_Asset_t* asset, ///< [IN] the asset used to send the data
    const char *pathPtr,   ///< [IN] the datastore path under which data will be stored relative to the asset node,
                           ///<      the last path segment will be used as a datastore key
    const char* policyPtr, ///< [IN] optional name of the policy controlling when the data must be sent to the server.
                           ///<      If omitted, the default policy is used.
    uint32_t timestamp,    ///< [IN] optional timestamp, in second since Unix Epoch, #swi_av_timestamp_t values can be used
                           ///<      to request automatic or no timestamp.
    double value           ///< [IN] float value to push
);

// end Data Sending Simple API
///@}
/**
* @name Data Sending Advanced API
* @{
*/


/**
*   A Table  is the Airvantage objects handling staging database tables, to buffer, consolidate
*   and send structured data.
*   Instances of this object must be created through #swi_av_table_Create.
*/
typedef struct swi_av_Table swi_av_Table_t;

/**
* Specific values for storage.
*/
typedef enum
{
  STORAGE_RAM = 0, ///< Non persistent, everything is saved only in RAM
  STORAGE_FLASH    ///< Persistent, everything is saved to the FLASH memory
} swi_av_Table_Storage_t;

/**
* Creates a table, the AirVantage objects handling staging database tables, to buffer, consolidate
* and send structured data.
*
* String parameters can be released by user once the call has returned.
*
* @return RC_OK on success
* @return RC_NOT_AVAILABLE if the Agent cannot be accessed.
* @return RC_BAD_FORMAT if error occurred during the payload generation (internally used to exchange data with the Agent)
*/
rc_ReturnCode_t swi_av_table_Create
(
    swi_av_Asset_t* asset,      ///< [IN] the asset used to send the data
    swi_av_Table_t** table,     ///< [OUT] pointer to hold the newly created table.
                                ///<       The AirVantage library is responsible for allocating the resources of this table,
                                ///<       The user is responsible of releasing the resources using swi_av_Table_Destroy.
    const char* pathPtr,        ///< [IN] the datastore path under which data will be stored relative to the asset node
    size_t numColumns,          ///< [IN] number of columns of the table
    const char** columnNamesPtr,///< [IN] pointer to an array of strings (with numColums entries): name of each column.
    const char* policyPtr,      ///< [IN] name of the policy controlling when the data must be sent to the server.
    swi_av_Table_Storage_t persisted, ///< [IN] value which describes how the table must be persisted, STORAGE_FLASH meaning file persistence,
                                ///<      STORAGE_RAM meaning in ram only.
    int purge                   ///< [IN] boolean value, indicates if existing table (if any) is recreated (true) or reused (false).
                                ///<      Recreation means the table will be dropped and then created from scratch
                                ///<      (so any data inside table will be lost).
);


/**
* Destroys table instance, releasing associated resources.
* Partial data not pushed yet to the agent will be lost.
*
* @return RC_OK on success
* @return RC_NOT_AVAILABLE if the Agent cannot be accessed.
* @return RC_BAD_FORMAT if error occurred during the payload generation (internally used to exchange data with the Agent)
*/
rc_ReturnCode_t swi_av_table_Destroy
(
    swi_av_Table_t* table ///< [IN] the table to destroy
);


/**
* Pushes a float value in the current row of the table.
*
* swi_av_table_Push* functions have be called in the correct order to match the table definition created in swi_av_table_Create.
* Until a row is complete and sent to the Agent by invoking swi_av_table_PushRow(), data is only pushed locally in the table database.
*
* @return RC_OK on success
* @return RC_OUT_OF_RANGE maximum len for the current row has been reached, the value cannot be pushed
*/
rc_ReturnCode_t swi_av_table_PushFloat
(
    swi_av_Table_t* table, ///< [IN] the table where to push the value
    double value           ///< [IN] the float to push
);



/**
* Pushes an integer value in the current row of the table.
*
* swi_av_table_Push* functions have be called in the correct order to match the table definition created in swi_av_table_Create.
* Until a row is complete and sent to the Agent by invoking swi_av_table_PushRow(), data is only pushed locally in the table database.
*
* @return RC_OK on success
* @return RC_OUT_OF_RANGE maximum len for the current row has been reached, the value cannot be pushed
*/
rc_ReturnCode_t swi_av_table_PushInteger
(
    swi_av_Table_t* table, ///< [IN] the table where to push the value
    int value              ///< [IN] the integer to push
);

/**
* Pushes a string value in the current row of the table.
*
* swi_av_table_Push* functions have be called in the correct order to match the table definition created in swi_av_table_Create.
* Until a row is complete and sent to the Agent by invoking swi_av_table_PushRow(), data is only pushed locally in the table database.
*
* @return RC_OK on success
* @return RC_OUT_OF_RANGE maximum len for the current row has been reached, the value cannot be pushed
*/
rc_ReturnCode_t swi_av_table_PushString
(
    swi_av_Table_t* table, ///< [IN] the table where to push the value
    const char* value      ///< [IN] the string to push
);


/**
* Pushes the current row of the database to the Agent.
*
* Once the current row has been pushed to the Agent, it is totally freed in the database,
* And the table is ready to received new pushed data using swi_av_table_Push*() functions.
*
* @return RC_OK on success
* @return RC_NOT_AVAILABLE if the Agent cannot be accessed.
* @return RC_BAD_FORMAT if error occurred during the payload generation (internally used to exchange data with the Agent)
*/
rc_ReturnCode_t swi_av_table_PushRow
(
    swi_av_Table_t* table ///< [IN] the table where to push the value
);



// end Data Sending Advanced API
///@}

/**
* @name Data Reception
* @{
*/


/**
* DataWrite callback to receive data coming from the server.
*
* No automatic acknowledge of received data will be done, so application that wants the server
* to receive acknowledge needs to call #swi_av_Acknowledge function.
*
* String parameters given to this function will be released when the callback returns.
*
* @param asset [IN] the asset receiving the data
* @param pathPtr [IN] the path targeted by the data sent by the server.
* @param data [IN] the data iterator containing the received data.
*             The data contained in the iterator will be automatically released when the callback returns.
* @param ack_id [IN] the id to be used to acknowledge the received data.
*               If ack_id=0 then there is no need to acknowledge.
* @param userDataPtr [IN] the user data given at callback registration.
*/
typedef void (*swi_av_DataWriteCB)
(
    swi_av_Asset_t *asset,        ///<
    const char *pathPtr,          ///<
    swi_dset_Iterator_t* data,    ///<
                                  ///<
    int ack_id,                   ///<
                                  ///<
    void *userDataPtr             ///<
);



/**
* Registers a callback to receive DataWrite notifications.
* The callback will be called in a new pthread.
*
* Usage example of the datawrite callback function:
* @code
*
* int my_datacallback(swi_av_Asset *asset, const char *path, swi_dset_Iterator* data, int ack_id, void *userData){
*   if(! strncmp(path, "command.setvalue", strlen("command.setvalue")) ){
*    int64_t cmd_value;
*    assert(RC_OK == swi_dset_GetIntegerByName(data, "cmd_value", &cmd_value) );
*    setvalue(cmd_value);
*   }
*   else{ //unknown command
*      printf("received data on path[%s]:\n", path);
*      while(RC_NOT_FOUND != swi_dset_Next(data) ){
*        printf("data name: [%s]\n", swi_dset_GetName(data));
*        //...
*      }
*   }
*
*   if(ack_id)
*    swi_av_Acknowledge(ack_id, 0, NULL, "now", 1);
*
*   return 0;
* }
*
* main(...){
*
*  swi_av_RegisterDataWrite(asset, my_datacallback, NULL)
*
* }
* @endcode
*
* @return RC_OK on success
* @return RC_BAD_FORMAT if provided asset param is invalid
*/
rc_ReturnCode_t swi_av_RegisterDataWrite
(
    swi_av_Asset_t *asset, ///< [IN] the asset listening to incoming data.
    swi_av_DataWriteCB cb, ///< [IN] the callback function to register to receive the data.
    void * userDataPtr     ///< [IN] user data that will be given back in callback.
);


/**
* Acknowledges a server message received with an acknowledgment ticket id.
* No automatic acknowledge will be done, so application that wants the server
* to receive acknowledge needs to call this function.
*
* @return RC_OK on success
* @return RC_BAD_PARAMETER if the requested policy name is not found.
*/
rc_ReturnCode_t swi_av_Acknowledge
(
    int ackId,             ///< [IN] the id to acknowledge, as given to the data reception callback.
    int status,            ///< [IN] status of the acknowledge: 0 means success, other values mean error
    const char* errMsgPtr, ///< [IN] an optional error message string
    const char* policyPtr, ///< [IN] optional triggering policy to send the acknowledgment, defaults to "now".
    int persisted          ///< [IN] boolean, if true, the ACK message will be persisted in flash by the agent,
                           ///<      and kept even if a reboot occurs before the policy is triggered.
);




// end Data Reception
/// @}

/**
* @name Asset Update
* Using those APIs and types, the application can receive update request, coming with update files from update package sent
* by AirVantage Services platform.
* @{
*/

/**
* asset update notification callback.
*
* The callback will be called when the associated asset is responsible to manage a software update request coming from
* AirVantage services platform.
*
* The application must call #swi_av_SendUpdateResult explicitly to send update result, otherwise no result is sent, retry mechanism will be
* started and eventually the update status will be set to failed.
*
* String parameters will be released when the callback returns.
*
* @param asset               [IN] the asset receiving the update notification
* @param componentNamePtr    [IN] is the identifier of the component to update, (the component name is a path in dotted notation)
*                                 the name is defined in update package manifest file, here it is provided without the assetid at the beginning.
* @param versionPtr          [IN] is the version of the component to install. Version can be empty string (but not NULL!)
*                                 to specify de-installation request, non empty string for regular update/install of software component.
* @param updateFilePathPtr   [IN] absolute path to local file to use on the device to do the update, can be empty string when version is empty too.
*                                 The file will be automatically deleted when the update process ends, so once #swi_av_SendUpdateResult has been called,
*                                 or all retries have been done for a single component update, the file existence on file system is not guaranteed anymore.
* @param customParams        [IN] application specifics parameters, defined in update package, can be NULL if no custom parameter was defined.
*                                 To be processed using #swi_dset_Iterator_t API, embedded data in the iterator will be automatically released when this callback returns.
* @param userDataPtr         [IN] the user data given at callback registration
*
*
*
*
* @return RC_OK when the callback ran correctly (it doesn't necessarily means the update was successful, see #swi_av_SendUpdateResult),
*                       any other return value will be interpreted as error.
*
*/
typedef rc_ReturnCode_t (*swi_av_updateNotificationCB)
(
    swi_av_Asset_t *asset,               ///<
    const char* componentNamePtr,        ///<
                                         ///<
    const char* versionPtr,              ///<
                                         ///<
    const char *updateFilePathPtr,       ///<
                                         ///<
                                         ///
    swi_dset_Iterator_t* customParams,   ///<
                                         ///<
    void *userDataPtr                    ///<
);


/**
* Registers the hook function to be called when the asset receives a software
* update request from the AirVantage services platform.
*
* The callback will be called in a new pthread.
*
* This feature targets applications that want to process their own update or applications that are responsible for updating another pieces of software,
* taking advantage of the integrated solution provided by AirVantage services.
*
* If the application wants to have a deeper control of the whole update process, it needs to use the functionalities provided by \ref swi_update.h.
*
* - There can be only one pending software update request at a time.
* - Only one hook can be registered for the whole asset
* - If no user update hook is set, the error code 472 (meaning "not supported /
*   not implemented") will be reported to the server.
* - Any error coming from this update request means that the whole update
*   process will be considered as failed.
* - When an update request tries to install a version that is already
*   installed, the application should return success value.
*   Indeed, in some cases the asset instance won't receive and report the hook's
*   result (e.g. because of a poorly timed reboot). As a result, the update
*   request will be sent again, and the hook should report a success immediately.
*
*
* @return RC_OK on success
* @return RC_BAD_FORMAT if provided asset param is invalid
*/
rc_ReturnCode_t swi_av_RegisterUpdateNotification
(
    swi_av_Asset_t* asset,           ///< [IN] the asset listening to update notification, can be a started or non-started asset.
    swi_av_updateNotificationCB cb,  ///< [IN] the callback function to register to receive asset update notification
                                     ///< Giving NULL as parameter will be treated as unregister of previous callback.
    void *userDataPtr                ///< [IN] user data that will be given back in callback
);


/**
* Sends the result of the software update request previously received by an asset.
*
* @return RC_OK on success
* @return RC_BAD_PARAMETER when no update request is matching asset and component name, the update result is discarded.
*/
rc_ReturnCode_t swi_av_SendUpdateResult
(
    swi_av_Asset_t* asset,        ///< [IN] the asset that was targeted by the software update request.
    const char* componentNamePtr, ///< [IN] this must be the same value as the one that was given as argument to the swi_av_updateNotificationCB.
                                  ///<      As only one software update is possible for the same component at the same time, the couple asset+componentName
                                  ///<      fully identifies the software update request.
    int updateResult              ///< [IN] the result of the update, 200 for success, any other value means error.
                                  ///<      Values from 480 to 499 are reserved for applicative error codes,so it is highly recommended
                                  ///<      to use one (or more) of those to signify an error coming from an asset update.
);


// end Asset Update
/// @}

#endif /* SWI_AIRVANTAGE_INCLUDE_GUARD */
