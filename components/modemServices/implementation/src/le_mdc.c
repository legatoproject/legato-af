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
#include "le_mdc.h"
#include "pa_mdc.h"
#include "le_mdc_local.h"
#include "le_cfg_interface.h"
#include "cfgEntries.h"

// Include macros for printing out values
#include "le_print.h"



//--------------------------------------------------------------------------------------------------
/**
 * Define the maximum size of various profile related fields.
 */
//--------------------------------------------------------------------------------------------------

#define LE_MDC_PROFILE_NAME_MAX_LEN 30
#define LE_MDC_PROFILE_NAME_MAX_BYTES (LE_MDC_PROFILE_NAME_MAX_LEN+1)


//--------------------------------------------------------------------------------------------------
/**
 * Data Control Profile structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mdc_Profile {
    char name[LE_MDC_PROFILE_NAME_MAX_BYTES];   ///< User settable name of the profile
    uint32_t profileIndex;                        ///< Index of the profile on the modem
    uint32_t callRef;                             ///< Reference for current call, if connected
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
 * Create a new data profile
 *
 * @return
 *      - A reference to the data profile
 *      - NULL if the maximum number of profiles has already been created
 *
 * @note
 *      The process exits, if a new data profile could not be created for any reason other than
 *      the maximum number of profiles has been reached.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mdc_Profile_Ref_t CreateProfile
(
    const char* nameStr                 ///< [IN] Name of the profile.
)
{
    le_mdc_Profile_t* profilePtr;
    int idx;

    profilePtr = le_mem_TryAlloc(DataProfilePool);
    if (profilePtr == NULL)
    {
        return NULL;
    }

    memset(profilePtr, 0, sizeof(*profilePtr));

    // It's okay if the name is truncated, since we use strncmp() in the load function
    if (le_utf8_Copy(profilePtr->name, nameStr, sizeof(profilePtr->name), NULL) == LE_OVERFLOW)
    {
        LE_WARN("Profile name '%s' truncated to '%s'.", nameStr, profilePtr->name);
    }

    // Each profile has its own event for reporting session state changes
    profilePtr->sessionStateEvent = le_event_CreateId(profilePtr->name, sizeof(bool));

    // Init the remaining fields
    profilePtr->callRef = 0;

    // Loop through the table until we find the first free modem profile index.  We are
    // guaranteed to find a free entry, otherwise, we would have already exited above.
    for (idx=0; idx<PA_MDC_MAX_PROFILE; idx++)
    {
        LE_DEBUG("ProfileTable[%i] = %p", idx, ProfileTable[idx])

        if (ProfileTable[idx] == NULL)
        {
            // Create a Safe Reference for this data profile object.
            ProfileTable[idx] = le_ref_CreateRef(DataProfileRefMap, profilePtr);
            profilePtr->profileIndex = idx+1;
            break;
        }
    }

    if ( IS_TRACE_ENABLED )
    {
        LE_PRINT_VALUE("%s", profilePtr->modemData.apn);
        LE_PRINT_VALUE("%X", profilePtr->callRef);
        LE_PRINT_VALUE("%s", profilePtr->name);
        LE_PRINT_VALUE("%u", profilePtr->profileIndex);
    }

    return ProfileTable[idx];
}


//--------------------------------------------------------------------------------------------------
/**
 * Store a data profile to the modem.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE on failure
 *      - LE_FAULT if the profile object is invalid.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StoreProfile
(
    le_mdc_Profile_Ref_t profileRef     ///< [IN] The profile object to be stored
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_CRIT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }

    return pa_mdc_WriteProfile(profilePtr->profileIndex, &profilePtr->modemData);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the Access Point Name (APN) for the given profile.
 *
 * The APN must be an ASCII string.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if the APN is is too long or an empty string
 *      - LE_NOT_POSSIBLE if the data session is currently connected for the given profile
 *      - LE_FAULT if the profile object is invalid.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetAPN
(
    le_mdc_Profile_Ref_t profileRef,    ///< [IN] Set APN for this profile object
    const char* apnStr                  ///< [IN] The Access Point Name
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_CRIT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (apnStr == NULL)
    {
        LE_CRIT("apnStr is NULL !");
        return LE_FAULT;
    }

    le_result_t result;
    bool isConnected;
    size_t apnLen;

    result = le_mdc_GetSessionState(profileRef, &isConnected);
    if ( (result != LE_OK) || isConnected )
    {
        return LE_NOT_POSSIBLE;
    }

    apnLen = strlen(apnStr);
    if ( (apnLen == 0) || ((apnLen+1) > sizeof(profilePtr->modemData.apn)) )
    {
        return LE_BAD_PARAMETER;
    }

    // We already know that the APN will fit, so strcpy() is okay here
    strcpy(profilePtr->modemData.apn, apnStr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function when an APN of a profile is change in configDB
 */
