/**
 * @file le_mdc.c
 *
 * Implementation of Modem Data Control API
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */


#include "legato.h"
#include "interfaces.h"
#include "pa_mdc.h"

// Include macros for printing out values
#include "le_print.h"

//--------------------------------------------------------------------------------------------------
/**
 * Data Control Profile structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mdc_Profile {
    uint32_t profileIndex;                  ///< Index of the profile on the modem
    void*    callRef;                       ///< Reference for current call, if connected
    le_event_Id_t sessionStateEvent;        ///< Event to report when session changes state
    pa_mdc_ProfileData_t modemData;         ///< Profile data that is written to the modem
    le_mdc_ProfileRef_t profileRef;         ///< Profile Safe Reference
    uint8_t isConnected;                    ///< Session is connected
    le_dls_Link_t link;                     ///< Link into the profile list - Must be the last field
}
le_mdc_Profile_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure holding data related to a SessionState event handler.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mdc_SessionStateHandler {
    le_event_HandlerRef_t handlerRef;
    le_mdc_ProfileRef_t profileRef;
}
le_mdc_SessionStateHandler_t;

//--------------------------------------------------------------------------------------------------
/**
 * List of profile
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t ProfileList;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for data profile objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t DataProfilePool;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for the handlers objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t HandlersPool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for data profile objects.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t DataProfileRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for session state handler objects.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t StateHandlerRefMap;

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
 * Search a profile in the profile dls list.
 */
