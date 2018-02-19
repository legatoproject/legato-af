//--------------------------------------------------------------------------------------------------
/**
 * @file fwupdateDownloader.c
 *
 * fwupdate downloader implementation
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include <sys/socket.h>
#include <netinet/in.h>


//--------------------------------------------------------------------------------------------------
/**
 * Server TCP port.
 *
 * @note this is an arbitrary value and can be changed as required
 */
//--------------------------------------------------------------------------------------------------
#define FWUPDATE_SERVER_PORT 5000

//--------------------------------------------------------------------------------------------------
/**
 * This function checks if the system is marked good, and marks them as good if necessary
 *
 * @return
 *      - LE_OK if succeed
 *      - LE_FAULT if error
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CheckSystemState
(
    void
)
{
    le_result_t result;
    bool isSystemGood;

    result = le_fwupdate_IsSystemMarkedGood(&isSystemGood);
    if (result != LE_OK)
    {
        LE_ERROR("Get system state failed. Error %s", LE_RESULT_TXT(result));
        return LE_FAULT;
    }
    if (false == isSystemGood)
    {
        result = le_fwupdate_MarkGood();
        if (result != LE_OK)
        {
            LE_ERROR("Mark good operation failed. Error %s", LE_RESULT_TXT(result));
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function wait for a connection and perform the download of the image when
 * a connection is done
 *
 */
//--------------------------------------------------------------------------------------------------
static void SocketEventHandler
(
    int fd
)
{
    struct sockaddr_in clientAddress;
    le_result_t result;
    int connFd;

    socklen_t addressLen = sizeof(clientAddress);

    LE_INFO("waiting connection ...");
    connFd = accept(fd, (struct sockaddr *)&clientAddress, &addressLen);

    if (connFd == -1)
    {
        LE_ERROR("accept error: %m");
    }
    else
    {
        LE_INFO("Connected ...");

        result = CheckSystemState();

        if (result == LE_OK)
        {
            result = le_fwupdate_Download(connFd);

            LE_INFO("Download result=%s", LE_RESULT_TXT(result));
            if (result == LE_OK)
            {
                le_fwupdate_InstallAndMarkGood();
                // if this function returns so there is an error => do a SYNC
                LE_ERROR("Swap And Sync failed -> Sync");
                result = le_fwupdate_MarkGood();
                if (result != LE_OK)
                {
                    LE_ERROR("SYNC failed");
                }
                // TODO send an error message to the host
            }
        }
        else
        {
            LE_ERROR("Connection error %d", result);
            close(connFd);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function call the appropriate handler on event reception
 *
 */
//--------------------------------------------------------------------------------------------------
static void SocketListenerHandler
(
    int fd,
    short events
)
{
    if (events & POLLERR)
    {
        LE_ERROR("socket Error");
    }

    if (events & POLLIN)
    {
        SocketEventHandler(fd);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the FW UPDATE DOWNLOADER module.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    struct sockaddr_in myAddress;
    int ret, optVal = 1;
    int sockFd;

    LE_INFO("FW UPDATE DOWNLOADER starts");

    // Create the socket
    sockFd = socket (AF_INET, SOCK_STREAM, 0);

    if (sockFd < 0)
    {
        LE_ERROR("creating socket failed: %m");
        return;
    }

    // set socket option
    // we use SO_REUSEADDR to be able to accept several clients without closing the socket
    ret = setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));
    if (ret)
    {
        LE_ERROR("error setting socket option %m");
        close(sockFd);
        return;
    }

    memset(&myAddress, 0, sizeof(myAddress));

    myAddress.sin_port = htons(FWUPDATE_SERVER_PORT);
    myAddress.sin_family = AF_INET;
    myAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind server - socket
    ret = bind(sockFd, (struct sockaddr_in *)&myAddress, sizeof(myAddress));
    if (ret)
    {
        LE_ERROR("bind failed %m");
        close(sockFd);
        return;
    }

    // Listen on the socket
    ret = listen(sockFd, 1);
    if (ret)
    {
        LE_ERROR("listen error: %m");
        close(sockFd);
        return;
    }

    le_fdMonitor_Create("fwDownloaderMonitor", sockFd, SocketListenerHandler, POLLIN);
}
