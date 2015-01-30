/** @file atMachineDevice.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_ATMACHINEDEVICE_INCLUDE_GUARD
#define LEGATO_ATMACHINEDEVICE_INCLUDE_GUARD

#include "../inc/atDevice.h"

#define DEVICENAME_SIZE    64
#define DEVICEPATH_SIZE    64

//--------------------------------------------------------------------------------------------------
/**
 * typedef of atDevice_t
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct atdevice
{
    char                    name[DEVICENAME_SIZE]; ///< Name of the device
    char                    path[DEVICEPATH_SIZE]; ///< Path of the device
    uint32_t                handle;     ///< Handle of the device.
    le_da_Device_t          deviceItf;  ///< Pointer to the device interface function pointer set
                                        ///< (write, read, close, ioctl).
    le_event_FdMonitorRef_t fdMonitor;  ///< fd event monitor associated to Handle
}
atdevice_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called when we want to read on device (or port)
 *
 * @return byte number read
 */
//--------------------------------------------------------------------------------------------------
int32_t atmachinedevice_Read
(
    atdevice_Ref_t  deviceRef,    ///< device pointer
    uint8_t        *rxDataPtr,    ///< Buffer where to read
    uint32_t        size           ///< size of buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to write on device (or port)
 *
 */
//--------------------------------------------------------------------------------------------------
void  atmachinedevice_Write
(
    atdevice_Ref_t  deviceRef,    ///< device pointer
    uint8_t        *rxDataPtr,    ///< Buffer to write
    uint32_t        size           ///< size of buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to print a buffer byte by byte
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinedevice_PrintBuffer
(
    char     *namePtr,          ///< buffer name
    uint8_t  *bufferPtr,     ///< the buffer to print
    uint32_t  bufferSize     ///< Number of element to print
);

#endif /* LEGATO_ATMACHINEDEVICE_INCLUDE_GUARD */
