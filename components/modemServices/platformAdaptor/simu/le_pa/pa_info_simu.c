/**
 * @file pa_info_simu.c
 *
 * Simulation implementation of @ref c_pa_info API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "pa_info.h"

#include "interfaces.h"
#include "pa_simu.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function get the International Mobile Equipment Identity (IMEI).
 *
 * @return  LE_NOT_POSSIBLE  The function failed to get the value.
 * @return  LE_COMM_ERROR    The communication device has returned an error.
 * @return  LE_TIMEOUT       No response was received from the Modem.
 * @return  LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetImei
(
    pa_info_Imei_t imei   ///< [OUT] IMEI value
)
{
    le_result_t res;
    char buffer[513];

    res = le_cfg_QuickGetString(PA_SIMU_CFG_MODEM_ROOT "/info/imei", buffer, sizeof(buffer), PA_SIMU_INFO_DEFAULT_IMEI);

    switch (res)
    {
        case LE_OK:
            res = le_utf8_Copy(imei, buffer, sizeof(pa_info_Imei_t), NULL);
            LE_WARN_IF(res != LE_OK, "Error when copying string: %d", res);
            break;

        default:
            LE_FATAL("Unexpected result: %i", res);
            break;
    }

    return LE_OK;
}

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
)
{
    le_result_t res;
    char buffer[PA_INFO_IMEI_MAX_LEN+1];

    res = le_cfg_QuickGetString(PA_SIMU_CFG_MODEM_ROOT "/info/fwVersion", buffer, sizeof(buffer), PA_SIMU_INFO_DEFAULT_FW_VERSION);

    switch (res)
    {
        case LE_OK:
            res = le_utf8_Copy(versionPtr, buffer, versionSize, NULL);
            LE_WARN_IF(res != LE_OK, "Error when copying string: %d", res);
            break;

        default:
            LE_FATAL("Unexpected result: %i", res);
            break;
    }

    return LE_OK;
}


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
)
{
    le_result_t res;
    char buffer[PA_INFO_IMEI_MAX_LEN+1];

    res = le_cfg_QuickGetString(PA_SIMU_CFG_MODEM_ROOT "/info/bootVersion", buffer, sizeof(buffer), PA_SIMU_INFO_DEFAULT_BOOT_VERSION);

    switch (res)
    {
        case LE_OK:
            res = le_utf8_Copy(versionPtr, buffer, versionSize, NULL);
            LE_WARN_IF(res != LE_OK, "Error when copying string: %d", res);
            break;

        default:
            LE_FATAL("Unexpected result: %i", res);
            break;
    }

    return LE_OK;
}


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
)
{
    le_result_t res;
    char buffer[PA_INFO_DEVICE_MODEL_MAX_LEN+1];

    res = le_cfg_QuickGetString(PA_SIMU_CFG_MODEM_ROOT "/info/deviceModel", buffer, sizeof(buffer), PA_SIMU_INFO_DEFAULT_DEVICE_MODEL);

    switch (res)
    {
        case LE_OK:
            res = le_utf8_Copy(model, buffer, sizeof(pa_info_DeviceModel_t), NULL);
            LE_WARN_IF(res != LE_OK, "Error when copying string: %d", res);
            break;

        default:
            LE_FATAL("Unexpected result: %i", res);
            break;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the CDMA device Mobile Equipment Identifier (MEID).
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *      - LE_OVERFLOW      The device Mobile Equipment identifier length exceed the maximum length.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetMeid
(
    char* meidStr,           ///< [OUT] Firmware version string
    size_t meidStrSize       ///< [IN] Size of version buffer
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the CDMA Electronic Serial Number (ESN) of the device.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *      - LE_OVERFLOW      The Electric SerialNumbe length exceed the maximum length.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetEsn
(
    char* esnStr,
        ///< [OUT]
        ///< The Electronic Serial Number (ESN) of the device
        ///<  string (null-terminated).

    size_t esnStrNumElements
        ///< [IN]
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the CDMA Mobile Identification Number (MIN).
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *      - LE_OVERFLOW      The CDMA Mobile Identification Number length exceed the maximum length.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetMin
(
    char        *minStr,    ///< [OUT] The phone Number
    size_t       minStrSize ///< [IN]  Size of phoneNumberStr
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the version of Preferred Roaming List (PRL).
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetPrlVersion
(
    uint16_t* prlVersionPtr
        ///< [OUT]
        ///< The Preferred Roaming List (PRL) version.
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Cdma PRL only preferences Flag.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_NOT_FOUND     The information is not availble.
 *      - LE_FAULT         The function failed to get the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetPrlOnlyPreference
(
    bool* prlOnlyPreferencePtr      ///< The Cdma PRL only preferences status.
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the CDMA Network Access Identifier (NAI) string in ASCII text.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *      - LE_OVERFLOW      The Network Access Identifier (NAI) length exceed the maximum length.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetNai
(
    char* naiStr,
        ///< [OUT]
        ///< The Network Access Identifier (NAI)
        ///<  string (null-terminated).

    size_t naiStrNumElements
        ///< [IN]
)
{
    return LE_FAULT;
}

