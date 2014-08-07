//--------------------------------------------------------------------------------------------------
/**
 *  Data Connection Server
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 *
 * The Data Connection Service (DCS) only supports in this version the 'Mobile' technology, so the
 * data connection is based on the Modem Data Control service (MDC).
 *
 * When a REQUEST command is received, the DCS first sends a REQUEST command to the Cellular Network
 * Service in order to ensure that there is a valid SIM and the modem is registered on the network.
 * The Data session is actually started when the Cellular Network Service State is 'ROAMING' or
 * 'HOME'.
 *
 *
 * todo:
 *  - assumes that DHCP client will always succeed; this is not always the case
 *  - only handles the default data connection on mobile network
 *  - has a very simple recovery mechanism after data connection is lost; this needs improvement.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"

#include "interfaces.h"
#include "le_cfg_interface.h"
#include "mdmCfgEntries.h"

#include "le_print.h"

#include "jansson.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * The config tree path and node definitions.
 */
//--------------------------------------------------------------------------------------------------
#define CFG_NODE_PREF_TECH      "prefTech"
#define CFG_DCS_PATH            "/dataConnectionService"

//--------------------------------------------------------------------------------------------------
/**
 * The preferred technology string max length.
 */
//--------------------------------------------------------------------------------------------------
#define DCS_TECH_LEN      16
#define DCS_TECH_BYTES    (DCS_TECH_LEN+1)

//--------------------------------------------------------------------------------------------------
/**
 * The file to read for the APN.
 */
//--------------------------------------------------------------------------------------------------
#define APN_FILE "/opt/legato/apps/dataConnectionService/usr/local/share/apns.json"
// @TODO change the APN file when dataConnectionService become a sandboxed app.
//#define APN_FILE "/usr/local/share/apns.json"


//--------------------------------------------------------------------------------------------------
/**
 * Definitions for sending request/release commands to data thread
 */
//--------------------------------------------------------------------------------------------------
#define REQUEST_COMMAND 1
#define RELEASE_COMMAND 2


//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
/**
 * Data associated with the above ConnStateEvent.
 *
 * interfaceName is only valid if isConnected is true.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool isConnected;
    char interfaceName[100+1];
}
ConnStateData_t;

//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Timer references
 */
//--------------------------------------------------------------------------------------------------
static le_timer_Ref_t StartDcsTimer = NULL;
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
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mdc_ProfileRef_t MobileProfileRef = NULL;

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
 * Safe Reference Map for the request reference
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t RequestRefMap;

// -------------------------------------------------------------------------------------------------
/**
 *  This function will know if the APN name for profileName is empty.
 *
 *  @return true, false.
 */
// -------------------------------------------------------------------------------------------------
static bool IsApnEmpty
(
    le_mdc_ProfileRef_t profileRef
)
{
    char apnName[LIMIT_MAX_PATH_BYTES] = {0};

    if ( le_mdc_GetAPN(profileRef,apnName,sizeof(apnName)) != LE_OK)
    {
        LE_WARN("APN was truncated");
        return false;
    }

    return (strcmp(apnName,"")==0);
}

// -------------------------------------------------------------------------------------------------
/**
 *  This function will attempt to read apn definition for mcc/mnc in file apnFilePtr
 *
 * @return LE_OK    function succeed
 * @return LE_FAULT function failed
 */
