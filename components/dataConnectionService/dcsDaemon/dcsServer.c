//--------------------------------------------------------------------------------------------------
/**
 *  Data Connection Server
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 * The Data Connection Service (DCS) supports two technologies in this version:
 * - the 'Mobile' technology, with a data connection based on the Modem Data Control service (MDC)
 * - the 'Wi-Fi' technology, with a data connection based on the Wifi Client.
 *
 * The technologies to use are saved in an ordered list. The default data connection is started
 * with the first technology to use. If this one is or becomes unavailable, the second one is used.
 * If the last technology of the list is also unavailable, the first one is used again.
 *
 * The connection establishment upon reception of a REQUEST command depends on the technology
 * to use:
 * - With the 'Mobile' technology, the DCS first sends a REQUEST command to the Cellular Network
 * Service in order to ensure that there is a valid SIM and the modem is registered on the network.
 * The data session is actually started when the Cellular Network Service State is 'ROAMING' or
 * 'HOME'.
 * - With the 'Wi-Fi' technology, the DCS first starts the wifi client and reads the Access Point
 * configuration in the config tree. The data session is then started by connecting to the
 * Access Point.
 *
 *
 * TODO:
 *  - 'Mobile' connection assumes that DHCP client will always succeed; this is not always the case
 */
//--------------------------------------------------------------------------------------------------

#include <arpa/inet.h>
#include <stdlib.h>

#include "legato.h"
#include "interfaces.h"
#include "le_cfg_interface.h"
#include "mdmCfgEntries.h"
#include "le_print.h"
#include "pa_mdc.h"
#include "pa_dcs.h"
#include "dcsServer.h"
#include "dcs.h"
#include "dcsCellular.h"
#include "watchdogChain.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The config tree path and node definitions.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_PATH_ROUTING            "routing"
#define CFG_NODE_DEFAULTROUTE       "useDefaultRoute"
#define CFG_PATH_WIFI               "wifi"
#define CFG_NODE_SSID               "SSID"
#define CFG_NODE_SECPROTOCOL        "secProtocol"
#define CFG_NODE_PASSPHRASE         "passphrase"
#define CFG_PATH_CELLULAR           "cellular"
#define CFG_NODE_PROFILEINDEX       "profileIndex"
#define CFG_PATH_TIME               "time"
#define CFG_NODE_PROTOCOL           "protocol"
#define CFG_NODE_SERVER             "server"

//--------------------------------------------------------------------------------------------------
/**
 * The technology string max length
 */
//--------------------------------------------------------------------------------------------------
#define DCS_TECH_LEN      16
#define DCS_TECH_BYTES    (DCS_TECH_LEN+1)


//--------------------------------------------------------------------------------------------------
/**
 * Number of technologies
 */
//--------------------------------------------------------------------------------------------------
#define DCS_TECH_NUMBER     LE_DATA_MAX

//--------------------------------------------------------------------------------------------------
/**
 * Wifi interface name
 * TODO: Should be retrieved from Wi-Fi client. To modify when API is available.
 */
//--------------------------------------------------------------------------------------------------
#define WIFI_INTF "wlan0"

//--------------------------------------------------------------------------------------------------
/**
 * Maximal number of retries to stop the data session
 */
//--------------------------------------------------------------------------------------------------
#define MAX_STOP_SESSION_RETRIES    5

//--------------------------------------------------------------------------------------------------
/**
 * Maximal length of a system command
 */
//--------------------------------------------------------------------------------------------------
#define MAX_SYSTEM_CMD_LENGTH       200

//--------------------------------------------------------------------------------------------------
/**
 * Size of the reference maps
 */
//--------------------------------------------------------------------------------------------------
#define REFERENCE_MAP_SIZE          5

//--------------------------------------------------------------------------------------------------
/**
 * Maximal length of a time server address
 */
//--------------------------------------------------------------------------------------------------
#define MAX_TIME_SERVER_LENGTH      200

//--------------------------------------------------------------------------------------------------
/**
 * Default time server used for Time Protocol
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_TIMEPROTOCOL_SERVER     "time.nist.gov"

//--------------------------------------------------------------------------------------------------
/**
 * Default time server used for Network Time Protocol
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_NTP_SERVER              "pool.ntp.org"

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 8

//--------------------------------------------------------------------------------------------------
/**
 * Retry Tech Timer's backoff durations:
 * Its initial value is 1 sec, and max 6 hrs, i.e. (60 * 60 * 6) secs.
 * After each failure, the next backoff time is doubled until it's capped by the max.
 */
//--------------------------------------------------------------------------------------------------
#define RETRY_TECH_BACKOFF_INIT 1                // init backoff: 1 sec
#define RETRY_TECH_BACKOFF_MAX (60 * 60 * 6)     // max backoff: 6 hrs

//--------------------------------------------------------------------------------------------------
// Data structures
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Data associated with the ConnStateEvent
 *
 * interfaceName is only valid if isConnected is true
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool isConnected;
    char interfaceName[LE_DATA_INTERFACE_NAME_MAX_BYTES];
}
ConnStateData_t;


//--------------------------------------------------------------------------------------------------
/**
 * Data associated with a technology record in the preference list
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_data_Technology_t tech;  ///< Technology
    uint32_t             rank;  ///< Technology rank
    le_dls_Link_t        link;  ///< Link in preference list
}
TechRecord_t;

//--------------------------------------------------------------------------------------------------
// Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Declaration of functions
 */
//--------------------------------------------------------------------------------------------------
static void ConnectionStatusHandler(le_data_Technology_t technology, bool connected);

//--------------------------------------------------------------------------------------------------
/**
 * Timer reference
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t StopDcsTimer = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Set DNS Configuration Timer reference
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t SetDNSConfigTimer = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Retry Tech Timer reference RetryTechTimer and the current backoff duration
 * RetryTechBackoffCurrent
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t RetryTechTimer = NULL;
static uint16_t RetryTechBackoffCurrent;

//--------------------------------------------------------------------------------------------------
/**
 * Timer to delay connection request to the modem.
 * This is a workaround until the bug in QTI netmgr is addressed.
 */
//--------------------------------------------------------------------------------------------------
#ifdef LEGATO_DATA_CONNECTION_DELAY
static le_timer_Ref_t DelayRequestTimer = NULL;
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Event for sending command to Process command handler
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t CommandEvent;

//--------------------------------------------------------------------------------------------------
/**
 * Event for sending connection to applications
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t ConnStateEvent;

//--------------------------------------------------------------------------------------------------
/**
 * Used profile when Mobile technology is selected
 */
//--------------------------------------------------------------------------------------------------
static le_mdc_ProfileRef_t MobileProfileRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Store the mobile session state handler reference
 */
//--------------------------------------------------------------------------------------------------
static le_mdc_SessionStateHandlerRef_t MobileSessionStateHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Used access point when wifi technology is selected
 */
//--------------------------------------------------------------------------------------------------
static le_wifiClient_AccessPointRef_t AccessPointRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Store the wifi event handler reference
 */
//--------------------------------------------------------------------------------------------------
static le_wifiClient_NewEventHandlerRef_t WifiEventHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * SSID of the Access Point used for the wifi connection
 */
//--------------------------------------------------------------------------------------------------
static char Ssid[LE_WIFIDEFS_MAX_SSID_BYTES] = {0};

//--------------------------------------------------------------------------------------------------
/**
 * Passphrase of the Access Point used for the wifi connection
 */
//--------------------------------------------------------------------------------------------------
static char Passphrase[LE_WIFIDEFS_MAX_PASSPHRASE_BYTES] = {0};

//--------------------------------------------------------------------------------------------------
/**
 * Security protocol of the Access Point used for the wifi connection
 */
//--------------------------------------------------------------------------------------------------
static le_wifiClient_SecurityProtocol_t SecProtocol = LE_WIFICLIENT_SECURITY_WPA2_PSK_PERSONAL;

//--------------------------------------------------------------------------------------------------
/**
 * Is the data session connected
 */
//--------------------------------------------------------------------------------------------------
static bool IsConnected = false;

//--------------------------------------------------------------------------------------------------
/**
 * Index profile use for data connection on cellular
 */
//--------------------------------------------------------------------------------------------------
static int32_t MdcIndexProfile = LE_MDC_DEFAULT_PROFILE;
static uint32_t ConnectedDataProfileIndex = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Count the number of requests
 */
//--------------------------------------------------------------------------------------------------
static uint32_t RequestCount = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Count the number of retries to stop the data session
 */
//--------------------------------------------------------------------------------------------------
static uint32_t StopSessionRetries = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for the request reference
 */
//--------------------------------------------------------------------------------------------------
le_ref_MapRef_t RequestRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Structure used to store data allowing to restore a functioning state upon disconnection
 */
//--------------------------------------------------------------------------------------------------
static pa_dcs_InterfaceDataBackup_t InterfaceDataBackup;

//--------------------------------------------------------------------------------------------------
/**
 * List of used technologies
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t TechList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Pointer on current technology in list
 */
//--------------------------------------------------------------------------------------------------
static le_dls_Link_t* CurrTechPtr = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Default list of technologies to use
 */
//--------------------------------------------------------------------------------------------------
static le_data_Technology_t DefaultTechList[DCS_TECH_NUMBER] = {LE_DATA_WIFI, LE_DATA_CELLULAR};

//--------------------------------------------------------------------------------------------------
/**
 * Pool used to store the list of technologies to use
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t TechListPoolRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Technologies availability
 */
//--------------------------------------------------------------------------------------------------
static bool TechAvailability[DCS_TECH_NUMBER];

//--------------------------------------------------------------------------------------------------
/**
 * Currently used technology
 */
//--------------------------------------------------------------------------------------------------
static le_data_Technology_t CurrentTech = LE_DATA_MAX;

//--------------------------------------------------------------------------------------------------
/**
 * Default route activation status, read at start-up in config tree.
 * - true:  default route is set by DCS
 * - false: default route is not set by DCS
 */
//--------------------------------------------------------------------------------------------------
static bool DefaultRouteStatus = true;

//--------------------------------------------------------------------------------------------------
/**
 * Local flag to indicate whether the active connection established over a cellular profile was
 * in the first place established via le_dcs than le_data
 */
//--------------------------------------------------------------------------------------------------
static bool IsDataProfileViaLeDcs = false;

//--------------------------------------------------------------------------------------------------
/**
 * Boolean to indicated whether le_data has called to start wifiClient already or not
 */
//--------------------------------------------------------------------------------------------------
static bool WifiHasStarted = false;

//--------------------------------------------------------------------------------------------------
/**
 * This function checks if le_data has already started wifiClient.  If not, call wifiClient to
 * start it.
 *
 * @return
 *     - LE_OK upon success in starting wifiClient anew
 *     - LE_DUPLICATE if DCS has it started already
 *     - LE_BUSY if wifiClient has already been started by its other client
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WifiClientStart
(
    void
)
{
    le_result_t ret;

    if (WifiHasStarted)
    {
        LE_DEBUG("DCS already got wifiClient started");
        return LE_DUPLICATE;
    }

    LE_INFO("Starting Wifi client");
    ret = le_wifiClient_Start();
    if ((LE_OK == ret) || (LE_BUSY == ret))
    {
        LE_DEBUG("DCS got wifiClient started; return code %d", ret);
        WifiHasStarted = true;
    }
    return ret;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function seeks to stop wifiClient if not yet by le_data.
 */