//--------------------------------------------------------------------------------------------------
static void ApnProfileUpdate
(
    void* contextPtr
)
{
    char apnName[LIMIT_MAX_PATH_BYTES] = {0};
    char apnConfigPath[LIMIT_MAX_PATH_BYTES] = {0};
    le_mdc_Profile_Ref_t profileRef = (le_mdc_Profile_Ref_t)contextPtr;

    snprintf(apnConfigPath, sizeof(apnConfigPath), "%s/%s",
             CFG_MODEMSERVICE_MDC_PATH,
             profileRef->name);

    LE_DEBUG("AccessPointName change for profile <%s>", profileRef->name);

    le_cfg_iteratorRef_t mdcCfg = le_cfg_CreateReadTxn(apnConfigPath);

    do
    {
        if ( le_cfg_GetString(mdcCfg,CFG_NODE_APN,apnName,sizeof(apnName)) != LE_OK )
        {
            LE_WARN("No APN configuration set for %s profile",profileRef->name);
            break;
        }

        if ( SetAPN(profileRef,apnName) != LE_OK )
        {
            LE_WARN("Could not Set the APN for the profile %s",profileRef->name);
            break;
        }

        LE_DEBUG("New APN <%s> set for profile <%s>",
                 profileRef->modemData.apn,
                 profileRef->name);
    } while (false);

    le_cfg_DeleteIterator(mdcCfg);
}

//--------------------------------------------------------------------------------------------------
/**
 * Create and load a profile from the configuration tree
 *
 * @return
 *      - LE_NOT_FOUND The profile does not exist.
 *      - LE_FAULT     The function failed.
 *      - LE_OK        The function succeed.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadOneProfile
(
    const char *profileNamePtr
)
{
    le_result_t result = LE_OK;
    char apnName[LIMIT_MAX_PATH_BYTES] = {0};
    char apnConfigPath[LIMIT_MAX_PATH_BYTES] = {0};

    char configPath[LIMIT_MAX_PATH_BYTES];
    snprintf(configPath, sizeof(configPath), "%s/%s",
             CFG_MODEMSERVICE_MDC_PATH,
             profileNamePtr);

    le_cfg_iteratorRef_t mdcCfg = le_cfg_CreateReadTxn(configPath);

    do
    {
        // check if the node exits
        if ( le_cfg_IsEmpty(mdcCfg,"") )
        {
            LE_DEBUG("Profile '%s' not found in configTree", profileNamePtr);
            result = LE_NOT_FOUND;
            break;
        }

        if ( le_cfg_GetString(mdcCfg,CFG_NODE_APN,apnName,sizeof(apnName)) != LE_OK )
        {
            LE_WARN("No APN configuration set for %s profile",profileNamePtr);
            result = LE_NOT_FOUND;
            break;
        }

        // Create the profile from the configDB
        le_mdc_Profile_Ref_t profileRef = CreateProfile(profileNamePtr);
        if (!profileRef)
        {
            LE_WARN("Could not create the profile %s",profileNamePtr);
            result = LE_FAULT;
            break;
        }

        if ( SetAPN(profileRef,apnName) != LE_OK )
        {
            LE_WARN("Could not Set the APN for the profile %s",profileNamePtr);
            result = LE_FAULT;
            break;
        }

        if ( StoreProfile(profileRef) != LE_OK )
        {
            LE_WARN("Could not store the profile %s",profileNamePtr);
            result = LE_FAULT;
            break;
        }

        snprintf(apnConfigPath, sizeof(apnConfigPath), "%s/%s/%s",
                 CFG_MODEMSERVICE_MDC_PATH,
                 profileNamePtr,
                 CFG_NODE_APN);
        // Add a configDb handler to check if the APN change.
        le_cfg_AddChangeHandler(apnConfigPath,ApnProfileUpdate,profileRef);
    } while (false);

    le_cfg_DeleteIterator(mdcCfg);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Load All profile from the configuration tree
 */
//--------------------------------------------------------------------------------------------------
static void LoadAllProfileFromConfigDb
(
    void
)
{
    // Check that the modemDataConnection has a configuration value.
    le_cfg_iteratorRef_t mdcCfg = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_MDC_PATH);

    // Check if there is one entry into the configDB, if not add a default one.
    if (le_cfg_GoToFirstChild(mdcCfg) != LE_OK)
    {
        LE_WARN("No configuration for modemServices installed.");
        le_cfg_DeleteIterator(mdcCfg);
        return;
    }

    // Read all profile from configDB
    do
    {
        // Get the profile name.
        char profileName[LIMIT_MAX_PATH_BYTES] = {0};

        if ( le_cfg_GetNodeName(mdcCfg, profileName, sizeof(profileName)) != LE_OK)
        {
            LE_WARN("No Profile configuration for modemServices installed.");
            break;
        }

        // Create and load the profile
        if ( LoadOneProfile(profileName) != LE_OK )
        {
            LE_WARN("Could not load '%s' profile from configTree", profileName);
            break;
        }
    }
    while (le_cfg_GoToNextSibling(mdcCfg) == LE_OK);

    le_cfg_DeleteIterator(mdcCfg);
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

    LoadAllProfileFromConfigDb();
}

// =============================================
//  PUBLIC API FUNCTIONS
// =============================================


