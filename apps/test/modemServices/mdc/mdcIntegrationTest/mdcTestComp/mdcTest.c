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
 * <profile number> <pdp_type> <apn> [<authentification_type> <username> <password>]
 * Where:
 *  - <profile number> is the profile number to be used (or "default" to use the default profile)
 *  - <pdp_type> if the packet data protocol to be used: "ipv4", "ipv6", or "ipv4v6"
 *  - <apn> is the APN to be used
 *  - <authentification_type> (optional): authentification requested: "auth_none" (default), pap",
 *  "chap", "pap-chap"
 *  - <username> (optional): username for authentification
 *  - <password> (optional): password for authentification
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
    char   auth[10];                                // authentification level
    char   userName[LE_MDC_USER_NAME_MAX_LEN];      // username for authentification
    char   password[LE_MDC_PASSWORD_NAME_MAX_LEN];  // password for authentification
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
    // Check the configuration file
    FILE* configFilePtr = fopen("/tmp/config.txt","r");
    Configuration_t configuration;


    memset(&configuration, 0, sizeof(Configuration_t));

    // if configuration file absent, use default settings
    if (configFilePtr == NULL)
    {
        strncpy(configuration.cid, DefaultCid, sizeof(DefaultCid));
        strncpy(configuration.pdp, PdpIpv4, sizeof(PdpIpv4));
        strncpy(configuration.apn, automaticApn, sizeof(automaticApn));
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
        LE_ASSERT(cidPtr != NULL);
        strncpy(configuration.cid, cidPtr, strlen(cidPtr));

        // Get pdp type
        char* pdpPtr = strtok_r(NULL, " ", &saveParamPtr);
        LE_ASSERT(pdpPtr != NULL);
        strncpy(configuration.pdp, pdpPtr, strlen(pdpPtr));

        // Get apn
        char* apnPtr = strtok_r(NULL, " ", &saveParamPtr);
        LE_ASSERT(apnPtr != NULL);
        strncpy(configuration.apn, apnPtr, strlen(apnPtr));

        // Get authentification
        char* authPtr = strtok_r(NULL, " ", &saveParamPtr);
        if (authPtr != NULL)
        {
            strncpy(configuration.auth, authPtr, strlen(authPtr));
        }

        // Get username
        char* userNamePtr = strtok_r(NULL, " ", &saveParamPtr);
        if (userNamePtr != NULL)
        {
            strncpy(configuration.userName, userNamePtr, strlen(userNamePtr));
        }

        // Get password
        char* passwordPtr = strtok_r(NULL, " ", &saveParamPtr);
        if(passwordPtr != NULL)
        {
            strncpy(configuration.password, passwordPtr, strlen(passwordPtr));
        }

        // Get optional lines
        cmdLinePtr = strtok_r(NULL, "\r\n", &saveLinePtr);

        while (cmdLinePtr)
        {
            char* optionPtr = strtok_r(cmdLinePtr, " ", &saveParamPtr);

            // Check mapping cid
            if ( strncmp(optionPtr, Map, strlen(Map)) == 0 )
            {
                LE_INFO("mapping cid");
                char* cidPtr = strtok_r(NULL, " ", &saveParamPtr);

                if ( strncmp(cidPtr, Cid, strlen(Cid)) == 0 )
                {
                    int cid = strtol((const char*) cidPtr+strlen(Cid), NULL, 10);

                    if (errno != 0)
                    {
                        LE_ERROR("Bad cid %d %m", errno);
                        exit(EXIT_FAILURE);
                    }

                    LE_INFO("map cid %d", cid);

                    char* rmnetPtr = strtok_r(NULL, " ", &saveParamPtr);

                    if ( strncmp(rmnetPtr, Rmnet, strlen(Rmnet)) == 0 )
                    {
                        int rmnet = strtol(rmnetPtr+strlen(Rmnet), NULL, 10);

                        if (errno != 0)
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

    // Get the profile reference
    *profileRefPtr = le_mdc_GetProfile(profile);
    LE_ASSERT( *profileRefPtr != NULL );

    // Check the current state of the cid
    le_mdc_ConState_t state = LE_MDC_DISCONNECTED;

    // Check the state
    LE_ASSERT( le_mdc_GetSessionState(*profileRefPtr, &state) == LE_OK );

    // If already connected, disconnect the session
    if ( state == LE_MDC_CONNECTED )
    {
        LE_ASSERT(le_mdc_StopSession(*profileRefPtr) == LE_OK);
    }

    // Set pdp type
    le_mdc_Pdp_t pdp = LE_MDC_PDP_UNKNOWN;

    if ( strncmp(configuration.pdp, PdpIpv4, sizeof(PdpIpv4)) == 0 )
    {
        pdp = LE_MDC_PDP_IPV4;
    }
    else if ( strncmp(configuration.pdp, PdpIpv6, sizeof(PdpIpv6)) == 0 )
    {
        pdp = LE_MDC_PDP_IPV6;
    }
    else if ( strncmp(configuration.pdp, PdpIpv4v6, sizeof(PdpIpv4v6)) == 0 )
    {
        pdp = LE_MDC_PDP_IPV4V6;
    }

    LE_ASSERT( le_mdc_SetPDP(*profileRefPtr, pdp) == LE_OK );

    // Set APN
    if ( strncmp(configuration.apn, automaticApn, sizeof(automaticApn)) == 0 )
    {
        // Set default APN
        LE_ASSERT( le_mdc_SetDefaultAPN(*profileRefPtr) == LE_OK );
    }
    else
    {
        LE_ASSERT( le_mdc_SetAPN(*profileRefPtr, configuration.apn) == LE_OK );
    }

    le_mdc_Auth_t auth = LE_MDC_AUTH_NONE;
    if ( configuration.auth[0] != '\0' )
    {
        // Set the authentification, username and password
        if ( strncmp(configuration.auth, AuthPapChap, sizeof(AuthPapChap)) == 0 )
        {
            auth = LE_MDC_AUTH_PAP | LE_MDC_AUTH_CHAP;
        }
        // Set the authentification, username and password
        else if ( strncmp(configuration.auth, AuthPap, sizeof(AuthPap)) == 0 )
        {
            auth = LE_MDC_AUTH_PAP;
        }
        else if ( strncmp(configuration.auth, AuthChap, sizeof(AuthChap)) == 0 )
        {
            auth = LE_MDC_AUTH_CHAP;
        }

        if (auth != LE_MDC_AUTH_NONE)
        {
            LE_ASSERT( le_mdc_SetAuthentication( *profileRefPtr,
                                                 auth,
                                                 configuration.userName,
                                                 configuration.password ) == LE_OK );
        }
    }

    LE_INFO("cid: %d pdp: %d apn: %s auth: %d username: %s password: %s", profile, pdp,
            configuration.apn, auth, configuration.userName, configuration.password);
}
//! [Profiles]

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
        LE_PRINT_VALUE("%d", le_mdc_GetDisconnectionReason(profileRef));
        LE_PRINT_VALUE("%d", le_mdc_GetPlatformSpecificDisconnectionCode(profileRef));
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
}



//! [Statistics]
//--------------------------------------------------------------------------------------------------
/**
 * Test the connectivity.
 *
 */
//--------------------------------------------------------------------------------------------------
void TestConnectivity
(
    le_mdc_ProfileRef_t profileRef
)
{
    le_result_t res;
    char systemCmd[200] = {0};
    char itfName[LE_MDC_INTERFACE_NAME_MAX_BYTES]="\0";

    le_mdc_DataBearerTechnology_t downlinkDataBearerTech;
    le_mdc_DataBearerTechnology_t uplinkDataBearerTech;

    LE_ASSERT(le_mdc_GetDataBearerTechnology(profileRef,
                                             &downlinkDataBearerTech,
                                             &uplinkDataBearerTech) == LE_OK);

    LE_INFO("downlinkDataBearerTech %d, uplinkDataBearerTech %d",
                downlinkDataBearerTech, uplinkDataBearerTech);

    // Get interface name
    LE_ASSERT(le_mdc_GetInterfaceName(profileRef, itfName, LE_MDC_INTERFACE_NAME_MAX_BYTES)
                                                                                          == LE_OK);

    if ( le_mdc_IsIPv4(profileRef) )
    {
        snprintf(systemCmd, sizeof(systemCmd), "ping -c 4 www.sierrawireless.com -I %s", itfName);
    }
    else
    {
        // TODO ping6 needs raw access to socket and therefore root permissions => find a different
        // way to test the connectivity
        snprintf(systemCmd, sizeof(systemCmd), "ping6 -c 4 www.sierrawireless.com -I %s", itfName);
    }

    res = system(systemCmd);
    if (res != LE_OK)
    {
        le_mdc_StopSession(profileRef);
    }
    LE_ASSERT(res == LE_OK);

    // Get data counters
    uint64_t rxBytes=0, txBytes=0;
    LE_ASSERT( le_mdc_GetBytesCounters(&rxBytes, &txBytes) == LE_OK );

    LE_INFO("rxBytes %ld, txBytes %ld", (long int) rxBytes, (long int) txBytes);
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
    le_result_t res;
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
    res = le_sem_WaitWithTimeOut(TestSemaphore, myTimeout);
    LE_ASSERT(res == LE_OK);

    while (testsDef[test].testCase != TEST_MAX)
    {
        LE_INFO("======= MDC %s STARTED =======", testsDef[test].testName);

        // start the profile
        switch (testsDef[test].testCase)
        {
            case TEST_SYNC:
                LE_ASSERT( le_mdc_StartSession(profileRef) == LE_OK );

                LE_ASSERT( le_mdc_ResetBytesCounter() == LE_OK );
            break;
            case TEST_ASYNC:
            {
                le_result_t sessionStart = LE_FAULT;

                le_event_QueueFunctionToThread(testThread,
                                   SessionStartAsync,
                                   profileRef,
                                   &sessionStart);

                // Wait for the call of the event handler
                res = le_sem_WaitWithTimeOut(AsyncTestSemaphore, myTimeout);
                LE_ASSERT(res == LE_OK);
                LE_ASSERT(sessionStart == LE_OK);

                LE_ASSERT( le_mdc_ResetBytesCounter() == LE_OK );
            }
            break;
            default:
                LE_ERROR("Unknown test case");
                exit(EXIT_FAILURE);
        }

        // Wait for the call of the event handler
        res = le_sem_WaitWithTimeOut(TestSemaphore, myTimeout);
        LE_ASSERT(res == LE_OK);

        // Set the network configuration
        SetNetworkConfiguration(profileRef);

        sleep(5);

        // Test the new interface
        TestConnectivity(profileRef);

        // Stop the session
        switch (testsDef[test].testCase)
        {
            case TEST_SYNC:
                LE_ASSERT(le_mdc_StopSession(profileRef) == LE_OK);
            break;
            case TEST_ASYNC:
            {
                le_result_t sessionStart = LE_FAULT;
                le_event_QueueFunctionToThread(testThread,
                                   SessionStopAsync,
                                   profileRef,
                                   &sessionStart);

                // Wait for the call of the event handler
                res = le_sem_WaitWithTimeOut(AsyncTestSemaphore, myTimeout);
                LE_ASSERT(res == LE_OK);
                LE_ASSERT(sessionStart == LE_OK);
            }
            break;
            default:
                LE_ERROR("Unknown test case");
                exit(EXIT_FAILURE);
        }


        // Wait for the call of the event handler
        res = le_sem_WaitWithTimeOut(TestSemaphore, myTimeout);
        LE_ASSERT(res == LE_OK);

        LE_INFO("======= MDC %s PASSED =======", testsDef[test].testName);

        sleep(5);

        test++;
    }

    LE_INFO("======= MDC TEST PASSED =======");

    le_thread_Cancel(testThread);

    exit(EXIT_SUCCESS);
}

