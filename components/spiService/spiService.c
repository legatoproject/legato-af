/**
 *
 * This file provides the implementation of @ref c_spi using the @c spidev Linux kernel driver.
 *
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_spiLibrary.h"
#include "watchdogChain.h"

#define MAX_EXPECTED_DEVICES (8)

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 8

typedef struct
{
    int fd;
    ino_t inode;
    le_msg_SessionRef_t owningSession;
} Device_t;


static bool IsDeviceOwnedByCaller(const Device_t* handle);
static Device_t const* FindDeviceWithInode(ino_t inode);
static void CloseDevice(le_spi_DeviceHandleRef_t handle, Device_t* device);
static void CloseAllHandlesOwnedByClient(le_msg_SessionRef_t owner);
static void ClientSessionClosedHandler(le_msg_SessionRef_t clientSession, void* context);

// Memory pool for allocating devices
static le_mem_PoolRef_t DevicePool;
// A map of safe references to device objects
static le_ref_MapRef_t DeviceHandleRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Opens an SPI device so that the attached device may be accessed.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if the device name string is bad
 *      - LE_NOT_FOUND if the SPI device file could not be found
 *      - LE_NOT_PERMITTED if the SPI device file can't be opened for read/write
 *      - LE_DUPLICATE if the given device file is already opened by another spiService client
 *      - LE_FAULT for non-specific failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_spi_Open
(
    const char* deviceName,        ///< [in] Name of the device file.  Do not include the "/dev/"
                                   ///  prefix.
    le_spi_DeviceHandleRef_t* handle  ///< [out] Handle for passing to related functions in order to
                                   ///  access the SPI device
)
{
    if (handle == NULL)
    {
        LE_KILL_CLIENT("handle is NULL.");
        return LE_FAULT;
    }

    *handle = NULL;

    char devicePath[256];
    const int snprintfResult = snprintf(devicePath, sizeof(devicePath), "/dev/%s", deviceName);
    if (snprintfResult > (sizeof(devicePath) - 1))
    {
        LE_ERROR("deviceName argument is too long (%s)", deviceName);
        return LE_BAD_PARAMETER;
    }
    if (snprintfResult < 0)
    {
        LE_ERROR("String formatting error");
        return LE_FAULT;
    }

    struct stat deviceFileStat;
    const int statResult = stat(devicePath, &deviceFileStat);
    if (statResult != 0)
    {
        if (errno == ENOENT)
        {
            return LE_NOT_FOUND;
        }
        else if (errno == EACCES)
        {
            return LE_NOT_PERMITTED;
        }
        else
        {
            return LE_FAULT;
        }
    }
    Device_t const* foundDevice = FindDeviceWithInode(deviceFileStat.st_ino);
    if (foundDevice != NULL)
    {
        LE_ERROR(
            "Device file \"%s\" has already been opened by a client with id (%p)",
            devicePath,
            foundDevice->owningSession);
        return LE_DUPLICATE;
    }

    const int openResult = open(devicePath, O_RDWR);
    if (openResult < 0)
    {
        if (errno == ENOENT)
        {
            return LE_NOT_FOUND;
        }
        else if (errno == EACCES)
        {
            return LE_NOT_PERMITTED;
        }
        else
        {
            return LE_FAULT;
        }
    }

    Device_t* newDevice = le_mem_ForceAlloc(DevicePool);
    newDevice->fd = openResult;
    newDevice->inode = deviceFileStat.st_ino;
    newDevice->owningSession = le_spi_GetClientSessionRef();
    *handle = le_ref_CreateRef(DeviceHandleRefMap, newDevice);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Closes the device associated with the given handle and frees the associated resources.
 *
 * @note
 *      Once a handle is closed, it is not permitted to use it for future SPI access without first
 *      calling le_spi_Open.
 */
//--------------------------------------------------------------------------------------------------
void le_spi_Close
(
    le_spi_DeviceHandleRef_t handle  ///< [in] Handle to close
)
{
    Device_t* device = le_ref_Lookup(DeviceHandleRefMap, handle);
    if (device == NULL)
    {
        LE_KILL_CLIENT("Failed to lookup device from handle!");
        return;
    }

    if (!IsDeviceOwnedByCaller(device))
    {
        LE_KILL_CLIENT("Cannot close handle as it is not owned by the caller");
        return;
    }

    CloseDevice(handle, device);
}


//--------------------------------------------------------------------------------------------------
/**
 * Configures an SPI device
 *
 * @note
 *      This function should be called before any of the Read/Write functions in order to ensure
 *      that the SPI bus configuration is in a known state.
 */
