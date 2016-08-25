/** @file le_dev.c
 *
 * Implementation of device access.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_dev.h"
#include <termios.h>

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to print a buffer byte by byte
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintBuffer
(
    char     *namePtr,       ///< buffer name
    uint8_t  *bufferPtr,     ///< the buffer to print
    uint32_t  bufferSize     ///< Number of element to print
)
{
    uint32_t i;
    for(i=0;i<bufferSize;i++)
    {
        if (bufferPtr[i] == '\r' )
        {
            LE_DEBUG("'%s' -> [%d] '0x%.2x' '%s'",(namePtr)?namePtr:"no name",i,bufferPtr[i],"CR");
        }
        else if (bufferPtr[i] == '\n')
        {
            LE_DEBUG("'%s' -> [%d] '0x%.2x' '%s'",(namePtr)?namePtr:"no name",i,bufferPtr[i],"LF");
        }
        else if (bufferPtr[i] == 0x1A)
        {
            LE_DEBUG("'%s' -> [%d] '0x%.2x' '%s'",(namePtr)?namePtr:"no name",i,bufferPtr[i],
                                                                                         "CTRL+Z");
        }
        else
        {
            LE_DEBUG("'%s' -> [%d] '0x%.2x' '%c'",(namePtr)?namePtr:"no name",i,bufferPtr[i],
                                                                                bufferPtr[i]);
        }
    }
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

    while (status>0)
    {
        status = read(devicePtr->handle, rxDataPtr, size);

        if(status > 0)
        {
            size      -= status;
            rxDataPtr += status;
            amount    += status;
        }
    }

    LE_DEBUG("%s -> Read (%d) on %d",
             devicePtr->path,
             amount,devicePtr->handle);

    PrintBuffer(devicePtr->path,
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

    LE_FATAL_IF(devicePtr->handle==-1,"Write Handle error\n");

    for(currentSize = 0; currentSize < size;)
    {
        sizeToWrite = size - currentSize;

        sizeWritten = write(devicePtr->handle, &txDataPtr[currentSize], sizeToWrite);

        if (sizeWritten < 0)
        {
            if ((errno != EINTR) && (errno != EAGAIN))
            {
                LE_ERROR("Cannot write on uart: %s",strerror(errno));
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

    LE_DEBUG("%s -> write (%d) on %d",
             devicePtr->path,
             amount,devicePtr->handle);

    PrintBuffer(devicePtr->path,txDataPtr,amount);

    return currentSize;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to open a device (or port)
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dev_Open
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
        LE_WARN("Interface %s already started",devicePtr->path);
        return LE_FAULT;
    }

    devicePtr->handle = open(devicePtr->path, O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (devicePtr->handle==-1)
    {
        LE_ERROR("Open device failed");
        return LE_FAULT;
    }

    struct termios term;
    bzero(&term, sizeof(term));

     // Default config
    tcgetattr(devicePtr->handle, &term);
    cfmakeraw(&term);
    term.c_oflag &= ~OCRNL;
    term.c_oflag &= ~ONLCR;
    term.c_oflag &= ~OPOST;
    tcsetattr(devicePtr->handle, TCSANOW, &term);
    tcflush(devicePtr->handle, TCIOFLUSH);

    // Create a File Descriptor Monitor object for the serial port's file descriptor.
    snprintf(monitorName,
             sizeof(monitorName),
             "Monitor-%d",
             devicePtr->handle);

    fdMonitorRef = le_fdMonitor_Create(monitorName,
                                       devicePtr->handle,
                                       handlerFunc,
                                       POLLIN);

    devicePtr->fdMonitor = fdMonitorRef;

    le_fdMonitor_SetContextPtr(fdMonitorRef, contextPtr);

    if (le_log_GetFilterLevel() == LE_LOG_DEBUG)
    {
        char threadName[25];
        le_thread_GetName(le_thread_GetCurrent(), threadName, 25);
        LE_DEBUG("Resume %s with handle(%d)(%p) [%s]",
                 threadName,
                 devicePtr->handle,
                 devicePtr->fdMonitor,
                 monitorName
                );
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to close a device (or port)
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dev_Close
(
    Device_t *devicePtr
)
{
    int closeRetVal;

    do
    {
        closeRetVal = close(devicePtr->handle);
    }
    while ((closeRetVal == -1) && (errno == EINTR));

    if (devicePtr->fdMonitor)
    {
        le_fdMonitor_Delete(devicePtr->fdMonitor);
    }

    return LE_OK;
}
