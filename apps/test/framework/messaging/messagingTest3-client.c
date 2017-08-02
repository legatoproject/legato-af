//--------------------------------------------------------------------------------------------------
/**
 * Client for unit test for the Low-Level Messaging APIs.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"


// NOTE: See messagingTest3-server.c for a description of the test.


static void ServerTriedToKillMe
(
    le_msg_SessionRef_t sessionRef,
    void* ignored
)
{
    LE_INFO("Server tried to kill me!  I expected that.");

    LE_TEST_EXIT;
}


static void ServerSentMeAnotherMessage
(
    le_msg_MessageRef_t msgRef,
    void* ignored
)
{
    LE_INFO("Server sent me a message.  Shouldn't have an fd in it.");

    LE_TEST(le_msg_GetFd(msgRef) == -1);

    le_msg_SessionRef_t sessionRef = le_msg_GetSession(msgRef);
    le_msg_ReleaseMsg(msgRef);

    // Create a pipe.
    int fdList[2];
    if (pipe(fdList) != 0)
    {
        LE_FATAL("Failed to create pipe (%m).");
    }

    LE_INFO("Created pipe (%d, %d).", fdList[0], fdList[1]);

    // Write something small into the pipe (small enough to fit in the pipe buffer without
    // blocking us).
    LE_INFO("Writing to fd %d.", fdList[1]);
    const char requestText[] = "FOO!";
    const char expectedResponseText[] = "NOO!";
    ssize_t byteCount;
    do
    {
        byteCount = write(fdList[1], requestText, sizeof(requestText));
    } while ((-1 == byteCount) && (EINTR == errno));

    LE_FATAL_IF(byteCount != sizeof(requestText), "write() returned %zd (errno = %m).", byteCount);

    // Pass the read end of the pipe to the server and receive back the read end of another pipe.
    LE_INFO("Sending fd %d to server.", fdList[0]);
    msgRef = le_msg_CreateMsg(sessionRef);
    le_msg_SetFd(msgRef, fdList[0]);
    msgRef = le_msg_RequestSyncResponse(msgRef);
    LE_ASSERT(NULL != msgRef);
    int fdFromServer = le_msg_GetFd(msgRef);
    LE_INFO("Received fd %d from server.", fdFromServer);
    LE_ASSERT(fdFromServer >= 0);
    le_msg_ReleaseMsg(msgRef);

    // Read what I wrote.  The server should have copied data from my pipe to its pipe, but
    // changed the first character to a 'N'.
    char rxBuff[sizeof(expectedResponseText) + 1];
    do
    {
        byteCount = read(fdFromServer, rxBuff, sizeof(rxBuff));
    } while ((-1 == byteCount) && (EINTR == errno));

    LE_FATAL_IF(byteCount < 0, "read() failed on fd %d (%m)", fdFromServer);
    LE_TEST(byteCount == sizeof(expectedResponseText));
    // Check whether number of bytes read from fdFromServer file descriptor is 0.
    LE_FATAL_IF(byteCount == 0, "number of bytes read from fdFromServer is 0");
    LE_TEST(rxBuff[byteCount - 1] == '\0');
    LE_INFO("Received '%s' from server.", rxBuff);
    LE_TEST(0 == strcmp(rxBuff, expectedResponseText));

    // Tell the server I'm evil.
    msgRef = le_msg_CreateMsg(sessionRef);
    char* buffPtr = le_msg_GetPayloadPtr(msgRef);
    LE_ASSERT(NULL != buffPtr);
    LE_ASSERT(le_utf8_Copy(buffPtr, "EVIL", le_msg_GetMaxPayloadSize(msgRef), NULL) == LE_OK);
    msgRef = le_msg_RequestSyncResponse(msgRef);

    // I expect the server to have tried to kill me by terminating my session.
    // This should result in a NULL response message reference, and my close handler will be called.
    LE_TEST(NULL == msgRef);
}


COMPONENT_INIT
{
    LE_TEST_INIT;

    le_msg_ProtocolRef_t protocolRef;
    le_msg_SessionRef_t sessionRef;
    le_msg_MessageRef_t msgRef;

    // Open a session with the server.
    protocolRef = le_msg_GetProtocolRef("testFwMessaging3", 10);
    sessionRef = le_msg_CreateSession(protocolRef, "messagingTest3");
    le_msg_SetSessionCloseHandler(sessionRef, ServerTriedToKillMe, NULL);
    le_msg_SetSessionRecvHandler(sessionRef, ServerSentMeAnotherMessage, NULL);
    le_msg_OpenSessionSync(sessionRef);

    // Do a synchronous request-response with an empty request.
    LE_INFO("Sending message to server without fd in it.");
    msgRef = le_msg_CreateMsg(sessionRef);
    msgRef = le_msg_RequestSyncResponse(msgRef);
    LE_ASSERT(msgRef != NULL);

    // Verify that we get -1 when we try to fetch an fd from the server's response.
    int fdFromServer = le_msg_GetFd(msgRef);
    LE_ASSERT(fdFromServer == -1);
    le_msg_ReleaseMsg(msgRef);

    // Server should send me another message after this.
}
