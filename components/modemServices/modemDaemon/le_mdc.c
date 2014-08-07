/**
 * @file le_mdc.c
 *
 * Implementation of Modem Data Control API
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 *
 * @todo
 *      Use safe references for the profile references
 */


#include "legato.h"
#include "interfaces.h"
#include "pa_mdc.h"
#include "le_cfg_interface.h"

// Include macros for printing out values
#include "le_print.h"


//--------------------------------------------------------------------------------------------------
/**
 * Data Control Profile structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mdc_Profile {
    uint32_t profileIndex;                      ///< Index of the profile on the modem
    uint32_t callRef;                           ///< Reference for current call, if connected
    le_event_Id_t sessionStateEvent;            ///< Event to report when session changes state
    pa_mdc_ProfileData_t modemData;             ///< Profile data that is written to the modem
}
le_mdc_Profile_t;


//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for data profile objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t DataProfilePool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for data profile objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t DataProfileRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * This table keeps track of the allocated data profile objects.
 *
 * Since the maximum number of profile objects is known, we can use a table here instead of a
 * linked list.  If a particular entry is NULL, then the profile has not been allocated yet.
 * Since this variable is static, all entries are initially 0 (i.e NULL).
 *
 * The modem profile index is the index into this table +1.
 */
//--------------------------------------------------------------------------------------------------
static le_mdc_Profile_t* ProfileTable[PA_MDC_MAX_PROFILE];

//--------------------------------------------------------------------------------------------------
/**
 * Trace reference used for controlling tracing in this module.
 */
//--------------------------------------------------------------------------------------------------
static le_log_TraceRef_t TraceRef;

/// Macro used to generate trace output in this module.
/// Takes the same parameters as LE_DEBUG() et. al.
#define TRACE(...) LE_TRACE(TraceRef, ##__VA_ARGS__)

/// Macro used to query current trace state in this module
#define IS_TRACE_ENABLED LE_IS_TRACE_ENABLED(TraceRef)



// =============================================
//  PRIVATE FUNCTIONS
// =============================================


//--------------------------------------------------------------------------------------------------
/**
 * The first-layer New Session State Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerSessionStateChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    bool*           isConnectedPtr = reportPtr;
    le_mdc_SessionStateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*isConnectedPtr, le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for new session state events from PA layer
 */
