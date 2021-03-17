/**
 * @file atProxy.c
 *
 * AT Proxy interface implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "atProxy.h"
#include "atProxyCmdHandler.h"
#include "pa_port.h"
#include "pa_atProxy.h"

#define AT_PROXY_CMD_REGISTRY_IMPL   1
#include "atProxyCmdRegistry.h"

//--------------------------------------------------------------------------------------------------
/**
 * Number of standard error strings defined in 3GPP TS 27.007 9.2 and 3GPP TS 27.005 3.2.5
 */
//--------------------------------------------------------------------------------------------------
#define STD_ERROR_CODE_SIZE       512

//--------------------------------------------------------------------------------------------------
/**
 * A magic number used to convert between command index and reference
 * This macro is temporarily needed to adapt static command registration to le_atServer.api, and
 * can be removed when change le_atServer_AddCommandHandler() in le_atServer.api to use command
 * index instead of reference.
 */
//--------------------------------------------------------------------------------------------------
#define AT_PROXY_CMD_MAGIC_NUMBER   0xF0000001

//--------------------------------------------------------------------------------------------------
/**
 * Convert command index to reference
 *
 * This macro is temporarily needed to adapt static command registration to le_atServer.api, and
 * can be removed when change le_atServer_AddCommandHandler() in le_atServer.api to use command
 * index instead of reference.
 */
//--------------------------------------------------------------------------------------------------
#define AT_PROXY_CONVERT_IND2REF(ind)   \
    ((le_atServer_CmdRef_t)(((ind) << 1) | AT_PROXY_CMD_MAGIC_NUMBER))


//--------------------------------------------------------------------------------------------------
/**
 * Convert command reference to index
 *
 * This macro is temporarily needed to adapt static command registration to le_atServer.api, and
 * can be removed when change le_atServer_AddCommandHandler() in le_atServer.api to use command
 * index instead of reference.
 */
//--------------------------------------------------------------------------------------------------
#define AT_PROXY_CONVERT_REF2IND(ref)    \
    (((uint32_t)(ref) & (~AT_PROXY_CMD_MAGIC_NUMBER)) >> 1)

//--------------------------------------------------------------------------------------------------
/**
 * Static map for AT Command references
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(AtCmdRefMap, AT_CMD_MAX);

//--------------------------------------------------------------------------------------------------
/**
 * Map for AT commands
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t  AtCmdRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Map for AT Command Sessions
 */
//--------------------------------------------------------------------------------------------------
extern le_ref_MapRef_t  atCmdSessionRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Pre-formatted strings corresponding to AT commands +CME error codes
 * (see 3GPP TS 27.007 9.2)
 *
 */
