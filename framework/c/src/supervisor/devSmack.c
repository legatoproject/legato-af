//--------------------------------------------------------------------------------------------------
/** @file appSmack.c
 *
 * This component implements the devSmack API which generates the SMACK label for devices.
 *
 *  - @ref c_devSmack_Labels
 *
 * @section c_devSmack_Labels Device SMACK Labels
 *
 * SMACK labels for devices contain an "dev." prefix and is based on the major and minor device
 * numbers.  This ensures that the device is always given the same label regardless of the device
 * name.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Gets a SMACK label for a device file from the device ID.
 *
 * @return
 *      LE_OK if the label was successfully returned in the provided buffer.
 *      LE_OVERFLOW if the label could not fit in the provided buffer.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
le_result_t devSmack_GetLabel
(
    dev_t devId,            ///< [IN] Device ID.
    char* bufPtr,           ///< [OUT] Buffer to hold the SMACK label for the device.
    size_t bufSize          ///< [IN] Size of the buffer.
)
{
    int n = snprintf(bufPtr, bufSize, "dev.%x%x", major(devId), minor(devId));

    if (n < 0)
    {
        LE_ERROR("Could not generate SMACK label for device (%d, %d).  %m.", major(devId), minor(devId));
        return LE_FAULT;
    }

    if (n >= bufSize)
    {
        return LE_OVERFLOW;
    }

    return LE_OK;
}
