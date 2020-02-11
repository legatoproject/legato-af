/**
 * This module is for testing of the modemServices MDC component.
 *
 * You must issue the following commands:
 * @verbatim
   $ app start mdcTest
   @endverbatim
 *
 * By default, the profile used is LE_MDC_DEFAULT_PROFILE, and the APN is automatically set.
 * Some customize parameters can be set by creating a "/tmp/config.txt" file and fill a command
 * line with the syntax:
 * <profile number> <pdp_type> <apn> [<authentication_type> <username> <password>]
 * Where:
 *  - <profile number> is the profile number to be used (or "default" to use the default profile)
 *  - <pdp_type> if the packet data protocol to be used: "ipv4", "ipv6", or "ipv4v6"
 *  - <apn> is the APN to be used
 *  - <authentication_type> (optional): authentication requested: "auth_none" (default), pap",
 *  "chap", "pap-chap"
 *  - <username> (optional): username for authentication
 *  - <password> (optional): password for authentication
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "le_print.h"
#include "errno.h"

#define TEST_DEF(X) {#X, X}

// Semaphore
static le_sem_Ref_t    TestSemaphore;
static le_sem_Ref_t    AsyncTestSemaphore;

//! [Profiles]
static const char DefaultCid[] = "default";
static const char automaticApn[] = "sp.telus.com";//"3gnet"; //"auto";
static const char PdpIpv4[] = "IPV4";//"ipv4";
static const char PdpIpv6[] = "ipv6";
static const char PdpIpv4v6[] = "ipv4v6";
static const char AuthPap[] = "pap";
static const char AuthChap[] = "chap";
static const char AuthPapChap[] = "pap-chap";
//static const char Map[] = "map";
//static const char Cid[] = "cid";
//static const char Rmnet[] = "rmnet";

// Structure used to set the configuration
typedef struct
{
    char   cid[10];                                 // profile identifier
    char   pdp[10];                                 // packet data protocol
    char   apn[LE_MDC_APN_NAME_MAX_LEN];            // access point network
    char   auth[10];                                // authentication level
    char   userName[LE_MDC_USER_NAME_MAX_LEN];      // username for authentication
    char   password[LE_MDC_PASSWORD_NAME_MAX_LEN];  // password for authentication
}
Configuration_t;

// Tests cases
typedef enum
{
    TEST_SYNC,
    TEST_ASYNC,
    TEST_MAX
} Testcase_t;

// Tests definition
struct
{
    char testName[20];
    Testcase_t  testCase;
} testsDef[] = {
                    TEST_DEF(TEST_SYNC),
                    TEST_DEF(TEST_ASYNC),
                    TEST_DEF(TEST_MAX)
};


//! [Data_session]
//--------------------------------------------------------------------------------------------------
/**
 * Session handler response for connection and disconnection.
 */