//--------------------------------------------------------------------------------------------------
const char* const CmeErrorCodes[STD_ERROR_CODE_SIZE] =
{
    ///< 3GPP TS 27.007 §9.2.1: General errors
    [0]   =   "Phone failure",
    [1]   =   "No connection to phone",
    [2]   =   "Phone-adaptor link reserved",
    [3]   =   "Operation not allowed",
    [4]   =   "Operation not supported",
    [5]   =   "PH-SIM PIN required",
    [6]   =   "PH-FSIM PIN required",
    [7]   =   "PH-FSIM PUK required",
    [10]  =   "SIM not inserted",
    [11]  =   "SIM PIN required",
    [12]  =   "SIM PUK required",
    [13]  =   "SIM failure",
    [14]  =   "SIM busy",
    [15]  =   "SIM wrong",
    [16]  =   "Incorrect password",
    [17]  =   "SIM PIN2 required",
    [18]  =   "SIM PUK2 required",
    [20]  =   "Memory full",
    [21]  =   "Invalid index",
    [22]  =   "Not found",
    [23]  =   "Memory failure",
    [24]  =   "Text string too long",
    [25]  =   "Invalid characters in text string",
    [26]  =   "Dial string too long",
    [27]  =   "Invalid characters in dial string",
    [30]  =   "No network service",
    [31]  =   "Network timeout",
    [32]  =   "Network not allowed - emergency calls only",
    [40]  =   "Network personalization PIN required",
    [41]  =   "Network personalization PUK required",
    [42]  =   "Network subset personalization PIN required",
    [43]  =   "Network subset personalization PUK required",
    [44]  =   "Service provider personalization PIN required",
    [45]  =   "Service provider personalization PUK required",
    [46]  =   "Corporate personalization PIN required",
    [47]  =   "Corporate personalization PUK required",
    [48]  =   "Hidden key required",
    [49]  =   "EAP method not supported",
    [50]  =   "Incorrect parameters",
    [51]  =   "Command implemented but currently disabled",
    [52]  =   "Command aborted by user",
    [53]  =   "Not attached to network due to MT functionality restrictions",
    [54]  =   "Modem not allowed - MT restricted to emergency calls only",
    [55]  =   "Operation not allowed because of MT functionality restrictions",
    [56]  =   "Fixed dial number only allowed - called number is not a fixed dial number",
    [57]  =   "Temporarily out of service due to other MT usage",
    [58]  =   "Language/alphabet not supported",
    [59]  =   "Unexpected data value",
    [60]  =   "System failure",
    [61]  =   "Data missing",
    [62]  =   "Call barred",
    [63]  =   "Message waiting indication subscription failure",
    [100] =   "Unknown",

    ///< 3GPP TS 27.007 §9.2.2.1: GPRS and EPS errors related to a failure to perform an attach
    [103] =   "Illegal MS",
    [106] =   "Illegal ME",
    [107] =   "GPRS services not allowed",
    [108] =   "GPRS services and non-GPRS services not allowed",
    [111] =   "PLMN not allowed",
    [112] =   "Location area not allowed",
    [113] =   "Roaming not allowed in this location area",
    [114] =   "GPRS services not allowed in this PLMN",
    [115] =   "No Suitable Cells In Location Area",
    [122] =   "Congestion",
    [125] =   "Not authorized for this CSG",
    [172] =   "Semantically incorrect message",
    [173] =   "Mandatory information element error",
    [174] =   "Information element non-existent or not implemented",
    [175] =   "Conditional IE error",
    [176] =   "Protocol error, unspecified",

    ///< 3GPP TS 27.007 §9.2.2.2: GPRS and EPS errors related to a failure to activate a context
    [177] =   "Operator Determined Barring",
    [126] =   "Insufficient resources",
    [127] =   "Missing or unknown APN",
    [128] =   "Unknown PDP address or PDP type",
    [129] =   "User authentication failed",
    [130] =   "Activation rejected by GGSN, Serving GW or PDN GW",
    [131] =   "Activation rejected, unspecified",
    [132] =   "Service option not supported",
    [133] =   "Requested service option not subscribed",
    [134] =   "Service option temporarily out of order",
    [140] =   "Feature not supported",
    [141] =   "Semantic error in the TFT operation",
    [142] =   "Syntactical error in the TFT operation",
    [143] =   "Unknown PDP context",
    [144] =   "Semantic errors in packet filter(s)",
    [145] =   "Syntactical errors in packet filter(s)",
    [146] =   "PDP context without TFT already activated",
    [149] =   "PDP authentication failure",
    [178] =   "Maximum number of PDP contexts reached",
    [179] =   "Requested APN not supported in current RAT and PLMN combination",
    [180] =   "Request rejected, Bearer Control Mode violation",
    [181] =   "Unsupported QCI value",

    ///< 3GPP TS 27.007 §9.2.2.2: GPRS and EPS errors related to a failure to disconnect a PDN
    [171] =   "Last PDN disconnection not allowed",

    ///< 3GPP TS 27.007 §9.2.2.4: Other GPRS errors
    [148] =   "Unspecified GPRS error",
    [150] =   "Invalid mobile class",
    [182] =   "User data transmission via control plane is congested",

    ///< 3GPP TS 27.007 §9.2.3: VBS, VGCS and eMLPP-related errors
    [151] =   "VBS/VGCS not supported by the network",
    [152] =   "No service subscription on SIM",
    [153] =   "No subscription for group ID",
    [154] =   "Group Id not activated on SIM",
    [155] =   "No matching notification",
    [156] =   "VBS/VGCS call already present",
    [157] =   "Congestion",
    [158] =   "Network failure",
    [159] =   "Uplink busy",
    [160] =   "No access rights for SIM file",
    [161] =   "No subscription for priority",
    [162] =   "Operation not applicable or not possible",
    [163] =   "Group Id prefixes not supported",
    [164] =   "Group Id prefixes not usable for VBS",
    [165] =   "Group Id prefix value invalid",
};

