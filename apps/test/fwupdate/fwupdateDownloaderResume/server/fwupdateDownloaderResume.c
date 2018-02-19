//--------------------------------------------------------------------------------------------------
/**
 * @file fwupdateDownloaderResume.c
 *
 * fwupdate downloader with resume behavior implementation
 *
 * The firmware update process can be invoked remotely by sending an update package on
 * the TCP port 5001.
 *
 * For example by using netcat:
 * @verbatim
 * nc [-q 0] <target_ip> 5001 < <spkg_name.cwe>
 * @endverbatim
 *
 * @note The default port is 5001 but it can be changed in the source code.
 *
 * @note If the cwe file is not correct, a timeout of 900 seconds may occur when the firmware update
 * process is expecting incoming data.
 *
 * @note If no data are sent by the host to the firmware update process during more than 900 seconds,
 * a timeout will occur and the download will fail.
 *
 * @note If the download is interrupted before the end for any reason, you can resume it by relaunch
 *  the command.
 *
 * <HR>
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
#define FWUPDATE_SERVER_PORT 5001


//--------------------------------------------------------------------------------------------------
/**
 * Buffer size
 */
//--------------------------------------------------------------------------------------------------
#define BUF_SIZE 1024

//--------------------------------------------------------------------------------------------------
/**
 * This function checks the systems synchronization, and synchronized them if necessary
 *
 * @return
 *      - LE_OK if succeed
 *      - LE_FAULT if error
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CheckSystemState
(
    size_t *resumePositionPtr        ///< resume position pointer
)
{
    le_result_t result;
    bool isSystemGood;

    result = le_fwupdate_GetResumePosition(resumePositionPtr);
    if ((result == LE_OK) && (*resumePositionPtr != 0))
    {
        LE_INFO("resume download at position %d", *resumePositionPtr);
    }
    else
    {// no resume context found => do a normal download
        LE_INFO("normal download");

        if (resumePositionPtr)
        {
            *resumePositionPtr = 0;
        }

        result = le_fwupdate_IsSystemMarkedGood(&isSystemGood);
        if (result != LE_OK)
        {
            LE_ERROR("System state check failed. Error %s", LE_RESULT_TXT(result));
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
    bool connFdClosed = false;

    socklen_t addressLen = sizeof(clientAddress);

    LE_INFO("waiting connection ...");
    connFd = accept(fd, (struct sockaddr *)&clientAddress, &addressLen);

    if (connFd == -1)
    {
        LE_ERROR("accept error: %m");
    }
    else
    {
        size_t resumePosition;

        LE_INFO("Connected ...");

        result = CheckSystemState(&resumePosition);

        if (result == LE_OK)
        {
            if (resumePosition)
            {// we are doing a resume download

                LE_INFO("resumePosition = %d", resumePosition);

                while (resumePosition)
                {
                    ssize_t readCount;
                    uint32_t buf[BUF_SIZE];
                    size_t length = (resumePosition > BUF_SIZE) ? BUF_SIZE : resumePosition;

                    readCount = read(connFd, buf, length);
                    if (readCount == -1)
                    {
                        LE_ERROR("read error %m");
                        break;
                    }
                    else
                    {
                        if (readCount)
                        {
                            resumePosition -= readCount;
                        }
                        else
                        {
                            LE_INFO("end of file");
                            break;
                        }
                    }
                }
            }

            if (resumePosition)
            {// error
                LE_ERROR("end of file with resumePosition != 0 (%d)", resumePosition);
                le_fwupdate_InitDownload();
            }
            else
            {
                result = le_fwupdate_Download(connFd);
                connFdClosed = true; // le_fwupdate_Download closes connFd

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
                }
            }
        }
        else
        {
            LE_ERROR("Connection error %d", result);
        }

        if (connFdClosed == false)
        {
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
 * This function must be called to initialize the FW UPDATE DOWNLOADER RESUME module.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    struct sockaddr_in myAddress;
    int ret, optVal = 1;
    int sockFd;

    LE_INFO("FW UPDATE DOWNLOADER RESUME starts");

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
