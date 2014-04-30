
// -------------------------------------------------------------------------------------------------
/**
 *  @file configTree.c
 *
 *  This file is the "Component Main" of the configTree daemon.  This is where all of the daemon's
 *  startup occurs.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013, 2014. All rights reserved. Use of this work is subject
 *  to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "stringBuffer.h"
#include "dynamicString.h"
#include "treeDb.h"
#include "treeUser.h"
#include "nodeIterator.h"
#include "requestQueue.h"




// -------------------------------------------------------------------------------------------------
/**
 *  This function is called when users close their connection to the configuration tree.  This
 *  function will make sure that any iterators left open by that client are properly cleaned up.
 */
// -------------------------------------------------------------------------------------------------
static void OnConfigSessionClosed
(
    le_msg_SessionRef_t sessionRef,  ///< Reference to the session that's going away.
    void* contextPtr                 ///< Our callback context.  Configured as NULL.
)
// -------------------------------------------------------------------------------------------------
{
    rq_CleanUpForSession(sessionRef);
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
    tu_Init();     // Tree user.
    tdb_Init();    // Tree DB.

    // Now that we're ready to handle requests, advertise the services we'll be providing.
    LE_DEBUG("** Opening config services.");

    le_cfg_StartServer("configTree");
    le_cfgAdmin_StartServer("configTreeAdmin");

    // Register our close handlers on those services so that we can properly free up resources if
    // clients unexpectedly disconnect.
    LE_DEBUG("** Setting up service close handlers.");

    le_msg_SetServiceCloseHandler(le_cfg_GetServiceRef(), OnConfigSessionClosed, NULL);

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
