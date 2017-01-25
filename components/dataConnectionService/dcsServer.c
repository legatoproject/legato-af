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

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The config tree path and node definitions.
 */
//--------------------------------------------------------------------------------------------------
#define DCS_CONFIG_TREE_ROOT_DIR    "dataConnectionService:"
#define CFG_PATH_WIFI               "wifi"
#define CFG_NODE_SSID               "SSID"
#define CFG_NODE_SECPROTOCOL        "secProtocol"
#define CFG_NODE_PASSPHRASE         "passphrase"

//--------------------------------------------------------------------------------------------------
/**
 * The technology string max length
 */
//--------------------------------------------------------------------------------------------------
#define DCS_TECH_LEN      16
#define DCS_TECH_BYTES    (DCS_TECH_LEN+1)

//--------------------------------------------------------------------------------------------------
/**
 * The linux system file to read for default gateway
 */
//--------------------------------------------------------------------------------------------------
#define ROUTE_FILE "/proc/net/route"

//--------------------------------------------------------------------------------------------------
/**
 * Definitions for sending request/release commands to data thread
 */
//--------------------------------------------------------------------------------------------------
#define REQUEST_COMMAND 1
#define RELEASE_COMMAND 2

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
 * Data associated to retrieve the state before the DCS started managing the default connection
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char defaultGateway[LE_MDC_IPV6_ADDR_MAX_BYTES];
    char defaultInterface[20];
    char newDnsIPv4[2][LE_MDC_IPV4_ADDR_MAX_BYTES];
    char newDnsIPv6[2][LE_MDC_IPV6_ADDR_MAX_BYTES];
}
InterfaceDataBackup_t;

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
 * Event for sending command to Process command handler
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t CommandEvent;

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
static le_ref_MapRef_t RequestRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Structure used to store data allowing to restore a functioning state upon disconnection
 */
//--------------------------------------------------------------------------------------------------
static InterfaceDataBackup_t InterfaceDataBackup;

//--------------------------------------------------------------------------------------------------
/**
 * Buffer to store resolv.conf cache
 */