//--------------------------------------------------------------------------------------------------
/**
 * Pre-formatted strings corresponding to AT commands +CMS error codes
 * (see 3GPP TS 27.005 3.2.5, 3GPP TS 24.011 E-2 and 3GPP TS 23.040 9.2.3.22)
 *
 */
const char* const CmsErrorCodes[STD_ERROR_CODE_SIZE] =
{
    ///< 3GPP TS 24.011 §E-2:  RP-cause definition mobile originating SM-transfer
    [1]    = "Unassigned (unallocated) number",
    [8]    = "Operator determined barring",
    [10]   = "Call barred",
    [21]   = "Short message transfer rejected",
    [27]   = "Destination out of service",
    [28]   = "Unidentified subscriber",
    [29]   = "Facility rejected",
    [30]   = "Unknown subscriber",
    [38]   = "Network out of order",
    [41]   = "Temporary failure",
    [42]   = "Congestion",
    [47]   = "Resources unavailable, unspecified",
    [50]   = "Requested facility not subscribed",
    [69]   = "Requested facility not implemented",
    [81]   = "Invalid short message transfer reference value",
    [95]   = "Invalid message, unspecified",
    [96]   = "Invalid mandatory information",
    [97]   = "Message type non-existent or not implemented",
    [98]   = "Message not compatible with short message protocol state",
    [99]   = "Information element non-existent or not implemented",
    [111]  = "Protocol error, unspecified",
    [17]   = "Network failure",
    [22]   = "Congestion",
    [127]  = "Interworking, unspecified",

    ///< 3GPP TS 23.040 §9.2.3.22: TP-Failure-Cause
    [128]   = "Telematic interworking not supported",
    [129]   = "Short message Type 0 not supported",
    [130]   = "Cannot replace short message",
    [143]   = "Unspecified TP-PID error",
    [144]   = "Data coding scheme (alphabet) not supported",
    [145]   = "Message class not supported",
    [159]   = "Unspecified TP-DCS error",
    [160]   = "Command cannot be actioned",
    [161]   = "Command unsupported",
    [175]   = "Unspecified TP-Command error",
    [176]   = "TPDU not supported",
    [192]   = "SC busy",
    [193]   = "No SC subscription",
    [194]   = "SC system failure ",
    [195]   = "Invalid SME address",
    [196]   = "Destination SME barred",
    [197]   = "SM Rejected-Duplicate SM",
    [198]   = "TP-VPF not supported",
    [199]   = "TP-VP not supported",
    [208]   = "(U)SIM SMS storage full",
    [209]   = "No SMS storage capability in (U)SIM",
    [210]   = "Error in MS",
    [211]   = "Memory Capacity Exceeded",
    [212]   = "(U)SIM Application Toolkit Busy",
    [213]   = "(U)SIM data download error",
    [255]   = "Unspecified error cause",

    ///< 3GPP TS 27.005 §3.2.5: Message service failure errors
    [300] =   "ME failure",
    [301] =   "SMS service of ME reserved",
    [302] =   "Operation not allowed",
    [303] =   "Operation not supported",
    [304] =   "Invalid PDU mode parameter",
    [305] =   "Invalid text mode parameter",
    [310] =   "(U)SIM not inserted",
    [311] =   "(U)SIM PIN required",
    [312] =   "PH-(U)SIM PIN required",
    [313] =   "(U)SIM failure",
    [314] =   "(U)SIM busy",
    [315] =   "(U)SIM wrong",
    [316] =   "(U)SIM PUK required",
    [317] =   "(U)SIM PIN2 required",
    [318] =   "(U)SIM PUK2 required",
    [320] =   "Memory failure",
    [321] =   "Invalid memory index",
    [322] =   "Memory full",
    [330] =   "SMSC address unknown",
    [331] =   "No network service",
    [332] =   "Network timeout",
    [340] =   "No +CNMA acknowledgement expected",
    [500] =   "Unknown error",
};


