/**
 * @file le_mdc.c
 *
 * Implementation of Modem Data Control API
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */


#include "legato.h"
#include "interfaces.h"
#include "le_print.h"
#include "jansson.h"
#include "mdmCfgEntries.h"
#include "pa_mdc.h"
#include "le_ms_local.h"
#include "watchdogChain.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The file to read for the APN.
 */
//--------------------------------------------------------------------------------------------------
#ifdef LEGATO_EMBEDDED
#define APN_IIN_FILE    \
    "/legato/systems/current/apps/modemService/read-only/usr/local/share/apns-iin.json"
#define APN_MCCMNC_FILE \
    "/legato/systems/current/apps/modemService/read-only/usr/local/share/apns-mccmnc.json"
#else
#define APN_IIN_FILE    le_arg_GetArg(0)
#define APN_MCCMNC_FILE le_arg_GetArg(1)
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of profile objects supported
 */
//--------------------------------------------------------------------------------------------------
#define LE_MDC_MAX_PROFILE_INDEX            16

//--------------------------------------------------------------------------------------------------
/**
 * MDC command Type.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    STOP_SESSION  = 0,  ///< Stop data session
    START_SESSION = 1   ///< Start data session
}
CmdType_t;

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Data Control Profile structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t profileIndex;                         ///< Index of the profile on the modem
    le_event_Id_t sessionStateEvent;               ///< Event to report when session changes state
    pa_mdc_ProfileData_t modemData;                ///< Profile data that is written to the modem
    le_mdc_ProfileRef_t profileRef;                ///< Profile Safe Reference
    le_mdc_ConState_t connectionStatus;            ///< Data session connection status
    pa_mdc_ConnectionFailureCode_t* conFailurePtr; ///< connection or disconnection failure reason
}
le_mdc_Profile_t;

//--------------------------------------------------------------------------------------------------
/**
 * Request command structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    CmdType_t                   command;             ///< Command Request.
    le_mdc_ProfileRef_t         profileRef;          ///< Profile reference.
    le_mdc_SessionHandlerFunc_t handlerFunc;         ///< The Handler function.
    void                        *contextPtr;         ///< Context.
}
CmdRequest_t;

//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Data statistics
 */
//--------------------------------------------------------------------------------------------------
static pa_mdc_PktStatistics_t DataStatistics;

//--------------------------------------------------------------------------------------------------
/**
 * MT-PDP change handler counter
 */
//--------------------------------------------------------------------------------------------------
static uint8_t MtPdpStateChangeHandlerCounter;

//--------------------------------------------------------------------------------------------------
/**
 * The memory pool for data profile objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t DataProfilePool;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for data profile objects.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t DataProfileRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Event IDs for MT-PDP notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t MtPdpEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for sending commands.
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t CommandEventId;

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
static le_mdc_ProfileRef_t CreateModemProfile( uint32_t index );

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
    le_ref_IterRef_t iterRef = le_ref_GetIterator(DataProfileRefMap);

    while (le_ref_NextNode(iterRef) == LE_OK)
    {
        le_mdc_Profile_t* profilePtr = (le_mdc_Profile_t*) le_ref_GetValue(iterRef);

        if ( profilePtr->profileIndex == index )
        {
            if ( IS_TRACE_ENABLED )
            {
                LE_PRINT_VALUE("%d", index);
            }

            return profilePtr;
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
    le_mdc_Profile_t** profilePtr = reportPtr;
    le_mdc_SessionStateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(  (*profilePtr)->profileRef,
                        (*profilePtr)->connectionStatus,
                        le_event_GetContextPtr()
                     );
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
    if ( IS_TRACE_ENABLED )
    {
        LE_PRINT_VALUE("%i", sessionStatePtr->profileIndex);
        LE_PRINT_VALUE("%i", sessionStatePtr->newState);
    }

    // Search the profile
    le_mdc_Profile_t* profilePtr = SearchProfileInList(sessionStatePtr->profileIndex);

    // All notifications except MT-PDP incoming
    if(sessionStatePtr->newState != LE_MDC_INCOMING)
    {
        if (profilePtr == NULL)
        {
             LE_WARN("Reference not created");
        }
        else
        {
            LE_DEBUG("profileIndex %d, old connection status %d, new state %d, pdp Type %d",
                                                            sessionStatePtr->profileIndex,
                                                            profilePtr->connectionStatus,
                                                            sessionStatePtr->newState,
                                                            sessionStatePtr->pdp);
            // Event report
            if (profilePtr->connectionStatus != sessionStatePtr->newState)
            {
                // Report the event for the given profile
                le_event_Report(profilePtr->sessionStateEvent, &profilePtr, sizeof(profilePtr));
                // Update Connection Status
                profilePtr->connectionStatus = sessionStatePtr->newState;
            }
        }
    }
    else // MT-PDP incoming notification
    {
        // Profile doesn't exist and should be created
        if(profilePtr == NULL)
        {
            LE_DEBUG("MT-PDP profile created - index %d", sessionStatePtr->profileIndex);
            le_mdc_ProfileRef_t profileRef = CreateModemProfile(sessionStatePtr->profileIndex);
            profilePtr =  le_ref_Lookup(DataProfileRefMap, profileRef);
        }
        else
        {
            LE_DEBUG("MT-PDP profile found - index %d", sessionStatePtr->profileIndex);
        }

        // MT-PDP notification management
        if(profilePtr != NULL)
        {
            // Check if an handler has been subscribed by the application
            if(!MtPdpStateChangeHandlerCounter)
            {
                LE_WARN("MT-PDP request automatically rejected");
                pa_mdc_RejectMtPdpSession(sessionStatePtr->profileIndex);
            }
            else // Event report
            {
                // Update profile
                profilePtr->sessionStateEvent= MtPdpEventId;
                profilePtr->connectionStatus = sessionStatePtr->newState;
                // Report the MT-PDP notification event with the given profile
                le_event_Report(MtPdpEventId, &profilePtr, sizeof(profilePtr));
            }
        }
        else
        {
            LE_ERROR("MT-PDP profile not found");
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

    /* Release the reference */
    le_ref_DeleteRef(DataProfileRefMap, profilePtr->profileRef);
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

        profilePtr = le_mem_ForceAlloc(DataProfilePool);

        if (profilePtr == NULL)
        {
            return NULL;
        }

        memset(profilePtr, 0, sizeof(le_mdc_Profile_t));

        profilePtr->profileIndex = index;

        // Each profile has its own event for reporting session state changes
        snprintf(eventName,sizeof(eventName)-1, "profile-%d",index);
        profilePtr->sessionStateEvent = le_event_CreateId(eventName, sizeof(le_mdc_Profile_t*));

        // Init the remaining fields
        profilePtr->connectionStatus = -1;
        profilePtr->conFailurePtr = NULL;

        // Create a Safe Reference for this data profile object.
        profilePtr->profileRef = le_ref_CreateRef(DataProfileRefMap, profilePtr);
    }

    LE_DEBUG("profileRef %p created for index %d",  profilePtr->profileRef, index);

    return profilePtr->profileRef;
}