//--------------------------------------------------------------------------------------------------
static le_mdc_Profile_t* SearchProfileInList
(
    uint32_t index ///< index of the search profile.
)
{
    le_dls_Link_t* profileLinkPtr = le_dls_Peek(&ProfileList);

    while(profileLinkPtr)
    {
        le_mdc_Profile_t* profilePtr = CONTAINER_OF(profileLinkPtr,le_mdc_Profile_t,link);

        if ( profilePtr->profileIndex == index )
        {
            if ( IS_TRACE_ENABLED )
            {
                LE_PRINT_VALUE("profile %d found", index);
            }

            return profilePtr;
        }
        else
        {
            profileLinkPtr = le_dls_PeekNext(&ProfileList,profileLinkPtr);
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer New Session State Change Handler.
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

    if ( IS_TRACE_ENABLED )
    {
        LE_PRINT_VALUE("%i", sessionStatePtr->profileIndex);
        LE_PRINT_VALUE("%i", sessionStatePtr->newState);
    }

    // Search the profile
    le_mdc_Profile_t* profilePtr = SearchProfileInList(sessionStatePtr->profileIndex);

    if (profilePtr == NULL)
    {
        LE_DEBUG("Reference not created");
    }
    else
    {
        // Init the data for the event report
        isConnected = (sessionStatePtr->newState != PA_MDC_DISCONNECTED);

        LE_DEBUG("profileIndex %d isConnected %d", sessionStatePtr->profileIndex,
                                                    profilePtr->isConnected);
        LE_DEBUG("isConnected %d",isConnected);

        if (profilePtr->isConnected != isConnected)
        {

            // Report the event for the given profile
            le_event_Report(profilePtr->sessionStateEvent, &isConnected, sizeof(isConnected));
            profilePtr->isConnected = isConnected;
        }
    }

    // Free the received report data
    le_mem_Release(sessionStatePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Destructor function that runs when a profile is deallocated
 */
//--------------------------------------------------------------------------------------------------
static void DataProfileDestructor (void *objPtr)
{
    le_mdc_Profile_t* profilePtr = (le_mdc_Profile_t*) objPtr;

    if ( le_dls_IsInList(&ProfileList, &profilePtr->link) )
    {
        /* Remove the profile from the dls list */
        le_dls_Remove(&ProfileList, &profilePtr->link);

        /* Release the reference */
        le_ref_DeleteRef(DataProfileRefMap, profilePtr->profileRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a modem data profile
 *
 * @note
 *      The process exits, if a new data profile could not be created for any reason other than
 *      the maximum number of profiles has been reached.
 */
//--------------------------------------------------------------------------------------------------
static le_mdc_ProfileRef_t CreateModemProfile
(
    uint32_t index ///< index of the profile.
)
{
    le_mdc_Profile_t* profilePtr;

    // Search the profile
    profilePtr = SearchProfileInList(index);

    // Profile doesn't exist
    if (!profilePtr)
    {
        char eventName[32] = {0};

        if ( pa_mdc_InitializeProfile(index) != LE_OK )
        {
            return NULL;
        }

        profilePtr = le_mem_ForceAlloc(DataProfilePool);

        if (profilePtr == NULL)
        {
            return NULL;
        }

        memset(profilePtr, 0, sizeof(le_mdc_Profile_t));

        // Read the requested profile
        if ( pa_mdc_ReadProfile(index, &profilePtr->modemData) != LE_OK )
        {
            le_mem_Release(profilePtr);

            return NULL;
        }

        profilePtr->profileIndex = index;

        // Each profile has its own event for reporting session state changes
        snprintf(eventName,sizeof(eventName)-1, "profile-%d",index);
        profilePtr->sessionStateEvent = le_event_CreateId(eventName, sizeof(bool));

        // Init the remaining fields
        profilePtr->callRef = 0;

        profilePtr->isConnected = 0xFF;

        // Init the link
        profilePtr->link = LE_DLS_LINK_INIT;

        // Add the profile to the Profile List
        le_dls_Stack(&ProfileList, &(profilePtr->link));

        if ( IS_TRACE_ENABLED )
        {
            LE_PRINT_VALUE("%u", index);
        }

        // Create a Safe Reference for this data profile object.
        profilePtr->profileRef = le_ref_CreateRef(DataProfileRefMap, profilePtr);
    }

    return profilePtr->profileRef;
}

// =============================================
//  MODULE/COMPONENT FUNCTIONS
// =============================================


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the MDC component.
 *
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
    le_mem_SetDestructor(DataProfilePool, DataProfileDestructor);

    // Allocate the handlers pool, and set the max number of objects, since it is already known.
    HandlersPool = le_mem_CreatePool("HandlersPool", sizeof(le_mdc_SessionStateHandler_t));
    le_mem_ExpandPool(HandlersPool, PA_MDC_MAX_PROFILE);

    // Create the Safe Reference Map to use for data profile object Safe References.
    DataProfileRefMap = le_ref_CreateMap("DataProfileMap", PA_MDC_MAX_PROFILE);

    // Create the Safe Reference Map to use for the session state handler object Safe References.
    StateHandlerRefMap = le_ref_CreateMap("StateHandlerRefMap", PA_MDC_MAX_PROFILE);

    // Subscribe to the session state handler
    pa_mdc_AddSessionStateHandler(NewSessionStateHandler, NULL);

    // Initialize the profile list
    ProfileList = LE_DLS_LIST_INIT;
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
    if ( index == LE_MDC_DEFAULT_PROFILE )
    {
        if (pa_mdc_GetDefaultProfileIndex(&index) != LE_OK)
        {
            return NULL;
        }
    }

    return CreateModemProfile(index);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a profile selected by its APN
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if an input parameter is not valid
 *      - LE_NOT_FOUND if the requested APN is not found
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetProfileFromApn
(
    const char* apnPtr,                 ///< [IN] The Access Point Name
    le_mdc_ProfileRef_t* profileRefPtr  ///< [OUT] profile reference
)
{
    if (apnPtr == NULL)
    {
        LE_CRIT("apnPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    size_t apnLen = strlen(apnPtr);
    if ( apnLen > LE_MDC_APN_NAME_MAX_LEN )
    {
        LE_CRIT("apnStr is too long (%zd) > LE_MDC_APN_NAME_MAX_LEN (%d)!",
            apnLen,LE_MDC_APN_NAME_MAX_LEN);
        return LE_BAD_PARAMETER;
    }

    /* Look for current radio technology */
    le_mrc_Rat_t rat;
    uint32_t profileIndexMax;
    uint32_t profileIndex;

    if (le_mrc_GetRadioAccessTechInUse(&rat) != LE_OK)
    {
        rat = LE_MRC_RAT_GSM;
    }

    switch (rat)
    {
        /* 3GPP2 */
        case LE_MRC_RAT_CDMA:
            profileIndex = PA_MDC_MIN_INDEX_3GPP2_PROFILE;
            profileIndexMax = PA_MDC_MAX_INDEX_3GPP2_PROFILE;
        break;

        /* 3GPP */
        default:
            profileIndex = PA_MDC_MIN_INDEX_3GPP_PROFILE;
            profileIndexMax = PA_MDC_MAX_INDEX_3GPP_PROFILE;
        break;
    }

    for (; profileIndex <= profileIndexMax; profileIndex++)
    {
        char tmpApn[LE_MDC_APN_NAME_MAX_LEN];

        memset(tmpApn, 0, LE_MDC_APN_NAME_MAX_LEN);

        *profileRefPtr = le_mdc_GetProfile(profileIndex);

        le_mdc_GetAPN(*profileRefPtr, tmpApn, LE_MDC_APN_NAME_MAX_LEN);

        if ( strncmp(apnPtr, tmpApn, apnLen ) == 0 )
        {
            return LE_OK;
        }
    }

    return LE_NOT_FOUND;
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
 *      - LE_BAD_PARAMETER if input parameter is incorrect
 *      - LE_DUPLICATE if the data session is already connected for the given profile
 *      - LE_FAULT for other failures
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
    le_result_t result;
    pa_mdc_CallRef_t callRef = NULL;

    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_BAD_PARAMETER;
    }

    switch (profilePtr->modemData.pdp)
    {
        case LE_MDC_PDP_IPV4:
        {
            result = pa_mdc_StartSessionIPV4(profilePtr->profileIndex, &callRef);
        }
        break;
        case LE_MDC_PDP_IPV6:
        {
            result =  pa_mdc_StartSessionIPV6(profilePtr->profileIndex, &callRef);
        }
        break;
        case LE_MDC_PDP_IPV4V6:
        {
            result =  pa_mdc_StartSessionIPV4V6(profilePtr->profileIndex, &callRef);
        }
        break;
        default:
            return LE_FAULT;
    }

    if ( ( result == LE_OK ) && callRef )
    {
        profilePtr->callRef = callRef;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop profile data session.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if the input parameter is not valid
 *      - LE_FAULT for other failures
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
        return LE_BAD_PARAMETER;
    }

    return pa_mdc_StopSession(profilePtr->callRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the current data session state.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if an input parameter is not valid
 *      - LE_FAULT on failure
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
        return LE_BAD_PARAMETER;
    }
    if (isConnectedPtr == NULL)
    {
        LE_KILL_CLIENT("isConnectedPtr is NULL !");
        return LE_BAD_PARAMETER;
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

    le_mdc_SessionStateHandler_t* sessionStateHandlerPtr = le_mem_ForceAlloc(HandlersPool);
    sessionStateHandlerPtr->profileRef = profileRef;
    sessionStateHandlerPtr->handlerRef = le_event_AddLayeredHandler("le_NewSessionStateHandler",
                                                                  profilePtr->sessionStateEvent,
                                                                  FirstLayerSessionStateChangeHandler,
                                                                  (le_event_HandlerFunc_t)handler);

    le_event_SetContextPtr(sessionStateHandlerPtr->handlerRef, contextPtr);

    le_mdc_SessionStateHandlerRef_t HandlerRef = le_ref_CreateRef( StateHandlerRefMap,
                                                                sessionStateHandlerPtr );

    LE_DEBUG("%p %p", sessionStateHandlerPtr, HandlerRef);

     return HandlerRef;
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
    le_mdc_SessionStateHandlerRef_t    sessionStateHandlerRef ///< [IN] The handler reference.
)
{
    le_mdc_SessionStateHandler_t* sessionStateHandlerPtr = le_ref_Lookup(StateHandlerRefMap,
                                                                         sessionStateHandlerRef);

    if (!sessionStateHandlerPtr)
    {
        LE_KILL_CLIENT("Invalid parameter (%p) found!", sessionStateHandlerRef);
        return;
    }

    le_event_RemoveHandler(sessionStateHandlerPtr->handlerRef);

    /* Release the reference */
    le_ref_DeleteRef(StateHandlerRefMap, sessionStateHandlerRef);

    /* Release the handlerRef */
    le_mem_Release(sessionStateHandlerPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the network interface name, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the interface name can't fit in interfaceNameStr
 *      - LE_FAULT on any other failure
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
 *      - LE_FAULT for all other errors
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
 *      - LE_FAULT for all other errors
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
 *      - LE_FAULT for all other errors
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
 *      - LE_FAULT for all other errors
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
 *      - LE_FAULT for all other errors
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
 *      - LE_FAULT for all other errors
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
 *      - LE_FAULT for all other errors
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
 *      - LE_FAULT for all other errors
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
 *      - LE_FAULT for all other errors
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
 *      - LE_FAULT if the data session is currently connected for the given profile
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
        return LE_BAD_PARAMETER;
    }

    le_result_t result;
    bool isConnected;

    result = le_mdc_GetSessionState(profileRef, &isConnected);
    if ( (result != LE_OK) || isConnected )
    {
        return LE_FAULT;
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
 *      - LE_BAD_PARAMETER if an input parameter is not valid
 *      - LE_FAULT if the data session is currently connected for the given profile
 *
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
        return LE_BAD_PARAMETER;
    }
    if (apnPtr == NULL)
    {
        LE_CRIT("apnStr is NULL !");
        return LE_BAD_PARAMETER;
    }

    size_t apnLen = strlen(apnPtr);
    if ( apnLen > LE_MDC_APN_NAME_MAX_LEN )
    {
        LE_CRIT("apnStr is too long (%zd) > LE_MDC_APN_NAME_MAX_LEN (%d)!",
            apnLen,LE_MDC_APN_NAME_MAX_LEN);
        return LE_BAD_PARAMETER;
    }

    le_result_t result;
    bool isConnected;

    result = le_mdc_GetSessionState(profileRef, &isConnected);
    if ( (result != LE_OK) || isConnected )
    {
        return LE_FAULT;
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
 *      - LE_BAD_PARAMETER if an input parameter is not valid
 *      - LE_OVERFLOW if the APN is is too long
 *      - LE_FAULT on failed
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
        return LE_BAD_PARAMETER;
    }
    if (apnPtr == NULL)
    {
        LE_CRIT("apnStr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if ( pa_mdc_ReadProfile(profilePtr->profileIndex,&profilePtr->modemData) != LE_OK )
    {
        LE_ERROR("Could not read profile at index %d", profilePtr->profileIndex);
        return LE_FAULT;
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
 *      - LE_BAD_PARAMETER if an input parameter is not valid
 *      - LE_OVERFLOW userName or password are too small
 *      - LE_FAULT on failed
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
        return LE_BAD_PARAMETER;
    }
    if (typePtr == NULL)
    {
        LE_CRIT("typePtr is NULL !");
        return LE_BAD_PARAMETER;
    }
    if (userNamePtr == NULL)
    {
        LE_CRIT("userNamePtr is NULL !");
        return LE_BAD_PARAMETER;
    }
    if (passwordPtr == NULL)
    {
        LE_CRIT("passwordPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    if ( pa_mdc_ReadProfile(profilePtr->profileIndex,&profilePtr->modemData) != LE_OK )
    {
        LE_ERROR("Could not read profile at index %d", profilePtr->profileIndex);
        return LE_FAULT;
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
    return PA_MDC_MAX_PROFILE;
}
