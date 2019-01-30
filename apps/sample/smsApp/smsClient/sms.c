#include "legato.h"
#include "interfaces.h"
#include "smsSample.h"

#ifndef LE_INFO_MAX_VERS_BYTES
#define LE_INFO_MAX_VERS_BYTES 257
#endif

//--------------------------------------------------------------------------------------------------
/**
 * SMS handler references.
 *
 */
//--------------------- -----------------------------------------------------------------------------
static le_sms_RxMessageHandlerRef_t         RxHdlrRef;
static le_sms_FullStorageEventHandlerRef_t  FullStorageHdlrRef;

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SMS message reception.
 *
 */
//--------------------- -----------------------------------------------------------------------------
static void RxMessageHandler
(
    le_sms_MsgRef_t msgRef,
    void*            contextPtr
)
{
    le_result_t  res;
    char         tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    char         timestamp[LE_SMS_TIMESTAMP_MAX_BYTES] = {0};
    char         text[LE_SMS_TEXT_MAX_BYTES] = {0};

    LE_INFO("A New SMS message is received with ref.%p", msgRef);

    if (le_sms_GetFormat(msgRef) == LE_SMS_FORMAT_TEXT)
    {
        res = le_sms_GetSenderTel(msgRef, tel, sizeof(tel));
        if(res != LE_OK)
        {
            LE_ERROR("le_sms_GetSenderTel has failed (res.%d)!", res);
        }
        else
        {
            LE_INFO("Message is received from %s.", tel);
        }

        res = le_sms_GetTimeStamp(msgRef, timestamp, sizeof(timestamp));
        if(res != LE_OK)
        {
            LE_ERROR("le_sms_GetTimeStamp has failed (res.%d)!", res);
        }
        else
        {
            LE_INFO("Message timestamp is %s.", timestamp);
        }

        res = le_sms_GetText(msgRef, text, sizeof(text));
        if(res != LE_OK)
        {
            LE_ERROR("le_sms_GetText has failed (res.%d)!", res);
        }
        else
        {
            LE_INFO("Message content: \"%s\"", text);
            decodeMsgRequest(tel, text);
        }
    }
    else
    {
        LE_WARN("Warning! I read only Text messages!");
    }

    le_sms_Delete(msgRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function simply sends a text message using Legato SMS APIs.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsmo_SendMessage
(
    const char*   destinationPtr, ///< [IN] The destination number.
    const char*   textPtr,        ///< [IN] The SMS message text.
    const bool    async          ///< [IN] The SMS message text.
)
{
    le_result_t           res;
    le_sms_MsgRef_t      myMsg;

    myMsg = le_sms_Create();
    if (!myMsg)
    {
        LE_ERROR("SMS message creation has failed!");
        return LE_FAULT;
    }

    res = le_sms_SetDestination(myMsg, destinationPtr);
    if (res != LE_OK)
    {
        LE_ERROR("le_sms_SetDestination has failed (res.%d)!", res);
        return LE_FAULT;
    }

    res = le_sms_SetText(myMsg, textPtr);
    if (res != LE_OK)
    {
        LE_ERROR("le_sms_SetText has failed (res.%d)!", res);
        return LE_FAULT;
    }

    if (async)
    {
        res = le_sms_SendAsync(myMsg, NULL, NULL);
    }
    else
    {
        res = le_sms_Send(myMsg);
    }

    if (res != LE_OK)
    {
        LE_ERROR("Failed to send sms (res.%d)!", res);
        return LE_FAULT;
    }
    else
    {
        LE_INFO("\"%s\" has been successfully sent to %s.", textPtr, destinationPtr);
    }

    le_sms_Delete(myMsg);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function simply sends a text message using Legato AT commands APIs.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsmo_SendMessageAT
(
    const char*   destinationPtr, ///< [IN] The destination number.
    const char*   textPtr         ///< [IN] The SMS message text.
)
{
    le_result_t res;
    le_atClient_CmdRef_t cmdRef;
    le_atClient_DeviceRef_t DevRef;
    const uint32_t timeOut = 10000;
    char cmgs[100];
    char cmgf[12];

    int fd = open("/dev/ttyAT", O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
    {
        LE_ERROR("Failed to open device.");
        return LE_FAULT;
    }

    DevRef = le_atClient_Start(fd);

    // Configure SMS message in text mode

    memset(cmgf, 0, 12);
    snprintf(cmgf, 12, "AT+CMGF=1");

    cmdRef = le_atClient_Create();
    if (cmdRef == NULL)
    {
        LE_ERROR("cmdRef NULL.");
        return LE_FAULT;
    }

    res = le_atClient_SetCommand(cmdRef, cmgf);
    if (res != LE_OK)
    {
        LE_ERROR("SetCommand failed.");
        return LE_FAULT;
    }


    res = le_atClient_SetDevice(cmdRef, DevRef);
    if (res != LE_OK)
    {
        LE_ERROR("SetDevice failed.");
        return LE_FAULT;
    }

    res = le_atClient_SetTimeout(cmdRef, timeOut);
    if (res != LE_OK)
    {
        LE_ERROR("SetTimeout failed.");
        return LE_FAULT;
    }

    res = le_atClient_SetFinalResponse(cmdRef, "OK");
    if (res != LE_OK)
    {
        LE_ERROR("SetFinalResponse failed.");
        return LE_FAULT;
    }

    res = le_atClient_Send(cmdRef);
    if (res != LE_OK)
    {
        LE_ERROR("Send failed.");
        return LE_FAULT;
    }

    // Delete reference
    res = le_atClient_Delete(cmdRef);
    if (res != LE_OK)
    {
        LE_ERROR("Delete failed.");
        return LE_FAULT;
    }

    // Send the text message

    memset(cmgs, 0, 100);
    snprintf(cmgs, 100, "AT+CMGS=\"%s\"", destinationPtr);

    cmdRef = le_atClient_Create();
    if (cmdRef == NULL)
    {
        LE_ERROR("cmdRef NULL.");
        return LE_FAULT;
    }

    res = le_atClient_SetCommand(cmdRef, cmgs);
    if (res != LE_OK)
    {
        LE_ERROR("SetCommand failed.");
        return LE_FAULT;
    }

    res = le_atClient_SetText(cmdRef, textPtr);
    if (res != LE_OK)
    {
        LE_ERROR("SetText failed.");
        return LE_FAULT;
    }

    res = le_atClient_SetDevice(cmdRef, DevRef);
    if (res != LE_OK)
    {
        LE_ERROR("SetDevice failed.");
        return LE_FAULT;
    }

    res = le_atClient_SetTimeout(cmdRef, timeOut);
    if (res != LE_OK)
    {
        LE_ERROR("SetTimeout failed.");
        return LE_FAULT;
    }

    res = le_atClient_SetFinalResponse(cmdRef, "OK");
    if (res != LE_OK)
    {
        LE_ERROR("SetFinalResponse failed.");
        return LE_FAULT;
    }

    res = le_atClient_Send(cmdRef);
    if (res != LE_OK)
    {
        LE_ERROR("Send failed.");
        return LE_FAULT;
    }

    // Delete reference
    res = le_atClient_Delete(cmdRef);
    if (res != LE_OK)
    {
        LE_ERROR("Delete failed.");
        return LE_FAULT;
    }

    // Stop device
    res = le_atClient_Stop(DevRef);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to stop device.");
        return LE_FAULT;
    }
    // Close the fd
    close(fd);

    return LE_OK;
}
//--------------------------------------------------------------------------------------------------
/**
 * Checks the contents of a text message for specific pre-defined commands. If command is recognized
 * a reply will be sent to satisfy the request.
 *
 */
//--------------------- -----------------------------------------------------------------------------
le_result_t decodeMsgRequest
(
    char* tel,
    char* text
)
{
    le_result_t res                                         = LE_OK;
    le_info_Reset_t ResetInformation                        = LE_INFO_RESET_UNKNOWN;
    char errText[LE_SMS_TEXT_MAX_BYTES]                     = {0};
    char versionBootPtr[LE_INFO_MAX_VERS_BYTES]             = {0};
    char versionFWPtr[LE_INFO_MAX_VERS_BYTES]               = {0};
    char modelDevice[LE_INFO_MAX_VERS_BYTES-1]              = {0};
    char imei[LE_INFO_IMEI_MAX_BYTES]                       = {0};
    char imeiSv[LE_INFO_IMEISV_MAX_BYTES]                   = {0};
    char meid[LE_INFO_MAX_MEID_BYTES]                       = {0};
    char esn[LE_INFO_MAX_ESN_BYTES]                         = {0};
    char mdn[LE_INFO_MAX_MDN_BYTES]                         = {0};
    char min[LE_INFO_MAX_MIN_BYTES]                         = {0};
    char nai[LE_INFO_MAX_NAI_BYTES]                         = {0};
    char manufacturerNameStr[LE_INFO_MAX_MFR_NAME_BYTES]    = {0};
    char priIdPn[LE_INFO_MAX_PRIID_PN_BYTES]                = {0};
    char priIdRev[LE_INFO_MAX_PRIID_REV_BYTES]              = {0};
    char skuId[LE_INFO_MAX_SKU_BYTES]                       = {0};
    char platformSerialNumberStr[LE_INFO_MAX_PSN_BYTES]     = {0};
    uint16_t prlVersion                                     =  0 ;
    bool PrlOnlyPreference                                  =  0 ;
    char ResetStr[LE_INFO_MAX_RESET_BYTES]                  = {0};

    if (strstr(text, "info firmware") || strstr(text, "Info firmware"))
    {
        res = le_info_GetFirmwareVersion(versionFWPtr, sizeof(versionFWPtr));
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get firmware version.", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, versionFWPtr, false);
    }
    else if (strstr(text, "info bootloader") || strstr(text, "Info bootloader"))
    {
        res = le_info_GetBootloaderVersion(versionBootPtr, sizeof(versionBootPtr));
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get bootloader version.", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, versionBootPtr, false);
    }
    else if (strstr(text, "info device model") || strstr(text, "Info device model"))
    {
        res = le_info_GetDeviceModel(modelDevice, sizeof(modelDevice));
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get target hardware platform identity.", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, modelDevice, false);
    }
    // using strcmp for imei because if we use strstr, it will also match with imeisv
    else if (strcmp(text, "info imei") == 0 || strcmp(text, "Info imei") == 0)
    {
        res = le_info_GetImei(imei, sizeof(imei));
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get International Mobile Equipment Identity (IMEI).", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, imei, false);
    }
    else if (strstr(text, "info imeisv") || strstr(text, "Info imeisv"))
    {
        res = le_info_GetImeiSv(imeiSv, sizeof(imeiSv));
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get International Mobile Equipment Identity software version number (IMEISV).", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, imeiSv, false);
    }
    else if (strstr(text, "info meid") || strstr(text, "Info meid"))
    {
        res = le_info_GetMeid(meid, sizeof(meid));
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get CDMA device Mobile Equipment Identifier (MEID).", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, meid, false);
    }
    else if (strstr(text, "info esn") || strstr(text, "Info esn"))
    {
        res = le_info_GetEsn(esn, sizeof(esn));
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get Electronic Serial Number (ESN) of the device.", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, esn, false);
    }
    else if (strstr(text, "info mdn") || strstr(text, "Info mdn"))
    {
        res = le_info_GetMdn(mdn, sizeof(mdn));
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get Mobile Directory Number (MDN) of the device.", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, mdn, false);
    }
    else if (strstr(text, "info min") || strstr(text, "Info min"))
    {
        res = le_info_GetMin(min, sizeof(min));
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get CDMA Mobile Identification Number (MIN).", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, min, false);
    }
    else if (strstr(text, "info prl version") || strstr(text, "Info prl version"))
    {
        res = le_info_GetPrlVersion(&prlVersion);
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get CDMA version of Preferred Roaming List (PRL).", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        int prlLength = snprintf(NULL, 0, "%d", prlVersion);
        char* buffer = malloc(prlLength + 1);
        snprintf(buffer, prlLength + 1, "%d", prlVersion);
        smsmo_SendMessage(tel, buffer, false);
        free(buffer);
    }
    else if (strstr(text, "info prl preference") || strstr(text, "Info prl preference"))
    {
        char buffer[8];
        res = le_info_GetPrlOnlyPreference(&PrlOnlyPreference);
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get CDMA Preferred Roaming List (PRL) only preferences status.", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        if (PrlOnlyPreference)
        {
            le_utf8_Copy(buffer, "True", sizeof(buffer), NULL);
        }
        else
        {
            le_utf8_Copy(buffer, "False", sizeof(buffer), NULL);
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, buffer, false);
    }
    else if (strstr(text, "info nai") || strstr(text, "Info nai"))
    {
        res = le_info_GetNai(nai, sizeof(nai));
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get CDMA Network Access Identifier (NAI) string.", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, nai, false);
    }
    else if (strstr(text, "info manufacturer") || strstr(text, "Info manufacturer"))
    {
        res = le_info_GetManufacturerName(manufacturerNameStr, LE_INFO_MAX_MFR_NAME_BYTES);
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get Manufacturer name.", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, manufacturerNameStr, false);
    }
    else if (strstr(text, "info pri") || strstr(text, "Info pri"))
    {
        char buffer[512] = {0};
        res = le_info_GetPriId(priIdPn, LE_INFO_MAX_PRIID_PN_BYTES, priIdRev, LE_INFO_MAX_PRIID_REV_BYTES);
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get Product Requirement Information Identifier (PRI ID) Part Number and the Revision Number.", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        le_utf8_Copy(buffer, "Part Number: ", sizeof(buffer), NULL);
        le_utf8_Append(buffer, priIdPn, sizeof(buffer), NULL);
        le_utf8_Append(buffer, "\n", sizeof(buffer), NULL);
        le_utf8_Append(buffer, "Revision Number: ", sizeof(buffer), NULL);
        le_utf8_Append(buffer, priIdRev, sizeof(buffer), NULL);
        smsmo_SendMessage(tel, buffer, false);
    }
    else if (strstr(text, "info sku") || strstr(text, "Info sku"))
    {
        res = le_info_GetSku(skuId, LE_INFO_MAX_SKU_BYTES);
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get product stock keeping unit number (SKU).", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, skuId, false);
    }
    else if (strstr(text, "info psn") || strstr(text, "Info psn"))
    {
        res = le_info_GetPlatformSerialNumber(platformSerialNumberStr, LE_INFO_MAX_PSN_BYTES);
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get Platform Serial Number (PSN) string.", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");
        smsmo_SendMessage(tel, platformSerialNumberStr, false);
    }
    else if (strstr(text, "info reset") || strstr(text, "Info reset"))
    {
        char buffer[512] = {0};
        uint64_t countUnexpected;
        uint64_t countExpected;
        res = le_info_GetResetInformation(&ResetInformation, ResetStr, sizeof(ResetStr));
        if (res != LE_OK)
        {
            le_utf8_Copy(errText, "Failed to get reset information.", sizeof(errText), NULL);
            LE_ERROR("%s", errText);
            smsmo_SendMessage(tel, errText, false);
            return LE_FAULT;
        }
        LE_INFO("Command processed.");

        // Formatting the output
        le_info_GetUnexpectedResetsCount(&countUnexpected);
        le_info_GetExpectedResetsCount(&countExpected);
        int countUnexpectedLength = snprintf(NULL, 0, "%llu", countUnexpected);
        char* countUnexpectedString = malloc(countUnexpectedLength + 1);
        snprintf(countUnexpectedString, countUnexpectedLength + 1, "%llu", countUnexpected);
        int countExpectedLength = snprintf(NULL, 0, "%llu", countExpected);
        char* countExpectedString = malloc(countExpectedLength + 1);
        snprintf(countExpectedString, countExpectedLength + 1, "%llu", countExpected);
        le_utf8_Append(buffer, ResetStr, sizeof(buffer), NULL);
        le_utf8_Append(buffer, "\n", sizeof(buffer), NULL);
        le_utf8_Append(buffer, "Unexpected Reset Count: ", sizeof(buffer), NULL);
        le_utf8_Append(buffer, countUnexpectedString, sizeof(buffer), NULL);
        le_utf8_Append(buffer, "\n", sizeof(buffer), NULL);
        le_utf8_Append(buffer, "Expected Reset Count: ", sizeof(buffer), NULL);
        le_utf8_Append(buffer, countExpectedString, sizeof(buffer), NULL);
        smsmo_SendMessage(tel, buffer, false);
        free(countExpectedString);
        free(countUnexpectedString);
    }
    else if (strstr(text, "info") || strstr(text, "Info"))
    {
        LE_ERROR("Unknown request!");
        smsmo_SendMessage(tel, "Unknown request!", false);
        return LE_FAULT;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SMS storage full message indication.
 *
 */
//--------------------- -----------------------------------------------------------------------------
static void StorageMessageHandler
(
    le_sms_Storage_t  storage,
    void*            contextPtr
)
{
    LE_INFO("A Full storage SMS message is received. Type of full storage %d", storage);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function installs an handler for message reception.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsmt_Receiver
(
    void
)
{
    RxHdlrRef = le_sms_AddRxMessageHandler(RxMessageHandler, NULL);
    if (!RxHdlrRef)
    {
        LE_ERROR("le_sms_AddRxMessageHandler has failed!");
        return LE_FAULT;
    }
    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function installs an handler for storage message indication.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t smsmt_MonitorStorage
(
    void
)
{
    FullStorageHdlrRef = le_sms_AddFullStorageEventHandler(StorageMessageHandler, NULL);
    if (!FullStorageHdlrRef)
    {
        LE_ERROR("le_sms_AddFullStorageEventHandler has failed!");
        return LE_FAULT;
    }

    else
    {
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function remove the handler for message reception.
 *
 */
//--------------------------------------------------------------------------------------------------
void smsmt_HandlerRemover
(
    void
)
{
    le_sms_RemoveRxMessageHandler(RxHdlrRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function remove the handler for storage message indication.
 *
 */
//--------------------------------------------------------------------------------------------------
void sms_StorageHandlerRemover
(
    void
)
{
    le_sms_RemoveFullStorageEventHandler(FullStorageHdlrRef);
}