//--------------------------------------------------------------------------------------------------
/**
 * Load an existing data profile
 *
 * The profile can be pre-configured in the configuration tree
 *
 * @return
 *      - A reference to the data profile
 *      - NULL if the profile does not exist
 */
//--------------------------------------------------------------------------------------------------
le_mdc_Profile_Ref_t le_mdc_LoadProfile
(
    const char* nameStr                 ///< [IN] Name of the profile.
)
{
    int idx;

    for (idx=0; idx<PA_MDC_MAX_PROFILE; idx++)
    {
        // No need to continue if NULL entry found, since all following entries will also be NULL
        if (ProfileTable[idx] == NULL)
        {
            le_result_t result = LoadOneProfile(nameStr);
            // try to read the configDB to check if there is this profile
            // This should create and load the profile into this good idx.
            if ( result == LE_NOT_FOUND )
            {
                // The profile does not exist
                LE_WARN("The profile '%s' does not exist in configTree",nameStr);
                return NULL;
            }
            else if (result == LE_FAULT )
            {
                LE_ERROR("Could not load and create the profile '%s'",nameStr);
                return NULL;
            }
        }

        le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, ProfileTable[idx]);
        if (profilePtr == NULL)
        {
            LE_CRIT("Invalid reference (%p) found!", ProfileTable[idx]);
            return NULL;
        }
        // Use strncmp, since the stored profile name could have been truncated.
        else if ( strncmp(nameStr, profilePtr->name, sizeof(profilePtr->name)) == 0 )
        {
            return ProfileTable[idx];
        }
    }


    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the name of the given profile.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the name would not fit in buffer
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetProfileName
(
    le_mdc_Profile_Ref_t profileRef,        ///< [IN] Query this profile object
    char*  nameStr,                         ///< [OUT] The name of the profile
    size_t nameStrSize                      ///< [IN] The size in bytes of the name buffer
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }
    if (nameStr == NULL)
    {
        LE_KILL_CLIENT("nameStr is NULL !");
        return LE_FAULT;
    }

    return le_utf8_Copy(nameStr, profilePtr->name, nameStrSize, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Start a data session for the given profile
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
    le_mdc_Profile_Ref_t profileRef     ///< [IN] Start data session for this profile object
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_FAULT;
    }

    return pa_mdc_StartSession(profilePtr->profileIndex, &profilePtr->callRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop a data session for the given profile
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
    le_mdc_Profile_Ref_t profileRef     ///< [IN] Stop data session for this profile object
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
 * Get the current data session state for the given profile
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
    le_mdc_Profile_Ref_t profileRef,        ///< [IN] Query this profile object
    bool* isConnectedPtr                    ///< [OUT] The data session state
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
 *      A handler reference, which is only needed for later removal of the handler.
 *
 * @note
 *      The process exits on failure
 */
//--------------------------------------------------------------------------------------------------
le_mdc_SessionStateHandlerRef_t le_mdc_AddSessionStateHandler
(
    le_mdc_Profile_Ref_t profileRef,        ///< [IN] The profile object of interest
    le_mdc_SessionStateHandlerFunc_t handler,   ///< [IN] The handler function.
    void* contextPtr                        ///< [IN] Context pointer
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
 * Get the name of the network interface for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the interface name would not fit in interfaceNameStr
 *      - LE_NOT_POSSIBLE on any other failure
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetInterfaceName
(
    le_mdc_Profile_Ref_t profileRef,        ///< [IN] Query this profile object
    char*  interfaceNameStr,                ///< [OUT] The name of the network interface
    size_t interfaceNameStrSize             ///< [IN] The size in bytes of the name buffer
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
 * Get the gateway IP address for the given profile, if the data session is connected.
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
le_result_t le_mdc_GetGatewayAddress
(
    le_mdc_Profile_Ref_t profileRef,        ///< [IN] Query this profile object
    char*  gatewayAddrStr,                  ///< [OUT] The gateway IP address in dotted format
    size_t gatewayAddrStrSize               ///< [IN] The size in bytes of the address buffer
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

    return pa_mdc_GetGatewayAddress(profilePtr->profileIndex, gatewayAddrStr, gatewayAddrStrSize);
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
 *      - If only one DNS address is available, then it will be returned, and an empty string will
 *        be returned for the unavailable address
 *      - The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetDNSAddresses
(
    le_mdc_Profile_Ref_t profileRef,        ///< [IN] Query this profile object
    char*  dns1AddrStr,                     ///< [OUT] The primary DNS IP address in dotted format
    size_t dns1AddrStrSize,                 ///< [IN] The size in bytes of the dns1AddrStr buffer
    char*  dns2AddrStr,                     ///< [OUT] The secondary DNS IP address in dotted format
    size_t dns2AddrStrSize                  ///< [IN] The size in bytes of the dns2AddrStr buffer
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

    return pa_mdc_GetDNSAddresses(profilePtr->profileIndex,
                                  dns1AddrStr, dns1AddrStrSize,
                                  dns2AddrStr, dns2AddrStrSize);
}