//--------------------------------------------------------------------------------------------------
static void SessionHandlerFunc
(
    le_mdc_ProfileRef_t profileRef,
    le_result_t result,
    void* contextPtr
)
{
    le_result_t* activationPtr = contextPtr;
    *activationPtr = result;

    LE_INFO("Session result %d for profile %d", result, le_mdc_GetProfileIndex(profileRef));

    le_sem_Post(AsyncTestSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Start asynchronous session.
 */
//--------------------------------------------------------------------------------------------------
static void SessionStartAsync
(
    void* param1Ptr,
    void* param2Ptr
)
{
    le_mdc_ProfileRef_t profileRef = param1Ptr;

    le_mdc_StartSessionAsync(profileRef, SessionHandlerFunc, param2Ptr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop asynchronous session.
 */
//--------------------------------------------------------------------------------------------------
static void SessionStopAsync
(
    void* param1Ptr,
    void* param2Ptr
)
{
    le_mdc_ProfileRef_t profileRef = param1Ptr;

    le_mdc_StopSessionAsync(profileRef, SessionHandlerFunc, param2Ptr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the configuration.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SetConfiguration
(
    le_mdc_ProfileRef_t* profileRefPtr
)
{
    Configuration_t configuration;
    memset(&configuration, 0, sizeof(Configuration_t));
    le_utf8_Copy(configuration.cid, DefaultCid, sizeof(configuration.cid), NULL);
    le_utf8_Copy(configuration.pdp, PdpIpv4, sizeof(configuration.pdp), NULL);
    le_utf8_Copy(configuration.apn, automaticApn, sizeof(configuration.apn), NULL);

    le_mdc_ProfileInfo_t profileList[LE_MDC_PROFILE_LIST_ENTRY_MAX];
    size_t listLen = LE_MDC_PROFILE_LIST_ENTRY_MAX;
    le_mdc_ProfileRef_t profileRef;
    char apnStr[LE_MDC_APN_NAME_MAX_BYTES];
    char userName[LE_MDC_USER_NAME_MAX_BYTES], password[LE_MDC_PASSWORD_NAME_MAX_BYTES];
    le_mdc_Auth_t authType;
    int i;

    LE_TEST_OK(LE_OK == le_mdc_GetProfileList(profileList, &listLen),
               "Test le_mdc_GetProfileList and %d profiles obtained", listLen);

    for (i=0; i<listLen; i++)
    {
        LE_DEBUG("Cellular profile retrieved index %i, type %i, name %s",
                 profileList[i].index, profileList[i].type, profileList[i].name);
        profileRef = le_mdc_GetProfile(profileList[i].index);
        if (!profileRef)
        {
            LE_WARN("Failed to get profile with index %d", profileList[i].index);
        }
        else
        {
            le_mdc_Pdp_t pdpType = le_mdc_GetPDP(profileRef);
            switch (pdpType)
            {
            case LE_MDC_PDP_IPV4:
                LE_TEST_INFO("PDP type is IPV4");
                break;
            case LE_MDC_PDP_IPV6:
                LE_TEST_INFO("PDP type is IPV6");
                break;
            case LE_MDC_PDP_IPV4V6:
                LE_TEST_INFO("PDP type is IPV4V6");
                break;
            default:
                LE_TEST_INFO("PDP type is UNKNOWN");
                break;
            }

            memset(apnStr, 0, sizeof(apnStr));
            LE_TEST_OK(LE_OK == le_mdc_GetAPN(profileRef, apnStr, sizeof(apnStr)),
                       "Test le_mdc_GetAPN()");
            LE_TEST_INFO("le_mdc_GetAPN returns APN: %s", apnStr);

            memset(userName, 0, sizeof(userName));
            memset(password, 0, sizeof(password));
            LE_TEST_OK(LE_OK == le_mdc_GetAuthentication(profileRef,
                                                         &authType,
                                                         userName,
                                                         sizeof(userName),
                                                         password,
                                                         sizeof(password)),
                       "Test le_mdc_GetAuthentication()");
            LE_TEST_INFO("The Authentication type is: %s",
                         authType == LE_MDC_AUTH_PAP ? "PAP" : \
                         (authType == LE_MDC_AUTH_CHAP ? "CHAP" : "NONE"));
            LE_TEST_INFO("The Authentication username: %s, password: %s", userName, password);
        }
    }

    uint32_t profile;

    if ( strncmp(configuration.cid, DefaultCid, sizeof(DefaultCid)) == 0 )
    {
        profile = (uint32_t)LE_MDC_DEFAULT_PROFILE;
    }
    else
    {
        profile = strtol(configuration.cid, NULL, 10);

        if (errno != 0)
        {
            LE_ERROR("Bad profile %d %m", errno);
            exit(EXIT_FAILURE);
        }
    }

    // Get the profile reference
    *profileRefPtr = le_mdc_GetProfile(profile);

    LE_TEST_OK(NULL != *profileRefPtr, "Test profileRefPtr");

    // Check the current state of the cid
    le_mdc_ConState_t state = LE_MDC_DISCONNECTED;

    // Check the state
    LE_TEST_OK(LE_OK == le_mdc_GetSessionState(*profileRefPtr, &state),
               "Test le_mdc_GetSessionState");

    // If already connected, disconnect the session
    if (LE_MDC_CONNECTED == state)
    {
        LE_TEST_OK(le_mdc_StopSession(*profileRefPtr) == LE_OK, "Stop connected session");
    }

    // Set pdp type
    le_mdc_Pdp_t pdp = LE_MDC_PDP_UNKNOWN;

    if (0 == strncmp(configuration.pdp, PdpIpv4, sizeof(PdpIpv4)))
    {
        pdp = LE_MDC_PDP_IPV4;
    }
    else if (0 == strncmp(configuration.pdp, PdpIpv6, sizeof(PdpIpv6)))
    {
        pdp = LE_MDC_PDP_IPV6;
    }
    else if (0 == strncmp(configuration.pdp, PdpIpv4v6, sizeof(PdpIpv4v6)))
    {
        pdp = LE_MDC_PDP_IPV4V6;
    }
    pdp = LE_MDC_PDP_IPV4;
    LE_TEST_OK(LE_OK == le_mdc_SetPDP(*profileRefPtr, pdp), "Test le_mdc_SetPDP");

    // Set APN
    LE_TEST_OK(LE_OK == le_mdc_SetAPN(*profileRefPtr, configuration.apn), "Test le_mdc_SetAPN");

    le_mdc_Auth_t auth = LE_MDC_AUTH_NONE;
    if ('\0' != configuration.auth[0])
    {
        // Set the authentication, username and password
        if (0 == strncmp(configuration.auth, AuthPapChap, sizeof(AuthPapChap)))
        {
            auth = LE_MDC_AUTH_PAP | LE_MDC_AUTH_CHAP;
        }
        // Set the authentication, username and password
        else if (0 == strncmp(configuration.auth, AuthPap, sizeof(AuthPap)))
        {
            auth = LE_MDC_AUTH_PAP;
        }
        else if (0 == strncmp(configuration.auth, AuthChap, sizeof(AuthChap)))
        {
            auth = LE_MDC_AUTH_CHAP;
        }

        if (LE_MDC_AUTH_NONE != auth)
        {
            LE_TEST_OK(LE_OK == le_mdc_SetAuthentication(*profileRefPtr,
                                                         auth,
                                                         configuration.userName,
                                                         configuration.password),
                       "Set authentication");
        }
    }

    LE_INFO("cid: %d pdp: %d apn: %s auth: %d username: %s password: %s",
            le_mdc_GetProfileIndex(*profileRefPtr), pdp, configuration.apn, auth,
            configuration.userName, configuration.password);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the network configuration.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SetNetworkConfiguration
(
    le_mdc_ProfileRef_t profileRef
)
{
    char ipAddr[100] = {0};
    char gatewayAddr[100] = {0};
    char dns1Addr[100] = {0};
    char dns2Addr[100] = {0};
    le_mdc_ConState_t state = LE_MDC_DISCONNECTED;

    // Check the state
    LE_TEST_OK( le_mdc_GetSessionState(profileRef, &state) == LE_OK, "Get session state" );
    LE_TEST_OK( state == LE_MDC_CONNECTED, "Session is connected" );

    // Get IP, gateway and DNS addresses for IPv4 or IPv6 connectivity
    if ( le_mdc_IsIPv4(profileRef) )
    {
        LE_TEST_OK(le_mdc_GetIPv4Address(profileRef, ipAddr, sizeof(ipAddr)) == LE_OK,
                   "Get IPv4 address");
        LE_TEST_INFO("IPv4 address: %s", ipAddr);

        LE_TEST_OK(le_mdc_GetIPv4GatewayAddress(profileRef, gatewayAddr, sizeof(gatewayAddr))
                   == LE_OK,
                   "Get IPv4 gateway address");
        LE_TEST_INFO("IPv4 gateway: %s", gatewayAddr);

        LE_TEST_OK(le_mdc_GetIPv4DNSAddresses( profileRef,
                                               dns1Addr, sizeof(dns1Addr),
                                               dns2Addr, sizeof(dns2Addr)) == LE_OK,
                   "Get IPv4 DNS addresses");
        LE_TEST_INFO("DNS1: %s", dns1Addr);
        LE_TEST_INFO("DNS2: %s", dns2Addr);

//        snprintf(systemCmd, sizeof(systemCmd), "/sbin/route add default gw %s", gatewayAddr);
    }
    else if ( le_mdc_IsIPv6(profileRef) )
    {
        LE_TEST_OK(le_mdc_GetIPv6Address(profileRef, ipAddr, sizeof(ipAddr)) == LE_OK,
                   "Get IPv6 address");
        LE_TEST_INFO("IPv6 address: %s", ipAddr);

        LE_TEST_OK(le_mdc_GetIPv6GatewayAddress(profileRef, gatewayAddr, sizeof(gatewayAddr))
                   == LE_OK,
                   "Get IPv6 gateway address");
        LE_TEST_INFO("IPv6 gateway: %s", gatewayAddr);

        LE_TEST_OK(le_mdc_GetIPv6DNSAddresses( profileRef,
                                              dns1Addr, sizeof(dns1Addr),
                                               dns2Addr, sizeof(dns2Addr)) == LE_OK,
                   "Get IPv6 DNS addresses");
        LE_TEST_INFO("DNS1: %s", dns1Addr);
        LE_TEST_INFO("DNS2: %s", dns2Addr);

//        snprintf(systemCmd, sizeof(systemCmd), "/sbin/route -A inet6 add default gw %s",
//                                                gatewayAddr);
    }

    le_thread_Sleep(5);
/*
    LE_DEBUG("%s", systemCmd);
    LE_ASSERT( system(systemCmd) == 0 );

    // allow fopen to create file with mode=644
    oldMask = umask(022);

    // open the resolver configuration
    resolvFilePtr = fopen("/etc/resolv.conf", "w+");

    if (resolvFilePtr == NULL)
    {
        LE_ERROR("Unable to open resolv.conf: %m");
    }
    LE_ASSERT( resolvFilePtr != NULL );

    LE_ASSERT( fprintf(resolvFilePtr, "nameserver %s\n", dns1Addr) > 0 );

    if (dns2Addr[0] != '\0')
    {
        LE_ASSERT( fprintf(resolvFilePtr, "nameserver %s\n", dns2Addr) > 0 );
    }

    LE_ASSERT( fclose(resolvFilePtr) == 0 );

    // restore old mask
    umask(oldMask);*/
}
//! [Sessions]

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for session state Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void StateChangeHandler
(
    le_mdc_ProfileRef_t profileRef,
    le_mdc_ConState_t ConnectionStatus,
    void* contextPtr
)
{
    LE_DEBUG("\n================================================");
    LE_PRINT_VALUE("%d", (int) le_mdc_GetProfileIndex(profileRef));
    LE_PRINT_VALUE("%u", ConnectionStatus);

    if (ConnectionStatus == LE_MDC_DISCONNECTED)
    {
        // Get disconnection reason
        if (LE_MDC_PDP_IPV4V6 == le_mdc_GetPDP(profileRef))
        {
            LE_PRINT_VALUE("%d", le_mdc_GetDisconnectionReasonExt(profileRef, LE_MDC_PDP_IPV4));
            LE_PRINT_VALUE("%d", le_mdc_GetPlatformSpecificDisconnectionCodeExt(profileRef,
                                                                              LE_MDC_PDP_IPV4));
            LE_PRINT_VALUE("%d", le_mdc_GetDisconnectionReasonExt(profileRef, LE_MDC_PDP_IPV6));
            LE_PRINT_VALUE("%d", le_mdc_GetPlatformSpecificDisconnectionCodeExt(profileRef,
                                                                              LE_MDC_PDP_IPV6));
        }
        else
        {
            LE_PRINT_VALUE("%d", le_mdc_GetDisconnectionReasonExt(profileRef, (le_mdc_Pdp_t)0));
            LE_PRINT_VALUE("%d", le_mdc_GetPlatformSpecificDisconnectionCodeExt(profileRef, (le_mdc_Pdp_t)0));
        }
    }

    LE_DEBUG("\n================================================");
    // Post a semaphore to synchronize the test
    le_sem_Post(TestSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test thread (to run the event loop and calling the event handler).
 *
 */
//--------------------------------------------------------------------------------------------------
static void* TestThread
(
    void* contextPtr
)
{
    le_mdc_ConnectService();

    le_mdc_ProfileRef_t profileRef = *((le_mdc_ProfileRef_t*)contextPtr);

    // Add state handler on the profile
    le_mdc_AddSessionStateHandler(profileRef, StateChangeHandler, NULL);

    // Post a semaphore to synchronize the test
    le_sem_Post(TestSemaphore);

    // Run the event loop
    le_event_RunLoop();

    return NULL;
}



//! [Statistics]
//--------------------------------------------------------------------------------------------------
/**
 * Test the connectivity.
 */
//--------------------------------------------------------------------------------------------------
void TestConnectivity
(
    le_mdc_ProfileRef_t profileRef
)
{
//    int status;
    char systemCmd[200] = {0};
    char itfName[LE_MDC_INTERFACE_NAME_MAX_BYTES] = "\0";
    le_mdc_DataBearerTechnology_t downlinkDataBearerTech;
    le_mdc_DataBearerTechnology_t uplinkDataBearerTech;
    uint64_t rxBytes = 0, txBytes = 0;
    uint64_t latestRxBytes = 0, latestTxBytes = 0;

    LE_TEST_OK(le_mdc_GetDataBearerTechnology(profileRef,
                                              &downlinkDataBearerTech,
                                              &uplinkDataBearerTech) == LE_OK,
               "Get data bearer technology");

    LE_TEST_INFO("downlinkDataBearerTech %d, uplinkDataBearerTech %d",
                 downlinkDataBearerTech, uplinkDataBearerTech);

    // Get interface name
    LE_TEST_OK(le_mdc_GetInterfaceName(profileRef, itfName, LE_MDC_INTERFACE_NAME_MAX_BYTES)
               == LE_OK,
               "Get interface name");

    LE_TEST_INFO("interface name %s", itfName);

    if (le_mdc_IsIPv4(profileRef))
    {
        snprintf(systemCmd, sizeof(systemCmd), "ping -c 4 www.sierrawireless.com -I %s", itfName);
    }
    else
    {
        // TODO ping6 needs raw access to socket and therefore root permissions => find a different
        // way to test the connectivity
        snprintf(systemCmd, sizeof(systemCmd), "ping6 -c 4 www.sierrawireless.com -I %s", itfName);
    }
/*
    // Ping to test the connectivity
    status = system(systemCmd);
    if (WEXITSTATUS(status))
    {
        le_mdc_StopSession(profileRef);
    }
    LE_ASSERT(!WEXITSTATUS(status));
*/
    // Get data counters
    LE_TEST_OK(le_mdc_GetBytesCounters(&rxBytes, &txBytes) == LE_OK,
               "Get RX/TX byte counts");
    latestRxBytes = rxBytes;
    latestTxBytes = txBytes;
    LE_TEST_INFO("rxBytes %"PRIu64", txBytes %"PRIu64, rxBytes, txBytes);

    // Stop data counters and ping to test the connectivity
    LE_TEST_OK(le_mdc_StopBytesCounter() == LE_OK, "Stop byte counter");/*
    status = system(systemCmd);
    if (WEXITSTATUS(status))
    {
        le_mdc_StopSession(profileRef);
    }
    LE_ASSERT(!WEXITSTATUS(status));
*/

    // Get data counters
    LE_TEST_OK(le_mdc_GetBytesCounters(&rxBytes, &txBytes) == LE_OK,
               "Get RX/TX byte counts");
    LE_TEST_INFO("rxBytes %"PRIu64", txBytes %"PRIu64, rxBytes, txBytes);
    LE_TEST_OK(latestRxBytes == rxBytes, "No change in number of bytes received");
    LE_TEST_OK(latestTxBytes == txBytes, "No change in number of bytes sent");

    // Start data counters
    LE_TEST_OK(le_mdc_StartBytesCounter() == LE_OK,
               "Restart byte counter");
}
//! [Statistics]

//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_mdc_ProfileRef_t profileRef  = NULL;
    le_clk_Time_t myTimeout = { 0, 0 };
    myTimeout.sec = 160;
    Testcase_t test = TEST_SYNC;
    le_thread_Ref_t testThread;

    TestSemaphore = le_sem_Create("TestSemaphore",0);
    AsyncTestSemaphore = le_sem_Create("AsyncTestSemaphore",0);

    LE_INFO("======= MDC TEST STARTED =======");

    // Set the configuration
    SetConfiguration(&profileRef);

    testThread = le_thread_Create("MDC_Test", TestThread, &profileRef);

    // Start a thread to treat the event handler.
    le_thread_Start(testThread);

    // Wait for the call of the event handler
    LE_ASSERT_OK(le_sem_WaitWithTimeOut(TestSemaphore, myTimeout));

    while (testsDef[test].testCase != TEST_MAX)
    {
        LE_INFO("======= MDC %s STARTED =======", testsDef[test].testName);
        le_thread_Sleep(1);
        // Start the profile
        switch (testsDef[test].testCase)
        {
            case TEST_SYNC:
                LE_TEST_OK(LE_OK == le_mdc_StartSession(profileRef), "Test le_mdc_StartSession");
                LE_TEST_OK(LE_OK == le_mdc_ResetBytesCounter(), "Test le_mdc_ResetBytesCounter");
            break;

            case TEST_ASYNC:
            {
                le_result_t sessionStart = LE_FAULT;
                le_event_QueueFunctionToThread(testThread,
                                               SessionStartAsync,
                                               profileRef,
                                               &sessionStart);

                // Wait for the call of the event handler
                LE_TEST_OK(le_sem_WaitWithTimeOut(AsyncTestSemaphore, myTimeout) == LE_OK,
                           "Wait for async session notification");
                LE_TEST_OK(LE_FAULT != sessionStart, "Async session started");
                LE_TEST_OK(LE_OK == le_mdc_ResetBytesCounter(), "Test le_mdc_ResetBytesCounter");
            }
            break;

            default:
                LE_ERROR("Unknown test case");
                exit(EXIT_FAILURE);
        }

        // Wait for the call of the event handler
        LE_TEST_OK(LE_OK == le_sem_WaitWithTimeOut(TestSemaphore, myTimeout), "Test le_sem_WaitWithTimeOut");

        // Set the network configuration
        SetNetworkConfiguration(profileRef);

        le_thread_Sleep(5);

        // Test the new interface
        TestConnectivity(profileRef);

        // Stop the session
        switch (testsDef[test].testCase)
        {
            case TEST_SYNC:
                LE_TEST_OK(LE_OK == le_mdc_StopSession(profileRef), "Test le_mdc_StopSession");
                break;

            case TEST_ASYNC:
            {
                le_result_t sessionStart = LE_FAULT;
                le_event_QueueFunctionToThread(testThread,
                                               SessionStopAsync,
                                               profileRef,
                                               &sessionStart);

                // Wait for the call of the event handler
                LE_TEST_OK(LE_OK == le_sem_WaitWithTimeOut(AsyncTestSemaphore, myTimeout), "Test le_sem_WaitWithTimeOut");
                LE_TEST_OK(LE_FAULT != sessionStart, "Session is stopped");
            }
            break;

            default:
                LE_ERROR("Unknown test case");
                exit(EXIT_FAILURE);
        }

        // Wait for the call of the event handler
        LE_TEST_OK(LE_OK == le_sem_WaitWithTimeOut(TestSemaphore, myTimeout), "Test le_sem_WaitWithTimeOut");

        LE_INFO("======= MDC %s PASSED =======", testsDef[test].testName);

        le_thread_Sleep(5);

        test++;
    }

    LE_INFO("======= MDC TEST PASSED =======");

    le_thread_Cancel(testThread);
    pthread_exit(NULL);
}
