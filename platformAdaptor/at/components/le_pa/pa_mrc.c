/** @file pa_mrc.c
 *
 * AT implementation of c_pa_mrc API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "le_atClient.h"

#include "pa_mrc.h"
#include "pa_utils_local.h"


//--------------------------------------------------------------------------------------------------
/**
 * Memory pool default value
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_REGSTATE_POOL_SIZE  1

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool reference
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t           RegStatePoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Unsolicited event identifier
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t              UnsolicitedEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Network registering event identifier
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t              NetworkRegEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Network registering mode
 */
//--------------------------------------------------------------------------------------------------
static pa_mrc_NetworkRegSetting_t RegNotification = PA_MRC_DISABLE_REG_NOTIFICATION;


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize pattern matching for unsolicited +CREG notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SubscribeUnsolCREG
(
    pa_mrc_NetworkRegSetting_t  mode ///< [IN] The selected Network registration mode.
)
{
    le_atClient_RemoveUnsolicitedResponseHandler(UnsolicitedEventId,"+CREG:");

    if ((mode==PA_MRC_ENABLE_REG_NOTIFICATION) || (mode==PA_MRC_ENABLE_REG_LOC_NOTIFICATION))
    {
        le_atClient_AddUnsolicitedResponseHandler(UnsolicitedEventId,
                                                  "+CREG:",
                                                  false);
    }

    RegNotification = mode;
}

