/**
 * @page c_pa_info Modem Information Platform Adapter API
 *
 * @ref pa_info.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * @section pa_info_toc Table of Contents
 *
 *  - @ref pa_info_intro
 *  - @ref pa_info_rational
 *
 *
 * @section pa_info_intro Introduction
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developped upon these APIs.
 *
 *
 * @section pa_info_rational Rational
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occured due to an interrupted communication with the modem.
 *
 * They all verify the validity and the range of the input parameters before performing the modem
 * operation.
 *
 * Some functions are used to get some information with a fixed pattern string,
 * in this case no buffer overflow will occur has they always get a fixed string length.
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file pa_info.h
 *
 * Legato @ref c_pa_info include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_PAINFO_INCLUDE_GUARD
#define LEGATO_PAINFO_INCLUDE_GUARD


#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Maximum 'International Mobile Equipment Identity length.
 */
//--------------------------------------------------------------------------------------------------
#define PA_INFO_IMEI_MAX_LEN     15


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of characters (excluding null terminator) in a device model
 * identification string.
 **/
//--------------------------------------------------------------------------------------------------
#define PA_INFO_DEVICE_MODEL_MAX_LEN     256


//--------------------------------------------------------------------------------------------------
/**
 * Type definition for an 'International Mobile Equipment Identity' (16 digits)
 */
//--------------------------------------------------------------------------------------------------
typedef char pa_info_Imei_t[PA_INFO_IMEI_MAX_LEN + 1];



//--------------------------------------------------------------------------------------------------
/**
 * Type definition for a 'Device Model ID'.
 */
//--------------------------------------------------------------------------------------------------
typedef char pa_info_DeviceModel_t[PA_INFO_DEVICE_MODEL_MAX_LEN + 1];




//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Get the firmware version string
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_NOT_POSSIBLE for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetFirmwareVersion
(
    char* versionPtr,        ///< [OUT] Firmware version string
    size_t versionSize       ///< [IN] Size of version buffer
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the bootloader version string
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_NOT_POSSIBLE for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetBootloaderVersion
(
    char* versionPtr,        ///< [OUT] Firmware version string
    size_t versionSize       ///< [IN] Size of version buffer
);


//--------------------------------------------------------------------------------------------------
/**
 * This function get the International Mobile Equipment Identity (IMEI).
 *
 * @return
 * - LE_NOT_POSSIBLE  The function failed to get the value.
 * - LE_TIMEOUT       No response was received from the Modem.
 * - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetIMEI
(
    pa_info_Imei_t imei   ///< [OUT] IMEI value
);



//--------------------------------------------------------------------------------------------------
/**
 * This function gets the device model identity.
 *
 * @return
 * - LE_NOT_POSSIBLE  The function failed to get the value.
 * - LE_TIMEOUT       No response was received from the Modem.
 * - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetDeviceModel
(
    pa_info_DeviceModel_t model   ///< [OUT] Model string (null-terminated).
);



#endif // LEGATO_PAINFO_INCLUDE_GUARD
