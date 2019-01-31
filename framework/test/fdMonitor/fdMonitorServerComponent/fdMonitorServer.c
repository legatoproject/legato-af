//--------------------------------------------------------------------------------------------------
/**
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#if LE_CONFIG_LINUX
#   include <sys/socket.h>
#   include <netinet/ip.h>
#   include <arpa/inet.h>
#endif

#define LOCAL_IP_ADDR "127.0.0.1"
#define SERVER_PORT_NUM 5000

static le_fdMonitor_Ref_t ServerFdMonitor = NULL;
static int ServerFd = -1;
static le_fdMonitor_Ref_t ClientFdMonitor = NULL;
static const char PingMsg[] = "ping";
static const char PongMsg[] = "pong";

//--------------------------------------------------------------------------------------------------
/**
 * This function is called when a message is received from a client
 */
//--------------------------------------------------------------------------------------------------
static void ClientListenerHandler
(
    int clientFd,
    short events
)
{
    char msg[2*sizeof(PongMsg)];

    LE_TEST_INFO("%s called", __func__);

    LE_TEST_OK(events & POLLIN, "Detected POLLIN event.");

    LE_TEST_OK(read(clientFd, msg, sizeof(msg)) <= sizeof(msg), "Read response from client");
    LE_TEST_OK(strncmp(msg, PongMsg, sizeof(PongMsg)) == 0,
               "Received %s from client (expected %s)", msg, PongMsg);

    le_fdMonitor_Delete(ServerFdMonitor);
    le_fdMonitor_Delete(ClientFdMonitor);

    LE_TEST_OK(true, "FD monitors deleted");
    LE_TEST_OK(close(clientFd) == 0, "Closed client connection");
    LE_TEST_OK(close(ServerFd) == 0, "Closed server connection");

    LE_TEST_EXIT;
    LE_TEST_INFO("======== END FD MONITOR TEST ========");
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is called when a new client connects.
 */
//--------------------------------------------------------------------------------------------------
static void SocketListenerHandler
(
    int sockFd,
    short events
)
{
    int clientFd;

    LE_TEST_INFO("%s called", __func__);

    LE_TEST_OK(events & POLLIN, "Detected POLLIN event.");

    clientFd = accept(sockFd, NULL, NULL);
    LE_TEST_ASSERT(clientFd != -1, "Client connection accepted.");

    // Monitor client connection
    ClientFdMonitor = le_fdMonitor_Create("fdMonitorTestClient",
                                          clientFd, ClientListenerHandler, POLLIN);
    LE_TEST_OK(ClientFdMonitor != NULL, "Created ClientFdMonitor test object on socket.");

    LE_TEST_OK(write(clientFd, PingMsg, sizeof(PingMsg)) == sizeof(PingMsg),
               "Write ping to client");
}

COMPONENT_INIT
{
    LE_TEST_INFO("======== BEGIN FD MONITOR TEST ========");

    struct sockaddr_in myAddress;
    int ret;
    int opt;

    LE_TEST_PLAN(15);

    // Create the socket
    ServerFd = socket(AF_INET, SOCK_STREAM, 0);
    LE_TEST_ASSERT(ServerFd >= 0, "Socket created.");

    // Set ServerFd nonblocking
    opt = fcntl(ServerFd, F_GETFL, 0);
    LE_TEST_ASSERT(opt >= 0,"Get socket status flags: %x.", opt );

    opt |= O_NONBLOCK;
    ret = fcntl(ServerFd, F_SETFL, opt);
    LE_TEST_ASSERT(ret == 0, "Set non-blocking socket flag.");

    memset(&myAddress, 0, sizeof(myAddress));
    myAddress.sin_family = AF_INET;
    myAddress.sin_addr.s_addr = inet_addr(LOCAL_IP_ADDR);
    myAddress.sin_port = htons(SERVER_PORT_NUM);

    // Bind server - socket
    ret = bind(ServerFd, (struct sockaddr*) &myAddress, sizeof(myAddress));
    LE_TEST_ASSERT(ret == 0, "Bind socket.");

    // Listen will return immediately since socket is non-blocking.
    // Monitoring for connection will be done through ServerFdMonitor.
    ret = listen(ServerFd, 1);
    LE_TEST_ASSERT(ret == 0, "Listen socket.");

    // Monitor connection on the socket.
    ServerFdMonitor = le_fdMonitor_Create("fdMonitorTestServer",
                                          ServerFd, SocketListenerHandler, POLLIN);
    LE_TEST_OK(ServerFdMonitor != NULL, "Created ServerFdMonitor test object on socket.");

    LE_TEST_INFO("Monitoring socket for client connection");
}