// -------------------------------------------------------------------------------------------------
static le_result_t FindApnFromFile
(
    const char* apnFilePtr, ///< [IN] apn file
    const char* mccPtr,     ///< [IN] mcc
    const char* mncPtr,     ///< [IN] mnc
    char * mccMncApnPtr,    ///< [OUT] apn for mcc/mnc
    size_t mccMncApnSize    ///< [IN] size of mccMncApn buffer
)
{
    le_result_t result = LE_FAULT;
    json_t *root, *apns, *apnArray;
    json_error_t error;
    int i;

    root = json_load_file(apnFilePtr, 0, &error);
    if (root == NULL ) {
        LE_WARN("Document not parsed successfully. \n");
        return result;
    }

    apns = json_object_get( root, "apns" );
    if ( !json_is_object(apns) )
    {
        LE_WARN("apns is not an object\n");
        json_decref(root);
        return result;
    }

    apnArray = json_object_get( apns, "apn" );
    if ( !json_is_array(apnArray) )
    {
        LE_WARN("apns is not an array\n");
        json_decref(root);
        return result;
    }

    for( i = 0; i < json_array_size(apnArray); i++ )
    {
        json_t *data, *mcc, *mnc, *apn;
        const char* mccRead;
        const char* mncRead;
        const char* apnRead;
        data = json_array_get( apnArray, i );
        if ( !json_is_object( data ))
        {
            LE_WARN("data %d is not an object",i);
            break;
        }

        mcc = json_object_get( data, "@mcc" );
        mccRead = json_string_value(mcc);

        mnc = json_object_get( data, "@mnc" );
        mncRead = json_string_value(mnc);

        if ( !(strcmp(mccRead,mccPtr))
            &&
             !(strcmp(mncRead,mncPtr))
           )
        {
            apn = json_object_get( data, "@apn" );
            apnRead = json_string_value(apn);

            if ( le_utf8_Copy(mccMncApnPtr,apnRead,mccMncApnSize,NULL) != LE_OK)
            {
                LE_WARN("Apn buffer is too small");
                break;
            }
            LE_INFO("[%s:%s] Got APN '%s'", mccPtr, mncPtr, mccMncApnPtr);
            // @note Stop on the first json entry for mcc/mnc, need to be improved?
            result = LE_OK;
            break;
        }
    }

    json_decref(root);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to extrat mcc/mnc from the IMSI code
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the mcc or mnc would not fit in buffer
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ExtractMccMncFromImsi
(
    const char  *imsiPtr,       ///< [IN] IMSI to parse
    char        *mccPtr,        ///< [OUT] Mobile Country Code
    size_t       mccPtrSize,    ///< [IN] mccPtr buffer size
    char        *mncPtr,        ///< [OUT] Mobile Network Code
    size_t       mncPtrSize     ///< [IN] mncPtr buffer size
)
{

    if ((mccPtrSize < LE_MRC_MCC_BYTES) || (mncPtrSize < LE_MRC_MNC_BYTES))
    {
        return LE_OVERFLOW;
    }
    // Copy mcc
    memcpy(mccPtr,imsiPtr,3);
    mccPtr[LE_MRC_MCC_LEN] = '\0';

    uint32_t mcc = atoi(mccPtr);
    if ( ( mcc >= 310 ) && ( mcc <= 316 ))
    {
        memcpy(mncPtr,&imsiPtr[3],3);
        mncPtr[LE_MRC_MNC_LEN] = '\0';
    }
    else
    {
        memcpy(mncPtr,&imsiPtr[3],2);
        mncPtr[LE_MRC_MNC_LEN-1] = '\0';
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the APN for the profile by reading the APN file.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetApnFromFile
(
    le_mdc_ProfileRef_t profileRef
)
{
    le_sim_ObjRef_t sim1Ref;

    char imsi[LE_SIM_IMSI_LEN] = {0};
    char mcc[LE_MRC_MCC_BYTES];
    char mnc[LE_MRC_MNC_BYTES];
    char mccMncApn[100+1] = {0};

    // Hard coded, supposed we use the sim-1, need to be improved ?
    sim1Ref = le_sim_Create(1);
    if ( sim1Ref == NULL)
    {
        LE_WARN("Could not create the sim-1");
        return LE_FAULT;
    }

    // Get the IMSI
    if ( le_sim_GetIMSI(sim1Ref,imsi,sizeof(imsi)) != LE_OK )
    {
        LE_WARN("Could not get the IMSI for sim-1");
        return LE_FAULT;
    }
    le_sim_Delete(sim1Ref);

    // Extract [MCC/MNC] from IMSI
    if ( ExtractMccMncFromImsi(imsi,mcc,sizeof(mcc),mnc,sizeof(mnc)) != LE_OK )
    {
        LE_WARN("Could not get the MCC/MNC from IMSI");
        return LE_FAULT;
    }

    LE_DEBUG("Search of [%s:%s] into file %s",mcc,mnc,APN_FILE);

    // Find APN value for [MCC/MNC]
    if ( FindApnFromFile(APN_FILE,mcc,mnc,mccMncApn,sizeof(mccMncApn)) != LE_OK )
    {
        LE_WARN("Could not find %s/%s in %s",mcc,mnc,APN_FILE);
        return LE_FAULT;
    }

    // Save the APN value into the modem
    return le_mdc_SetAPN(profileRef,mccMncApn);
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the profile of the selected technology (retrieved from the config tree)
 */
//--------------------------------------------------------------------------------------------------
static void LoadSelectedTechProfile
(
    const char* techPtr
)
{
    // TODO: only Cellular technology is supported and we try to load the 1st profile stored in MDC
    // database
    if (!strncmp(techPtr, "cellular", DCS_TECH_BYTES))
    {
        uint32_t profileIndex = 1;

        LE_DEBUG("Try to use the first profile in the modem");
        le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile(profileIndex);

        if ( profileRef == NULL )
        {
            LE_CRIT("Profile index %d is not available", profileIndex);
            return;
        }

        // MobileProfileRef is now referencing the default profile to use for data connection
        if ( IsApnEmpty(profileRef) )
        {
            LE_INFO("Search for APN in the database");
            if ( SetApnFromFile(profileRef) != LE_OK )
            {
                LE_ERROR("Could not set APN from file");
                return;
            }
        }

        // load the profile
        MobileProfileRef = profileRef;
        LE_DEBUG("profile at index %d is successfully loaded.", profileIndex);
    }
    else
    {
        LE_FATAL("Only 'cellular' technology is supported");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Load preferences from the configuration tree
 */
//--------------------------------------------------------------------------------------------------
static void LoadPreferencesFromConfigDb
(
    void
)
{
    char configPath[LIMIT_MAX_PATH_BYTES];
    char techStr[DCS_TECH_BYTES] = {0};

    snprintf(configPath, sizeof(configPath), "%s/%s", CFG_DCS_PATH, CFG_NODE_PREF_TECH);

    LE_DEBUG("Start reading DCS information in ConfigDB");

    le_cfg_IteratorRef_t cfg = le_cfg_CreateReadTxn(configPath);

    if ( le_cfg_GetString(cfg, CFG_NODE_PREF_TECH, techStr, sizeof(techStr), "cellular") != LE_OK )
    {
        LE_WARN("String value for '%s' too large." ,CFG_NODE_PREF_TECH);
        le_cfg_CancelTxn(cfg);
        return;
    }

    if ( strncmp(techStr, "", sizeof(techStr)) == 0 )
    {
        LE_WARN("No node value set for '%s'", CFG_NODE_PREF_TECH);
        le_cfg_CancelTxn(cfg);
        return;
    }

    le_cfg_CancelTxn(cfg);

    LE_DEBUG("%s is the preferred technology for data connection.", techStr);
    LoadSelectedTechProfile(techStr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the default route for a profile
 *
 * return
 *      LE_NOT_POSSIBLE Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetRouteConfiguration
(
    le_mdc_ProfileRef_t profileRef
)
{
    const char *optionPtr;
    char gatewayAddr[100] = {0};
    char interface[100] = {0};
    char systemCmd[200] = {0};
    le_result_t (*getGatewayFunction)(le_mdc_ProfileRef_t,char*,size_t) = NULL;

    if ( le_mdc_IsIPv6(profileRef) ) {
        optionPtr = "-A inet6";
        getGatewayFunction = le_mdc_GetIPv6GatewayAddress;
    }
    else if ( le_mdc_IsIPv4(profileRef) )
    {
        optionPtr = "";
        getGatewayFunction = le_mdc_GetIPv4GatewayAddress;
    }
    else
    {
        LE_WARN("Profile is not using IPv4 nor IPv6");
        return LE_NOT_POSSIBLE;
    }

    if ( getGatewayFunction &&
         (*getGatewayFunction)(profileRef, gatewayAddr, sizeof(gatewayAddr)) != LE_OK )
    {
        LE_INFO("le_mdc_GetGatewayAddress failed");
        return LE_NOT_POSSIBLE;
    }

    if ( le_mdc_GetInterfaceName(profileRef, interface, sizeof(interface)) != LE_OK )
    {
        LE_INFO("le_mdc_GetInterfaceName failed");
        return LE_NOT_POSSIBLE;
    }

    LE_DEBUG("set the gateway %s for interface %s",gatewayAddr, interface);
    // Remove the last default GW.
    LE_DEBUG("Execute 'route del default'");
    if ( system("/sbin/route del default") == -1 )
    {
        LE_WARN("system '%s' failed", systemCmd);
        return LE_NOT_POSSIBLE;
    }

    // @TODO use of ioctl instead, should be done when rework the DCS.
    snprintf(systemCmd,
             sizeof(systemCmd),
             "/sbin/route %s add default gw %s %s",
             optionPtr,
             gatewayAddr,
             interface);

    LE_DEBUG("Execute '%s",systemCmd);
    if ( system(systemCmd) == -1 )
    {
        LE_WARN("system '%s' failed", systemCmd);
        return LE_NOT_POSSIBLE;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Write the DNS configuration into /etc/resolv.cof
 *
 * return
 *      LE_NOT_POSSIBLE Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteResolvFile
(
    const char *dns1Ptr,
    const char *dns2Ptr
)
{
    char *linePtr = NULL;
    size_t len = 0;
    bool addDns1 = true;
    bool addDns2 = true;
    LE_INFO("Set DNS '%s' '%s'", dns1Ptr,dns2Ptr);

    FILE* resolvFilePtr;

    resolvFilePtr = fopen("/etc/resolv.conf", "a+");
    if (resolvFilePtr == NULL)
    {
        LE_WARN("fopen on /etc/resolv.conf failed");
        return LE_NOT_POSSIBLE;
    }

    addDns1 = (strcmp(dns1Ptr,"") != 0);
    addDns2 = (strcmp(dns2Ptr,"") != 0);
    // Search in file if the DNS are already set.
    while (getline(&linePtr, &len, resolvFilePtr) != -1)
    {
        if ( addDns1 && (strstr(linePtr, dns1Ptr) != NULL))
        {
            LE_DEBUG("'%s' found in file",dns1Ptr);
            addDns1 = false;
        }
        if ( addDns2 && (strstr(linePtr, dns2Ptr) != NULL))
        {
            LE_DEBUG("'%s' found in file",dns2Ptr);
            addDns2 = false;
        }
    }

    // Free the line
    if (linePtr)
    {
        free(linePtr);
    }

    // Add dns1 if needed
    if ( addDns1 && (fprintf(resolvFilePtr, "nameserver %s\n", dns1Ptr) < 0) )
    {
        LE_WARN("fprintf failed");
        if ( fclose(resolvFilePtr) != 0 )
        {
            LE_WARN("fclose failed");
        }
        return LE_NOT_POSSIBLE;
    }

    // Add dns2 if needed
    if ( addDns2 && (fprintf(resolvFilePtr, "nameserver %s\n", dns2Ptr) < 0) )
    {
        LE_WARN("fprintf failed");
        if ( fclose(resolvFilePtr) != 0 )
        {
            LE_WARN("fclose failed");
        }
        return LE_NOT_POSSIBLE;
    }

    if ( fclose(resolvFilePtr) != 0 )
    {
        LE_WARN("fclose failed");
        return LE_NOT_POSSIBLE;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the DNS configuration for a profile
 *
 * return
 *      LE_NOT_POSSIBLE Function failed
 *      LE_OK           Function succeed
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SetDnsConfiguration
(
    le_mdc_ProfileRef_t profileRef
)
{
    char dns1Addr[100] = {0};
    char dns2Addr[100] = {0};

    if ( le_mdc_IsIPv4(profileRef) )
    {
        if ( le_mdc_GetIPv4DNSAddresses(profileRef,
                                        dns1Addr, sizeof(dns1Addr),
                                        dns2Addr, sizeof(dns2Addr)) != LE_OK )
        {
            LE_INFO("le_mdc_GetDNSAddresses failed");
            return LE_NOT_POSSIBLE;
        }
        if ( WriteResolvFile(dns1Addr,dns2Addr) != LE_OK )
        {
            LE_INFO("Could not write in resolv file");
            return LE_NOT_POSSIBLE;
        }
    }

    if ( le_mdc_IsIPv6(profileRef) )
    {
        if ( le_mdc_GetIPv6DNSAddresses(profileRef,
                                        dns1Addr, sizeof(dns1Addr),
                                        dns2Addr, sizeof(dns2Addr)) != LE_OK )
        {
            LE_INFO("le_mdc_GetDNSAddresses failed");
            return LE_NOT_POSSIBLE;
        }
        if ( WriteResolvFile(dns1Addr,dns2Addr) != LE_OK )
        {
            LE_INFO("Could not write in resolv file");
            return LE_NOT_POSSIBLE;
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Try to start the data session
 */
//--------------------------------------------------------------------------------------------------
static void TryStartDataSession
(
    le_timer_Ref_t timerRef
)
{
    le_result_t          result;

    result = le_mdc_StartSession(MobileProfileRef);
    if (result != LE_OK)
    {
        if (le_timer_Start(timerRef) != LE_OK)
        {
            LE_ERROR("Could not start the StartDcs timer!");
            return;
        }
    }
    else
    {
        // First wait a few seconds for the default DHCP client.
        sleep(3);

        if (SetRouteConfiguration(MobileProfileRef) != LE_OK)
        {
            LE_ERROR("Failed to get configuration route.");
            if (le_timer_Start(timerRef) != LE_OK)
            {
                LE_ERROR("Could not start the StartDcs timer!");
                return;
            }
        }

        if (SetDnsConfiguration(MobileProfileRef) != LE_OK)
        {
            LE_ERROR("Failed to get configuration DNS.");
            if (le_timer_Start(timerRef) != LE_OK)
            {
                LE_ERROR("Could not start the StartDcs timer!");
                return;
            }
        }

        // Wait a few seconds to prevent rapid toggling of data connection
        sleep(5);

        IsConnected = true;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start Data Connection Service Timer Handler.
 * When the timer expires, verify if the session is connected, if NOT retry to connect it and rearm
 * the timer.
 */
//--------------------------------------------------------------------------------------------------
static void StartDcsTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    bool        sessionState;
    le_result_t result;

    if(RequestCount == 0)
    {
        // Release has been requested in the meantime, I must cancel the Request command process
        le_timer_Stop(timerRef);
    }
    else
    {
        result=le_mdc_GetSessionState(MobileProfileRef, &sessionState);
        if ((result == LE_OK) && (sessionState))
        {
            if (SetRouteConfiguration(MobileProfileRef) != LE_OK)
            {
                LE_ERROR("Failed to get configuration route.");
                TryStartDataSession(timerRef);
                return;
            }

            if (SetDnsConfiguration(MobileProfileRef) != LE_OK)
            {
                LE_ERROR("Failed to get configuration DNS.");
                TryStartDataSession(timerRef);
                return;
            }

            // The radio is ON, stop and delete the Timer.
            le_timer_Stop(timerRef);

            // Wait a few seconds to prevent rapid toggling of data connection
            sleep(5);

            IsConnected = true;
        }
        else
        {
            TryStartDataSession(timerRef);
            // TODO: find a solution to get off of this infinite loop
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Try to stop the data session
 */
//--------------------------------------------------------------------------------------------------
static void TryStopDataSession
(
    le_timer_Ref_t timerRef
)
{
    bool        sessionState;
    le_result_t result;

    result=le_mdc_GetSessionState(MobileProfileRef, &sessionState);
    if ((result == LE_OK) && (!sessionState))
    {
        IsConnected = false;
        return;
    }
    else
    {
        // Try to shutdown the radio anyway
        result=le_mdc_StopSession(MobileProfileRef);
        if (result != LE_OK)
        {
            if (le_timer_Start(timerRef) != LE_OK)
            {
                LE_ERROR("Could not start the StopDcs timer!");
            }
        }
        else
        {
            IsConnected = false;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop Data Connection Service Timer Handler.
 * When the timer expires, verify if the session is disconnected, if NOT retry to disconnect it and
 * rearm the timer.
 */
//--------------------------------------------------------------------------------------------------
static void StopDcsTimerHandler
(
    le_timer_Ref_t timerRef
)
{
    bool        sessionState;
    le_result_t result;

    if(RequestCount != 0)
    {
        // Request has been requested in the meantime, I must cancel the Release command process
        le_timer_Stop(timerRef);
    }
    else
    {
        result=le_mdc_GetSessionState(MobileProfileRef, &sessionState);
        if ((result == LE_OK) && (!sessionState))
        {
            IsConnected = false;
            // The radio is OFF, stop and delete the Timer.
            le_timer_Stop(timerRef);
        }
        else
        {
            TryStopDataSession(timerRef);
            // TODO: find a solution to get off of this infinite loop
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Send connection state event
 */
//--------------------------------------------------------------------------------------------------
static void SendConnStateEvent
(
    bool isConnected
)
{

    // Init the event data
    ConnStateData_t eventData;
    eventData.isConnected = isConnected;
    if ( isConnected )
    {
        le_mdc_GetInterfaceName(MobileProfileRef,
                                eventData.interfaceName,
                                sizeof(eventData.interfaceName));
    }
    else
    {
        // init to empty string
        eventData.interfaceName[0] = '\0';
    }

    LE_PRINT_VALUE("%s", eventData.interfaceName);
    LE_PRINT_VALUE("%i", eventData.isConnected);

    // Send the event to interested applications
    le_event_Report(ConnStateEvent, &eventData, sizeof(eventData));
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler to process a command
 */
//--------------------------------------------------------------------------------------------------
static void ProcessCommand
(
    void* reportPtr
)
{
    uint32_t command = *(uint32_t*)reportPtr;

    LE_PRINT_VALUE("%i", command);

    if (command == REQUEST_COMMAND)
    {
        RequestCount++;
        if (!IsConnected)
        {
            // Ensure that cellular network service is available
            le_cellnet_Request();
        }
        else
        {
            // There is already a data session, so send a fake event so that the new application
            // that just sent the command knows about the current state.  This will also cause
            // redundant info to be sent to the other registered apps, but that's okay.
            SendConnStateEvent(true);
        }
    }
    else if (command == RELEASE_COMMAND)
    {
        // Don't decrement below zero, as it will wrap-around.
        if (RequestCount > 0)
        {
            RequestCount--;
        }

        if ((RequestCount == 0) && IsConnected)
        {
            TryStopDataSession(StopDcsTimer);
        }
    }
    else
    {
        LE_ERROR("Command %i is not valid", command);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  Event callback for data session state changes.
 */
//--------------------------------------------------------------------------------------------------
static void DataSessionStateHandler
(
    bool   isConnected,
    void*  contextPtr
)
{
    le_mdc_ProfileRef_t profileRef = (*(le_mdc_ProfileRef_t*)contextPtr);

    uint32_t profileIndex = le_mdc_GetProfileIndex(profileRef);

    LE_PRINT_VALUE("%d", profileIndex);
    LE_PRINT_VALUE("%i", isConnected);

    // Update global state variable
    IsConnected = isConnected;

    // Send the state event to applications
    SendConnStateEvent(isConnected);

    // Restart data connection, if it has gone down, and there are still valid requests
    // todo: this mechanism needs to be much better
    if ( ( RequestCount>0 ) && ( !isConnected ) )
    {
        // Give the modem some time to recover from whatever caused the loss of the data
        // connection, before trying to recover.
        sleep(30);

        // Try to restart
        TryStartDataSession(StartDcsTimer);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  Event callback for Cellular Network Service state changes.
 */
//--------------------------------------------------------------------------------------------------
static void CellNetStateHandler
(
    le_cellnet_State_t state,
    void*              contextPtr
)
{
    LE_DEBUG("Cellular Network Service is in state.%d", state);
    switch (state)
    {
        case LE_CELLNET_RADIO_OFF:
        case LE_CELLNET_REG_EMERGENCY:
        case LE_CELLNET_REG_UNKNOWN:
            break;

        case LE_CELLNET_REG_HOME:
        case LE_CELLNET_REG_ROAMING:
            if ((RequestCount > 0) && (!IsConnected))
            {
                // If starting data session for first time, load the profile
                if ( MobileProfileRef == NULL )
                {
                    // This will initialize MobileProfileRef
                    LoadPreferencesFromConfigDb();
                    
                    // Register for data session state changes
                    le_mdc_AddSessionStateHandler(MobileProfileRef,DataSessionStateHandler,&MobileProfileRef);
                }

                TryStartDataSession(StartDcsTimer);
            }
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This thread does the actual work of starting/stopping a data connection
 *
 * @NOTE: For now, only one MobileProfile is manage, need to improve it for all profiles.
 */
//--------------------------------------------------------------------------------------------------
static void* DataThread
(
    void* contextPtr
)
{
    LE_INFO("Data Thread Started");

    // Register for command events
    le_event_AddHandler("ProcessCommand",
                        CommandEvent,
                        ProcessCommand);

    // Register for Cellular Network Service state changes
    le_cellnet_AddStateHandler(CellNetStateHandler, NULL);

    // Run the event loop
    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * The first-layer Connection State Handler
 *
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
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a handler ...
 */
//--------------------------------------------------------------------------------------------------
le_data_ConnectionStateHandlerRef_t le_data_AddConnectionStateHandler
(
    le_data_ConnectionStateHandlerFunc_t handlerPtr,
    void* contextPtr
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
 * This function removes a handler ...
 */
//--------------------------------------------------------------------------------------------------
void le_data_RemoveConnectionStateHandler
(
    le_data_ConnectionStateHandlerRef_t addHandlerRef
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
    le_data_RequestObjRef_t reqRef = le_ref_CreateRef(RequestRefMap, (void*)1);
    return reqRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Release a previously requested data connection
 */
//--------------------------------------------------------------------------------------------------
void le_data_Release
(
    le_data_RequestObjRef_t requestRef
    ///< Reference to a previously requested data connection
)
{
    // Look up the reference.  If it is NULL, then the reference is not valid.
    // Otherwise, delete the reference and send the release command to the data thread.
    void* dataPtr = le_ref_Lookup(RequestRefMap, requestRef);
    if ( dataPtr == NULL )
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
 *  Server Init
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Init the various events
    CommandEvent = le_event_CreateId("Data Command", sizeof(uint32_t));
    ConnStateEvent = le_event_CreateId("Conn State", sizeof(ConnStateData_t));

    // Create safe reference map for request references. The size of the map should be based on
    // the expected number of simultaneous data requests, so take a reasonable guess.
    RequestRefMap = le_ref_CreateMap("Requests", 5);

    // Set a timer to retry the start data session.
    StartDcsTimer = le_timer_Create("StartDcsTimer");
    le_clk_Time_t  interval = {15, 0}; // 15 seconds

    if ( (le_timer_SetHandler(StartDcsTimer, StartDcsTimerHandler) != LE_OK) ||
         (le_timer_SetRepeat(StartDcsTimer, 0) != LE_OK) ||
         (le_timer_SetInterval(StartDcsTimer, interval) != LE_OK) )
    {
        LE_ERROR("Could not start the StartDcs timer!");
    }

    // Set a timer to retry the stop data session.
    StopDcsTimer = le_timer_Create("StopDcsTimer");
    interval.sec = 5; // 5 seconds

    if ( (le_timer_SetHandler(StopDcsTimer, StopDcsTimerHandler) != LE_OK) ||
         (le_timer_SetRepeat(StopDcsTimer, 0) != LE_OK) ||
         (le_timer_SetInterval(StopDcsTimer, interval) != LE_OK) )
    {
        LE_ERROR("Could not start the StopDcs timer!");
    }

    // Start the data thread
    le_thread_Start( le_thread_Create("Data Thread", DataThread, NULL) );

    LE_INFO("Data Connection Server is ready");
}