//--------------------------------------------------------------------------------------------------
static void NewSessionStateHandler
(
    pa_mdc_SessionStateData_t* sessionStatePtr      ///< New session state
)
{
    bool isConnected;
    le_event_Id_t event;

    if ( IS_TRACE_ENABLED )
    {
        LE_PRINT_VALUE("%i", sessionStatePtr->profileIndex);
        LE_PRINT_VALUE("%i", sessionStatePtr->newState);
    }

    // Init the data for the event report
    isConnected = (sessionStatePtr->newState != PA_MDC_DISCONNECTED);

    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap,
                                                 ProfileTable[sessionStatePtr->profileIndex-1]);

    if (profilePtr == NULL)
    {
        LE_CRIT("Invalid reference (%p) found!", ProfileTable[sessionStatePtr->profileIndex-1]);
        return;
    }

    // Report the event for the given profile
    event = profilePtr->sessionStateEvent;
    le_event_Report(event, &isConnected, sizeof(isConnected));

    // Free the received report data
    le_mem_Release(sessionStatePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Load modem data profile
 *
 * @note
 *      The process exits, if a new data profile could not be created for any reason other than
 *      the maximum number of profiles has been reached.
 */
//--------------------------------------------------------------------------------------------------
static void LoadModemProfile
(
    void
)
{
    le_mdc_Profile_t* profilePtr;
    int idx;

    for (idx=0; idx<PA_MDC_MAX_PROFILE; idx++)
    {
        char eventName[32] = {0};
        profilePtr = le_mem_TryAlloc(DataProfilePool);
        if (profilePtr == NULL)
        {
            return;
        }

        memset(profilePtr, 0, sizeof(*profilePtr));

        if ( pa_mdc_ReadProfile(idx+1,&profilePtr->modemData) != LE_OK )
        {
            return;
        }
        profilePtr->profileIndex = idx+1;

        // Each profile has its own event for reporting session state changes
        snprintf(eventName,sizeof(eventName)-1, "profile-%d",profilePtr->profileIndex);
        profilePtr->sessionStateEvent = le_event_CreateId(eventName, sizeof(bool));

        // Init the remaining fields
        profilePtr->callRef = 0;

        // Create a Safe Reference for this data profile object.
        ProfileTable[idx] = le_ref_CreateRef(DataProfileRefMap, profilePtr);
        profilePtr->profileIndex = idx+1;

        if ( IS_TRACE_ENABLED )
        {
            LE_PRINT_VALUE("%X", profilePtr->callRef);
            LE_PRINT_VALUE("%u", profilePtr->profileIndex);
        }
    }
}

// =============================================
//  MODULE/COMPONENT FUNCTIONS
// =============================================


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the MDC component.
 *
 * @note
 *      The process exits on failure
 */
//--------------------------------------------------------------------------------------------------
void le_mdc_Init
(
    void
)
{
    // Get a reference to the trace keyword that is used to control tracing in this module.
    TraceRef = le_log_GetTraceRef("mdc");

    // Allocate the profile pool, and set the max number of objects, since it is already known.
    DataProfilePool = le_mem_CreatePool("DataProfilePool", sizeof(le_mdc_Profile_t));
    le_mem_ExpandPool(DataProfilePool, PA_MDC_MAX_PROFILE);

    // Create the Safe Reference Map to use for data profile object Safe References.
    DataProfileRefMap = le_ref_CreateMap("DataProfileMap", PA_MDC_MAX_PROFILE);

    pa_mdc_SetSessionStateHandler(NewSessionStateHandler);

    LoadModemProfile();
}

// =============================================
//  PUBLIC API FUNCTIONS
// =============================================

//--------------------------------------------------------------------------------------------------
/**
 * Get Profile Reference for index
 *
 * @return
 *      - Reference to the data profile
 *      - NULL if the profile index does not exist
 */
//--------------------------------------------------------------------------------------------------
le_mdc_ProfileRef_t le_mdc_GetProfile
(
    uint32_t index ///< index of the profile.
)
{
    if ((index>0) & (index<=PA_MDC_MAX_PROFILE))
    {
        return ProfileTable[index-1];
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the index for the given Profile.
 *
 * @return
 *      - index of the profile in the modem
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_mdc_GetProfileIndex
(
    le_mdc_ProfileRef_t profileRef ///< Query this profile object
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }

    return profilePtr->profileIndex;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start profile data session.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected for the given profile
 *      - LE_NOT_POSSIBLE for other failures
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_StartSession
(
    le_mdc_ProfileRef_t profileRef     ///< [IN] Start data session for this profile object
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }

    switch (profilePtr->modemData.pdp)
    {
        case LE_MDC_PDP_IPV4:
            return pa_mdc_StartSessionIPV4(profilePtr->profileIndex, &profilePtr->callRef);
        case LE_MDC_PDP_IPV6:
            return pa_mdc_StartSessionIPV6(profilePtr->profileIndex, &profilePtr->callRef);
        case LE_MDC_PDP_IPV4V6:
            return pa_mdc_StartSessionIPV4V6(profilePtr->profileIndex, &profilePtr->callRef);
        default:
            return LE_NOT_POSSIBLE;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop profile data session.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session has already been stopped (i.e. it is disconnected)
 *      - LE_NOT_POSSIBLE for other failures
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_StopSession
(
    le_mdc_ProfileRef_t profileRef     ///< [IN] Stop data session for this profile object
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }

    return pa_mdc_StopSession(profilePtr->callRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the current data session state.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE on failure
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetSessionState
(
    le_mdc_ProfileRef_t profileRef,        ///< [IN] Query this profile object
    bool*   isConnectedPtr       ///< [OUT] Data session state
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (isConnectedPtr == NULL)
    {
        LE_KILL_CLIENT("isConnectedPtr is NULL !");
        return LE_FAULT;
    }

    le_result_t result;
    pa_mdc_SessionState_t sessionState;

    result = pa_mdc_GetSessionState(profilePtr->profileIndex, &sessionState);

    *isConnectedPtr = ( (result == LE_OK) && (sessionState != PA_MDC_DISCONNECTED) );

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Register a handler for session state changes on the given profile.
 *
 * @return
 *      - A handler reference, which is only needed for later removal of the handler.
 *      - NULL if the profile index is invalid
 *
 * @note
 *      Process exits on failure.
 */
//--------------------------------------------------------------------------------------------------
le_mdc_SessionStateHandlerRef_t le_mdc_AddSessionStateHandler
(
    le_mdc_ProfileRef_t profileRef,             ///< [IN] profile object of interest
    le_mdc_SessionStateHandlerFunc_t handler,   ///< [IN] Handler function
    void* contextPtr                            ///< [IN] Context pointer
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return NULL;
    }
    if (handler == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler("le_NewSessionStateHandler",
                                                                  profilePtr->sessionStateEvent,
                                                                  FirstLayerSessionStateChangeHandler,
                                                                  (le_event_HandlerFunc_t)handler);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mdc_SessionStateHandlerRef_t)(handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler for session state changes
 *
 * @note
 *      The process exits on failure
 */
//--------------------------------------------------------------------------------------------------
void le_mdc_RemoveSessionStateHandler
(
    le_mdc_SessionStateHandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the network interface name, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the interface name can't fit in interfaceNameStr
 *      - LE_NOT_POSSIBLE on any other failure
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetInterfaceName
(
    le_mdc_ProfileRef_t profileRef,        ///< [IN] Query this profile object
    char*   interfaceNameStr,    ///< [OUT] Network interface name
    size_t  interfaceNameStrSize ///< [IN] Name buffer size in bytes
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (interfaceNameStr == NULL)
    {
        LE_KILL_CLIENT("interfaceNameStr is NULL !");
        return LE_FAULT;
    }

    return pa_mdc_GetInterfaceName(profilePtr->profileIndex, interfaceNameStr, interfaceNameStrSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the IPv4 address for the given profile, if the data session is connected and has an IPv4
 * address.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in ipAddrStr
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetIPv4Address
(
    le_mdc_ProfileRef_t profileRef,   ///< [IN] Query this profile object
    char*   ipAddrStr,      ///< [OUT] The IP address in dotted format
    size_t  ipAddrStrSize   ///< [IN] The size in bytes of the address buffer
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (ipAddrStr == NULL)
    {
        LE_KILL_CLIENT("IPAddrStr is NULL !");
        return LE_FAULT;
    }

    return pa_mdc_GetIPAddress(profilePtr->profileIndex,LE_MDMDEFS_IPV4,
                               ipAddrStr, ipAddrStrSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the gateway IPv4 address for the given profile, if the data session is connected and has an
 * IPv4 address.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in gatewayAddrStr
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetIPv4GatewayAddress
(
    le_mdc_ProfileRef_t profileRef,        ///< [IN] Query this profile object
    char*   gatewayAddrStr,      ///< [OUT] Gateway IP address in dotted format
    size_t  gatewayAddrStrSize   ///< [IN] Address buffer size in bytes
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (gatewayAddrStr == NULL)
    {
        LE_KILL_CLIENT("gatewayAddrStr is NULL !");
        return LE_FAULT;
    }

    return pa_mdc_GetGatewayAddress(profilePtr->profileIndex,LE_MDMDEFS_IPV4,
                                    gatewayAddrStr,gatewayAddrStrSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the primary/secondary DNS v4 addresses for the given profile, if the data session is
 * connected and has an IPv4 address.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in buffer
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      - If only one DNS address is available, then it will be returned, and an empty string will
 *        be returned for the unavailable address
 *      - The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetIPv4DNSAddresses
(
    le_mdc_ProfileRef_t profileRef,         ///< [IN] Query this profile object
    char*   dns1AddrStr,          ///< [OUT] Primary DNS IP address in dotted format
    size_t  dns1AddrStrSize,      ///< [IN] dns1AddrStr buffer size in bytes
    char*   dns2AddrStr,          ///< [OUT] Secondary DNS IP address in dotted format
    size_t  dns2AddrStrSize       ///< [IN] dns2AddrStr buffer size in bytes
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (dns1AddrStr == NULL)
    {
        LE_KILL_CLIENT("dns1AddrStr is NULL !");
        return LE_FAULT;
    }
    if (dns2AddrStr == NULL)
    {
        LE_KILL_CLIENT("dns2AddrStr is NULL !");
        return LE_FAULT;
    }

    return pa_mdc_GetDNSAddresses(profilePtr->profileIndex,LE_MDMDEFS_IPV4,
                                  dns1AddrStr, dns1AddrStrSize,
                                  dns2AddrStr, dns2AddrStrSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the IPv6 address for the given profile, if the data session is connected and has an IPv6
 * address.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in ipAddrStr
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetIPv6Address
(
    le_mdc_ProfileRef_t profileRef,    ///< [IN] Query this profile object
    char*   ipAddrStr,       ///< [OUT] The IP address in dotted format
    size_t  ipAddrStrSize    ///< [IN] The size in bytes of the address buffer
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (ipAddrStr == NULL)
    {
        LE_KILL_CLIENT("IPAddrStr is NULL !");
        return LE_FAULT;
    }

    return pa_mdc_GetIPAddress(profilePtr->profileIndex,LE_MDMDEFS_IPV6,
                               ipAddrStr, ipAddrStrSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the gateway IPv6 address for the given profile, if the data session is connected and has an
 * IPv6 address.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in gatewayAddrStr
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetIPv6GatewayAddress
(
    le_mdc_ProfileRef_t profileRef,         ///< [IN] Query this profile object
    char*   gatewayAddrStr,       ///< [OUT] Gateway IP address in dotted format
    size_t  gatewayAddrStrSize    ///< [IN] Address buffer size in bytes
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (gatewayAddrStr == NULL)
    {
        LE_KILL_CLIENT("gatewayAddrStr is NULL !");
        return LE_FAULT;
    }

    return pa_mdc_GetGatewayAddress(profilePtr->profileIndex,LE_MDMDEFS_IPV6,
                                    gatewayAddrStr,gatewayAddrStrSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the primary/secondary DNS v6 addresses, if the data session is connected and has an IPv6
 * address.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address can't fit in buffer
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      - If only one DNS address is available, it will be returned, and an empty string will
 *        be returned for the unavailable address.
 *      - The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetIPv6DNSAddresses
(
    le_mdc_ProfileRef_t profileRef,        ///< [IN] Query this profile object
    char*   dns1AddrStr,         ///< [OUT] Primary DNS IP address in dotted format
    size_t  dns1AddrStrSize,     ///< [IN] dns1AddrStr buffer size in bytes
    char*   dns2AddrStr,         ///< [OUT] Secondary DNS IP address in dotted format
    size_t  dns2AddrStrSize      ///< [IN] dns2AddrStr buffer size in bytes
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (dns1AddrStr == NULL)
    {
        LE_KILL_CLIENT("dns1AddrStr is NULL !");
        return LE_FAULT;
    }
    if (dns2AddrStr == NULL)
    {
        LE_KILL_CLIENT("dns2AddrStr is NULL !");
        return LE_FAULT;
    }

    return pa_mdc_GetDNSAddresses(profilePtr->profileIndex,LE_MDMDEFS_IPV6,
                                  dns1AddrStr, dns1AddrStrSize,
                                  dns2AddrStr, dns2AddrStrSize);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the Data Bearer Technology for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetDataBearerTechnology
(
    le_mdc_ProfileRef_t            profileRef,                ///< [IN] Query this profile object
    le_mdc_DataBearerTechnology_t* downlinkDataBearerTechPtr, ///< [OUT] downlink data bearer technology
    le_mdc_DataBearerTechnology_t* uplinkDataBearerTechPtr    ///< [OUT] uplink data bearer technology
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (downlinkDataBearerTechPtr == NULL)
    {
        LE_KILL_CLIENT("downlinkDataBearerTechPtr is NULL !");
        return LE_FAULT;
    }
    if (uplinkDataBearerTechPtr == NULL)
    {
        LE_KILL_CLIENT("uplinkDataBearerTechPtr is NULL !");
        return LE_FAULT;
    }

    return pa_mdc_GetDataBearerTechnology(profilePtr->profileIndex,
                                          downlinkDataBearerTechPtr,
                                          uplinkDataBearerTechPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Allow the caller to know if the given profile is actually supporting IPv4, if the data session
 * is connected.
 *
 * @return TRUE if PDP type is IPv4, FALSE otherwise.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
bool le_mdc_IsIPv4
(
    le_mdc_ProfileRef_t profileRef        ///< [IN] Query this profile object
)
{
    pa_mdc_SessionType_t ipFamily;
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }

    if (pa_mdc_GetSessionType(profilePtr->profileIndex,&ipFamily) != LE_OK)
    {
        LE_WARN("Could not get the Session Type");
        return false;
    }

    return (ipFamily == PA_MDC_SESSION_IPV4) || (ipFamily == PA_MDC_SESSION_IPV4V6);
}


//--------------------------------------------------------------------------------------------------
/**
 * Allow the caller to know if the given profile is actually supporting IPv6, if the data session
 * is connected.
 *
 * @return TRUE if PDP type is IPv6, FALSE otherwise.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
bool le_mdc_IsIPv6
(
    le_mdc_ProfileRef_t profileRef        ///< [IN] Query this profile object
)
{
    pa_mdc_SessionType_t ipFamily;
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }

    if (pa_mdc_GetSessionType(profilePtr->profileIndex,&ipFamily) != LE_OK)
    {
        LE_WARN("Could not get the Session Type");
        return false;
    }

    return (ipFamily == PA_MDC_SESSION_IPV6) || (ipFamily == PA_MDC_SESSION_IPV4V6);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get number of bytes received/transmitted without error since the last reset.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      - The process exits, if an invalid pointer is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetBytesCounters
(
    uint64_t *rxBytes,  ///< [OUT] bytes amount received since the last counter reset
    uint64_t *txBytes   ///< [OUT] bytes amount transmitted since the last counter reset
)
{
    if (rxBytes == NULL)
    {
        LE_KILL_CLIENT("rxBytes is NULL !");
        return LE_FAULT;
    }
    if (txBytes == NULL)
    {
        LE_KILL_CLIENT("txBytes is NULL !");
        return LE_FAULT;
    }

    pa_mdc_PktStatistics_t data;
    le_result_t result = pa_mdc_GetDataFlowStatistics(&data);
    if ( result != LE_OK )
    {
        return result;
    }

    *rxBytes = data.receivedBytesCount;
    *txBytes = data.transmittedBytesCount;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset received/transmitted data flow statistics
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_ResetBytesCounter
(
    void
)
{
    return pa_mdc_ResetDataFlowStatistics();
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the Packet Data Protocol (PDP) for the given profile.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if the PDP is not supported
 *      - LE_NOT_POSSIBLE if the data session is currently connected for the given profile
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_SetPDP
(
    le_mdc_ProfileRef_t profileRef,  ///< [IN] Query this profile object
    le_mdc_Pdp_t        pdp          ///< [IN] The Packet Data Protocol
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }

    le_result_t result;
    bool isConnected;

    result = le_mdc_GetSessionState(profileRef, &isConnected);
    if ( (result != LE_OK) || isConnected )
    {
        return LE_NOT_POSSIBLE;
    }

    profilePtr->modemData.pdp = pdp;

    return pa_mdc_WriteProfile(profilePtr->profileIndex, &profilePtr->modemData);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Packet Data Protocol (PDP) for the given profile.
 *
 * @return
 *      - packet data protocol value
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_mdc_Pdp_t le_mdc_GetPDP
(
    le_mdc_ProfileRef_t profileRef  ///< [IN] Query this profile object
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_MDC_PDP_UNKNOWN;
    }

    if ( pa_mdc_ReadProfile(profilePtr->profileIndex,&profilePtr->modemData) != LE_OK )
    {
        LE_ERROR("Could not read profile at index %d", profilePtr->profileIndex);
        return LE_MDC_PDP_UNKNOWN;
    }

    return profilePtr->modemData.pdp;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the Access Point Name (APN) for the given profile.
 *
 * The APN must be an ASCII string.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE if the data session is currently connected for the given profile
 *
 * @note If APN is too long (max APN_NAME_MAX_LEN digits), it is a fatal error, the
 *       function will not return.
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_SetAPN
(
    le_mdc_ProfileRef_t profileRef, ///< [IN] Query this profile object
    const char         *apnPtr      ///< [IN] The Access Point Name
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (apnPtr == NULL)
    {
        LE_CRIT("apnStr is NULL !");
        return LE_FAULT;
    }

    size_t apnLen = strlen(apnPtr);
    if ( apnLen > LE_MDC_APN_NAME_MAX_LEN )
    {
        LE_CRIT("apnStr is too long (%zd) > LE_MDC_APN_NAME_MAX_LEN (%d)!",
            apnLen,LE_MDC_APN_NAME_MAX_LEN);
        return LE_FAULT;
    }

    le_result_t result;
    bool isConnected;

    result = le_mdc_GetSessionState(profileRef, &isConnected);
    if ( (result != LE_OK) || isConnected )
    {
        return LE_NOT_POSSIBLE;
    }

    // We already know that the APN will fit, so strcpy() is okay here
    strcpy(profilePtr->modemData.apn, apnPtr);

    return pa_mdc_WriteProfile(profilePtr->profileIndex, &profilePtr->modemData);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Access Point Name (APN) for the given profile.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the APN is is too long
 *      - LE_NOT_POSSIBLE on failed
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetAPN
(
    le_mdc_ProfileRef_t profileRef, ///< [IN] Query this profile object
    char               *apnPtr,     ///< [OUT] The Access Point Name
    size_t              apnSize     ///< [IN] apnPtr buffer size
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (apnPtr == NULL)
    {
        LE_CRIT("apnStr is NULL !");
        return LE_FAULT;
    }

    if ( pa_mdc_ReadProfile(profilePtr->profileIndex,&profilePtr->modemData) != LE_OK )
    {
        LE_ERROR("Could not read profile at index %d", profilePtr->profileIndex);
        return LE_NOT_POSSIBLE;
    }

    return le_utf8_Copy(apnPtr,profilePtr->modemData.apn,apnSize,NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set authentication property
 *
 * @return
 *      - LE_OK on success
 *
 * @note
 *      - The process exits, if an invalid profile object is given
 * @note If userName is too long (max USER_NAME_MAX_LEN digits), it is a fatal error, the
 *       function will not return.
 * @note If password is too long (max PASSWORD_NAME_MAX_LEN digits), it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_SetAuthentication
(
    le_mdc_ProfileRef_t profileRef, ///< [IN] Query this profile object
    le_mdc_Auth_t       type,       ///< [IN] Authentication type
    const char         *userName,   ///< [IN] UserName used by authentication
    const char         *password    ///< [IN] Password used by authentication
)
{
    le_result_t result;
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (userName == NULL)
    {
        LE_CRIT("userName is NULL !");
        return LE_FAULT;
    }
    if (password == NULL)
    {
        LE_CRIT("password is NULL !");
        return LE_FAULT;
    }
    if ( strlen(userName) > LE_MDC_USER_NAME_MAX_LEN )
    {
        LE_KILL_CLIENT("User name is too long (%zd) > LE_MDC_USER_NAME_MAX_LEN (%d)!",
                        strlen(userName),LE_MDC_USER_NAME_MAX_LEN);
        return LE_FAULT;
    }
    if ( strlen(password) > LE_MDC_PASSWORD_NAME_MAX_LEN )
    {
        LE_KILL_CLIENT("Password name is too long (%zd) > LE_MDC_PASSWORD_NAME_MAX_LEN (%d)!",
                        strlen(password),LE_MDC_PASSWORD_NAME_MAX_LEN);
        return LE_FAULT;
    }

    profilePtr->modemData.authentication.type = type;

    result = le_utf8_Copy(profilePtr->modemData.authentication.userName,
                          userName,
                          sizeof(profilePtr->modemData.authentication.userName),
                          NULL);
    if (result != LE_OK) {
        return result;
    }
    result = le_utf8_Copy(profilePtr->modemData.authentication.password,
                          password,
                          sizeof(profilePtr->modemData.authentication.password),
                          NULL);
    if (result != LE_OK) {
        return result;
    }

    return pa_mdc_WriteProfile(profilePtr->profileIndex, &profilePtr->modemData);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get authentication property
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW userName or password are too small
 *      - LE_NOT_POSSIBLE on failed
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetAuthentication
(
    le_mdc_ProfileRef_t profileRef,    ///< [IN] Query this profile object
    le_mdc_Auth_t      *typePtr,       ///< [OUT] Authentication type
    char               *userNamePtr,   ///< [OUT] UserName used by authentication
    size_t              userNameSize,  ///< [IN] userName buffer size
    char               *passwordPtr,   ///< [OUT] Password used by authentication
    size_t              passwordSize   ///< [IN] password buffer size
)
{
    le_result_t result;
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (typePtr == NULL)
    {
        LE_CRIT("typePtr is NULL !");
        return LE_FAULT;
    }
    if (userNamePtr == NULL)
    {
        LE_CRIT("userNamePtr is NULL !");
        return LE_FAULT;
    }
    if (passwordPtr == NULL)
    {
        LE_CRIT("passwordPtr is NULL !");
        return LE_FAULT;
    }

    if ( pa_mdc_ReadProfile(profilePtr->profileIndex,&profilePtr->modemData) != LE_OK )
    {
        LE_ERROR("Could not read profile at index %d", profilePtr->profileIndex);
        return LE_NOT_POSSIBLE;
    }

    *typePtr = profilePtr->modemData.authentication.type ;

    result = le_utf8_Copy(userNamePtr,
                          profilePtr->modemData.authentication.userName,
                          userNameSize,
                          NULL);
    if (result != LE_OK) {
        return result;
    }
    result = le_utf8_Copy(passwordPtr,
                          profilePtr->modemData.authentication.password,
                          passwordSize,
                          NULL);
    if (result != LE_OK) {
        return result;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the number of profiles on the modem.
 *
 * @return
 *      - number of profile existing on modem
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_mdc_NumProfiles
(
    void
)
{
    return NUM_ARRAY_MEMBERS(ProfileTable);
}
