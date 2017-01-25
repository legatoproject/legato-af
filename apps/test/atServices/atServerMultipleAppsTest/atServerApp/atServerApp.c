/**
 * This module implements the integration tests for AT commands server API.
 *
 * How to use this test:
 * Open a first connection on port 1234
 *      Example using telnet on a Linux machine:
 *      telnet $TARGET_IP 1234
 *      Trying 192.168.2.2...
 *      Connected to 192.168.2.2.
 *      Escape character is '^]'.
 *      at
 *
 *      OK
 * Open a second connection on port 1234
 *
 * Both clients can use all of the below created commands
 * If the client that created the commands dies, the other client can't use
 * them anymore, an ERROR will be sent instead
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "handlers/handlers.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of connected clients
 */
//--------------------------------------------------------------------------------------------------
#define CLIENTS_MAX 2

//--------------------------------------------------------------------------------------------------
/**
 * Port number
 */
//--------------------------------------------------------------------------------------------------
#define PORT        1234

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of file descriptors to monitor
 */
//--------------------------------------------------------------------------------------------------
#define EVENTS_MAX  3

//--------------------------------------------------------------------------------------------------
/**
 * clientInfo_t struct definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int fd;
    le_thread_Ref_t ref;
}
clientInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * First client thread function
 */
