/**
 * This module implements the Data Connection Service default route test.
 *
 * This test addresses the use case where multi-PDP contexts are used and the default route is set
 * outside of the data connection service: one application uses a PDP context to connect through
 * the mobile data connection service and configures the default route, and another application
 * uses another PDP context to connect through the data connection service.
 *
 * The following scenario is therefore simulated:
 * - The default route is deactivated in the data connection service
 * - One client connects using DCS and a first data profile through interface A
 * - Another client connects using MDC and a second data profile through interface B
 * - No default route is set at this stage:
 *     -> The test addresses 1 and 2 are not reachable through interface A or B
 * - A route to the test address 1 is added to DCS:
 *     -> The test address 1 is reachable through interface A but not through interface B
 * - The route to the test address 1 is removed:
 *     -> The test addresses 1 and 2 are not reachable through interface A or B
 * - The default route is set for MDC:
 *     -> The test addresses 1 and 2 are reachable only through interface B
 * - A route to the test address 1 is added to DCS:
 *     -> The test address 1 is reachable only through the interface A
 *     -> The test address 2 is reachable only through the interface B
 *
 * Before running the test, you have to configure the data profiles which will be used: profile 1
 * for the DCS connection, profile 2 for the MDC connection. This configuration can be done
 * with the cm data tool or the AT commands.
 * The default route should also be deactivated in the data connection configuration before
 * launching the test:
 * @verbatim
   $ config set dataConnectionService:/routing/useDefaultRoute false bool
   $ app restart dataConnectionService
   $ app start dcsGatewayTest
   @endverbatim
 *
 * @warning This test should be run with a SIM card capable of supporting multi-PDP contexts.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "le_data_interface.h"

//--------------------------------------------------------------------------------------------------
/**
 * The technology string max length
 */
//--------------------------------------------------------------------------------------------------
#define TECH_MAX_LEN            16
#define TECH_MAX_BYTES          (TECH_MAX_LEN+1)

//--------------------------------------------------------------------------------------------------
/**
 * Data profiles to use
 */
//--------------------------------------------------------------------------------------------------
#define DATA_PROFILE_FOR_DCS    1
#define DATA_PROFILE_FOR_MDC    2

//--------------------------------------------------------------------------------------------------
/**
 * Number of seconds to wait for the semaphore
 */
//--------------------------------------------------------------------------------------------------
#define TIME_TO_WAIT            20

//--------------------------------------------------------------------------------------------------
/**
 * Maximal length of a system command
 */
//--------------------------------------------------------------------------------------------------
#define MAX_SYSTEM_CMD_LENGTH   200

//--------------------------------------------------------------------------------------------------
/**
 *  The Data Connection reference
 */
//--------------------------------------------------------------------------------------------------
static le_data_RequestObjRef_t RequestRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 *  List of technologies to use
 */
//--------------------------------------------------------------------------------------------------
static le_data_Technology_t TechList[LE_DATA_MAX] = {};

//--------------------------------------------------------------------------------------------------
/**
 *  The data connection status handler reference
 */
//--------------------------------------------------------------------------------------------------
static le_data_ConnectionStateHandlerRef_t DcsStateHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 *  The mobile data connection status handler reference
 */
//--------------------------------------------------------------------------------------------------
static le_mdc_SessionStateHandlerRef_t MdcStateHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 *  Semaphore used to synchronize the test threads
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t TestSemaphore = NULL;

//--------------------------------------------------------------------------------------------------
/**
 *  Interface used for the DCS connection
 */
//--------------------------------------------------------------------------------------------------
static char DcsInterfaceName[LE_MDC_INTERFACE_NAME_MAX_BYTES] = {0};

//--------------------------------------------------------------------------------------------------
/**
 *  Interface used for the MDC connection
 */
//--------------------------------------------------------------------------------------------------
static char MdcInterfaceName[LE_MDC_INTERFACE_NAME_MAX_BYTES] = {0};

//--------------------------------------------------------------------------------------------------
/**
 *  Addresses to test the connection
 */
//--------------------------------------------------------------------------------------------------
static const char TestAddress1[] = "8.8.8.8";
static const char TestAddress2[] = "8.8.4.4";

//--------------------------------------------------------------------------------------------------
/**
 *  Synchronize the test by waiting for the semaphore
 */
