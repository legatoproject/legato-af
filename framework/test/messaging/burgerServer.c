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

#if defined(TEST_LOCAL)
//--------------------------------------------------------------------------------------------------
/**
 * Pool for burger messages.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(BurgerMessage,
                          2,
                          LE_MSG_LOCAL_HEADER_SIZE +
                          sizeof(burger_Message_t));

//--------------------------------------------------------------------------------------------------
/**
 * On RTOS, use a local service.
 */
//--------------------------------------------------------------------------------------------------
le_msg_LocalService_t BurgerService;
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Burger server service reference.
 */
//--------------------------------------------------------------------------------------------------
static le_msg_ServiceRef_t BurgerServiceRef = NULL;


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

    LE_TEST_ASSERT(msgRef != NULL,
                   "message pointer %p set in message receive", msgRef);
    LE_TEST_ASSERT(contextPtr != NULL,
                   "context pointer set in message receive");
    LE_TEST_OK(contextPtr->strPtr == ContextStr,
               "context pointer correct address in message receive");
    LE_TEST_OK(strcmp(contextPtr->strPtr, ContextStr) == 0,
               "context pointer correct value in message receive");

    le_msg_SessionRef_t sessionRef = le_msg_GetSession(msgRef);
    LE_TEST_ASSERT(sessionRef != NULL, "session set in message receive");

    burger_Message_t* msgPtr = le_msg_GetPayloadPtr(msgRef);
    LE_TEST_ASSERT(msgPtr != NULL, "message pointer %p valid in message receive", msgPtr);

    LE_TEST_INFO("Received '%"PRIx32"'", msgPtr->payload);

    switch (msgPtr->payload)
    {
        case 0xBEEFBEEF:
            LE_TEST_OK(le_msg_NeedsResponse(msgRef) == false, "check no-response message");
            le_msg_ReleaseMsg(msgRef);

            LE_TEST_INFO("Message released");

            // Ping back to client so it knows data has been processed and can continue
            // with test
            msgRef = le_msg_CreateMsg(sessionRef);
            msgPtr = le_msg_GetPayloadPtr(msgRef);
            msgPtr->payload = 0xBEEFBEEF;
            le_msg_Send(msgRef);

            LE_TEST_INFO("message ping back sent");

            break;

        case 0xDEADBEEF:
            LE_TEST_OK(le_msg_NeedsResponse(msgRef) == true, "check response needed message");
            contextPtr->requestCount++;
            LE_TEST_INFO("Received transaction request (%d/%d).",
                         contextPtr->requestCount,
                         contextPtr->maxRequestCount);

            // Construct and send response.
            msgPtr->payload = 0xBEEFDEAD;
            le_msg_Respond(msgRef);

            // If we have received the magic number of requests, tell the client to
            // terminate the test by sending 0xDEADDEAD to the client.
            if (contextPtr->requestCount >= contextPtr->maxRequestCount)
            {
                LE_DEBUG("Maximum number of request-response transactions reached.");
                msgRef = le_msg_CreateMsg(sessionRef);
                msgPtr = le_msg_GetPayloadPtr(msgRef);
                msgPtr->payload = 0xDEADDEAD;
                le_msg_Send(msgRef);
            }
            break;

        default:
            LE_TEST_FATAL("Unexpected message payload (%"PRIx32")", msgPtr->payload);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that gets called when a client opens a new session.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((unused)) static void NewSessionHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
//--------------------------------------------------------------------------------------------------
{
    LE_TEST_INFO("Client started a new session.");

    LE_TEST_INFO("contextPtr = %p.", contextPtr);
    LE_TEST_OK(contextPtr == &ServiceOpenContextPtr, "context pointer set in new session handler");

    // Because the unit tests are run always as a single, non-root user, we expect the user ID
    // of the client to be the same user ID that we are running as.
    LE_TEST_BEGIN_SKIP(!LE_CONFIG_IS_ENABLED(LE_CONFIG_LINUX), 1)
    uid_t clientUserId;
    uid_t myUserId = getuid();
    le_result_t result = le_msg_GetClientUserId(sessionRef, &clientUserId);
    LE_TEST_INFO("le_msg_GetClientUserId() returned '%s' with UID %u.",
                 LE_RESULT_TXT(result),
                 clientUserId);
    LE_TEST_INFO("getuid() returned %u.", myUserId);
    LE_TEST_OK(clientUserId == myUserId, "check client uid");
    LE_TEST_END_SKIP();
}



//--------------------------------------------------------------------------------------------------
/**
 * Initializes burger server.  On non-Linux systems must be run before any client can even try
 * to connect, but doesn't need to be run on server thread.
 */
//--------------------------------------------------------------------------------------------------
void burgerServer_Init
(
    const char* serviceInstanceName
)
{
#if defined(TEST_LOCAL)
    BurgerServiceRef = le_msg_InitLocalService(&BurgerService,
                                               serviceInstanceName,
                                               le_mem_InitStaticPool(BurgerMessage,
                                                                     2,
                                                                     LE_MSG_LOCAL_HEADER_SIZE +
                                                                     sizeof(burger_Message_t)));
#endif
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
    Context_t* contextPtr = malloc(sizeof(Context_t));
    LE_ASSERT(contextPtr != NULL);

    contextPtr->strPtr = ContextStr;
    contextPtr->maxRequestCount = maxRequests;
    contextPtr->requestCount = 0;

#if defined(TEST_UNIX_SOCKET)
    {
        le_msg_ProtocolRef_t protocolRef;
        protocolRef = le_msg_GetProtocolRef(BURGER_PROTOCOL_ID_STR, sizeof(burger_Message_t));
        BurgerServiceRef = le_msg_CreateService(protocolRef, serviceInstanceName);
    }
#endif
    le_msg_SetServiceRecvHandler(BurgerServiceRef, MsgRecvHandler, contextPtr);
    LE_TEST_INFO("&ServiceOpenContextPtr = %p.", &ServiceOpenContextPtr);
#if defined(TEST_UNIX_SOCKET)
    {
        le_msg_AddServiceOpenHandler(BurgerServiceRef, NewSessionHandler, &ServiceOpenContextPtr);
    }
#endif
    le_msg_AdvertiseService(BurgerServiceRef);

    return BurgerServiceRef;
}
