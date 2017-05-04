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


//--------------------------------------------------------------------------------------------------
/**
 * Socket path
 */
//--------------------------------------------------------------------------------------------------
#define AT_BINDER_SOCK_PATH "/tmp/atBinder"

//------------------------------------------------------------------------------
/**
 * Initialize the atBinder application.
 *
 */
//------------------------------------------------------------------------------
COMPONENT_INIT
{
    struct sockaddr_un myAddress;

    LE_INFO("atBinder starts");

    if (unlink(AT_BINDER_SOCK_PATH) < 0)
    {
        LE_ERROR("unlink socket failed: %m");
        goto exit;
    }

    // Create the socket
    int sockFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockFd < 0)
    {
        LE_ERROR("creating socket failed: %m");
        goto exit;
    }

    memset(&myAddress, 0, sizeof(myAddress));
    myAddress.sun_family = AF_UNIX;
    strncpy(myAddress.sun_path, AT_BINDER_SOCK_PATH, sizeof(myAddress.sun_path) - 1);

    if (bind(sockFd, (struct sockaddr *) &myAddress, sizeof(myAddress)) < 0)
    {
        LE_ERROR("binding socket failed: %m");
        goto exit_close_fd;
    }

    // Listen on the socket
    if (listen(sockFd, 1) < 0)
    {
        LE_ERROR("listening socket failed: %m");
        goto exit_close_fd;
    }

    // Socket's file descriptor used for AT commands monitoring
    int atSockFd = accept(sockFd, NULL, NULL);
    if (atSockFd < 0)
    {
        LE_ERROR("accepting socket failed: %m");
        goto exit_close_fd;
    }

    if (NULL == le_atServer_Open(dup(atSockFd)))
    {
        LE_ERROR("Cannot open the device!");
        goto exit_close_fd;
    }
    else
    {
        LE_INFO("atBinder is ready");
        return;
    }

exit_close_fd:
    close(sockFd);
exit:
    exit(EXIT_FAILURE);
}