//--------------------------------------------------------------------------------------------------
static void SynchronizeTest
(
    void
)
{
    le_clk_Time_t timeToWait = {TIME_TO_WAIT, 0};

    LE_ASSERT_OK(le_sem_WaitWithTimeOut(TestSemaphore, timeToWait));
}

//--------------------------------------------------------------------------------------------------
/**
 *  This function will set the technologies to use
 */
//--------------------------------------------------------------------------------------------------
static void SetTechnologies
(
    void
)
{
    int techCounter = 0;

    LE_INFO("Setting the technologies to use for the data connection");

    // Add 'Cellular' as the only technology to use
    LE_ASSERT(LE_OK == le_data_SetTechnologyRank(1, LE_DATA_CELLULAR));
    TechList[techCounter++] = LE_DATA_CELLULAR;
}

//--------------------------------------------------------------------------------------------------
/**
 *  This function will get the technologies to use
 */
//--------------------------------------------------------------------------------------------------
static void CheckTechnologies
(
    void
)
{
    int techCounter = 0;

    LE_INFO("Checking the technologies to use for the data connection");

    // Get first technology to use
    le_data_Technology_t technology = le_data_GetFirstUsedTechnology();

    if (technology != TechList[techCounter])
    {
        LE_ERROR("Unexpected first technology %d, should be %d", technology, TechList[techCounter]);
        exit(EXIT_FAILURE);
    }
    else
    {
        techCounter++;
    }

    // Get next technologies to use
    while (LE_DATA_MAX != (technology = le_data_GetNextUsedTechnology()))
    {
        if (technology != TechList[techCounter])
        {
            LE_ERROR("Unexpected technology %d, should be %d", technology, TechList[techCounter]);
            exit(EXIT_FAILURE);
        }
        else
        {
            techCounter++;
        }
    }
}

//! [DefaultRoute]
//--------------------------------------------------------------------------------------------------
/**
 * Set the modem default route for the mobile data connection
 */
//--------------------------------------------------------------------------------------------------
static void SetMdcDefaultRoute
(
    le_mdc_ProfileRef_t profileRef
)
{
    char ipv4GwAddr[LE_MDC_IPV4_ADDR_MAX_BYTES] = {0};
    char ipv6GwAddr[LE_MDC_IPV6_ADDR_MAX_BYTES] = {0};
    char systemCmd[MAX_SYSTEM_CMD_LENGTH] = {0};

    // Get IP gateway for IPv4 or IPv6 connectivity
    if (le_mdc_IsIPv4(profileRef))
    {
        LE_ASSERT_OK(le_mdc_GetIPv4GatewayAddress(profileRef, ipv4GwAddr, sizeof(ipv4GwAddr)));
        snprintf(systemCmd, sizeof(systemCmd), "/sbin/route add default gw %s", ipv4GwAddr);
    }
    else if (le_mdc_IsIPv6(profileRef))
    {
        LE_ASSERT_OK(le_mdc_GetIPv6GatewayAddress(profileRef, ipv6GwAddr, sizeof(ipv6GwAddr)));
        snprintf(systemCmd, sizeof(systemCmd), "/sbin/route -A inet6 add default gw %s",
                                                ipv6GwAddr);
    }
    else
    {
        LE_ERROR("Profile is neither IPv4 nor IPv6!");
        exit(EXIT_FAILURE);
    }

    LE_DEBUG("Trying to execute '%s'", systemCmd);
    LE_ASSERT(0 == system(systemCmd));
}
//! [DefaultRoute]

//--------------------------------------------------------------------------------------------------
/**
 * Set the modem DNS for the mobile data connection
 */
