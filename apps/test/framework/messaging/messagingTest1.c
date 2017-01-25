//--------------------------------------------------------------------------------------------------
/**
 * Automated unit test for the Low-Level Messaging APIs.
 *
 * Test 1:
 *  - Create a thread that serves up a named service and then acts as its own client.
 *  - Tests creating and advertising services and opening services.
 *  - Also tests for conflicts with server and client being in the same process.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "burgerProtocol.h"
#include "burgerServer.h"


#define SERVICE_INSTANCE_NAME "BoeufMort1"


#define MAX_REQUEST_RESPONSE_TXNS 32


// ==================================
//  CLIENT
// ==================================

static int ClientResponseCount = 0; // Count of the number of responses received from the server.

static const char ClientIndContextStr[] = "This is the client receiving an indication message.";
static const char ClientRespContextStr[] = "This is the client receiving a response message.";
static const char ClientOpenContextStr[] = "This is the client opening a session.";


static void SendSomeStuffToServer(le_msg_SessionRef_t  sessionRef);


// This function will be called whenever the server sends us an indication message (as opposed to
// a response message).
static void ClientIndicationRecvHandler
(
    le_msg_MessageRef_t  msgRef,    // Reference to the received message.
    void*                contextPtr // contextPtr passed into le_msg_SetSessionRecvHandler().
)
{
    LE_TEST(contextPtr == ClientIndContextStr);
    LE_TEST(strcmp(contextPtr, ClientIndContextStr) == 0);

    // Process notification message from the server.
    burger_Message_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
    LE_INFO("Indication message %x received from server.", msgPtr->payload);
    LE_TEST(msgPtr->payload == 0xDEADDEAD);

    // Release the message, now that we are finished with it.
    le_msg_ReleaseMsg(msgRef);

    // This is now the end of the test.  Check that we received all the responses that we expected.
    LE_TEST(ClientResponseCount == MAX_REQUEST_RESPONSE_TXNS);

    LE_TEST_SUMMARY
}


// This function will be called whenever the server sends us a response message or our
// request-response transaction fails.
static void ClientResponseRecvHandler
(
    le_msg_MessageRef_t  msgRef,    // Reference to response message (NULL if transaction failed).
    void*                contextPtr // contextPtr passed into le_msg_RequestResponse().
)
{
    LE_TEST(contextPtr == ClientRespContextStr);
    LE_TEST(strcmp(contextPtr, ClientRespContextStr) == 0);

    ClientResponseCount++;

    // Check if we got a response.
    if (msgRef == NULL)
    {
        // Transaction failed.  No response received.
        // This might happen if the server deleted the request without sending a response,
        // or if we had registered a "Session End Handler" and the session terminated before
        // the response was sent.
        LE_ERROR("Transaction failed!");
        LE_TEST(false);
    }
    else
    {
        // Process response message from the server.
        burger_Message_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
        LE_INFO("Response %x received from server.", msgPtr->payload);
        LE_TEST(msgPtr->payload == 0xBEEFDEAD);

        // Get the session reference before releasing the message.
        le_msg_SessionRef_t sessionRef = le_msg_GetSession(msgRef);

        // Release the response message, now that we are finished with it.
        le_msg_ReleaseMsg(msgRef);

        // Send some more stuff to the server.
        SendSomeStuffToServer(sessionRef);
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

    // Send a request to the server.
    msgRef = le_msg_CreateMsg(sessionRef);
    msgPtr = le_msg_GetPayloadPtr(msgRef);
    msgPtr->payload = 0xDEADBEEF;
    le_msg_RequestResponse(msgRef, ClientResponseRecvHandler, (void*)ClientRespContextStr);

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


// Start the client.
static void ClientStart
(
    const char* serviceInstanceName
)
{
    le_msg_ProtocolRef_t protocolRef;
    le_msg_SessionRef_t sessionRef;

    // Open a session.
    protocolRef = le_msg_GetProtocolRef(BURGER_PROTOCOL_ID_STR, sizeof(burger_Message_t));
    sessionRef = le_msg_CreateSession(protocolRef, serviceInstanceName);
    le_msg_SetSessionRecvHandler(sessionRef, ClientIndicationRecvHandler, (void*)ClientIndContextStr);
    le_msg_OpenSession(sessionRef, SessionOpenHandlerFunc, (void*)ClientOpenContextStr);

}


// Component initialization function.
COMPONENT_INIT
{
    LE_INFO("======= Test 1: Server and Client in same process ========");

    system("testFwMessaging-Setup");

    burgerServer_Start(SERVICE_INSTANCE_NAME, MAX_REQUEST_RESPONSE_TXNS);

    ClientStart(SERVICE_INSTANCE_NAME);
}
