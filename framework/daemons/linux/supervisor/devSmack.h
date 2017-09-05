//--------------------------------------------------------------------------------------------------
/** @file devSmack.h
 *
 * Internal API for generating SMACK labels for device files.  This API should be used consistently
 * for all device files so that device file labels are consistent and unique.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#ifndef LEGATO_SRC_DEVICE_SMACK_INCLUDE_GUARD
#define LEGATO_SRC_DEVICE_SMACK_INCLUDE_GUARD


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
);

#endif  // LEGATO_SRC_DEVICE_SMACK_INCLUDE_GUARD
