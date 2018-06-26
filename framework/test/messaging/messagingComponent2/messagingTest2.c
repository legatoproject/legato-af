//--------------------------------------------------------------------------------------------------
/**
 * Automated unit test for the Low-Level Messaging APIs.
 *
 * Test 2:
 * - Create a server thread and two client threads in the same process.
 * - Use synchronous request-response.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "../burgerProtocol.h"
#include "../burgerServer.h"


#define SERVICE_INSTANCE_NAME "BoeufMort2"


#define MAX_REQUEST_RESPONSE_TXNS 32


// ==================================
//  SERVER
// ==================================


//--------------------------------------------------------------------------------------------------
/**
 * Main function for the server thread.
 **/
//--------------------------------------------------------------------------------------------------
static void* ServerThreadMain
(
    void* opaqueContextPtr  ///< not used
)
//--------------------------------------------------------------------------------------------------
{
    burgerServer_Start(SERVICE_INSTANCE_NAME, MAX_REQUEST_RESPONSE_TXNS);

    le_event_RunLoop();
}


//--------------------------------------------------------------------------------------------------
/**
 * Start the server thread.
 **/
//--------------------------------------------------------------------------------------------------
static __attribute__((unused)) void StartServer
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_thread_Start(le_thread_Create("MsgTest2Server", ServerThreadMain, NULL));
}


// ==================================
//  CLIENT
// ==================================

static int ResponseCount = 0; // Count of the number of responses received from the server.

static const char ClientIndContextStr[] = "This is the client receiving an indication message.";
static const char ClientOpenContextStr[] = "This is the client opening a session.";


static void SendSomeStuffToServer(le_msg_SessionRef_t  sessionRef);


//--------------------------------------------------------------------------------------------------
/**
 * Process a response message from the server.
 **/
//--------------------------------------------------------------------------------------------------
static void ProcessResponse
(
    le_msg_MessageRef_t  msgRef,
    le_msg_SessionRef_t  sessionRef  // Reference to the session.
)
{
    ResponseCount++;

    // Process response message from the server.
    burger_Message_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
    LE_INFO("Response %"PRIx32" (%d/%d) received from server.",
            msgPtr->payload,
            ResponseCount,
            MAX_REQUEST_RESPONSE_TXNS);
    LE_TEST(msgPtr->payload == 0xBEEFDEAD);

    // Get the session reference before releasing the message and check it.
    LE_TEST(le_msg_GetSession(msgRef) == sessionRef);

    // Release the response message, now that we are finished with it.
    le_msg_ReleaseMsg(msgRef);
}


// This function will be called whenever the server sends us an indication message (as opposed to
// a response message).
static void IndicationRecvHandler
(
    le_msg_MessageRef_t  msgRef,    // Reference to the received message.
    void*                contextPtr // contextPtr passed into le_msg_SetSessionRecvHandler().
)
{
    LE_TEST(contextPtr == ClientIndContextStr);
    LE_TEST(strcmp(contextPtr, ClientIndContextStr) == 0);

    // Process notification message from the server.
    burger_Message_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
    LE_TEST_INFO("Indication message %"PRIx32" received from server.", msgPtr->payload);

    le_msg_SessionRef_t sessionRef = le_msg_GetSession(msgRef);

    switch(msgPtr->payload)
    {
        case 0xBEEFBEEF:
            if (ResponseCount < MAX_REQUEST_RESPONSE_TXNS)
            {
                // Send a request to the server and wait for a synchronous response.
                msgRef = le_msg_CreateMsg(sessionRef);
                msgPtr = le_msg_GetPayloadPtr(msgRef);
                msgPtr->payload = 0xDEADBEEF;
                msgRef = le_msg_RequestSyncResponse(msgRef);
                if (msgRef == NULL)
                {
                    // Transaction failed.  No response received.
                    // This might happen if the server deleted the request without sending a
                    // response, or if we had registered a "Session End Handler" and the session
                    // terminated before the response was sent.
                    LE_TEST_FATAL("Transaction failed!");
                }
                ProcessResponse(msgRef, sessionRef);
                // Queue the sending of more stuff.
                le_event_QueueFunction((le_event_DeferredFunc_t)SendSomeStuffToServer,
                                       sessionRef,
                                       NULL);
            }

            break;
        case 0xDEADDEAD:
            LE_TEST(msgPtr->payload == 0xDEADDEAD);

            // Release the message, now that we are finished with it.
            le_msg_ReleaseMsg(msgRef);

            // This is now the end of the test.  Check that we received all the responses that we
            // expected.
            LE_TEST_OK(ResponseCount == MAX_REQUEST_RESPONSE_TXNS,
                       "ResponseCount(%d) == MAX_REQUEST_RESPONSE_TXNS(%d)",
                       ResponseCount, MAX_REQUEST_RESPONSE_TXNS);

            LE_TEST_EXIT;
            break;
        default:
            LE_TEST_FATAL("Unexpected response from server");
    }

}


// Send some stuff to the server.
static void SendSomeStuffToServer
(
    le_msg_SessionRef_t  sessionRef  // Reference to the session.
)
{
    le_msg_MessageRef_t msgRef;
    burger_Message_t* msgPtr;

    // Send a non-request message to the server.
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgPtr->payload = 0xBEEFBEEF;
    le_msg_Send(msgRef);
}


// This function will be called when the client-server session opens.
static void SessionOpenHandlerFunc
(
    le_msg_SessionRef_t  sessionRef, // Reference to the session that opened.
    void*                contextPtr  // contextPtr passed into le_msg_OpenSession().
)
{
    LE_TEST(contextPtr == ClientOpenContextStr);
    LE_TEST(strcmp(contextPtr, ClientOpenContextStr) == 0);

    SendSomeStuffToServer(sessionRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Start the client.
 **/
//--------------------------------------------------------------------------------------------------
static void StartClient
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_SessionRef_t sessionRef;

    // Open a session.
#if defined(TEST_UNIX_SOCKET)
    le_msg_ProtocolRef_t protocolRef;
    protocolRef = le_msg_GetProtocolRef(BURGER_PROTOCOL_ID_STR, sizeof(burger_Message_t));
    sessionRef = le_msg_CreateSession(protocolRef, SERVICE_INSTANCE_NAME);
#elif defined(TEST_LOCAL)
    sessionRef = le_msg_CreateLocalSession(&BurgerService);
#endif
    LE_TEST_INFO("Created session %p", sessionRef);
    le_msg_SetSessionRecvHandler(sessionRef, IndicationRecvHandler, (void*)ClientIndContextStr);
    LE_TEST_INFO("Set session recv handler");
    le_msg_OpenSession(sessionRef, SessionOpenHandlerFunc, (void*)ClientOpenContextStr);
    LE_TEST_INFO("Session opened");
}


// Component initialization function.
COMPONENT_INIT
{
    LE_TEST_PLAN(LE_TEST_NO_PLAN);
    LE_TEST_INFO("Server and Client in same process but different threads - Sync");

    burgerServer_Init(SERVICE_INSTANCE_NAME);

    StartServer();

    StartClient();
}
