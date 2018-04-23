//-------------------------------------------------------------------------------------------------
/**
 * @file cm_data.c
 *
 * Handle data connection control related functionality
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//-------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_data.h"
#include "cm_common.h"

//-------------------------------------------------------------------------------------------------
/**
 * Print the data help text to stdout.
 */
//-------------------------------------------------------------------------------------------------
void cm_data_PrintDataHelp
(
    void
)
{
    printf("Data usage\n"
            "==========\n\n"
            "To get info on profile in use:\n"
            "\tcm data\n"
            "\tcm data info\n\n"
            "To set profile in use:\n"
            "\tcm data profile <index>\n\n"
            "To set apn for profile in use:\n"
            "\tcm data apn <apn>\n\n"
            "To set pdp type for profile in use:\n"
            "\tcm data pdp <pdp>\n\n"
            "To set authentication for profile in use:\n"
            "\tcm data auth <none/pap/chap> <username> <password>\n\n"
            "To start a data connection:\n"
            "\tcm data connect <optional timeout (secs)>\n\n"
            "To stop a data connection:\n"
            "\tcm data disconnect\n\n"
            "To monitor the data connection:\n"
            "\tcm data watch\n\n"
            "To start a data connection, please ensure that your profile has been configured correctly.\n"
            "Also ensure your modem is registered to the network. To verify, use 'cm radio' and check 'Status'.\n\n"
            );
}

//--------------------------------------------------------------------------------------------------
/**
 * Structure to store both uplink & downlink data bearer technologies
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    le_mdc_DataBearerTechnology_t uplink;
    le_mdc_DataBearerTechnology_t downlink;
} DataBearerTechnologies_t;

//-------------------------------------------------------------------------------------------------
/**
 * Timer used for data bearer monitoring.
 */
//-------------------------------------------------------------------------------------------------
static le_timer_Ref_t DataBearerTimerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Cache storing latest known uplink & downlink data bearer technologies
 */
//--------------------------------------------------------------------------------------------------
static DataBearerTechnologies_t DataBearerTechnologies = {
    .uplink = LE_MDC_DATA_BEARER_TECHNOLOGY_UNKNOWN,
    .downlink = LE_MDC_DATA_BEARER_TECHNOLOGY_UNKNOWN,
};

//-------------------------------------------------------------------------------------------------
/**
 * Identifies which profile index we are configuring with the data tool
 * Note: When starting a data connection, it will only utilize the default profile index 1
 */
//-------------------------------------------------------------------------------------------------
#define PROFILE_IN_USE  "tools/cmodem/profileInUse"

#define MAX_STR_SIZE    256

//-------------------------------------------------------------------------------------------------
/**
 * Network configuration structure for per IP version
 */
//-------------------------------------------------------------------------------------------------
typedef struct {
        char family[8];                 ///< IP family
        char ip[MAX_STR_SIZE];          ///< IP address
        char gw[MAX_STR_SIZE];          ///< Gateway address
        char dns1[MAX_STR_SIZE];        ///< DNS1 address
        char dns2[MAX_STR_SIZE];        ///< DNS2 address
}
NetConfIp_t;

//-------------------------------------------------------------------------------------------------
/**
 * Network configuration structure
 */
//-------------------------------------------------------------------------------------------------
typedef struct {
    le_mdc_ProfileRef_t profile;    ///< Profile reference
    char itfName[MAX_STR_SIZE];     ///< Interface name
    NetConfIp_t ipv4;               ///< IPv4 info
    NetConfIp_t ipv6;               ///< IPv6 info
}
NetConf_t;

//-------------------------------------------------------------------------------------------------
/**
 * Handle result
 */
