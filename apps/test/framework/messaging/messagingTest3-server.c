//--------------------------------------------------------------------------------------------------
/**
 * Server for unit test for the Low-Level Messaging APIs.
 *
 * Copyright (C) 2014, Sierra Wireless, Inc. All rights reserved. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"


static void MessageReceiveHandler
(
    le_msg_MessageRef_t msgRef,
    void* ignored
)
{
    LE_KILL_CLIENT("Die, client, die!");

    le_msg_Respond(msgRef);
}


COMPONENT_INIT
{
    le_msg_ProtocolRef_t protocolRef;
    le_msg_ServiceRef_t serviceRef;

    LE_INFO("======= Test 3: Server and Client in different processes, server kills client. ========");

    // Create and advertise the service.
    protocolRef = le_msg_GetProtocolRef("testFwMessaging3", 0);
    serviceRef = le_msg_CreateService(protocolRef, "messagingTest3");
    le_msg_SetServiceRecvHandler(serviceRef, MessageReceiveHandler, NULL);
    le_msg_AdvertiseService(serviceRef);
}
