/** @file le_dev.c
 *
 * Implementation of device access.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_dev.h"
#include <pwd.h>
#include <grp.h>


//--------------------------------------------------------------------------------------------------
/**
 * Default buffer size for device information and error messages
 */
//--------------------------------------------------------------------------------------------------
#define DSIZE   256

//--------------------------------------------------------------------------------------------------
/**
 * String size for the buffer that contains a summary of all the device information available
 */
//--------------------------------------------------------------------------------------------------
#define DSIZE_INFO_STR   1600

//--------------------------------------------------------------------------------------------------
/**
 * struct DevInfo contains useful information about the device in use
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int             fd;                         ///< file descriptor in use
    char            linkName[DSIZE];            ///< device full path
    char            fdSysPath[DSIZE];           ///< /proc/PID/fd/FD
    unsigned int    major;                      ///< device's major number
    unsigned int    minor;                      ///< device's minor number
    char            uName[DSIZE];               ///< user name
    char            gName[DSIZE];               ///< group name
    char            devInfoStr[DSIZE_INFO_STR]; ///< formatted string for all info
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
 * Return a description string of err
 *
 */
//--------------------------------------------------------------------------------------------------
static char* StrError
(
    int err
)
{
    static char errMsg[DSIZE];

#ifdef __USE_GNU
    snprintf(errMsg, DSIZE, "%s", strerror_r(err, errMsg, DSIZE));
#else /* XSI-compliant */
    strerror_r(err, errMsg, sizeof(errMsg));
#endif
    return errMsg;
}

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

        // Note: Better use sizeof() operator instead of any macro(i.e. DSIZE), as it is compile
        //       time operator and gives the code better flexibility.

        memset(DevInfo.fdSysPath, 0, sizeof(DevInfo.fdSysPath));
        memset(DevInfo.linkName, 0, sizeof(DevInfo.linkName));
        memset(DevInfo.devInfoStr, 0, sizeof(DevInfo.devInfoStr));

        // build the path to fd
        snprintf(DevInfo.fdSysPath,
            sizeof(DevInfo.fdSysPath), "/proc/%d/fd/%d", getpid(), DevInfo.fd);

        int len = readlink(DevInfo.fdSysPath, DevInfo.linkName, sizeof(DevInfo.fdSysPath));
        // get device path
        if (len < 0) {
            LE_ERROR("readlink failed %s", StrError(errno));
            return LE_FAULT;
        }
        else if (len >= sizeof(DevInfo.fdSysPath))
        {
            LE_ERROR("Too long path. Max allowed: %zd", sizeof(DevInfo.fdSysPath)-1);
            return LE_FAULT;
        }

        // try to get device stats
        if (fstat(DevInfo.fd, &fdStats) == -1)
        {
            LE_ERROR("fstat failed %s", StrError(errno));
            return LE_FAULT;
        }

        passwd = getpwuid(fdStats.st_uid);
        group = getgrgid(fdStats.st_gid);

        if ((NULL == passwd) || (NULL == group))
        {
            LE_ERROR("Get passwd and group failed %s", StrError(errno));
            return LE_FAULT;
        }

        DevInfo.major = major(fdStats.st_rdev);
        DevInfo.minor = minor(fdStats.st_rdev);
        snprintf(DevInfo.uName, sizeof(DevInfo.uName), "%s", passwd->pw_name);
        snprintf(DevInfo.gName, sizeof(DevInfo.gName), "%s", group->gr_name);
        snprintf(DevInfo.devInfoStr, sizeof(DevInfo.devInfoStr), "%s, %s [%u, %u], (u: %s, g: %s)",
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
        uint32_t dataLen = 1;
        DevInfo.fd = fd;

        for(i=0;i<bufferSize;i++)
        {
            if (bufferPtr[i] == '\r' )
            {
                dataLen+=4;
            }
            else if (bufferPtr[i] == '\n')
            {
                dataLen+=4;
            }
            else if (bufferPtr[i] == 0x1A)
            {
                dataLen+=8;
            }
            else
            {
                dataLen+=1;
            }
        }

        char string[dataLen];
        memset(string, 0, dataLen);

        for(i=0;i<bufferSize;i++)
        {
            if (bufferPtr[i] == '\r' )
            {
                snprintf(string+strlen(string), dataLen-strlen(string), "<CR>");
            }
            else if (bufferPtr[i] == '\n')
            {
                snprintf(string+strlen(string), dataLen-strlen(string), "<LF>");
            }
            else if (bufferPtr[i] == 0x1A)
            {
                snprintf(string+strlen(string), dataLen-strlen(string), "<CTRL+Z>");
            }
            else
            {
                snprintf(string+strlen(string), dataLen-strlen(string), "%c", bufferPtr[i]);
            }
        }

        if (GetDeviceInformation())
        {
            LE_DEBUG("'%d' -> %s", fd, string);
        }
        else
        {
            LE_DEBUG("'%s' -> %s", DevInfo.linkName, string);
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

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        LE_ERROR("fcntl failed, %s", StrError(errno));
        return LE_FAULT;
    }

    if (flags & O_NONBLOCK)
    {
        return LE_OK;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0)
    {
        LE_ERROR("fcntl failed, %s", StrError(errno));
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
ssize_t le_dev_Read
(
    Device_t*   devicePtr,    ///< device pointer
    uint8_t*    rxDataPtr,    ///< Buffer where to read
    size_t      size          ///< size of buffer
)
{
    ssize_t count;

    if (NULL == devicePtr)
    {
        LE_ERROR("devicePtr is NULL!");
        return -1;
    }

    if (NULL == rxDataPtr)
    {
        LE_ERROR("rxDataPtr is NULL!");
        return -1;
    }

    if (0 == size)
    {
        LE_ERROR("size is 0!");
        return -1;
    }

    DevInfo.fd = devicePtr->fd;
    if (!GetDeviceInformation())
    {
        LE_INFO("%s", DevInfo.devInfoStr);
    }

    count = read(devicePtr->fd, rxDataPtr, size);
    if (-1 == count)
    {
        LE_ERROR("read error: %s", StrError(errno));
        return -1;
    }

    PrintBuffer(devicePtr->fd, rxDataPtr, count);

    return count;
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

    DevInfo.fd = devicePtr->fd;
    if (!GetDeviceInformation())
    {
        LE_DEBUG("%s", DevInfo.devInfoStr);
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
                LE_ERROR("Cannot write on fd: %s", StrError(errno));
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
        LE_DEBUG("%s", DevInfo.devInfoStr);
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
                                       POLLIN | POLLPRI | POLLRDHUP);

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
        LE_DEBUG("%s", DevInfo.devInfoStr);
    }

    if (devicePtr->fdMonitor)
    {
        le_fdMonitor_Delete(devicePtr->fdMonitor);
    }

    devicePtr->fdMonitor = NULL;
}
