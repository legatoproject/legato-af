/**
 * This module implements some stubs for the Data Connection service unit tests.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_dcs.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

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
static char WifiPassphrase[LE_WIFIDEFS_MAX_PASSPHRASE_BYTES] = {0};
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
 * Event ID for New Packet Switched change notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t PSChangeId = NULL;

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
static le_event_Id_t MdcSessionStateEvent = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Event for sending Wifi state to applications
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t NewWifiEventId = NULL;


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
 * Simulate a new MDC session state
 */
//--------------------------------------------------------------------------------------------------
void le_mdcTest_SimulateState
(
    le_mdc_ConState_t state
)
{
    // Check if event is created before using it
    if (MdcSessionStateEvent)
    {
        le_mdc_Profile_t* profilePtr = &MdcProfile;
        MdcProfile.connectionStatus = state;

        // Notify all the registered client handlers
        le_event_Report(MdcSessionStateEvent, &profilePtr, sizeof(profilePtr));
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
    else if (0 == strncmp(path, CFG_NODE_PASSPHRASE, strlen(CFG_NODE_PASSPHRASE)))
    {
        memset(WifiPassphrase, 0, sizeof(WifiPassphrase));
        strncpy(WifiPassphrase, value, sizeof(WifiPassphrase));
    }
    else if (0 == strncmp(path, CFG_NODE_SERVER, strlen(CFG_NODE_SERVER)))
    {
        memset(TimeServer, 0, sizeof(TimeServer));
        strncpy(TimeServer, value, sizeof(TimeServer));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set a simulated int value for a specific node.
 */
//--------------------------------------------------------------------------------------------------
void le_cfgTest_SetIntNodeValue
(
    le_cfg_IteratorRef_t iteratorRef,
        ///< [IN]
        ///< Iterator to use as a basis for the transaction.

    const char* path,
        ///< [IN]
        ///< Path to the target node. Can be an absolute path,
        ///< or a path relative from the iterator's current
        ///< position.

    int32_t value
        ///< [IN]
        ///< Value to set in node.
)
{
    IteratorRefSimu = iteratorRef;

    if (0 == strncmp(path, CFG_NODE_SECPROTOCOL, strlen(CFG_NODE_SECPROTOCOL)))
    {
        WifiSecProtocol = value;
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
    return (le_msg_SessionRef_t) 0x1001;
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
/**
 * This function starts the WiFi device.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_BUSY          If the WiFi device is already started.
 * @return LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_wifiClient_Start
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Connect to the WiFi Access Point.
 * All authentication must be set prior to calling this function.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER Parameter is invalid.
 * @return LE_OK            Function succeeded.
 *
 * @note For PSK credentials see le_wifiClient_SetPassphrase() or le_wifiClient_SetPreSharedKey() .
 * @note For WPA-Enterprise credentials see le_wifiClient_SetUserCredentials()
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_wifiClient_Connect
(
    le_wifiClient_AccessPointRef_t apRef
        ///< [IN]
        ///< WiFi access point reference.
)
{
    // Wait to simulate a real scan
    sleep(2);

    // Simulation of a scan complete event for UT purpose
    le_wifiClientTest_SimulateEvent(LE_WIFICLIENT_EVENT_SCAN_DONE);

    // Wait to simulate a real connection
    sleep(2);

    // Connection requested, simulate a connected event
    le_wifiClientTest_SimulateEvent(LE_WIFICLIENT_EVENT_CONNECTED);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disconnect from the WiFi Access Point.
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_wifiClient_Disconnect
(
)
{
    // Disconnection requested, simulate a disconnected event
    le_wifiClientTest_SimulateEvent(LE_WIFICLIENT_EVENT_DISCONNECTED);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer WiFi Client Event Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerWifiClientEventHandler
(
    void *reportPtr,
    void *secondLayerHandlerFunc
)
{
    le_wifiClient_NewEventHandlerFunc_t  clientHandlerFunc = secondLayerHandlerFunc;
    le_wifiClient_Event_t               *wifiEventPtr      = (le_wifiClient_Event_t *)reportPtr;

    if (NULL != wifiEventPtr)
    {
        LE_DEBUG("Event: %d", *wifiEventPtr);
        clientHandlerFunc(*wifiEventPtr, le_event_GetContextPtr());
    }
    else
    {
        LE_ERROR("Event is NULL");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_wifiClient_NewEvent'
 *
 * This event provide information on Wifi Client event changes.
 */
//--------------------------------------------------------------------------------------------------
le_wifiClient_NewEventHandlerRef_t le_wifiClient_AddNewEventHandler
(
    le_wifiClient_NewEventHandlerFunc_t handlerFuncPtr,
        ///< [IN]
        ///< Event handling function

    void *contextPtr
        ///< [IN]
        ///< Associated event context
)
{
    le_event_HandlerRef_t handlerRef;

    if (handlerFuncPtr == NULL)
    {
        LE_KILL_CLIENT("handlerFuncPtr is NULL !");
        return NULL;
    }

    // Create an event Id for new Wifi state notification if not already done
    if (!NewWifiEventId)
    {
        NewWifiEventId = le_event_CreateId("WifiClientEvent", sizeof(le_wifiClient_Event_t));
    }

    handlerRef = le_event_AddLayeredHandler("NewWiFiClientMsgHandler",
                                            NewWifiEventId,
                                            FirstLayerWifiClientEventHandler,
                                            (le_event_HandlerFunc_t)handlerFuncPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_wifiClient_NewEventHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * If an Access Point is not announcing it's presence, it will not show up in the scan.
 * But if the SSID is known, a connection can be tried using this create function.
 * First create the Access Point, then le_wifiClient_Connect() to connect to it.
 *
 * @return AccessPoint reference to the current
 */
//--------------------------------------------------------------------------------------------------
le_wifiClient_AccessPointRef_t le_wifiClient_Create
(
    const uint8_t *ssidPtr,
        ///< [IN]
        ///< The SSID as a octet array.

    size_t ssidNumElements
        ///< [IN]
        ///< Length of the SSID in octets.
)
{
    return (le_wifiClient_AccessPointRef_t) 0x90000009;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the security mode for connection
 *
 * @return LE_FAULT         Function failed.
 * @return LE_BAD_PARAMETER Parameter is invalid.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_wifiClient_SetSecurityProtocol
(
    le_wifiClient_AccessPointRef_t apRef,
        ///< [IN]
        ///< WiFi Access Point reference.

    le_wifiClient_SecurityProtocol_t securityProtocol
        ///< [IN]
        ///< Security Mode
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the passphrase used to generate the PSK.
 *
 * @note This is one way to authenticate against the access point. The other one is provided by the
 * le_wifiClient_SetPreSharedKey() function. Both ways are exclusive and are effective only when used
 * with WPA-personal authentication.
 *
 * @return LE_BAD_PARAMETER Parameter is invalid.
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_wifiClient_SetPassphrase
(
    le_wifiClient_AccessPointRef_t apRef,
        ///< [IN]
        ///< WiFi Access Point reference.

    const char *passPhrasePtr
        ///< [IN]
        ///< pass-phrase for PSK
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
    else if (0 == strncmp(path, CFG_NODE_PASSPHRASE, strlen(CFG_NODE_PASSPHRASE)))
    {
        exists = strncmp("", WifiPassphrase, strlen(WifiPassphrase));
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
    else if (0 == strncmp(path, CFG_NODE_PASSPHRASE, strlen(CFG_NODE_PASSPHRASE)))
    {
        result = le_utf8_Copy(value, WifiPassphrase, valueNumElements, NULL);
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
    return LE_OK;
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
    return true;
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
    return false;
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
    snprintf(gatewayAddrStr, gatewayAddrStrSize, "192.168.0.254");
    return LE_OK;
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
    snprintf(gatewayAddrStr, gatewayAddrStrSize, "192.168.0.254");
    return LE_OK;
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
    dns1AddrStr[0] = 0;
    dns2AddrStr[0] = 0;
    return LE_OK;
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
    dns1AddrStr[0] = 0;
    dns2AddrStr[0] = 0;
    return LE_OK;
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
    snprintf(interfaceNameStr, interfaceNameStrSize, MDC_INTERFACE_NAME);
    return LE_OK;
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
    // Update connection status
    MdcProfile.connectionStatus = LE_MDC_CONNECTED;

    // Start requested, simulate a connected event
    le_mdcTest_SimulateState(LE_MDC_CONNECTED);

    return LE_OK;
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
    // Update connection status
    MdcProfile.connectionStatus = LE_MDC_DISCONNECTED;

    return LE_OK;
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
    *statePtr = MdcProfile.connectionStatus;
    return LE_OK;
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
    snprintf(apnPtr, apnSize, "internet.sierrawireless.com");
    return LE_OK;
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
    if (handler == NULL)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    // Create an event Id for new MDC session state notification if not already done
    if (!MdcSessionStateEvent)
    {
        MdcSessionStateEvent = le_event_CreateId("MDC state", sizeof(le_mdc_Profile_t*));
    }

    le_event_HandlerRef_t handlerRef =
                                    le_event_AddLayeredHandler("le_NewSessionStateHandler",
                                    MdcSessionStateEvent,
                                    FirstLayerSessionStateChangeHandler,
                                    (le_event_HandlerFunc_t)handler);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mdc_SessionStateHandlerRef_t) handlerRef;
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
 * The first-layer Packet Switched Change Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerPSChangeHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    le_mrc_NetRegState_t*         serviceStatePtr = (le_mrc_NetRegState_t*) reportPtr;
    le_mrc_PacketSwitchedChangeHandlerFunc_t clientHandlerFunc =
                    (le_mrc_PacketSwitchedChangeHandlerFunc_t) secondLayerHandlerFunc;

    clientHandlerFunc(*serviceStatePtr, le_event_GetContextPtr());

    // The reportPtr is a reference counted object, so need to release it
    le_mem_Release(reportPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_mrc_PacketSwitchedChange'
 *
 * This event provides information on Packet Switched service changes.
 *
 * @note <b>multi-app safe</b>
 */
//--------------------------------------------------------------------------------------------------
le_mrc_PacketSwitchedChangeHandlerRef_t le_mrc_AddPacketSwitchedChangeHandler
(
    le_mrc_PacketSwitchedChangeHandlerFunc_t packetHandlerPtr,  ///< [IN] The handler function.
    void* contextPtr                                            ///< [IN] The handler's context.
)
{
    if (!PSChangeId)
    {
        PSChangeId = le_event_CreateId("Packet switch state", sizeof(le_mrc_NetRegState_t));
    }

    le_event_HandlerRef_t        handlerRef;

    if (NULL == packetHandlerPtr)
    {
        LE_KILL_CLIENT("Handler function is NULL !");
        return NULL;
    }

    handlerRef = le_event_AddLayeredHandler("PacketSwitchedChangeHandler",
                                            PSChangeId,
                                            FirstLayerPSChangeHandler,
                                            (le_event_HandlerFunc_t)packetHandlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_mrc_PacketSwitchedChangeHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_mrc_PacketSwitchedChange'
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_RemovePacketSwitchedChangeHandler
(
    le_mrc_PacketSwitchedChangeHandlerRef_t handlerRef ///< [IN]
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
 *
 * @return
 *      LE_FAULT        Function failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_dcs_SetDnsNameServers
(
    const char* dns1Ptr,    ///< [IN] Pointer on first DNS address
    const char* dns2Ptr     ///< [IN] Pointer on second DNS address
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stub
 *
 * @return
 *      - LE_OK     Function successful
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_dcs_AskForIpAddress
(
    const char* interfaceStrPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stub
 *
 * return
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_dcs_ChangeRoute
(
    pa_dcs_RouteAction_t  routeAction,
    const char*           ipDestAddrStr,
    const char*           interfaceStr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stub
 *
 * return
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_dcs_SetDefaultGateway
(
    const char* interfacePtr,   ///< [IN] Pointer on the interface name
    const char* gatewayPtr,     ///< [IN] Pointer on the gateway name
    bool        isIpv6          ///< [IN] IPv6 or not
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Save the default route
 */
//--------------------------------------------------------------------------------------------------
void pa_dcs_SaveDefaultGateway
(
    pa_dcs_InterfaceDataBackup_t* interfaceDataBackupPtr
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stub
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_dcs_RestoreInitialDnsNameServers
(
    pa_dcs_InterfaceDataBackup_t* pInterfaceDataBackup
)
{
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
