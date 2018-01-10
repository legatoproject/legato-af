/**
 * This module implements the stub code for atServer API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */


#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Device pool size.
 */
//--------------------------------------------------------------------------------------------------
#define DEVICE_POOL_SIZE 2

//--------------------------------------------------------------------------------------------------
/**
 * Max length for error string.
 */
//--------------------------------------------------------------------------------------------------
#define ERR_MSG_MAX 256

//--------------------------------------------------------------------------------------------------
/**
 * String size for the buffer that contains a summary of all the device information available.
 */
//--------------------------------------------------------------------------------------------------
#define DSIZE_INFO_STR 1600

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of AT command request/response.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_LEN_CMD 100

//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of device monitor name.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_LEN_MONITOR_NAME 64

//--------------------------------------------------------------------------------------------------
/**
 * AT server device reference to send the return value from le_atServer_Open() API.
 */
//--------------------------------------------------------------------------------------------------
#define AT_SERVER_DEVICE_REFERENCE 0x12345678


//--------------------------------------------------------------------------------------------------
/**
 * Device structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct Device
{
    int32_t            fd;         ///< The file descriptor.
    le_fdMonitor_Ref_t fdMonitor;  ///< fd event monitor associated to Handle.
}
Device_t;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for device context.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t DevicesPool;

//--------------------------------------------------------------------------------------------------
/**
 * AT command buffer.
 */
//--------------------------------------------------------------------------------------------------
static uint8_t Cmd[MAX_LEN_CMD];

//--------------------------------------------------------------------------------------------------
/**
 * AT server device reference.
 */
//--------------------------------------------------------------------------------------------------
static le_atServer_DeviceRef_t AtServerDevRef = (le_atServer_DeviceRef_t)AT_SERVER_DEVICE_REFERENCE;

//--------------------------------------------------------------------------------------------------
/**
 * This function creates the fdMonitor for file descriptor
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dev_AddFdMonitoring
(
    Device_t*                   devicePtr,    ///< [IN] Device pointer
    le_fdMonitor_HandlerFunc_t  handlerFunc,  ///< [IN] Handler function
    void*                       contextPtr    ///< [IN] Context pointer
)
{
    char monitorName[MAX_LEN_MONITOR_NAME];
    le_fdMonitor_Ref_t fdMonitorRef;

    // Create a File Descriptor Monitor object for the file descriptor.
    snprintf(monitorName, sizeof(monitorName), "Monitor-%d", devicePtr->fd);

    fdMonitorRef = le_fdMonitor_Create(monitorName, devicePtr->fd, handlerFunc,
                                       POLLIN | POLLPRI | POLLRDHUP);

    le_fdMonitor_SetContextPtr(fdMonitorRef, contextPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove the file descriptor monitoring from the event loop.
 */
