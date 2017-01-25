//--------------------------------------------------------------------------------------------------
/**
 * Unit test 3 for the Low-Level Messaging APIs.
 *
 *  - Server and Client in different processes,
 *  - Client does synchronous IPC to server.
 *  - Server kills client.
 *  - Both client and server have session close handlers registered, which catch the close event
 *    and exit with EXIT_SUCCESS.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"

COMPONENT_INIT
{
    LE_TEST_INIT;

    LE_INFO("======= Test 3: Server and Client in different processes, server kills client. ========");

    system("testFwMessaging-Setup");

    le_test_ChildRef_t client = LE_TEST_FORK("testFwMessaging-Test3-client");
    le_test_ChildRef_t server = LE_TEST_FORK("testFwMessaging-Test3-server");

    LE_TEST_JOIN(client);
    LE_TEST_JOIN(server);

    LE_TEST_EXIT;
}
