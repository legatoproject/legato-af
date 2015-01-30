/** @file le_info.c
 *
 * Legato @ref c_info implementation.
 *
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */
#include "legato.h"
#include "interfaces.h"
#include "pa_info.h"
#include "pa_sim.h"


//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to retrieve the International Mobile Equipment Identity (IMEI).
 *
 * @return LE_FAULT       The function failed to retrieve the IMEI.
 * @return LE_OVERFLOW    The IMEI length exceed the maximum length.
 * @return LE_OK          The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_info_GetImei
(
    char*            imeiPtr,  ///< [OUT] The IMEI string.
    size_t           len       ///< [IN] The length of IMEI string.
)
{
    pa_info_Imei_t imei = {0};

    if (imeiPtr == NULL)
    {
        LE_KILL_CLIENT("imeiPtr is NULL !");
        return LE_FAULT;
    }

    if(pa_info_GetImei(imei) != LE_OK)
    {
        LE_ERROR("Failed to get the IMEI");
        imeiPtr[0] = '\0';
        return LE_FAULT;
    }
    else
    {
        return (le_utf8_Copy(imeiPtr, imei, len, NULL));
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Get the firmware version string
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_FAULT for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_info_GetFirmwareVersion
(
    char* version,
        ///< [OUT]
        ///< Firmware version string

    size_t versionNumElements
        ///< [IN]
)
{
    return pa_info_GetFirmwareVersion(version, versionNumElements);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the bootloader version string
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_FAULT for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_info_GetBootloaderVersion
(
    char* version,
        ///< [OUT]
        ///< Bootloader version string

    size_t versionNumElements
        ///< [IN]
)
{
    return pa_info_GetBootloaderVersion(version, versionNumElements);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the device model identity (Target Hardware Platform).
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *      - LE_OVERFLOW      The device model identity length exceed the maximum length.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_info_GetDeviceModel
(
    char* modelPtr,
        ///< [OUT] The model identity string (null-terminated).

    size_t modelNumElements
        ///< [IN] The length of Model identity string.
)
{
    pa_info_DeviceModel_t modelVersion;

    if(modelPtr == NULL)
    {
        LE_KILL_CLIENT("model pointer is NULL");
        return LE_FAULT;
    }

    if(pa_info_GetDeviceModel(modelVersion) != LE_OK)
    {
        LE_ERROR("Failed to get the device model");
        modelPtr[0] = '\0';
        return LE_FAULT;
    }
    else
    {
        return (le_utf8_Copy(modelPtr, modelVersion, modelNumElements, NULL));
    }
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
le_result_t le_info_GetMeid
(
    char* meidStr,
        ///< [OUT]
        ///< The Mobile Equipment identifier (MEID)
        ///<  string (null-terminated).

    size_t meidStrNumElements
        ///< [IN]
)
{
    if (meidStr == NULL)
    {
        LE_KILL_CLIENT("meidStr is NULL !");
        return LE_FAULT;
    }

    return pa_info_GetMeid(meidStr, meidStrNumElements);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the CDMA Electronic Serial Number (ESN) of the device.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *      - LE_OVERFLOW      The Electronic Serial Number length exceed the maximum length.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_info_GetEsn
(
    char* esnStr,
        ///< [OUT]
        ///< The Electronic Serial Number (ESN) of the device.
        ///<  string (null-terminated).

    size_t esnStrNumElements
        ///< [IN]
)
{
    if (esnStr == NULL)
    {
        LE_KILL_CLIENT("esnStr is NULL !");
        return LE_FAULT;
    }

    return pa_info_GetEsn(esnStr, esnStrNumElements);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the CDMA Mobile Directory Number (MDN) of the device.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *      - LE_OVERFLOW      The Mobile Directory Number length exceed the maximum length.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_info_GetMdn
(
    char* mdnStr,
        ///< [OUT]
        ///< The Mobile Directory Number (MDN)
        ///<  string (null-terminated).

    size_t mdnStrNumElements
        ///< [IN]
)
{
    if (mdnStr == NULL)
    {
        LE_KILL_CLIENT("mdnStr is NULL !");
        return LE_FAULT;
    }

    return pa_sim_GetSubscriberPhoneNumber(mdnStr, mdnStrNumElements);
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
le_result_t le_info_GetMin
(
    char* minStr,
        ///< [OUT]
        ///< The Mobile Identification Number (MIN)
        ///<  string (null-terminated).

    size_t minStrNumElements
        ///< [IN]
)
{
    if (minStr == NULL)
    {
        LE_KILL_CLIENT("minStr is NULL !");
        return LE_FAULT;
    }

    le_result_t res = pa_info_GetMin(minStr, minStrNumElements);
    if ((res != LE_OK) && (res != LE_OVERFLOW))
    {
        res = LE_FAULT;
    }

    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the CDMA version of Preferred Roaming List (PRL).
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_NOT_FOUND     The information is not availble.
 *      - LE_FAULT         The function failed to get the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_info_GetPrlVersion
(
    uint16_t* prlVersionPtr
        ///< [OUT]
        ///< The Preferred Roaming List (PRL) version.
)
{
    if (prlVersionPtr == NULL)
    {
        LE_KILL_CLIENT("prlVersionPtr is NULL !");
        return LE_FAULT;
    }

    return pa_info_GetPrlVersion(prlVersionPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the CDMA Preferred Roaming List (PRL) only preferences status.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_NOT_FOUND     The information is not availble.
 *      - LE_FAULT         The function failed to get the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_info_GetPrlOnlyPreference
(
    bool* prlOnlyPreferencePtr
        ///< [OUT]
        ///< The Cdma PRL only preferences status.
)
{
    if (prlOnlyPreferencePtr == NULL)
    {
        LE_KILL_CLIENT("prlVersionPtr is NULL !");
        return LE_FAULT;
    }

    return pa_info_GetPrlOnlyPreference(prlOnlyPreferencePtr);
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
le_result_t le_info_GetNai
(
    char* naiStr,
        ///< [OUT]
        ///< The Network Access Identifier (NAI)
        ///<  string (null-terminated).

    size_t naiStrNumElements
        ///< [IN]
)
{
    if (naiStr == NULL)
    {
        LE_KILL_CLIENT("naiStr is NULL !");
        return LE_FAULT;
    }

    return pa_info_GetNai(naiStr, naiStrNumElements);
}
