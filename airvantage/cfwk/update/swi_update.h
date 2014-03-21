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
* @brief This API provides functionalities to control an update process in the Agent context.
*
* Agent update process is working with the AirVantage services platform.
* The basic concepts are:
*  - An update job is setup on the AirVantage services platform for your device,
*  - The Agent receives the update request,
*  - The Agent checks the update request,
*  - The Agent downloads the update package,
*  - The Agent checks the update package,
*  - The Agent dispatches the update package content to the software components (those components can be Agent internals or external applications
*    that want to process their own update or applications that are responsible for updating another pieces of software),
*  - The Agent receives the result of each software component responsible for a part of the update,
*  - The Agent sends the update result to the AirVantage services platform.
*
* Here are some details on the update package:
*  - contains a Manifest file that describes the update package
*  - several components can be updated within the same update package
*  - the order of components updated can be specified
*  - can embedded application custom parameters to make the update process more flexible.
*
* The functionalities provided by this library are available for both applications that are targeted by
* an update request and for applications that only want to monitor and control the update process.
*
* The API to deal with application/asset update request  is documented here: \ref swi_av_RegisterUpdateNotification function and related definitions in \ref swi_airvantage.h API.
*
* For more details on the update process, please read the Agent product documentation.
* <HR>
*/

#ifndef SWI_UPDATE_INCLUDE_GUARD
#define SWI_UPDATE_INCLUDE_GUARD

/* Update control */


#include "returncodes.h"


/**
* Initializes the module.
* A call to init is mandatory to enable Update library APIs.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_update_Init();

/**
* Destroys the Update library.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_update_Destroy();

/**
* This enum lists the events that will notified by the Agent
* while an update process is running.
*/
typedef enum swi_update_Event
{
  SWI_UPDATE_NEW,                  ///< A new update process is started, the update request from the AirVantage platform has been validated.
  SWI_UPDATE_DOWNLOAD_IN_PROGRESS, ///< The download of the update package is in progress, this event will be sent for each download progression notification.
  SWI_UPDATE_DOWNLOAD_OK,          ///< The download of the update package is successful.
  SWI_UPDATE_CRC_OK,               ///< The checking of the package is successful.
  SWI_UPDATE_UPDATE_IN_PROGRESS,   ///< The update is being dispatched to each component. This event will be sent for each component to be updated by the update package.
  SWI_UPDATE_FAILED,               ///< The update process has failed, the cause of the failure will be given in event details:
                                   ///< e.g. CRC failure, download failure, aborted.
                                   ///< When this event is received, no more update is in progress until the event SWI_UPDATE_NEW is received again.
  SWI_UPDATE_SUCCESSFUL,           ///< The whole update progress is successful, the new software state is saved.
                                   ///< When this event is received, no more update is in progress until the event SWI_UPDATE_NEW is received again.
  SWI_UPDATE_PAUSED,               ///< The update is paused. Note that there is no specific resume event,
                                   ///< on resume, the emitted event will be the one related to the step starting after the resume.
}swi_update_Event_t;


/**
* This struct contains the update process notification
*
*
* <table>
* <tr>
*   <th>Event</th>
*   <th>Event Details</th>
* </tr>
* <tr>
*   <td>SWI_UPDATE_NEW</td>
*   <td>N/A</td>
* </tr>
* <tr>
*   <td>SWI_UPDATE_DOWNLOAD_IN_PROGRESS </td>
*   <td>Download percentage (e.g.: "15%")</td>
* </tr>
* <tr>
*   <td>SWI_UPDATE_DOWNLOAD_OK </td>
*   <td>N/A</td>
* </tr>
* <tr>
*   <td>SWI_UPDATE_CRC_OK </td>
*   <td>N/A</td>
* </tr>
* <tr>
*   <td>SWI_UPDATE_UPDATE_IN_PROGRESS Cell</td>
*   <td>Name of the current component being updated (e.g.: "someasset.foo.bar") </td>
* </tr>
* <tr>
*   <td>SWI_UPDATE_FAILED </td>
*   <td>Failure details (e.g. "CRC failed") </td>
* </tr>
* <tr>
*   <td>SWI_UPDATE_SUCCESSFUL </td>
*   <td>N/A</td>
* </tr>
* <tr>
*   <td>SWI_UPDATE_PAUSED </td>
*   <td>Current update step that will be resumed when SWI_UPDATE_REQ_RESUME is received</td>
* </tr>
* </table>
*
*/
typedef struct swi_update_Notification
{
  swi_update_Event_t event; ///< The event being notified. See #swi_update_Event_t for list of event.
  const char* eventDetails; ///< This field will be used to indicate the download progress, failure details etc.
}swi_update_Notification_t;


/**
*  Callback to receive to receive update process notification.
*
*  @param indPtr [IN] Update process event notification. The indication data will be released when the callback returns
*
* @return RC_OK on success, other value will be interpreted has error and will trigger callback unregister.
*/
typedef rc_ReturnCode_t (*swi_update_StatusNotifictionCB_t)
(
    swi_update_Notification_t* indPtr //<
);



/**
* Registers to receive notification about update process.
*
* The callback will be called in a new pthread.
*
* @return RC_OK on success
*/
rc_ReturnCode_t swi_update_RegisterStatusNotification
(
    swi_update_StatusNotifictionCB_t cb ///< [IN] callback to register to receive software update, previous value is replaced.
                                         ///<      NULL value for the callback is interpreted as unregister request.
);



/**
* This enum is used to send requests to the Agent in order to change the update process.
*/
typedef enum swi_update_Request
{
  SWI_UPDATE_REQ_PAUSE,  ///< Request to pause the current update.
  SWI_UPDATE_REQ_RESUME, ///< Request to resume the current update.
  SWI_UPDATE_REQ_ABORT   ///< Request to abort the current update. (The update will be set as failed).
}swi_update_Request_t;


/**
* Requests to act on update process.
*
* If called within a swi_update_StatusNotifictionCB_t, then a Pause/Abort request will be processed right after the swi_update_StatusNotifictionCB_t
* has returned. Otherwise, a Pause/Abort request will be processed when the next update interruptible step will start.
*
* @return RC_OK on success
* @return RC_SERVICE_UNAVAILABLE if no update was in progress
* @return RC_UNSPECIFIED_ERROR if the action can not be performed, e.g. trying to resume an update that is already running.
*/
rc_ReturnCode_t swi_update_Request
(
    swi_update_Request_t req ///< [IN] the request to send to the Agent in order to change the update process.
);


#endif /* SWI_UPDATE_INCLUDE_GUARD */


