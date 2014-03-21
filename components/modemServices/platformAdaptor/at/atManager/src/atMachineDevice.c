/** @file atmachinedevice.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "atMachineDevice.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called when we want to read on device (or port)
 *
 * @return byte number read
 */
//--------------------------------------------------------------------------------------------------
int32_t atmachinedevice_Read
(
    struct atdevice  *devicePtr,    ///< device pointer
    uint8_t     *rxDataPtr,    ///< Buffer where to read
    uint32_t    size           ///< size of buffer
)
{
    int32_t status;
    int32_t r_amount = 0;

    status = devicePtr->deviceItf.read(devicePtr->handle, rxDataPtr, size);

    if(status > 0)
    {
        size      -= status;
        rxDataPtr += status;
        r_amount  += status;
    }

    LE_DEBUG("%s[%s] -> Read (%d) on %d",
             devicePtr->name,devicePtr->path,
             r_amount,devicePtr->handle);

    return r_amount;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to write on device (or port)
 *
 */
//--------------------------------------------------------------------------------------------------
void  atmachinedevice_Write
(
    struct atdevice *devicePtr,    ///< device pointer
    uint8_t     *rxDataPtr,    ///< Buffer to write
    uint32_t    size           ///< size of buffer
)
{
    int32_t status;
    int32_t w_amount = 0;

    status = devicePtr->deviceItf.write(devicePtr->handle, rxDataPtr, size);

    if(status > 0)
    {
        size      -= status;
        w_amount  += status;
    }

    LE_DEBUG("%s[%s] -> write (%d) on %d",
             devicePtr->name,devicePtr->path,
             w_amount,devicePtr->handle);

    atmachinedevice_PrintBuffer(devicePtr->name,rxDataPtr,w_amount);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to print a buffer byte by byte
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinedevice_PrintBuffer
(
    char     *namePtr,       ///< buffer name
    uint8_t  *bufferPtr,     ///< the buffer to print
    uint32_t  bufferSize     ///< Number of element to print
)
{
    uint32_t i;
    for(i=0;i<bufferSize;i++){
        if (bufferPtr[i] == '\r' ) {
            LE_DEBUG("'%s' -> [%d] '0x%.2x' '%s'",(namePtr)?namePtr:"no name",i,bufferPtr[i],"CR");
        } else if (bufferPtr[i] == '\n') {
            LE_DEBUG("'%s' -> [%d] '0x%.2x' '%s'",(namePtr)?namePtr:"no name",i,bufferPtr[i],"LF");
        } else if (bufferPtr[i] == 0x1A) {
            LE_DEBUG("'%s' -> [%d] '0x%.2x' '%s'",(namePtr)?namePtr:"no name",i,bufferPtr[i],"CTRL+Z");
        } else {
            LE_DEBUG("'%s' -> [%d] '0x%.2x' '%c'",(namePtr)?namePtr:"no name",i,bufferPtr[i],bufferPtr[i]);
        }
    }
}


