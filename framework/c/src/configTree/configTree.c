
// -------------------------------------------------------------------------------------------------
/**
 *  @file configTree.c
 *
 *  This file is the "Component Main" of the configTree daemon.  This is where all of the daemon's
 *  startup occurs.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved.
 *  Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "stringBuffer.h"
#include "dynamicString.h"
#include "treeDb.h"
#include "treeUser.h"
#include "nodeIterator.h"
#include "treeIterator.h"
#include "requestQueue.h"
#include "internalConfig.h"




// -------------------------------------------------------------------------------------------------
/**
 *  Used to keep track of service handler functions and their corrisponding context pointers.
 */
// -------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionEventHandler_t handlerFunc;  ///< The event handler function to call.  May be NULL
                                               ///<   if a function was not specified.
    void* contextPtr;                          ///< A context pointer used by the function.
}
ServiceHandlerInfo_t;




/// The original handler of the configApi open session event.
static ServiceHandlerInfo_t CfgOriginalOpenedHandler = { NULL, NULL };

/// The original handler of the configAdminApi open session event.
static ServiceHandlerInfo_t CfgAdminOriginalOpenedHandler = { NULL, NULL };

/// The original handler of the configApi close session event.
static ServiceHandlerInfo_t CfgOriginalSessionClosedHandler = { NULL, NULL };

/// The original handler of the configAdminApi close session event.
static ServiceHandlerInfo_t CfgAdminOriginalSessionClosedHandler = { NULL, NULL };




// -------------------------------------------------------------------------------------------------
/**
 *  Handle the calling of an event handler.  If the handler pointer is NULL then nothing will
 *  happen.
 */
// -------------------------------------------------------------------------------------------------
static void FireSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] Reference to the session that the event occured on.
    ServiceHandlerInfo_t* handler    ///< [IN] The handler function to fire in response to this
                                     ///<      event.
)
// -------------------------------------------------------------------------------------------------
{
    if (handler->handlerFunc != NULL)
    {
        handler->handlerFunc(sessionRef, handler->contextPtr);
    }
}




// -------------------------------------------------------------------------------------------------
/**
 *  Called when a config API, or a configAdmin session is opened.  This function will call into the
 *  user subsystem to allow it to keep track of the active users of a system.
 */
// -------------------------------------------------------------------------------------------------
static void OnConfigSessionOpened
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] Reference to the session that's going away.
    void* contextPtr                 ///< [IN] Our callback context.  Configured as NULL.
)
// -------------------------------------------------------------------------------------------------
{
    tu_SessionConnected(sessionRef);

    FireSessionEventHandler(sessionRef, &CfgOriginalOpenedHandler);
}




// -------------------------------------------------------------------------------------------------
/**
 *  This function is called when users close their connection to the configuration tree.  This
 *  function will make sure that any iterators left open by that client are properly cleaned up.
 */
// -------------------------------------------------------------------------------------------------
static void OnConfigSessionClosed
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] Reference to the session that's going away.
    void* contextPtr                 ///< [IN] Our callback context.  Configured as NULL.
)
// -------------------------------------------------------------------------------------------------
{
    rq_CleanUpForSession(sessionRef);
    tdb_CleanUpHandlers(sessionRef);
    tu_SessionDisconnected(sessionRef);

    FireSessionEventHandler(sessionRef, &CfgOriginalSessionClosedHandler);
}




// -------------------------------------------------------------------------------------------------
/**
 *  When clients from the admin API disconnect from the service this function is called.  This will
 *  then take care of releasing any resources allocated for that connection.
 */
// -------------------------------------------------------------------------------------------------
static void OnConfigAdminSessionClosed
(
    le_msg_SessionRef_t sessionRef,  ///< [IN] Reference to the session that's going away.
    void* contextPtr                 ///< [IN] Our callback context.  Configured as NULL.
)
// -------------------------------------------------------------------------------------------------
{
    ti_CleanUpForSession(sessionRef);
    tu_SessionDisconnected(sessionRef);

    FireSessionEventHandler(sessionRef, &CfgAdminOriginalSessionClosedHandler);
}




// -------------------------------------------------------------------------------------------------
/**
 *  Initialize the configTree server interfaces and all of it's subsystems.
 */
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_DEBUG("** Config Tree, begin init.");

    // Initilize our internal subsystems.
    sb_Init();     // String buffers.
    dstr_Init();   // Dynamic strings.
    rq_Init();     // Request queue.
    ni_Init();     // Node iterator.
    ti_Init();     // Tree iterator.
    tu_Init();     // Tree user.
    tdb_Init();    // Tree DB.
    ic_Init();     // Internal config, this depends on other subsystems and so need to go last.

    // Register our service handlers on those services so that we can properly free up resources if
    // clients unexpectedly disconnect.  Also make sure to grab any pre-existing handlers so that
    // they can still be called.
    LE_DEBUG("** Setting up service event handlers.");

    le_msg_GetServiceOpenHandler(le_cfg_GetServiceRef(),
                                 &CfgOriginalOpenedHandler.handlerFunc,
                                 &CfgOriginalOpenedHandler.contextPtr);
    le_msg_GetServiceOpenHandler(le_cfgAdmin_GetServiceRef(),
                                 &CfgAdminOriginalOpenedHandler.handlerFunc,
                                 &CfgAdminOriginalOpenedHandler.contextPtr);

    le_msg_GetServiceCloseHandler(le_cfg_GetServiceRef(),
                                  &CfgOriginalSessionClosedHandler.handlerFunc,
                                  &CfgOriginalSessionClosedHandler.contextPtr);
    le_msg_GetServiceCloseHandler(le_cfgAdmin_GetServiceRef(),
                                  &CfgAdminOriginalSessionClosedHandler.handlerFunc,
                                  &CfgAdminOriginalSessionClosedHandler.contextPtr);

    le_msg_SetServiceOpenHandler(le_cfg_GetServiceRef(), OnConfigSessionOpened, NULL);
    le_msg_SetServiceOpenHandler(le_cfgAdmin_GetServiceRef(), OnConfigSessionOpened, NULL);

    le_msg_SetServiceCloseHandler(le_cfg_GetServiceRef(), OnConfigSessionClosed, NULL);
    le_msg_SetServiceCloseHandler(le_cfgAdmin_GetServiceRef(), OnConfigAdminSessionClosed, NULL);

    // Because this is a system process, we need to close our standard in.  This way the supervisor
    // is properly informed we have completed our startup sequence.  Standard in is reopened on
    // /dev/null so that the file descriptor isn't accidently reused for some other file.
    LE_DEBUG("** Notifing the supervisor the configuration tree is ready.");

    FILE* filePtr;

    do
    {
        filePtr = freopen("/dev/null", "r", stdin);
    }
    while ((filePtr == NULL) && (errno == EINTR));

    LE_FATAL_IF(filePtr == NULL, "Failed to redirect standard in to /dev/null.  %m.");
    LE_DEBUG("The configTree service has been started.");
}
