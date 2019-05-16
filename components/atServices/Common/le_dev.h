/** @file le_dev.c
 *
 * Implementation of device access.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_LE_DEV_INCLUDE_GUARD
#define LEGATO_LE_DEV_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * device structure
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct Device
{
    int32_t            fd;                               ///< The file descriptor.
    le_fdMonitor_Ref_t fdMonitor;                        ///< fd event monitor associated to Handle
}
Device_t;

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
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to write on device (or port)
 *
 * @return written byte number
 */
//--------------------------------------------------------------------------------------------------
int32_t le_dev_Write
(
    Device_t*   devicePtr,    ///< device pointer
    uint8_t*    txDataPtr,    ///< Buffer to write
    uint32_t    size          ///< size of buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to monitor the specified file descriptor in the calling thread event
 * loop.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dev_EnableFdMonitoring
(
    Device_t                    *devicePtr,     ///< Device pointer.
    le_fdMonitor_HandlerFunc_t   handlerFunc,   ///< Handler function.
    void                        *contextPtr,    ///< Context data.
    short                        events         ///< Events to monitor.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove the file descriptor monitoring from the event loop.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_DeleteFdMonitoring
(
    Device_t*   devicePtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Disable monitoring of the device.  Monitoring can be resumed with le_dev_EnableFdMonitoring().
 */
//--------------------------------------------------------------------------------------------------
void le_dev_DisableFdMonitoring
(
    Device_t    *devicePtr, ///< Device to stop monitoring.
    short        events     ///< Events to stop monitoring.
);

#endif /* end LEGATO_LE_DEV_INCLUDE_GUARD */
