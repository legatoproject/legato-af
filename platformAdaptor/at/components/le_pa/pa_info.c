/**
 * @file pa_info.c
 *
 * AT implementation of @ref c_pa_info API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "le_atClient.h"

#include "pa_info.h"
#include "pa_utils_local.h"

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
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_OK;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!imei)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CGSN",
                                        "0|1|2|3|4|5|6|7|8|9",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,
                                       finalResponse,
                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

    if ((res != LE_OK) || (strcmp(finalResponse,"OK") != 0))
    {
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the response");
    }
    else
    {
        strncpy(imei, intermediateResponse, strlen(intermediateResponse));
    }
    le_atClient_Delete(cmdRef);
    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the firmware version string.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_FAULT for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetFirmwareVersion
(
    char*  versionPtr,       ///< [OUT] Firmware version string.
    size_t versionSize       ///< [IN] Size of version buffer.
)
{
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_OK;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!versionPtr)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CGMR",
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,
                                       finalResponse,
                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

    if ((res != LE_OK) || (strcmp(finalResponse,"OK") != 0))
    {
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the response");
    }
    else
    {
        strncpy(versionPtr, intermediateResponse, versionSize);
    }
    le_atClient_Delete(cmdRef);
    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the bootloader version string.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_FAULT for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetBootloaderVersion
(
    char*  versionPtr,       ///< [OUT] Firmware version string.
    size_t versionSize       ///< [IN] Size of version buffer.
)
{
    return LE_FAULT;
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
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_OK;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!model)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CGMM",
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,
                                       finalResponse,
                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if ((res != LE_OK) || (strcmp(finalResponse,"OK") != 0))
    {
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the response");
    }
    else
    {
        strncpy(model, intermediateResponse, strlen(intermediateResponse));
    }
    le_atClient_Delete(cmdRef);
    return res;
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
    char*  meidStr,          ///< [OUT] Firmware version string.
    size_t meidStrSize       ///< [IN] Size of version buffer.
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
    char*        minStr,        ///< [OUT] The phone Number.
    size_t       minStrSize     ///< [IN]  Size of phoneNumberStr.
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
    char*  esnStr,            ///< [OUT] The Electronic Serial Number (ESN) of the device string (null-terminated).
    size_t esnStrNumElements  ///< [IN] Size of Electronic Serial Number string.
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
    char*  mdnStr,            ///< [OUT] The Mobile Directory Number (MDN) string (null-terminated).
    size_t mdnStrNumElements  ///< [IN] Size of Mobile Directory Number string.
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
    uint16_t* prlVersionPtr   ///< [OUT] The Preferred Roaming List (PRL) version.
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the CDMA PRL only preferences Flag.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_NOT_FOUND     The information is not availble.
 *      - LE_FAULT         The function failed to get the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetPrlOnlyPreference
(
    bool* prlOnlyPreferencePtr   ///< The Cdma PRL only preferences status.
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
    char*  naiStr,            ///< [OUT] The Network Access Identifier (NAI) string (null-terminated).
    size_t naiStrNumElements  ///< [IN] Size of Network Access Identifier string.
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
    char*  mfrNameStr,              ///< [OUT] The Manufacturer Name string (null-terminated).
    size_t mfrNameStrNumElements    ///< [IN] Size of Manufacturer Name string.
)
{
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_OK;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!mfrNameStr)
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CGMI",
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,
                                       finalResponse,
                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);

    if ((res != LE_OK) || (strcmp(finalResponse,"OK") != 0))
    {
        LE_ERROR("Failed to get the response");
        le_atClient_Delete(cmdRef);
        return res;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the response");
    }
    else
    {
        strncpy(mfrNameStr, intermediateResponse, mfrNameStrNumElements);
    }
    le_atClient_Delete(cmdRef);
    return res;
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
    char*  priIdPnStr,              ///< [OUT] The Product Requirement Information Identifier
                                    ///<       (PRI ID) Part Number string (null-terminated).
    size_t priIdPnStrNumElements,   ///< [IN] Size of Product Requirement Information Identifier string.
    char*  priIdRevStr,             ///< [OUT] The Product Requirement Information Identifier
                                    ///<       (PRI ID) Revision Number string (null-terminated).
    size_t priIdRevStrNumElements   ///< [IN] Size of Product Requirement Information Identifier string.
)
{
    le_result_t res = LE_FAULT;

    if ((priIdPnStr == NULL) || (priIdRevStr == NULL))
    {
        LE_ERROR("priIdPnStr or priIdRevStr is NULL.");
        return LE_FAULT;
    }

    if (priIdPnStrNumElements < LE_INFO_MAX_PRIID_PN_BYTES)
    {
        LE_ERROR("priIdPnStrNumElements lentgh (%d) too small < %d",
                 (int) priIdPnStrNumElements,
                 LE_INFO_MAX_PRIID_PN_BYTES);
        res = LE_OVERFLOW;
    }

    if (priIdRevStrNumElements < LE_INFO_MAX_PRIID_REV_BYTES)
    {
        LE_ERROR("priIdRevStrNumElements lentgh (%d) too small < %d",
                 (int) priIdRevStrNumElements,
                 LE_INFO_MAX_PRIID_REV_BYTES);
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
    char*  platformSerialNumberStr,             ///< [OUT] Platform Serial Number string.
    size_t platformSerialNumberStrNumElements   ///< [IN] Size of Platform Serial Number string.
)
{
    return LE_FAULT;
}