//--------------------------------------------------------------------------------------------------
static void SetMdcDefaultDns
(
    le_mdc_ProfileRef_t profileRef
)
{
    char dns1Addr[LE_MDC_IPV6_ADDR_MAX_BYTES] = {0};
    char dns2Addr[LE_MDC_IPV6_ADDR_MAX_BYTES] = {0};
    FILE* resolvFilePtr;
    mode_t oldMask;

    // Get DNS addresses for IPv4 or IPv6 connectivity
    if (le_mdc_IsIPv4(profileRef))
    {
        LE_ASSERT_OK(le_mdc_GetIPv4DNSAddresses(profileRef,
                                                dns1Addr, sizeof(dns1Addr),
                                                dns2Addr, sizeof(dns2Addr)));
    }
    else if (le_mdc_IsIPv6(profileRef))
    {
        LE_ASSERT_OK(le_mdc_GetIPv6DNSAddresses(profileRef,
                                                dns1Addr, sizeof(dns1Addr),
                                                dns2Addr, sizeof(dns2Addr)));
    }
    else
    {
        LE_ERROR("Profile is neither IPv4 nor IPv6!");
        exit(EXIT_FAILURE);
    }

    // Allow fopen to create file with mode=644
    oldMask = umask(022);

    // Open the resolver configuration
    resolvFilePtr = fopen("/etc/resolv.conf", "w+");
    if (!resolvFilePtr)
    {
        LE_ERROR("Unable to open resolv.conf: %m");
        exit(EXIT_FAILURE);
    }
    if (dns1Addr[0] != '\0')
    {
        LE_ASSERT(fprintf(resolvFilePtr, "nameserver %s\n", dns1Addr) > 0);
    }
    if (dns2Addr[0] != '\0')
    {
        LE_ASSERT(fprintf(resolvFilePtr, "nameserver %s\n", dns2Addr) > 0);
    }
    LE_ASSERT(0 == fclose(resolvFilePtr));

    // Restore old mask
    umask(oldMask);
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the network configuration for the mobile data connection
 */
//--------------------------------------------------------------------------------------------------
static void SetMdcNetworkConfiguration
(
    le_mdc_ProfileRef_t profileRef
)
{
    le_mdc_ConState_t state = LE_MDC_DISCONNECTED;

    // Check the state
    LE_ASSERT_OK(le_mdc_GetSessionState(profileRef, &state));
    LE_ASSERT(LE_MDC_CONNECTED == state);

    // Set the modem default route
    SetMdcDefaultRoute(profileRef);

    // Set the modem default DNS
    SetMdcDefaultDns(profileRef);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Event callback for data connection state changes
 */
//--------------------------------------------------------------------------------------------------
static void DcsStateChangeHandler
(
    const char* intfName,
    bool isConnected,
    void* contextPtr
)
{
    if (isConnected)
    {
        LE_INFO("Profile %d connected through '%s'", DATA_PROFILE_FOR_DCS, intfName);
        snprintf(DcsInterfaceName, sizeof(DcsInterfaceName), "%s", intfName);

        // Post a semaphore to synchronize the test
        le_sem_Post(TestSemaphore);
    }
    else
    {
        LE_INFO("Profile %d disconnected", DATA_PROFILE_FOR_DCS);
        memset(DcsInterfaceName, 0, sizeof(DcsInterfaceName));

        // Post a semaphore to synchronize the test
        le_sem_Post(TestSemaphore);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Event callback for mobile data connection state changes
 */
//--------------------------------------------------------------------------------------------------
static void MdcStateChangeHandler
(
    le_mdc_ProfileRef_t profileRef,
    le_mdc_ConState_t ConnectionStatus,
    void* contextPtr
)
{
    uint32_t profileIndex;

    LE_ASSERT(profileRef);
    profileIndex = le_mdc_GetProfileIndex(profileRef);

    if (LE_MDC_CONNECTED == ConnectionStatus)
    {
        // Retrieve the interface name used by the mobile data connection
        char intfName[LE_MDC_INTERFACE_NAME_MAX_BYTES] = {0};
        LE_ASSERT_OK(le_mdc_GetInterfaceName(profileRef, intfName, sizeof(intfName)));
        LE_INFO("Profile %d connected through '%s'", profileIndex, intfName);
        snprintf(MdcInterfaceName, sizeof(MdcInterfaceName), "%s", intfName);

        // Post a semaphore to synchronize the test
        le_sem_Post(TestSemaphore);
    }
    else if (LE_MDC_DISCONNECTED == ConnectionStatus)
    {
        LE_INFO("Profile %d disconnected", profileIndex);
        memset(MdcInterfaceName, 0, sizeof(MdcInterfaceName));

        // Post a semaphore to synchronize the test
        le_sem_Post(TestSemaphore);
    }
    else
    {
        LE_DEBUG("Profile %d, new connection status: %d", profileIndex, ConnectionStatus);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Data connection service test thread
 */
//--------------------------------------------------------------------------------------------------
static void* DcsTestThread
(
    void* contextPtr
)
{
    le_data_ConnectService();

    // Register handler for data connection state change
    DcsStateHandlerRef = le_data_AddConnectionStateHandler(DcsStateChangeHandler, NULL);
    LE_ASSERT(DcsStateHandlerRef);

    // Set technologies to use
    SetTechnologies();

    // Check if the technologies list was correctly updated
    CheckTechnologies();

    // Set the data profile to use
    LE_ASSERT_OK(le_data_SetCellularProfileIndex(DATA_PROFILE_FOR_DCS));

    // Start the data connection
    if (RequestRef)
    {
        LE_ERROR("A data connection request already exist");
        exit(EXIT_FAILURE);
    }
    RequestRef = le_data_Request();
    LE_ASSERT(RequestRef);
    LE_INFO("Requesting the data connection: %p", RequestRef);

    // Run the event loop
    le_event_RunLoop();

    return 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Mobile data connection test thread
 */
//--------------------------------------------------------------------------------------------------
static void* MdcTestThread
(
    void* contextPtr
)
{
    le_mdc_ConnectService();

    le_mdc_ProfileRef_t profileRef = *((le_mdc_ProfileRef_t*)contextPtr);

    // Add state handler on the profile
    MdcStateHandlerRef = le_mdc_AddSessionStateHandler(profileRef, MdcStateChangeHandler, NULL);
    LE_ASSERT(MdcStateHandlerRef);

    // Start the mobile data connection
    LE_ASSERT_OK(le_mdc_StartSession(profileRef));

    // Run the event loop
    le_event_RunLoop();

    return 0;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Stop mobile data connection
 */
// -------------------------------------------------------------------------------------------------
static void StopMdcConnection
(
    void* param1Ptr,
    void* param2Ptr
)
{
    le_mdc_ProfileRef_t profileRef = *((le_mdc_ProfileRef_t*)param1Ptr);

    // Stop the session
    LE_INFO("Stop MDC connection");
    LE_ASSERT_OK(le_mdc_StopSession(profileRef));
}

// -------------------------------------------------------------------------------------------------
/**
 *  Remove mobile data connection status handler
 */
// -------------------------------------------------------------------------------------------------
static void RemoveMdcHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("Remove MDC status handler");
    LE_ASSERT(MdcStateHandlerRef);
    le_mdc_RemoveSessionStateHandler(MdcStateHandlerRef);

    // Post a semaphore to synchronize the test
    le_sem_Post(TestSemaphore);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Stop data connection
 */
// -------------------------------------------------------------------------------------------------
static void StopDcsConnection
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("Stop DCS connection");
    LE_ASSERT(RequestRef);
    le_data_Release(RequestRef);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Remove mobile data connection status handler
 */
// -------------------------------------------------------------------------------------------------
static void RemoveDcsHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    // Remove the status handler
    LE_INFO("Remove DCS status handler");
    LE_ASSERT(DcsStateHandlerRef);
    le_data_RemoveConnectionStateHandler(DcsStateHandlerRef);

    // Post a semaphore to synchronize the test
    le_sem_Post(TestSemaphore);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Test if the connection to an address is possible through a specific interface by pinging it
 */
// -------------------------------------------------------------------------------------------------
static void TestConnection
(
    const char* addrStr,
    char* interfaceName,
    bool expectedConnection
)
{
    char systemCmd[MAX_SYSTEM_CMD_LENGTH] = {0};
    int result;

    snprintf(systemCmd, sizeof(systemCmd), "ping -c 4 -I %s %s", interfaceName, addrStr);
    LE_DEBUG("Executing '%s'", systemCmd);
    result = system(systemCmd);

    if (expectedConnection)
    {
        LE_ASSERT(0 == result);
    }
    else
    {
        LE_ASSERT(0 != result);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Test main function
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_thread_Ref_t dcsThreadRef, mdcThreadRef;
    le_mdc_ProfileRef_t profileRef;

    LE_INFO("Running data connection service route test");

    // Check if the default route is deactivated in DCS
    if (le_data_GetDefaultRouteStatus())
    {
        LE_ERROR("The default route should be deactivated in DCS!");
        exit(EXIT_FAILURE);
    }

    // Create semaphore used to synchronize the test
    TestSemaphore = le_sem_Create("dcsGatewaySem", 0);

    // Start the thread using the DCS connection
    dcsThreadRef = le_thread_Create("DcsTestThread", DcsTestThread, NULL);
    le_thread_Start(dcsThreadRef);

    // Wait for the DCS connection establishment
    SynchronizeTest();

    // Start the thread using the MDC connection
    profileRef = le_mdc_GetProfile(DATA_PROFILE_FOR_MDC);
    mdcThreadRef = le_thread_Create("MdcTestThread", MdcTestThread, &profileRef);
    le_thread_Start(mdcThreadRef);

    // Wait for the MDC connection establishment
    SynchronizeTest();

    // Both connections are established, but no default route is set:
    // both test addresses shouldn't be reached through any interface
    LE_INFO("No route set");
    TestConnection(TestAddress1, DcsInterfaceName, false);
    TestConnection(TestAddress1, MdcInterfaceName, false);
    TestConnection(TestAddress2, DcsInterfaceName, false);
    TestConnection(TestAddress2, MdcInterfaceName, false);

    // Add a route for the test address 1, which should now be reached through the DCS interface.
    // The test address 2 shouldn't be reached through any interface
    LE_ASSERT_OK(le_data_AddRoute(TestAddress1));
    LE_INFO("Route added for %s through %s interface", TestAddress1, DcsInterfaceName);
    TestConnection(TestAddress1, DcsInterfaceName, true);
    TestConnection(TestAddress1, MdcInterfaceName, false);
    TestConnection(TestAddress2, DcsInterfaceName, false);
    TestConnection(TestAddress2, MdcInterfaceName, false);

    // Remove the route, the test addresses shouldn't be reached through any interface
    LE_ASSERT_OK(le_data_DelRoute(TestAddress1));
    LE_INFO("Route removed for %s through %s interface", TestAddress1, DcsInterfaceName);
    TestConnection(TestAddress1, DcsInterfaceName, false);
    TestConnection(TestAddress1, MdcInterfaceName, false);
    TestConnection(TestAddress2, DcsInterfaceName, false);
    TestConnection(TestAddress2, MdcInterfaceName, false);

    // Add the default route for the MDC connection,
    // both test addresses should be reached only through the MDC interface
    SetMdcNetworkConfiguration(profileRef);
    LE_INFO("Default route set for %s interface", MdcInterfaceName);
    TestConnection(TestAddress1, DcsInterfaceName, false);
    TestConnection(TestAddress1, MdcInterfaceName, true);
    TestConnection(TestAddress2, DcsInterfaceName, false);
    TestConnection(TestAddress2, MdcInterfaceName, true);

    // Add a route for the test address 1, which should now be reached only through the DCS
    // interface. The test address 2 should be reached only through the MDC interface
    LE_ASSERT_OK(le_data_AddRoute(TestAddress1));
    LE_INFO("Route added for %s through %s interface", TestAddress1, DcsInterfaceName);
    TestConnection(TestAddress1, DcsInterfaceName, true);
    TestConnection(TestAddress1, MdcInterfaceName, false);
    TestConnection(TestAddress2, DcsInterfaceName, false);
    TestConnection(TestAddress2, MdcInterfaceName, true);
    LE_ASSERT_OK(le_data_DelRoute(TestAddress1));

    // Stop the mobile data connection
    le_event_QueueFunctionToThread(mdcThreadRef, StopMdcConnection, &profileRef, NULL);

    // Wait for the disconnection
    SynchronizeTest();

    // Remove MDC status handler
    le_event_QueueFunctionToThread(mdcThreadRef, RemoveMdcHandler, &profileRef, NULL);

    // Wait for the removal
    SynchronizeTest();

    // Stop the data connection
    le_event_QueueFunctionToThread(dcsThreadRef, StopDcsConnection, NULL, NULL);

    // Wait for the disconnection
    SynchronizeTest();

    // Remove DCS status handler
    le_event_QueueFunctionToThread(dcsThreadRef, RemoveDcsHandler, NULL, NULL);

    // Wait for the removal
    SynchronizeTest();

    // Cleaning
    le_thread_Cancel(dcsThreadRef);
    le_thread_Cancel(mdcThreadRef);
    le_sem_Delete(TestSemaphore);

    LE_INFO("Data connection service gateway test is successful!");

    exit(EXIT_SUCCESS);
}
