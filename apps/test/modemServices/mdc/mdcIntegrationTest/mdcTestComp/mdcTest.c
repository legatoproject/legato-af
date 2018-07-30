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
static const char automaticApn[] = "auto";
static const char PdpIpv4[] = "ipv4";
static const char PdpIpv6[] = "ipv6";
static const char PdpIpv4v6[] = "ipv4v6";
static const char AuthPap[] = "pap";
static const char AuthChap[] = "chap";
static const char AuthPapChap[] = "pap-chap";
static const char Map[] = "map";
static const char Cid[] = "cid";
static const char Rmnet[] = "rmnet";

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

//! [Data_session]
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
    // Check the configuration file
    FILE* configFilePtr = fopen("/tmp/config.txt","r");
    Configuration_t configuration;

    memset(&configuration, 0, sizeof(Configuration_t));

    // if configuration file absent, use default settings
    if (NULL == configFilePtr)
    {
        le_utf8_Copy(configuration.cid, DefaultCid, sizeof(configuration.cid), NULL);
        le_utf8_Copy(configuration.pdp, PdpIpv4, sizeof(configuration.pdp), NULL);
        le_utf8_Copy(configuration.apn, automaticApn, sizeof(configuration.apn), NULL);
    }
    else
    {
        // Get the configuration from the file
        fseek(configFilePtr, 0, SEEK_END);
        uint32_t len = ftell(configFilePtr);
        char cmdLine[ len + 1 ];
        char* saveLinePtr;
        char* saveParamPtr;
        char* cmdLinePtr;

        memset(cmdLine,0,len+1);
        fseek(configFilePtr, 0, SEEK_SET);
        fread(cmdLine, 1, len, configFilePtr);
        fclose(configFilePtr);

        // Get first line
        cmdLinePtr = strtok_r(cmdLine, "\r\n", &saveLinePtr);

        // Get profile number
        char* cidPtr =  strtok_r(cmdLinePtr, " ", &saveParamPtr);
        LE_ASSERT(NULL != cidPtr);
        le_utf8_Copy(configuration.cid, cidPtr, sizeof(configuration.cid), NULL);

        // Get pdp type
        char* pdpPtr = strtok_r(NULL, " ", &saveParamPtr);
        LE_ASSERT(NULL != pdpPtr);
        le_utf8_Copy(configuration.pdp, pdpPtr, sizeof(configuration.pdp), NULL);

        // Get apn
        char* apnPtr = strtok_r(NULL, " ", &saveParamPtr);
        LE_ASSERT(NULL != apnPtr);
        le_utf8_Copy(configuration.apn, apnPtr, sizeof(configuration.apn), NULL);

        // Get authentication
        char* authPtr = strtok_r(NULL, " ", &saveParamPtr);
        if (NULL != authPtr)
        {
            le_utf8_Copy(configuration.auth, authPtr, sizeof(configuration.auth), NULL);
        }

        // Get username
        char* userNamePtr = strtok_r(NULL, " ", &saveParamPtr);
        if (NULL != userNamePtr)
        {
            le_utf8_Copy(configuration.userName, userNamePtr, sizeof(configuration.userName), NULL);
        }

        // Get password
        char* passwordPtr = strtok_r(NULL, " ", &saveParamPtr);
        if (NULL != passwordPtr)
        {
            le_utf8_Copy(configuration.password, passwordPtr, sizeof(configuration.password), NULL);
        }

        // Get optional lines
        cmdLinePtr = strtok_r(NULL, "\r\n", &saveLinePtr);

        while (cmdLinePtr)
        {
            char* optionPtr = strtok_r(cmdLinePtr, " ", &saveParamPtr);

            // Check mapping cid
            if (strncmp(optionPtr, Map, strlen(Map)) == 0)
            {
                LE_INFO("mapping cid");
                char* cidPtr = strtok_r(NULL, " ", &saveParamPtr);

                if (strncmp(cidPtr, Cid, strlen(Cid)) == 0)
                {
                    int cid = strtol((const char*) cidPtr+strlen(Cid), NULL, 10);

                    if (0 != errno)
                    {
                        LE_ERROR("Bad cid %d %m", errno);
                        exit(EXIT_FAILURE);
                    }

                    LE_INFO("map cid %d", cid);

                    char* rmnetPtr = strtok_r(NULL, " ", &saveParamPtr);

                    if (0 == strncmp(rmnetPtr, Rmnet, strlen(Rmnet)))
                    {
                        int rmnet = strtol(rmnetPtr+strlen(Rmnet), NULL, 10);

                        if (0 != errno)
                        {
                            LE_ERROR("Bad rmnet %d %m", errno);
                            exit(EXIT_FAILURE);
                        }

                        le_mdc_ProfileRef_t profileRef = le_mdc_GetProfile(cid);

                        char string[20];
                        memset(string,0,sizeof(string));
                        snprintf(string,sizeof(string),"rmnet_data%d", rmnet);
                        LE_INFO("cid %d rmnet: %s", cid, string);

                        LE_ASSERT(le_mdc_MapProfileOnNetworkInterface(profileRef, string) == LE_OK);
                    }
                }
            }

            cmdLinePtr = strtok_r(NULL, "\r\n", &saveLinePtr);
        }
    }

    uint32_t profile;

    if ( strncmp(configuration.cid, DefaultCid, sizeof(DefaultCid)) == 0 )
    {
        profile = LE_MDC_DEFAULT_PROFILE;
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

//! [Profile_parameters]

    // Get the profile reference
    *profileRefPtr = le_mdc_GetProfile(profile);
    LE_ASSERT(NULL != *profileRefPtr);

    // Check the current state of the cid
    le_mdc_ConState_t state = LE_MDC_DISCONNECTED;

    // Check the state
    LE_ASSERT(LE_OK == le_mdc_GetSessionState(*profileRefPtr, &state));

    // If already connected, disconnect the session
    if (LE_MDC_CONNECTED == state)
    {
        LE_ASSERT(le_mdc_StopSession(*profileRefPtr) == LE_OK);
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

    LE_ASSERT(LE_OK == le_mdc_SetPDP(*profileRefPtr, pdp));

    // Set APN
    if (0 == strncmp(configuration.apn, automaticApn, sizeof(automaticApn)))
    {
        // Set default APN
        LE_ASSERT(LE_OK == le_mdc_SetDefaultAPN(*profileRefPtr));
    }
    else
    {
        LE_ASSERT(LE_OK == le_mdc_SetAPN(*profileRefPtr, configuration.apn));
    }

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
            LE_ASSERT(LE_OK == le_mdc_SetAuthentication(*profileRefPtr,
                                                auth,
                                                configuration.userName,
                                                configuration.password));
        }
    }

    LE_INFO("cid: %d pdp: %d apn: %s auth: %d username: %s password: %s",
            le_mdc_GetProfileIndex(*profileRefPtr), pdp, configuration.apn, auth,
            configuration.userName, configuration.password);
}
//! [Profiles]
//! [Profile_parameters]

