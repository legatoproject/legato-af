/**
 * @file pa_mdc_default.c
 *
 * Default implementation of @ref c_pa_mdc.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_mdc.h"

//--------------------------------------------------------------------------------------------------
/**
 * Get the index of the default profile (link to the platform)
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDefaultProfileIndex
(
    uint32_t* profileIndexPtr
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the index of the default profile for Bearer Independent Protocol (link to the platform)
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetBipDefaultProfileIndex
(
    uint32_t* profileIndexPtr   ///< [OUT] index of the profile.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the profile data for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_ReadProfile
(
    uint32_t profileIndex,                  ///< [IN] The profile to read
    pa_mdc_ProfileData_t* profileDataPtr    ///< [OUT] The profile data
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check whether the profile already exists on the modem ; if not, ask to the modem to create a new
 * profile.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_InitializeProfile
(
    uint32_t   profileIndex     ///< [IN] The profile to write
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the profile data for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_WriteProfile
(
    uint32_t profileIndex,                  ///< [IN] The profile to write
    pa_mdc_ProfileData_t* profileDataPtr    ///< [IN] The profile data
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV4
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StartSessionIPV4
(
    uint32_t profileIndex          ///< [IN] The profile to use
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV6
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StartSessionIPV6
(
    uint32_t profileIndex        ///< [IN] The profile to use
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV4-V6
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StartSessionIPV4V6
(
    uint32_t profileIndex        ///< [IN] The profile to use
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get session type for the given profile ( IP V4 or V6 )
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetSessionType
(
    uint32_t profileIndex,              ///< [IN] The profile to use
    pa_mdc_SessionType_t* sessionIpPtr  ///< [OUT] IP family session
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop a data session for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if the input parameter is not valid
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StopSession
(
    uint32_t profileIndex              ///< [IN] The profile to use
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reject a MT-PDP data session for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_UNSUPPORTED if not supported by the target
 *      - LE_FAULT for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_RejectMtPdpSession
(
    uint32_t profileIndex             ///< [IN] The profile to use
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the session state for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on error
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetSessionState
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    le_mdc_ConState_t* sessionStatePtr      ///< [OUT] The data session state
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a handler for session state notifications.
 *
 * If the handler is NULL, then the previous handler will be removed.
 *
 * @note
 *      The process exits on failure
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_mdc_AddSessionStateHandler
(
    pa_mdc_SessionStateHandler_t handlerRef, ///< [IN] The session state handler function.
    void*                        contextPtr  ///< [IN] The context to be given to the handler.

)
{
    LE_ERROR("Unsupported function called");
    return NULL;
}

/**
 * Get the connection failure reason
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mdc_GetConnectionFailureReason
(
    uint32_t profileIndex,              ///< [IN] The profile to use
    pa_mdc_ConnectionFailureCode_t** failureCodesPtr  ///< [OUT] The specific Failure Reason codes
)
{
    *failureCodesPtr = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the connection failure reason for IPv4v6 mode
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mdc_GetConnectionFailureReasonExt
(
    uint32_t profileIndex,                           ///< [IN] The profile to use
    le_mdc_Pdp_t pdp,                                ///< [IN] The failure reason pdp type
    pa_mdc_ConnectionFailureCode_t** failureCodesPtr  ///< [OUT] The specific Failure Reason codes
)
{
    *failureCodesPtr = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the network interface for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the interface name would not fit in interfaceNameStr
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetInterfaceName
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    char*  interfaceNameStr,                ///< [OUT] The name of the network interface
    size_t interfaceNameStrSize             ///< [IN] The size in bytes of the name buffer
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the IP address for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in gatewayAddrStr
 *      - LE_UNSUPPORTED if the IP version is unsupported
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetIPAddress
(
    uint32_t profileIndex,             ///< [IN] The profile to use
    le_mdmDefs_IpVersion_t ipVersion,          ///< [IN] IP Version
    char*  ipAddrStr,                  ///< [OUT] The IP address in dotted format
    size_t ipAddrStrSize               ///< [IN] The size in bytes of the address buffer
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the gateway IP address for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in gatewayAddrStr
 *      - LE_UNSUPPORTED if the IP version is unsupported
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetGatewayAddress
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    le_mdmDefs_IpVersion_t ipVersion,               ///< [IN] IP Version
    char*  gatewayAddrStr,                  ///< [OUT] The gateway IP address in dotted format
    size_t gatewayAddrStrSize               ///< [IN] The size in bytes of the address buffer
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the primary/secondary DNS addresses for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in buffer
 *      - LE_UNSUPPORTED if the IP version is unsupported
 *      - LE_FAULT for all other errors
 *
 * @note
 *      If only one DNS address is available, then it will be returned, and an empty string will
 *      be returned for the unavailable address
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDNSAddresses
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    le_mdmDefs_IpVersion_t ipVersion,               ///< [IN] IP Version
    char*  dns1AddrStr,                     ///< [OUT] The primary DNS IP address in dotted format
    size_t dns1AddrStrSize,                 ///< [IN] The size in bytes of the dns1AddrStr buffer
    char*  dns2AddrStr,                     ///< [OUT] The secondary DNS IP address in dotted format
    size_t dns2AddrStrSize                  ///< [IN] The size in bytes of the dns2AddrStr buffer
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Access Point Name for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Access Point Name would not fit in apnNameStr
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetAccessPointName
(
    uint32_t profileIndex,             ///< [IN] The profile to use
    char*  apnNameStr,                 ///< [OUT] The Access Point Name
    size_t apnNameStrSize              ///< [IN] The size in bytes of the address buffer
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Data Bearer Technology for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDataBearerTechnology
(
    uint32_t                       profileIndex,              ///< [IN] The profile to use
    le_mdc_DataBearerTechnology_t* downlinkDataBearerTechPtr, ///< [OUT] downlink data bearer technology
    le_mdc_DataBearerTechnology_t* uplinkDataBearerTechPtr    ///< [OUT] uplink data bearer technology
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get data flow statistics since the last reset.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDataFlowStatistics
(
    pa_mdc_PktStatistics_t *dataStatisticsPtr ///< [OUT] Statistics data
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset data flow statistics
 *
 * * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_ResetDataFlowStatistics
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop collecting data flow statistics
 *
 * * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StopDataFlowStatistics
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start collecting data flow statistics
 *
 * * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StartDataFlowStatistics
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Map a profile on a network interface
 *
 * * @return
 *      - LE_OK on success
 *      - LE_UNSUPPORTED if not supported by the target
 *      - LE_FAULT for all other errors
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_mdc_MapProfileOnNetworkInterface
(
    uint32_t         profileIndex,         ///< [IN] The profile to use
    const char*      interfaceNamePtr      ///< [IN] Network interface name
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the list of all profiles
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetProfileList
(
    le_mdc_ProfileInfo_t* profileList, ///< [OUT] list of available profiles
    size_t* listSize                   ///< [INOUT] list size
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}
