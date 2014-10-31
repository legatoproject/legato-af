/**
 * @file pa_mdc_stub.c
 *
 * QMI implementation of @ref c_pa_mdc API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */


#include "legato.h"
#include "pa_mdc.h"

bool StartProfile[PA_MDC_MAX_PROFILE]={0};

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Component initializer automatically called by the application framework when the process starts.
 *
 **/
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the gateway IP address for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in gatewayAddrStr
 *      - LE_NOT_POSSIBLE for all other errors
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
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get session type for the given profile ( IP V4 or V6 )
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetSessionType
(
    uint32_t profileIndex,              ///< [IN] The profile to use
    pa_mdc_SessionType_t* sessionIpPtr  ///< [OUT] IP family session
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the network interface for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the interface name would not fit in interfaceNameStr
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetInterfaceName
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    char*  interfaceNameStr,                ///< [OUT] The name of the network interface
    size_t interfaceNameStrSize             ///< [IN] The size in bytes of the name buffer
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the profile data for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_WriteProfile
(
    uint32_t profileIndex,                    ///< [IN] The profile to write
    pa_mdc_ProfileData_t* profileDataPtr    ///< [IN] The profile data
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the profile data for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_ReadProfile
(
    uint32_t profileIndex,                  ///< [IN] The profile to read
    pa_mdc_ProfileData_t* profileDataPtr    ///< [OUT] The profile data
)
{
    strcpy( &profileDataPtr->apn[0], "TstAPN");
    profileDataPtr->authentication.type = LE_MDC_AUTH_NONE;
    profileDataPtr->pdp = LE_MDC_PDP_IPV4;

    return LE_OK;
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
void pa_mdc_SetSessionStateHandler
(
    pa_mdc_SessionStateHandler_t handlerRef, ///< [IN] The session state handler function.
    void*                        contextPtr  ///< [IN] The context to be given to the handler.
)
{

}

//--------------------------------------------------------------------------------------------------
/**
 * Get the IP address for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in gatewayAddrStr
 *      - LE_NOT_POSSIBLE for all other errors
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
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the session state for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE on error
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetSessionState
(
    uint32_t profileIndex,                  ///< [IN] The profile to use
    pa_mdc_SessionState_t* sessionStatePtr  ///< [OUT] The data session state
)
{
    *sessionStatePtr = PA_MDC_DISCONNECTED;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Data Bearer Technology for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDataBearerTechnology
(
    uint32_t                       profileIndex,              ///< [IN] The profile to use
    le_mdc_DataBearerTechnology_t* downlinkDataBearerTechPtr, ///< [OUT] downlink data bearer technology
    le_mdc_DataBearerTechnology_t* uplinkDataBearerTechPtr    ///< [OUT] uplink data bearer technology
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV4
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_NOT_POSSIBLE for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StartSessionIPV4
(
    uint32_t profileIndex,        ///< [IN] The profile to use
    pa_mdc_CallRef_t* callRefPtr  ///< [OUT] Reference used for stopping the data session
)
{
    if ( StartProfile[profileIndex] )
    {
        *callRefPtr = NULL;
        return LE_NOT_POSSIBLE;
    }
    else
    {
        *callRefPtr = &StartProfile[profileIndex];
        StartProfile[profileIndex] = 1;
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV6
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_NOT_POSSIBLE for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StartSessionIPV6
(
    uint32_t profileIndex,        ///< [IN] The profile to use
    pa_mdc_CallRef_t* callRefPtr  ///< [OUT] Reference used for stopping the data session
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data session with the given profile using IPV4-V6
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected
 *      - LE_NOT_POSSIBLE for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StartSessionIPV4V6
(
    uint32_t profileIndex,        ///< [IN] The profile to use
    pa_mdc_CallRef_t* callRefPtr  ///< [OUT] Reference used for stopping the data session
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get data flow statistics since the last reset.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_GetDataFlowStatistics
(
    pa_mdc_PktStatistics_t *dataStatisticsPtr ///< [OUT] Statistics data
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset data flow statistics
 *
 * * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_ResetDataFlowStatistics
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop a data session for the given profile
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session has already been stopped (i.e. it is disconnected)
 *      - LE_NOT_POSSIBLE for other failures
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_StopSession
(
    pa_mdc_CallRef_t callRef         ///< [IN] The call reference returned when starting the sessions
)
{
    bool startProfile = *((bool*) callRef);

    if (startProfile)
    {
        return LE_OK;
    }
    else
    {
        return LE_NOT_POSSIBLE;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the primary/secondary DNS addresses for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in buffer
 *      - LE_NOT_POSSIBLE for all other errors
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
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check the availability of the given profile index
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdc_IsProfileAllowed
(
    uint32_t  profileIndex,              ///< [IN] The profile to check
    bool*     isAllowed                  ///< [OUT] profile using permission
)
{


    if (( profileIndex == PA_MDC_MIN_INDEX_3GPP_PROFILE + 1 ) ||
         (( profileIndex >= PA_MDC_MIN_INDEX_3GPP_PROFILE + 3 ) &&
         ( profileIndex <= PA_MDC_MAX_INDEX_3GPP_PROFILE )) ||
         (( profileIndex > PA_MDC_MIN_INDEX_3GPP2_PROFILE ) &&
         ( profileIndex < PA_MDC_MAX_INDEX_3GPP2_PROFILE )))
    {
         *isAllowed = false;
    }
    else
    {
         *isAllowed = true;
    }

    return LE_OK;
}
