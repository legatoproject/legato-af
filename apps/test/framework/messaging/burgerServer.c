//--------------------------------------------------------------------------------------------------
/**
 * Automated unit test for the Low-Level Messaging APIs.
 *
 * Burger Protocol Server functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "burgerProtocol.h"
#include "burgerServer.h"


static const char ContextStr[] = "This is the server.";
static const char* ServiceOpenContextPtr = ContextStr;

//--------------------------------------------------------------------------------------------------
/**
 * Context object for a single server instance.
 **/
//--------------------------------------------------------------------------------------------------
typedef struct
{
    const char* strPtr;     ///< Always set to point to ContextStr.
    int requestCount;       ///< Count of the number of requests received from the client.
    int maxRequestCount;    ///< Maximum number of request-response transactions that a client
                            /// can start before the server sends it a 0xDEADDEAD message.
}
Context_t;


//--------------------------------------------------------------------------------------------------
/**
 * Message receive handler for the service instance.
 **/
//--------------------------------------------------------------------------------------------------
static void MsgRecvHandler
(
    le_msg_MessageRef_t msgRef,             ///< Reference to the received message.
    void*               opaqueContextPtr    ///< contextPtr passed to le_msg_SetServiceRecvHandler()
)
//--------------------------------------------------------------------------------------------------
{
    Context_t* contextPtr = opaqueContextPtr;

    LE_ASSERT(contextPtr != NULL);
    LE_TEST(contextPtr->strPtr == ContextStr);
    LE_TEST(strcmp(contextPtr->strPtr, ContextStr) == 0);

    le_msg_SessionRef_t sessionRef = le_msg_GetSession(msgRef);
    LE_ASSERT(sessionRef != NULL);

    burger_Message_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
    LE_ASSERT(msgPtr != NULL);

    LE_INFO("Received '%x'", msgPtr->payload);

    switch (msgPtr->payload)
    {
        case 0xBEEFBEEF:
            LE_TEST(le_msg_NeedsResponse(msgRef) == false);
            le_msg_ReleaseMsg(msgRef);

            break;

        case 0xDEADBEEF:
            LE_TEST(le_msg_NeedsResponse(msgRef) == true);
            contextPtr->requestCount++;
            LE_INFO("Received transaction request (%d/%d).",
                    contextPtr->requestCount,
                    contextPtr->maxRequestCount);

            // Construct and send response.
            msgPtr->payload = 0xBEEFDEAD;
            le_msg_Respond(msgRef);

            // If we have received the magic number of requests, tell the client to
            // terminate the test by sending 0xDEADDEAD to the client.
            if (contextPtr->requestCount >= contextPtr->maxRequestCount)
            {
                LE_INFO("Maximum number of request-response transactions reached.");
                msgRef = le_msg_CreateMsg(sessionRef);
                msgPtr = le_msg_GetPayloadPtr(msgRef);
                msgPtr->payload = 0xDEADDEAD;
                le_msg_Send(msgRef);
            }
            break;

        default:
            LE_FATAL("Unexpected message payload (%x)", msgPtr->payload);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called when a client opens a new session.
 */
//--------------------------------------------------------------------------------------------------
static void NewSessionHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
//--------------------------------------------------------------------------------------------------
{
    LE_INFO("Client started a new session.");

    LE_INFO("contextPtr = %p.", contextPtr);
    LE_TEST(contextPtr == &ServiceOpenContextPtr);

    // Because the unit tests are run always as a single, non-root user, we expect the user ID
    // of the client to be the same user ID that we are running as.
    uid_t clientUserId;
    uid_t myUserId = getuid();
    le_result_t result = le_msg_GetClientUserId(sessionRef, &clientUserId);
    LE_INFO("le_msg_GetClientUserId() returned '%s' with UID %u.",
            LE_RESULT_TXT(result),
            clientUserId);
    LE_INFO("getuid() returned %u.", myUserId);
    LE_TEST(clientUserId == myUserId);
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts an instance of the Burger Protocol server in the calling thread.
 **/
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t burgerServer_Start
(
    const char* serviceInstanceName,
    size_t maxRequests
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_ProtocolRef_t protocolRef;
    le_msg_ServiceRef_t serviceRef;

    Context_t* contextPtr = malloc(sizeof(Context_t));
    LE_ASSERT(contextPtr != NULL);

    contextPtr->strPtr = ContextStr;
    contextPtr->maxRequestCount = maxRequests;
    contextPtr->requestCount = 0;

    protocolRef = le_msg_GetProtocolRef(BURGER_PROTOCOL_ID_STR, sizeof(burger_Message_t));
    serviceRef = le_msg_CreateService(protocolRef, serviceInstanceName);
    le_msg_SetServiceRecvHandler(serviceRef, MsgRecvHandler, contextPtr);
    LE_INFO("&ServiceOpenContextPtr = %p.", &ServiceOpenContextPtr);
    le_msg_AddServiceOpenHandler(serviceRef, NewSessionHandler, &ServiceOpenContextPtr);
    le_msg_AdvertiseService(serviceRef);

    return serviceRef;
}