//--------------------------------------------------------------------------------------------------
static void WifiClientStop
(
    void
)
{
    le_result_t ret;

    if (!WifiHasStarted)
    {
        LE_DEBUG("DCS hasn't started wifiClient and thus won't seek to stop it");
        return;
    }

    LE_INFO("Stopping Wifi client");
    ret = le_wifiClient_Stop();
    if ((ret != LE_OK) && (ret != LE_DUPLICATE))
    {
        LE_ERROR("DCS failed to stop wifiClient");
    }
    else
    {
        LE_INFO("DCS succeeded to stop wifiClient");
        WifiHasStarted = false;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * API for the le_dcs component to use to check if any app has used le_data to successfully
 * bring up a data connection
 *
 * @return
 *     - It returns the current request count back in its function argument as well as the
 *       the data profile index, if connected, back in the function's return value
 */
//--------------------------------------------------------------------------------------------------
int32_t le_data_ConnectedDataProfileIndex
(
    uint32_t *requestCount
)
{
    *requestCount = RequestCount;
    return ConnectedDataProfileIndex;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the list of technologies to use with the default values
 */
//--------------------------------------------------------------------------------------------------
static void InitDefaultTechList
(
    void
)
{
    int i;
    // Start to fill the list at rank 1
    int listRank = 1;

    // Fill technologies list with default values
    for (i = 0; i < DCS_TECH_NUMBER; i++)
    {
        if (LE_OK == le_data_SetTechnologyRank(listRank, DefaultTechList[i]))
        {
            // Technology was correctly added to the list, increase the rank
            listRank++;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Checks if the specified technology is already in the list
 *
 * @return
 *      - pointer on technology record if technology is present
 *      - NULL otherwise
 */
//--------------------------------------------------------------------------------------------------
static TechRecord_t* IsTechInList
(
    le_data_Technology_t tech   ///< [IN] Technology index
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&TechList);

    // Go through the list
    while (NULL != linkPtr)
    {
        TechRecord_t* techPtr = CONTAINER_OF(linkPtr, TechRecord_t, link);

        if (techPtr->tech == tech)
        {
            // Technology found
            return techPtr;
        }

        // Get next list element
        linkPtr = le_dls_PeekNext(&TechList, linkPtr);
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Increment the rank of all technologies present in the list beginning with
 * the pointer given in input
 */
//--------------------------------------------------------------------------------------------------
static void IncrementTechRanks
(
    le_dls_Link_t* linkPtr  ///< [IN] Pointer on the first node to update
)
{
    // Go through the list
    while (linkPtr != NULL)
    {
        TechRecord_t* techPtr = CONTAINER_OF(linkPtr, TechRecord_t, link);

        // Increase rank
        techPtr->rank++;

        // Get next list element
        linkPtr = le_dls_PeekNext(&TechList, linkPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert le_data_Technology_t technology into a human-readable string
 *
 * @return
 *      - LE_OK             The string is copied to the output buffer
 *      - LE_OVERFLOW       The output buffer is too small
 *      - LE_BAD_PARAMETER  The technology is unknown
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetTechnologyString
(
    le_data_Technology_t tech,              ///< [IN] Technology
    char*                techStr,           ///< [IN] Buffer for technology string
    size_t               techStrNumElem     ///< [IN] Size of buffer
)
{
    le_result_t result = LE_OK;

    switch (tech)
    {
        case LE_DATA_WIFI:
            if (LE_OK != le_utf8_Copy(techStr, "wifi", techStrNumElem, NULL))
            {
                LE_WARN("Technology buffer is too small");
                result = LE_OVERFLOW;
            }
            break;
        case LE_DATA_CELLULAR:
            if (LE_OK != le_utf8_Copy(techStr, "cellular", techStrNumElem, NULL))
            {
                LE_WARN("Technology buffer is too small");
                result = LE_OVERFLOW;
            }
            break;
        default:
            LE_ERROR("Unknown technology: %s", techStr);
            result = LE_BAD_PARAMETER;
            break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send connection state event to registered applications
 */
//--------------------------------------------------------------------------------------------------
static void SendConnStateEvent
(
    bool isConnected    ///< [IN] Connection state
)
{
    // Initialize the event data
    ConnStateData_t eventData;
    eventData.isConnected = isConnected;

    // Initialize interface name to empty string
    eventData.interfaceName[0] = '\0';

    // Set the interface name
    switch (CurrentTech)
    {
        case LE_DATA_CELLULAR:
            if (isConnected)
            {
                le_result_t result = le_mdc_GetInterfaceName(MobileProfileRef,
                                                             eventData.interfaceName,
                                                             sizeof(eventData.interfaceName));
                if (result == LE_FAULT)
                {
                    LE_ERROR("Cellular technology disconnected. Drop this event.");
                    return;
                }
            }
            break;

        case LE_DATA_WIFI:
            if (isConnected)
            {
                snprintf(eventData.interfaceName, sizeof(eventData.interfaceName), WIFI_INTF);
            }
            break;

        default:
            LE_ERROR("Unknown current technology %d", CurrentTech);
            break;
    }

    LE_DEBUG("Reporting '%s' state[%i]",
        eventData.interfaceName,
        eventData.isConnected);

    // Send the event to interested applications
    le_event_Report(ConnStateEvent, &eventData, sizeof(eventData));
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next technology to use after the one given as an input
 *
 * The only goal of this function is to get a technology to use for the default data connection,
 * the current one being unavailable. If the end of the list is reached, the first technology
 * is used again. The technology finally used (first one or not) is identified later when
 * the new connection status is notified.
 *
 * @return
 *      - The next technology if the end of the list is not reached
 *      - The first technology of the list if the end is reached
 */
//--------------------------------------------------------------------------------------------------
static le_data_Technology_t GetNextTech
(
    le_data_Technology_t technology     ///< [IN] Technology to find in the list
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&TechList);
    le_dls_Link_t* nextTechPtr = NULL;

    // Go through the list
    while (NULL != linkPtr)
    {
        TechRecord_t* techPtr = CONTAINER_OF(linkPtr, TechRecord_t, link);

        if (techPtr->tech == technology)
        {
            // Technology found, get the next one in the list
            nextTechPtr = le_dls_PeekNext(&TechList, linkPtr);
            break;
        }

        // Get next list element
        linkPtr = le_dls_PeekNext(&TechList, linkPtr);
    }

    if (NULL == nextTechPtr)
    {
        // No next technology, get the first one
        return le_data_GetFirstUsedTechnology();
    }
    else
    {
        // Next technology
        return CONTAINER_OF(nextTechPtr, TechRecord_t, link)->tech;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Event callback for Wifi Client changes
 */
//--------------------------------------------------------------------------------------------------
static void WifiClientEventHandler
(
    le_wifiClient_Event_t event,    ///< [IN] Wifi event
    void* contextPtr                ///< [IN] Associated context pointer
)
{
    LE_DEBUG("Wifi event received: %d", event);

    switch (event)
    {
        case LE_WIFICLIENT_EVENT_CONNECTED:
            if (strlen(Ssid) == 0)
            {
                // Wifi connection managed not via le_data but le_dcs or else
                break;
            }

            LE_INFO("Wifi client connected");

            // Request an IP address through DHCP if DCS initiated the connection
            // and update connection status
            if ((LE_DATA_WIFI == CurrentTech) && (RequestCount > 0))
            {
                if (LE_OK == pa_dcs_AskForIpAddress(WIFI_INTF))
                {
                    IsConnected = true;
                }
                else
                {
                    IsConnected = false;
                }
            }
            else
            {
                IsConnected = true;
            }

            // Send notification to registered applications
            SendConnStateEvent(IsConnected);

            // Handle new connection status for this technology
            ConnectionStatusHandler(LE_DATA_WIFI, IsConnected);
            break;

        case LE_WIFICLIENT_EVENT_DISCONNECTED:
            if (strlen(Ssid) == 0)
            {
                // Wifi connection managed not via le_data but le_dcs or else
                break;
            }
            LE_INFO("Wifi client disconnected");

            // Update connection status and send notification to registered applications
            IsConnected = false;
            memset(Ssid, '\0', sizeof(Ssid));
            SendConnStateEvent(IsConnected);

            // Handle new connection status for this technology
            ConnectionStatusHandler(LE_DATA_WIFI, IsConnected);
            WifiClientStop();
            break;

        case LE_WIFICLIENT_EVENT_SCAN_DONE:
            LE_DEBUG("Wifi client: scan done");
            break;

        default:
            LE_ERROR("Unknown wifi client event %d", event);
            break;
    }
}

// -------------------------------------------------------------------------------------------------
/**
 * This function will know if the APN name for profileName is empty
 *
 * @return
 *      True or False
 */
// -------------------------------------------------------------------------------------------------
bool IsApnEmpty
(
    le_mdc_ProfileRef_t profileRef  ///< [IN] Modem data connection profile reference
)
{
    char apnName[LE_CFG_STR_LEN_BYTES] = {0};

    if (LE_OK != le_mdc_GetAPN(profileRef, apnName, sizeof(apnName)))
    {
        LE_WARN("APN was truncated");
        return true;
    }

    return (!strcmp(apnName, ""));
}

//--------------------------------------------------------------------------------------------------
/**
 *  Event callback for data session state changes
 */
//--------------------------------------------------------------------------------------------------
static void DataSessionStateHandler
(
    le_mdc_ProfileRef_t profileRef,         ///< [IN] Mobile data connection profile reference
    le_mdc_ConState_t connectionStatus,     ///< [IN] Mobile data connection status
    void* contextPtr                        ///< [IN] Associated context pointer
)
{
    uint32_t profileIndex = le_mdc_GetProfileIndex(profileRef);

    LE_PRINT_VALUE("%d", profileIndex);
    LE_PRINT_VALUE("%i", connectionStatus);

    // Check if the session state change is for the used cellular profile
    if (profileRef != MobileProfileRef)
    {
        return;
    }

    #ifdef LEGATO_DATA_CONNECTION_DELAY
    // Restart timer to delay connection request to modem
    le_timer_Restart(DelayRequestTimer);
    #endif

    // Update connection status and send notification to registered applications
    IsConnected = (connectionStatus == LE_MDC_CONNECTED) ? true : false;
    if (IsConnected)
    {
        ConnectedDataProfileIndex = profileIndex;
    }
    else
    {
        ConnectedDataProfileIndex = 0;
    }

    SendConnStateEvent(IsConnected);

    // Handle new connection status for this technology
    ConnectionStatusHandler(LE_DATA_CELLULAR, IsConnected);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Set data profile index to use for cellular technology in the config tree
 */
//--------------------------------------------------------------------------------------------------
static void SetDataProfileIndex
(
    int32_t profileIndex            ///< [IN] Profile index to be stored
)
{
    le_mdc_ProfileRef_t profileRef;

    char configPath[LE_CFG_STR_LEN_BYTES];
    snprintf(configPath, sizeof(configPath), "%s/%s", DCS_CONFIG_TREE_ROOT_DIR, CFG_PATH_CELLULAR);

    profileRef = le_mdc_GetProfile(profileIndex);
    if (NULL == profileRef)
    {
        // If data profile could not be created/retrieved, we keep the index from config tree as is.
        LE_ERROR("Unable to retrieve data profile");
    }
    else
    {
        profileIndex = le_mdc_GetProfileIndex(profileRef);
    }

    le_cfg_IteratorRef_t cfg = le_cfg_CreateWriteTxn(configPath);
    // Set Cid Profile
    le_cfg_SetInt(cfg, CFG_NODE_PROFILEINDEX, profileIndex);
    le_cfg_CommitTxn(cfg);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Get default route activation status from config tree
 */
//--------------------------------------------------------------------------------------------------
static bool GetDefaultRouteStatus
(
    void
)
{
    bool defaultRouteStatus = true;

    char configPath[LE_CFG_STR_LEN_BYTES];
    snprintf(configPath, sizeof(configPath), "%s/%s", DCS_CONFIG_TREE_ROOT_DIR, CFG_PATH_ROUTING);

    le_cfg_IteratorRef_t cfg = le_cfg_CreateReadTxn(configPath);

    // Get default gateway activation status
    if (le_cfg_NodeExists(cfg, CFG_NODE_DEFAULTROUTE))
    {
        defaultRouteStatus = le_cfg_GetBool(cfg, CFG_NODE_DEFAULTROUTE, true);
        LE_DEBUG("Default gateway activation status = %d", defaultRouteStatus);
    }
    le_cfg_CancelTxn(cfg);

    return defaultRouteStatus;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Get the time protocol to use from config tree
 */
//--------------------------------------------------------------------------------------------------
static le_data_TimeProtocol_t GetTimeProtocol
(
    void
)
{
    le_data_TimeProtocol_t protocol = LE_DATA_TP;

    char configPath[LE_CFG_STR_LEN_BYTES];
    snprintf(configPath, sizeof(configPath), "%s/%s", DCS_CONFIG_TREE_ROOT_DIR, CFG_PATH_TIME);

    le_cfg_IteratorRef_t cfg = le_cfg_CreateReadTxn(configPath);
    if (le_cfg_NodeExists(cfg, CFG_NODE_PROTOCOL))
    {
        protocol = le_cfg_GetInt(cfg, CFG_NODE_PROTOCOL, LE_DATA_TP);
    }
    le_cfg_CancelTxn(cfg);

    LE_DEBUG("Use time protocol %d", protocol);
    return protocol;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Get the time server to use from config tree
 */
//--------------------------------------------------------------------------------------------------
static void GetTimeServer
(
    char* serverPtr,                ///< [IN] Time server buffer
    const size_t serverSize,        ///< [IN] Time server buffer size
    const char* defaultServerPtr    ///< [IN] Default time server
)
{
    char configPath[LE_CFG_STR_LEN_BYTES];
    snprintf(configPath, sizeof(configPath), "%s/%s", DCS_CONFIG_TREE_ROOT_DIR, CFG_PATH_TIME);

    le_cfg_IteratorRef_t cfg = le_cfg_CreateReadTxn(configPath);
    if (le_cfg_NodeExists(cfg, CFG_NODE_SERVER))
    {
        if (LE_OK != le_cfg_GetString(cfg,
                                      CFG_NODE_SERVER,
                                      serverPtr,
                                      serverSize,
                                      defaultServerPtr))
        {
            LE_ERROR("Unable to retrieve time server");
            le_utf8_Copy(serverPtr, defaultServerPtr, serverSize, NULL);
        }
    }
    else
    {
        LE_WARN("No server configured, use the default one");
        le_utf8_Copy(serverPtr, defaultServerPtr, serverSize, NULL);
    }
    le_cfg_CancelTxn(cfg);

    LE_DEBUG("Use time server '%s'", serverPtr);
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the cellular profile used indicated by the given index & update its saved profile reference
 * when necessary
 *
 * @return
 *     - LE_OK is returned in the function's return value after it has succeeded to set the profile;
 *       otherwise, other le_result_t failure code
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetCellularProfile
(
    int32_t index
)
{
    le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile(index);

    if (!profileRef)
    {
        LE_ERROR("Profile of index %d is not available", index);
        return LE_FAULT;
    }

    // Updating profileRef
    if (profileRef != MobileProfileRef)
    {
        if (MobileSessionStateHandlerRef)
        {
            le_mdc_RemoveSessionStateHandler(MobileSessionStateHandlerRef);
            MobileSessionStateHandlerRef = NULL;
        }

        MobileProfileRef = profileRef;
        LE_DEBUG("Working with profile %p at index %d", MobileProfileRef, index);
    }

    // Register for data session state changes
    if (!MobileSessionStateHandlerRef)
    {
        MobileSessionStateHandlerRef = le_mdc_AddSessionStateHandler(MobileProfileRef,
                                                                     DataSessionStateHandler,
                                                                     NULL);
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Load the profile of the selected technology
 *
 * @return
 *  - LE_OK             Success
 *  - LE_FAULT          Failure
 *  - LE_NOT_FOUND      Config tree item absent
 *  - LE_OVERFLOW       Config tree item too long
 */
//--------------------------------------------------------------------------------------------------
static le_result_t LoadSelectedTechProfile
(
    le_data_Technology_t technology     ///< [IN] Technology to use for the profile
)
{
    le_result_t result = LE_OK;

    switch (technology)
    {
        case LE_DATA_CELLULAR:
        {
            // Get profile Index to use for cellular data connection service
            MdcIndexProfile = le_dcsCellular_GetProfileIndex(MdcIndexProfile);

            result = SetCellularProfile(MdcIndexProfile);
            if (result != LE_OK)
            {
                return result;
            }

            // MobileProfileRef is now referencing the default profile to use for data connection
            if (IsApnEmpty(MobileProfileRef))
            {
                LE_INFO("Set default APN");
                if (LE_OK != le_mdc_SetDefaultAPN(MobileProfileRef))
                {
                    LE_WARN("Could not set APN from file");
                }
            }
        }
        break;

        case LE_DATA_WIFI:
        {
            // TODO: Only one Access Point can be configured in the config tree for now. DCS should
            // not manage APs and Wifi client should handle the known SSIDs used for the wifi
            // connection. This is a temporary solution until the Wifi client API is improved.

            // Retrieve Access Point data from config tree
            char configPath[LE_CFG_STR_LEN_BYTES];
            snprintf(configPath, sizeof(configPath), "%s/%s",
                     DCS_CONFIG_TREE_ROOT_DIR, CFG_PATH_WIFI);

            le_cfg_IteratorRef_t cfg = le_cfg_CreateReadTxn(configPath);

            // Security protocol
            if (le_cfg_NodeExists(cfg, CFG_NODE_SECPROTOCOL))
            {
                SecProtocol = le_cfg_GetInt(cfg, CFG_NODE_SECPROTOCOL,
                                            LE_WIFICLIENT_SECURITY_WPA2_PSK_PERSONAL);
                LE_DEBUG("AP configuration, Security protocol: %d", SecProtocol);
            }
            else
            {
                LE_WARN("No value set for '%s'!", CFG_NODE_SECPROTOCOL);
                le_cfg_CancelTxn(cfg);
                return LE_NOT_FOUND;
            }

            // Passphrase
            // TODO: passphrase should not be stored without ciphering in the config tree
            if (LE_WIFICLIENT_SECURITY_NONE != SecProtocol)
            {
                if (le_cfg_NodeExists(cfg, CFG_NODE_PASSPHRASE))
                {
                    if (LE_OK != le_cfg_GetString(cfg, CFG_NODE_PASSPHRASE, Passphrase,
                                                  sizeof(Passphrase), "passphrase"))
                    {
                        LE_WARN("String value for '%s' too large", CFG_NODE_PASSPHRASE);
                        le_cfg_CancelTxn(cfg);
                        return LE_OVERFLOW;
                    }
                    LE_DEBUG("AP configuration, Passphrase: '%s'", Passphrase);
                }
                else
                {
                    LE_WARN("No value set for '%s'!", CFG_NODE_PASSPHRASE);
                    le_cfg_CancelTxn(cfg);
                    return LE_NOT_FOUND;
                }
            }

            le_cfg_CancelTxn(cfg);

            // Create the Access Point to connect to
            AccessPointRef = le_wifiClient_Create((const uint8_t *)Ssid, strlen(Ssid));

            if (NULL != AccessPointRef)
            {
                // Configure the Access Point
                LE_ASSERT(LE_OK == le_wifiClient_SetSecurityProtocol(AccessPointRef, SecProtocol));

                if ((LE_WIFICLIENT_SECURITY_WPA_PSK_PERSONAL == SecProtocol) ||
                    (LE_WIFICLIENT_SECURITY_WPA2_PSK_PERSONAL == SecProtocol))
                {
                    LE_ASSERT(LE_OK == le_wifiClient_SetPassphrase(AccessPointRef, Passphrase));
                }

                // Register for Wifi Client state changes if not already done
                if (NULL == WifiEventHandlerRef)
                {
                    WifiEventHandlerRef = le_wifiClient_AddNewEventHandler(WifiClientEventHandler,
                                                                           NULL);
                }
            }
            else
            {
                LE_ERROR("Impossible to create Access Point");
                result = LE_FAULT;
            }

            // Delete sensitive information
            memset(Passphrase, '\0', sizeof(Passphrase));
        }
        break;

        default:
        {
            LE_ERROR("Unknown technology %d", technology);
            result = LE_FAULT;
        }
        break;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Restore the default gateway in the system
 *
 * return
 *      LE_OK           Function succeed
 *      LE_FAULT        Function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RestoreDefaultGateway
(
    void
)
{
    // Restore backed up interface and gateway
    le_result_t result = pa_dcs_SetDefaultGateway(InterfaceDataBackup.defaultInterface,
                                                  InterfaceDataBackup.defaultGateway,
                                                  false);

    // Delete backed up parameters
    memset(InterfaceDataBackup.defaultInterface, '\0',
           sizeof(InterfaceDataBackup.defaultInterface));
    memset(InterfaceDataBackup.defaultGateway, '\0',
           sizeof(InterfaceDataBackup.defaultGateway));

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the default route for a profile
 *
 * return
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetRouteConfiguration
(
    le_mdc_ProfileRef_t profileRef  ///< [IN] Modem data connection profile reference
)
{
    bool isIpv6 = true;
    char ipv6GatewayAddr[LE_MDC_IPV6_ADDR_MAX_BYTES] = {0};
    char ipv4GatewayAddr[LE_MDC_IPV4_ADDR_MAX_BYTES] = {0};
    char interface[LE_MDC_INTERFACE_NAME_MAX_BYTES] = {0};

    if (!(le_mdc_IsIPv6(profileRef) || le_mdc_IsIPv4(profileRef)))
    {
        LE_ERROR("Profile is not using IPv4 nor IPv6");
        return LE_FAULT;
    }

    if (LE_OK != le_mdc_GetInterfaceName(profileRef, interface, sizeof(interface)))
    {
        LE_ERROR("le_mdc_GetInterfaceName failed");
        return LE_FAULT;
    }

    if (le_mdc_IsIPv6(profileRef))
    {
        if (LE_OK != le_mdc_GetIPv6GatewayAddress(profileRef,
                                                  ipv6GatewayAddr,
                                                  sizeof(ipv6GatewayAddr)))
        {
            LE_ERROR("le_mdc_GetIPv6GatewayAddress failed");
            return LE_FAULT;
        }

        // Set the default ipv6 gateway retrieved from modem
        if (LE_OK != pa_dcs_SetDefaultGateway(interface, ipv6GatewayAddr, isIpv6))
        {
            LE_ERROR("SetDefaultGateway for ipv6 gateway failed");
            return LE_FAULT;
        }
    }

    if (le_mdc_IsIPv4(profileRef))
    {

        if (LE_OK != le_mdc_GetIPv4GatewayAddress(profileRef,
                                                  ipv4GatewayAddr,
                                                  sizeof(ipv4GatewayAddr)))
        {
            LE_ERROR("le_mdc_GetIPv4GatewayAddress failed");
            return LE_FAULT;
        }

        // Set the default ipv4 gateway retrieved from modem
        if (LE_OK != pa_dcs_SetDefaultGateway(interface, ipv4GatewayAddr, !(isIpv6)))
        {
            LE_ERROR("SetDefaultGateway for ipv4 gateway failed");
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the DNS configuration for a profile
 *
 * @return
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetDnsConfiguration
(
    le_mdc_ProfileRef_t profileRef,     ///< [IN] Modem data connection profile reference
    bool addDnsRoutes                   ///< [IN] Add routes for DNS
)
{
    char dns1Addr[LE_MDC_IPV6_ADDR_MAX_BYTES] = {0};
    char dns2Addr[LE_MDC_IPV6_ADDR_MAX_BYTES] = {0};

    if (le_mdc_IsIPv4(profileRef))
    {
        if (LE_OK != le_mdc_GetIPv4DNSAddresses(profileRef,
                                                dns1Addr, sizeof(dns1Addr),
                                                dns2Addr, sizeof(dns2Addr)))
        {
            LE_ERROR("IPv4: le_mdc_GetDNSAddresses failed");
            return LE_FAULT;
        }

        if (LE_OK != pa_dcs_SetDnsNameServers(dns1Addr, dns2Addr))
        {
            LE_ERROR("IPv4: Could not write in resolv file");
            return LE_FAULT;
        }

        le_utf8_Copy(InterfaceDataBackup.newDnsIPv4[0], dns1Addr,
                     sizeof(InterfaceDataBackup.newDnsIPv4[0]), NULL);

        le_utf8_Copy(InterfaceDataBackup.newDnsIPv4[1], dns2Addr,
                     sizeof(InterfaceDataBackup.newDnsIPv4[1]), NULL);

        // Add the DNS routes if necessary
        if (addDnsRoutes)
        {
            if (   (LE_OK != le_data_AddRoute(dns1Addr))
                || (LE_OK != le_data_AddRoute(dns2Addr)))
            {
                LE_ERROR("IPv4: Could not add DNS routes");
            }
        }
    }
    else
    {
        InterfaceDataBackup.newDnsIPv4[0][0] = '\0';
        InterfaceDataBackup.newDnsIPv4[1][0] = '\0';
    }

    if (le_mdc_IsIPv6(profileRef))
    {
        if (LE_OK != le_mdc_GetIPv6DNSAddresses(profileRef,
                                                dns1Addr, sizeof(dns1Addr),
                                                dns2Addr, sizeof(dns2Addr)))
        {
            LE_ERROR("IPv6: le_mdc_GetDNSAddresses failed");
            return LE_FAULT;
        }

        if (LE_OK != pa_dcs_SetDnsNameServers(dns1Addr, dns2Addr))
        {
            LE_ERROR("IPv6: Could not write in resolv file");
            return LE_FAULT;
        }

        le_utf8_Copy(InterfaceDataBackup.newDnsIPv6[0], dns1Addr,
                     sizeof(InterfaceDataBackup.newDnsIPv6[0]), NULL);

        le_utf8_Copy(InterfaceDataBackup.newDnsIPv6[1], dns2Addr,
                     sizeof(InterfaceDataBackup.newDnsIPv6[1]), NULL);

        // Add the DNS routes if necessary
        if (addDnsRoutes)
        {
            if (   (LE_OK != le_data_AddRoute(dns1Addr))
                || (LE_OK != le_data_AddRoute(dns2Addr)))
            {
                LE_ERROR("IPv6: Could not add DNS routes");
            }
        }
    }
    else
    {
        InterfaceDataBackup.newDnsIPv6[0][0] = '\0';
        InterfaceDataBackup.newDnsIPv6[1][0] = '\0';
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the default route (if necessary) and DNS
 *
 * @return
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetDefaultRouteAndDns
(
    bool setDefaultRoute    ///< [IN] Should the default route be set?
)
{
    // Check if the default route should be set
    if (setDefaultRoute)
    {
        pa_dcs_GetDefaultGateway(&InterfaceDataBackup);

        if (LE_OK != SetRouteConfiguration(MobileProfileRef))
        {
            LE_ERROR("Failed to set configuration route");
            return LE_FAULT;
        }
    }

    // Set the DNS configuration in all cases and add the DNS routes
    // if the default route is not set
    if (LE_OK != SetDnsConfiguration(MobileProfileRef, !(setDefaultRoute)))
    {
        LE_INFO("Failed to set DNS configuration. Retry later");

        if (!le_timer_IsRunning(SetDNSConfigTimer))
        {
            if (LE_OK != le_timer_Start(SetDNSConfigTimer))
            {
                LE_ERROR("Could not start the SetDNSConfig timer!");
                return LE_FAULT;
            }
        }
    }
    else
    {
        LE_INFO("DNS configuration is set successfully");
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set DNS Configuration Service Timer Handler.
 * When the timer expires, this handler attempts to set DNS configuration.
 */
//--------------------------------------------------------------------------------------------------
static void SetDNSConfigTimerHandler
(
    le_timer_Ref_t timerRef    ///< [IN] Timer used to ensure DNS address is present.
)
{
    le_mdc_ConState_t  sessionState;
    le_result_t result;

    if(0 == RequestCount)
    {
        // Release has been requested in the meantime, We must cancel the Request command process.
        return;
    }

    result = le_mdc_GetSessionState(MobileProfileRef, &sessionState);
    if ((LE_OK == result) && (LE_MDC_CONNECTED == sessionState))
    {
        if (LE_OK != SetDnsConfiguration(MobileProfileRef, !DefaultRouteStatus))
        {
            LE_ERROR("Failed to set DNS configuration.");
        }
        else
        {
            LE_INFO("DNS configuration is set successfully");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the current backoff duration of the RetryTechTimer to its init value. Stop the timer 1st
 * if it's running before adjusting its time interval.
 */
//--------------------------------------------------------------------------------------------------
static void ResetRetryTechBackoff
(
    void
)
{
    if (le_timer_IsRunning(RetryTechTimer)) {
        // make sure the timer is stopped before adjusting its time interval
        le_timer_Stop(RetryTechTimer);
    }

    RetryTechBackoffCurrent = RETRY_TECH_BACKOFF_INIT;

    le_clk_Time_t retryInterval = {RETRY_TECH_BACKOFF_INIT, 0};
    if (LE_OK != le_timer_SetInterval(RetryTechTimer, retryInterval))
    {
        LE_ERROR("Failed to adjust RetryTechTimer timer to %d secs",
                 RetryTechBackoffCurrent);
    }
    else
    {
        LE_DEBUG("RetryTechTimer stopped & backoff reset to %d secs", RetryTechBackoffCurrent);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Try to start the mobile data session
 */
//--------------------------------------------------------------------------------------------------
static void TryStartDataSession
(
    void
)
{
    char channelName[LE_DCS_CHANNEL_NAME_MAX_LEN];
    #ifdef LEGATO_DATA_CONNECTION_DELAY
    if (le_timer_IsRunning(DelayRequestTimer))
    {
        LE_INFO("Delaying data connection attempt.");
        ConnectionStatusHandler(LE_DATA_CELLULAR, false);
        return;
    }
    #endif

    le_dcsCellular_GetNameFromIndex(le_dcsCellular_GetProfileIndex(MdcIndexProfile), channelName);
    if (!le_dcs_CreateChannelDb(LE_DCS_TECH_CELLULAR, channelName))
    {
        // Shouldn't happen; let it go for backward compatibility concerns
        LE_DEBUG("Failed to create a channelDb for le_dcs");
        ConnectionStatusHandler(LE_DATA_CELLULAR, false);
        return;
    }

    le_result_t result = le_mdc_StartSession(MobileProfileRef);

    // Start data session
    if (result == LE_OK)
    {
        LE_DEBUG("Data connection has been started successfully.");

        // First wait a few seconds for the default DHCP client
        sleep(3);

        // Set the default route (if necessary) and the DNS
        if (LE_OK != SetDefaultRouteAndDns(DefaultRouteStatus))
        {
            // Impossible to use this technology, try the next one
            ConnectionStatusHandler(LE_DATA_CELLULAR, false);
        }
        else
        {
            // Wait a few seconds to prevent rapid toggling of data connection
            LE_DEBUG("Waiting a few seconds to prevent rapid toggling of data connection.");
            sleep(5);
        }
    }
    else if (result == LE_DUPLICATE)
    {
        // Connection already established, don't do anything
        LE_DEBUG("Data connection is already established.");

        // To stay safe, reset RetryTechBackoffCurrent here since there won't be a connection state
        // notification since it's already established
        ResetRetryTechBackoff();
        return;
    }
    else
    {
        // Impossible to use this technology, try the next one
        ConnectionStatusHandler(LE_DATA_CELLULAR, false);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Try to start the wifi session
 */
//--------------------------------------------------------------------------------------------------
static void TryStartWifiSession
(
    void
)
{
    // Load Access Point configuration
    le_result_t result = LoadSelectedTechProfile(LE_DATA_WIFI);
    if (LE_OK != result)
    {
        LE_WARN("Impossible to use Wifi profile, result %d (%s)", result, LE_RESULT_TXT(result));

        // Impossible to use this technology, try the next one
        ConnectionStatusHandler(LE_DATA_WIFI, false);
        le_dcs_MarkChannelSharingStatus(Ssid, LE_DCS_TECH_WIFI, false);
        return;
    }

    if (!le_dcs_CreateChannelDb(LE_DCS_TECH_WIFI, Ssid))
    {
        // Shouldn't happen; let it go for backward compatibility concerns
        LE_DEBUG("Failed to create a channelDb for le_dcs");
        ConnectionStatusHandler(LE_DATA_WIFI, false);
        return;
    }

    // Start Wifi client
    result = WifiClientStart();
    if ((LE_OK != result) && (LE_BUSY != result) && (LE_DUPLICATE != result))
    {
        LE_ERROR("Wifi client not started, result %d (%s)", result, LE_RESULT_TXT(result));

        // Impossible to use this technology, try the next one
        ConnectionStatusHandler(LE_DATA_WIFI, false);
        le_dcs_MarkChannelSharingStatus(Ssid, LE_DCS_TECH_WIFI, false);
        return;
    }

    LE_INFO("Wifi client started");

    // Check if the Access Point is created
    if (NULL == AccessPointRef)
    {
        LE_ERROR("No reference to AP");

        // Impossible to use this technology, try the next one
        ConnectionStatusHandler(LE_DATA_WIFI, false);
        le_dcs_MarkChannelSharingStatus(Ssid, LE_DCS_TECH_WIFI, false);
        WifiClientStop();
        return;
    }

    // Connect to the Access Point
    result = le_wifiClient_Connect(AccessPointRef);
    if (LE_OK != result)
    {
        LE_ERROR("Impossible to connect to AP, result %d (%s)", result, LE_RESULT_TXT(result));

        // Impossible to use this technology, try the next one
        ConnectionStatusHandler(LE_DATA_WIFI, false);
        le_dcs_MarkChannelSharingStatus(Ssid, LE_DCS_TECH_WIFI, false);
        WifiClientStop();
        return;
    }

    LE_INFO("Connecting to AP");
}

//--------------------------------------------------------------------------------------------------
/**
 * Try to retrieve the configured SSID from the config tree and set it into the global variable
 * Ssid if found.
 *
 * @return
 *     - true upon success; false otherwise
 */
//--------------------------------------------------------------------------------------------------
static bool RetrieveWifiCfgSsid
(
    char *ssid
)
{
    char configPath[LE_CFG_STR_LEN_BYTES];

    if (!ssid)
    {
        return false;
    }

    snprintf(configPath, sizeof(configPath), "%s/%s", DCS_CONFIG_TREE_ROOT_DIR, CFG_PATH_WIFI);
    le_cfg_IteratorRef_t cfg = le_cfg_CreateReadTxn(configPath);
    if (!le_cfg_NodeExists(cfg, CFG_NODE_SSID))
    {
        LE_WARN("No value set for '%s'!", CFG_NODE_SSID);
        le_cfg_CancelTxn(cfg);
        ConnectionStatusHandler(LE_DATA_WIFI, false);
        return false;
    }

    if (LE_OK != le_cfg_GetString(cfg, CFG_NODE_SSID, ssid, LE_WIFIDEFS_MAX_SSID_LENGTH,
                                  "testSsid"))
    {
        LE_WARN("String value for '%s' too large", CFG_NODE_SSID);
        le_cfg_CancelTxn(cfg);
        ConnectionStatusHandler(LE_DATA_WIFI, false);
        return false;
    }

    // SSID successfully retrieved
    LE_DEBUG("AP configuration, SSID: '%s'", ssid);
    le_cfg_CancelTxn(cfg);
    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Try to start the default data session with a defined technology
 */
//--------------------------------------------------------------------------------------------------
static void TryStartTechSession
(
    le_data_Technology_t technology     ///< [IN] Technology to use for the data session
)
{
    char channelName[LE_DCS_CHANNEL_NAME_MAX_LEN];

    if (LE_DATA_MAX == technology)
    {
        LE_ERROR("Unknown technology used to start the data session!");
    }
    else
    {
        char techStr[DCS_TECH_BYTES] = {0};

        if ((RequestCount == 0) || IsConnected)
        {
            LE_INFO("No need to try starting data session: reqCount %d, connected %d",
                    RequestCount, IsConnected);
            return;
        }

        GetTechnologyString(technology, techStr, sizeof(techStr));
        LE_DEBUG("Technology used for the le_data connection: '%s'", techStr);

        // Store the currently used technology
        CurrentTech = technology;

        switch (technology)
        {
            case LE_DATA_CELLULAR:
            {
                // Check if the device is attached yet
                le_mrc_NetRegState_t serviceState;
                le_result_t res = le_mrc_GetPacketSwitchedState(&serviceState);

                // Check for any cellular connection already established via le_dcs APIs
                MdcIndexProfile = le_dcsCellular_GetProfileIndex(MdcIndexProfile);
                le_dcsCellular_GetNameFromIndex(MdcIndexProfile, channelName);

                if (le_dcs_ChannelIsInUse(channelName, LE_DCS_TECH_CELLULAR)) {
                    // reuse this profile & don't call TryStartDataSession() again below because
                    // its start has been initiated via le_dcs API already
                    LE_DEBUG("Reuse cellular connection with index %d", MdcIndexProfile);

                    // inform le_dcs about le_data's start sharing this cellular connection
                    IsDataProfileViaLeDcs = true;
                    SetCellularProfile(MdcIndexProfile);
                    le_dcs_MarkChannelSharingStatus(channelName, LE_DCS_TECH_CELLULAR, true);
                    ConnectionStatusHandler(LE_DATA_CELLULAR, true);
                    break;
                }

                // Load MobileProfileRef
                le_result_t result = LoadSelectedTechProfile(LE_DATA_CELLULAR);
                if (LE_OK == result)
                {
                    if ((res == LE_OK) &&
                        ((LE_MRC_REG_HOME == serviceState) || (LE_MRC_REG_ROAMING == serviceState)))
                    {
                        LE_INFO("Device is attached, ready to start a data session");

                        // Start a connection
                        le_dcs_MarkChannelSharingStatus(channelName, LE_DCS_TECH_CELLULAR, true);
                        TryStartDataSession();
                    }
                    else
                    {
                        LE_INFO("Cellular not available for starting a data session");
                        ConnectionStatusHandler(LE_DATA_CELLULAR, false);
                    }
                }
                else
                {
                    LE_WARN("Impossible to use cellular profile, error %d (%s)",
                            result, LE_RESULT_TXT(result));

                    // Impossible to use this technology, try the next one
                    ConnectionStatusHandler(LE_DATA_CELLULAR, false);
                }
            }
            break;

            case LE_DATA_WIFI:
            {
                char ssid[LE_WIFIDEFS_MAX_SSID_BYTES];
                if (!RetrieveWifiCfgSsid(ssid) || (strlen(ssid) == 0))
                {
                    LE_ERROR("Failed to retrieve wifi config SSID");
                    return;
                }

                // Check for any wifi connection already established via le_dcs APIs
                le_result_t ret = le_dcsTech_AllowChannelStart(LE_DCS_TECH_WIFI, ssid);
                switch (ret)
                {
                    case LE_DUPLICATE:
                        // This wifi connection is already established via le_dcs
                        LE_INFO("Reuse Wifi connection %s that is already connected", ssid);
                        strncpy(Ssid, ssid, LE_WIFIDEFS_MAX_SSID_LENGTH);
                        IsDataProfileViaLeDcs = true;
                        le_dcs_MarkChannelSharingStatus(Ssid, LE_DCS_TECH_WIFI, true);
                        if (!WifiEventHandlerRef)
                        {
                            WifiEventHandlerRef = le_wifiClient_AddNewEventHandler
                                (WifiClientEventHandler, NULL);
                        }
                        ConnectionStatusHandler(LE_DATA_WIFI, true);
                        SendConnStateEvent(true);
                        return;
                    case LE_NOT_PERMITTED:
                        LE_ERROR("Failed to start wifi connection as it's already up over a "
                                 "different SSID");
                        ConnectionStatusHandler(LE_DATA_WIFI, false);
                        SendConnStateEvent(false);
                        return;
                    case LE_OK:
                        if (!WifiEventHandlerRef)
                        {
                            WifiEventHandlerRef = le_wifiClient_AddNewEventHandler
                                (WifiClientEventHandler, NULL);
                        }
                        break;
                    default:
                        LE_DEBUG("Failure code %d while checking status with le_dcs", ret);
                        break;
                }
                // Try to establish the wifi connection
                strncpy(Ssid, ssid, LE_WIFIDEFS_MAX_SSID_LENGTH);
                le_dcs_MarkChannelSharingStatus(Ssid, LE_DCS_TECH_WIFI, true);
                TryStartWifiSession();
            }
            break;

            default:
                LE_ERROR("Unknown technology %d to start", technology);
                break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Try to stop the mobile data session
 */
//--------------------------------------------------------------------------------------------------
static void TryStopDataSession
(
    le_timer_Ref_t timerRef     ///< [IN] Timer used to ensure the end of the session
)
{
    le_mdc_ConState_t sessionState;
    le_result_t result;
    char channelName[LE_DCS_CHANNEL_NAME_MAX_LEN];

    // Check if the mobile data session is already disconnected
    result = le_mdc_GetSessionState(MobileProfileRef, &sessionState);
    le_dcsCellular_GetNameFromIndex(MdcIndexProfile, channelName);
    LE_DEBUG("Sessions state: %d, result: %d, connection to be stopped: %s", sessionState, result,
             channelName);

    if ((LE_OK == result) && (!sessionState))
    {
        IsConnected = false;

        // Reset number of retries
        StopSessionRetries = 0;

        if (!IsDataProfileViaLeDcs)
        {
            // Restore backed up parameters
            if (DefaultRouteStatus)
            {
                RestoreDefaultGateway();
                pa_dcs_RestoreInitialDnsNameServers(&InterfaceDataBackup);
            }
        }
        else
        {
            IsDataProfileViaLeDcs = false;
        }
        ConnectedDataProfileIndex = 0;
        // inform le_dcs about le_data's stop sharing of this data channel
        le_dcs_MarkChannelSharingStatus(channelName, LE_DCS_TECH_CELLULAR, false);
    }
    else
    {
        // Only tear down the session if no request has been made
        if (0 == RequestCount)
        {
            if (le_dcs_ChannelIsInUse(channelName, LE_DCS_TECH_CELLULAR))
            {
                LE_INFO("Skip trying to stop cellular connection %s still in use via le_dcs",
                        channelName);
                le_dcs_MarkChannelSharingStatus(channelName, LE_DCS_TECH_CELLULAR, false);
                IsConnected = false;
                SendConnStateEvent(false);
                ConnectionStatusHandler(LE_DATA_CELLULAR, false);
                return;
            }
            LE_INFO("Tearing down data session");
        }
        else
        {
            LE_INFO("There is a request for a data session, do not try and stop.");
            return;
        }

        // Try to shutdown the connection anyway
        result = le_mdc_StopSession(MobileProfileRef);
        LE_DEBUG("le_mdc_StopSession return code: %d", result);
        if (LE_OK != result)
        {
            LE_ERROR("Impossible to stop mobile data session");

            // Increase number of retries
            StopSessionRetries++;

            // Start timer
            if (!le_timer_IsRunning(timerRef))
            {
                if (LE_OK != le_timer_SetContextPtr(timerRef, (void*)((intptr_t)LE_DATA_CELLULAR)))
                {
                    LE_ERROR("Could not set context pointer for the StopDcs timer!");
                }
                else
                {
                    if (LE_OK != le_timer_Start(timerRef))
                    {
                        LE_ERROR("Could not start the StopDcs timer!");
                    }
                }
            }
        }
        else
        {
            IsConnected = false;

            // Reset number of retries
            StopSessionRetries = 0;

            if (!IsDataProfileViaLeDcs)
            {
                // Restore backed up parameters
                RestoreDefaultGateway();
                pa_dcs_RestoreInitialDnsNameServers(&InterfaceDataBackup);
            }
            else
            {
                IsDataProfileViaLeDcs = false;
            }

            ConnectedDataProfileIndex = 0;
            // inform le_dcs about le_data's stop sharing of this data channel
            le_dcs_MarkChannelSharingStatus(channelName, LE_DCS_TECH_CELLULAR, false);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Try to stop the wifi session
 */
//--------------------------------------------------------------------------------------------------
static void TryStopWifiSession
(
    le_timer_Ref_t timerRef     ///< [IN] Timer used to ensure the end of the session
)
{
    if (RequestCount > 0)
    {
        return;
    }
    if ((strlen(Ssid) > 0) && le_dcs_ChannelIsInUse(Ssid, LE_DCS_TECH_WIFI))
    {
        LE_INFO("Skip trying to stop Wifi connection still in use via le_dcs");
        le_dcs_MarkChannelSharingStatus(Ssid, LE_DCS_TECH_WIFI, false);
        IsConnected = false;
        memset(Ssid, '\0', sizeof(Ssid));
        SendConnStateEvent(false);
        ConnectionStatusHandler(LE_DATA_WIFI, false);
        return;
    }

    if (LE_OK != le_wifiClient_Disconnect())
    {
        LE_ERROR("Impossible to disconnect wifi client");

        // Increase number of retries
        StopSessionRetries++;

        // Start timer
        if (!le_timer_IsRunning(timerRef))
        {
            if (LE_OK != le_timer_SetContextPtr(timerRef, (void*)((intptr_t)LE_DATA_WIFI)))
            {
                LE_ERROR("Could not set context pointer for the StopDcs timer!");
                WifiClientStop();
            }
            else
            {
                if (LE_OK != le_timer_Start(timerRef))
                {
                    LE_ERROR("Could not start the StopDcs timer!");
                    WifiClientStop();
                }
            }
        }
    }
    else
    {
        IsConnected = false;
        le_dcs_MarkChannelSharingStatus(Ssid, LE_DCS_TECH_WIFI, false);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Try to stop the default data session using a defined technology
 */
//--------------------------------------------------------------------------------------------------
static void TryStopTechSession
(
    le_data_Technology_t technology     ///< [IN] Technology used for the connection to stop
)
{
    if (LE_DATA_MAX == technology)
    {
        LE_ERROR("Unknown technology used to stop the data session!");
    }
    else
    {
        switch (technology)
        {
            case LE_DATA_CELLULAR:
                TryStopDataSession(StopDcsTimer);
                break;

            case LE_DATA_WIFI:
                TryStopWifiSession(StopDcsTimer);
                break;

            default:
                LE_ERROR("Unknown technology %d to stop", technology);
                break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop Data Connection Service Timer Handler
 * When the timer expires, verify if the session is disconnected, if NOT retry to disconnect it and
 * rearm the timer
 */
//--------------------------------------------------------------------------------------------------
static void StopDcsTimerHandler
(
    le_timer_Ref_t timerRef     ///< [IN] Timer used to ensure the end of the session
)
{
    le_data_Technology_t technology = (intptr_t)le_timer_GetContextPtr(timerRef);

    if (0 != RequestCount)
    {
        // Request has been requested in the meantime, the Release command process
        // can be interrupted

        // Reset number of retries
        StopSessionRetries = 0;
    }
    else
    {
        switch (technology)
        {
            case LE_DATA_CELLULAR:
            {
                // Try again if necessary
                if (StopSessionRetries < MAX_STOP_SESSION_RETRIES)
                {
                    TryStopDataSession(timerRef);
                }
                else
                {
                    // Reset number of retries
                    StopSessionRetries = 0;

                    LE_WARN("Impossible to stop mobile data session after %d retries, stop trying",
                            MAX_STOP_SESSION_RETRIES);
                }
            }
            break;

            case LE_DATA_WIFI:
            {
                // Try again if necessary
                if (StopSessionRetries < MAX_STOP_SESSION_RETRIES)
                {
                    if ((strlen(Ssid) != 0) && !le_dcs_ChannelIsInUse(Ssid, LE_DCS_TECH_WIFI))
                    {
                        TryStopWifiSession(timerRef);
                    }
                }
                else
                {
                    // Reset number of retries
                    StopSessionRetries = 0;

                    LE_WARN("Impossible to disconnect wifi client after %d retries, stop trying",
                             MAX_STOP_SESSION_RETRIES);
                }
            }
            break;

            default:
                LE_ERROR("Unknown current technology %d", CurrentTech);
                break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Increase the current backoff duration of the RetryTechTimer. Each time it's doubled after a
 * failed retry until it's capped by its max backoff value RETRY_TECH_BACKOFF_MAX
 * No need to stop the timer before adjusting its time interval, since it's called from the timer's
 * handler which guarantees its having been not running.
 */
//--------------------------------------------------------------------------------------------------
static void IncreaseRetryTechBackoff
(
    void
)
{
    uint16_t time2 = RetryTechBackoffCurrent * 2;

    // cap it to the max backoff allowed
    RetryTechBackoffCurrent = (time2 > RETRY_TECH_BACKOFF_MAX) ?
        RETRY_TECH_BACKOFF_MAX : time2;

    le_clk_Time_t retryInterval = {RetryTechBackoffCurrent, 0};
    if (LE_OK != le_timer_SetInterval(RetryTechTimer, retryInterval))
    {
        LE_ERROR("Failed to adjust RetryTechTimer timer to %d secs",
                 RetryTechBackoffCurrent);
    }
    else
    {
        LE_DEBUG("Adjusted RetryTechTimer timer to %d secs", RetryTechBackoffCurrent);
    }

}


//--------------------------------------------------------------------------------------------------
/**
 * Retry next tech Timer Handler
 * When the timer expires, proceed to trying the next technology.
 */
//--------------------------------------------------------------------------------------------------
static void RetryTechTimerHandler
(
    le_timer_Ref_t timerRef     ///< [IN] Timer used to ensure the end of the session
)
{
    le_data_Technology_t technology = (intptr_t)le_timer_GetContextPtr(timerRef);
    LE_DEBUG("RetryTechTimer expired for technology %d", technology);

    // Disconnect the current technology which is not available anymore
    TryStopTechSession(technology);

    IncreaseRetryTechBackoff();

    // Connect the next technology to use
    TryStartTechSession(GetNextTech(technology));
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a command
 */
//--------------------------------------------------------------------------------------------------
static void ProcessCommand
(
    void* reportPtr     ///< [IN] Command to process
)
{
    CommandData_t cmdData = *(CommandData_t*)reportPtr;

    LE_PRINT_VALUE("%i", cmdData.command);

    switch (cmdData.command) {
    case REQUEST_COMMAND:
    {
        RequestCount++;
        LE_DEBUG("RequestCount %d, IsConnected %d", RequestCount, IsConnected);

        if (!IsConnected)
        {
            // Check if this is the first connection request.
            // If the request count is strictly greater than one, it means that a default
            // data connection was already requested. No need to try and connect again.
            // The connection notification will be sent when DCS retrieves the data session.
            if (1 == RequestCount)
            {
                // Get the technology to use from the list and start the data session
                TryStartTechSession(le_data_GetFirstUsedTechnology());
            }
        }
        else
        {
            // There is already a data session, so send a fake event so that the new application
            // that just sent the command knows about the current state.  This will also cause
            // redundant info to be sent to the other registered apps, but that's okay
            SendConnStateEvent(true);
        }
    }
    break;
    case RELEASE_COMMAND:
    {
        // Don't decrement below zero, as it will wrap-around
        if (RequestCount > 0)
        {
            RequestCount--;
        }

        if (0 == RequestCount)
        {
            // Try and disconnect the current technology
            TryStopTechSession(CurrentTech);
        }
        else
        {
            LE_DEBUG("Skip stopping technology %d as request count is %d", CurrentTech,
                     RequestCount);
        }
    }
    break;
    case START_COMMAND:
        le_dcsTech_Start(cmdData.channelName, cmdData.technology);
        break;
    case STOP_COMMAND:
        le_dcsTech_Stop(cmdData.channelName, cmdData.technology);
        break;
    default:
    {
        LE_ERROR("Command %i is not valid", cmdData.command);
    }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for PS change notifications.
 * When the connection is disconnected, we depend on packet switch event to determine when to try
 * again.
 */
//--------------------------------------------------------------------------------------------------
static void PacketSwitchHandler
(
    le_mrc_NetRegState_t psState,
    void*        contextPtr
)
{
    LE_DEBUG("New PS state: %d", psState);
    switch(psState)
    {
        case LE_MRC_REG_NONE:
            LE_DEBUG("New PS state LE_MRC_REG_NONE");
            break;
        case LE_MRC_REG_HOME:
        case LE_MRC_REG_ROAMING:
            LE_DEBUG("New PS state ATTACHED");
            if ((LE_DATA_CELLULAR == CurrentTech) && (RequestCount > 0) && (!IsConnected) &&
                !IsDataProfileViaLeDcs)
            {
                // Start a connection
                TryStartDataSession();
            }
            break;
        default:
            LE_ERROR("New PS state unknown PS state %d", psState);
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function to report network registration reject indication.
 * When network did a local implicit detach, a data session retry
 * is needed.
 */
//--------------------------------------------------------------------------------------------------
static void DcsNetRejectHandler
(
    const le_mrc_NetRegRejectInd_t*  networkRejectIndPtr,
    void*           contextPtr
)
{
    LE_DEBUG("Network Reject Ind with reject cause.%d, domain.%d, RAT.%d, mcc.%s and mnc.%s",
             networkRejectIndPtr->cause, networkRejectIndPtr->rejDomain, networkRejectIndPtr->rat,
             networkRejectIndPtr->mcc, networkRejectIndPtr->mnc);

    if ( networkRejectIndPtr->cause == LE_MRC_NETWORK_IMPLICIT_DETACH )
    {
        if ((LE_DATA_CELLULAR == CurrentTech) && (RequestCount > 0) &&
            !IsDataProfileViaLeDcs)
        {
            TryStartDataSession();
        }
    }

}

//--------------------------------------------------------------------------------------------------
/**
 * Handler for connection status
 */
//--------------------------------------------------------------------------------------------------
static void ConnectionStatusHandler
(
    le_data_Technology_t technology,    ///< [IN] Affected technology
    bool connected                      ///< [IN] Connection status
)
{
    LE_DEBUG("Technology: %d Connected: %d", technology, connected);
    // Check if the default data connection is still necessary
    if ((false == connected) && (RequestCount > 0))
    {
        // Start timer to start the next technology
        if (!le_timer_IsRunning(RetryTechTimer))
        {
            if (LE_OK != le_timer_SetContextPtr(RetryTechTimer, (void*)((intptr_t)technology)))
            {
                LE_ERROR("Could not set context pointer for the RetryTechTimer timer!");
            }
            else
            {
                if (LE_OK != le_timer_Start(RetryTechTimer))
                {
                    LE_ERROR("Could not start the RetryTechTimer timer!");
                }
                else
                {
                    LE_DEBUG("RetryTechTimer started with duration %d", RetryTechBackoffCurrent);
                }
            }
        }
    }
    else if (connected)
    {
        ResetRetryTechBackoff();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Connection State Handler
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerConnectionStateHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    ConnStateData_t* eventDataPtr = reportPtr;
    le_data_ConnectionStateHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc(eventDataPtr->interfaceName,
                      eventDataPtr->isConnected,
                      le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for the close session service
 */
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    LE_INFO("Client %p killed, remove allocated resources", sessionRef);

    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    // Search the data reference used by the killed client
    le_ref_IterRef_t iterRef = le_ref_GetIterator(RequestRefMap);
    le_result_t result = le_ref_NextNode(iterRef);

    while (LE_OK == result)
    {
        le_msg_SessionRef_t session = (le_msg_SessionRef_t) le_ref_GetValue(iterRef);

        // Check if the session reference saved matches with the current session reference
        if (session == sessionRef)
        {
            // Release the data connection
            le_data_Release((le_data_RequestObjRef_t) le_ref_GetSafeRef(iterRef));
        }

        // Get the next value in the reference map
        result = le_ref_NextNode(iterRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check IP address
 *
 * @return
 *      - LE_OK     on success
 *      - LE_FAULT  on failure
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CheckIpAddress
(
    int af,             ///< Address family
    const char* addStr  ///< IP address to check
)
{
    struct sockaddr_in sa;

    if (inet_pton(af, addStr, &(sa.sin_addr)))
    {
        return LE_OK;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Change the route on the data connection service interface, if the data session is connected using
 * the cellular technology and has an IPv4 or IPv6 address.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    cellular technology not currently used
 *      - LE_BAD_PARAMETER  incorrect IP address
 *      - LE_FAULT          for all other errors
 *
 * @note Limitations:
 *      - only IPv4 is supported for the moment
 *      - route only removed for a cellular connection
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ChangeRoute
(
    const char*          ipDestAddrStr,  ///< Destination IP address in dotted format
    pa_dcs_RouteAction_t action          ///< Add or remove the route
)
{
    char interfaceStr[LE_MDC_INTERFACE_NAME_MAX_BYTES] = {0};

    // Check if the cellular technology is being used
    if (LE_DATA_CELLULAR != CurrentTech)
    {
        LE_ERROR("Cellular technology not used");
        return LE_UNSUPPORTED;
    }

    // Check if the given address is in IPv4 format
    if (LE_OK != CheckIpAddress(AF_INET, ipDestAddrStr))
    {
        LE_ERROR("Bad address");
        return LE_BAD_PARAMETER;
    }

    // Retrieve the cellular interface name
    if (   (!MobileProfileRef)
        || (LE_OK != le_mdc_GetInterfaceName(MobileProfileRef, interfaceStr, sizeof(interfaceStr))))
    {
        LE_WARN("le_mdc_GetInterfaceName failed");
        return LE_FAULT;
    }

    return pa_dcs_ChangeRoute(action, ipDestAddrStr, "", interfaceStr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve time from a time server with the configuration indicated by the configTree.
 *
 * @return
 *      - LE_OK     The function succeeded
 *      - LE_FAULT  The function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RetrieveTimeFromServer
(
    pa_dcs_TimeStruct_t* timePtr    ///< [OUT] Time structure
)
{
    le_data_TimeProtocol_t timeProtocol = GetTimeProtocol();
    char timeServer[MAX_TIME_SERVER_LENGTH] = {0};

    switch (timeProtocol)
    {
        case LE_DATA_TP:
            GetTimeServer(timeServer, sizeof(timeServer), DEFAULT_TIMEPROTOCOL_SERVER);
            if (LE_OK != pa_dcs_GetTimeWithTimeProtocol(timeServer, timePtr))
            {
                LE_ERROR("Unable to retrieve time from server");
                return LE_FAULT;
            }
            break;

        case LE_DATA_NTP:
            GetTimeServer(timeServer, sizeof(timeServer), DEFAULT_NTP_SERVER);
            if (LE_OK != pa_dcs_GetTimeWithNetworkTimeProtocol(timeServer, timePtr))
            {
                LE_ERROR("Unable to retrieve time from server");
                return LE_FAULT;
            }
            break;

        default:
            LE_WARN("Unsupported time protocol %d", timeProtocol);
            return LE_FAULT;
    }

    LE_DEBUG("Time retrieved from server: %04d-%02d-%02d %02d:%02d:%02d:%03d",
             timePtr->year, timePtr->mon, timePtr->day,
             timePtr->hour, timePtr->min, timePtr->sec, timePtr->msec);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
// APIs
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler...
 */
//--------------------------------------------------------------------------------------------------
le_data_ConnectionStateHandlerRef_t le_data_AddConnectionStateHandler
(
    le_data_ConnectionStateHandlerFunc_t handlerPtr,    ///< [IN] Handler pointer
    void* contextPtr                                    ///< [IN] Associated context pointer
)
{
    LE_PRINT_VALUE("%p", handlerPtr);
    LE_PRINT_VALUE("%p", contextPtr);

    le_event_HandlerRef_t handlerRef = le_event_AddLayeredHandler(
                                                    "DataConnState",
                                                    ConnStateEvent,
                                                    FirstLayerConnectionStateHandler,
                                                    (le_event_HandlerFunc_t)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_data_ConnectionStateHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function removes a handler...
 */
//--------------------------------------------------------------------------------------------------
void le_data_RemoveConnectionStateHandler
(
    le_data_ConnectionStateHandlerRef_t addHandlerRef   ///< [IN] Connection state handler reference
)
{
    LE_PRINT_VALUE("%p", addHandlerRef);

    le_event_RemoveHandler((le_event_HandlerRef_t)addHandlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Request the default data connection
 *
 * @return
 *      - A reference to the data connection (to be used later for releasing the connection)
 *      - NULL if the data connection requested could not be processed
 */
//--------------------------------------------------------------------------------------------------
le_data_RequestObjRef_t le_data_Request
(
    void
)
{
    CommandData_t cmdData;
    le_msg_SessionRef_t sessionRef = le_data_GetClientSessionRef();

    cmdData.command = REQUEST_COMMAND;
    le_event_Report(CommandEvent, &cmdData, sizeof(cmdData));

    // Need to return a unique reference that will be used by Release.  Don't actually have
    // any data for now, but have to use some value other than NULL for the data pointer.
    le_data_RequestObjRef_t reqRef = le_ref_CreateRef(RequestRefMap, (void*)sessionRef);

    LE_DEBUG("Connection requested by session %p, reference %p", sessionRef, reqRef);

    return reqRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release a previously requested data connection
 */
//--------------------------------------------------------------------------------------------------
void le_data_Release
(
    le_data_RequestObjRef_t requestRef  ///< [IN] Reference to a previously requested connection
)
{
    CommandData_t cmdData;

    // Look up the reference.  If it is NULL, then the reference is not valid.
    // Otherwise, delete the reference and send the release command to the data thread.
    void* dataPtr = le_ref_Lookup(RequestRefMap, requestRef);
    if (NULL == dataPtr)
    {
        LE_ERROR("Invalid data request reference %p", requestRef);
    }
    else
    {
        LE_PRINT_VALUE("%p", requestRef);
        le_ref_DeleteRef(RequestRefMap, requestRef);

        cmdData.command = RELEASE_COMMAND;
        le_event_Report(CommandEvent, &cmdData, sizeof(cmdData));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the rank of the technology used for the data connection service
 *
 * @return
 *      - @ref LE_OK if the technology is added to the list
 *      - @ref LE_BAD_PARAMETER if the technology is unknown
 *      - @ref LE_UNSUPPORTED if the technology is not available
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_data_SetTechnologyRank
(
    uint32_t rank,                  ///< [IN] Rank of the used technology
    le_data_Technology_t technology ///< [IN] Technology
)
{
    // Check if technology is correct
    if (technology >= LE_DATA_MAX)
    {
        LE_WARN("Unknown technology %d, not added to the list", technology);
        return LE_BAD_PARAMETER;
    }

    // Get technology string
    char techStr[DCS_TECH_BYTES] = {0};
    GetTechnologyString(technology, techStr, sizeof(techStr));

    // Check if technology is available
    if (false == TechAvailability[technology])
    {
        LE_WARN("Unsupported technology '%s', not added to the list", techStr);
        return LE_UNSUPPORTED;
    }

    LE_DEBUG("Adding technology '%s' with the rank %d to the list", techStr, rank);

    // Check if technology is already in the list
    TechRecord_t* techPtr = IsTechInList(technology);
    if (NULL != techPtr)
    {
        if (rank != techPtr->rank)
        {
            // Remove the technology from it's current rank
            le_dls_Remove(&TechList, &techPtr->link);

            // The technology can now be added at the new rank
            LE_DEBUG("Technology %s was already in list with rank %d, setting new rank %d",
                     techStr, techPtr->rank, rank);
        }
        else
        {
            // Technology already in list with correct rank, nothing to do
            LE_DEBUG("Technology %s already in list with same rank %d", techStr, rank);
            return LE_OK;
        }
    }

    // Check if list is empty
    if (true == le_dls_IsEmpty(&TechList))
    {
        // Create the new technology node
        TechRecord_t* newTechPtr = le_mem_ForceAlloc(TechListPoolRef);
        newTechPtr->tech = technology;
        newTechPtr->rank = rank;
        newTechPtr->link = LE_DLS_LINK_INIT;

        // Add the technology node to the list
        le_dls_Stack(&TechList, &(newTechPtr->link));
    }
    else
    {
        // Insert in the list according to the rank
        le_dls_Link_t* currentLinkPtr = TechList.headLinkPtr;
        le_dls_Link_t* nextLinkPtr = NULL;
        uint32_t currRank = 0;
        uint32_t nextRank = UINT32_MAX;
        bool inserted = false;

        // Create the new technology node
        TechRecord_t* newTechPtr = le_mem_ForceAlloc(TechListPoolRef);
        newTechPtr->tech = technology;
        newTechPtr->rank = rank;
        newTechPtr->link = LE_DLS_LINK_INIT;

        // Find the correct rank for the new technology
        do
        {
            currRank = CONTAINER_OF(currentLinkPtr, TechRecord_t, link)->rank;
            nextLinkPtr = le_dls_PeekNext(&TechList, currentLinkPtr);
            nextRank = (NULL != nextLinkPtr ? CONTAINER_OF(nextLinkPtr, TechRecord_t, link)->rank
                                            : UINT32_MAX);

            if (rank < currRank)
            {
                // Lower rank for the new technology, add it before the current technology

                // Add the node before the current one
                le_dls_AddBefore(&TechList, currentLinkPtr, &(newTechPtr->link));
                inserted = true;
            }
            else if (rank == currRank)
            {
                // Same rank for the new technology, add it before the current technology
                // and increment the rank of the current and next ones

                // Add the node before the current one
                le_dls_AddBefore(&TechList, currentLinkPtr, &(newTechPtr->link));
                inserted = true;
                // The next ranks are incremented
                IncrementTechRanks(le_dls_PeekNext(&TechList, &(newTechPtr->link)));
            }
            else
            {
                // Higher rank for the new technology, check the next rank to know where
                // it should be inserted in the list

                if (rank < nextRank)
                {
                    // Higher next rank, add the new technology between the current one
                    // and the next one

                    // Add the node after the current one and before the next one
                    le_dls_AddAfter(&TechList, currentLinkPtr, &(newTechPtr->link));
                    inserted = true;
                }
                else if (rank == nextRank)
                {
                    // Same next rank, add the new technology between the current one
                    // and the next one, and increment the rank of the current and next technologies

                    // Add the node after the current one and replace the next one
                    le_dls_AddAfter(&TechList, currentLinkPtr, &(newTechPtr->link));
                    inserted = true;
                    // The next ranks are incremented
                    IncrementTechRanks(le_dls_PeekNext(&TechList, &(newTechPtr->link)));
                }
                else
                {
                    // Lower next rank, try the next link in list
                }
            }

            // Move to the next link
            currentLinkPtr = currentLinkPtr->nextPtr;

        } while ((currentLinkPtr != TechList.headLinkPtr) && (false == inserted));
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the first technology to use
 * @return
 *      - One of the technologies from @ref le_data_Technology_t enumerator if the list is not empty
 *      - @ref LE_DATA_MAX if the list is empty
 */
//--------------------------------------------------------------------------------------------------
le_data_Technology_t le_data_GetFirstUsedTechnology
(
    void
)
{
    le_data_Technology_t firstTech = LE_DATA_MAX;

    // Check if list is empty
    if (false == le_dls_IsEmpty(&TechList))
    {
        // Get first technology
        le_dls_Link_t* linkPtr = le_dls_Peek(&TechList);

        if (NULL != linkPtr)
        {
            uint32_t firstRank = 0;
            char techStr[DCS_TECH_BYTES] = {0};

            // Retrieve technology and rank
            TechRecord_t* techPtr = CONTAINER_OF(linkPtr, TechRecord_t, link);
            firstTech = techPtr->tech;
            firstRank = techPtr->rank;
            // Store last peeked technology
            CurrTechPtr = linkPtr;

            GetTechnologyString(firstTech, techStr, sizeof(techStr));

            LE_DEBUG("First used technology: '%s' with rank %d", techStr, firstRank);
        }
        else
        {
            LE_WARN("Cannot get first used technology");
        }
    }
    else
    {
        LE_INFO("Used technologies list is empty");
    }

    return firstTech;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the next technology to use
 * @return
 *      - One of the technologies from @ref le_data_Technology_t enumerator if the list is not empty
 *      - @ref LE_DATA_MAX if the list is empty or the end of the list is reached
 */
//--------------------------------------------------------------------------------------------------
le_data_Technology_t le_data_GetNextUsedTechnology
(
    void
)
{
    le_data_Technology_t nextTech = LE_DATA_MAX;

    // Check if list is empty
    if (false == le_dls_IsEmpty(&TechList))
    {
        // Check if CurrTechPtr is coherent
        if (   (NULL != CurrTechPtr)
            && (true == le_dls_IsInList(&TechList, CurrTechPtr))
           )
        {
            le_dls_Link_t* linkPtr = le_dls_PeekNext(&TechList, CurrTechPtr);

            if (NULL != linkPtr)
            {
                uint32_t nextRank = 0;
                char techStr[DCS_TECH_BYTES] = {0};

                // Retrieve technology and rank
                TechRecord_t* techPtr = CONTAINER_OF(linkPtr, TechRecord_t, link);
                nextTech = techPtr->tech;
                nextRank = techPtr->rank;
                // Store last peeked technology
                CurrTechPtr = linkPtr;

                GetTechnologyString(nextTech, techStr, sizeof(techStr));

                LE_DEBUG("Next used technology: '%s' with rank %d", techStr, nextRank);
            }
            else
            {
                LE_DEBUG("End of used technologies list, cannot get the next one");
            }
        }
        else
        {
            LE_ERROR("Incoherent CurrTechPtr %p", CurrTechPtr);
        }
    }
    else
    {
        LE_INFO("Used technologies list is empty");
    }

    return nextTech;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the technology currently used for the default data connection
 *
 * @return
 *      - One of the technologies from @ref le_data_Technology_t enumerator
 *      - @ref LE_DATA_MAX if the current technology is not set
 *
 * @note The supported technologies are @ref LE_DATA_WIFI and @ref LE_DATA_CELLULAR
 */
//--------------------------------------------------------------------------------------------------
le_data_Technology_t le_data_GetTechnology
(
    void
)
{
    // Return the currently used technology stored
    return CurrentTech;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the cellular profile index used by the data connection service when the cellular technology
 * is active.
 *
 * @return
 *      - Cellular profile index
 */
//--------------------------------------------------------------------------------------------------
int32_t le_data_GetCellularProfileIndex
(
    void
)
{
    return le_dcsCellular_GetProfileIndex(MdcIndexProfile);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the cellular profile index used by the data connection service when the cellular technology
 * is active.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_BAD_PARAMETER  if the profile index is invalid
 *      - LE_BUSY           the cellular connection is in use
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_data_SetCellularProfileIndex
(
    int32_t profileIndex    ///< [IN] Cellular profile index to be used
)
{
    le_mrc_Rat_t rat;
    int32_t profileIndexMin;
    int32_t profileIndexMax;

    if ((IsConnected) && (LE_DATA_CELLULAR == CurrentTech))
    {
        LE_ERROR("Cellular connection in use");
        return LE_BUSY;
    }

    if (LE_OK != le_mrc_GetRadioAccessTechInUse(&rat))
    {
        rat = LE_MRC_RAT_GSM;
    }

    switch (rat)
    {
        /* 3GPP2 */
        case LE_MRC_RAT_CDMA:
            profileIndexMin = PA_MDC_MIN_INDEX_3GPP2_PROFILE;
            profileIndexMax = PA_MDC_MAX_INDEX_3GPP2_PROFILE;
        break;

        /* 3GPP */
        default:
            profileIndexMin = PA_MDC_MIN_INDEX_3GPP_PROFILE;
            profileIndexMax = PA_MDC_MAX_INDEX_3GPP_PROFILE;
        break;
    }

    if (   ((profileIndex >= profileIndexMin) && (profileIndex <= profileIndexMax))
        || (LE_MDC_DEFAULT_PROFILE == profileIndex))
    {
        MdcIndexProfile = profileIndex;
        SetDataProfileIndex(profileIndex);
        return LE_OK;
    }

    return LE_BAD_PARAMETER;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the default route activation status for the data connection service interface.
 *
 * @return
 *      - true:  the default route is set by the data connection service
 *      - false: the default route is not set by the data connection service
 */
//--------------------------------------------------------------------------------------------------
bool le_data_GetDefaultRouteStatus
(
    void
)
{
    return DefaultRouteStatus;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add a route on the data connection service interface, if the data session is connected using
 * the cellular technology and has an IPv4 or IPv6 address.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    cellular technology not currently used
 *      - LE_BAD_PARAMETER  incorrect IP address
 *      - LE_FAULT          for all other errors
 *
 * @note Limitations:
 *      - only IPv4 is supported for the moment
 *      - route only added for a cellular connection
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_data_AddRoute
(
    const char* ipDestAddrStr     ///< [IN] The destination IP address in dotted format
)
{
    return ChangeRoute(ipDestAddrStr, PA_DCS_ROUTE_ADD);
}

//--------------------------------------------------------------------------------------------------
/**
 * Delete a route on the data connection service interface, if the data session is connected using
 * the cellular technology and has an IPv4 or IPv6 address.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    cellular technology not currently used
 *      - LE_BAD_PARAMETER  incorrect IP address
 *      - LE_FAULT          for all other errors
 *
 * @note Limitations:
 *      - only IPv4 is supported for the moment
 *      - route only removed for a cellular connection
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_data_DelRoute
(
    const char* ipDestAddrStr     ///< [IN] The destination IP address in dotted format
)
{
    return ChangeRoute(ipDestAddrStr, PA_DCS_ROUTE_DELETE);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the date from the configured server using the configured time protocol.
 *
 * @warning An active data connection is necessary to retrieve the date.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_BAD_PARAMETER  if a parameter is incorrect
 *      - LE_FAULT          if an error occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_data_GetDate
(
    uint16_t* yearPtr,      ///< [OUT] UTC Year A.D. [e.g. 2017].
    uint16_t* monthPtr,     ///< [OUT] UTC Month into the year [range 1...12].
    uint16_t* dayPtr        ///< [OUT] UTC Days into the month [range 1...31].
)
{
    pa_dcs_TimeStruct_t timeStruct;

    if ((!yearPtr) || (!monthPtr) || (!dayPtr))
    {
        LE_ERROR("Incorrect parameter");
        return LE_BAD_PARAMETER;
    }

    if (!IsConnected)
    {
        LE_ERROR("Data Connection Service is not connected");
        return LE_FAULT;
    }

    memset(&timeStruct, 0, sizeof(timeStruct));
    if (LE_OK != RetrieveTimeFromServer(&timeStruct))
    {
        return LE_FAULT;
    }

    *yearPtr  = (uint16_t) timeStruct.year;
    *monthPtr = (uint16_t) timeStruct.mon;
    *dayPtr   = (uint16_t) timeStruct.day;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the time from the configured server using the configured time protocol.
 *
 * @warning An active data connection is necessary to retrieve the time.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_BAD_PARAMETER  if a parameter is incorrect
 *      - LE_FAULT          if an error occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_data_GetTime
(
    uint16_t* hoursPtr,         ///< [OUT] UTC Hours into the day [range 0..23].
    uint16_t* minutesPtr,       ///< [OUT] UTC Minutes into the hour [range 0..59].
    uint16_t* secondsPtr,       ///< [OUT] UTC Seconds into the minute [range 0..59].
    uint16_t* millisecondsPtr   ///< [OUT] UTC Milliseconds into the second [range 0..999].
)
{
    pa_dcs_TimeStruct_t timeStruct;

    if ((!hoursPtr) || (!minutesPtr) || (!secondsPtr) || (!millisecondsPtr))
    {
        LE_ERROR("Incorrect parameter");
        return LE_BAD_PARAMETER;
    }

    if (!IsConnected)
    {
        LE_ERROR("Data Connection Service is not connected");
        return LE_FAULT;
    }

    memset(&timeStruct, 0, sizeof(timeStruct));
    if (LE_OK != RetrieveTimeFromServer(&timeStruct))
    {
        return LE_FAULT;
    }

    *hoursPtr        = (uint16_t) timeStruct.hour;
    *minutesPtr      = (uint16_t) timeStruct.min;
    *secondsPtr      = (uint16_t) timeStruct.sec;
    *millisecondsPtr = (uint16_t) timeStruct.msec;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 *  Server initialization
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Initialize the various events
    CommandEvent = le_event_CreateId("Data Command", sizeof(CommandData_t));
    ConnStateEvent = le_event_CreateId("Conn State", sizeof(ConnStateData_t));

    // Create memory pool and expand it
    TechListPoolRef = le_mem_CreatePool("Technologies list pool", sizeof(TechRecord_t));
    le_mem_ExpandPool(TechListPoolRef, DCS_TECH_NUMBER);

    // Create safe reference map for request references. The size of the map should be based on
    // the expected number of simultaneous data requests, so take a reasonable guess.
    RequestRefMap = le_ref_CreateMap("Requests", REFERENCE_MAP_SIZE);

    // Set a one-shot timer for requesting the DNS configuration.
    SetDNSConfigTimer = le_timer_Create("SetDNSConfigTimer");
    le_clk_Time_t dnsInterval = {30, 0}; // 30 seconds

    if (   (LE_OK != le_timer_SetHandler(SetDNSConfigTimer, SetDNSConfigTimerHandler))
        || (LE_OK != le_timer_SetRepeat(SetDNSConfigTimer, 1))    // One shot timer
        || (LE_OK != le_timer_SetInterval(SetDNSConfigTimer, dnsInterval))
       )
    {
        LE_ERROR("Could not start the SetDNSConfig timer!");
    }

    // Retrieve default gateway activation status
    DefaultRouteStatus = GetDefaultRouteStatus();

    // Set a timer to retry the tech
    RetryTechTimer = le_timer_Create("RetryTechTimer");
    RetryTechBackoffCurrent = RETRY_TECH_BACKOFF_INIT;
    le_clk_Time_t retryInterval = {RETRY_TECH_BACKOFF_INIT, 0};

    if (   (LE_OK != le_timer_SetHandler(RetryTechTimer, RetryTechTimerHandler))
        || (LE_OK != le_timer_SetRepeat(RetryTechTimer, 1))       // One shot timer
        || (LE_OK != le_timer_SetInterval(RetryTechTimer, retryInterval))
       )
    {
        LE_ERROR("Could not configure the RetryTechTimer timer!");
    }


    // Set a timer to retry the stop data session
    StopDcsTimer = le_timer_Create("StopDcsTimer");
    le_clk_Time_t interval = {0, 5};    // 5 seconds

    if (   (LE_OK != le_timer_SetHandler(StopDcsTimer, StopDcsTimerHandler))
        || (LE_OK != le_timer_SetRepeat(StopDcsTimer, 1))       // One shot timer
        || (LE_OK != le_timer_SetInterval(StopDcsTimer, interval))
       )
    {
        LE_ERROR("Could not configure the StopDcs timer!");
    }

    #ifdef LEGATO_DATA_CONNECTION_DELAY
    // Set a timer to delay connection request to the modem
    DelayRequestTimer = le_timer_Create("DelayRequestTimer");

    le_clk_Time_t delayInterval = {60, 0};   // 60 seconds
    if (   (LE_OK != le_timer_SetRepeat(DelayRequestTimer, 1))       // One shot timer
        || (LE_OK != le_timer_SetInterval(DelayRequestTimer, delayInterval))
       )
    {
        LE_ERROR("Could not configure the DelayRequestTimer timer!");
    }
    #endif

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler(le_data_GetServiceRef(),
                                  CloseSessionEventHandler,
                                  NULL);

    // Services required by DCS

    // 1. Mobile services
    // Mobile services are always available
    TechAvailability[LE_DATA_CELLULAR] = true;

    // 2. Wifi service
    // Check wifi client availability
    if (LE_OK == le_wifiClient_TryConnectService())
    {
        LE_INFO("Wifi client is available");
        TechAvailability[LE_DATA_WIFI] = true;
    }
    else
    {
        LE_INFO("Wifi client is not available");
        TechAvailability[LE_DATA_WIFI] = false;
    }

    // Initialize technologies list with default values
    InitDefaultTechList();

    // Register for packet switch state event
    le_mrc_AddPacketSwitchedChangeHandler(PacketSwitchHandler, NULL);

    // Register for network registration reject event
    le_mrc_AddNetRegRejectHandler(DcsNetRejectHandler, NULL);

    // Register for command events
    le_event_AddHandler("ProcessCommand", CommandEvent, ProcessCommand);

    // Register main loop with watchdog chain
    // Try to kick a couple of times before each timeout.

    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);

    LE_INFO("Data Connection Service is ready");
}