//--------------------------------------------------------------------------------------------------
/**
 * The handler for a new Network Registration Notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CREGUnsolHandler
(
    void* reportPtr
)
{
    char*                 unsolPtr = reportPtr;
    uint32_t              numParam = 0;
    le_mrc_NetRegState_t* statePtr;

    numParam = pa_utils_CountAndIsolateLineParameters(unsolPtr);

    if (RegNotification == PA_MRC_ENABLE_REG_NOTIFICATION)
    {
        if (numParam == 2)
        {
            statePtr = le_mem_ForceAlloc(RegStatePoolRef);
            switch(atoi(pa_utils_IsolateLineParameter(unsolPtr,2)))
            {
                case 0:
                    *statePtr = LE_MRC_REG_NONE;
                    break;
                case 1:
                    *statePtr = LE_MRC_REG_HOME;
                    break;
                case 2:
                    *statePtr = LE_MRC_REG_SEARCHING;
                    break;
                case 3:
                    *statePtr = LE_MRC_REG_DENIED;
                    break;
                case 4:
                    *statePtr = LE_MRC_REG_UNKNOWN;
                    break;
                case 5:
                    *statePtr = LE_MRC_REG_ROAMING;
                    break;
                default:
                    *statePtr = LE_MRC_REG_UNKNOWN;
                    break;
            }
            LE_DEBUG("Send Event with state %d",*statePtr);
            le_event_ReportWithRefCounting(NetworkRegEventId,statePtr);
        }
        else
        {
            LE_WARN("this Response pattern is not expected -%s-",unsolPtr);
        }
    }
    else if (RegNotification == PA_MRC_ENABLE_REG_LOC_NOTIFICATION)
    {
        if (numParam == 5)
        {
            statePtr = le_mem_ForceAlloc(RegStatePoolRef);
            switch(atoi(pa_utils_IsolateLineParameter(unsolPtr,2)))
            {
                case 0:
                    *statePtr = LE_MRC_REG_NONE;
                    break;
                case 1:
                    *statePtr = LE_MRC_REG_HOME;
                    break;
                case 2:
                    *statePtr = LE_MRC_REG_SEARCHING;
                    break;
                case 3:
                    *statePtr = LE_MRC_REG_DENIED;
                    break;
                case 4:
                    *statePtr = LE_MRC_REG_UNKNOWN;
                    break;
                case 5:
                    *statePtr = LE_MRC_REG_ROAMING;
                    break;
                default:
                    *statePtr = LE_MRC_REG_UNKNOWN;
                    break;
            }
            LE_DEBUG("Send Event with state %d",*statePtr);
            le_event_ReportWithRefCounting(NetworkRegEventId,statePtr);
        }
        else
        {
            LE_WARN("this Response pattern is not expected -%s-",unsolPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function set text or number mode for get the network operator.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetOperatorTextMode
(
    bool    text
)
{
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (text == true)
    {
        res = le_atClient_SetCommandAndSend(&cmdRef,
                                            "AT+COPS=3,0",
                                            "",
                                            DEFAULT_AT_RESPONSE,
                                            DEFAULT_AT_CMD_TIMEOUT);
    }
    else
    {
        res = le_atClient_SetCommandAndSend(&cmdRef,
                                            "AT+COPS=3,2",
                                            "",
                                            DEFAULT_AT_RESPONSE,
                                            DEFAULT_AT_CMD_TIMEOUT);
    }
    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,finalResponse,LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if ((res != LE_OK) || (strcmp(finalResponse,"OK") != 0))
    {
        LE_ERROR("Failed to get the response");
    }

    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets information of the Network registration.
 *
 * @return LE_FAULT        The function failed.
 * @return LE_TIMEOUT      No response was received.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetNetworkReg
(
    bool        mode,      ///< true -> mode, false -> state
    int32_t*    valuePtr   ///< value that will be return
)
{
    le_atClient_CmdRef_t cmdRef   = NULL;
    le_result_t          res      = LE_FAULT;
    char*                tokenPtr = NULL;
    char*                savePtr  = NULL;
    char intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!valuePtr)
    {
        LE_WARN("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CREG?",
                                        "+CREG:",
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
        tokenPtr = strtok_r(intermediateResponse+strlen("+CREG: "), ",", &savePtr);
        if (mode)
        {
            switch(atoi(tokenPtr))
            {
                case 0:
                    *valuePtr = PA_MRC_DISABLE_REG_NOTIFICATION;
                    break;
                case 1:
                    *valuePtr = PA_MRC_ENABLE_REG_NOTIFICATION;
                    break;
                case 2:
                    *valuePtr = PA_MRC_ENABLE_REG_LOC_NOTIFICATION;
                    break;
                default:
                    LE_ERROR("Failed to get the response");
                    break;
            }
        }
        else
        {
            tokenPtr = strtok_r(NULL, ",", &savePtr);
            switch(atoi(tokenPtr))
            {
                case 0:
                    *valuePtr = LE_MRC_REG_NONE;
                    break;
                case 1:
                    *valuePtr = LE_MRC_REG_HOME;
                    break;
                case 2:
                    *valuePtr = LE_MRC_REG_SEARCHING;
                    break;
                case 3:
                    *valuePtr = LE_MRC_REG_DENIED;
                    break;
                case 4:
                    *valuePtr = LE_MRC_REG_UNKNOWN;
                    break;
                case 5:
                    *valuePtr = LE_MRC_REG_ROAMING;
                    break;
                default:
                    *valuePtr = LE_MRC_REG_UNKNOWN;
                    break;
            }
        }
    }
    le_atClient_Delete(cmdRef);
    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the mrc module
 *
 * @return LE_FAULT         The function failed to initialize the module.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_Init
(
    void
)
{
    UnsolicitedEventId = le_event_CreateId("MrcUnsolEventId",LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    NetworkRegEventId = le_event_CreateIdWithRefCounting("NetworkRegEventId");

    le_event_AddHandler("MrcUnsolHandler",UnsolicitedEventId  ,CREGUnsolHandler);

    RegStatePoolRef = le_mem_CreatePool("RegStatePool",sizeof(le_mrc_NetRegState_t));
    RegStatePoolRef = le_mem_ExpandPool(RegStatePoolRef,DEFAULT_REGSTATE_POOL_SIZE);

    SubscribeUnsolCREG(PA_MRC_ENABLE_REG_LOC_NOTIFICATION);

    pa_mrc_GetNetworkRegConfig(&RegNotification);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the power of the Radio Module.
 *
 * @return LE_BAD_PARAMETER Bad power mode specified.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetRadioPower
(
    le_onoff_t    power   ///< [IN] The power state.
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    le_result_t          res    = LE_FAULT;
    le_atClient_CmdRef_t cmdRef = NULL;

    if (power == LE_ON)
    {
        snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CFUN=1");
    }
    else if (power == LE_OFF)
    {
        snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CFUN=4");
    }
    else
    {
        return LE_BAD_PARAMETER;
    }

    le_atClient_SetCommandAndSend(&cmdRef,
                                  command,
                                  "",
                                  DEFAULT_AT_RESPONSE,
                                  DEFAULT_AT_CMD_TIMEOUT);

    res = le_atClient_GetFinalResponse(cmdRef,finalResponse,LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if ((res != LE_OK) || (strcmp(finalResponse,"OK") != 0))
    {
        LE_ERROR("Failed to get the response");
    }
    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Radio Module power state.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetRadioPower
(
     le_onoff_t*    powerPtr   ///< [OUT] The power state.
)
{
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CFUN?",
                                        "+CFUN:",
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
        LE_DEBUG("Failed to get the response");
    }
    else
    {
        if(atoi(&intermediateResponse[strlen("+CFUN: ")]) == 1)
        {
            *powerPtr = LE_ON;
        }
        else
        {
            *powerPtr = LE_OFF;
        }
    }
    le_atClient_Delete(cmdRef);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Radio Access Technology change handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_mrc_SetRatChangeHandler
(
    pa_mrc_RatChangeHdlrFunc_t handlerFuncPtr ///< [IN] The handler function.
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for Radio Access Technology change
 * handling.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_RemoveRatChangeHandler
(
    le_event_HandlerRef_t handlerRef
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Network registration state handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_mrc_AddNetworkRegHandler
(
    pa_mrc_NetworkRegHdlrFunc_t regStateHandler ///< [IN] The handler function to handle the
                                                 ///       Network registration state.
)
{
    LE_DEBUG("Set new Radio Control handler");

    if (regStateHandler == NULL)
    {
        LE_FATAL("new Radio Control handler is NULL");
    }

    return (le_event_AddHandler("NewRegStateHandler",
                                NetworkRegEventId,
                                (le_event_HandlerFunc_t) regStateHandler));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for Network registration state handling.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_RemoveNetworkRegHandler
(
    le_event_HandlerRef_t handlerRef
)
{
    le_event_RemoveHandler(handlerRef);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function configures the Network registration setting.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_ConfigureNetworkReg
(
    pa_mrc_NetworkRegSetting_t  setting ///< [IN] The selected Network registration setting.
)
{
    char                 command[LE_ATCLIENT_CMD_SIZE_MAX_LEN];
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;

    snprintf(command,LE_ATCLIENT_CMD_SIZE_MAX_LEN,"AT+CREG=%d", setting);

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        command,
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);
    if (res == LE_OK)
    {
        le_atClient_Delete(cmdRef);
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Network registration setting.
 *
 * @return LE_BAD_PARAMETER Bad parameter passed to the function
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegConfig
(
    pa_mrc_NetworkRegSetting_t* settingPtr   ///< [OUT] The selected Network registration setting.
)
{
    int32_t val;

    if (!settingPtr)
    {
        LE_WARN("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    GetNetworkReg(true,&val);
   *settingPtr = (pa_mrc_NetworkRegSetting_t)val;
    RegNotification = val;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the platform specific network registration error code.
 *
 * @return the platform specific registration error code
 *
 */