//-------------------------------------------------------------------------------------------------
static void HandleResult
(
    const char *msg,
    le_result_t result,
    bool quit
)
{
    FILE *stream = (result != LE_OK) ? stderr : stdout;

    fprintf(stream, "%s: %s\n", msg, LE_RESULT_TXT(result));

    if (quit)
    {
        exit(result);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Gets the profile in use from configDB
 */
//-------------------------------------------------------------------------------------------------
static uint32_t GetProfileInUse
 (
     void
 )
{
    uint32_t profileIndex;
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn(PROFILE_IN_USE);

    // if node does not exist, use the default profile
    if (!le_cfg_NodeExists(iteratorRef, ""))
    {
        profileIndex = LE_MDC_DEFAULT_PROFILE;
    }
    else
    {
        profileIndex = le_cfg_GetInt(iteratorRef, "", LE_MDC_DEFAULT_PROFILE);
    }

    le_cfg_CancelTxn(iteratorRef);

    return profileIndex;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get the profile used by the data connection service
 *
 * @todo Rework that part upon change of MDC / Data interface
 */
//-------------------------------------------------------------------------------------------------
static le_mdc_ProfileRef_t GetDataProfile
(
    void
)
{
    return le_mdc_GetProfile( GetProfileInUse() );
}

//-------------------------------------------------------------------------------------------------
/**
 * Get IPv4 Configuration
 *
 */
//-------------------------------------------------------------------------------------------------
static le_result_t GetIPv4Configuration
(
    const le_mdc_ProfileRef_t profileRef,
    NetConfIp_t *netConfIp
)
{
    le_result_t result;

    if (!netConfIp)
    {
        return LE_FAULT;
    }

    result = le_mdc_GetIPv4Address(profileRef,
                                   netConfIp->ip, sizeof(netConfIp->ip));
    if (result)
    {
        HandleResult("Failed to get IP address",result, false);
        snprintf(netConfIp->ip, sizeof(netConfIp->ip), "N/A");
    }

    result = le_mdc_GetIPv4GatewayAddress(profileRef,
                                          netConfIp->gw, sizeof(netConfIp->gw));
    if (result)
    {
        HandleResult("Failed to get Gateway address", result, false);
        snprintf(netConfIp->gw, sizeof(netConfIp->gw), "N/A");
    }

    result = le_mdc_GetIPv4DNSAddresses(profileRef,
                                        netConfIp->dns1, sizeof(netConfIp->dns1),
                                        netConfIp->dns2, sizeof(netConfIp->dns2));
    if (result)
    {
        HandleResult("Failed to get DNS addresses", result, false);
    }

    if (netConfIp->dns1[0] == '\0')
    {
        snprintf(netConfIp->dns1, sizeof(netConfIp->dns1), "N/A");
    }

    if (netConfIp->dns2[0] == '\0')
    {
        snprintf(netConfIp->dns2, sizeof(netConfIp->dns2), "N/A");
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get IPv6 Configuration
 *
 */
//-------------------------------------------------------------------------------------------------
static le_result_t GetIPv6Configuration
(
    const le_mdc_ProfileRef_t profileRef,
    NetConfIp_t *netConfIp
)
{
    le_result_t result;

    if (!netConfIp)
    {
        return LE_FAULT;
    }

    result = le_mdc_GetIPv6Address(profileRef,
                                   netConfIp->ip, sizeof(netConfIp->ip));
    if (result)
    {
        HandleResult("Failed to get IP address", result, false);
    }

    result = le_mdc_GetIPv6GatewayAddress(profileRef,
                                          netConfIp->gw, sizeof(netConfIp->gw));
    if (result)
    {
        HandleResult("Failed to get Gateway address", result, false);
    }

    result = le_mdc_GetIPv6DNSAddresses(profileRef,
                                        netConfIp->dns1, sizeof(netConfIp->dns1),
                                        netConfIp->dns2, sizeof(netConfIp->dns2));
    if (result)
    {
        HandleResult("Failed to get DNS addresses", result, false);
    }

    if (netConfIp->ip[0] == '\0')
    {
        snprintf(netConfIp->ip, sizeof(netConfIp->ip), "N/A");
    }

    if (netConfIp->gw[0] == '\0')
    {
        snprintf(netConfIp->gw, sizeof(netConfIp->gw), "N/A");
    }

    if (netConfIp->dns1[0] == '\0')
    {
        snprintf(netConfIp->dns1, sizeof(netConfIp->dns1), "N/A");
    }

    if (netConfIp->dns2[0] == '\0')
    {
        snprintf(netConfIp->dns2, sizeof(netConfIp->dns2), "N/A");
    }

    return LE_OK;
}

//-------------------------------------------------------------------------------------------------
/**
 * Get Network Configuration
 *
 */
//-------------------------------------------------------------------------------------------------
static le_result_t GetNetworkConfiguration
(
    NetConf_t *netConf
)
{
    le_result_t result;
    le_mdc_ConState_t state;

    if (!netConf)
    {
        return LE_FAULT;
    }

    if ( (result = le_mdc_GetSessionState(netConf->profile, &state)) )
    {
        HandleResult("Failed to get connection state", result, false);
        return result;
    }

    if (state != LE_MDC_CONNECTED)
    {
        return LE_FAULT;
    }

    result = le_mdc_GetInterfaceName(netConf->profile,
                                     netConf->itfName, sizeof(netConf->itfName));
    if (result)
    {
        HandleResult("Failed to get interface name", result, false);
        snprintf(netConf->itfName, sizeof(netConf->itfName), "N/A");
    }

    if (le_mdc_IsIPv4(netConf->profile))
    {
        snprintf(netConf->ipv4.family, sizeof(netConf->ipv4.family), "inet");

        if (GetIPv4Configuration(netConf->profile, &(netConf->ipv4)))
        {
            HandleResult("Failed to get IPv4 configuration", LE_FAULT, false);
            return LE_FAULT;
        }
    }

    if (le_mdc_IsIPv6(netConf->profile))
    {
        snprintf(netConf->ipv6.family, sizeof(netConf->ipv6.family), "inet6");

        if (GetIPv6Configuration(netConf->profile, &(netConf->ipv6)))
        {
            HandleResult("Failed to get IPv6 configuration", LE_FAULT, false);
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to convert a le_mdc_DataBearerTechnology_t to a string
 */
//--------------------------------------------------------------------------------------------------
static const char * DataBearerTechnologyToString
(
    le_mdc_DataBearerTechnology_t technology
)
{
    switch (technology)
    {
        case LE_MDC_DATA_BEARER_TECHNOLOGY_UNKNOWN:             return "-";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_GSM:                 return "GSM";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_GPRS:                return "GPRS";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_EGPRS:               return "Edge";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_WCDMA:               return "WCDMA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_HSPA:                return "HSPA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_HSPA_PLUS:           return "HSPA+";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_DC_HSPA_PLUS:        return "DC-HSPA+";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_HSDPA:               return "HSDPA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_HSUPA:               return "HSUPA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_DC_HSUPA:            return "DC HSUPA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_DC_HSPA:             return "DC HSPA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_LTE:                 return "LTE";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_LTE_FDD:             return "LTE FDD";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_LTE_TDD:             return "LTE TDD";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_LTE_CA_DL:           return "LTE CA DL";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_LTE_CA_UL:           return "LTE CA UL";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_TD_SCDMA:            return "TD-SCDMA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_CDMA2000_1X:         return "CDMA 1X";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO:       return "CDMA Ev-DO";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_CDMA2000_EVDO_REVA:  return "CDMA Ev-DO Rev.A";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_CDMA2000_EHRPD:      return "CDMA eHRPD";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_IS95_1X:             return "IS95 1X";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_HDR_REV0_DPA:        return "HDR REV0 DPA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_HDR_REVA_DPA:        return "HDR REVA DPA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_HDR_REVB_DPA:        return "HDR REVB DPA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_HDR_REVA_MPA:        return "HDR REVA MPA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_HDR_REVB_MPA:        return "HDR REVB MPA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_HDR_REVA_EMPA:       return "HDR REVA EMPA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_HDR_REVB_EMPA:       return "HDR REVB EMPA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_HDR_REVB_MMPA:       return "HDR REVB MMPA";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_HDR_EVDO_FMC:        return "HDR EVDO FMC";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_64_QAM:              return "64 QAM";
        case LE_MDC_DATA_BEARER_TECHNOLOGY_S2B:                 return "S2B";
    }

    return "";
}

//--------------------------------------------------------------------------------------------------
/**
 * Polling function to print data bearer information
 */
//--------------------------------------------------------------------------------------------------
static void PrintDataBearerInformation
(
    le_timer_Ref_t timerRef
)
{
    le_mdc_ProfileRef_t profileRef = (le_mdc_ProfileRef_t)le_timer_GetContextPtr(timerRef);
    DataBearerTechnologies_t currentDataBearerTechnologies;
    le_result_t result;

    result = le_mdc_GetDataBearerTechnology(profileRef,
                    &(currentDataBearerTechnologies.downlink),
                    &(currentDataBearerTechnologies.uplink));

    if (result != LE_OK)
    {
        /* Back to default */
        DataBearerTechnologies.uplink = LE_MDC_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        DataBearerTechnologies.downlink = LE_MDC_DATA_BEARER_TECHNOLOGY_UNKNOWN;
        return;
    }

    if ( (currentDataBearerTechnologies.uplink   == DataBearerTechnologies.uplink) ||
         (currentDataBearerTechnologies.downlink == DataBearerTechnologies.downlink) )
    {
        /* No evolution */
        return;
    }

    printf(" Dl %-18s | Ul %-18s\n",
        DataBearerTechnologyToString(currentDataBearerTechnologies.downlink),
        DataBearerTechnologyToString(currentDataBearerTechnologies.uplink) );

    DataBearerTechnologies = currentDataBearerTechnologies;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to start data bearer information monitoring
 */
//--------------------------------------------------------------------------------------------------
static void StartDataBearerMonitoring
(
    le_mdc_ProfileRef_t profileRef
)
{
    const le_clk_Time_t pollingPeriod = {
        .sec = 2,
        .usec = 0,
    };

    DataBearerTimerRef = le_timer_Create("CmDataBearer");

    le_timer_SetHandler(DataBearerTimerRef, PrintDataBearerInformation);
    le_timer_SetContextPtr(DataBearerTimerRef, profileRef);
    le_timer_SetInterval(DataBearerTimerRef, pollingPeriod);
    le_timer_SetRepeat(DataBearerTimerRef, 0);

    le_timer_Start(DataBearerTimerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to stop data bearer information monitoring
 */
//--------------------------------------------------------------------------------------------------
static void StopDataBearerMonitoring
(
    void
)
{
    if (DataBearerTimerRef != NULL)
    {
        le_timer_Delete(DataBearerTimerRef);
        DataBearerTimerRef = NULL;
    }

    /* Back to default */
    DataBearerTechnologies.uplink = LE_MDC_DATA_BEARER_TECHNOLOGY_UNKNOWN;
    DataBearerTechnologies.downlink = LE_MDC_DATA_BEARER_TECHNOLOGY_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
/**
 * Callback for the connection state
 */
//--------------------------------------------------------------------------------------------------
static void ConnectionStateHandler
(
    le_mdc_ProfileRef_t profileRef,
    le_mdc_ConState_t state,
    void* contextPtr
)
{
    if (LE_MDC_DISCONNECTED == state)
    {
        StopDataBearerMonitoring();
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Callback for checking if our data connection was was successful before the timeout.
 */
//--------------------------------------------------------------------------------------------------
static void ExpiryHandler(le_timer_Ref_t timerRef)
{
    HandleResult("Timed out wating for data connection", LE_TIMEOUT, true);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start timer for the data connection request.
 *
 * @return LE_OK if the call was successful.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartTimer
(
    const char * timeout
)
{
    // cast our timeout to int
    unsigned long time = strtoul(timeout, NULL, 0);

    if (time == 0)
    {
        printf("Invalid argument for timeout value.\n");
        return LE_NOT_POSSIBLE;
    }

    // Set timer for data connection request
   le_timer_Ref_t timerRef = NULL;
   le_clk_Time_t interval = { time, 0 };
   le_result_t res = LE_NOT_POSSIBLE;

   timerRef = le_timer_Create("Data_Request_Timeout");
   res = le_timer_SetInterval(timerRef, interval);

   if (res != LE_OK)
   {
       LE_ERROR("Unable to set timer interval.");
       return res;
   }

   res = le_timer_SetHandler(timerRef, ExpiryHandler);

   if (res != LE_OK)
   {
       LE_ERROR("Unable to set timer handler.");
       return res;
   }

   res = le_timer_Start(timerRef);

   if (res != LE_OK)
   {
       LE_ERROR("Unable to start timer.");
       return res;
   }

   return res;
}

//-------------------------------------------------------------------------------------------------
/**
 * Set the profile in use in configDB
 */
//-------------------------------------------------------------------------------------------------
int cm_data_SetProfileInUse
(
    int profileInUse
)
{
    if ( 0 == profileInUse )
    {
       printf("profile 0 is not valid!\n");
       return EXIT_FAILURE;
    }
    else
    {
        le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn(PROFILE_IN_USE);

        le_cfg_SetInt(iteratorRef, "", profileInUse);
        le_cfg_CommitTxn(iteratorRef);
        return EXIT_SUCCESS;
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * This function return the string associated
 */
//-------------------------------------------------------------------------------------------------
static const char* ConvertPdp
(
    le_mdc_Pdp_t pdp    ///< [IN] Packet data protocol
)
{
    switch (pdp)
    {
        case LE_MDC_PDP_IPV4:       return "IPV4";
        case LE_MDC_PDP_IPV6:       return "IPV6";
        case LE_MDC_PDP_IPV4V6:     return "IPV4V6";
        case LE_MDC_PDP_UNKNOWN:    return "UNKNOWN";
    }

    return "ERROR"; // Should not happen
}

//-------------------------------------------------------------------------------------------------
/**
 * This function return the string associated
 */
//-------------------------------------------------------------------------------------------------
static const char* ConvertAuthentication
(
    le_mdc_Auth_t type    ///< [IN] Authentication type
)
{
    switch (type)
    {
        case LE_MDC_AUTH_PAP:   return "PAP";
        case LE_MDC_AUTH_CHAP:  return "CHAP";
        case LE_MDC_AUTH_NONE:  return "NONE";
    }

    return "ERROR"; // Should not happen
}

//-------------------------------------------------------------------------------------------------
/**
 * Callback for the session Connection
 */
//-------------------------------------------------------------------------------------------------
static void SessionHandler
(
    le_mdc_ProfileRef_t profile,
    le_result_t result,
    void* contextPtr
)
{
    if (!result)
    {
        HandleResult("Connection Success", result, true);
    }
    else
    {
        HandleResult("Connection Failure", result, true);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Start a data connection.
 */
//--------------------------------------------------------------------------------------------------
void cm_data_StartDataConnection
(
    const char * timeoutPtr    ///< [IN] Data connection timeout timer
)
{
    le_mdc_ProfileRef_t profile;
    le_result_t result;

    profile = GetDataProfile();

    if (!timeoutPtr)
    {
        if ( (result = le_mdc_StartSession(profile)) )
        {
            HandleResult("Connection Failure", result, true);
        }

        HandleResult("Connection Success", result, true);
    }
    else if (strtol(timeoutPtr, NULL, 10) == -1)
    {
        if ( (result = le_mdc_StopSession(profile)) )
        {
            HandleResult("Stop Failure", result, true);
        }
        HandleResult("Stop Success", result, true);
    }
    else
    {
        le_mdc_StartSessionAsync(profile, SessionHandler, NULL);
        if ( (result = StartTimer(timeoutPtr)) )
        {
            HandleResult("Failed to start data session timer",result, true);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Monitor a data connection.
 */
//--------------------------------------------------------------------------------------------------
void cm_data_MonitorDataConnection
(
    void
)
{
    le_mdc_ProfileRef_t profile;

    profile = GetDataProfile();

    StartDataBearerMonitoring(profile);

    le_mdc_AddSessionStateHandler(profile, ConnectionStateHandler, NULL);
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to set the APN name.
 *
 * @todo Hardcoded to set the apn for first profile. Will revisit when dcsDaemon allows us to start
 * a data connection on another profile.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_data_SetApnName
(
    const char * apn        ///< [IN] Access point name
)
{
    le_mdc_ProfileRef_t profileRef = GetDataProfile();

    if (profileRef == NULL)
    {
        return EXIT_FAILURE;
    }

    if (le_mdc_SetAPN(profileRef, apn) != LE_OK)
    {
        printf("Could not set APN '%s' for profile %u.\n"
               "Maybe the profile is connected", apn, le_mdc_GetProfileIndex(profileRef));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to set the PDP type.
 *
 * @todo Hardcoded to set the pdp for first profile. Will revisit when dcsDaemon allows us to start
 * a data connection on another profile.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_data_SetPdpType
(
    const char * pdpType    ///< [IN] Packet data protocol
)
{
    le_mdc_ProfileRef_t profileRef = GetDataProfile();

    if (profileRef == NULL)
    {
        printf("Invalid profile\n");
        return EXIT_FAILURE;
    }

    le_mdc_Pdp_t pdp = LE_MDC_PDP_UNKNOWN;

    char pdpTypeToUpper[CMODEM_COMMON_PDP_STR_LEN];
    cm_cmn_ToUpper(pdpType, pdpTypeToUpper, sizeof(pdpTypeToUpper));

    if (strcmp(pdpTypeToUpper, "IPV4") == 0)
    {
        pdp = LE_MDC_PDP_IPV4;
    }
    else if (strcmp(pdpTypeToUpper, "IPV6") == 0)
    {
        pdp = LE_MDC_PDP_IPV6;
    }
    else if (strcmp(pdpTypeToUpper, "IPV4V6") == 0)
    {
        pdp = LE_MDC_PDP_IPV4V6;
    }
    else
    {
        printf("'%s' is not supported\n", pdpTypeToUpper);
        return EXIT_FAILURE;
    }

    if (le_mdc_SetPDP(profileRef, pdp) != LE_OK)
    {
        printf("Could not set PDP '%s' for profile %u.\n"
               "Maybe the profile is connected",
               pdpTypeToUpper,
               le_mdc_GetProfileIndex(profileRef));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to set the authentication information.
 *
 * @todo Hardcoded to set the authentication for "internet" profile. Will revisit when dcsDaemon
 * allows us to start a data connection on another profile.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_data_SetAuthentication
(
    const char * type,      ///< [IN] Authentication type
    const char * userName,  ///< [IN] Authentication username
    const char * password   ///< [IN] Authentication password
)
{
    le_mdc_Auth_t auth;

    le_mdc_ProfileRef_t profileRef = GetDataProfile();

    if (profileRef == NULL)
    {
        printf("Invalid profile\n");
        return EXIT_FAILURE;
    }

    char typeToLower[100];
    cm_cmn_ToLower(type, typeToLower, sizeof(typeToLower));

    if (strcmp(typeToLower, "none") == 0)
    {
        auth = LE_MDC_AUTH_NONE;
    }
    else if (strcmp(typeToLower, "pap") == 0)
    {
        auth = LE_MDC_AUTH_PAP;
    }
    else if (strcmp(typeToLower, "chap") == 0)
    {
        auth = LE_MDC_AUTH_CHAP;
    }
    else
    {
        printf("Type of authentication '%s' is not available\n"
               "try using 'none', 'chap', 'pap'\n", typeToLower);
        return EXIT_FAILURE;
    }

    if (le_mdc_SetAuthentication(profileRef, auth, userName, password) != LE_OK)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function prints a profile index.
 */
//-------------------------------------------------------------------------------------------------
static void PrintProfileIndex
(
    uint32_t profileIndex
)
{
    char profileIndexStr[5];

    snprintf(profileIndexStr, sizeof(profileIndexStr), "%u", profileIndex);
    cm_cmn_FormatPrint("Index", profileIndexStr);
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the apn name from a specified index.
 *
 * @return LE_OK if the call was successful
 */
//-------------------------------------------------------------------------------------------------
static le_result_t PrintApnName
(
    le_mdc_ProfileRef_t profileRef   ///< [IN] profile reference
)
{
    char apnName[LE_MDC_APN_NAME_MAX_BYTES];
    le_result_t res = LE_OK;

    res = le_mdc_GetAPN(profileRef, apnName, sizeof(apnName));

    if (res != LE_OK)
    {
        return res;
    }

    cm_cmn_FormatPrint("APN", apnName);

    return res;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the pdp type from a specified iterator.
 *
 * @return LE_OK if the call was successful
 */
//-------------------------------------------------------------------------------------------------
static le_result_t PrintPdpType
(
    le_mdc_ProfileRef_t profileRef    ///< [IN] profile reference
)
{
    le_mdc_Pdp_t pdp = LE_MDC_PDP_UNKNOWN;
    le_result_t res = LE_OK;

    pdp = le_mdc_GetPDP(profileRef);

    cm_cmn_FormatPrint("PDP Type", ConvertPdp(pdp));

    return res;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to get the authentication data from a specified iterator. Since only
 * one authentication is supported, if both authentication are enable, only the first auth is taken.
 *
 * @return LE_OK if the call was successful
 */
//-------------------------------------------------------------------------------------------------
static le_result_t PrintAuthentication
(
    le_mdc_ProfileRef_t profileRef    ///< [IN] profile reference
)
{
    le_result_t res = LE_OK;
    le_mdc_Auth_t authenticationType;

    char userName[LE_MDC_USER_NAME_MAX_BYTES]={0};
    char password[LE_MDC_PASSWORD_NAME_MAX_BYTES]={0};

    res = le_mdc_GetAuthentication(profileRef,
                                   &authenticationType,
                                   userName,sizeof(userName),
                                   password,sizeof(password));

    if (res != LE_OK)
    {
        return res;
    }

    if (authenticationType != LE_MDC_AUTH_NONE)
    {
        cm_cmn_FormatPrint("Auth type", ConvertAuthentication(authenticationType));
        cm_cmn_FormatPrint("User name", userName);
        cm_cmn_FormatPrint("Password", password);
    }

    return res;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will attempt to print the state of the profile.
 *
 * @return LE_OK if the call was successful
 */
//-------------------------------------------------------------------------------------------------
static le_result_t PrintIsConnected
(
    le_mdc_ProfileRef_t profileRef    ///< [IN] profile reference
)
{
    le_result_t res = LE_OK;
    le_mdc_ConState_t state;

    res = le_mdc_GetSessionState(profileRef, &state);

    if (res != LE_OK)
    {
        return res;
    }

    cm_cmn_FormatPrint("Connected", (state == LE_MDC_CONNECTED) ? "yes" : "no");

    return res;
}

//-------------------------------------------------------------------------------------------------
/**
 * Print Network Configuration
 *
 */
//-------------------------------------------------------------------------------------------------
static le_result_t PrintNetworkConfiguration
(
    le_mdc_ProfileRef_t profile    ///< [IN] profile reference
)
{
    le_result_t result;
    NetConf_t netConf = {0};

    netConf.profile = profile;

    result = GetNetworkConfiguration(&netConf);
    if (result == LE_OK)
    {
        cm_cmn_FormatPrint("Interface", netConf.itfName);

        // Per IP family
        int i = 0;
        for (; i <= 1; i++)
        {
            NetConfIp_t * netConfIp = &(netConf.ipv4);
            const char * netConfName = "IPv4";
            char lineName[CMODEM_COMMON_COLUMN_LEN+1];
            if (i == 1)
            {
                netConfIp = &(netConf.ipv6);
                netConfName = "IPv6";
            }

            if (netConfIp->family[0] == '\0')
            {
                continue;
            }

            snprintf(lineName, sizeof(lineName), "Family[%s]", netConfName);
            cm_cmn_FormatPrint(lineName, netConfIp->family);
            snprintf(lineName, sizeof(lineName), "IP[%s]", netConfName);
            cm_cmn_FormatPrint(lineName, netConfIp->ip);
            snprintf(lineName, sizeof(lineName), "Gateway[%s]", netConfName);
            cm_cmn_FormatPrint(lineName, netConfIp->gw);
            snprintf(lineName, sizeof(lineName), "Dns1[%s]", netConfName);
            cm_cmn_FormatPrint(lineName, netConfIp->dns1);
            snprintf(lineName, sizeof(lineName), "Dns2[%s]", netConfName);
            cm_cmn_FormatPrint(lineName, netConfIp->dns2);
        }
    }

    return result;
}

//-------------------------------------------------------------------------------------------------
/**
 * This function will return profile information for profile that it will be using.
 *
 * @todo Hardcoded to return the first profile at the moment, will revisit when dcsDaemon allows
 * us to start a data connection on another profile.
 *
 * @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
//-------------------------------------------------------------------------------------------------
int cm_data_GetProfileInfo
(
    void
)
{
    int exitStatus = EXIT_SUCCESS;

    le_mdc_ProfileRef_t profileRef = GetDataProfile();

    if (NULL == profileRef)
    {
        printf("Invalid profile %p\n", profileRef);
        return EXIT_FAILURE;
    }

    PrintProfileIndex(le_mdc_GetProfileIndex(profileRef));

    if (LE_OK != PrintApnName(profileRef))
    {
        exitStatus = EXIT_FAILURE;
    }

    if (LE_OK != PrintPdpType(profileRef))
    {
        exitStatus = EXIT_FAILURE;
    }

    if (LE_OK != PrintIsConnected(profileRef))
    {
        exitStatus = EXIT_FAILURE;
    }

    if (LE_OK != PrintAuthentication(profileRef))
    {
        exitStatus = EXIT_FAILURE;
    }

    if (LE_OK != PrintNetworkConfiguration(profileRef))
    {
        exitStatus = EXIT_FAILURE;
    }

    return exitStatus;
}

//--------------------------------------------------------------------------------------------------
/**
 * Process commands for data service.
 */
//--------------------------------------------------------------------------------------------------
void cm_data_ProcessDataCommand
(
    const char * command,   ///< [IN] Data commands
    size_t numArgs          ///< [IN] Number of arguments
)
{
    const char*  dataParam = le_arg_GetArg(2);
    if (strcmp(command, "help") == 0)
    {
        cm_data_PrintDataHelp();
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(command, "info") == 0)
    {
        exit(cm_data_GetProfileInfo());
    }
    else if (strcmp(command, "profile") == 0)
    {
        if (cm_cmn_CheckEnoughParams(1,
                                     numArgs,
                                     "Profile index missing. e.g. cm data profile <index>"))
        {
            if (NULL == dataParam)
            {
                LE_ERROR("dataParam is NULL");
                exit(EXIT_FAILURE);
            }
            exit(cm_data_SetProfileInUse(atoi(dataParam)));
        }
    }
    else if (strcmp(command, "connect") == 0)
    {
        if (numArgs > 3)
        {
            printf("Invalid argument when starting a data connection. "
                   "e.g. cm data connect <optional timeout (secs)>\n");
            exit(EXIT_FAILURE);
        }

        if (NULL == dataParam)
        {
            LE_INFO("dataParam is NULL");
        }
        cm_data_StartDataConnection(dataParam);
    }
    else if (strcmp(command, "apn") == 0)
    {
        if (cm_cmn_CheckEnoughParams(1, numArgs, "APN name missing. e.g. cm data apn <apn name>"))
        {
            if (NULL == dataParam)
            {
                LE_ERROR("dataParam is NULL");
                exit(EXIT_FAILURE);
            }
            exit(cm_data_SetApnName(dataParam));
        }
    }
    else if (strcmp(command, "pdp") == 0)
    {
        if (cm_cmn_CheckEnoughParams(1,
                                     numArgs,
                                     "PDP type name missing. e.g. cm data pdp <pdp type>"))
        {
            if (NULL == dataParam)
            {
                LE_ERROR("dataParam is NULL");
                exit(EXIT_FAILURE);
            }
            exit(cm_data_SetPdpType(dataParam));
        }
    }
    else if (strcmp(command, "auth") == 0)
    {
        // configure all authentication info
        if (numArgs == 5)
        {
            const char* userNamePtr = le_arg_GetArg(3);
            const char* passwordPtr = le_arg_GetArg(4);
            if (NULL == dataParam)
            {
                LE_ERROR("dataParam is NULL");
                exit(EXIT_FAILURE);
            }
            if (NULL == userNamePtr)
            {
                LE_ERROR("userNamePtr is NULL");
                exit(EXIT_FAILURE);
            }
            if (NULL == passwordPtr)
            {
                LE_ERROR("passwordPtr is NULL");
                exit(EXIT_FAILURE);
            }
            exit(cm_data_SetAuthentication(dataParam, userNamePtr, passwordPtr));
        }
        // for none option
        else if (numArgs == 3)
        {
            if (NULL == dataParam)
            {
                LE_ERROR("dataParam is NULL");
                exit(EXIT_FAILURE);
            }
            exit(cm_data_SetAuthentication(dataParam, "", ""));
        }
        else
        {
            printf("Auth parameters incorrect. "
                   "e.g. cm data auth [<auth type>] [<username>] [<password>]\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (strcmp(command, "watch") == 0)
    {
        cm_data_MonitorDataConnection();
    }
    else
    {
        printf("Invalid command for data service.\n");
        exit(EXIT_FAILURE);
    }
}