//--------------------------------------------------------------------------------------------------
static void* FirstClientStartServer
(
    void* contextPtr
)
{
    int i = 0;
    clientInfo_t* myInfo;
    static atSession_t AtSession;
    myInfo = (clientInfo_t *)contextPtr;

    LE_INFO("%s started", le_thread_GetMyName());

    atCmd_t AtCmdCreation[] =
    {
        {
            .atCmdPtr = "AT+DEL",
            .handlerPtr = DelCmdHandler,
        },
        {
            .atCmdPtr = "AT+CLOSE",
            .handlerPtr = CloseCmdHandler,
        },
        {
            .atCmdPtr = "AT+ABCD",
            .handlerPtr = GenericCmdHandler,
        },
        {
            .atCmdPtr = "AT",
            .handlerPtr = AtCmdHandler,
        },
        {
            .atCmdPtr = "ATA",
            .handlerPtr = GenericCmdHandler,
        },
        {
            .atCmdPtr = "ATE",
            .handlerPtr = GenericCmdHandler,
        },
    };

    le_atServer_ConnectService();

    AtSession.devRef = le_atServer_Open(dup(myInfo->fd));
    LE_ASSERT(AtSession.devRef != NULL);

    AtSession.cmdsCount = NUM_ARRAY_MEMBERS(AtCmdCreation);

    // AT commands subscriptions
    while ( i < AtSession.cmdsCount )
    {
        AtCmdCreation[i].cmdRef = le_atServer_Create(AtCmdCreation[i].atCmdPtr);
        LE_ASSERT(AtCmdCreation[i].cmdRef != NULL);

        AtSession.atCmds[i] = AtCmdCreation[i];

        le_atServer_AddCommandHandler(
            AtCmdCreation[i].cmdRef, AtCmdCreation[i].handlerPtr,
            (void *) &AtSession);

        i++;
    }
    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Second client thread function
 */
//--------------------------------------------------------------------------------------------------
static void *SecondClientStartServer
(
    void* contextPtr
)
{
    int i = 0;
    clientInfo_t* myInfo;
    static atSession_t AtSession;

    myInfo = (clientInfo_t *)contextPtr;

    LE_INFO("%s started", le_thread_GetMyName());

    atCmd_t AtCmdCreation[] =
    {
        {
            .atCmdPtr = "ATD",
            .handlerPtr = DelCmdHandler,
        },
        {
            .atCmdPtr = "ATC",
            .handlerPtr = CloseCmdHandler,
        },
        {
            .atCmdPtr = "AT+ABCD",
            .handlerPtr = GenericCmdHandler,
        },
        {
            .atCmdPtr = "AT",
            .handlerPtr = AtCmdHandler,
        },
        {
            .atCmdPtr = "ATA",
            .handlerPtr = GenericCmdHandler,
        },
        {
            .atCmdPtr = "ATE",
            .handlerPtr = GenericCmdHandler,
        },
    };

    le_atServer_ConnectService();

    AtSession.devRef = le_atServer_Open(dup(myInfo->fd));
    LE_ASSERT(AtSession.devRef != NULL);

    AtSession.cmdsCount = NUM_ARRAY_MEMBERS(AtCmdCreation);

    // AT commands subscriptions
    while ( i < AtSession.cmdsCount )
    {
        AtCmdCreation[i].cmdRef = le_atServer_Create(AtCmdCreation[i].atCmdPtr);
        LE_ASSERT(AtCmdCreation[i].cmdRef != NULL);

        AtSession.atCmds[i] = AtCmdCreation[i];

        le_atServer_AddCommandHandler(
            AtCmdCreation[i].cmdRef, AtCmdCreation[i].handlerPtr,
            (void *) &AtSession);

        i++;
    }
    le_event_RunLoop();
    return NULL;
}
//------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//------------------------------------------------------------------------------
COMPONENT_INIT
{
    int ret, optVal = 1, epfd, i, j, clientsCount = 0, status = 0;
    int sockFd, connFd, flags;
    struct sockaddr_in myAddress, clientAddress;
    struct epoll_event event, events[EVENTS_MAX];
    clientInfo_t clientInfo[CLIENTS_MAX];

    LE_INFO("AT server test started");

    // Create the socket
    sockFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (sockFd == -1)
    {
        LE_ERROR("creating socket failed: %m");
        exit(errno);
    }

    // set socket option
    ret = setsockopt(sockFd, SOL_SOCKET, \
            SO_REUSEADDR, &optVal, sizeof(optVal));
    if (ret)
    {
        LE_ERROR("error setting socket option %m");
        close(sockFd);
        exit(errno);
    }


    memset(&myAddress,0,sizeof(myAddress));

    myAddress.sin_port = htons(PORT);
    myAddress.sin_family = AF_INET;
    myAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind server - socket
    ret = bind(sockFd,(struct sockaddr_in *)&myAddress,sizeof(myAddress));
    if (ret)
    {
        LE_ERROR("bind to socket failed %m");
        close(sockFd);
        exit(errno);
    }

    // Listen to the socket
    ret = listen(sockFd, CLIENTS_MAX);
    if (ret)
    {
        LE_ERROR("listen failed %m");
        close(sockFd);
        exit(errno);
    }

    // setup epoll
    epfd = epoll_create1(0);
    if (epfd == -1)
    {
        LE_ERROR("epoll_create1 failed %m");
        close(sockFd);
        exit(errno);
    }

    event.events = EPOLLIN | EPOLLRDHUP;
    event.data.fd = sockFd;

    // add socket fd to monitor
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockFd, &event);
    if (ret)
    {
        LE_ERROR("epoll_ctl failed %m");
        close(sockFd);
        close(epfd);
        exit(errno);
    }

    // prepare first client thread
    clientInfo[0].ref = le_thread_Create("atServer-first-client",
                            FirstClientStartServer, (void *) &clientInfo[0]);
    le_thread_SetJoinable(clientInfo[0].ref);

    // prepare second client thread
    clientInfo[1].ref = le_thread_Create("atServer-second-client",
                            SecondClientStartServer, (void *) &clientInfo[1]);
    le_thread_SetJoinable(clientInfo[1].ref);

    /*
     * The socket fd is in non blocking mode so wait for a client to connect
     * Once a client connects, accept the connection, register its fd within
     * the epoll waiting loop and start an atServer thread.
     * When a client dies cancel its thread and deregister its fd from epoll
     * waiting loop. If all clients die cleanup and exit
    */
    while (1)
    {
        // wait for events on file descriptors
        ret = epoll_wait(epfd, events, EVENTS_MAX, -1);
        if (ret == -1)
        {
            LE_ERROR("epoll_wait failed %m");
            close(sockFd);
            close(epfd);
            exit(errno);
        }

        for (i=0; i<ret; i++)
        {
            if (events[i].events & EPOLLRDHUP)
            {
                for (j=0; j<clientsCount; j++)
                {
                    if (clientInfo[j].fd == events[i].data.fd)
                    {
                        errno = ECONNRESET;
                        if (LE_OK == le_thread_Cancel(clientInfo[j].ref))
                        {
                            le_thread_Join(clientInfo[j].ref, NULL);
                            epoll_ctl(epfd,
                                EPOLL_CTL_DEL, clientInfo[j].fd, &event);
                            if(close(clientInfo[j].fd))
                                LE_INFO("%m");
                            status--;
                        }
                    }
                }
            }

            if (events[i].data.fd == sockFd)
            {
                socklen_t addressLen = sizeof(clientAddress);
                connFd = accept(sockFd,
                            (struct sockaddr *)&clientAddress,
                            &addressLen);

                flags = fcntl(connFd, F_GETFL, 0);
                if (flags >= 0)
                {
                    flags |= O_NONBLOCK;
                    if (fcntl(connFd, F_SETFL, flags) == -1)
                    {
                        LE_ERROR("fcntl failed: %m");
                    }
                }

                event.events = EPOLLIN | EPOLLRDHUP;
                event.data.fd = connFd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, connFd, &event);

                clientInfo[clientsCount].fd = connFd;

                le_thread_Start(clientInfo[clientsCount].ref);

                clientsCount++;
                status++;
            }
        }

        if (status <= 0)
        {
            break;
        }
    }

    close(sockFd);
    close(epfd);

    exit(0);
}
