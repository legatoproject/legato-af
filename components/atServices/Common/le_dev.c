/** @file le_dev.c
 *
 * Implementation of device access.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

// make sure we're using the gnu version of strerror_r
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pwd.h>
#include <grp.h>
#include "legato.h"
#include "interfaces.h"
#include "le_dev.h"

//--------------------------------------------------------------------------------------------------
/**
 * Default buffer size for device information and error messages
 */
//--------------------------------------------------------------------------------------------------
#define DSIZE   256

//--------------------------------------------------------------------------------------------------
/**
 * struct DevInfo contains useful information about the device in use
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int             fd;                 ///< file descriptor in use
    char            linkName[DSIZE];    ///< device full path
    char            fdSysPath[DSIZE];   ///< /proc/PID/fd/FD
    unsigned int    major;              ///< device's major number
    unsigned int    minor;              ///< device's minor number
    char            uName[DSIZE];       ///< user name
    char            gName[DSIZE];       ///< group name
    char            devInfoStr[DSIZE];  ///< formatted string for all info
}
DevInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * device information
 */
//--------------------------------------------------------------------------------------------------
static DevInfo_t DevInfo;

//--------------------------------------------------------------------------------------------------
/**
 * Get device information
 *
 * Warning: this function works only on *nix systems
 *
 * @return
        LE_OK       things went Ok and information is printed
 *      LE_FAULT    something wrong happened, check the logs
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetDeviceInformation
(
    void
)
{
    if(le_log_GetFilterLevel() == LE_LOG_DEBUG)
    {
        struct stat fdStats;
        struct passwd* passwd;
        struct group* group;
        char errMsg[DSIZE];

        memset(DevInfo.fdSysPath, 0, DSIZE);
        memset(DevInfo.linkName, 0, DSIZE);
        memset(errMsg, 0, DSIZE);
        memset(DevInfo.devInfoStr, 0, DSIZE);

        // build the path to fd
        snprintf(DevInfo.fdSysPath,
            DSIZE, "/proc/%d/fd/%d", getpid(), DevInfo.fd);

        // get device path
        if (readlink(DevInfo.fdSysPath, DevInfo.linkName, DSIZE) == -1) {
            LE_ERROR("readlink failed %s", strerror_r(errno, errMsg, DSIZE));
            return LE_FAULT;
        }
        // try to get device stats
        if (fstat(DevInfo.fd, &fdStats) == -1)
        {
            LE_ERROR("fstat failed %s", strerror_r(errno, errMsg, DSIZE));
            return LE_FAULT;
        }

        passwd = getpwuid(fdStats.st_uid);
        group = getgrgid(fdStats.st_gid);

        DevInfo.major = major(fdStats.st_rdev);
        DevInfo.minor = minor(fdStats.st_rdev);
        snprintf(DevInfo.uName, DSIZE, "%s", passwd->pw_name);
        snprintf(DevInfo.gName, DSIZE, "%s", group->gr_name);
        snprintf(DevInfo.devInfoStr, DSIZE, "%s, %s [%u, %u], (u: %s, g: %s)",
            DevInfo.fdSysPath,
            DevInfo.linkName,
            DevInfo.major,
            DevInfo.minor,
            DevInfo.uName,
            DevInfo.gName);

        return LE_OK;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to print a buffer byte by byte
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintBuffer
(
    int32_t   fd,            ///< The file descriptor
    uint8_t*  bufferPtr,     ///< the buffer to print
    uint32_t  bufferSize     ///< Number of element to print
)
{
    if(le_log_GetFilterLevel() == LE_LOG_DEBUG)
    {
        uint32_t i;
        char dev[DSIZE];

        DevInfo.fd = fd;

        if (GetDeviceInformation())
        {
            snprintf(dev, DSIZE, "%d", fd);
        }
        else
        {
            snprintf(dev, DSIZE, "%s", DevInfo.linkName);
        }

        for(i=0; i<bufferSize; i++)
        {
            if (bufferPtr[i] == '\r' )
            {
                LE_DEBUG("'%s' -> [%d] '0x%.2x' '%s'",
                    dev, i, bufferPtr[i], "CR");
            }
            else if (bufferPtr[i] == '\n')
            {
                LE_DEBUG("'%s' -> [%d] '0x%.2x' '%s'",
                    dev, i, bufferPtr[i], "LF");
            }
            else if (bufferPtr[i] == 0x1A)
            {
                LE_DEBUG("'%s' -> [%d] '0x%.2x' '%s'",
                    dev, i, bufferPtr[i], "CTRL+Z");
            }
            else
            {
                LE_DEBUG("'%s' -> [%d] '0x%.2x' '%c'",
                    dev, i, bufferPtr[i], bufferPtr[i]);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the function in non blocking mode
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetSocketNonBlocking
(
    int fd
)
{
    int flags;
    char errMsg[DSIZE];

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        memset(errMsg, 0, DSIZE);
        LE_ERROR("fcntl failed, %s",
            strerror_r(errno, errMsg, DSIZE));
        return LE_FAULT;
    }

    if (flags & O_NONBLOCK)
    {
        return LE_OK;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0)
    {
        memset(errMsg, 0, DSIZE);
        LE_ERROR("fcntl failed, %s",
            strerror_r(errno, errMsg, DSIZE));
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called when we want to read on device (or port)
 *
 * @return byte number read
 */
