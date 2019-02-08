
// -------------------------------------------------------------------------------------------------
/**
 *  @file configTree.c
 *
 *  This file is the "Component Main" of the configTree daemon.  This is where all of the daemon's
 *  startup occurs.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "dynamicString.h"
#include "treeDb.h"
#include "treeUser.h"
#include "nodeIterator.h"
#include "treeIterator.h"
#include "requestQueue.h"
#include "internalConfig.h"

#if LE_CONFIG_LINUX
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
}
#endif /* end LE_CONFIG_LINUX */



// -------------------------------------------------------------------------------------------------
/**
 *  Initialize the configTree server interfaces and all of it's subsystems.
 */
// -------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_DEBUG("** Config Tree, begin init.");

    // Initilize our internal subsystems.
    dstr_Init();   // Dynamic strings.
    rq_Init();     // Request queue.
    ni_Init();     // Node iterator.
    ti_Init();     // Tree iterator.
    tu_Init();     // Tree user.
    tdb_Init();    // Tree DB.
    ic_Init();     // Internal config, this depends on other subsystems and so need to go last.

    // Register our service handlers on those services so that we can properly free up resources if
    // clients unexpectedly disconnect.
#if LE_CONFIG_LINUX
    LE_DEBUG("** Setting up service event handlers.");

    le_msg_AddServiceOpenHandler(le_cfg_GetServiceRef(), OnConfigSessionOpened, NULL);
    le_msg_AddServiceOpenHandler(le_cfgAdmin_GetServiceRef(), OnConfigSessionOpened, NULL);

    le_msg_AddServiceCloseHandler(le_cfg_GetServiceRef(), OnConfigSessionClosed, NULL);
    le_msg_AddServiceCloseHandler(le_cfgAdmin_GetServiceRef(), OnConfigAdminSessionClosed, NULL);

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
#endif /* end LE_CONFIG_LINUX */

    LE_DEBUG("The configTree service has been started.");
}
