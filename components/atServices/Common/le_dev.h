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
le_result_t le_dev_AddFdMonitoring
(
    Device_t*                   devicePtr,    ///< device pointer
    le_fdMonitor_HandlerFunc_t  handlerFunc, ///< [in] Handler function.
    void*                       contextPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove the file descriptor monitoring from the event loop.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_RemoveFdMonitoring
(
    Device_t*   devicePtr
);

#endif