// -------------------------------------------------------------------------------------------------
/**
 *  This function will attempt to read APN definition for MCC/MNC in file apnFilePtr
 *
 * @return LE_OK        Function was able to find an APN
 * @return LE_NOT_FOUND Function was not able to find an APN for this (MCC,MNC)
 * @return LE_FAULT     There was an issue with the APN source
 */
// -------------------------------------------------------------------------------------------------
static le_result_t FindApnWithMccMncFromFile
(
    const char* apnFilePtr, ///< [IN]  apn file
    const char* mccPtr,     ///< [IN]  mcc
    const char* mncPtr,     ///< [IN]  mnc
    char * mccMncApnPtr,    ///< [OUT] apn for mcc/mnc
    size_t mccMncApnSize    ///< [IN]  size of mccMncApn buffer
)
{
    le_result_t result = LE_FAULT;
    json_t *root, *apns, *apnArray;
    json_error_t error;
    int i;

    root = json_load_file(apnFilePtr, 0, &error);
    if (NULL == root)
    {
        LE_WARN("Document not parsed successfully (error '%s')", error.text);
        return result;
    }

    apns = json_object_get(root, "apns");
    if (!json_is_object(apns))
    {
        LE_WARN("apns is not an object");
        json_decref(root);
        return result;
    }

    apnArray = json_object_get(apns, "apn");
    if (!json_is_array(apnArray))
    {
        LE_WARN("apns is not an array");
        json_decref(root);
        return result;
    }

    result = LE_NOT_FOUND;

    for (i = 0; i < json_array_size(apnArray); i++)
    {
        json_t *data, *mcc, *mnc, *apn, *type;
        const char* mccRead;
        const char* mncRead;
        const char* apnRead;
        const char* typeRead;

        data = json_array_get(apnArray, i);
        if (!json_is_object(data))
        {
            LE_WARN("data %d is not an object", i);
            result = LE_FAULT;
            break;
        }

        mcc = json_object_get(data, "@mcc");
        mccRead = json_string_value(mcc);

        mnc = json_object_get(data, "@mnc");
        mncRead = json_string_value(mnc);

        type = json_object_get(data, "@type");
        if (!json_is_string(type))
        {
            // No type set for this carrier, set it to "default"
            typeRead = "default";
        }
        else
        {
            typeRead = json_string_value(type);
        }

        if (   (NULL != strstr(typeRead, "default"))    // Consider only "default" type for APN
            && !(strcmp(mccRead, mccPtr))
            && !(strcmp(mncRead, mncPtr))
           )
        {
            apn = json_object_get(data, "@apn");
            apnRead = json_string_value(apn);

            if (LE_OK != le_utf8_Copy(mccMncApnPtr, apnRead, mccMncApnSize, NULL))
            {
                LE_WARN("APN buffer is too small");
                break;
            }
            LE_INFO("Got APN '%s' for MCC/MNC [%s/%s]", mccMncApnPtr, mccPtr, mncPtr);

            // @note Stop on the first JSON entry for MCC/MNC with type default:
            // needs to be improved?
            result = LE_OK;
            break;
        }
    }

    json_decref(root);
    return result;
}

// -------------------------------------------------------------------------------------------------
/**
 *  This function will attempt to read APN definition for ICCID in file apnFilePtr
 *
 * @return LE_OK        Function was able to find an APN
 * @return LE_NOT_FOUND Function was not able to find an APN for this ICCID
 * @return LE_FAULT     There was an issue with the APN source
 */