//--------------------------------------------------------------------------------------------------
static char ResolvConfBuffer[256];

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

    // Set the interface name
    switch (CurrentTech)
    {
        case LE_DATA_CELLULAR:
            if (isConnected)
            {
                le_mdc_GetInterfaceName(MobileProfileRef,
                                        eventData.interfaceName,
                                        sizeof(eventData.interfaceName));
            }
            else
            {
                // Initialize to empty string
                eventData.interfaceName[0] = '\0';
            }
            break;

        case LE_DATA_WIFI:
            snprintf(eventData.interfaceName, sizeof(eventData.interfaceName), WIFI_INTF);
            break;

        default:
            eventData.interfaceName[0] = '\0';
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
 * IP Handling to be done once the wifi link is established
 *
 * @return
 *      - LE_OK     Function successful
 *      - LE_FAULT  Function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AskForIpAddress
(
    void
)
{
    int16_t systemResult;
    char tmpString[512];

    // DHCP Client
    snprintf(tmpString,
             sizeof(tmpString),
             "PATH=/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin;"
             "/sbin/udhcpc -R -b -i %s",
             WIFI_INTF
            );

    systemResult = system(tmpString);
    // Return value of -1 means that the fork() has failed (see man system)
    if (0 == WEXITSTATUS(systemResult))
    {
        LE_INFO("DHCP client successful!");
        return LE_OK;
    }
    else
    {
        LE_ERROR("DHCP client failed: command %s, result %d", tmpString, systemResult);
        return LE_FAULT;
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
    LE_DEBUG("Wifi event received");

    switch (event)
    {
        case LE_WIFICLIENT_EVENT_CONNECTED:
            LE_INFO("Wifi client connected");

            // Request an IP address through DHCP if DCS initiated the connection
            // and update connection status
            if ((LE_DATA_WIFI == CurrentTech) && (RequestCount > 0))
            {
                if (LE_OK == AskForIpAddress())
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
            LE_INFO("Wifi client disconnected");

            // Update connection status and send notification to registered applications
            IsConnected = false;
            SendConnStateEvent(IsConnected);

            // Handle new connection status for this technology
            ConnectionStatusHandler(LE_DATA_WIFI, IsConnected);
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
static bool IsApnEmpty
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

    // Update connection status and send notification to registered applications
    IsConnected = (connectionStatus == LE_MDC_CONNECTED) ? true : false;
    SendConnStateEvent(IsConnected);

    // Handle new connection status for this technology
    ConnectionStatusHandler(LE_DATA_CELLULAR, IsConnected);
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
        // TODO: we only try to load the 1st profile stored in MDC database
        case LE_DATA_CELLULAR:
        {
            le_mdc_ProfileRef_t profileRef;

            LE_DEBUG("Use the default cellular profile");
            profileRef = le_mdc_GetProfile(LE_MDC_DEFAULT_PROFILE);

            if (!profileRef)
            {
                LE_ERROR("Default profile not available");
                return LE_FAULT;
            }

            // Updating profileRef
            if (profileRef != MobileProfileRef)
            {
                if (NULL != MobileSessionStateHandlerRef)
                {
                    le_mdc_RemoveSessionStateHandler(MobileSessionStateHandlerRef);
                    MobileSessionStateHandlerRef = NULL;
                }

                MobileProfileRef = profileRef;

                uint32_t profileIndex;
                profileIndex = le_mdc_GetProfileIndex(MobileProfileRef);
                LE_DEBUG("Working with profile %p at index %d", MobileProfileRef, profileIndex);
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

            // Register for data session state changes
            if (NULL == MobileSessionStateHandlerRef)
            {
                MobileSessionStateHandlerRef = le_mdc_AddSessionStateHandler(MobileProfileRef,
                                                                         DataSessionStateHandler,
                                                                         NULL);
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

            // SSID
            if (le_cfg_NodeExists(cfg, CFG_NODE_SSID))
            {
                if (LE_OK != le_cfg_GetString(cfg, CFG_NODE_SSID, Ssid, sizeof(Ssid), "testSsid"))
                {
                    LE_WARN("String value for '%s' too large", CFG_NODE_SSID);
                    le_cfg_CancelTxn(cfg);
                    return LE_OVERFLOW;
                }
                LE_DEBUG("AP configuration, SSID: '%s'", Ssid);
            }
            else
            {
                LE_WARN("No value set for '%s'!", CFG_NODE_SSID);
                le_cfg_CancelTxn(cfg);
                return LE_NOT_FOUND;
            }

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

            le_cfg_CancelTxn(cfg);

            // Create the Access Point to connect to
            AccessPointRef = le_wifiClient_Create((const uint8_t *)Ssid, strlen(Ssid));

            if (NULL != AccessPointRef)
            {
                // Configure the Access Point
                LE_ASSERT(LE_OK == le_wifiClient_SetSecurityProtocol(AccessPointRef, SecProtocol));
                LE_ASSERT(LE_OK == le_wifiClient_SetPassphrase(AccessPointRef, Passphrase));

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
            memset(Ssid, '\0', sizeof(Ssid));
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
 * Check if a default gateway is set
 *
 * @return
 *      True or False
 */
//--------------------------------------------------------------------------------------------------
static bool IsDefaultGatewayPresent
(
    void
)
{
    bool result = false;
    le_result_t openResult;
    FILE *routeFile;
    char line[100] , *ifacePtr , *destPtr, *gwPtr, *saveptr;

    routeFile = le_flock_OpenStream(ROUTE_FILE , LE_FLOCK_READ, &openResult);

    if (NULL == routeFile)
    {
        LE_WARN("le_flock_OpenStream failed with error %d", openResult);
        return result;
    }

    while (fgets(line, sizeof(line), routeFile))
    {
        ifacePtr = strtok_r(line, " \t", &saveptr);
        destPtr  = strtok_r(NULL, " \t", &saveptr);
        gwPtr    = strtok_r(NULL, " \t", &saveptr);

        if ((NULL != ifacePtr) && (NULL != destPtr))
        {
            if (0 == strcmp(destPtr, "00000000"))
            {
                if (gwPtr)
                {
                    result = true;
                    break;
                }
            }
        }
    }

    le_flock_CloseStream(routeFile);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Save the default route
 *
 * @return
 *      - LE_OK           Function succeed
 *      - LE_OVERFLOW     buffer provided are too small
 *      - LE_NOT_FOUND    No default gateway is set
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SaveDefaultGateway
(
    char    *interfacePtr,      ///< [IN] Pointer on the interface name
    size_t   interfaceSize,     ///< [IN] Interface name size
    char    *gatewayPtr,        ///< [IN] Pointer on the gateway name
    size_t   gatewaySize        ///< [IN] Gateway name size
)
{
    le_result_t result;
    FILE *routeFile;
    char line[100] , *ifacePtr , *destPtr, *gwPtr, *saveptr;

    routeFile = le_flock_OpenStream(ROUTE_FILE, LE_FLOCK_READ, &result);

    if (NULL == routeFile)
    {
        return result;
    }

    // Initialize default value
    interfacePtr[0] = '\0';
    gatewayPtr[0]   = '\0';

    result = LE_NOT_FOUND;
    while (fgets(line, sizeof(line), routeFile))
    {
        ifacePtr = strtok_r(line, " \t", &saveptr);
        destPtr  = strtok_r(NULL, " \t", &saveptr);
        gwPtr    = strtok_r(NULL, " \t", &saveptr);

        if ((NULL != ifacePtr) && (NULL != destPtr))
        {
            if (0 == strcmp(destPtr , "00000000"))
            {
                if (gwPtr)
                {
                    char *pEnd;
                    uint32_t ng=strtoul(gwPtr,&pEnd,16);
                    struct in_addr addr;
                    addr.s_addr=ng;

                    result = le_utf8_Copy(interfacePtr, ifacePtr, interfaceSize, NULL);
                    if (result != LE_OK)
                    {
                        LE_WARN("inferface buffer is too small");
                        break;
                    }

                    result = le_utf8_Copy(gatewayPtr, inet_ntoa(addr), gatewaySize, NULL);
                    if (result != LE_OK)
                    {
                        LE_WARN("gateway buffer is too small");
                        break;
                    }
                }
                break;
            }
        }
    }

    le_flock_CloseStream(routeFile);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the default gateway in the system
 *
 * return
 *      LE_OK           Function succeed
 *      LE_FAULT        Function failed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetDefaultGateway
(
    const char *interfacePtr,   ///< [IN] Pointer on the interface name
    const char *gatewayPtr,     ///< [IN] Pointer on the gateway name
    bool isIpv6                 ///< [IN] IPv6 or not
)
{
    const char *optionPtr = "";
    char systemCmd[200] = {0};

    if ((0 == strcmp(gatewayPtr,"")) || (0 == strcmp(interfacePtr,"")))
    {
        LE_WARN("Default gateway or interface is empty");
        return LE_FAULT;
    }
    else
    {
        LE_DEBUG("Try set the gateway %s on %s", gatewayPtr, interfacePtr);
    }

    if (IsDefaultGatewayPresent())
    {
        // Remove the last default GW
        LE_DEBUG("Execute '/sbin/route del default'");
        if (-1 == system("/sbin/route del default"))
        {
            LE_WARN("system '%s' failed", systemCmd);
            return LE_FAULT;
        }
    }

    if (isIpv6)
    {
        optionPtr = "-A inet6";
    }

    // TODO: use of ioctl instead, should be done when rework the DCS
    snprintf(systemCmd, sizeof(systemCmd),
             "/sbin/route %s add default gw %s %s", optionPtr, gatewayPtr, interfacePtr);

    LE_DEBUG("Execute '%s", systemCmd);
    if (-1 == system(systemCmd))
    {
        LE_WARN("system '%s' failed", systemCmd);
        return LE_FAULT;
    }

    return LE_OK;
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
    le_result_t result = SetDefaultGateway(InterfaceDataBackup.defaultInterface,
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
        LE_WARN("Profile is not using IPv4 nor IPv6");
        return LE_FAULT;
    }


    if (le_mdc_IsIPv6(profileRef))
    {

        if (LE_OK != le_mdc_GetIPv6GatewayAddress(profileRef,
                                                  ipv6GatewayAddr,
                                                  sizeof(ipv6GatewayAddr)))
        {
            LE_INFO("le_mdc_GetIPv6GatewayAddress failed");
            return LE_FAULT;
        }

        if (LE_OK != le_mdc_GetInterfaceName(profileRef, interface, sizeof(interface)))
        {
            LE_WARN("le_mdc_GetInterfaceName failed");
            return LE_FAULT;
        }

        // Set the default ipv6 gateway retrieved from modem
        if (LE_OK != SetDefaultGateway(interface, ipv6GatewayAddr, isIpv6))
        {
            LE_WARN("SetDefaultGateway for ipv6 gateway failed");
            return LE_FAULT;
        }
    }

    if (le_mdc_IsIPv4(profileRef))
    {

        if (LE_OK != le_mdc_GetIPv4GatewayAddress(profileRef,
                                                  ipv4GatewayAddr,
                                                  sizeof(ipv4GatewayAddr)))
        {
            LE_INFO("le_mdc_GetIPv4GatewayAddress failed");
            return LE_FAULT;
        }

        if (LE_OK != le_mdc_GetInterfaceName(profileRef, interface, sizeof(interface)))
        {
            LE_WARN("le_mdc_GetInterfaceName failed");
            return LE_FAULT;
        }

        // Set the default ipv4 gateway retrieved from modem
        if (LE_OK != SetDefaultGateway(interface, ipv4GatewayAddr, !(isIpv6)))
        {
            LE_WARN("SetDefaultGateway for ipv4 gateway failed");
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read DNS configuration from /etc/resolv.conf
 *
 * @return File content in a statically allocated string (shouldn't be freed)
 */
//--------------------------------------------------------------------------------------------------
static char * ReadResolvConf
(
    void
)
{
    int fd;
    char * fileContent = NULL;
    size_t fileSz;

    fd = open("/etc/resolv.conf", O_RDONLY);
    if (fd < 0)
    {
        LE_WARN("fopen on /etc/resolv.conf failed");
        return NULL;
    }

    fileSz = lseek(fd, 0, SEEK_END);
    LE_FATAL_IF( (fileSz < 0), "Unable to get resolv.conf size" );

    if (0 != fileSz)
    {

        LE_DEBUG("Caching resolv.conf: size[%zu]", fileSz);

        lseek(fd, 0, SEEK_SET);

        if (fileSz > (sizeof(ResolvConfBuffer) - 1))
        {
            LE_ERROR("Buffer is too small (%zu), file will be truncated from %zu",
                    sizeof(ResolvConfBuffer), fileSz);
            fileSz = sizeof(ResolvConfBuffer) - 1;
        }

        fileContent = ResolvConfBuffer;

        if (0 > read(fd, fileContent, fileSz))
        {
            LE_ERROR("Caching resolv.conf failed");
            fileContent[0] = '\0';
            fileSz = 0;
        }
        else
        {
            fileContent[fileSz] = '\0';
        }
    }

    if (0 != close(fd))
    {
        LE_FATAL("close failed");
    }

    LE_FATAL_IF( fileContent && (strlen(fileContent) > fileSz),
                 "Content size (%zu) and File size (%zu) differ",
                 strlen(fileContent), fileSz );

    return fileContent;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the DNS configuration into /etc/resolv.conf
 *
 * @return
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AddNameserversToResolvConf
(
    const char *dns1Ptr,    ///< [IN] Pointer on first DNS address
    const char *dns2Ptr     ///< [IN] Pointer on second DNS address
)
{
    bool addDns1 = true;
    bool addDns2 = true;

    LE_INFO("Set DNS '%s' '%s'", dns1Ptr, dns2Ptr);

    addDns1 = ('\0' != dns1Ptr[0]);
    addDns2 = ('\0' != dns2Ptr[0]);

    // Look for entries to add in the existing file
    char* resolvConfSourcePtr = ReadResolvConf();

    if (NULL != resolvConfSourcePtr)
    {
        char* currentLinePtr = resolvConfSourcePtr;
        int currentLinePos = 0;

        // For each line in source file
        while (true)
        {
            if (   ('\0' == currentLinePtr[currentLinePos])
                || ('\n' == currentLinePtr[currentLinePos])
               )
            {
                char sourceLineEnd = currentLinePtr[currentLinePos];
                currentLinePtr[currentLinePos] = '\0';

                if (NULL != strstr(currentLinePtr, dns1Ptr))
                {
                    LE_DEBUG("DNS 1 '%s' found in file", dns1Ptr);
                    addDns1 = false;
                }
                else if (NULL != strstr(currentLinePtr, dns2Ptr))
                {
                    LE_DEBUG("DNS 2 '%s' found in file", dns2Ptr);
                    addDns2 = false;
                }

                if ('\0' == sourceLineEnd)
                {
                    break;
                }
                else
                {
                    currentLinePtr[currentLinePos] = sourceLineEnd;
                    currentLinePtr += (currentLinePos+1); // Next line
                    currentLinePos = 0;
                }
            }
            else
            {
                currentLinePos++;
            }
        }
    }

    if (!addDns1 && !addDns2)
    {
        // No need to change the file
        return LE_OK;
    }

    FILE*  resolvConfPtr;
    mode_t oldMask;

    // allow fopen to create file with mode=644
    oldMask = umask(022);

    resolvConfPtr = fopen("/etc/resolv.conf", "w");
    if (NULL == resolvConfPtr)
    {
        // restore old mask
        umask(oldMask);

        LE_WARN("fopen on /etc/resolv.conf failed");
        return LE_FAULT;
    }

    // Set DNS 1 if needed
    if (addDns1 && (fprintf(resolvConfPtr, "nameserver %s\n", dns1Ptr) < 0))
    {
        // restore old mask
        umask(oldMask);

        LE_WARN("fprintf failed");
        if (0 != fclose(resolvConfPtr))
        {
            LE_WARN("fclose failed");
        }
        return LE_FAULT;
    }

    // Set DNS 2 if needed
    if (addDns2 && (fprintf(resolvConfPtr, "nameserver %s\n", dns2Ptr) < 0))
    {
        // restore old mask
        umask(oldMask);

        LE_WARN("fprintf failed");
        if (0 != fclose(resolvConfPtr))
        {
            LE_WARN("fclose failed");
        }
        return LE_FAULT;
    }

    // Append rest of the file
    if (NULL != resolvConfSourcePtr)
    {
        size_t writeLen = strlen(resolvConfSourcePtr);

        if (writeLen != fwrite(resolvConfSourcePtr, sizeof(char), writeLen, resolvConfPtr))
        {
            // restore old mask
            umask(oldMask);

            LE_CRIT("Writing resolv.conf failed");
            if (0 != fclose(resolvConfPtr))
            {
                LE_WARN("fclose failed");
            }
            return LE_FAULT;
        }
    }

    // restore old mask
    umask(oldMask);

    if (0 != fclose(resolvConfPtr))
    {
        LE_WARN("fclose failed");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove the DNS configuration from /etc/resolv.conf
 *
 * @return
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t RemoveNameserversFromResolvConf
(
    const char *dns1Ptr,    ///< [IN] Pointer on first DNS address
    const char *dns2Ptr     ///< [IN] Pointer on second DNS address
)
{
    char* resolvConfSourcePtr = ReadResolvConf();
    char* currentLinePtr = resolvConfSourcePtr;
    int currentLinePos = 0;

    FILE*  resolvConfPtr;
    mode_t oldMask;

    if (NULL == resolvConfSourcePtr)
    {
        // Nothing to remove
        return LE_OK;
    }

    // allow fopen to create file with mode=644
    oldMask = umask(022);

    resolvConfPtr = fopen("/etc/resolv.conf", "w");
    if (NULL == resolvConfPtr)
    {
        // restore old mask
        umask(oldMask);

        LE_WARN("fopen on /etc/resolv.conf failed");
        return LE_FAULT;
    }

    // For each line in source file
    while (true)
    {
        if (   ('\0' == currentLinePtr[currentLinePos])
            || ('\n' == currentLinePtr[currentLinePos])
           )
        {
            char sourceLineEnd = currentLinePtr[currentLinePos];
            currentLinePtr[currentLinePos] = '\0';

            // Got to the end of the source file
            if ('\0' == (sourceLineEnd) && (0 == currentLinePos))
            {
                break;
            }

            // If line doesn't contains an entry to remove,
            // copy line to new content
            if (   (NULL == strstr(currentLinePtr, dns1Ptr))
                && (NULL == strstr(currentLinePtr, dns2Ptr))
               )
            {
                // The original file contents may not have the final line terminated by
                // a new-line; always terminate with a new-line, since this is what is
                // usually expected on linux.
                currentLinePtr[currentLinePos] = '\n';
                fwrite(currentLinePtr, sizeof(char), (currentLinePos+1), resolvConfPtr);
            }

            if ('\0' == sourceLineEnd)
            {
                // This should only occur if the last line was not terminated by a new-line.
                break;
            }
            else
            {
                currentLinePtr += (currentLinePos+1); // Next line
                currentLinePos = 0;
            }
        }
        else
        {
            currentLinePos++;
        }
    }

    // restore old mask
    umask(oldMask);

    if (0 != fclose(resolvConfPtr))
    {
        LE_WARN("fclose failed");
        return LE_FAULT;
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
    le_mdc_ProfileRef_t profileRef      ///< [IN] Modem data connection profile reference
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
            LE_INFO("IPv4: le_mdc_GetDNSAddresses failed");
            return LE_FAULT;
        }

        if (LE_OK != AddNameserversToResolvConf(dns1Addr, dns2Addr))
        {
            LE_INFO("IPv4: Could not write in resolv file");
            return LE_FAULT;
        }

        strcpy(InterfaceDataBackup.newDnsIPv4[0], dns1Addr);
        strcpy(InterfaceDataBackup.newDnsIPv4[1], dns2Addr);
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
            LE_INFO("IPv6: le_mdc_GetDNSAddresses failed");
            return LE_FAULT;
        }

        if (LE_OK != AddNameserversToResolvConf(dns1Addr, dns2Addr))
        {
            LE_INFO("IPv6: Could not write in resolv file");
            return LE_FAULT;
        }

        strcpy(InterfaceDataBackup.newDnsIPv6[0], dns1Addr);
        strcpy(InterfaceDataBackup.newDnsIPv6[1], dns2Addr);
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
 * Set the default gateway retrieved from modem
 *
 * @return
 *      LE_FAULT        Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetModemGateway
(
    void
)
{
    // Save gateway configuration
    if (LE_OK != SaveDefaultGateway(InterfaceDataBackup.defaultInterface,
                                    sizeof(InterfaceDataBackup.defaultInterface),
                                    InterfaceDataBackup.defaultGateway,
                                    sizeof(InterfaceDataBackup.defaultGateway)))
    {
        LE_WARN("Could not save the default gateway");
    }
    else
    {
        LE_DEBUG("default gw is: '%s' on '%s'", InterfaceDataBackup.defaultGateway,
                                                InterfaceDataBackup.defaultInterface);
    }

    if (LE_OK != SetRouteConfiguration(MobileProfileRef))
    {
        LE_ERROR("Failed to get configuration route");
        return LE_FAULT;
    }

    if (LE_OK != SetDnsConfiguration(MobileProfileRef))
    {
        LE_ERROR("Failed to get configuration DNS");
        return LE_FAULT;
    }

    return LE_OK;
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
    // Start data session
    if (LE_OK != le_mdc_StartSession(MobileProfileRef))
    {
        // Impossible to use this technology, try the next one
        ConnectionStatusHandler(LE_DATA_CELLULAR, false);
    }
    else
    {
        // First wait a few seconds for the default DHCP client
        sleep(3);

        // Set the gateway retrieved from the modem
        if (LE_OK != SetModemGateway())
        {
            // Impossible to use this technology, try the next one
            ConnectionStatusHandler(LE_DATA_CELLULAR, false);
        }
        else
        {
            // Wait a few seconds to prevent rapid toggling of data connection
            sleep(5);
        }
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

        return;
    }

    // Start Wifi client
    result = le_wifiClient_Start();

    if (LE_OK != result)
    {
        LE_ERROR("Wifi client not started, result %d (%s)", result, LE_RESULT_TXT(result));

        // Impossible to use this technology, try the next one
        ConnectionStatusHandler(LE_DATA_WIFI, false);

        return;
    }

    LE_INFO("Wifi client started");

    // Check if the Access Point is created
    if (NULL == AccessPointRef)
    {
        LE_ERROR("No reference to AP");

        // Impossible to use this technology, try the next one
        ConnectionStatusHandler(LE_DATA_WIFI, false);

        return;
    }

    // Connect to the Access Point
    result = le_wifiClient_Connect(AccessPointRef);
    if (LE_OK != result)
    {
        LE_ERROR("Impossible to connect to AP, result %d (%s)", result, LE_RESULT_TXT(result));

        // Impossible to use this technology, try the next one
        ConnectionStatusHandler(LE_DATA_WIFI, false);

        return;
    }

    LE_INFO("Connecting to AP");
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
    if (LE_DATA_MAX == technology)
    {
        LE_ERROR("Unknown technology used to start the data session!");
    }
    else
    {
        char techStr[DCS_TECH_BYTES] = {0};
        GetTechnologyString(technology, techStr, sizeof(techStr));
        LE_DEBUG("Technology used for the data connection: '%s'", techStr);

        // Store the currently used technology
        CurrentTech = technology;

        switch (technology)
        {
            case LE_DATA_CELLULAR:
            {
                // Load MobileProfileRef
                le_result_t result = LoadSelectedTechProfile(LE_DATA_CELLULAR);
                if (LE_OK == result)
                {
                    // Ensure that cellular network service is available.
                    // Data connection will be started when cellular network registration
                    // notification is received.
                    le_cellnet_Request();
                }
                else
                {
                    LE_WARN("Impossible to use Cellular profile, error %d (%s)",
                            result, LE_RESULT_TXT(result));

                    // Impossible to use this technology, try the next one
                    ConnectionStatusHandler(LE_DATA_CELLULAR, false);
                }
            }
            break;

            case LE_DATA_WIFI:
                // Try to establish the wifi connection
                TryStartWifiSession();
                break;

            default:
                LE_ERROR("Unknown technology %d to start", technology);
                break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Used the data backup upon connection to remove DNS entries locally added
 */
//--------------------------------------------------------------------------------------------------
static void RestoreInitialNameservers
(
    void
)
{
    if (   ('\0' != InterfaceDataBackup.newDnsIPv4[0][0])
        || ('\0' != InterfaceDataBackup.newDnsIPv4[1][0])
       )
    {
        RemoveNameserversFromResolvConf(InterfaceDataBackup.newDnsIPv4[0],
                                        InterfaceDataBackup.newDnsIPv4[1]);

        // Delete backed up data
        memset(InterfaceDataBackup.newDnsIPv4[0], '\0',
               sizeof(InterfaceDataBackup.newDnsIPv4[0]));
        memset(InterfaceDataBackup.newDnsIPv4[1], '\0',
               sizeof(InterfaceDataBackup.newDnsIPv4[1]));
    }

    if (   ('\0' != InterfaceDataBackup.newDnsIPv6[0][0])
        || ('\0' != InterfaceDataBackup.newDnsIPv6[1][0])
       )
    {
        RemoveNameserversFromResolvConf(InterfaceDataBackup.newDnsIPv6[0],
                                        InterfaceDataBackup.newDnsIPv6[1]);

        // Delete backed up data
        memset(InterfaceDataBackup.newDnsIPv6[0], '\0',
               sizeof(InterfaceDataBackup.newDnsIPv6[0]));
        memset(InterfaceDataBackup.newDnsIPv6[1], '\0',
               sizeof(InterfaceDataBackup.newDnsIPv6[1]));
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

    // Check if the mobile data session is already disconnected
    result = le_mdc_GetSessionState(MobileProfileRef, &sessionState);
    if ((LE_OK == result) && (!sessionState))
    {
        IsConnected = false;

        // Reset number of retries
        StopSessionRetries = 0;

        // Restore backed up parameters
        RestoreDefaultGateway();
        RestoreInitialNameservers();
    }
    else
    {
        // Try to shutdown the connection anyway
        if (LE_OK != le_mdc_StopSession(MobileProfileRef))
        {
            LE_ERROR("Impossible to stop mobile data session");

            // Increase number of retries
            StopSessionRetries++;

            // Start timer
            if (!le_timer_IsRunning(timerRef))
            {
                if (LE_OK != le_timer_Start(timerRef))
                {
                    LE_ERROR("Could not start the StopDcs timer!");
                }
            }
        }
        else
        {
            IsConnected = false;

            // Reset number of retries
            StopSessionRetries = 0;

            // Restore backed up parameters
            RestoreDefaultGateway();
            RestoreInitialNameservers();
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
    if (LE_OK != le_wifiClient_Disconnect())
    {
        LE_ERROR("Impossible to disconnect wifi client");

        // Increase number of retries
        StopSessionRetries++;

        // Start timer
        if (!le_timer_IsRunning(timerRef))
        {
            if (LE_OK != le_timer_Start(timerRef))
            {
                LE_ERROR("Could not start the StopDcs timer!");
            }
        }
    }
    else
    {
        IsConnected = false;
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
    if (0 != RequestCount)
    {
        // Request has been requested in the meantime, the Release command process
        // can be interrupted

        // Reset number of retries
        StopSessionRetries = 0;
    }
    else
    {
        switch (CurrentTech)
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
                    TryStopWifiSession(timerRef);
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
 * Handler to process a command
 */
//--------------------------------------------------------------------------------------------------
static void ProcessCommand
(
    void* reportPtr     ///< [IN] Command to process
)
{
    uint32_t command = *(uint32_t*)reportPtr;

    LE_PRINT_VALUE("%i", command);

    if (REQUEST_COMMAND == command)
    {
        RequestCount++;

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
    else if (RELEASE_COMMAND == command)
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
    }
    else
    {
        LE_ERROR("Command %i is not valid", command);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Event callback for Cellular Network Service state changes
 */
//--------------------------------------------------------------------------------------------------
static void CellNetStateHandler
(
    le_cellnet_State_t state,       ///< [IN] Cellular network state
    void*              contextPtr   ///< [IN] Associated context pointer
)
{
    LE_DEBUG("Cellular Network Service is in state %d", state);

    switch (state)
    {
        case LE_CELLNET_RADIO_OFF:
        case LE_CELLNET_REG_EMERGENCY:
        case LE_CELLNET_REG_UNKNOWN:
        case LE_CELLNET_SIM_ABSENT:
            break;

        case LE_CELLNET_REG_HOME:
        case LE_CELLNET_REG_ROAMING:
            // Check if the mobile data session should be started
            if ((LE_DATA_CELLULAR == CurrentTech) && (RequestCount > 0) && (!IsConnected))
            {
                TryStartDataSession();
            }
            break;
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
    // Check if the default data connection is still necessary
    if ((false == connected) && (RequestCount > 0))
    {
        // Disconnect the current technology which is not available anymore
        TryStopTechSession(CurrentTech);

        // Connect the next technology to use
        TryStartTechSession(GetNextTech(CurrentTech));
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
    uint32_t command = REQUEST_COMMAND;
    le_event_Report(CommandEvent, &command, sizeof(command));

    // Need to return a unique reference that will be used by Release.  Don't actually have
    // any data for now, but have to use some value other than NULL for the data pointer.
    le_data_RequestObjRef_t reqRef = le_ref_CreateRef(RequestRefMap,
                                                     (void*)le_data_GetClientSessionRef());

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

        uint32_t command = RELEASE_COMMAND;
        le_event_Report(CommandEvent, &command, sizeof(command));
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
 *  Server initialization
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Initialize the various events
    CommandEvent = le_event_CreateId("Data Command", sizeof(uint32_t));
    ConnStateEvent = le_event_CreateId("Conn State", sizeof(ConnStateData_t));

    // Create memory pool and expand it
    TechListPoolRef = le_mem_CreatePool("Technologies list pool", sizeof(TechRecord_t));
    le_mem_ExpandPool(TechListPoolRef, DCS_TECH_NUMBER);

    // Create safe reference map for request references. The size of the map should be based on
    // the expected number of simultaneous data requests, so take a reasonable guess.
    RequestRefMap = le_ref_CreateMap("Requests", 5);

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

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler(le_data_GetServiceRef(),
                                  CloseSessionEventHandler,
                                  NULL);

    // Services required by DCS

    // 1. Mobile services
    // Mobile services are always available
    TechAvailability[LE_DATA_CELLULAR] = true;

    // Register for Cellular Network Service state changes
    le_cellnet_AddStateEventHandler(CellNetStateHandler, NULL);

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

    // Register for command events
    le_event_AddHandler("ProcessCommand", CommandEvent, ProcessCommand);

    LE_INFO("Data Connection Service is ready");
}
