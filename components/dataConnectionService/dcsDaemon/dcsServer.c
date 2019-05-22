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
 * The data connection is actually started when the Cellular Network Service State is 'ROAMING' or
 * 'HOME'.
 * - With the 'Wi-Fi' technology, the DCS first starts the wifi client and reads the Access Point
 * configuration in the config tree. The data connection is then started by connecting to the
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
#include "le_print.h"
#include "pa_dcs.h"
#include "dcsServer.h"
#include "dcs.h"
#include "dcsNet.h"
#include "dcsTechRank.h"
#include "watchdogChain.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

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
DcsConnStateData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Declaration of functions
 */
//--------------------------------------------------------------------------------------------------
static void UpdateTechnologyStatus(le_data_Technology_t technology, bool connected, bool notify);
static void TryStopTechSession(void);

//--------------------------------------------------------------------------------------------------
// Declarations of variables
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// For use with le_dcs interface
//--------------------------------------------------------------------------------------------------
static le_dcs_EventHandlerRef_t DataChannelEventHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Currently used data channel with its technology type & object reference
 */
//--------------------------------------------------------------------------------------------------
static char DataChannelName[LE_DCS_CHANNEL_NAME_MAX_LEN] = {0};
static le_data_Technology_t CurrentTech = LE_DATA_MAX;
static le_dcs_ChannelRef_t DataChannelRef = NULL;
static le_dcs_ReqObjRef_t DataChannelReqRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Set DNS Configuration Timer reference
 */
//--------------------------------------------------------------------------------------------------
#define DNS_CONFIG_RETRY_TIMEOUT 10
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
 * Event for sending command to Process command handler
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t dcs_CommandEventId;

//--------------------------------------------------------------------------------------------------
/**
 * Event for sending connection to applications
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t ConnStateEvent;

//--------------------------------------------------------------------------------------------------
/**
 * Is the data connection connected
 */
//--------------------------------------------------------------------------------------------------
static bool IsConnected = false;
static bool RoutesAdded = false;

//--------------------------------------------------------------------------------------------------
/**
 * Count the number of requests
 */
//--------------------------------------------------------------------------------------------------
static uint32_t RequestCount = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for the request reference
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t RequestRefMap;

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
 * Has default GW and route set on the device for the connected data connection
 */
//--------------------------------------------------------------------------------------------------
static bool IsDefaultRouteSet = false;


//--------------------------------------------------------------------------------------------------
/**
 * Return the value of dcs_CommandEventId
 *
 * @return
 *     - dcs_CommandEventId
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t dcs_GetCommandEventId
(
    void
)
{
    return dcs_CommandEventId;
}


//--------------------------------------------------------------------------------------------------
/**
 * Return the value of RequestRefMap
 *
 * @return
 *     - RequestRefMap
 */
//--------------------------------------------------------------------------------------------------
le_ref_MapRef_t dcs_GetRequestRefMap
(
    void
)
{
    return RequestRefMap;
}


//--------------------------------------------------------------------------------------------------
/**
 * Return the value of DataChannelRef which is the le_dcs channel reference of the currently chosen
 * data channel to be established for le_data
 *
 * @return
 *     - value of DataChannelRef
 */