// -------------------------------------------------------------------------------------------------
static le_result_t FindApnWithIccidFromFile
(
    const char* apnFilePtr, ///< [IN]  apn file
    const char* iccidPtr,   ///< [IN]  iccid
    char * iccidApnPtr,     ///< [OUT] apn for iccid
    size_t iccidApnSize     ///< [IN]  size of iccidApn buffer
)
{
    le_result_t result = LE_FAULT;
    json_t *root, *apns, *apnArray;
    json_error_t error;
    int i;

    root = json_load_file(apnFilePtr, 0, &error);
    if (NULL == root)
    {
        LE_WARN("Document not parsed successfully (error '%s')", error.text);
        return result;
    }

    apns = json_object_get(root, "apns");
    if (!json_is_object(apns))
    {
        LE_WARN("apns is not an object");
        json_decref(root);
        return result;
    }

    apnArray = json_object_get(apns, "apn");
    if (!json_is_array(apnArray))
    {
        LE_WARN("apns is not an array");
        json_decref(root);
        return result;
    }

    result = LE_NOT_FOUND;

    for (i = 0; i < json_array_size(apnArray); i++)
    {
        json_t *data, *iin, *apn;
        const char* iinRead;
        const char* apnRead;

        data = json_array_get(apnArray, i);
        if (!json_is_object(data))
        {
            LE_WARN("data %d is not an object", i);
            result = LE_FAULT;
            break;
        }

        // Retrieve Issuer Identification Number (IIN), which is the beginning
        // of the ICCID number and allows identifying an operator (cf. ITU Rec E.118)
        iin = json_object_get(data, "@iin");
        if (json_is_string(iin))
        {
            iinRead = json_string_value(iin);

            // Check if IIN matches the beginning of ICCID
            if (0 == strncmp(iccidPtr, iinRead, strlen(iinRead)))
            {
                apn = json_object_get(data, "@apn");
                apnRead = json_string_value(apn);

                if (LE_OK != le_utf8_Copy(iccidApnPtr, apnRead, iccidApnSize, NULL))
                {
                    LE_WARN("APN buffer is too small");
                    break;
                }
                LE_INFO("Got APN '%s' for ICCID %s", iccidApnPtr, iccidPtr);

                // @note Stop on the first JSON entry for IIN with type default:
                // needs to be improved?
                result = LE_OK;
                break;
            }
        }
    }

    json_decref(root);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a command
 */
//--------------------------------------------------------------------------------------------------
static void ProcessCommandEventHandler
(
    void* msgCommand
)
{
    le_result_t result = LE_BAD_PARAMETER;
    CmdRequest_t* cmdRequestPtr = msgCommand;
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, cmdRequestPtr->profileRef);

    if (profilePtr == NULL)
    {
        LE_ERROR("Invalid reference (%p) found!", cmdRequestPtr->profileRef);
    }
    else
    {
        if (cmdRequestPtr->command == START_SESSION)
        {
            result = le_mdc_StartSession(cmdRequestPtr->profileRef);
            if (result != LE_OK)
            {
                LE_ERROR("le_mdc_StartSession error %d", result);
            }
        }
        else if(cmdRequestPtr->command == STOP_SESSION)
        {
            result = le_mdc_StopSession(cmdRequestPtr->profileRef);
            if (result != LE_OK)
            {
                LE_ERROR("le_mdc_StopSession error %d", result);
            }
        }
        else
        {
            LE_ERROR("Command undefined");
        }

        // Check if a handler function is available.
        if (cmdRequestPtr->handlerFunc)
        {
            LE_DEBUG("Calling Handler (%p), Status %d", cmdRequestPtr->handlerFunc, result);
            cmdRequestPtr->handlerFunc( cmdRequestPtr->profileRef,
                                        result,
                                        cmdRequestPtr->contextPtr );
        }
        else
        {
            LE_WARN("No CallhandlerFunction, status %d!!", result);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This thread stacks queries and starts or stops the data session.
 */
//--------------------------------------------------------------------------------------------------
static void* CommandThread
(
    void* contextPtr
)
{
    le_sem_Ref_t initSemaphore = (le_sem_Ref_t)contextPtr;

    // Connect to services used by this thread
    le_cfg_ConnectService();

    // Register for MDC command events
    le_event_AddHandler("ProcessCommandHandler", CommandEventId, ProcessCommandEventHandler);

    le_sem_Post(initSemaphore);

    // Monitor event loop
    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_MonitorEventLoop(MS_WDOG_MDC_LOOP, watchdogInterval);

    // Run the event loop
    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the data counter activation state
 */
//--------------------------------------------------------------------------------------------------
static bool GetDataCounterState
(
    void
)
{
    bool activationState;
    le_cfg_IteratorRef_t iteratorRef;

    iteratorRef = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_MDC_PATH);
    activationState = le_cfg_GetBool(iteratorRef, CFG_NODE_COUNTING, true);
    le_cfg_CancelTxn(iteratorRef);

    LE_DEBUG("Retrieved data counter activation state: %d", activationState);

    return activationState;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the message counting state
 */
//--------------------------------------------------------------------------------------------------
static void SetDataCounterState
(
    bool activationState    ///< New data counter activation state
)
{
    le_cfg_IteratorRef_t iteratorRef;

    LE_DEBUG("New data counter activation state: %d", activationState);

    iteratorRef = le_cfg_CreateWriteTxn(CFG_MODEMSERVICE_MDC_PATH);
    le_cfg_SetBool(iteratorRef, CFG_NODE_COUNTING, activationState);
    le_cfg_CommitTxn(iteratorRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the saved data counters
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetDataCounters
(
    uint64_t* rxBytesPtr,   ///< Received bytes
    uint64_t* txBytesPtr    ///< Transmitted bytes
)
{
    le_cfg_IteratorRef_t iteratorRef;

    iteratorRef = le_cfg_CreateReadTxn(CFG_MODEMSERVICE_MDC_PATH);
    *rxBytesPtr = le_cfg_GetFloat(iteratorRef, CFG_NODE_RX_BYTES, 0);
    *txBytesPtr = le_cfg_GetFloat(iteratorRef, CFG_NODE_TX_BYTES, 0);
    le_cfg_CancelTxn(iteratorRef);

    LE_DEBUG("Saved rxBytes=%"PRIu64", txBytes=%"PRIu64, *rxBytesPtr, *txBytesPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the saved data counters
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetDataCounters
(
    uint64_t rxBytes,   ///< Received bytes
    uint64_t txBytes    ///< Transmitted bytes
)
{
    le_cfg_IteratorRef_t iteratorRef;

    iteratorRef = le_cfg_CreateWriteTxn(CFG_MODEMSERVICE_MDC_PATH);
    le_cfg_SetFloat(iteratorRef, CFG_NODE_RX_BYTES, rxBytes);
    le_cfg_SetFloat(iteratorRef, CFG_NODE_TX_BYTES, txBytes);
    le_cfg_CommitTxn(iteratorRef);

    LE_DEBUG("Saved rxBytes=%"PRIu64", txBytes=%"PRIu64, rxBytes, txBytes);

    return LE_OK;
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

    // Create the Safe Reference Map to use for data profile object Safe References.
    DataProfileRefMap = le_ref_CreateMap("DataProfileMap", PA_MDC_MAX_PROFILE);

    // Subscribe to the session state handler
    pa_mdc_AddSessionStateHandler(NewSessionStateHandler, NULL);

    // Initialize data counter state and values
    if (GetDataCounterState())
    {
        pa_mdc_StartDataFlowStatistics();
    }
    else
    {
        pa_mdc_StopDataFlowStatistics();
    }
    GetDataCounters(&DataStatistics.receivedBytesCount, &DataStatistics.transmittedBytesCount);

    /* MT-PDP management */
    // Create an event Id for MT-PDP notification
    MtPdpEventId = le_event_CreateId("MtPdpNotif", sizeof(le_mdc_Profile_t*));

    CommandEventId = le_event_CreateId("CommandEventId", sizeof(CmdRequest_t));

    // initSemaphore is used to wait for CommandThread() execution. It ensures that the thread is
    // ready when we exit from le_mdc_Init().
    le_sem_Ref_t initSemaphore = le_sem_Create("InitSem", 0);
    le_thread_Start(le_thread_Create(WDOG_THREAD_NAME_MDC_COMMAND_EVENT,
                                     CommandThread,
                                     (void*)initSemaphore));
    le_sem_Wait(initSemaphore);
    le_sem_Delete(initSemaphore);

    //  MT-PDP change handler counter initialization
    MtPdpStateChangeHandlerCounter = 0;
}

// =============================================
//  PUBLIC API FUNCTIONS
// =============================================

//--------------------------------------------------------------------------------------------------
/**
 * Get Profile Reference for index
 *
 * @note Create a new profile if the profile's index can't be found.
 *
 * @warning 0 is not a valid index.
 *
 * @warning Ensure to check the list of supported data profiles for your specific platform.
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
    if ( 0 == index )
    {
       LE_ERROR("index 0 is not valid!");
       return NULL;
    }
    else if ( LE_MDC_DEFAULT_PROFILE == index )
    {
        if (pa_mdc_GetDefaultProfileIndex(&index) != LE_OK)
        {
            return NULL;
        }
    }
    else if ( LE_MDC_SIMTOOLKIT_BIP_DEFAULT_PROFILE == index )
    {
        if (pa_mdc_GetBipDefaultProfileIndex(&index) != LE_OK)
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
        pa_mdc_ProfileData_t profileData;

        *profileRefPtr = NULL;

        if ( pa_mdc_ReadProfile(profileIndex, &profileData) == LE_OK )
        {
            if ( strncmp(apnPtr, profileData.apn, apnLen ) == 0 )
            {
                *profileRefPtr = le_mdc_GetProfile(profileIndex);
                return LE_OK;
            }
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
 *      - LE_TIMEOUT for session start timeout
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
    le_mdc_Pdp_t pdpType;

    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_BAD_PARAMETER;
    }

    pdpType = le_mdc_GetPDP(profileRef);
    switch (pdpType)
    {
        case LE_MDC_PDP_IPV4:
        {
            result = pa_mdc_StartSessionIPV4(profilePtr->profileIndex);
        }
        break;
        case LE_MDC_PDP_IPV6:
        {
            result = pa_mdc_StartSessionIPV6(profilePtr->profileIndex);
        }
        break;
        case LE_MDC_PDP_IPV4V6:
        {
            result = pa_mdc_StartSessionIPV4V6(profilePtr->profileIndex);
        }
        break;
        default:
        {
            LE_DEBUG("Unknown PDP type %d", pdpType);
            return LE_FAULT;
        }
    }

    if ((LE_OK != result) && (LE_MDC_PDP_UNKNOWN != pdpType))
    {
        if (LE_MDC_PDP_IPV4V6 == pdpType)
        {
            pa_mdc_GetConnectionFailureReasonExt(profilePtr->profileIndex,
                                                LE_MDC_PDP_IPV4, &(profilePtr->conFailurePtr));
            if (NULL == profilePtr->conFailurePtr)
            {
                LE_ERROR("conFailurePtr is NULL");
                return LE_FAULT;
            }
            LE_ERROR("Get IPv4v6 Async Connection failureV4 %d, %d, %d, %d",
                profilePtr->conFailurePtr->callEndFailure,
                profilePtr->conFailurePtr->callEndFailureCode,
                profilePtr->conFailurePtr->callConnectionFailureType,
                profilePtr->conFailurePtr->callConnectionFailureCode);

            pa_mdc_GetConnectionFailureReasonExt(profilePtr->profileIndex,
                                                LE_MDC_PDP_IPV6, &(profilePtr->conFailurePtr));
            if (NULL == profilePtr->conFailurePtr)
            {
                LE_ERROR("conFailurePtr is NULL");
                return LE_FAULT;
            }
            LE_ERROR("Get IPv4v6 Async Connection failureV6 %d, %d, %d, %d",
                profilePtr->conFailurePtr->callEndFailure,
                profilePtr->conFailurePtr->callEndFailureCode,
                profilePtr->conFailurePtr->callConnectionFailureType,
                profilePtr->conFailurePtr->callConnectionFailureCode);
        }
        else
        {
            pa_mdc_GetConnectionFailureReason(profilePtr->profileIndex,
                                              &(profilePtr->conFailurePtr));
            if (NULL == profilePtr->conFailurePtr)
            {
                LE_ERROR("conFailurePtr is NULL");
                return LE_FAULT;
            }
            LE_ERROR("Get Async Connection failure %d, %d, %d, %d",
                profilePtr->conFailurePtr->callEndFailure,
                profilePtr->conFailurePtr->callEndFailureCode,
                profilePtr->conFailurePtr->callConnectionFailureType,
                profilePtr->conFailurePtr->callConnectionFailureCode);
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start profile data session.
 * The start result is given through given handler.
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
void le_mdc_StartSessionAsync
(
    le_mdc_ProfileRef_t profileRef,
        ///< [IN] Start data session for this profile object

    le_mdc_SessionHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    CmdRequest_t cmd;
    memset(&cmd,0,sizeof(CmdRequest_t));

    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return;
    }

    cmd.command = START_SESSION;
    cmd.profileRef = profileRef;
    cmd.contextPtr = contextPtr;
    cmd.handlerFunc = handlerPtr;

    // Sending start data session command
    LE_DEBUG("Send start data session command");
    le_event_Report(CommandEventId, &cmd, sizeof(cmd));
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
    uint64_t rxBytes, txBytes;
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_BAD_PARAMETER;
    }

    // Store data counters
    le_mdc_GetBytesCounters(&rxBytes, &txBytes);

    return pa_mdc_StopSession(profilePtr->profileIndex);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop profile data session.
 * The stop result is given through given handler.
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
void le_mdc_StopSessionAsync
(
    le_mdc_ProfileRef_t profileRef,
        ///< [IN] Stop data session for this profile object

    le_mdc_SessionHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    CmdRequest_t cmd;

    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return;
    }

    cmd.command = STOP_SESSION;
    cmd.profileRef = profileRef;
    cmd.contextPtr = contextPtr;
    cmd.handlerFunc = handlerPtr;

    // Sending stop data session command
    LE_DEBUG("Send stop data session command");
    le_event_Report(CommandEventId, &cmd, sizeof(cmd));
}

//--------------------------------------------------------------------------------------------------
/**
 * Reject MT-PDP profile data session.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if the input parameter is not valid
 *      - LE_UNSUPPORTED if not supported by the target
 *      - LE_FAULT for other failures
 *
 * @note
 *      The process exits, if an invalid profile object is given
 *
 * @warning The MT-PDP context activation feature is not supported on all platforms. Please refer to
 * @ref MT-PDP_context section for full details.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_RejectMtPdpSession
(
    le_mdc_ProfileRef_t profileRef     ///< [IN] Reject MT-PDP data session for this profile object
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_BAD_PARAMETER;
    }

    return pa_mdc_RejectMtPdpSession(profilePtr->profileIndex);
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
    le_mdc_ConState_t*   statePtr          ///< [OUT] Data session state
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_BAD_PARAMETER;
    }
    if (statePtr == NULL)
    {
        LE_KILL_CLIENT("isConnectedPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    le_result_t result;

    result = pa_mdc_GetSessionState(profilePtr->profileIndex, statePtr);

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

    le_event_HandlerRef_t handlerRef =
                                    le_event_AddLayeredHandler("le_NewSessionStateHandler",
                                    profilePtr->sessionStateEvent,
                                    FirstLayerSessionStateChangeHandler,
                                    (le_event_HandlerFunc_t)handler);

    le_event_SetContextPtr(handlerRef, contextPtr);

    LE_DEBUG("handlerRef %p", handlerRef);

     return (le_mdc_SessionStateHandlerRef_t) handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Register a handler for MT-PDP session state changes.
 *
 * @return
 *      - A handler reference, which is only needed for later removal of the handler.
 *
 * @note
 *      Process exits on failure.
 *
 * @warning The MT-PDP context activation feature is not supported on all platforms. Please refer to
 * @ref MT-PDP_context section for full details.
 */
//--------------------------------------------------------------------------------------------------
le_mdc_MtPdpSessionStateHandlerRef_t le_mdc_AddMtPdpSessionStateHandler
(
    le_mdc_SessionStateHandlerFunc_t handler,   ///< [IN] Handler function
    void* contextPtr                            ///< [IN] Context pointer
)
{

    if (handler == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    // Create Handler reference
    le_event_HandlerRef_t handlerRef =
                                le_event_AddLayeredHandler( "le_NewMtPdpSessionStateHandler",
                                MtPdpEventId,
                                FirstLayerSessionStateChangeHandler,
                                (le_event_HandlerFunc_t)handler);

    le_event_SetContextPtr(handlerRef, contextPtr);


    LE_DEBUG("handlerRef %p", handlerRef);

    //  Update MT-PDP change handler counter
    MtPdpStateChangeHandlerCounter++;

    return (le_mdc_MtPdpSessionStateHandlerRef_t) handlerRef;
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
    le_event_RemoveHandler((le_event_HandlerRef_t)sessionStateHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler for MT-PDP session state changes
 *
 * @note
 *      The process exits on failure
 *
 * @warning The MT-PDP context activation feature is not supported on all platforms. Please refer to
 * @ref MT-PDP_context section for full details.
 */
//--------------------------------------------------------------------------------------------------
void le_mdc_RemoveMtPdpSessionStateHandler
(
    le_mdc_MtPdpSessionStateHandlerRef_t sessionStateHandlerRef ///< [IN] The handler reference.
)
{
    LE_DEBUG("Handler counter %d", MtPdpStateChangeHandlerCounter);

    //  Update MT-PDP change handler counter
    MtPdpStateChangeHandlerCounter--;

    le_event_RemoveHandler((le_event_HandlerRef_t)sessionStateHandlerRef);
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
    if (!rxBytes)
    {
        LE_KILL_CLIENT("rxBytes is NULL !");
        return LE_FAULT;
    }
    if (!txBytes)
    {
        LE_KILL_CLIENT("txBytes is NULL !");
        return LE_FAULT;
    }

    pa_mdc_PktStatistics_t data;
    le_result_t result = pa_mdc_GetDataFlowStatistics(&data);
    if (LE_OK != result)
    {
        return result;
    }

    *rxBytes = DataStatistics.receivedBytesCount + data.receivedBytesCount;
    *txBytes = DataStatistics.transmittedBytesCount + data.transmittedBytesCount;
    LE_DEBUG("Received and transmitted bytes: rx=%"PRIu64", tx=%"PRIu64, *rxBytes, *txBytes);

    SetDataCounters(*rxBytes, *txBytes);

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
    LE_DEBUG("Reset received and transmitted bytes");

    if (LE_OK == pa_mdc_ResetDataFlowStatistics())
    {
        DataStatistics.receivedBytesCount = 0;
        DataStatistics.transmittedBytesCount = 0;
        SetDataCounters(DataStatistics.receivedBytesCount, DataStatistics.transmittedBytesCount);
        return LE_OK;
    }

    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stop collecting received/transmitted data flow statistics
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_StopBytesCounter
(
    void
)
{
    LE_DEBUG("Stop counting received and transmitted bytes");

    if (LE_OK == pa_mdc_StopDataFlowStatistics())
    {
        SetDataCounterState(false);
        return LE_OK;
    }

    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Start collecting received/transmitted data flow statistics
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT for all other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_StartBytesCounter
(
    void
)
{
    LE_DEBUG("Start counting received and transmitted bytes");

    if (LE_OK == pa_mdc_StartDataFlowStatistics())
    {
        SetDataCounterState(true);
        return LE_OK;
    }

    return LE_FAULT;
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
    le_result_t result;
    le_mdc_ConState_t state;
    le_mdc_Pdp_t originalPdp;
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_BAD_PARAMETER;
    }

    // Update the le_mdc copy of modemData of the given profile in case its contents got
    // changed outside le_mdc's notice, e.g. via the AT command AT+CGDCONT
    result = pa_mdc_ReadProfile(profilePtr->profileIndex, &profilePtr->modemData);
    if ((result != LE_OK) && (result != LE_NOT_FOUND))
    {
        // LE_OK and LE_NOT_FOUND are the normal results, and any other isn't
        LE_ERROR("Error in reading profile at index %d; error %d", profilePtr->profileIndex,
                 result);
        return result;
    }

    result = le_mdc_GetSessionState(profileRef, &state);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get session state of profile at index %d", profilePtr->profileIndex);
        return LE_FAULT;

    }
    if (state == LE_MDC_CONNECTED)
    {
        LE_ERROR("Failed to set PDP on profile at index %d with a connected session",
                 profilePtr->profileIndex);
        return LE_FAULT;
    }

    // Set the PDP into modemData and write it back into the modem
    originalPdp = profilePtr->modemData.pdp;
    profilePtr->modemData.pdp = pdp;
    result = pa_mdc_WriteProfile(profilePtr->profileIndex, &profilePtr->modemData);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to write PDP data into modem for profile at index %d",
                 profilePtr->profileIndex);
        // Revert back to original setting
        profilePtr->modemData.pdp = originalPdp;
    }
    return result;
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
    le_result_t status;

    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_MDC_PDP_UNKNOWN;
    }

    status = pa_mdc_ReadProfile(profilePtr->profileIndex,&profilePtr->modemData);
    if (status != LE_OK)
    {
        if (status == LE_NOT_FOUND)
        {
            // Fill PDP type with default value
            profilePtr->modemData.pdp = LE_MDC_PDP_IPV4V6;
        }
        else
        {
            LE_ERROR("Could not read profile at index %d", profilePtr->profileIndex);
            return LE_MDC_PDP_UNKNOWN;
        }
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
 * @warning The maximum APN length might be limited by the platform.
 *          Please refer to the platform documentation @ref platformConstraintsMdc.
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
    le_result_t result;
    le_mdc_ConState_t state;
    char originalApn[PA_MDC_APN_MAX_BYTES];
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
        return LE_BAD_PARAMETER;
    }

    size_t apnLen = strlen(apnPtr);
    if ( apnLen > LE_MDC_APN_NAME_MAX_LEN )
    {
        LE_CRIT("apnStr is too long (%zd) > LE_MDC_APN_NAME_MAX_LEN (%d)!",
            apnLen,LE_MDC_APN_NAME_MAX_LEN);
        return LE_BAD_PARAMETER;
    }

    // Update the le_mdc copy of modemData of the given profile in case its contents got
    // changed outside le_mdc's notice, e.g. via the AT command AT+CGDCONT
    result = pa_mdc_ReadProfile(profilePtr->profileIndex, &profilePtr->modemData);
    if ((result != LE_OK) && (result != LE_NOT_FOUND))
    {
        // LE_OK and LE_NOT_FOUND are the normal results, and any other isn't
        LE_ERROR("Error in reading profile at index %d; error %d", profilePtr->profileIndex,
                 result);
        return result;
    }

    result = le_mdc_GetSessionState(profileRef, &state);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get session state of profile at index %d", profilePtr->profileIndex);
        return LE_FAULT;

    }
    if (state == LE_MDC_CONNECTED)
    {
        LE_ERROR("Failed to set APN on profile at index %d with a connected session",
                 profilePtr->profileIndex);
        return LE_FAULT;
    }

    // Set the APN into modemData and write it back into the modem
    LE_ASSERT(le_utf8_Copy(originalApn, profilePtr->modemData.apn, PA_MDC_APN_MAX_BYTES, NULL)
              == LE_OK);
    LE_ASSERT(le_utf8_Copy(profilePtr->modemData.apn, apnPtr, sizeof(profilePtr->modemData.apn),
                           NULL) == LE_OK);
    result = pa_mdc_WriteProfile(profilePtr->profileIndex, &profilePtr->modemData);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to write APN data into modem for profile at index %d",
                 profilePtr->profileIndex);
        // Revert back to original setting
        LE_ASSERT(le_utf8_Copy(profilePtr->modemData.apn, originalApn, PA_MDC_APN_MAX_BYTES, NULL)
                  == LE_OK);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the Access Point Name (APN) for the given profile according to the SIM identification
 * number (ICCID). If no APN is found using the ICCID, fall back on the home network (MCC/MNC)
 * to determine the default APN.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if an input parameter is not valid
 *      - LE_FAULT for all other errors
 *
 * @note
 *      The process exits if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_SetDefaultAPN
(
    le_mdc_ProfileRef_t profileRef ///< [IN] Query this profile object
)
{
    le_result_t error = LE_FAULT;
    char mccString[LE_MRC_MCC_BYTES]           = {0};
    char mncString[LE_MRC_MNC_BYTES]           = {0};
    char iccidString[LE_SIM_ICCID_BYTES]       = {0};
    char defaultApn[LE_MDC_APN_NAME_MAX_BYTES] = {0};

    // Load SIM configuration from Config DB
    le_sim_Id_t simSelected = le_sim_GetSelectedCard();

    // Get ICCID
    error = le_sim_GetICCID(simSelected, iccidString, sizeof(iccidString));
    if (error != LE_OK)
    {
        LE_WARN("Could not retrieve ICCID");
        return LE_FAULT;
    }

    LE_DEBUG("Search for ICCID %s in file %s", iccidString, APN_IIN_FILE);

    // Try to find the APN with the ICCID first
    if (LE_OK != FindApnWithIccidFromFile(APN_IIN_FILE, iccidString,
                                          defaultApn, sizeof(defaultApn)))
    {
        LE_WARN("Could not find ICCID %s in file %s", iccidString, APN_IIN_FILE);

        // Fallback mechanism: try to find the APN with the MCC/MNC

        // Get MCC/MNC
        error = le_sim_GetHomeNetworkMccMnc(simSelected, mccString, sizeof(mccString),
                                            mncString, sizeof(mncString));
        if (error != LE_OK)
        {
            LE_WARN("Could not retrieve MCC/MNC");
            return LE_FAULT;
        }

        LE_DEBUG("Search for MCC/MNC %s/%s in file %s", mccString, mncString, APN_MCCMNC_FILE);

        if (LE_OK != FindApnWithMccMncFromFile(APN_MCCMNC_FILE, mccString, mncString,
                                               defaultApn, sizeof(defaultApn)))
        {
            LE_WARN("Could not find MCC/MNC %s/%s in file %s",
                    mccString, mncString, APN_MCCMNC_FILE);
            return LE_FAULT;
        }
    }

    // Save the APN value into the modem
    return le_mdc_SetAPN(profileRef, defaultApn);
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
    le_result_t status;

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

    status = pa_mdc_ReadProfile(profilePtr->profileIndex,&profilePtr->modemData);
    if (status != LE_OK)
    {
        LE_ERROR("Failed to read profile at index %d; error %d", profilePtr->profileIndex, status);
        return status;
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
 * @note Both PAP and CHAP authentication can be set for 3GPP network: in this case, the device
 *       decides which authentication procedure is performed. For example, the device can have a
 *       policy to select the most secure authentication mechanism.
 *
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
    le_mdc_ConState_t state;
    le_mdc_Auth_t originalType;
    char originalUsername[PA_MDC_USERNAME_MAX_BYTES], originalPassword[PA_MDC_PWD_MAX_BYTES];
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) found!", profileRef);
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

    // Update the le_mdc copy of modemData of the given profile in case its contents got
    // changed outside le_mdc's notice, e.g. via the AT command AT+CGDCONT
    result = pa_mdc_ReadProfile(profilePtr->profileIndex, &profilePtr->modemData);
    if ((result != LE_OK) && (result != LE_NOT_FOUND))
    {
        // LE_OK and LE_NOT_FOUND are the normal results, and any other isn't
        LE_ERROR("Error in reading profile at index %d; error %d", profilePtr->profileIndex,
                 result);
        return result;
    }

    result = le_mdc_GetSessionState(profileRef, &state);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to get session state of profile at index %d", profilePtr->profileIndex);
        return LE_FAULT;

    }
    if (state == LE_MDC_CONNECTED)
    {
        LE_ERROR("Failed to set authentication on profile at index %d with a connected session",
                 profilePtr->profileIndex);
        return LE_FAULT;
    }

    // Set the authentication property into modemData and write it back into the modem
    originalType = profilePtr->modemData.authentication.type;
    profilePtr->modemData.authentication.type = type;
    LE_ASSERT(le_utf8_Copy(originalUsername, profilePtr->modemData.authentication.userName,
                           PA_MDC_USERNAME_MAX_BYTES, NULL) == LE_OK);
    LE_ASSERT(le_utf8_Copy(profilePtr->modemData.authentication.userName, userName,
                           sizeof(profilePtr->modemData.authentication.userName), NULL) == LE_OK);

    LE_ASSERT(le_utf8_Copy(originalPassword, profilePtr->modemData.authentication.password,
                           PA_MDC_PWD_MAX_BYTES, NULL) == LE_OK);
    LE_ASSERT(le_utf8_Copy(profilePtr->modemData.authentication.password, password,
                           sizeof(profilePtr->modemData.authentication.password), NULL) == LE_OK);

    result = pa_mdc_WriteProfile(profilePtr->profileIndex, &profilePtr->modemData);
    if (result != LE_OK)
    {
        LE_ERROR("Failed to write authentication data into modem for profile at index %d",
                 profilePtr->profileIndex);
        // Revert back to original setting
        profilePtr->modemData.authentication.type = originalType;
        LE_ASSERT(le_utf8_Copy(profilePtr->modemData.authentication.userName, originalUsername,
                               PA_MDC_USERNAME_MAX_BYTES, NULL) == LE_OK);
        LE_ASSERT(le_utf8_Copy(profilePtr->modemData.authentication.password, originalPassword,
                               PA_MDC_PWD_MAX_BYTES, NULL) == LE_OK);
    }
    return result;
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

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the disconnection reason.
 *
 * @return The disconnection reason.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 *
 * @deprecated This function is deprecated, le_mdc_GetDisconnectionReasonExt should be used for the
 *             new code.
 */
//--------------------------------------------------------------------------------------------------
le_mdc_DisconnectionReason_t le_mdc_GetDisconnectionReason
(
    le_mdc_ProfileRef_t profileRef
        ///< [IN]
        ///< profile reference
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);

    if (NULL == profilePtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", profileRef);
        return LE_MDC_DISC_UNDEFINED;
    }

    if (NULL == profilePtr->conFailurePtr)
    {
        LE_ERROR("Unspecified disconnection reason");
        return LE_MDC_DISC_UNDEFINED;
    }

    return (profilePtr->conFailurePtr->callEndFailure);
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the platform specific disconnection code.
 *
 * Refer to @ref platformConstraintsSpecificErrorCodes for platform specific
 * disconnection code description.
 *
 * @return The platform specific disconnection code.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 *
 * @deprecated This function is deprecated, le_mdc_GetPlatformSpecificDisconnectionCodeExt should
 *             be used for the new code.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_mdc_GetPlatformSpecificDisconnectionCode
(
    le_mdc_ProfileRef_t profileRef
        ///< [IN]
        ///< profile reference
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);

    if (NULL == profilePtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", profileRef);
        return LE_MDC_DISC_UNDEFINED;
    }

    if (NULL == profilePtr->conFailurePtr)
    {
        LE_ERROR("Unspecified disconnection code");
        return INT32_MAX;
    }

    return (profilePtr->conFailurePtr->callEndFailureCode);
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the platform specific connection failure reason.
 *
 * Refer to @ref platformConstraintsSpecificErrorCodes for platform specific
 * connection failure types and code descriptions.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 *
 * @deprecated This function is deprecated, le_mdc_GetPlatformSpecificFailureConnectionReasonExt
 *             should be used for new code.
 */
//--------------------------------------------------------------------------------------------------
void le_mdc_GetPlatformSpecificFailureConnectionReason
(
    le_mdc_ProfileRef_t profileRef,
        ///< [IN]
        ///< profile reference

    int32_t* failureTypePtr,
        ///< [OUT]
        ///< platform specific failure type

    int32_t* failureCodePtr
        ///< [OUT]
        ///< platform specific failure code
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);

    if ((!failureTypePtr) || (!failureCodePtr))
    {
        LE_KILL_CLIENT("failureTypePtr or failureCodePtr is NULL !");
        return;
    }

    *failureTypePtr = LE_MDC_DISC_UNDEFINED;
    *failureCodePtr = INT32_MAX;

    if (NULL == profilePtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", profileRef);
        return;
    }

    if (NULL == profilePtr->conFailurePtr)
    {
        LE_ERROR("Unspecified disconnection code/reason");
        return;
    }

    *failureCodePtr = profilePtr->conFailurePtr->callConnectionFailureCode;
    *failureTypePtr = profilePtr->conFailurePtr->callConnectionFailureType;
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the disconnection reason.
 *
 * @return The disconnection reason.
 *
 * @warning The return value le_mdc_DisconnectionReason_t might be limited by the platform.
 *          Please refer to the platform documentation @ref platformConstraintsMdc.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 * @note For IPv4v6 mode, pdpType specifies which session's disconnect reason to get(IPv4 or IPv6
 *       session). For IPv4 and IPv6 mode, pdpType is ignored because there is only one session for
 *       IPv4 and IPv6 mode.
 */
//--------------------------------------------------------------------------------------------------
le_mdc_DisconnectionReason_t le_mdc_GetDisconnectionReasonExt
(
    le_mdc_ProfileRef_t profileRef,
        ///< [IN] profile reference
    le_mdc_Pdp_t pdpType
        ///< [IN] pdp type of session
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);

    if (NULL == profilePtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", profileRef);
        return LE_MDC_DISC_UNDEFINED;
    }

    if (LE_MDC_PDP_UNKNOWN == profilePtr->modemData.pdp)
    {
        LE_ERROR("Session PDP type unknown!");
        return LE_MDC_DISC_UNDEFINED;
    }

    if ((LE_MDC_PDP_IPV4V6 == profilePtr->modemData.pdp) &&
        (LE_MDC_PDP_IPV4 != pdpType) && (LE_MDC_PDP_IPV6 != pdpType))
    {
        LE_ERROR("Unsupported PDP type provided: %d", pdpType);
        return LE_MDC_DISC_UNDEFINED;
    }

    pa_mdc_GetConnectionFailureReasonExt(profilePtr->profileIndex, pdpType,
                                         &(profilePtr->conFailurePtr));

    if (NULL == profilePtr->conFailurePtr)
    {
        LE_ERROR("Unable to get the connection failure reason. Null conFailurePtr");
        return LE_MDC_DISC_UNDEFINED;
    }

    return (profilePtr->conFailurePtr->callEndFailure);
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the platform specific disconnection code.
 *
 * Refer to @ref platformConstraintsSpecificErrorCodes for platform specific
 * disconnection code description.
 *
 * @return The platform specific disconnection code.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 * @note For IPv4v6 mode, pdpType specifies which session's disconnect reason to get(IPv4 or IPv6
 *       session). For IPv4 and IPv6 mode, pdpType is ignored because there is only one session for
 *       IPv4 and IPv6 mode.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_mdc_GetPlatformSpecificDisconnectionCodeExt
(
    le_mdc_ProfileRef_t profileRef,
        ///< [IN] profile reference
    le_mdc_Pdp_t pdpType
        ///< [IN] pdp type of session
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);

    if (NULL == profilePtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", profileRef);
        return INT32_MAX;
    }

    if (LE_MDC_PDP_UNKNOWN == profilePtr->modemData.pdp)
    {
        LE_ERROR("Session PDP type unknown!");
        return INT32_MAX;
    }

    if ((LE_MDC_PDP_IPV4V6 == profilePtr->modemData.pdp) &&
        (LE_MDC_PDP_IPV4 != pdpType) && (LE_MDC_PDP_IPV6 != pdpType))
    {
        LE_ERROR("Unsupported PDP type provided: %d", pdpType);
        return INT32_MAX;
    }

    pa_mdc_GetConnectionFailureReasonExt(profilePtr->profileIndex, pdpType,
                                         &(profilePtr->conFailurePtr));

    if (NULL == profilePtr->conFailurePtr)
    {
        LE_ERROR("Unable to get the connection failure reason. Null conFailurePtr");
        return INT32_MAX;
    }

    return (profilePtr->conFailurePtr->callEndFailureCode);
}

//--------------------------------------------------------------------------------------------------
/**
 * Called to get the platform specific connection failure reason.
 *
 * Refer to @ref platformConstraintsSpecificErrorCodes for platform specific connection failure
 * types and code descriptions.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 * @note For IPv4v6 mode, pdpType specifies which session's disconnect reason to get(IPv4 or IPv6
 *       session). For IPv4 and IPv6 mode, pdpType is ignored because there is only one session for
 *       IPv4 and IPv6 mode.
 */
//--------------------------------------------------------------------------------------------------
void le_mdc_GetPlatformSpecificFailureConnectionReasonExt
(
    le_mdc_ProfileRef_t profileRef,
        ///< [IN] profile reference
    le_mdc_Pdp_t pdpType,
        ///< [IN] pdp type of session
    int32_t* failureTypePtr,
        ///< [OUT] platform specific failure type
    int32_t* failureCodePtr
        ///< [OUT] platform specific failure code
)
{
    if ((!failureTypePtr) || (!failureCodePtr))
    {
        LE_KILL_CLIENT("failureTypePtr or failureCodePtr is NULL !");
        return;
    }

    *failureTypePtr = LE_MDC_DISC_UNDEFINED;
    *failureCodePtr = INT32_MAX;

    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);
    if (NULL == profilePtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", profileRef);
        return;
    }

    if (LE_MDC_PDP_UNKNOWN == profilePtr->modemData.pdp)
    {
        LE_ERROR("Session PDP type unknown!");
        return;
    }

    if ((LE_MDC_PDP_IPV4V6 == profilePtr->modemData.pdp) &&
        (LE_MDC_PDP_IPV4 != pdpType) && (LE_MDC_PDP_IPV6 != pdpType))
    {
        LE_ERROR("Unsupported PDP type provided: %d", pdpType);
        return;
    }

    pa_mdc_GetConnectionFailureReasonExt(profilePtr->profileIndex, pdpType,
                                         &(profilePtr->conFailurePtr));

    if (NULL == profilePtr->conFailurePtr)
    {
        LE_ERROR("Unable to get the connection failure reason. Null conFailurePtr");
        return;
    }

    *failureCodePtr = profilePtr->conFailurePtr->callConnectionFailureCode;
    *failureTypePtr = profilePtr->conFailurePtr->callConnectionFailureType;
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
le_result_t le_mdc_MapProfileOnNetworkInterface
(
    le_mdc_ProfileRef_t profileRef,        ///< [IN] Profile reference
    const char*      interfaceNamePtr      ///< [IN] Network interface name
)
{
    le_mdc_Profile_t* profilePtr = le_ref_Lookup(DataProfileRefMap, profileRef);

    if (profilePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", profileRef);
        return LE_FAULT;
    }

    return pa_mdc_MapProfileOnNetworkInterface(profilePtr->profileIndex, interfaceNamePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the list of all profiles
 *
 * @return
 *      - LE_OK upon success; otherwise, another le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetProfileList
(
    le_mdc_ProfileInfo_t *profileList, ///< [OUT] list of available profiles
    size_t *listSize                   ///< [INOUT] max list size
)
{
    le_result_t ret;

    LE_INFO("%s: profile list size given %d", __FUNCTION__, (int)*listSize);
    ret = pa_mdc_GetProfileList(profileList, listSize);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get profile list");
    }
    return ret;
}
