/**
 * This module implements the atBinder application.
 *
 * This application creates a socket to listen and accept one connection from a platform dependent
 * process which routes the AT commands.
 * Then, it opens a device with the le_atServer_Open() function to start the AT commands
 * monitoring with the atServer.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include "watchdogChain.h"

//--------------------------------------------------------------------------------------------------
/**
 * Socket path
 */
//--------------------------------------------------------------------------------------------------
#define AT_BINDER_SOCK_PATH "/tmp/atBinder"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of clients
 */
//--------------------------------------------------------------------------------------------------
#define MAX_CLIENTS         1

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 8

//--------------------------------------------------------------------------------------------------
/**
 * Close a fd and log a warning message if an error occurs
 */
//--------------------------------------------------------------------------------------------------
static void CloseWarn
(
    int fd
)
{
    if (-1 == close(fd))
    {
        LE_WARN("failed to close fd %d: %m", fd);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Monitor the client's fd
 */
//--------------------------------------------------------------------------------------------------
static void MonitorClient
(
    int clientFd,
    short events
)
{
    le_result_t result;
    le_atServer_DeviceRef_t atServerRef;

    if (POLLRDHUP & events)
    {
        LE_INFO("fd %d: connection reset by peer", clientFd);
    }
    else
    {
        LE_WARN("events %.8x not handled", events);
    }

    atServerRef = (le_atServer_DeviceRef_t)le_fdMonitor_GetContextPtr();
    if (atServerRef)
    {
        result = le_atServer_Close(atServerRef);
        if (LE_OK != result)
        {
            LE_ERROR("failed to close atServer device");
        }
    }
    else
    {
        LE_ERROR("failed to get atServer device reference");
    }

    le_fdMonitor_Delete(le_fdMonitor_GetMonitor());

    CloseWarn(clientFd);
}

//--------------------------------------------------------------------------------------------------
/**
 * Monitor the socket's fd
 */
//--------------------------------------------------------------------------------------------------
static void MonitorSocket
(
    int sockFd,
    short events
)
{
    int clientFd;
    le_fdMonitor_Ref_t fdMonitorRef;
    le_atServer_DeviceRef_t atServerRef;

    if (POLLIN & events)
    {
        clientFd = accept(sockFd, NULL, NULL);
        if (-1 == clientFd)
        {
            LE_ERROR("accepting socket failed: %m");
            goto exit_close_socket;
        }
        atServerRef = le_atServer_Open(dup(clientFd));
        if (!atServerRef)
        {
            LE_ERROR("Cannot open the device!");
            CloseWarn(clientFd);
            goto exit_close_socket;
        }
        fdMonitorRef = le_fdMonitor_Create("atBinder-client", clientFd, MonitorClient, POLLRDHUP);
        le_fdMonitor_SetContextPtr(fdMonitorRef, atServerRef);
        LE_INFO("atBinder is ready");
        return;
    }

    LE_WARN("events %.8x not handled", events);

exit_close_socket:
    le_fdMonitor_Delete(le_fdMonitor_GetMonitor());
    CloseWarn(sockFd);
    exit(EXIT_FAILURE);
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the atBinder application.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int sockFd;
    struct sockaddr_un myAddress;

    LE_INFO("atBinder starts");

    if ( (-1 == unlink(AT_BINDER_SOCK_PATH)) && (ENOENT != errno) )
    {
        LE_ERROR("unlink socket failed: %m");
        exit(EXIT_FAILURE);
    }

    // Create the socket
    sockFd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (-1 == sockFd)
    {
        LE_ERROR("creating socket failed: %m");
        exit(EXIT_FAILURE);
    }

    memset(&myAddress, 0, sizeof(myAddress));
    myAddress.sun_family = AF_UNIX;
    strncpy(myAddress.sun_path, AT_BINDER_SOCK_PATH, sizeof(myAddress.sun_path) - 1);

    if (-1 == bind(sockFd, (struct sockaddr *) &myAddress, sizeof(myAddress)))
    {
        LE_ERROR("binding socket failed: %m");
        goto exit_close_socket;
    }

    // Listen on the socket
    if (-1 == listen(sockFd, MAX_CLIENTS))
    {
        LE_ERROR("listening socket failed: %m");
        goto exit_close_socket;
    }

    le_fdMonitor_Create("atBinder-socket", sockFd, MonitorSocket, POLLIN);

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);

    return;

exit_close_socket:
    CloseWarn(sockFd);
    exit(EXIT_FAILURE);
}