//--------------------------------------------------------------------------------------------------
int32_t pa_mrc_GetPlatformSpecificRegistrationErrorCode
(
    void
)
{
    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Network registration state.
 *
 * @return LE_BAD_PARAMETER Bad parameter passed to the function
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegState
(
    le_mrc_NetRegState_t* statePtr  ///< [OUT] The network registration state.
)
{
    int32_t val;

    if (!statePtr)
    {
        LE_WARN("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    GetNetworkReg(false,&val);
   *statePtr = (le_mrc_NetRegState_t)val;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Signal Strength information.
 *
 * @return LE_BAD_PARAMETER Bad parameter passed to the function
 * @return LE_OUT_OF_RANGE  The signal strength values are not known or not detectable.
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetSignalStrength
(
    int32_t*          rssiPtr    ///< [OUT] The received signal strength (in dBm).
)
{
    le_atClient_CmdRef_t cmdRef   = NULL;
    le_result_t          res      = LE_FAULT;
    char*                tokenPtr = NULL;
    char*                rest     = NULL;
    char*                savePtr  = NULL;
    int32_t              val      = 0;
    char intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (!rssiPtr)
    {
        LE_WARN("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CSQ",
                                        "+CSQ:",
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
        rest     = intermediateResponse+strlen("+CSQ: ");
        tokenPtr = strtok_r(rest, ",", &savePtr);
        val      = atoi(tokenPtr);

        if (val == 99)
        {
            LE_WARN("Quality signal not detectable");
            res = LE_OUT_OF_RANGE;
        }
        else
        {
            *rssiPtr = (-113+(2*val));
            res = LE_OK;
        }
    }
    le_atClient_Delete(cmdRef);
    return res;

}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current network information.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the current network name can't fit in nameStr
 *      - LE_FAULT on any other failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetCurrentNetwork
(
    char*       nameStr,               ///< [OUT] the home network Name
    size_t      nameStrSize,           ///< [IN]  the nameStr size
    char*       mccStr,                ///< [OUT] the mobile country code
    size_t      mccStrNumElements,     ///< [IN]  the mccStr size
    char*       mncStr,                ///< [OUT] the mobile network code
    size_t      mncStrNumElements      ///< [IN]  the mncStr size
)
{
    le_atClient_CmdRef_t cmdRef   = NULL;
    le_result_t          res      = LE_FAULT;
    char*                tokenPtr = NULL;
    char*                savePtr  = NULL;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

     if (nameStr != NULL)
    {
        res = SetOperatorTextMode(1);
    }
    else if ((mccStr != NULL) && (mncStr != NULL))
    {
        res = SetOperatorTextMode(0);
    }
    else
    {
        LE_DEBUG("One parameter is NULL");
        return LE_BAD_PARAMETER;
    }

    if (res != LE_OK)
    {
        LE_ERROR("Failed to set the command");
        return res;
    }

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+COPS?",
                                        "+COPS:",
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
        if (nameStr != NULL)
        {
            // Cut the string for keep just the phone number
            tokenPtr = strtok_r(intermediateResponse, "\"", &savePtr);
            tokenPtr = strtok_r(NULL, "\"", &savePtr);

            strncpy(nameStr, tokenPtr, nameStrSize);
        }
        else
        {
            // Cut the string for keep just the phone number
            tokenPtr = strtok_r(intermediateResponse, ",", &savePtr);
            tokenPtr = strtok_r(NULL, ",", &savePtr);
            tokenPtr = strtok_r(NULL, ",", &savePtr);

            strncpy(mccStr, tokenPtr, mccStrNumElements);
            strncpy(mncStr, tokenPtr + 3, mncStrNumElements);

            SetOperatorTextMode(1);
        }
    }

    le_atClient_Delete(cmdRef);
    return res;

}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of Scan Information
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_DeleteScanInformation
(
    le_dls_List_t *scanInformationListPtr ///< [IN] list of pa_mrc_ScanInformation_t
)
{
    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to perform a network scan.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_COMM_ERROR    Radio link failure occurred.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_PerformNetworkScan
(
    le_mrc_RatBitMask_t ratMask,               ///< [IN] The network mask
    pa_mrc_ScanType_t   scanType,              ///< [IN] the scan type
    le_dls_List_t*      scanInformationListPtr ///< [OUT] list of pa_mrc_ScanInformation_t
)
{
    LE_WARN("Network scan is not supported");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to perform a network scan.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the operator name would not fit in buffer
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetScanInformationName
(
    pa_mrc_ScanInformation_t* scanInformationPtr,   ///< [IN] The scan information
    char*                     namePtr,              ///< [OUT] Name of operator
    size_t                    nameSize              ///< [IN] The size in bytes of the namePtr buffer
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the number of preferred operators present in the list.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_CountPreferredOperators
(
    bool      plmnStatic,   ///< [IN] Include Static preferred Operators.
    bool      plmnUser,     ///< [IN] Include Users preferred Operators.
    int32_t*  nbItemPtr     ///< [OUT] number of Preferred operator found if success.
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current preferred operators.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if Preferred operator list is not available
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetPreferredOperators
(
    pa_mrc_PreferredNetworkOperator_t*   preferredOperatorPtr,
                          ///< [IN/OUT] The preferred operators pointer.
    bool     plmnStatic,  ///< [IN] Include Static preferred Operators.
    bool     plmnUser,    ///< [IN] Include Users preferred Operators.
    int32_t* nbItemPtr    ///< [IN/OUT] number of Preferred operator to find (in) and written (out).
)
{
    return LE_NOT_FOUND;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to apply the preferred list into the modem
 *
 * @return
 *      - LE_OK             on success
 *      - LE_FAULT          for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SavePreferredOperators
(
    le_dls_List_t* preferredOperatorsListPtr ///< [IN] List of preferred network operator
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register on a mobile network [mcc;mnc]
 *
 * @return LE_FAULT  The function failed to register.
 * @return LE_OK            The function succeeded to register,
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_RegisterNetwork
(
    const char *mccPtr,   ///< [IN] Mobile Country Code
    const char *mncPtr    ///< [IN] Mobile Network Code
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register automatically on network
 *
 * @return
 *      - LE_OK             on success
 *      - LE_FAULT          for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetAutomaticNetworkRegistration
(
    void
)
{
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CREG=1",
                                        "",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command !");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,
                                       finalResponse,
                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if ((res != LE_OK) || (strcmp(finalResponse, "OK") != 0))
    {
        LE_ERROR("Function failed !");
        le_atClient_Delete(cmdRef);
        return LE_FAULT;
    }

    LE_DEBUG("Set automatic network registration.");
    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get current registration mode
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetNetworkRegistrationMode
(
    bool*   isManualPtr,  ///< [OUT] true if the scan mode is manual, false if it is automatic.
    char*   mccPtr,       ///< [OUT] Mobile Country Code
    size_t  mccPtrSize,   ///< [IN] mccPtr buffer size
    char*   mncPtr,       ///< [OUT] Mobile Network Code
    size_t  mncPtrSize    ///< [IN] mncPtr buffer size
)
{
    char*                tokenPtr   = NULL;
    char*                restPtr    = NULL;
    char*                savePtr    = NULL;
    le_atClient_CmdRef_t cmdRef     = NULL;
    le_result_t          res        = LE_FAULT;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+CREG?",
                                        "+CREG:",
                                        DEFAULT_AT_RESPONSE,
                                        DEFAULT_AT_CMD_TIMEOUT);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to send the command !");
        return res;
    }

    res = le_atClient_GetFinalResponse(cmdRef,
                                       finalResponse,
                                       LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if ((res != LE_OK) || (strcmp(finalResponse, "OK") != 0))
    {
        LE_ERROR("Function failed !");
        le_atClient_Delete(cmdRef);
        return LE_FAULT;
    }

    res = le_atClient_GetFirstIntermediateResponse(cmdRef,
                                                   intermediateResponse,
                                                   LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES);
    if (res != LE_OK)
    {
        LE_ERROR("Failed to get the response !");
        le_atClient_Delete(cmdRef);
        return res;
    }

    restPtr  = intermediateResponse+strlen("+CREG: ");
    tokenPtr = strtok_r(restPtr, ",", &savePtr);

    if (atoi(tokenPtr) == 1)
    {
        *isManualPtr = false;
    }
    else
    {
        *isManualPtr = true;
    }

    res = pa_mrc_GetCurrentNetwork(NULL, 0, mccPtr, mccPtrSize, mncPtr, mncPtrSize);

    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the Radio Access Technology.
 *
 * @return LE_FAULT The function failed to get the Signal strength information.
 * @return LE_OK    The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetRadioAccessTechInUse
(
    le_mrc_Rat_t*   ratPtr    ///< [OUT] The Radio Access Technology.
)
{
    le_atClient_CmdRef_t cmdRef  = NULL;
    le_result_t          res     = LE_FAULT;
    char*                bandPtr = NULL;
    int                  bitMask = 0;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+KBND?",
                                        "+KBND:",
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
        bandPtr = intermediateResponse+strlen("+KBND: ");
        bitMask = atoi(bandPtr);

        if ((bitMask >= 1) && (bitMask <= 8))
        {
            *ratPtr = LE_MRC_RAT_GSM;
        }
        else if ((bitMask >= 10) && (bitMask <= 200))
        {
            *ratPtr = LE_MRC_RAT_UMTS;
        }
        else
        {
            *ratPtr = LE_MRC_RAT_UNKNOWN;
        }
    }
    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Radio Access Technology Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetRatPreferences
(
    le_mrc_RatBitMask_t ratMask ///< [IN] A bit mask to set the Radio Access Technology preferences.
)
{
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    if (ratMask == LE_MRC_BITMASK_RAT_GSM)
    {
        res = le_atClient_SetCommandAndSend(&cmdRef,
                                            "AT+KSRAT=1",
                                            "",
                                            DEFAULT_AT_RESPONSE,
                                            DEFAULT_AT_CMD_TIMEOUT);
    }
    else if (ratMask == LE_MRC_BITMASK_RAT_UMTS)
    {
        res = le_atClient_SetCommandAndSend(&cmdRef,
                                            "AT+KSRAT=2",
                                            "",
                                            DEFAULT_AT_RESPONSE,
                                            DEFAULT_AT_CMD_TIMEOUT);
    }
    else if (ratMask == LE_MRC_BITMASK_RAT_ALL)
    {
        res = le_atClient_SetCommandAndSend(&cmdRef,
                                            "AT+KSRAT=4",
                                            "",
                                            DEFAULT_AT_RESPONSE,
                                            DEFAULT_AT_CMD_TIMEOUT);
    }
    else
    {
        LE_ERROR("Impossible to set the Radio Access technology");
        return LE_FAULT;
    }

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
    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Rat Automatic Radio Access Technology Preference
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetAutomaticRatPreference
(
    void
)
{
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+KSRAT=4",
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
    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Radio Access Technology Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetRatPreferences
(
    le_mrc_RatBitMask_t* ratMaskPtr ///< [OUT] A bit mask to get the Radio Access Technology
                                    ///<  preferences.
)
{
    le_atClient_CmdRef_t cmdRef = NULL;
    le_result_t          res    = LE_FAULT;
    char*                ratPtr = NULL;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+KSRAT?",
                                        "+KSRAT:",
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
        LE_DEBUG("Failed to get the response");
    }
    else
    {
        ratPtr = intermediateResponse+strlen("+KSRAT: ");

        switch (atoi(ratPtr))
        {
        case 1:
            *ratMaskPtr = LE_MRC_BITMASK_RAT_GSM;
            break;
        case 2:
            *ratMaskPtr = LE_MRC_BITMASK_RAT_UMTS;
            break;
        case 3:
            *ratMaskPtr = LE_MRC_BITMASK_RAT_GSM;
            break;
        case 4:
            *ratMaskPtr = LE_MRC_BITMASK_RAT_ALL;
            break;
        default:
            LE_ERROR("An error occurred");
            res = LE_FAULT;
            break;
        }
    }
    le_atClient_Delete(cmdRef);
    return res;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the Band Preferences
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetBandPreferences
(
    le_mrc_BandBitMask_t bands ///< [IN] A bit mask to set the Band preferences.
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Band Preferences
 *
 * @return
 * - LE_FAULT           on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetBandPreferences
(
    le_mrc_BandBitMask_t* bandsPtr ///< [OUT] A bit mask to get the Band preferences.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the LTE Band Preferences
 *
 * @return
 * - LE_FAULT           on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetLteBandPreferences
(
    le_mrc_LteBandBitMask_t bands ///< [IN] A bit mask to set the LTE Band preferences.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the LTE Band Preferences
 *
 * @return
 * - LE_FAULT           on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetLteBandPreferences
(
    le_mrc_LteBandBitMask_t* bandsPtr ///< [OUT] A bit mask to get the LTE Band preferences.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the TD-SCDMA Band Preferences
 *
 * @return
 * - LE_FAULT           on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetTdScdmaBandPreferences
(
    le_mrc_TdScdmaBandBitMask_t bands ///< [IN] A bit mask to set the TD-SCDMA Band Preferences.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the TD-SCDMA Band Preferences
 *
 * @return
 * - LE_FAULT           on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetTdScdmaBandPreferences
(
    le_mrc_TdScdmaBandBitMask_t* bandsPtr ///< [OUT] A bit mask to set the TD-SCDMA Band
                                          ///<  preferences.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function retrieves the Neighboring Cells information.
 * Each cell information is queued in the list specified with the IN/OUT parameter.
 * Neither add nor remove of elements in the list can be done outside this function.
 *
 * @return LE_FAULT          The function failed to retrieve the Neighboring Cells information.
 * @return a positive value  The function succeeded. The number of cells which the information have
 *                           been retrieved.
 */
//--------------------------------------------------------------------------------------------------
int32_t pa_mrc_GetNeighborCellsInfo
(
    le_dls_List_t* cellInfoListPtr  ///< [OUT] The Neighboring Cells information.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete the list of neighboring cells information.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_DeleteNeighborCellsInfo
(
    le_dls_List_t* cellInfoListPtr   ///< [IN] list of pa_mrc_CellInfo_t
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function measures the Signal metrics.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_MeasureSignalMetrics
(
    pa_mrc_SignalMetrics_t* metricsPtr    ///< [OUT] The signal metrics.
)
{
    le_mrc_Rat_t  rat;
    int32_t       signal;

    metricsPtr->rat = pa_mrc_GetRadioAccessTechInUse(&rat);
    metricsPtr->ss  = pa_mrc_GetSignalStrength(&signal);
    metricsPtr->er  = 0xFFFFFFFF;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for Signal Strength change handling.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_mrc_AddSignalStrengthIndHandler
(
    pa_mrc_SignalStrengthIndHdlrFunc_t ssIndHandler, ///< [IN] The handler function to handle the
                                                     ///        Signal Strength change indication.
    void*                              contextPtr    ///< [IN] The context to be given to the handler.
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to unregister the handler for Signal Strength change handling.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void pa_mrc_RemoveSignalStrengthIndHandler
(
    le_event_HandlerRef_t handlerRef
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set and activate the signal strength thresholds for signal
 * strength indications
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_SetSignalStrengthIndThresholds
(
    le_mrc_Rat_t rat,                 ///< Radio Access Technology
    int32_t      lowerRangeThreshold, ///< [IN] lower-range threshold in dBm
    int32_t      upperRangeThreshold  ///< [IN] upper-range strength threshold in dBm
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the serving cell Identifier.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetServingCellId
(
    uint32_t* cellIdPtr ///< [OUT] main Cell Identifier.
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Tracking Area Code of the serving cell.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetServingCellLteTracAreaCode
(
    uint16_t* tacPtr ///< [OUT] Tracking Area Code of the serving cell.
)
{
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the Location Area Code of the serving cell.
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetServingCellLocAreaCode
(
    uint32_t* lacPtr ///< [OUT] Location Area Code of the serving cell.
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Band capabilities
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetBandCapabilities
(
    le_mrc_BandBitMask_t* bandsPtr ///< [OUT] A bit mask to get the Band capabilities.
)
{
    le_atClient_CmdRef_t cmdRef  = NULL;
    le_result_t          res     = LE_FAULT;
    char*                bandPtr = NULL;
    le_mrc_BandBitMask_t bands   = 0;
    int                  bitMask = 0;
    char                 intermediateResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
    char                 finalResponse[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];

    res = le_atClient_SetCommandAndSend(&cmdRef,
                                        "AT+KBND?",
                                        "+KBND:",
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
        bandPtr = intermediateResponse+strlen("+KBND: ");
        bitMask = atoi(bandPtr);

        if (bitMask & 0x00)
        {
            LE_ERROR("Band capabilities not available !");
            res = LE_FAULT;
        }
        if (bitMask == 1)
        {
            bands |= LE_MRC_BITMASK_BAND_GSM_850;
        }
        if (bitMask == 2)
        {
            bands |= LE_MRC_BITMASK_BAND_EGSM_900;
        }
        if (bitMask == 4)
        {
            bands |= LE_MRC_BITMASK_BAND_GSM_DCS_1800;
        }
        if (bitMask == 8)
        {
            bands |= LE_MRC_BITMASK_BAND_GSM_PCS_1900;
        }
        if (bitMask == 10)
        {
            bands |= LE_MRC_BITMASK_BAND_WCDMA_EU_J_CH_IMT_2100;
        }
        if (bitMask == 20)
        {
            bands |= LE_MRC_BITMASK_BAND_WCDMA_US_PCS_1900;
        }
        if (bitMask == 40)
        {
            bands |= LE_MRC_BITMASK_BAND_WCDMA_US_850;
        }
        if (bitMask == 80)
        {
            bands |= LE_MRC_BITMASK_BAND_WCDMA_J_800;
        }
        if (bitMask == 100)
        {
            bands |= LE_MRC_BITMASK_BAND_WCDMA_EU_J_900;
        }
        if (bitMask == 200)
        {
            bands |= LE_MRC_BITMASK_BAND_WCDMA_J_800;
        }

        if (bandsPtr)
        {
            *bandsPtr = bands;
        }
    }

    le_atClient_Delete(cmdRef);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the LTE Band capabilities
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetLteBandCapabilities
(
    le_mrc_LteBandBitMask_t* bandsPtr ///< [OUT] Bit mask to get the LTE Band capabilities.
)
{
    LE_WARN("LTE not available");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the TD-SCDMA Band capabilities
 *
 * @return
 * - LE_OK              on success
 * - LE_FAULT           on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mrc_GetTdScdmaBandCapabilities
(
    le_mrc_TdScdmaBandBitMask_t* bandsPtr ///< [OUT] Bit mask to get the TD-SCDMA Band capabilities.
)
{
    LE_WARN("CDMA not available");
    return LE_FAULT;
}
