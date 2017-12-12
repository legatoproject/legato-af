/**
 * client.c implements the clientserver part of the unit test
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "defs.h"
#include "strerror.h"

//--------------------------------------------------------------------------------------------------
/**
 * Byte length to read from fd.
 */
//--------------------------------------------------------------------------------------------------
#define READ_BYTES           100

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of monitorname
 */
//--------------------------------------------------------------------------------------------------
#define MAX_LEN_MONITOR_NAME 64

//--------------------------------------------------------------------------------------------------
/**
 * ClientData_t definition
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int socketFd;
    int connFd;
}
ClientData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Client thread shared data
 */
//--------------------------------------------------------------------------------------------------
static ClientData_t ClientData;


//--------------------------------------------------------------------------------------------------
/**
 * This function is called when data are available to be read on fd
 */
//--------------------------------------------------------------------------------------------------
static void RxNewData
(
    int fd,      ///< [IN] File descriptor to read on
    short events ///< [IN] Event reported on fd
)
{
    char buffer[READ_BYTES];
    ssize_t count;

    SharedData_t* sharedDataPtr = le_fdMonitor_GetContextPtr();
    le_sem_Post(sharedDataPtr->semRef);

    memset(buffer, 0, sizeof(buffer));

    if (events & (POLLIN | POLLPRI))
    {
        count = read(fd, buffer, READ_BYTES);
        if (count == -1)
        {
            LE_ERROR("read error: %s", strerror(errno));
            return;
        }
        else if (count > 0)
        {
            if (strcmp(buffer, "AT+CREG?\r") == 0)
            {
                LE_INFO("Received AT command: %s", buffer);
                // Send the response of AT command
                write(fd, "\r\n\r\n+CREG: 0,1\r\n\r\n\r\nOK\r\n", 24);
                return;
            }
            else if (strcmp(buffer, "AT+CGSN\r") == 0)
            {
                LE_INFO("Received AT command: %s", buffer);
                // Send the response of AT command
                write(fd, "\r\n359377060033064\r\n\r\nOK\r\n", 25);
                return;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * AtClientServer function to receive the response from the server
 */
//--------------------------------------------------------------------------------------------------
void AtClientServer
(
    SharedData_t* sharedDataPtr
)
{
    struct sockaddr_un addr;
    int epollFd;
    struct epoll_event event;
    char monitorName[MAX_LEN_MONITOR_NAME];
    le_fdMonitor_Ref_t fdMonitorRef;

    LE_INFO("AtClientServer Started !!!");

    ClientData.socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
    LE_ASSERT(ClientData.socketFd != -1);

    epollFd = epoll_create1(0);
    LE_ASSERT(epollFd != -1);

    event.events = EPOLLIN | EPOLLRDHUP;
    event.data.fd = ClientData.socketFd;
    LE_ASSERT(epoll_ctl(epollFd, EPOLL_CTL_ADD, ClientData.socketFd, &event) == 0);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family= AF_UNIX;
    strncpy(addr.sun_path, sharedDataPtr->devPathPtr, sizeof(addr.sun_path)-1);

    LE_ASSERT(bind(ClientData.socketFd, (struct sockaddr*) &addr, sizeof(addr)) != -1);
    LE_ASSERT(listen(ClientData.socketFd, 1) != -1);

    le_sem_Post(sharedDataPtr->semRef);

    ClientData.connFd = accept(ClientData.socketFd, NULL, NULL);
    LE_ASSERT(ClientData.connFd != -1);

    le_sem_Post(sharedDataPtr->semRef);

    snprintf(monitorName, sizeof(monitorName), "Monitor-%d", ClientData.connFd);

    fdMonitorRef = le_fdMonitor_Create(monitorName, dup(ClientData.connFd), RxNewData,
                                       POLLIN | POLLPRI | POLLRDHUP);
    le_fdMonitor_SetContextPtr(fdMonitorRef, sharedDataPtr);
}
