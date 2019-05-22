/**
 * This module implements some stubs for the Data Connection service unit tests.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_dcs.h"
#include "dcs.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------
#define DCS_DUMMY_CHANNEL_REF 0xffff0000
#define DCS_DUMMY_CHANNEL_REQ_REF 0xffff2222
#define DCS_DUMMY_DNS_SERVER_ADDR_IPV4 "11.22.33.44"
#define DCS_DUMMY_DNS_SERVER_ADDR_IPV6 "fe80::c84:bfff:fea6:afea"
#define DCS_DUMMY_CLIENT_SESSION_REF ((le_msg_SessionRef_t)0x1001)

//--------------------------------------------------------------------------------------------------
/**
 * Iterator reference for simulated config tree
 */
//--------------------------------------------------------------------------------------------------
static le_cfg_IteratorRef_t IteratorRefSimu;

//--------------------------------------------------------------------------------------------------
/**
 * Simulated routing config tree
 */
//--------------------------------------------------------------------------------------------------
static bool UseDefaultRoute = true;

//--------------------------------------------------------------------------------------------------
/**
 * Simulated wifi config tree
 */
//--------------------------------------------------------------------------------------------------
#define WIFI_SECPROTOCOL_INIT   0xFF
static char WifiSsid[LE_WIFIDEFS_MAX_SSID_BYTES] = {0};
static int  WifiSecProtocol = WIFI_SECPROTOCOL_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Simulated time config tree
 */
//--------------------------------------------------------------------------------------------------
#define MAX_TIME_SERVER_LENGTH  200
static uint8_t TimeProtocol = 0;
static char TimeServer[MAX_TIME_SERVER_LENGTH] = {0};

//--------------------------------------------------------------------------------------------------
/**
 * Simulated RAT
 */
//--------------------------------------------------------------------------------------------------
static le_mrc_Rat_t RatInUse = LE_MRC_RAT_UNKNOWN;

//--------------------------------------------------------------------------------------------------
/**
 * Simulated MDC profile
 */
//--------------------------------------------------------------------------------------------------
static int32_t MdcProfileIndex = LE_MDC_DEFAULT_PROFILE;

//--------------------------------------------------------------------------------------------------
/**
 * Event for sending Cellular Network Registration state to applications
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t CellNetStateEvent = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for Network Reject notification.
 * It is used in le_mrc_AddNetRegRejectHandler
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NetRegRejectId = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Data Control Profile structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t profileIndex;                     ///< Index of the profile on the modem
    le_mdc_ProfileRef_t profileRef;            ///< Profile Safe Reference
    le_mdc_ConState_t connectionStatus;        ///< Data session connection status
}
le_mdc_Profile_t;

//--------------------------------------------------------------------------------------------------
/**
 * Dummy MDC profile
 */
//--------------------------------------------------------------------------------------------------
static le_mdc_Profile_t MdcProfile =
{
    1,
    (le_mdc_ProfileRef_t) 0x10000001,
    LE_MDC_DISCONNECTED
};

//--------------------------------------------------------------------------------------------------
/**
 * Event for sending MDC session state to applications
 */
//--------------------------------------------------------------------------------------------------
static le_dcs_channelDb_t DcsChannelDb;
static le_dcs_channelDbEventHdlr_t DcsChannelEvtHdlr;


//--------------------------------------------------------------------------------------------------
/**
 * Event for sending Wifi state to applications
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewWifiEventId = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Event ID for WiFi Event indication notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t WifiEventId = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for WifiClient state events reporting.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t WifiEventPool;

//--------------------------------------------------------------------------------------------------
// Unit test specific functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Simulate a new Cellular Network state
 */