//! [Sessions]
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
    char systemCmd[200] = {0};
    FILE* resolvFilePtr;
    le_mdc_ConState_t state = LE_MDC_DISCONNECTED;
    mode_t oldMask;

    // Check the state
    LE_ASSERT( le_mdc_GetSessionState(profileRef, &state) == LE_OK );
    LE_ASSERT( state == LE_MDC_CONNECTED );

    // Get IP, gateway and DNS addresses for IPv4 or IPv6 connectivity
    if ( le_mdc_IsIPv4(profileRef) )
    {
        LE_ASSERT(le_mdc_GetIPv4Address(profileRef, ipAddr, sizeof(ipAddr)) == LE_OK);
        LE_PRINT_VALUE("%s", ipAddr);

        LE_ASSERT(le_mdc_GetIPv4GatewayAddress(profileRef, gatewayAddr, sizeof(gatewayAddr))
                                                                                          == LE_OK);
        LE_PRINT_VALUE("%s", gatewayAddr);

        LE_ASSERT(le_mdc_GetIPv4DNSAddresses( profileRef,
                                              dns1Addr, sizeof(dns1Addr),
                                              dns2Addr, sizeof(dns2Addr)) == LE_OK );
        LE_PRINT_VALUE("%s", dns1Addr);
        LE_PRINT_VALUE("%s", dns2Addr);

        snprintf(systemCmd, sizeof(systemCmd), "/sbin/route add default gw %s", gatewayAddr);
    }
    else if ( le_mdc_IsIPv6(profileRef) )
    {
        LE_ASSERT(le_mdc_GetIPv6Address(profileRef, ipAddr, sizeof(ipAddr)) == LE_OK);
        LE_PRINT_VALUE("%s", ipAddr);

        LE_ASSERT(le_mdc_GetIPv6GatewayAddress(profileRef, gatewayAddr, sizeof(gatewayAddr))
                                                                                          == LE_OK);
        LE_PRINT_VALUE("%s", gatewayAddr);

        LE_ASSERT(le_mdc_GetIPv6DNSAddresses( profileRef,
                                              dns1Addr, sizeof(dns1Addr),
                                              dns2Addr, sizeof(dns2Addr)) == LE_OK );
        LE_PRINT_VALUE("%s", dns1Addr);
        LE_PRINT_VALUE("%s", dns2Addr);

        snprintf(systemCmd, sizeof(systemCmd), "/sbin/route -A inet6 add default gw %s",
                                                gatewayAddr);
    }

    sleep(5);

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
    umask(oldMask);
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
            LE_PRINT_VALUE("%d", le_mdc_GetDisconnectionReasonExt(profileRef, 0));
            LE_PRINT_VALUE("%d", le_mdc_GetPlatformSpecificDisconnectionCodeExt(profileRef, 0));
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
    int status;
    char systemCmd[200] = {0};
    char itfName[LE_MDC_INTERFACE_NAME_MAX_BYTES] = "\0";
    le_mdc_DataBearerTechnology_t downlinkDataBearerTech;
    le_mdc_DataBearerTechnology_t uplinkDataBearerTech;
    uint64_t rxBytes = 0, txBytes = 0;
    uint64_t latestRxBytes = 0, latestTxBytes = 0;

    LE_ASSERT_OK(le_mdc_GetDataBearerTechnology(profileRef,
                                                &downlinkDataBearerTech,
                                                &uplinkDataBearerTech));

    LE_INFO("downlinkDataBearerTech %d, uplinkDataBearerTech %d",
            downlinkDataBearerTech, uplinkDataBearerTech);

    // Get interface name
    LE_ASSERT_OK(le_mdc_GetInterfaceName(profileRef, itfName, LE_MDC_INTERFACE_NAME_MAX_BYTES));

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

    // Ping to test the connectivity
    status = system(systemCmd);
    if (WEXITSTATUS(status))
    {
        le_mdc_StopSession(profileRef);
    }
    LE_ASSERT(!WEXITSTATUS(status));

    // Get data counters
    LE_ASSERT_OK(le_mdc_GetBytesCounters(&rxBytes, &txBytes));
    latestRxBytes = rxBytes;
    latestTxBytes = txBytes;
    LE_INFO("rxBytes %"PRIu64", txBytes %"PRIu64, rxBytes, txBytes);

    // Stop data counters and ping to test the connectivity
    LE_ASSERT_OK(le_mdc_StopBytesCounter());
    status = system(systemCmd);
    if (WEXITSTATUS(status))
    {
        le_mdc_StopSession(profileRef);
    }
    LE_ASSERT(!WEXITSTATUS(status));

    // Get data counters
    LE_ASSERT_OK(le_mdc_GetBytesCounters(&rxBytes, &txBytes));
    LE_INFO("rxBytes %"PRIu64", txBytes %"PRIu64, rxBytes, txBytes);
    LE_ASSERT(latestRxBytes == rxBytes);
    LE_ASSERT(latestTxBytes == txBytes);

    // Start data counters
    LE_ASSERT_OK(le_mdc_StartBytesCounter());
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
    myTimeout.sec = 120;
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

        // Start the profile
        switch (testsDef[test].testCase)
        {
            case TEST_SYNC:
                LE_ASSERT_OK(le_mdc_StartSession(profileRef));
                LE_ASSERT_OK(le_mdc_ResetBytesCounter());
            break;

            case TEST_ASYNC:
            {
                le_result_t sessionStart = LE_FAULT;
                le_event_QueueFunctionToThread(testThread,
                                               SessionStartAsync,
                                               profileRef,
                                               &sessionStart);

                // Wait for the call of the event handler
                LE_ASSERT_OK(le_sem_WaitWithTimeOut(AsyncTestSemaphore, myTimeout));
                LE_ASSERT_OK(sessionStart);
                LE_ASSERT_OK(le_mdc_ResetBytesCounter());
            }
            break;

            default:
                LE_ERROR("Unknown test case");
                exit(EXIT_FAILURE);
        }

        // Wait for the call of the event handler
        LE_ASSERT_OK(le_sem_WaitWithTimeOut(TestSemaphore, myTimeout));

        // Set the network configuration
        SetNetworkConfiguration(profileRef);

        sleep(5);

        // Test the new interface
        TestConnectivity(profileRef);

        // Stop the session
        switch (testsDef[test].testCase)
        {
            case TEST_SYNC:
                LE_ASSERT_OK(le_mdc_StopSession(profileRef));
                break;

            case TEST_ASYNC:
            {
                le_result_t sessionStart = LE_FAULT;
                le_event_QueueFunctionToThread(testThread,
                                               SessionStopAsync,
                                               profileRef,
                                               &sessionStart);

                // Wait for the call of the event handler
                LE_ASSERT_OK(le_sem_WaitWithTimeOut(AsyncTestSemaphore, myTimeout));
                LE_ASSERT_OK(sessionStart);
            }
            break;

            default:
                LE_ERROR("Unknown test case");
                exit(EXIT_FAILURE);
        }

        // Wait for the call of the event handler
        LE_ASSERT_OK(le_sem_WaitWithTimeOut(TestSemaphore, myTimeout));

        LE_INFO("======= MDC %s PASSED =======", testsDef[test].testName);

        sleep(5);

        test++;
    }

    LE_INFO("======= MDC TEST PASSED =======");

    le_thread_Cancel(testThread);

    exit(EXIT_SUCCESS);
}