//--------------------------------------------------------------------------------------------------
void dcs_ChannelSetChannelName
(
    const char *channelName
)
{
    if (!channelName)
    {
        memset(DataChannelName, '\0', sizeof(DataChannelName));
    }
    else
    {
        le_utf8_Copy(DataChannelName, channelName, sizeof(DataChannelName), NULL);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Return the value of DataChannelRef which is the le_dcs channel reference of the currently chosen
 * data channel to be established for le_data
 *
 * @return
 *     - value of DataChannelRef
 */
//--------------------------------------------------------------------------------------------------
le_dcs_ChannelRef_t dcs_ChannelGetCurrentReference
(
    void
)
{
    return DataChannelRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the DataChannelRef which is the le_dcs channel reference of the currently chosen data
 * channel to be established for le_data
 */
//--------------------------------------------------------------------------------------------------
void dcs_ChannelSetCurrentReference
(
    le_dcs_ChannelRef_t channelRef
)
{
    DataChannelRef = channelRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Return the value of CurrentTech which is the currently chosen technology for establishing a
 * data connection
 *
 * @return
 *     - value of CurrentTech
 */
//--------------------------------------------------------------------------------------------------
le_data_Technology_t dcs_ChannelGetCurrentTech
(
    void
)
{
    return CurrentTech;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the value of CurrentTech which is the currently chosen technology for establishing a
 * data connection to the givent technology in the input
 */
//--------------------------------------------------------------------------------------------------
void dcs_ChannelSetCurrentTech
(
    le_data_Technology_t technology
)
{
    CurrentTech = technology;
}


//--------------------------------------------------------------------------------------------------
/**
 * Return the status of the le_data connection whether it's connected or not.
 *
 * @return
 *     - true if connection; false otherwise
 */
//--------------------------------------------------------------------------------------------------
bool dcs_ChannelIsConnected
(
    void
)
{
    return IsConnected;
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
    DcsConnStateData_t eventData;
    eventData.isConnected = isConnected;
    eventData.interfaceName[0] = '\0';

    // Set the interface name
    if (isConnected &&
        (LE_OK != le_dcsTech_GetNetInterface(dcsTechRank_ConvertToDcsTechEnum(CurrentTech),
                                             DataChannelRef, eventData.interfaceName,
                                             sizeof(eventData.interfaceName))))
    {
        LE_WARN("Failed to get net interface for up event on channel %s of technology %d",
                DataChannelName, CurrentTech);
    }

    LE_DEBUG("Reporting for net interface '%s' state[%i]", eventData.interfaceName,
             eventData.isConnected);

    // Send the event to interested applications
    le_event_Report(ConnStateEvent, &eventData, sizeof(eventData));
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the default GW configuration
 *
 */
//--------------------------------------------------------------------------------------------------
static void SetDefaultGWConfiguration
(
    void
)
{
    le_result_t ret;
    if (!DefaultRouteStatus || IsDefaultRouteSet)
    {
        return;
    }

    LE_INFO("Setting default GW address on device");
    le_net_BackupDefaultGW();
    ret = le_net_SetDefaultGW(DataChannelRef);
    if (LE_OK != ret)
    {
        LE_ERROR("Failed to set default GW address");
    }
    else
    {
        IsDefaultRouteSet = true;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function seeks to set or remove a host route for each of the known DNS server address
 * assigned for use for the selected data channel, according to the input argument whether to add
 * is true or false.
 *
 * @return
 *     - LE_OK upon successful any addition or removal of such routes
 *     - LE_FAULT upon failure for all attempts to add
 *     - LE_UNSUPPORTED if the technology of the selected data channel does not support DNS server
 *       address retrieval
 *     - LE_NOT_POSSIBLE if no DNS server address has been assigned for use for the selected data
 *       channel
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetDnsRoutes
(
    bool isAdd
)
{
    le_result_t ret;
    bool hasSet = false;
    static char v4DnsAddrs[2][PA_DCS_IPV4_ADDR_MAX_BYTES];
    static char v6DnsAddrs[2][PA_DCS_IPV6_ADDR_MAX_BYTES];

    // In entering here, DataChannelRef & CurrentTech are guaranteed to have been set

    // For removing added routes for DNS addresses
    if (!isAdd)
    {
        if (strlen(v4DnsAddrs[0]))
        {
            le_net_ChangeRoute(DataChannelRef, v4DnsAddrs[0], "", false);
            memset(v4DnsAddrs[0], '\0', PA_DCS_IPV4_ADDR_MAX_BYTES);
        }
        if (strlen(v4DnsAddrs[1]))
        {
            le_net_ChangeRoute(DataChannelRef, v4DnsAddrs[1], "", false);
            memset(v4DnsAddrs[1], '\0', PA_DCS_IPV4_ADDR_MAX_BYTES);
        }
        if (strlen(v6DnsAddrs[0]))
        {
            le_net_ChangeRoute(DataChannelRef, v6DnsAddrs[0], "", false);
            memset(v6DnsAddrs[0], '\0', PA_DCS_IPV6_ADDR_MAX_BYTES);
        }
        if (strlen(v6DnsAddrs[1]))
        {
            le_net_ChangeRoute(DataChannelRef, v6DnsAddrs[1], "", false);
            memset(v6DnsAddrs[1], '\0', PA_DCS_IPV6_ADDR_MAX_BYTES);
        }

        RoutesAdded = false;
        return LE_OK;
    }

    // For adding routes for DNS addresses
    ret = le_dcsTech_GetDNSAddresses(CurrentTech, DataChannelRef,
                                     (char *)v4DnsAddrs, PA_DCS_IPV4_ADDR_MAX_BYTES,
                                     (char *)v6DnsAddrs, PA_DCS_IPV6_ADDR_MAX_BYTES);
    if ((ret == LE_UNSUPPORTED) || (ret == LE_FAULT))
    {
        LE_WARN("No DNS server address retrievable from technology %d for data channel %s",
                CurrentTech, DataChannelName);
        return ret;
    }
    else if ((ret != LE_OK) ||
             ((strlen(v4DnsAddrs[0]) == 0) && (strlen(v4DnsAddrs[1]) == 0) &&
              (strlen(v6DnsAddrs[0]) == 0) && (strlen(v6DnsAddrs[1]) == 0)))
    {
        LE_INFO("Data channel %s of technology %d got no DNS server address assigned",
                DataChannelName, CurrentTech);
        return LE_NOT_POSSIBLE;
    }

    if (strlen(v4DnsAddrs[0]))
    {
        ret = le_net_ChangeRoute(DataChannelRef, v4DnsAddrs[0], "", true);
        if (LE_OK != ret)
        {
            memset(v4DnsAddrs[0], '\0', PA_DCS_IPV4_ADDR_MAX_BYTES);
        }
        hasSet = true;
    }
    if (strlen(v4DnsAddrs[1]))
    {
        ret = le_net_ChangeRoute(DataChannelRef, v4DnsAddrs[1], "", true);
        if (LE_OK != ret)
        {
            memset(v4DnsAddrs[1], '\0', PA_DCS_IPV4_ADDR_MAX_BYTES);
        }
        hasSet = true;
    }
    if (strlen(v6DnsAddrs[0]))
    {
        ret = le_net_ChangeRoute(DataChannelRef, v6DnsAddrs[0], "", true);
        if (LE_OK != ret)
        {
            memset(v6DnsAddrs[0], '\0', PA_DCS_IPV6_ADDR_MAX_BYTES);
        }
        hasSet = true;
    }
    if (strlen(v6DnsAddrs[1]))
    {
        ret = le_net_ChangeRoute(DataChannelRef, v6DnsAddrs[1], "", true);
        if (LE_OK != ret)
        {
            memset(v6DnsAddrs[1], '\0', PA_DCS_IPV6_ADDR_MAX_BYTES);
        }
        hasSet = true;
    }

    if (hasSet)
    {
        LE_INFO("Succeeded setting routes for DNS server address");
        RoutesAdded = true;
        return LE_OK;
    }

    return LE_NOT_POSSIBLE;
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
    void
)
{
    le_result_t ret;

    if (!DataChannelRef)
    {
        LE_ERROR("Unknown data channel reference for setting DNS server addresses");
        return LE_FAULT;
    }

    LE_INFO("Setting DNS server addresses on device");
    ret = le_net_SetDNS(DataChannelRef);
    if (LE_OK != ret)
    {
        LE_ERROR("Failed to set DNS server addresses; error %d", ret);
        return ret;
    }

    // Add the DNS route
    if (!DefaultRouteStatus && (SetDnsRoutes(true) == LE_OK))
    {
        LE_INFO("Succeeded to add routes for DNS addresses");
    }

    LE_INFO("Succeeded setting DNS configuration");
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the default route (if necessary) and DNS
 *
 * @return
 *      LE_FAULT        Function failed and no retry will follow
 *      LE_BUSY         Function hasn't succeeded but retry will follow
 *      LE_OK           Function succeeded
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetDefaultRouteAndDns
(
    void
)
{
    le_result_t ret;

    if (!DataChannelRef)
    {
        LE_ERROR("Unknown data channel reference for setting default GW and DNS server addresses");
        return LE_FAULT;
    }

    // Check if the default route should be set
    SetDefaultGWConfiguration();

    // Set the DNS configuration and add routes for it if default GW & route are not to be set
    ret = SetDnsConfiguration();
    if (LE_OK != ret)
    {
        LE_INFO("Failed to set DNS configuration");

        if (le_timer_IsRunning(SetDNSConfigTimer))
        {
            return LE_BUSY;
        }

        if ((LE_OK != le_timer_SetContextPtr(SetDNSConfigTimer, (void *)DataChannelRef)) ||
            (LE_OK != le_timer_Start(SetDNSConfigTimer)))
        {
            LE_ERROR("Failed to set SetDNSConfig timer to retry setting DNS configuration");
            return LE_FAULT;
        }

        LE_INFO("Wait for next retry to set DNS configuration");
        return LE_BUSY;
    }
    else
    {
        LE_INFO("Succeeded setting DNS configuration");
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This is the event handler to be added via le_dcs_AddEventHandler() for the selected channel to
 * be started via le_dcs
 */
//--------------------------------------------------------------------------------------------------
static void ChannelEventHandler
(
    le_dcs_ChannelRef_t channelRef, ///< [IN] The channel for which the event is
    le_dcs_Event_t event,           ///< [IN] Event up or down
    int32_t code,                   ///< [IN] Additional event code, like error code
    void *contextPtr                ///< [IN] Associated user context pointer
)
{
    const char *eventString = le_dcs_ConvertEventToString(event);
    LE_INFO("Received for channel reference %p event %s", channelRef, eventString);

    if (channelRef != DataChannelRef)
    {
        LE_DEBUG("Data channel event %d skipped; current channel in use: reference %p, name %s, "
                 "technology %d", event, DataChannelRef, DataChannelName, CurrentTech);
        return;
    }

    LE_DEBUG("Channel state IsConnected before event: %d", IsConnected);
    IsConnected = (event == LE_DCS_EVENT_UP) ? true : false;

    switch (CurrentTech)
    {
        case LE_DATA_CELLULAR:
            if (IsConnected)
            {
                // Up event. Set the default route (if necessary) and the DNS.
                switch (SetDefaultRouteAndDns())
                {
                    case LE_OK:
                        LE_DEBUG("Channel state IsConnected after event: %d", IsConnected);
                        UpdateTechnologyStatus(CurrentTech, IsConnected, true);
                        break;
                    case LE_BUSY:
                        LE_DEBUG("Failed to set default GW and DNS server addresses immediately; "
                                 "wait for retry");
                        break;
                    case LE_FAULT:
                    default:
                        // Impossible to use this technology, try the next one
                        LE_ERROR("Failed to set default GW and DNS server addresses; "
                                 "stopping current technology to try the next");
                        TryStopTechSession();
                        UpdateTechnologyStatus(CurrentTech, false, false);
                        break;
                }
                break;
            }

            // Down event
            LE_DEBUG("Channel state IsConnected after event: %d", IsConnected);
            if (event == LE_DCS_EVENT_DOWN)
            {
                TryStopTechSession();
                UpdateTechnologyStatus(CurrentTech, IsConnected, true);
            }
            else if (event == LE_DCS_EVENT_TEMP_DOWN)
            {
                // Don't stop tech nor start tech retry timer since le_dcs will retry it again
                // Just send a notification to upper app(s)
                SendConnStateEvent(IsConnected);
            }

            if (IsDefaultRouteSet)
            {
                le_net_RestoreDefaultGW();
                le_net_RestoreDNS();
                IsDefaultRouteSet = false;
            }
            else if (RoutesAdded)
            {
                // Here means having IsDefaultRouteSet false; remove routes for DNS addresses
                // previously added by DCS
                SetDnsRoutes(false);
            }
            break;
        case LE_DATA_WIFI:
            LE_DEBUG("Channel state IsConnected after event: %d", IsConnected);

            if (IsConnected)
            {
                UpdateTechnologyStatus(CurrentTech, IsConnected, true);
                break;
            }

            // Down event
            if (event == LE_DCS_EVENT_DOWN)
            {
                TryStopTechSession();
                UpdateTechnologyStatus(CurrentTech, IsConnected, true);
            }
            else if (event == LE_DCS_EVENT_TEMP_DOWN)
            {
                // Don't stop tech nor start tech retry timer since le_dcs will retry it again
                // Just send a notification to upper app(s)
                SendConnStateEvent(IsConnected);
            }
            break;
        default:
            break;
    }
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
 * SetDNSConfigTimer Handler for retrying setting DNS server addresses upon this timer's expiration
 */
//--------------------------------------------------------------------------------------------------
static void SetDNSConfigTimerHandler
(
    le_timer_Ref_t timerRef    ///< [IN] Timer used to ensure DNS address is present.
)
{
    le_result_t ret;
    le_dcs_ChannelRef_t dataChannelRefInContext;

    if ((0 == RequestCount) || !DataChannelRef)
    {
        // No need to retry setting DNS config anymore
        LE_INFO("No need to retry setting DNS configuration: RequestCount %d", RequestCount);
        return;
    }

    dataChannelRefInContext = le_timer_GetContextPtr(timerRef);
    if (dataChannelRefInContext != DataChannelRef)
    {
        // No need to retry setting DNS config anymore for the data channel set into the timer
        // context as during the retry backoff a newer channel has been brought up
        LE_DEBUG("No need to retry setting DNS configuration: previous channel reference %p, "
                 "newer channel reference %p", dataChannelRefInContext, DataChannelRef);
        return;
    }

    // Retry setting DNS configuration
    ret = SetDnsConfiguration();
    if (LE_OK != ret)
    {
        LE_ERROR("Releasing data channel upon failure to retry setting DNS configuration; error %d",
                 ret);
        TryStopTechSession();
        UpdateTechnologyStatus(CurrentTech, false, false);
    }
    else
    {
        LE_INFO("Succeeded setting DNS configuration");
        IsConnected = true;
        LE_DEBUG("Channel state IsConnected after event: %d", IsConnected);
        UpdateTechnologyStatus(CurrentTech, IsConnected, true);
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
    if (le_timer_IsRunning(RetryTechTimer))
    {
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
 * This function resets the previously selected data channel without altering the currently
 * selected technology. This includes de-registering its event handler for le_dcs channel events.
 */
//--------------------------------------------------------------------------------------------------
static void ResetDataChannel
(
    void
)
{
    DataChannelRef = NULL;
    memset(DataChannelName, '\0', sizeof(DataChannelName));
    if (DataChannelEventHandlerRef)
    {
        le_dcs_RemoveEventHandler(DataChannelEventHandlerRef);
        DataChannelEventHandlerRef = NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Update status of the given technology with the given connection state in the input argument
 */
//--------------------------------------------------------------------------------------------------
static void UpdateTechnologyStatus
(
    le_data_Technology_t technology,    ///< [IN] Affected technology
    bool connected,                     ///< [IN] Connection status
    bool notify                         ///< [IN] Notify apps about latest status or not
)
{
    LE_DEBUG("Technology %d connected status: %d", technology, connected);

    if (notify)
    {
        SendConnStateEvent(connected);
    }

    // Case: connected
    if (connected)
    {
        ResetRetryTechBackoff();
        return;
    }

    // Case: not connected
    if (RequestCount == 0)
    {
        LE_INFO("No need to retry connecting with RequestCount 0");
        if (technology == CurrentTech)
        {
            ResetDataChannel();
        }
        return;
    }

    // Start timer to start the next technology
    if (le_timer_IsRunning(RetryTechTimer))
    {
        LE_INFO("Technology retry timer is already running; wait for next retry");
        return;
    }

    if ((LE_OK != le_timer_SetContextPtr(RetryTechTimer, (void*)((intptr_t)technology))) ||
        (LE_OK != le_timer_Start(RetryTechTimer)))
    {
        LE_ERROR("Failed to start RetryTechTimer to retry connecting");
        if (technology == CurrentTech)
        {
            ResetDataChannel();
        }
        return;
    }

    LE_INFO("Technology retry to connect will happen after %d sec", RetryTechBackoffCurrent);
}


//--------------------------------------------------------------------------------------------------
/**
 * Try to start the default data connection with a defined technology
 */
//--------------------------------------------------------------------------------------------------
static void TryStartTechSession
(
    le_data_Technology_t technology     ///< [IN] Technology to use for the data connection
)
{
    le_result_t ret = dcsTechRank_SelectDataChannel(technology);
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to pick technology %d to start a data connection", technology);
        UpdateTechnologyStatus(technology, false, false);
        return;
    }

    // By here, DataChannelRef, DataChannelName & CurrentTech should have been set
    if (!DataChannelRef)
    {
        LE_ERROR("Found no valid data channel reference to start a data connection");
        return;
    }
    if (technology != CurrentTech)
    {
        LE_ERROR("Failed to start technology %d when the selected one is %d", technology,
                 CurrentTech);
        return;
    }
    if (DataChannelReqRef)
    {
        LE_ERROR("A data connection with request reference %p already connected",
                 DataChannelReqRef);
    }

    if (DataChannelEventHandlerRef)
    {
        le_dcs_RemoveEventHandler(DataChannelEventHandlerRef);
    }
    DataChannelEventHandlerRef = le_dcs_AddEventHandler(DataChannelRef, ChannelEventHandler,
                                                        NULL);
    if (!DataChannelEventHandlerRef)
    {
        LE_ERROR("Failed to add event handler for channel %s of technology %d", DataChannelName,
                 technology);
        UpdateTechnologyStatus(technology, false, false);
        return;
    }
    LE_DEBUG("Data channel event handler %p added", DataChannelEventHandlerRef);

    DataChannelReqRef = le_dcs_Start(DataChannelRef);
    if (!DataChannelReqRef)
    {
        LE_ERROR("Failed to initiate the selected data channel");
        LE_DEBUG("Removing data channel event handler %p", DataChannelEventHandlerRef);
        UpdateTechnologyStatus(technology, false, false);
        ResetDataChannel();
        return;
    }

    LE_INFO("Successfully initiated data channel %s of technology %d", DataChannelName, technology);
    LE_DEBUG("Request reference %p", DataChannelReqRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Try to stop the default data connection using a defined technology
 */
//--------------------------------------------------------------------------------------------------
static void TryStopTechSession
(
    void
)
{
    if (!DataChannelReqRef)
    {
        LE_DEBUG("Found no valid data channel request reference to stop its data connection");
        return;
    }

    if (LE_OK != le_dcs_Stop(DataChannelReqRef))
    {
        LE_ERROR("Failed to stop data channel with request reference %p", DataChannelReqRef);
        SendConnStateEvent(false);
        ResetDataChannel();
    }
    else
    {
        LE_INFO("Successfully initiated stopping active data connection %s of technology %d",
                DataChannelName, CurrentTech);
    }

    DataChannelReqRef = NULL;
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

    if (0 == RequestCount)
    {
        return;
    }

    IncreaseRetryTechBackoff();

    // Retry connecting over the next technology
    TryStartTechSession(dcsTechRank_GetNextTech(technology));
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

    switch (cmdData.command)
    {
    case REQUEST_COMMAND:
        RequestCount++;
        LE_DEBUG("RequestCount %d, IsConnected %d", RequestCount, IsConnected);

        if (!IsConnected)
        {
            // Check if this is the first connection request.
            // If the request count is strictly greater than one, it means that a default
            // data connection was already requested. No need to try and connect again.
            // The connection notification will be sent when DCS retrieves the data connection.
            if (1 == RequestCount)
            {
                // Get the technology to use from the list and start the data connection
                TryStartTechSession(le_data_GetFirstUsedTechnology());
            }
            else
            {
                LE_DEBUG("Selected data channel %s of technology %d in the process of coming up",
                         DataChannelName, CurrentTech);
            }
        }
        else
        {
            // There is already a data connection, so send a connected event so that the new app
            // that just sent the command knows about the current state.  This will also cause
            // redundant info to be sent to the other registered apps, but that's okay
            LE_INFO("Sharing the already connected data channel %s of technology %d",
                    DataChannelName, CurrentTech);
            UpdateTechnologyStatus(CurrentTech, true, true);
        }
        return;
    case RELEASE_COMMAND:
        // Don't decrement below zero, as it will wrap-around
        if (RequestCount > 0)
        {
            RequestCount--;
        }

        if (0 == RequestCount)
        {
            // Try and disconnect the current technology
            TryStopTechSession();
        }
        else
        {
            LE_DEBUG("Skip stopping technology %d as request count is %d", CurrentTech,
                     RequestCount);
        }
        break;
    case START_COMMAND:
        le_dcsTech_Start(cmdData.channelName, cmdData.technology);
        break;
    case STOP_COMMAND:
        le_dcsTech_Stop(cmdData.channelName, cmdData.technology);
        break;
    default:
        LE_ERROR("Command %i is not valid", cmdData.command);
        break;
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
    DcsConnStateData_t* eventDataPtr = reportPtr;
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
 * Change the route on the data connection service interface, if the data connection is connected
 * using the cellular technology and has an IPv4 or IPv6 address.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_BAD_PARAMETER  incorrect IP address
 *      - LE_FAULT          for all other errors
 *
 * @note Limitations:
 *      - only IPv4 is supported for the moment
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ChangeRoute
(
    const char*          ipDestAddrStr,  ///< Destination IP address in dotted format
    pa_dcs_RouteAction_t action          ///< Add or remove the route
)
{
    le_result_t ret;

    // Check if the given address is in IPv4 format
    if (LE_OK != CheckIpAddress(AF_INET, ipDestAddrStr))
    {
        LE_ERROR("Bad address");
        return LE_BAD_PARAMETER;
    }

    if (!DataChannelRef)
    {
        LE_ERROR("Unknown data channel reference for making route change for address %s",
                 ipDestAddrStr);
        return LE_FAULT;
    }

    ret = le_net_ChangeRoute(DataChannelRef, ipDestAddrStr, "", (action == PA_DCS_ROUTE_ADD));
    if (LE_OK != ret)
    {
        LE_ERROR("Failed to %s route for address %s onto data channel %s of technology %d, "
                 "error %d", (action == PA_DCS_ROUTE_ADD) ? "add" : "remove", ipDestAddrStr,
                 DataChannelName, CurrentTech, ret);
    }
    else
    {
        LE_INFO("Succeeded to %s route for address %s onto data channel %s of technology %d",
                (action == PA_DCS_ROUTE_ADD) ? "add" : "remove", ipDestAddrStr,
                DataChannelName, CurrentTech);
    }
    return ret;

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
    le_event_Report(dcs_CommandEventId, &cmdData, sizeof(cmdData));

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
        le_event_Report(dcs_CommandEventId, &cmdData, sizeof(cmdData));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the technology of the currently connected data connection. In the absence of an actively
 * connected data connection, @ref LE_DATA_MAX is returned.
 *
 * @return
 *      - One of the technologies from @ref le_data_Technology_t enumerator
 *      - @ref LE_DATA_MAX if not connected
 */
//--------------------------------------------------------------------------------------------------
le_data_Technology_t le_data_GetTechnology
(
    void
)
{
    // Return the currently connected technology or unknown state if not connected
    return (IsConnected ? CurrentTech : LE_DATA_MAX);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the default route activation status for the data connection service interface set on the
 * config tree at the time of last Legato restart.  Any change in its value there won't be picked
 * up until next restart.
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
 * Add a route on the data connection service interface, if the data connection is connected using
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
 * Delete a route on the data connection service interface, if the data connection is connected
 * using the cellular technology and has an IPv4 or IPv6 address.
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
 * @warning To get the date and time, use GetDateTime rather than sequential calls to GetDate and
 * GetTime to avoid the possibility of a date change between the two calls.
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
    uint16_t hours;
    uint16_t minutes;
    uint16_t seconds;
    uint16_t milliseconds;

    return le_data_GetDateTime(yearPtr, monthPtr, dayPtr,
                               &hours, &minutes, &seconds, &milliseconds);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the time from the configured server using the configured time protocol.
 *
 * @warning To get the date and time, use GetDateTime rather than sequential calls to GetDate and
 * GetTime to avoid the possibility of a date change between the two calls.
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
    uint16_t year;
    uint16_t month;
    uint16_t day;

    return le_data_GetDateTime(&year, &month, &day,
                               hoursPtr, minutesPtr, secondsPtr, millisecondsPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the date and time from the configured server using the configured time protocol.
 *
 * @warning An active data connection is necessary to retrieve the date and time.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_BAD_PARAMETER  if a parameter is incorrect
 *      - LE_FAULT          if an error occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_data_GetDateTime
(
    uint16_t* yearPtr,          ///< [OUT] UTC Year A.D. [e.g. 2017].
    uint16_t* monthPtr,         ///< [OUT] UTC Month into the year [range 1...12].
    uint16_t* dayPtr,           ///< [OUT] UTC Days into the month [range 1...31].
    uint16_t* hoursPtr,         ///< [OUT] UTC Hours into the day [range 0..23].
    uint16_t* minutesPtr,       ///< [OUT] UTC Minutes into the hour [range 0..59].
    uint16_t* secondsPtr,       ///< [OUT] UTC Seconds into the minute [range 0..59].
    uint16_t* millisecondsPtr   ///< [OUT] UTC Milliseconds into the second [range 0..999].
)
{
    pa_dcs_TimeStruct_t timeStruct;

    if ((!yearPtr) || (!monthPtr) || (!dayPtr) ||
        (!hoursPtr) || (!minutesPtr) || (!secondsPtr) || (!millisecondsPtr))
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

    *yearPtr         = (uint16_t) timeStruct.year;
    *monthPtr        = (uint16_t) timeStruct.mon;
    *dayPtr          = (uint16_t) timeStruct.day;
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
    dcs_CommandEventId = le_event_CreateId("Data Command", sizeof(CommandData_t));
    ConnStateEvent = le_event_CreateId("Conn State", sizeof(DcsConnStateData_t));

    // Create safe reference map for request references. The size of the map should be based on
    // the expected number of simultaneous data requests, so take a reasonable guess.
    RequestRefMap = le_ref_CreateMap("Requests", REFERENCE_MAP_SIZE);

    // Set a one-shot timer for requesting the DNS configuration.
    SetDNSConfigTimer = le_timer_Create("SetDNSConfigTimer");
    le_clk_Time_t dnsInterval = {DNS_CONFIG_RETRY_TIMEOUT, 0};

    if (   (LE_OK != le_timer_SetHandler(SetDNSConfigTimer, SetDNSConfigTimerHandler))
        || (LE_OK != le_timer_SetRepeat(SetDNSConfigTimer, 1))    // One shot timer
        || (LE_OK != le_timer_SetInterval(SetDNSConfigTimer, dnsInterval))
       )
    {
        LE_ERROR("Could not start the SetDNSConfig timer!");
    }

    // Retrieve default gateway activation status from config tree
    // Any change in its value there won't be picked up until next Legato restart
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

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler(le_data_GetServiceRef(), CloseSessionEventHandler, NULL);

    dcsTechRank_Init();

    // Register for command events
    le_event_AddHandler("ProcessCommand", dcs_CommandEventId, ProcessCommand);

    // Register main loop with watchdog chain
    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);

    LE_INFO("Data Channel Service le_data is ready");
}
