//--------------------------------------------------------------------------------------------------
/**
 * Server for unit test 3 for the Low-Level Messaging APIs.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"

// 1. Client sends a request message with nothing in it.
// 2. Server checks that when it gets an fd from that message it gets -1.
// 3. Server sends back a response containing nothing, followed by a new message with nothing in it.
// 4. Client verifies that both of these don't carry an fd (le_msg_GetFd() returns -1).
// 5. Client creates a pipe and writes a small amount of data into the write end.
// 6. Clients sends a request with the read end of the pipe to the server.
// 7. Server responds with another fd that is the read end of another pipe, which it copies the
//    data into from the client's pipe into.
// 8. Client verifies that it can read back the data it wrote, with the first character changed to
//    an 'N'.
// 9. Client sends a request message with the word "EVIL" in it.
// 10. When the server receives the "EVIL" message, it uses LE_KILL_CLIENT() to terminate the
//     connection to the client and then shuts itself down.
// 11. When the client verifies that the server severed the connection, it shuts itself down.

static void MessageReceiveHandler
(
    le_msg_MessageRef_t msgRef,
    void* ignored
)
{
    int fdFromClient = le_msg_GetFd(msgRef);
    LE_INFO("Received fd %d from client.", fdFromClient);

    if (fdFromClient >= 0)
    {
        // Make sure the fd isn't one of stdin, stdout, or stderr.
        // If it is, then one of those was accidentally closed earlier, which is an error.
        LE_TEST(fdFromClient > 2);

        // Open a pipe.
        int fdList[2];
        if (pipe(fdList) != 0)
        {
            LE_FATAL("Failed to create pipe (%m).");
        }
        LE_INFO("Created pipe (%d, %d).", fdList[0], fdList[1]);

        // Set the client fd non-blocking.
        if (fcntl(fdFromClient, F_SETFL, O_NONBLOCK) != 0)
        {
            LE_FATAL("Failed to set fd %d non-blocking (%m)", fdFromClient);
        }

        // Copy data from the fd the client sent us to the pipe we just created.
        LE_INFO("Copying data from fd %d to fd %d.", fdFromClient, fdList[1]);
        ssize_t bytesRead;
        bool isFirstChar = true;
        char c;
        for (;;)
        {
            do
            {
                bytesRead = read(fdFromClient, &c, 1);
            } while ((bytesRead == -1) && (errno == EINTR));

            if (   (bytesRead == 0)
                || ((bytesRead == -1) && ((errno = EWOULDBLOCK) || (errno == EAGAIN)))  )
            {
                // Nothing left to read.
                break;
            }

            LE_FATAL_IF(bytesRead < 0, "Failed to read from client fd (%m).");

            if (isFirstChar)
            {
                c = 'N';
                isFirstChar = false;
            }

            ssize_t bytesWritten;
            do
            {
                bytesWritten = write(fdList[1], &c, 1);
            } while ((bytesWritten == -1) && (errno == EINTR));

            LE_FATAL_IF(bytesWritten < 0, "Failed to write to pipe (%m).");
            LE_ASSERT(bytesWritten != 0);
        }
        close(fdList[1]);

        // Send the read end of my pipe back to the client.
        // Do this after copying the data so there's no race with the client reading.
        LE_INFO("Sending fd %d to client.", fdList[0]);
        le_msg_SetFd(msgRef, fdList[0]);
        le_msg_Respond(msgRef);
    }
    else
    {
        char* buffPtr = le_msg_GetPayloadPtr(msgRef);
        LE_ASSERT(buffPtr != NULL);
        LE_ASSERT(le_msg_GetMaxPayloadSize(msgRef) > 5);

        if (buffPtr[0] == 'E')
        {
            LE_INFO("Message received from evil client.  Session ref %p.",
                    le_msg_GetSession(msgRef));

            LE_KILL_CLIENT("Die, client, die!");

            le_msg_Respond(msgRef);

            LE_TEST_EXIT;
        }
        else
        {
            // Received a non-evil request from a client with no fd in it.
            // Send a response with no fd followed by a server-to-client request with no fd in it.
            le_msg_SessionRef_t sessionRef = le_msg_GetSession(msgRef);
            le_msg_Respond(msgRef);

            le_msg_MessageRef_t newMsgRef = le_msg_CreateMsg(sessionRef);
            LE_ASSERT(newMsgRef != NULL);
            le_msg_Send(newMsgRef);
        }
    }
}


COMPONENT_INIT
{
    LE_TEST_INIT;

    le_msg_ProtocolRef_t protocolRef;
    le_msg_ServiceRef_t serviceRef;

    // Create and advertise the service.
    protocolRef = le_msg_GetProtocolRef("testFwMessaging3", 10);
    serviceRef = le_msg_CreateService(protocolRef, "messagingTest3");
    le_msg_SetServiceRecvHandler(serviceRef, MessageReceiveHandler, NULL);
    le_msg_AdvertiseService(serviceRef);
}
