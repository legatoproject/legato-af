
// -------------------------------------------------------------------------------------------------
/**
 *  @file cm_data.c
 *
 *  Handle data connection control related functionality
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "cm_data.h"
#include "cm_common.h"

// Hard coded: the default profile is the first one
// @todo: add an API in DCS to know which profile is used.
static int32_t DefaultProfile = 1;

// -------------------------------------------------------------------------------------------------
/**
 *  The data connection reference.
 */
// -------------------------------------------------------------------------------------------------
static le_data_RequestObjRef_t RequestRef = NULL;

// -------------------------------------------------------------------------------------------------
/**
 *  Boolean to check whether if the data connection is connected or not.
 */
// -------------------------------------------------------------------------------------------------
static bool DataConnected = false;

// -------------------------------------------------------------------------------------------------
/**
 *  Boolean to check whether data connection was killed by sigterm/sigint.
 */
// -------------------------------------------------------------------------------------------------
static bool IsTerminated = false;

// -------------------------------------------------------------------------------------------------
/**
 *  Identifies which profile index we are configuring with the data tool
 *  Note: When starting a data connection, it will only utilize the default profile index 1
 */
// -------------------------------------------------------------------------------------------------
#define PROFILE_IN_USE  "tools/cmodem/ProfileInUse"


// -------------------------------------------------------------------------------------------------
/**
*  Print the data help text to stdout.
*/
// -------------------------------------------------------------------------------------------------
void cm_data_PrintDataHelp
(
    void
)
{
    printf("Data usage\n"
            "==========\n\n"
            "To get info on profile in use:\n"
            "\tcm data\n\n"
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
            "To list all the profiles:\n"
            "\tcm data list\n\n"
            "To monitor the data connection:\n"
            "\tcm data watch\n\n"
            "To start a data connection, please ensure that your profile has been configured correctly.\n"
            "Also ensure your modem is registered to the network. To verify, use 'cm radio' and check 'Status'.\n\n"
            );
}