//--------------------------------------------------------------------------------------------------
int32_t le_dev_Read
(
    Device_t*  devicePtr,    ///< device pointer
    uint8_t*   rxDataPtr,    ///< Buffer where to read
    uint32_t   size          ///< size of buffer
)
{
    int32_t status = 1;
    char errMsg[DSIZE];

    DevInfo.fd = devicePtr->fd;
    if (!GetDeviceInformation())
    {
        LE_INFO("%s", DevInfo.devInfoStr);
    }

    status = read(devicePtr->fd, rxDataPtr, size);

    if (status < 0)
    {
        memset(errMsg, 0 , 256);
        LE_ERROR("read error: %s", strerror_r(errno, errMsg, DSIZE));
        return 0;
    }

    LE_DEBUG("Read (%d) on %d", status, devicePtr->fd);

    PrintBuffer(devicePtr->fd, rxDataPtr, status);

    return status;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to write on device (or port)
 *
 * @return written byte number
 *
 */
//--------------------------------------------------------------------------------------------------
int32_t le_dev_Write
(
    Device_t*   devicePtr,    ///< device pointer
    uint8_t*    txDataPtr,    ///< Buffer to write
    uint32_t    size          ///< size of buffer
)
{
    int32_t amount = 0;
    size_t currentSize;
    size_t sizeToWrite;
    ssize_t sizeWritten;
    char errMsg[DSIZE];

    DevInfo.fd = devicePtr->fd;
    if (!GetDeviceInformation())
    {
        LE_INFO("%s", DevInfo.devInfoStr);
    }

    LE_FATAL_IF(devicePtr->fd==-1,"Write Handle error\n");

    for(currentSize = 0; currentSize < size;)
    {
        sizeToWrite = size - currentSize;

        sizeWritten =
            write(devicePtr->fd, &txDataPtr[currentSize], sizeToWrite);

        if (sizeWritten < 0)
        {
            if ((errno != EINTR) && (errno != EAGAIN))
            {
                memset(errMsg, 0, DSIZE);
                LE_ERROR("Cannot write on fd: %s",
                    strerror_r(errno, errMsg, DSIZE));
                return currentSize;
            }
        }
        else
        {
            currentSize += sizeWritten;
        }
    }

    if(currentSize > 0)
    {
        size  -= currentSize;
        amount  += currentSize;
    }

    LE_DEBUG("write (%d) on %d", amount, devicePtr->fd);

    PrintBuffer(devicePtr->fd,txDataPtr,amount);

    return currentSize;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to monitor the specified file descriptor
 * in the calling thread event loop.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dev_AddFdMonitoring
(
    Device_t*                   devicePtr,    ///< device pointer
    le_fdMonitor_HandlerFunc_t  handlerFunc, ///< [in] Handler function.
    void*                       contextPtr
)
{
    char monitorName[64];
    le_fdMonitor_Ref_t fdMonitorRef;

    DevInfo.fd = devicePtr->fd;
    if (!GetDeviceInformation())
    {
        LE_INFO("%s", DevInfo.devInfoStr);
    }

    if (devicePtr->fdMonitor)
    {
        LE_WARN("Interface %d already started",devicePtr->fd);
        return LE_FAULT;
    }

    // Set the fd in non blocking
    if (SetSocketNonBlocking( devicePtr->fd ) != LE_OK)
    {
        return LE_FAULT;
    }

    // Create a File Descriptor Monitor object for the file descriptor.
    snprintf(monitorName,
             sizeof(monitorName),
             "Monitor-%d",
             devicePtr->fd);

    fdMonitorRef = le_fdMonitor_Create(monitorName,
                                       devicePtr->fd,
                                       handlerFunc,
                                       POLLIN | POLLRDHUP);

    devicePtr->fdMonitor = fdMonitorRef;

    le_fdMonitor_SetContextPtr(fdMonitorRef, contextPtr);

    if (le_log_GetFilterLevel() == LE_LOG_DEBUG)
    {
        char threadName[25];
        le_thread_GetName(le_thread_GetCurrent(), threadName, 25);
        LE_DEBUG("Resume %s with fd(%d)(%p) [%s]",
                 threadName,
                 devicePtr->fd,
                 devicePtr->fdMonitor,
                 monitorName
                );
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove the file descriptor
 * monitoring from the event loop.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_RemoveFdMonitoring
(
    Device_t*   devicePtr
)
{
    DevInfo.fd = devicePtr->fd;
    if (!GetDeviceInformation())
    {
        LE_INFO("%s", DevInfo.devInfoStr);
    }

    if (devicePtr->fdMonitor)
    {
        le_fdMonitor_Delete(devicePtr->fdMonitor);
    }
}
