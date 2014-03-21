//--------------------------------------------------------------------------------------------------
/**
 * Client for unit test for the Low-Level Messaging APIs.
 *
 * Copyright (C) 2014, Sierra Wireless, Inc. All rights reserved. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"


COMPONENT_INIT
{
    le_msg_ProtocolRef_t protocolRef;
    le_msg_SessionRef_t sessionRef;
    le_msg_MessageRef_t msgRef;

    LE_INFO("======= Test 3: Server and Client in different processes, server kills client. ========");

    // Open a session with the server.
    protocolRef = le_msg_GetProtocolRef("testFwMessaging3", 0);
    sessionRef = le_msg_CreateSession(protocolRef, "messagingTest3");
    le_msg_OpenSessionSync(sessionRef);

    // Do a synchronous request=response transaction.
    msgRef = le_msg_CreateMsg(sessionRef);
    msgRef = le_msg_RequestSyncResponse(msgRef);

    LE_TEST(msgRef == NULL);
}
