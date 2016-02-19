/**
 * @file pa_info.c
 *
 * AT implementation of @ref c_pa_info API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


#include "legato.h"
#include "pa.h"
#include "pa_info.h"




//--------------------------------------------------------------------------------------------------
/**
 * This function get the International Mobile Equipment Identity (IMEI).
 *
 * @return  LE_FAULT         The function failed to get the value.
 * @return  LE_TIMEOUT       No response was received from the Modem.
 * @return  LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetImei
(
    pa_info_Imei_t imei   ///< [OUT] IMEI value
)
{
    //To do

    return LE_OK;
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
le_result_t pa_info_GetFirmwareVersion
(
    char* versionPtr,        ///< [OUT] Firmware version string
    size_t versionSize       ///< [IN] Size of version buffer
)
{
    return LE_NOT_FOUND;
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
le_result_t pa_info_GetBootloaderVersion
(
    char* versionPtr,        ///< [OUT] Firmware version string
    size_t versionSize       ///< [IN] Size of version buffer
)
{
    return LE_NOT_FOUND;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function gets the device model identity.
 *
 * @return
 * - LE_FAULT         The function failed to get the value.
 * - LE_OVERFLOW      The device model identity length exceed the maximum length.
 * - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetDeviceModel
(
    pa_info_DeviceModel_t model   ///< [OUT] Model string (null-terminated).
)
{
    //To do

    return LE_FAULT;
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
 * Get the CDMA Mobile Directory Number (MDN) of the device.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *      - LE_OVERFLOW      The Mobile Directory Number length exceed the maximum length.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetMdn
(
    char* mdnStr,
        ///< [OUT]
        ///< The Mobile Directory Number (MDN)
        ///<  string (null-terminated).

    size_t mdnStrNumElements
        ///< [IN]
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

//--------------------------------------------------------------------------------------------------
/**
 * Get the Manufacturer Name string in ASCII text.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *      - LE_OVERFLOW      The Manufacturer Name length exceed the maximum length.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetManufacturerName
(
    char* mfrNameStr,
        ///< [OUT]
        ///< The Manufacturer Name string (null-terminated).

    size_t mfrNameStrNumElements
        ///< [IN]
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Product Requirement Information Part Number and Revision Number strings in ASCII text.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the value.
 *      - LE_OVERFLOW      The Part or the Revision Number strings length exceed the maximum length.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetPriId
(
    char* priIdPnStr,
        ///< [OUT]
        ///< The Product Requirement Information Identifier
        ///<  (PRI ID) Part Number string (null-terminated).

    size_t priIdPnStrNumElements,
        ///< [IN]

    char* priIdRevStr,
        ///< [OUT]
        ///< The Product Requirement Information Identifier
        ///<  (PRI ID) Revision Number string (null-terminated).

    size_t priIdRevStrNumElements
        ///< [IN]
)
{
    le_result_t res = LE_FAULT;

    if ( (priIdPnStr == NULL) || (priIdRevStr == NULL))
    {
        LE_ERROR("priIdPnStr or priIdRevStr is NULL.");
        return LE_FAULT;
    }

    if (priIdPnStrNumElements < LE_INFO_MAX_PRIID_PN_BYTES)
    {
        LE_ERROR("priIdPnStrNumElements lentgh (%d) too small < %d",
                        (int) priIdPnStrNumElements, LE_INFO_MAX_PRIID_PN_BYTES);
        res = LE_OVERFLOW;
    }

    if (priIdRevStrNumElements < LE_INFO_MAX_PRIID_REV_BYTES)
    {
        LE_ERROR("priIdRevStrNumElements lentgh (%d) too small < %d",
                        (int) priIdRevStrNumElements, LE_INFO_MAX_PRIID_REV_BYTES);
        res = LE_OVERFLOW;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Platform Serial Number (PSN) string.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if Platform Serial Number to big to fit in provided buffer
 *      - LE_FAULT for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetPlatformSerialNumber
(
    char* platformSerialNumberStr,
        ///< [OUT]
        ///< Platform Serial Number string.

    size_t platformSerialNumberStrNumElements
        ///< [IN]
)
{
    return LE_FAULT;
}