//--------------------------------------------------------------------------------------------------
void le_spi_Configure
(
    le_spi_DeviceHandleRef_t handle, ///< [in] Handle for the SPI master to configure
    int mode,       ///< [in] Mode options for the bus as defined in le_spi.api.
    uint8_t bits,   ///< [in]  bits per word, typically 8 bits per word
    uint32_t speed, ///< [in] max speed (Hz), this is slave dependant
    int msb         ///< [in] set as 0 for MSB as first bit or 1 for LSB as first bit
)
{
    Device_t* device = le_ref_Lookup(DeviceHandleRefMap, handle);
    if (device == NULL)
    {
        LE_KILL_CLIENT("Failed to lookup device from handle!");
        return;
    }

    if (!IsDeviceOwnedByCaller(device))
    {
        LE_KILL_CLIENT("Cannot assign handle to configure as it is not owned by the caller");
    }

    le_spiLib_Configure(device->fd, mode, bits, speed, msb);
}


//--------------------------------------------------------------------------------------------------
/**
 * SPI Half Duplex Write followed by Half Duplex Read
 *
 * @return
 *      LE_OK on success or LE_FAULT on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_spi_WriteReadHD
(
    le_spi_DeviceHandleRef_t handle, ///< [in] Handle for the SPI master to perform the write-read on
    const uint8_t* writeData,     ///< [in] Tx command/address being sent to slave
    size_t writeDataLength,       ///< [in] Number of bytes in tx message
    uint8_t* readData,            ///< [out] Rx response from slave
    size_t* readDataLength        ///< [in/out] Number of bytes in rx message
)
{
    if (readData == NULL)
    {
        LE_KILL_CLIENT("readData is NULL.");
        return LE_FAULT;
    }

    Device_t* device = le_ref_Lookup(DeviceHandleRefMap, handle);
    if (device == NULL)
    {
        LE_KILL_CLIENT("Failed to lookup device from handle!");
        return LE_FAULT;
    }

    if (!IsDeviceOwnedByCaller(device))
    {
        LE_KILL_CLIENT("Cannot assign handle to read as it is not owned by the caller");
        return LE_FAULT;
    }

    return le_spiLib_WriteReadHD(
        device->fd,
        writeData,
        writeDataLength,
        readData,
        readDataLength) == LE_OK ? LE_OK : LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * SPI Write for Half Duplex Communication
 *
 * @return
 *      LE_OK on success or LE_FAULT on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_spi_WriteHD
(
    le_spi_DeviceHandleRef_t handle, ///< [in] Handle for the SPI master to perform the write on
    const uint8_t* writeData,     ///< [in] Command/address being sent to slave
    size_t writeDataLength        ///< [in] Number of bytes in tx message
)
{
    Device_t* device = le_ref_Lookup(DeviceHandleRefMap, handle);
    if (device == NULL)
    {
        LE_KILL_CLIENT("Failed to lookup device from handle!");
        return LE_FAULT;
    }

    if (!IsDeviceOwnedByCaller(device))
    {
        LE_KILL_CLIENT("Cannot assign handle to write  as it is not owned by the caller");
    }

    return le_spiLib_WriteHD(device->fd, writeData, writeDataLength) == LE_OK ? LE_OK : LE_FAULT;
}



//--------------------------------------------------------------------------------------------------
/**
 * Simultaneous SPI Write and  Read for full duplex communication
 *
 * @return
 *      LE_OK on success or LE_FAULT on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_spi_WriteReadFD
(
    le_spi_DeviceHandleRef_t handle, ///< [in] Handle for the SPI master to perform the write-read on
    const uint8_t* writeData,     ///< [in] Tx command/address being sent to slave
    size_t writeDataLength,       ///< [in] Number of bytes in tx message
    uint8_t* readData,            ///< [out]Rx response from slave
    size_t *readDataLength        ///< [in/out] Number of bytes in rx message
)
{
    if (readData == NULL)
    {
        LE_KILL_CLIENT("readData is NULL.");
        return LE_FAULT;
    }

    Device_t* device = le_ref_Lookup(DeviceHandleRefMap, handle);
    if (device == NULL)
    {
        LE_KILL_CLIENT("Failed to lookup device from handle!");
        return LE_FAULT;
    }

    if (!IsDeviceOwnedByCaller(device))
    {
        LE_KILL_CLIENT("Cannot assign handle to read as it is not owned by the caller");
        return LE_FAULT;
    }

    if(*readDataLength < writeDataLength)
    {
        LE_KILL_CLIENT("readData length cannot be less than writeData length");
    }

    return le_spiLib_WriteReadFD(
        device->fd,
        writeData,
        readData,
        writeDataLength) == LE_OK ? LE_OK : LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * SPI Read for Half Duplex Communication
 *
 * @return
 *      LE_OK on success or LE_FAULT on failure.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_spi_ReadHD
(
    le_spi_DeviceHandleRef_t handle, ///< [in] Handle for the SPI master to perform the write on
    uint8_t* readData,      ///< [out] Command/address being sent to slave
    size_t* readDataLength        ///< [in/out] Number of bytes in tx message
)
{
    if (readData == NULL)
    {
        LE_KILL_CLIENT("readData is NULL.");
        return LE_FAULT;
    }

    Device_t* device = le_ref_Lookup(DeviceHandleRefMap, handle);
    if (device == NULL)
    {
        LE_KILL_CLIENT("Failed to lookup device from handle!");
        return LE_FAULT;
    }

    if (!IsDeviceOwnedByCaller(device))
    {
        LE_KILL_CLIENT("Cannot assign handle to write  as it is not owned by the caller");
    }

    return le_spiLib_ReadHD(device->fd, readData, readDataLength) == LE_OK ? LE_OK : LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks if the given handle is owned by the current client.
 *
 * @return
 *      true if the handle is owned by the current client or false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool IsDeviceOwnedByCaller
(
    const Device_t* device  ///< [in] Device to check the ownership of
)
{
    return device->owningSession == le_spi_GetClientSessionRef();
}

//--------------------------------------------------------------------------------------------------
/**
 * Searches for a device which contains the given inode value. It is assumed that there will be
 * either 0 or 1 device containing the given inode.
 *
 * @return
 *      Handle with the given inode or NULL if a matching handle was not found.
 */
