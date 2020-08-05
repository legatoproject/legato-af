/** @file le_dev.c
 *
 * Implementation of device access.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_dev.h"
#include "le_fd.h"

#if LE_CONFIG_LINUX
#   include <pwd.h>
#   include <grp.h>
#endif

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

#if LE_DEBUG_ENABLED
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

#if LE_CONFIG_LINUX
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
            LE_ERROR("readlink failed %s", LE_ERRNO_TXT(errno));
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
            LE_ERROR("fstat failed %s", LE_ERRNO_TXT(errno));
            return LE_FAULT;
        }

        passwd = getpwuid(fdStats.st_uid);
        group = getgrgid(fdStats.st_gid);

        if ((NULL == passwd) || (NULL == group))
        {
            LE_ERROR("Get passwd and group failed %s", LE_ERRNO_TXT(errno));
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
#endif // CONFIG_LINUX
#endif // LE_DEBUG_ENABLED

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
#if LE_DEBUG_ENABLED
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

#if LE_CONFIG_LINUX
        if (GetDeviceInformation())
        {
            LE_DEBUG("'%d' -> %s", fd, string);
        }
        else
#endif
        {
            LE_DEBUG("'%s' -> %s", DevInfo.linkName, string);
        }
    }
#endif
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

#if LE_DEBUG_ENABLED
    DevInfo.fd = devicePtr->fd;

#if LE_CONFIG_LINUX
    if (!GetDeviceInformation())
    {
        LE_INFO("%s", DevInfo.devInfoStr);
    }
#endif
#endif

    count = le_fd_Read(devicePtr->fd, rxDataPtr, size);
    if (-1 == count)
    {
        LE_ERROR("read error: %s", LE_ERRNO_TXT(errno));
        return 0;
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

#if LE_DEBUG_ENABLED
    DevInfo.fd = devicePtr->fd;
#if LE_CONFIG_LINUX
    if (!GetDeviceInformation())
    {
        LE_DEBUG("%s", DevInfo.devInfoStr);
    }
#endif
#endif

    LE_FATAL_IF(devicePtr->fd==-1,"Write Handle error\n");

    for(currentSize = 0; currentSize < size;)
    {
        sizeToWrite = size - currentSize;

        sizeWritten =
            le_fd_Write(devicePtr->fd, &txDataPtr[currentSize], sizeToWrite);

        if (sizeWritten < 0)
        {
            if ((errno != EINTR) && (errno != EAGAIN))
            {
                LE_ERROR("Cannot write on fd: %s", LE_ERRNO_TXT(errno));
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
        amount  += currentSize;
    }

    PrintBuffer(devicePtr->fd,txDataPtr,amount);

    return currentSize;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to monitor the specified file descriptor
 * in the calling thread event loop.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dev_EnableFdMonitoring
(
    Device_t                    *devicePtr,     ///< Device pointer.
    le_fdMonitor_HandlerFunc_t   handlerFunc,   ///< Handler function.
    void                        *contextPtr,    ///< Context data.
    short                        events         ///< Events to monitor.
)
{
    char monitorName[64];

#if LE_DEBUG_ENABLED
    DevInfo.fd = devicePtr->fd;

#if LE_CONFIG_LINUX
    if (!GetDeviceInformation())
    {
        LE_DEBUG("%s", DevInfo.devInfoStr);
    }
#endif
#endif

    if (devicePtr->fdMonitor != NULL)
    {
        le_fdMonitor_Enable(devicePtr->fdMonitor, events);
    }
    else
    {
        // Create a File Descriptor Monitor object for the file descriptor.
        snprintf(monitorName,
                 sizeof(monitorName),
                 "Monitor-%"PRIi32"",
                 devicePtr->fd);

        devicePtr->fdMonitor = le_fdMonitor_Create(monitorName,
                                           devicePtr->fd,
                                           handlerFunc,
                                           events);
        if (devicePtr->fdMonitor == NULL)
        {
            return LE_FAULT;
        }
        le_fdMonitor_SetContextPtr(devicePtr->fdMonitor, contextPtr);

        if (le_log_GetFilterLevel() == LE_LOG_DEBUG)
        {
            char threadName[25];
            le_thread_GetName(le_thread_GetCurrent(), threadName, 25);
            LE_DEBUG("Resume %s with fd(%"PRIi32")(%p) [%s]",
                     threadName,
                     devicePtr->fd,
                     devicePtr->fdMonitor,
                     monitorName
                    );
        }
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
void le_dev_DeleteFdMonitoring
(
    Device_t*   devicePtr
)
{
#if LE_DEBUG_ENABLED
    DevInfo.fd = devicePtr->fd;
#if LE_CONFIG_LINUX
    if (!GetDeviceInformation())
    {
        LE_DEBUG("%s", DevInfo.devInfoStr);
    }
#endif
#endif

    if (devicePtr->fdMonitor)
    {
        le_fdMonitor_Delete(devicePtr->fdMonitor);
    }

    devicePtr->fdMonitor = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable monitoring of the device.  Monitoring can be resumed with le_dev_EnableFdMonitoring().
 */
//--------------------------------------------------------------------------------------------------
void le_dev_DisableFdMonitoring
(
    Device_t    *devicePtr, ///< Device to stop monitoring.
    short        events     ///< Events to stop monitoring.
)
{
    if ((devicePtr != NULL) && (devicePtr->fdMonitor != NULL))
    {
        le_fdMonitor_Disable(devicePtr->fdMonitor, events);
    }
}