//--------------------------------------------------------------------------------------------------
/**
* Callback for the connection state
*/
//--------------------------------------------------------------------------------------------------
void ConnectionStateHandler
(
    const char* intfName,
    bool isConnected,
    void* contextPtr
)
{
    if (isConnected)
    {
        DataConnected = isConnected;
        printf("%s connected\n", intfName);
    }
    else
    {
        printf("disconnected\n");

        if (IsTerminated)
        {
            exit(EXIT_SUCCESS);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGINT/SIGTERM when process dies.
 */
//--------------------------------------------------------------------------------------------------
void SigHandler
(
    int sigNum
)
{
    le_data_Release(RequestRef);

    if (DataConnected)
    {
        IsTerminated = true;
    }
    else
    {
        exit(EXIT_SUCCESS);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Callback for checking if our data connection was was successful before the timeout.
 */
//--------------------------------------------------------------------------------------------------
void ExpiryHandler(le_timer_Ref_t timerRef)
{
    if (!DataConnected)
    {
        fprintf(stderr, "Timed-out\n");
        le_data_Release(RequestRef);
        exit(2); // 2 signifies timeout
    }
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


// -------------------------------------------------------------------------------------------------
/**
*  Gets the profile in use from configDB
*/
// -------------------------------------------------------------------------------------------------
static int GetProfileInUse
(
    void
)
{
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateReadTxn(PROFILE_IN_USE);

    // if node does not exist, populate with default profile
    if (!le_cfg_NodeExists(iteratorRef, ""))
    {
        return DefaultProfile;
    }

    return le_cfg_GetInt(iteratorRef, "", DefaultProfile);
}


// -------------------------------------------------------------------------------------------------
/**
*  Set the profile in use in configDB
*/
// -------------------------------------------------------------------------------------------------
int cm_data_SetProfileInUse
(
    int profileInUse
)
{
    le_cfg_IteratorRef_t iteratorRef = le_cfg_CreateWriteTxn(PROFILE_IN_USE);

    le_cfg_SetInt(iteratorRef, "", profileInUse);
    le_cfg_CommitTxn(iteratorRef);
    return EXIT_SUCCESS;
}


// -------------------------------------------------------------------------------------------------
/**
*  This function return the string associated
*/
// -------------------------------------------------------------------------------------------------
static const char* ConvertPdp
(
    le_mdc_Pdp_t pdp    ///< [IN] Packet data protocol
)
{
    switch (pdp)
    {
        case LE_MDC_PDP_IPV4: return "IPV4";
        case LE_MDC_PDP_IPV6: return "IPV6";
        case LE_MDC_PDP_IPV4V6: return "IPV4V6";
        case LE_MDC_PDP_UNKNOWN: return "UNKNOWN";
    }

    return "ERROR"; // Should not happen
}

// -------------------------------------------------------------------------------------------------
/**
*  This function return the string associated
*/
// -------------------------------------------------------------------------------------------------
static const char* ConvertAuthentication
(
    le_mdc_Auth_t type    ///< [IN] Authentication type
)
{
    switch (type)
    {
        case LE_MDC_AUTH_PAP: return "PAP";
        case LE_MDC_AUTH_CHAP: return "CHAP";
        case LE_MDC_AUTH_NONE: return "NONE";
    }

    return "ERROR"; // Should not happen
}

//--------------------------------------------------------------------------------------------------
/**
* Start a data connection.
*/
//--------------------------------------------------------------------------------------------------
void cm_data_StartDataConnection
(
    const char * timeout        ///< [IN] Data connection timeout timer
)
{
    // Register a call-back
    le_data_AddConnectionStateHandler(ConnectionStateHandler, NULL);

    // Block Signals that we are going to use.
    // @todo: This can be done in main by the code generator later.  This could also be a function
    //        in the signals API.
    sigset_t sigSet;
    LE_ASSERT(sigemptyset(&sigSet) == 0);
    LE_ASSERT(sigaddset(&sigSet, SIGINT) == 0);
    LE_ASSERT(sigaddset(&sigSet, SIGTERM) == 0);
    LE_ASSERT(pthread_sigmask(SIG_BLOCK, &sigSet, NULL) == 0);

    // Register a signal event handler for SIGINT/SIGTERM when user interrupts/terminates process
    le_sig_SetEventHandler(SIGINT, SigHandler);
    le_sig_SetEventHandler(SIGTERM, SigHandler);

    // start data request timer
    if (timeout != NULL)
    {
        le_result_t res = StartTimer(timeout);

        if (res != LE_OK)
        {
            exit(EXIT_FAILURE);
        }
    }

    // Request data connection
    RequestRef = le_data_Request();
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
    // Register a call-back
    le_data_AddConnectionStateHandler(ConnectionStateHandler, NULL);
}


// -------------------------------------------------------------------------------------------------
/**
*  This function will attempt to set the APN name.
*
*  @todo Hardcoded to set the apn for first profile. Will revisit when dcsDaemon allows us to start
*  a data connection on another profile.
*
*  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
*/
// -------------------------------------------------------------------------------------------------
int cm_data_SetApnName
(
    const char * apn        ///< [IN] Access point name
)
{
    le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile(GetProfileInUse());

    if (profileRef == NULL)
    {
        return EXIT_FAILURE;
    }

    if (le_mdc_SetAPN(profileRef, apn) != LE_OK)
    {
        printf("Could not set APN '%s' for profile %d.\n"
               "Maybe the profile is connected", apn, GetProfileInUse());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


// -------------------------------------------------------------------------------------------------
/**
*  This function will attempt to set the PDP type.
*
*  @todo Hardcoded to set the pdp for first profile. Will revisit when dcsDaemon allows us to start
*  a data connection on another profile.
*
*  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
*/
// -------------------------------------------------------------------------------------------------
int cm_data_SetPdpType
(
    const char * pdpType    ///< [IN] Packet data protocol
)
{
    le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile(GetProfileInUse());

    if (profileRef == NULL)
    {
        printf("Invalid profile\n");
        return EXIT_FAILURE;
    }

    le_mdc_Pdp_t pdp = LE_MDC_PDP_UNKNOWN;

    char pdpTypeToUpper[100];
    cm_cmn_toUpper(pdpType, pdpTypeToUpper, sizeof(pdpTypeToUpper));

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
        printf("Could not set PDP '%s' for profile %d.\n"
               "Maybe the profile is connected", pdpTypeToUpper, GetProfileInUse());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


// -------------------------------------------------------------------------------------------------
/**
*  This function will attempt to set the authentication information.
*
*  @todo Hardcoded to set the authentication for "internet" profile. Will revisit when dcsDaemon allows us to start
*  a data connection on another profile.
*
*  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
*/
// -------------------------------------------------------------------------------------------------
int cm_data_SetAuthentication
(
    const char * type,      ///< [IN] Authentication type
    const char * userName,  ///< [IN] Authentication username
    const char * password   ///< [IN] Authentication password
)
{
    le_mdc_Auth_t auth;
    le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile(GetProfileInUse());

    if (profileRef == NULL)
    {
        printf("Invalid profile\n");
        return EXIT_FAILURE;
    }

    char typeToLower[100];
    cm_cmn_toLower(type, typeToLower, sizeof(typeToLower));

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


// -------------------------------------------------------------------------------------------------
/**
*  This function will attempt to get the apn name from a specified index.
*
*  @return LE_OK if the call was successful
*/
// -------------------------------------------------------------------------------------------------
static le_result_t GetApnName
(
    le_mdc_ProfileRef_t profileRef   ///< [IN] profile reference
)
{
    char apnName[100];
    le_result_t res = LE_OK;

    res = le_mdc_GetAPN(profileRef, apnName, sizeof(apnName));

    if (res != LE_OK)
    {
        return res;
    }

    cm_cmn_FormatPrint("APN", apnName);

    return res;
}


// -------------------------------------------------------------------------------------------------
/**
*  This function will attempt to get the pdp type from a specified iterator.
*
*  @return LE_OK if the call was successful
*/
// -------------------------------------------------------------------------------------------------
static le_result_t GetPdpType
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


// -------------------------------------------------------------------------------------------------
/**
*  This function will attempt to get the authentication data from a specified iterator. Since only
*  one authentication is supported, if both authentication are enable, only the first auth is taken.
*
*  @return LE_OK if the call was successful
*/
// -------------------------------------------------------------------------------------------------
static le_result_t GetAuthentication
(
    le_mdc_ProfileRef_t profileRef    ///< [IN] profile reference
)
{
    le_result_t res = LE_OK;
    le_mdc_Auth_t authenticationType;

    char userName[100]={0};
    char password[100]={0};

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


// -------------------------------------------------------------------------------------------------
/**
*  This function will return profile information for profile that it will be using.
*
*  @todo Hardcoded to return the first profile at the moment, will revisit when dcsDaemon allows
*  us to start a data connection on another profile.
*
*  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
*/
// -------------------------------------------------------------------------------------------------
int cm_data_GetProfileInfo
(
    void
)
{
    int exitStatus = EXIT_SUCCESS;

    le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile(GetProfileInUse());

    if (profileRef == NULL)
    {
        printf("Invalid profile\n");
        return LE_NOT_FOUND;
    }

    char defaultProfileName[32] = {0};
    snprintf(defaultProfileName, sizeof(defaultProfileName)-1, "%d", GetProfileInUse());

    le_result_t res;

    // defaulted
    cm_cmn_FormatPrint("Profile", defaultProfileName);

    res = GetApnName(profileRef);

    if (res != LE_OK)
    {
        exitStatus = EXIT_FAILURE;
    }

    res = GetPdpType(profileRef);

    if (res != LE_OK)
    {
        exitStatus = EXIT_FAILURE;
    }

    res = GetAuthentication(profileRef);

    if (res != LE_OK)
    {
        exitStatus = EXIT_FAILURE;
    }

    printf("\n");

    return exitStatus;
}


// -------------------------------------------------------------------------------------------------
/**
 *  This function will list all profiles with their pdp and apn information.
 *
 *  @return EXIT_SUCCESS if the call was successful, EXIT_FAILURE otherwise.
 */
// -------------------------------------------------------------------------------------------------
int cm_data_ListProfileName
(
    void
)
{
    int profileIndex;
    int maxProfile = le_mdc_NumProfiles();

    for( profileIndex = 1;
         profileIndex <= maxProfile;
         profileIndex++)
    {
        le_result_t res;
        le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile(profileIndex);

        if (profileRef == NULL)
        {
            printf("Invalid profile\n");
            return EXIT_FAILURE;
        }

        // Get PDP
        le_mdc_Pdp_t pdp = LE_MDC_PDP_UNKNOWN;
        pdp = le_mdc_GetPDP(profileRef);

        // Get APN
        char apnName[100];
        res = le_mdc_GetAPN(profileRef, apnName, sizeof(apnName));

        if (res != LE_OK)
        {
            LE_ERROR("Unable to get APN");
            return EXIT_FAILURE;
        }

        printf("%d, %s, %s\n", profileIndex, ConvertPdp(pdp), apnName);
    }

    printf("\n");

    return EXIT_SUCCESS;
}