//--------------------------------------------------------------------------------------------------
static Device_t const* FindDeviceWithInode
(
    ino_t inode
)
{
    le_ref_IterRef_t it = le_ref_GetIterator(DeviceHandleRefMap);
    while (le_ref_NextNode(it) == LE_OK)
    {
        Device_t const* device = le_ref_GetValue(it);
        LE_ASSERT(device != NULL);
        if (device->inode == inode)
        {
            return device;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close the device associated with the handle.
 *
 * No check is performed to verify that the device is associated with the handle
 * because it is assumed that the caller will have already verified this.
 */
//--------------------------------------------------------------------------------------------------
static void CloseDevice(le_spi_DeviceHandleRef_t handle, Device_t* device)
{
    // Remove the handle from the map so it can't be used again
    le_ref_DeleteRef(DeviceHandleRefMap, handle);

    int closeResult = close(device->fd);
    if (closeResult != 0)
    {
        LE_WARN("Couldn't close the fd cleanly: (%m)");
    }
    le_mem_Release(device);
}

//--------------------------------------------------------------------------------------------------
/**
 * Closes all of the handles that are owned by a specific client session.  The purpose of this
 * function is to free resources on the server side when it is detected that a client has
 * disconnected.
 */
//--------------------------------------------------------------------------------------------------
static void CloseAllHandlesOwnedByClient
(
    le_msg_SessionRef_t owner
)
{
    le_ref_IterRef_t it = le_ref_GetIterator(DeviceHandleRefMap);


    bool finished = le_ref_NextNode(it) != LE_OK;
    while (!finished)
    {
        Device_t* device = le_ref_GetValue(it);
        LE_ASSERT(device != NULL);
        // In order to prevent invalidating the iterator, we store the reference of the device we
        // want to close and advance the iterator before calling le_spi_Close which will remove the
        // reference from the hashmap.
        le_spi_DeviceHandleRef_t toClose =
            (device->owningSession == owner) ? ((void*)le_ref_GetSafeRef(it)) : NULL;
        finished = le_ref_NextNode(it) != LE_OK;
        if (toClose != NULL)
        {
            CloseDevice(toClose, device);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * A handler for client disconnects which frees all resources associated with the client.
 */
//--------------------------------------------------------------------------------------------------
static void ClientSessionClosedHandler
(
    le_msg_SessionRef_t clientSession,
    void* context
)
{
    CloseAllHandlesOwnedByClient(clientSession);
}

COMPONENT_INIT
{
    LE_DEBUG("spiServiceComponent initializing");

    DevicePool = le_mem_CreatePool("SPI Pool", sizeof(Device_t));
    DeviceHandleRefMap = le_ref_CreateMap("SPI handles", MAX_EXPECTED_DEVICES);

    // Register a handler to be notified when clients disconnect
    le_msg_AddServiceCloseHandler(le_spi_GetServiceRef(), ClientSessionClosedHandler, NULL);

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);
}
