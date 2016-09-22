/** @file le_dev.c
 *
 * Implementation of device access.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_dev.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to print a buffer byte by byte
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintBuffer
(
    int32_t   fd,            ///< The file descriptor
    uint8_t  *bufferPtr,     ///< the buffer to print
    uint32_t  bufferSize     ///< Number of element to print
)
{
    uint32_t i;
    for(i=0;i<bufferSize;i++)
    {
        if (bufferPtr[i] == '\r' )
        {
            LE_DEBUG("'%d' -> [%d] '0x%.2x' '%s'",fd,i,bufferPtr[i],"CR");
        }
        else if (bufferPtr[i] == '\n')
        {
            LE_DEBUG("'%d' -> [%d] '0x%.2x' '%s'",fd,i,bufferPtr[i],"LF");
        }
        else if (bufferPtr[i] == 0x1A)
        {
            LE_DEBUG("'%d' -> [%d] '0x%.2x' '%s'",fd,i,bufferPtr[i],"CTRL+Z");
        }
        else
        {
            LE_DEBUG("'%d' -> [%d] '0x%.2x' '%c'",fd,i,bufferPtr[i],bufferPtr[i]);
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
      LE_ERROR("fcntl error errno %d, %s", errno, strerror(errno));
      return LE_FAULT;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0)
    {
        LE_ERROR("fcntl error errno %d, %s", errno, strerror(errno));
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
    Device_t  *devicePtr,    ///< device pointer
    uint8_t   *rxDataPtr,    ///< Buffer where to read
    uint32_t   size          ///< size of buffer
)
{
    int32_t status=1;
    int32_t amount = 0;

    while ((status > 0) && size)
    {
        status = read(devicePtr->fd, rxDataPtr, size);

        if (status > 0)
        {
            size      -= status;
            rxDataPtr += status;
            amount    += status;
        }
        else if (status < 0)
        {
            if (errno == EINTR)
            {
                // read again
                status = 1;
            }
            else
            {
                LE_ERROR("read error, errno %d, %s", errno, strerror(errno));
                return amount;
            }
        }
    }

    LE_DEBUG("Read (%d) on %d",
             amount,devicePtr->fd);

    PrintBuffer(devicePtr->fd,
                rxDataPtr-amount,
                amount);

    return amount;
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
    Device_t *devicePtr,    ///< device pointer
    uint8_t  *txDataPtr,    ///< Buffer to write
    uint32_t  size          ///< size of buffer
)
{
    int32_t amount = 0;

    size_t currentSize;
    size_t sizeToWrite;
    ssize_t sizeWritten;

    LE_FATAL_IF(devicePtr->fd==-1,"Write Handle error\n");

    for(currentSize = 0; currentSize < size;)
    {
        sizeToWrite = size - currentSize;

        sizeWritten = write(devicePtr->fd, &txDataPtr[currentSize], sizeToWrite);

        if (sizeWritten < 0)
        {
            if ((errno != EINTR) && (errno != EAGAIN))
            {
                LE_ERROR("Cannot write on uart: errno=%d, %s", errno, strerror(errno));
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

    LE_DEBUG("write (%d) on %d",
             amount,devicePtr->fd);

    PrintBuffer(devicePtr->fd,txDataPtr,amount);

    return currentSize;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to monitor the specified file descriptor in the calling thread event
 * loop.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dev_AddFdMonitoring
(
    Device_t *devicePtr,    ///< device pointer
    le_fdMonitor_HandlerFunc_t handlerFunc, ///< [in] Handler function.
    void* contextPtr
)
{
    char monitorName[64];
    le_fdMonitor_Ref_t fdMonitorRef;

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

    // Create a File Descriptor Monitor object for the serial port's file descriptor.
    snprintf(monitorName,
             sizeof(monitorName),
             "Monitor-%d",
             devicePtr->fd);

    fdMonitorRef = le_fdMonitor_Create(monitorName,
                                       devicePtr->fd,
                                       handlerFunc,
                                       POLLIN);

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
 * This function must be called to remove the file descriptor monitoring from the event loop.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dev_RemoveFdMonitoring
(
    Device_t *devicePtr
)
{
    if (devicePtr->fdMonitor)
    {
        le_fdMonitor_Delete(devicePtr->fdMonitor);
    }

    return LE_OK;
}