//--------------------------------------------------------------------------------------------------
/**
 * Get standard verbose message code
 *
 * @return
 *      - char* pointer to the verbose message
 *      - NULL if unable to retreive a verbose message
 */
//--------------------------------------------------------------------------------------------------
static const char* GetStdVerboseMsg
(
    uint32_t errorCode,
         ///< [IN] Numerical error code

    const char* patternPtr
        ///< [IN] Prefix of the final response string
)
{
    if (errorCode >= STD_ERROR_CODE_SIZE)
    {
        return NULL;
    }

    if (0 == strcmp(patternPtr, LE_ATDEFS_CME_ERROR))
    {
        return CmeErrorCodes[errorCode];
    }

    if (0 == strcmp(patternPtr, LE_ATDEFS_CMS_ERROR))
    {
        return CmsErrorCodes[errorCode];
    }

    LE_DEBUG("Not a standard pattern");
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Helper function to generate and send the final result code.
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void atProxy_SendFinalResultCode
(
    le_atProxy_PortRef_t portRef,       ///< [IN] Port Reference
    uint32_t errorCode,                 ///< [IN] CME Error Code
    ErrorCodesMode_t errorCodeMode,     ///< [IN] Error Code Mode
    le_atServer_FinalRsp_t finalResult, ///< [IN] Final Response Result
    const char* pattern,                ///< [IN] Pattern string indicating type of verbose error
    size_t patternLen                   ///< [IN] Pattern string length
)
{
    char buffer[LE_ATDEFS_RESPONSE_MAX_BYTES];

    switch (finalResult)
    {
        case LE_ATSERVER_OK:
            pa_port_Write(portRef,
                          LE_AT_PROXY_OK,
                          strlen(LE_AT_PROXY_OK));
            break;

        case LE_ATSERVER_NO_CARRIER:
            pa_port_Write(portRef,
                          LE_AT_PROXY_NO_CARRIER,
                          strlen(LE_AT_PROXY_NO_CARRIER));
            break;

        case LE_ATSERVER_NO_DIALTONE:
            pa_port_Write(portRef,
                          LE_AT_PROXY_NO_DIALTONE,
                          strlen(LE_AT_PROXY_NO_DIALTONE));
            break;

        case LE_ATSERVER_BUSY:
            pa_port_Write(portRef,
                          LE_AT_PROXY_BUSY,
                          strlen(LE_AT_PROXY_BUSY));
            break;

        case LE_ATSERVER_ERROR:
            if ((MODE_DISABLED == errorCodeMode) || (0 == patternLen))
            {
                pa_port_Write(portRef,
                              LE_AT_PROXY_ERROR,
                              strlen(LE_AT_PROXY_ERROR));
                break;
            }

            // Compose the pattern part for the response
            strncpy(buffer, pattern, LE_ATDEFS_RESPONSE_MAX_LEN);
            buffer[LE_ATDEFS_RESPONSE_MAX_LEN] = '\0';

            // Compose the code part for the response
            if (MODE_EXTENDED == errorCodeMode)
            {
                LE_DEBUG("Extended mode");
                snprintf(buffer + patternLen,
                         LE_ATDEFS_RESPONSE_MAX_BYTES - patternLen,
                         "%"PRIu32"\r\n",
                         errorCode);
            }
            else if (MODE_VERBOSE == errorCodeMode)
            {
                // This verbose mode section is added for implementing extented error codes handling
                // for atProxy on all platforms.
                // Since this verbose mode is not supported on HL78 and thus not tested.
                LE_DEBUG("Verbose mode");
                if (errorCode < STD_ERROR_CODE_SIZE)
                {
                    const char* msgPtr = GetStdVerboseMsg(errorCode, pattern);
                    if (NULL != msgPtr)
                    {
                        snprintf(buffer + patternLen,
                                 LE_ATDEFS_RESPONSE_MAX_BYTES - patternLen,
                                 "%s\r\n",
                                 msgPtr);
                    }
                    else
                    {
                        snprintf(buffer + patternLen,
                                 LE_ATDEFS_RESPONSE_MAX_BYTES - patternLen,
                                 "%"PRIu32"\r\n",
                                 errorCode);
                    }
                }
                else
                {
                    // TODO: Add custom error codes (per product) handling
                    snprintf(buffer + patternLen,
                             LE_ATDEFS_RESPONSE_MAX_BYTES - patternLen,
                             "%"PRIu32"\r\n",
                             errorCode);
                }
            }

            pa_port_Write(portRef,
                          buffer,
                          strnlen(buffer, LE_ATDEFS_RESPONSE_MAX_LEN));
            break;

        default:
            snprintf(buffer, LE_ATDEFS_RESPONSE_MAX_LEN, "%s%lu\r\n", pattern, errorCode);
            pa_port_Write(portRef,
                          buffer,
                          strnlen(buffer, LE_ATDEFS_RESPONSE_MAX_LEN));
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve the AT Command Registry
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
struct le_atProxy_StaticCommand* atProxy_GetCmdRegistry
(
    void
)
{
    return &AtCmdRegistry[0];
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to retrieve the AT Command Registry entry for a specific command
 *
 * @return Pointer of the command entry in registry
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED struct le_atProxy_StaticCommand* atProxy_GetCmdRegistryEntry
(
    uint32_t command        ///< [IN] Index of AT command in Registry
)
{
    // Verify the command index is within range
    LE_ASSERT(command < AT_CMD_MAX);

    return &AtCmdRegistry[command];
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_atProxy_Command'
 *
 * This event provides information when the AT command is detected.
 *
 */
//--------------------------------------------------------------------------------------------------
le_atServer_CommandHandlerRef_t le_atServer_AddCommandHandler
(
    le_atServer_CmdRef_t cmdRef,
        ///< [IN] AT Command index
    le_atServer_CommandHandlerFunc_t handlerPtr,
        ///< [IN] Handler to called when the AT command is detected
    void* contextPtr
        ///< [IN] Context pointer provided by caller and returned when handlePtr is called
)
{
    LE_DEBUG("Calling le_atProxy_AddCommandHandler");

    uint32_t command = AT_PROXY_CONVERT_REF2IND(cmdRef);

    if (NULL == cmdRef || command >= AT_CMD_MAX)
    {
        return NULL;
    }

    // Set pointer to AT Command Register entry
    struct le_atProxy_StaticCommand* atCmdRegistryPtr = &AtCmdRegistry[command];
    LE_FATAL_IF(!atCmdRegistryPtr, "Invalid command entry!");

    // Register AT command to firmware for allowing forwarding
    if (LE_OK != pa_atProxy_Register(atCmdRegistryPtr->commandStr))
    {
        LE_ERROR("Couldn't register command '%s' to firmware!", atCmdRegistryPtr->commandStr);
        return NULL;
    }

    // Set the Command Handler Callback function and Context Pointer
    atCmdRegistryPtr->commandHandlerPtr = handlerPtr;
    atCmdRegistryPtr->contextPtr = contextPtr;

    // Create Safe Reference to AT Command Registry entry
    return le_ref_CreateRef(AtCmdRefMap, atCmdRegistryPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_atProxy_Command'
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_RemoveCommandHandler
(
    le_atServer_CommandHandlerRef_t handlerRef
        ///< [IN] Reference of Command Handler to be removed
)
{
    // Look-up AT Command Register entry using handlerRef
    struct le_atProxy_StaticCommand* atCmdRegistryPtr = le_ref_Lookup(AtCmdRefMap, handlerRef);

    // Delete Safe Reference to AT Command Registry entry
    le_ref_DeleteRef(AtCmdRefMap, handlerRef);

    if (atCmdRegistryPtr == NULL)
    {
        LE_INFO("Unable to retrieve AT Command Registry entry, handlerRef [%" PRIuPTR "]",
                (uintptr_t) handlerRef);
        return;
    }

    // Reset the Command Handler Callback function and Context Pointer
    atCmdRegistryPtr->commandHandlerPtr = NULL;
    atCmdRegistryPtr->contextPtr = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to get the parameters of a received AT command.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_OVERFLOW      The client parameter buffer is too small.
 *      - LE_FAULT         The function failed to get the requested parameter.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_GetParameter
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_CmdRef_t commandRef,    ///< [IN] AT Command Reference
    uint32_t index,                     ///< [IN] Index of Parameter to be retrieved
    size_t parameterSize                ///< [IN] Size of parameterSize buffer
)
{
    le_result_t result = LE_OK;
    char parameter[LE_ATDEFS_PARAMETER_MAX_BYTES] = {0};

    struct le_atProxy_AtCommandSession* atCmdSessionPtr =
        le_ref_Lookup(atCmdSessionRefMap, commandRef);

    // Verify AT Command Session pointer is valid
    if (atCmdSessionPtr == NULL)
    {
        LE_ERROR("AT Command Session reference pointer is NULL");
        result = LE_FAULT;
    }
    else if (parameterSize <= atCmdSessionPtr->parameterList[index].length)
    {
        LE_ERROR("Parameter buffer too small");
        result = LE_OVERFLOW;
    }
    else if (sizeof(parameter) <= atCmdSessionPtr->parameterList[index].length)
    {
        LE_ERROR("Internal parameter buffer too small");
        result = LE_OVERFLOW;
    }
    else
    {
        // Create a NULL-terminated paramemter string for the client response
        strncpy(parameter,
                atCmdSessionPtr->parameterList[index].parameter,
                atCmdSessionPtr->parameterList[index].length);

        // NULL terminated parameter string
        parameter[atCmdSessionPtr->parameterList[index].length] = 0;

        LE_DEBUG("parameters = %s", parameter);
    }

    // Send response to client
    le_atServer_GetParameterRespond(cmdRef, result, parameter);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to get the AT command string.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the AT command string.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_GetCommandName
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_CmdRef_t commandRef,    ///< [IN] AT Command Reference
    size_t nameSize                     ///< [IN] Size of nameSize buffer
)
{
    le_result_t result = LE_OK;
    const char* name = NULL;

    struct le_atProxy_AtCommandSession* atCmdSessionPtr =
        le_ref_Lookup(atCmdSessionRefMap, commandRef);

    // Verify AT Command Session pointer is valid
    if (atCmdSessionPtr == NULL)
    {
        LE_ERROR("AT Command Session reference pointer is NULL");
        result = LE_FAULT;
    }
    else if (nameSize < sizeof(AtCmdRegistry[atCmdSessionPtr->registryIndex].commandStr))
    {
        LE_ERROR("Name buffer too small");
        result = LE_OVERFLOW;
    }
    else
    {
        // Set pointer to response command name
        name = AtCmdRegistry[atCmdSessionPtr->registryIndex].commandStr;
    }

    // Send response to client
    le_atServer_GetCommandNameRespond(cmdRef, result, name);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to send an intermediate response.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to send the intermediate response.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_SendIntermediateResponse
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_CmdRef_t commandRef,    ///< [IN] AT Command Reference
    const char* LE_NONNULL responseStr  ///< [IN] Intermediate Response String
)
{
    le_result_t result = LE_OK;

    struct le_atProxy_AtCommandSession* atCmdSessionPtr =
        le_ref_Lookup(atCmdSessionRefMap, commandRef);

    if (!atCmdSessionPtr)
    {
        LE_ERROR("Could not find AT session!");
        result = LE_FAULT;
        le_atServer_SendIntermediateResponseRespond(cmdRef, result);
        return;
    }

    // Write the responseStr out to the console port
    pa_port_Write(atCmdSessionPtr->port, "\r\n", strlen("\r\n"));
    pa_port_Write(atCmdSessionPtr->port, (char*) responseStr, strlen(responseStr));
    pa_port_Write(atCmdSessionPtr->port, "\r\n", strlen("\r\n"));

    le_atServer_SendIntermediateResponseRespond(cmdRef, result);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to send the final result code.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to send the final result code.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_SendFinalResultCode
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_CmdRef_t commandRef,    ///< [IN] AT Command Reference
    le_atServer_FinalRsp_t finalResult, ///< [IN] Final Response Result
    const char* LE_NONNULL pattern,     ///< [IN] Final Response Pattern String
    uint32_t errorCode                  ///< [IN] Final Response Error Code
)
{
    le_result_t result = LE_OK;
    le_atProxy_AtCommandSession_t* atCmdSessionPtr = le_ref_Lookup(atCmdSessionRefMap, commandRef);

    size_t patternLen = strnlen(pattern, LE_ATDEFS_RESPONSE_MAX_LEN);

    // Verify AT Command Session pointer is valid
    if (atCmdSessionPtr == NULL)
    {
        LE_ERROR("AT Command Session reference pointer is NULL");
        result = LE_FAULT;
        le_atServer_SendFinalResultCodeRespond(cmdRef, result);
        return;
    }

    // Send the AT Server Error result code
    atProxy_SendFinalResultCode(
        atCmdSessionPtr->port,
        errorCode,
        pa_atProxy_GetExtendedErrorCodes(),
        finalResult,
        pattern,
        patternLen);

    le_atServer_SendFinalResultCodeRespond(cmdRef, result);

    // After sending out final response, set current AT session to complete
    atProxyCmdHandler_Complete(atCmdSessionPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to send the unsolicited response.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to send the unsolicited response.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_SendUnsolicitedResponse
(
    le_atServer_ServerCmdRef_t cmdRef,             ///< [IN] Asynchronous Server Command Reference
    const char* LE_NONNULL responseStr,            ///< [IN] Unsolicited Response String
    le_atServer_AvailableDevice_t availableDevice, ///< [IN] device to send the unsolicited response
    le_atServer_DeviceRef_t device                 ///< [IN] device reference where the unsolicited
                                                   ///<      response has to be sent
)
{
    struct le_atProxy_AtCommandSession* atSessionPtr = NULL;
    le_result_t result = LE_OK;

    if (LE_ATSERVER_SPECIFIC_DEVICE == availableDevice)
    {
        // Make sure that device is a reference of AT session
        // atProxy doesn't have device (DeviceContext_t), instead it uses AT session.
        atSessionPtr = le_ref_Lookup(atCmdSessionRefMap, device);
        if (!atSessionPtr)
        {
            LE_ERROR("Could not find AT session!");
            result = LE_FAULT;
            le_atServer_SendUnsolicitedResponseRespond(cmdRef, result);
            return;
        }

        atProxyCmdHandler_SendUnsolicitedResponse(responseStr, atSessionPtr);
    }
    else
    {
        le_ref_IterRef_t iterRef = le_ref_GetIterator(atCmdSessionRefMap);
        while (LE_OK == le_ref_NextNode(iterRef))
        {
            atSessionPtr = (struct le_atProxy_AtCommandSession*) le_ref_GetValue(iterRef);

            atProxyCmdHandler_SendUnsolicitedResponse(responseStr, atSessionPtr);
        }
    }

    le_atServer_SendUnsolicitedResponseRespond(cmdRef, result);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function creates an AT command and registers it into the AT parser.
 *
 * @return
 *      - Reference to the AT command.
 *      - NULL if an error occurs.
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_Create
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    const char* namePtr                 ///< [IN] AT command name string
)
{
    int i, num = sizeof(AtCmdRegistry)/sizeof(AtCmdRegistry[0]);
    le_atServer_CmdRef_t ref = NULL;

    for (i=0; i<num; i++)
    {
        if (0 == strncmp(AtCmdRegistry[i].commandStr, namePtr, LE_ATDEFS_COMMAND_MAX_LEN))
        {
            break;
        }
    }

    if (i != num)
    {
        ref = AT_PROXY_CONVERT_IND2REF((uint32_t)i);
    }

    le_atServer_CreateRespond(cmdRef, ref);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to send stored unsolicited responses.
 * It can be used to send unsolicited reponses that were stored before switching to data mode.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to send the intermediate response.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_SendStoredUnsolicitedResponses
(
    le_atServer_ServerCmdRef_t cmdRef,  ///< [IN] Asynchronous Server Command Reference
    le_atServer_CmdRef_t commandRef     ///< [IN] AT command reference
)
{
    le_result_t res = atProxyCmdHandler_FlushStoredURC(commandRef);

    le_atServer_SendStoredUnsolicitedResponsesRespond(cmdRef, res);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to get the device reference in use for an AT command specified with
 * its reference.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the AT command string.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_GetDevice
(
    le_atServer_ServerCmdRef_t cmdRef,      ///< [IN] Asynchronous Server Command Reference
    le_atServer_CmdRef_t commandRef         ///< [IN] AT command reference
)
{
    struct le_atProxy_AtCommandSession* atCmdSessionPtr =
        le_ref_Lookup(atCmdSessionRefMap, commandRef);

    le_atServer_DeviceRef_t deviceRefPtr = NULL;

    if (atCmdSessionPtr != NULL)
    {
        // Set the Device Reference using the AT Command Session ref
        deviceRefPtr = (le_atServer_DeviceRef_t)atCmdSessionPtr->ref;
    }
    else
    {
        LE_ERROR("[%s] AT Command Session is NULL", __FUNCTION__);
    }

    le_atServer_GetDeviceRespond(cmdRef, deviceRefPtr ? LE_OK : LE_FAULT, deviceRefPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function enables extended error codes on the selected device
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_EnableExtendedErrorCodes
(
    le_atServer_ServerCmdRef_t cmdRef  ///< [IN] Asynchronous Server Command Reference
)
{
    pa_atProxy_EnableExtendedErrorCodes();

    le_atServer_EnableExtendedErrorCodesRespond(cmdRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function disables the current error codes mode on the selected device
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_DisableExtendedErrorCodes
(
    le_atServer_ServerCmdRef_t cmdRef  ///< [IN] Asynchronous Server Command Reference
)
{
    pa_atProxy_DisableExtendedErrorCodes();

    le_atServer_DisableExtendedErrorCodesRespond(cmdRef);
}

//-------------------------------------------------------------------------------------------------
/**
 * Component initialisation once for all component instances.
 */
//-------------------------------------------------------------------------------------------------
COMPONENT_INIT_ONCE
{
    // Initialize the AT Command Handler
    atProxyCmdHandler_Init();
}

//-------------------------------------------------------------------------------------------------
/**
 * Component initialisation.
 */
//-------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Starting AT Proxy");

    // AT Command Reference pool allocation
    AtCmdRefMap = le_ref_InitStaticMap(AtCmdRefMap, AT_CMD_MAX);

}