//--------------------------------------------------------------------------------------------------
void le_dev_RemoveFdMonitoring
(
    Device_t* devicePtr    ///< [IN] Device pointer
)
{
    if (NULL == devicePtr)
    {
        LE_ERROR("devicePtr is NULL!");
        return;
    }

    if (devicePtr->fdMonitor)
    {
        le_fdMonitor_Delete(devicePtr->fdMonitor);
    }

    devicePtr->fdMonitor = NULL;
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
    Device_t*   devicePtr,    ///< [IN] Device pointer
    uint8_t*    rxDataPtr,    ///< [IN] Buffer where to read
    size_t      size          ///< [IN] Size of buffer
)
{
    ssize_t count;

    count = read(devicePtr->fd, rxDataPtr, (size - 1));
    if (count == -1)
    {
        LE_ERROR("read error: %s", strerror(errno));
        return 0;
    }

    *(char *)(rxDataPtr+count) = '\0';
    return count;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to write on device (or port)
 *
 * @return written byte number
 */
//--------------------------------------------------------------------------------------------------
int32_t le_dev_Write
(
    Device_t*   devicePtr,    ///< [IN] Device pointer
    uint8_t*    txDataPtr,    ///< [IN] Buffer to write
    uint32_t    size          ///< [IN] Size of buffer
)
{
    size_t currentSize;
    size_t sizeToWrite;
    ssize_t sizeWritten;

    for(currentSize = 0; currentSize < size;)
    {
        sizeToWrite = size - currentSize;

        LE_INFO("bytes to be write is %s", txDataPtr);
        sizeWritten =
            write(devicePtr->fd, &txDataPtr[currentSize], sizeToWrite);

        if (sizeWritten < 0)
        {
            if ((EINTR != errno) && (EAGAIN != errno))
            {
                LE_ERROR("Cannot write on fd: %s", strerror(errno));
                return currentSize;
            }
        }
        else
        {
            currentSize += sizeWritten;
        }
    }

    return currentSize;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function handles receiving AT commands
 */
//--------------------------------------------------------------------------------------------------
static void ReceiveCmd
(
    Device_t *devPtr  ///< [IN] Device pointer
)
{
    ssize_t size;

    size = le_dev_Read(devPtr, Cmd, MAX_LEN_CMD);
    if (size > 0)
    {
        LE_INFO("buffer is %s", Cmd);
    }

    // Sends the AT command response.
    snprintf((char*)Cmd, MAX_LEN_CMD, "\r\nOK\r\n");
    le_dev_Write(devPtr, Cmd, strlen((const char *)Cmd));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called when data are available to be read on fd
 */
//--------------------------------------------------------------------------------------------------
static void RxNewData
(
    int fd,      ///< [IN] File descriptor to read on
    short events ///< [IN] Event reported on fd (expect only POLLIN)
)
{
    Device_t *devPtr;

    devPtr = le_fdMonitor_GetContextPtr();

    if (events & POLLRDHUP)
    {
        LE_INFO("fd %d: Connection reset by peer", fd);
        le_dev_RemoveFdMonitoring(devPtr);
        return;
    }
    if (events & (POLLIN | POLLPRI))
    {
        LE_INFO("Receiving AT command..");
        ReceiveCmd(devPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function opens an AT server session on the requested device.
 *
 * @return
 *      - Reference to the requested device.
 *      - NULL if the device is not available or fd is a BAD FILE DESCRIPTOR.
 */
//--------------------------------------------------------------------------------------------------
le_atServer_DeviceRef_t le_atServer_Open
(
    int32_t fd          ///< The file descriptor
)
{
    char errMsg[ERR_MSG_MAX];

    // check if the file descriptor is valid
    if (fcntl(fd, F_GETFD) == -1)
    {
        memset(errMsg, 0, ERR_MSG_MAX);
        LE_ERROR("%s", strerror_r(errno, errMsg, ERR_MSG_MAX));
        return NULL;
    }

    // Device pool allocation
    DevicesPool = le_mem_CreatePool("DevicesPool",sizeof(Device_t));
    le_mem_ExpandPool(DevicesPool, DEVICE_POOL_SIZE);

    Device_t* devPtr = le_mem_ForceAlloc(DevicesPool);
    if (!devPtr)
    {
        LE_ERROR("devPtr is NULL!");
        return NULL;
    }

    memset(devPtr, 0, sizeof(Device_t));

    LE_INFO("Create a new interface for %d", fd);
    devPtr->fd = fd;

    if (LE_OK != le_dev_AddFdMonitoring(devPtr, RxNewData, devPtr))
    {
        LE_ERROR("Error during adding the fd monitoring");
        return NULL;
    }

    LE_INFO("created device");
    return AtServerDevRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function closes the AT server session on the requested device.
 *
 * @return
 *      - LE_OK             The function succeeded.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 *      - LE_BUSY           The requested device is busy.
 *      - LE_FAULT          Failed to stop the server, check logs for more information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_Close
(
    le_atServer_DeviceRef_t devRef    ///< [IN] device to be unbinded
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Suspend server / enter data mode
 *
 * When this function is called the server stops monitoring the fd for events
 * hence no more I/O operations are done on the fd by the server.
 *
 * @return
 *      - LE_OK             Success.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 *      - LE_FAULT          Device not monitored
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_Suspend
(
    le_atServer_DeviceRef_t devRef ///< [IN] device to be unbinded
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume server / enter command mode
 *
 * When this function is called the server resumes monitoring the fd for events
 * and is able to interpret AT commands again.
 *
 * @return
 *      - LE_OK             Success.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 *      - LE_FAULT          Device not monitored
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_Resume
(
    le_atServer_DeviceRef_t devRef ///< [IN] device to be unbinded
)
{
    return LE_OK;
}