//--------------------------------------------------------------------------------------------------
void le_cellnetTest_SimulateState
(
    le_cellnet_State_t state
)
{
    // Check if event is created before using it
    if (CellNetStateEvent)
    {
        // Notify all the registered client handlers
        le_event_Report(CellNetStateEvent, &state, sizeof(state));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate a new Wifi event
 */
//--------------------------------------------------------------------------------------------------
void le_wifiClientTest_SimulateEvent
(
    le_wifiClient_Event_t event
)
{
    // Check if event is created before using it
    if (NewWifiEventId)
    {
        // Notify all the registered client handlers
        le_event_Report(NewWifiEventId, (void *)&event, sizeof(le_wifiClient_Event_t));
    }

    if (WifiEventId)
    {
        le_wifiClient_EventInd_t* wifiEventPtr = le_mem_ForceAlloc(WifiEventPool);
        wifiEventPtr->event = event;
        wifiEventPtr->disconnectionCause = LE_WIFICLIENT_UNKNOWN_CAUSE;
        wifiEventPtr->apBssid[0] = '\0';

        if (LE_WIFICLIENT_EVENT_CONNECTED == event)
        {
            memset(wifiEventPtr->ifName, '\0', sizeof(wifiEventPtr->ifName));
            snprintf(wifiEventPtr->ifName, sizeof(wifiEventPtr->ifName), WIFI_INTERFACE_NAME);
        }
        else
        {
            wifiEventPtr->ifName[0] = '\0';
        }

        le_event_ReportWithRefCounting(WifiEventId, wifiEventPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set a simulated string value for a specific node.
 */
//--------------------------------------------------------------------------------------------------
void le_cfgTest_SetStringNodeValue
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path,
        ///< or a path relative from the iterator's current
        ///< position.

    const char* value
        ///< [IN]
        ///< Buffer to write the value into.
)
{
    IteratorRefSimu = iteratorRef;

    if (0 == strncmp(path, CFG_NODE_SSID, strlen(CFG_NODE_SSID)))
    {
        memset(WifiSsid, 0, sizeof(WifiSsid));
        strncpy(WifiSsid, value, sizeof(WifiSsid));
    }
    else if (0 == strncmp(path, CFG_NODE_SERVER, strlen(CFG_NODE_SERVER)))
    {
        memset(TimeServer, 0, sizeof(TimeServer));
        strncpy(TimeServer, value, sizeof(TimeServer));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Dummy function to replace system calls
 */
//--------------------------------------------------------------------------------------------------
int MySystem
(
    const char *command
)
{
    return 0;
}


//--------------------------------------------------------------------------------------------------
// Data Connection service stubbing
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_data_GetClientSessionRef
(
    void
)
{
    return DCS_DUMMY_CLIENT_SESSION_REF;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_data_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.  (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t MyAddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
)
{
    return NULL;
}


//--------------------------------------------------------------------------------------------------
// Cellular Network service stubbing
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Connection State Handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerCellNetStateHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_cellnet_State_t* eventDataPtr = reportPtr;
    le_cellnet_StateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(*eventDataPtr,
                      le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler ...
 */
//--------------------------------------------------------------------------------------------------
le_cellnet_StateEventHandlerRef_t le_cellnet_AddStateEventHandler
(
    le_cellnet_StateHandlerFunc_t handlerPtr,
    void* contextPtr
)
{
    // Create an event Id for new cellular network state notification if not already done
    if (!CellNetStateEvent)
    {
        CellNetStateEvent = le_event_CreateId("CellNet State", sizeof(le_cellnet_State_t));
    }

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                    "CellNetState",
                                                    CellNetStateEvent,
                                                    FirstLayerCellNetStateHandler,
                                                    handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_cellnet_StateEventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Request a cellular network
 *
 * @return Reference to the cellular network
 */
//--------------------------------------------------------------------------------------------------
le_cellnet_RequestObjRef_t le_cellnet_Request
(
    void
)
{
    // Service requested, simulate a registered event
    le_cellnetTest_SimulateState(LE_CELLNET_REG_HOME);

    return NULL;
}


//--------------------------------------------------------------------------------------------------
// Wifi Client service stubbing
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *
 * Try to connect the current client thread to the service providing this API. Return with an error
 * if the service is not available.
 *
 * For each thread that wants to use this API, either ConnectService or TryConnectService must be
 * called before any other functions in this API.  Normally, ConnectService is automatically called
 * for the main thread, but not for any other thread. For details, see @ref apiFilesC_client.
 *
 * This function is created automatically.
 *
 * @return
 *  - LE_OK if the client connected successfully to the service.
 *  - LE_UNAVAILABLE if the server is not currently offering the service to which the client is bound.
 *  - LE_NOT_PERMITTED if the client interface is not bound to any service (doesn't have a binding).
 *  - LE_COMM_ERROR if the Service Directory cannot be reached.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_wifiClient_TryConnectService
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
// Config Tree service stubbing
//--------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------
/**
 *  Create a read transaction and open a new iterator for traversing the configuration tree.
 *
 *  @note: This action creates a read lock on the given tree.  Which will start a read-timeout.
 *         Once the read timeout expires, then all active read iterators on that tree will be
 *         expired and the clients killed.
 *
 *  @note: A tree transaction is global to that tree; a long held read transaction will block other
 *         users write transactions from being committed.
 *
 *  @return This will return a newly created iterator reference.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateReadTxn
(
    const char* basePath
        ///< [IN]
        ///< Path to the location to create the new iterator.
)
{
    return (le_cfg_IteratorRef_t)IteratorRefSimu;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Create a write transaction and open a new iterator for both reading and writing.
 *
 *  @note: This action creates a write transaction. If the application holds the iterator for
 *         longer than the configured write transaction timeout, the iterator will cancel the
 *         transaction.  All further reads will fail to return data and all writes will be thrown
 *         away.
 *
 *  @note A tree transaction is global to that tree, so a long held write transaction will block
 *        other user's write transactions from being started.  However other trees in the system
 *        will be unaffected.
 *
 *  \b Responds \b With:
 *
 *  This will respond with a newly created iterator reference.
 */
// -------------------------------------------------------------------------------------------------
le_cfg_IteratorRef_t le_cfg_CreateWriteTxn
(
    const char *    basePath
        ///< [IN]
        ///< Path to the location to create the new iterator.
)
{
    return (le_cfg_IteratorRef_t)IteratorRefSimu;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close the write iterator and commit the write transaction. This updates the config tree
 * with all of the writes that occured using the iterator.
 *
 * @note This operation will also delete the iterator object.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_CommitTxn
(
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN]
        ///< Iterator object to commit.
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close and free the given iterator object. If the iterator is a write iterator, the transaction
 * will be canceled. If the iterator is a read iterator, the transaction will be closed.
 *
 * @note This operation will also delete the iterator object.
 */
//--------------------------------------------------------------------------------------------------
void le_cfg_CancelTxn
(
    le_cfg_IteratorRef_t iteratorRef
        ///< [IN]
        ///< Iterator object to close.
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check to see if a given node in the config tree exists.
 *
 * @return True if the specified node exists in the tree. False if not.
 */
//--------------------------------------------------------------------------------------------------
bool le_cfg_NodeExists
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.
)
{
    bool exists = false;

    if (0 == strncmp(path, CFG_NODE_SSID, strlen(CFG_NODE_SSID)))
    {
        exists = strncmp("", WifiSsid, strlen(WifiSsid));
    }
    else if (0 == strncmp(path, CFG_NODE_SECPROTOCOL, strlen(CFG_NODE_SECPROTOCOL)))
    {
        exists = (WifiSecProtocol != WIFI_SECPROTOCOL_INIT);
    }
    else if (0 == strncmp(path, CFG_NODE_PROFILEINDEX, strlen(CFG_NODE_PROFILEINDEX)))
    {
        exists = true;
    }

    return exists;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a string value from the config tree. If the value isn't a string, or if the node is
 * empty or doesn't exist, the default value will be returned.
 *
 * Valid for both read and write transactions.
 *
 * If the path is empty, the iterator's current node will be read.
 *
 * @return - LE_OK       - Read was completed successfully.
 *         - LE_OVERFLOW - Supplied string buffer was not large enough to hold the value.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_cfg_GetString
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path,
        ///< or a path relative from the iterator's current
        ///< position.

    char* value,
        ///< [OUT]
        ///< Buffer to write the value into.

    size_t valueNumElements,
        ///< [IN]

    const char* defaultValue
        ///< [IN]
        ///< Default value to use if the original can't be
        ///<   read.
)
{
    le_result_t result = LE_FAULT;

    if (0 == strncmp(path, CFG_NODE_SSID, strlen(CFG_NODE_SSID)))
    {
        result = le_utf8_Copy(value, WifiSsid, valueNumElements, NULL);
    }
    else if (0 == strncmp(path, CFG_NODE_SERVER, strlen(CFG_NODE_SERVER)))
    {
        result = le_utf8_Copy(value, TimeServer, valueNumElements, NULL);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a signed integer value from the config tree.
 *
 * If the underlying value is not an integer, the default value will be returned instead. The
 * default value is also returned if the node does not exist or if it's empty.
 *
 * If the value is a floating point value, then it will be rounded and returned as an integer.
 *
 * Valid for both read and write transactions.
 *
 * If the path is empty, the iterator's current node will be read.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_cfg_GetInt
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path, or
        ///< a path relative from the iterator's current position.

    int32_t defaultValue
        ///< [IN]
        ///< Default value to use if the original can't be
        ///<   read.
)
{
    int32_t value = defaultValue;

    if (0 == strncmp(path, CFG_NODE_SECPROTOCOL, strlen(CFG_NODE_SECPROTOCOL)))
    {
        value = WifiSecProtocol;
    }
    else if (0 == strncmp(path, CFG_NODE_PROFILEINDEX, strlen(CFG_NODE_PROFILEINDEX)))
    {
        value = MdcProfileIndex;
    }
    else if (0 == strncmp(path, CFG_NODE_PROTOCOL, strlen(CFG_NODE_PROTOCOL)))
    {
        value = TimeProtocol;
    }

    return value;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Write a signed integer value to the configuration tree. Only valid during a write transaction.
 *
 *  If the path is empty, the iterator's current node will be set.
 */
// -------------------------------------------------------------------------------------------------
void le_cfg_SetInt
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Full or relative path to the value to write.

    int32_t value
        ///< [IN]
        ///< Value to write.
)
{
    if (0 == strncmp(path, CFG_NODE_SECPROTOCOL, strlen(CFG_NODE_SECPROTOCOL)))
    {
        WifiSecProtocol = value;
    }
    else if (0 == strncmp(path, CFG_NODE_PROFILEINDEX, strlen(CFG_NODE_PROFILEINDEX)))
    {
        MdcProfileIndex = value;
    }
    else if (0 == strncmp(path, CFG_NODE_PROTOCOL, strlen(CFG_NODE_PROTOCOL)))
    {
        TimeProtocol = value;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Read a value from the tree as a boolean. If the node is empty or doesn't exist, the default
 * value is returned. Default value is also returned if the node is a different type than
 * expected.
 *
 * Valid for both read and write transactions.
 *
 * If the path is empty, the iterator's current node will be read.
 */
//--------------------------------------------------------------------------------------------------
bool le_cfg_GetBool
(
    le_cfg_IteratorRef_t iteratorRef,   ///< [IN] Iterator to use as a basis for the transaction.
    const char* path,                   ///< [IN] Path to the target node. Can be an absolute path,
                                        ///<      or a path relative from the iterator's current
                                        ///<      position
    bool defaultValue                   ///< [IN] Default value to use if the original can't be
                                        ///<      read.
)
{
    bool value;

    if (0 == strncmp(path, CFG_NODE_DEFAULTROUTE, strlen(CFG_NODE_DEFAULTROUTE)))
    {
        value = UseDefaultRoute;
    }
    else
    {
        value = defaultValue;
        LE_ERROR("Unsupported path '%s', using default value %d", path, defaultValue);
    }

    return value;
}


//--------------------------------------------------------------------------------------------------
// Modem Data Control service stubbing
//--------------------------------------------------------------------------------------------------

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
    MdcProfile.profileIndex = index;
    return MdcProfile.profileRef;
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
    return MdcProfile.profileIndex;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the current Radio Access Technology in use.
 *
 * @return LE_FAULT         Function failed to get the Radio Access Technology.
 * @return LE_BAD_PARAMETER A bad parameter was passed.
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetRadioAccessTechInUse
(
    le_mrc_Rat_t*   ratPtr  ///< [OUT] The Radio Access Technology.
)
{
    if (ratPtr == NULL)
    {
        return LE_BAD_PARAMETER;
    }

    *ratPtr = RatInUse;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Simulate RAT
 */
//--------------------------------------------------------------------------------------------------
void le_mrcTest_SetRatInUse
(
    le_mrc_Rat_t rat    ///< [IN] RAT in use
)
{
    if (LE_MRC_RAT_CDMA >= rat)
    {
        RatInUse = rat;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer network registration reject indication handler.
 * It will replace FirstLayerNetworkRejectHandler in near future.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerNetRegRejectHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    if (NULL == reportPtr)
    {
        LE_ERROR("reportPtr is NULL");
        return;
    }

    if (NULL == secondLayerHandlerFunc)
    {
        LE_ERROR("secondLayerHandlerFunc is NULL");
        return;
    }

    le_mrc_NetRegRejectInd_t* networkRejectIndPtr =
                                      (le_mrc_NetRegRejectInd_t*)reportPtr;

    le_mrc_NetRegRejectHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(networkRejectIndPtr, le_event_GetContextPtr());

    // The reportPtr is a reference counted object, so need to release it
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register a handler for network registration reject indication.
 * It will replace le_mrc_AddNetworkRejectHandler in near future.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_mrc_NetRegRejectHandlerRef_t le_mrc_AddNetRegRejectHandler
(
    le_mrc_NetRegRejectHandlerFunc_t handlerFuncPtr,      ///< [IN] The handler function
    void*                             contextPtr           ///< [IN] The handler's context
)
{
    if (!NetRegRejectId)
    {
        NetRegRejectId = le_event_CreateIdWithRefCounting("NetRegReject");
    }

    le_event_HandlerRef_t handlerRef;

    if (NULL == handlerFuncPtr)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("NetRegRejectHandler",
                                             NetRegRejectId,
                                             FirstLayerNetRegRejectHandler,
                                             (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mrc_NetRegRejectHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_mrc_NetRegReject'.
 * It will replace EVENT 'le_mrc_NetworkReject' in near future.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemoveNetRegRejectHandler
(
    le_mrc_NetRegRejectHandlerRef_t handlerRef ///< [IN] The handler reference
)
{
    le_event_RemoveHandler((le_event_HandlerRef_t)handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stub
 *
 * @return
 *  - LE_FAULT  Function failed.
 *  - LE_OK     Function succeeded.
 *
 * @note <b>multi-app safe</b>
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mrc_GetPacketSwitchedState
(
    le_mrc_NetRegState_t* statePtr ///< [OUT] The current Packet switched state.
)
{
    if (statePtr == NULL)
    {
        LE_KILL_CLIENT("Parameters pointer are NULL!!");
        return LE_FAULT;
    }

    *statePtr = LE_MRC_REG_HOME;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stub
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_dcs_GetTimeWithTimeProtocol
(
    const char* serverStrPtr,       ///< [IN]  Time server
    pa_dcs_TimeStruct_t* timePtr    ///< [OUT] Time structure
)
{
    timePtr->msec = 0;
    timePtr->sec  = 30;
    timePtr->min  = 34;
    timePtr->hour = 15;
    timePtr->day  = 9;
    timePtr->mon  = 10;
    timePtr->year = 2017;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stub
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_dcs_GetTimeWithNetworkTimeProtocol
(
    const char* serverStrPtr,       ///< [IN]  Time server
    pa_dcs_TimeStruct_t* timePtr    ///< [OUT] Time structure
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start watchdogs 0..N-1.  Typically this is used in COMPONENT_INIT to start all watchdogs needed
 * by the process.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_Init
(
    uint32_t wdogCount
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Begin monitoring the event loop on the current thread.
 */
//--------------------------------------------------------------------------------------------------
void le_wdogChain_MonitorEventLoop
(
    uint32_t watchdog,          ///< Watchdog to use for monitoring
    le_clk_Time_t watchdogInterval ///< Interval at which to check event loop is functioning
)
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function to get the default profile's index
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_dcsCellular_GetProfileIndex
(
    int32_t mdcIndex
)
{
    uint32_t index;
    index = (MdcProfileIndex < 0) ? mdcIndex : MdcProfileIndex;
    return index;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for setting the default profile index
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsCellular_SetProfileIndex
(
    int32_t mdcIndex
)
{
    MdcProfileIndex = mdcIndex;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function to retrieve the name of the channel at the given profile index
 */
//--------------------------------------------------------------------------------------------------
void le_dcsCellular_GetNameFromIndex
(
    uint32_t index,
    char channelName[LE_DCS_CHANNEL_NAME_MAX_LEN]
)
{
    if (index == 0)
    {
        channelName[0] = '\0';
        return;
    }
    snprintf(channelName, LE_DCS_CHANNEL_NAME_MAX_LEN-1, "%d", index);
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for creating a channelDb for the given channel in the argument
 */
//--------------------------------------------------------------------------------------------------
le_dcs_ChannelRef_t le_dcs_CreateChannelDb
(
    le_dcs_Technology_t tech,
    const char *channelName
)
{
    memset(&DcsChannelDb, 0, sizeof(DcsChannelDb));
    DcsChannelDb.technology = tech;
    strncpy(DcsChannelDb.channelName, channelName, LE_DCS_CHANNEL_NAME_MAX_LEN);
    DcsChannelDb.channelRef = (le_dcs_ChannelRef_t)DCS_DUMMY_CHANNEL_REF;
    return DcsChannelDb.channelRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for querying le_dcs for the channel reference of a channel given by its name
 */
//--------------------------------------------------------------------------------------------------
le_dcs_ChannelRef_t le_dcs_GetReference
(
    const char *name,
    le_dcs_Technology_t technology
)
{
    return (!DcsChannelDb.channelRef ?
            le_dcs_CreateChannelDb(technology, name) : DcsChannelDb.channelRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Utility for converting an le_dcs event into a string for printing
 *
 * @return
 *     - The string name of the given le_dcs event
 */
//--------------------------------------------------------------------------------------------------
const char *le_dcs_ConvertEventToString
(
    le_dcs_Event_t event
)
{
    switch (event)
    {
        case LE_DCS_EVENT_UP:
            return "Up";
        case LE_DCS_EVENT_DOWN:
            return "Down";
        case LE_DCS_EVENT_TEMP_DOWN:
            return "Temporary Down";
        default:
            return "Unknown";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * The first-layer channel event handler used by the stub function le_dcs_AddEventHandler
 */
//--------------------------------------------------------------------------------------------------
static void DcsFirstLayerEventHandler (void *reportPtr, void *secondLayerHandlerFunc)
{
    le_dcs_channelDbEventReport_t *evtReport = reportPtr;
    le_dcs_channelDb_t *channelDb;
    le_dcs_EventHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    channelDb = evtReport->channelDb;
    if (!channelDb)
    {
        return;
    }
    clientHandlerFunc(channelDb->channelRef, evtReport->event, 0, le_event_GetContextPtr());
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for adding an le_dcs channel event handler
 */
//--------------------------------------------------------------------------------------------------
le_dcs_EventHandlerRef_t le_dcs_AddEventHandler
(
    le_dcs_ChannelRef_t channelRef,
    le_dcs_EventHandlerFunc_t channelHandlerPtr,
    void *contextPtr
)
{
    le_event_HandlerRef_t handlerRef;

    if (DcsChannelEvtHdlr.channelEventId != 0)
    {
        return DcsChannelEvtHdlr.hdlrRef;
    }
    if (!channelHandlerPtr)
    {
        LE_ERROR("Event handler can't be null");
        return NULL;
    }
    memset(&DcsChannelEvtHdlr, 0, sizeof(le_dcs_channelDbEventHdlr_t));
    DcsChannelEvtHdlr.channelEventId = le_event_CreateId(DcsChannelDb.channelName,
                                                         sizeof(le_dcs_channelDbEventReport_t));
    DcsChannelEvtHdlr.channelEventHdlr = channelHandlerPtr;
    handlerRef = le_event_AddLayeredHandler("le_dcs_EventHandler",
                                            DcsChannelEvtHdlr.channelEventId,
                                            DcsFirstLayerEventHandler,
                                            (le_event_HandlerFunc_t)channelHandlerPtr);
    DcsChannelEvtHdlr.hdlrRef = (le_dcs_EventHandlerRef_t)handlerRef;
    le_event_SetContextPtr(handlerRef, contextPtr);
    return DcsChannelEvtHdlr.hdlrRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for removing the channel event handler given via a reference object in the argument
 */
//--------------------------------------------------------------------------------------------------
void le_dcs_RemoveEventHandler
(
    le_dcs_EventHandlerRef_t channelHandlerRef
)
{
    if (channelHandlerRef != DcsChannelEvtHdlr.hdlrRef)
    {
        return;
    }
    le_event_RemoveHandler((le_event_HandlerRef_t)DcsChannelEvtHdlr.hdlrRef);
    memset(&DcsChannelEvtHdlr, 0, sizeof(le_dcs_channelDbEventHdlr_t));
}


//--------------------------------------------------------------------------------------------------
/**
 * Simulate a DCS connection event
 */
//--------------------------------------------------------------------------------------------------
void le_dcsTest_SimulateConnEvent
(
    le_dcs_Event_t evt
)
{
    le_dcs_channelDbEventReport_t evtReport;

    if (DcsChannelEvtHdlr.channelEventId == 0)
    {
        return;
    }

    LE_INFO("Simulating event %d", evt);
    evtReport.channelDb = &DcsChannelDb;
    evtReport.event = evt;
    le_event_Report(DcsChannelEvtHdlr.channelEventId, &evtReport, sizeof(evtReport));
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for requesting cellular to start the given data channel in the 1st argument
 * after its technology type is retrieved
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsTech_Start
(
    const char *channelName,
    le_dcs_Technology_t tech
)
{
    le_dcsTest_SimulateConnEvent(LE_DCS_EVENT_UP);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for stopping the given data channel in the argument after its technology type
 * is retrieved
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsTech_Stop
(
    const char *channelName,
    le_dcs_Technology_t tech
)
{
    le_dcsTest_SimulateConnEvent(LE_DCS_EVENT_DOWN);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for requesting to start a data channel
 */
//--------------------------------------------------------------------------------------------------
le_dcs_ReqObjRef_t le_dcs_Start
(
    le_dcs_ChannelRef_t channelRef
)
{
    le_dcsTest_SimulateConnEvent(LE_DCS_EVENT_UP);
    return (le_dcs_ReqObjRef_t)DCS_DUMMY_CHANNEL_REQ_REF;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stub function for requesting to stop a previously started data channel
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcs_Stop
(
    le_dcs_ReqObjRef_t reqRef
)
{
    le_dcsTest_SimulateConnEvent(LE_DCS_EVENT_DOWN);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Utility for converting a technology type enum into its name
 *
 * @return
 *     - The string name of the given technology in the function's return value
 */
//--------------------------------------------------------------------------------------------------
const char *le_dcs_ConvertTechEnumToName
(
    le_dcs_Technology_t tech
)
{
    char *DcsTechnologyNames[LE_DCS_TECH_MAX] = {"", "wifi", "cellular"};
    if (tech < LE_DCS_TECH_MAX)
    {
        return DcsTechnologyNames[tech];
    }
    return "";
}

//--------------------------------------------------------------------------------------------------
/**
 * Stub function for getting the network interface's name of a given channel
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsTech_GetNetInterface
(
    le_dcs_Technology_t tech,
    le_dcs_ChannelRef_t channelRef,
    char *intfName,
    int nameSize
)
{
    char *intf;

    switch (tech)
    {
        case LE_DCS_TECH_CELLULAR:
            intf = "rmnet0";
            break;
        case LE_DCS_TECH_WIFI:
            intf = "wlan0";
            break;
        default:
            LE_ERROR("Channel's technology type %s not supported",
                     le_dcs_ConvertTechEnumToName(tech));
            return LE_UNSUPPORTED;
    }

    if (nameSize <= strlen(intf))
    {
        return LE_OVERFLOW;
    }

    strncpy(intfName, intf, strlen(intf));
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for querying the DNS addresses of the given connection
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsTech_GetDNSAddresses
(
    le_dcs_Technology_t tech,
    void *techRef,
    char *v4DnsAddrsPtr,
    size_t v4DnsAddrSize,
    char *v6DnsAddrsPtr,
    size_t v6DnsAddrSize
)
{
    char *dns1Addr, *dns2Addr;
    le_result_t result = LE_FAULT;

    dns1Addr = v4DnsAddrsPtr;
    dns2Addr = v4DnsAddrsPtr + v4DnsAddrSize;
    if (strlen(DCS_DUMMY_DNS_SERVER_ADDR_IPV4) >= v4DnsAddrSize)
    {
        dns1Addr[0] = '\0';
    }
    else
    {
        le_utf8_Copy(dns1Addr, DCS_DUMMY_DNS_SERVER_ADDR_IPV4, v4DnsAddrSize, NULL);
        result = LE_OK;
    }
    dns2Addr[0] = '\0';

    dns1Addr = v6DnsAddrsPtr;
    dns2Addr = v6DnsAddrsPtr + v6DnsAddrSize;
    if (strlen(DCS_DUMMY_DNS_SERVER_ADDR_IPV6) >= v6DnsAddrSize)
    {
        dns1Addr[0] = '\0';
    }
    else
    {
        le_utf8_Copy(dns1Addr, DCS_DUMMY_DNS_SERVER_ADDR_IPV6, v6DnsAddrSize, NULL);
        result = LE_OK;
    }
    dns2Addr[0] = '\0';
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for backing up default GW config in the system
 */
//--------------------------------------------------------------------------------------------------
void le_net_BackupDefaultGW
(
    void
)
{
    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for restoring the default GW config in the system
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_RestoreDefaultGW
(
    void
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for setting the default GW config in the system
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_SetDefaultGW
(
    le_dcs_ChannelRef_t channelRef
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for setting the system's DNS server addresses
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_SetDNS
(
    le_dcs_ChannelRef_t channelRef
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for restoring the system's DNS server addresses to the original
 */
//--------------------------------------------------------------------------------------------------
void le_net_RestoreDNS
(
    void
)
{
    return;
}


//--------------------------------------------------------------------------------------------------
/**
 * Stub function for changing route
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_net_ChangeRoute
(
    le_dcs_ChannelRef_t channelRef,
    const char *destAddr,
    const char *destMask,
    bool isAdd
)
{
    return LE_OK;
}
