/**
 * This module implements the unit tests for the Data Connection service.
 *
 * Tested API:
 * - le_data_SetTechnologyRank
 * - le_data_GetFirstUsedTechnology
 * - le_data_GetNextUsedTechnology
 * - le_data_GetTechnology
 * - le_data_AddConnectionStateHandler
 * - le_data_Request
 * - le_data_Release
 * - le_data_RemoveConnectionStateHandler
 *
 * Unit test steps:
 *  1. Test DCS API to set and get the technologies list
 *      a. Set list with Cellular at rank 2 and Wifi at rank 5
 *      b. Retrieve technologies from list and check if list is coherent
 *      c. Set list with Wifi at rank 1 and Cellular at rank 2
 *      d. Retrieve technologies from list and check if list is coherent
 *  2. Test Data Connection Service
 *      a. Add application handlers to be notified of DCS events
 *      b. Several applications request a data connection through DCS
 *      c. No Wifi configuration available in DCS
 *      d. Connection is established with cellular technology
 *      e. Wifi configuration available in DCS
 *      f. Cellular connection is lost, Wifi connection is established
 *      g. The applications release the DCS connection
 *      h. DCS events handlers are removed
 *      i. Simulate a Wifi event to check that handlers are removed
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_mdc.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Number of clients for the data connection service
 */
//--------------------------------------------------------------------------------------------------
#define CLIENTS_NB  2

//--------------------------------------------------------------------------------------------------
/**
 *  DCS technology string size
 */
//--------------------------------------------------------------------------------------------------
#define TECH_STR_MAX_SIZE   30

//--------------------------------------------------------------------------------------------------
/**
 *  Short semaphore timeout in seconds
 */
//--------------------------------------------------------------------------------------------------
#define SHORT_TIMEOUT   1

//--------------------------------------------------------------------------------------------------
/**
 *  Long semaphore timeout in seconds
 */
//--------------------------------------------------------------------------------------------------
#define LONG_TIMEOUT    20

//--------------------------------------------------------------------------------------------------
/**
 *  List of technologies to use
 */
//--------------------------------------------------------------------------------------------------
static le_data_Technology_t TechList[LE_DATA_MAX] = {};

//--------------------------------------------------------------------------------------------------
/**
 *  Expected interface name used by data connection
 */
//--------------------------------------------------------------------------------------------------
static char ExpectedIntf[LE_DATA_INTERFACE_NAME_MAX_BYTES] = {0};

//--------------------------------------------------------------------------------------------------
/**
 *  Expected data connection status
 */
//--------------------------------------------------------------------------------------------------
static bool ExpectedConnectionStatus = false;

// -------------------------------------------------------------------------------------------------
/**
 *  Application context structure
 */
// -------------------------------------------------------------------------------------------------
typedef struct
{
    uint32_t                            appId;
    le_sem_Ref_t                        appSemaphore;
    le_thread_Ref_t                     appThreadRef;
    le_data_ConnectionStateHandlerRef_t appStateHandlerRef;
    le_data_RequestObjRef_t             appRequestRef;
} AppContext_t;

// -------------------------------------------------------------------------------------------------
/**
 *  Application contexts
 */
// -------------------------------------------------------------------------------------------------
static AppContext_t AppCtx[CLIENTS_NB];


//--------------------------------------------------------------------------------------------------
// Utility functions
//--------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------
/**
 *  Convert DCS technology to a string
 */
// -------------------------------------------------------------------------------------------------
static void TechnologyStr
(
    le_data_Technology_t technology,
    char* techStrPtr,
    size_t techStrLen
)
{
    switch (technology)
    {
        case LE_DATA_WIFI:
            snprintf(techStrPtr, techStrLen, "Wifi");
            break;

        case LE_DATA_CELLULAR:
            snprintf(techStrPtr, techStrLen, "Cellular");
            break;

        case LE_DATA_MAX:
        default:
            snprintf(techStrPtr, techStrLen, "Unknown technology");
            break;
    }
}

// -------------------------------------------------------------------------------------------------
/**
 *  Check the list of technologies
 */
// -------------------------------------------------------------------------------------------------
static void TestTechnologies
(
    void
)
{
    int techCounter = 0;

    LE_INFO("Check the technologies list");

    // Get first technology to use
    le_data_Technology_t technology = le_data_GetFirstUsedTechnology();
    LE_ASSERT(technology == TechList[techCounter]);
    techCounter++;

    // Get next technologies to use
    while (LE_DATA_MAX != (technology = le_data_GetNextUsedTechnology()))
    {
        LE_ASSERT(technology == TechList[techCounter]);
        techCounter++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 *  Synchronize test thread (i.e. main) and application threads
 */
//--------------------------------------------------------------------------------------------------
static void SynchronizeTest
(
    void
)
{
    int i = 0;
    le_clk_Time_t timeToWait = {LONG_TIMEOUT, 0};

    for (i = 0; i < CLIENTS_NB; i++)
    {
        LE_ASSERT_OK(le_sem_WaitWithTimeOut(AppCtx[i].appSemaphore, timeToWait));
    }
}


//--------------------------------------------------------------------------------------------------
// Test functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Set and Get technologies in the DCS list
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void Testle_data_Technologies
(
    void
)
{
    int techCounter = 0;

    // Set initial technologies list: 2=Cellular, 5=Wifi
    LE_INFO("Set technologies: 2=Cellular, 5=Wifi");
    LE_ASSERT_OK(le_data_SetTechnologyRank(2, LE_DATA_CELLULAR));
    TechList[techCounter++] = LE_DATA_CELLULAR;
    LE_ASSERT_OK(le_data_SetTechnologyRank(5, LE_DATA_WIFI));
    TechList[techCounter++] = LE_DATA_WIFI;

    // Check technologies list
    TestTechnologies();

    // Change technologies list: 1=Wifi, 2=Cellular
    LE_INFO("Set technologies: 1=Wifi, 2=Cellular");
    techCounter = 0;
    LE_ASSERT_OK(le_data_SetTechnologyRank(1, LE_DATA_WIFI));
    TechList[techCounter++] = LE_DATA_WIFI;
    LE_ASSERT_OK(le_data_SetTechnologyRank(2, LE_DATA_CELLULAR));
    TechList[techCounter++] = LE_DATA_CELLULAR;

    // Check technologies list
    TestTechnologies();
}

// -------------------------------------------------------------------------------------------------
/**
 *  Request data connection
 */
// -------------------------------------------------------------------------------------------------
static void DcsRequest
(
    void* param1Ptr,
    void* param2Ptr
)
{
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;

    // Request data connection
    LE_INFO("Request of data connection by application #%d", appCtxPtr->appId);
    appCtxPtr->appRequestRef = le_data_Request();
    LE_ASSERT(NULL != appCtxPtr->appRequestRef);
    LE_INFO("Received reference: %p", appCtxPtr->appRequestRef);
}

// -------------------------------------------------------------------------------------------------
/**
 *  Release data connection
 */
// -------------------------------------------------------------------------------------------------
static void DcsRelease
(
    void* param1Ptr,
    void* param2Ptr
)
{
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;

    // Release data connection
    LE_INFO("Release of data connection by application #%d", appCtxPtr->appId);
    LE_INFO("Releasing the data connection reference %p", appCtxPtr->appRequestRef);
    le_data_Release(appCtxPtr->appRequestRef);
    appCtxPtr->appRequestRef = NULL;
}

// -------------------------------------------------------------------------------------------------
/**
 *  Remove data connection status handler
 */
// -------------------------------------------------------------------------------------------------
static void DcsRemoveHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    AppContext_t* appCtxPtr = (AppContext_t*) param1Ptr;

    // Deregister handler
    le_data_RemoveConnectionStateHandler(appCtxPtr->appStateHandlerRef);
    LE_INFO("DcsStateHandler %p removed for application #%d",
             appCtxPtr->appStateHandlerRef, appCtxPtr->appId);
    appCtxPtr->appStateHandlerRef = NULL;

    le_sem_Post(appCtxPtr->appSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Event callback for data connection state changes.
 */
//--------------------------------------------------------------------------------------------------
static void DcsStateHandler
(
    const char* intfName,
    bool isConnected,
    void* contextPtr
)
{
    AppContext_t* appCtxPtr = (AppContext_t*)contextPtr;

    LE_INFO("Received connection status %d for interface '%s'", isConnected, intfName);
    if (isConnected)
    {
        le_data_Technology_t currentTech = le_data_GetTechnology();
        char currentTechStr[TECH_STR_MAX_SIZE];
        TechnologyStr(currentTech, currentTechStr, sizeof(currentTechStr));
        LE_INFO("Currently used technology: %d=%s", currentTech, currentTechStr);
    }

    // Check if connection status is coherent
    LE_ASSERT(ExpectedConnectionStatus == isConnected);

    // Check interface name when connected
    if (isConnected)
    {
        LE_ASSERT(0 == strncmp(intfName, ExpectedIntf, strlen(ExpectedIntf)));
    }

    // Note: the technology retrieved by le_data_GetTechnology() cannot be tested again an expected
    // value as it changes as soon as the current technology is not available anymore.

    le_sem_Post(appCtxPtr->appSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 *  Thread used to simulate an application
 */
//--------------------------------------------------------------------------------------------------
static void* AppHandler
(
    void* ctxPtr
)
{
    AppContext_t* appCtxPtr = (AppContext_t*) ctxPtr;

    // Register handler for data connection state change
    appCtxPtr->appStateHandlerRef = le_data_AddConnectionStateHandler(DcsStateHandler, ctxPtr);
    LE_ASSERT(NULL != appCtxPtr->appStateHandlerRef);
    LE_INFO("DcsStateHandler %p added for application #%d",
             appCtxPtr->appStateHandlerRef, appCtxPtr->appId);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(appCtxPtr->appSemaphore);

    // Run the event loop
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test data connection service
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void Testle_data_Service
(
    void
)
{
    int i;
    int32_t profileIndex, defaultProfileIndex;
    le_result_t result;
    uint32_t loop;
    uint16_t year, month, day, hour, minute, second, millisecond;

    // Initialize application contexts
    memset(AppCtx, 0, CLIENTS_NB * sizeof(AppContext_t));

    // Test time APIs
    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetDate(NULL, &month, &day));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetDate(&year, NULL, &day));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetDate(&year, &month, NULL));
    LE_ASSERT(LE_FAULT == le_data_GetDate(&year, &month, &day));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetTime(NULL, &minute, &second, &millisecond));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetTime(&hour, NULL, &second, &millisecond));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetTime(&hour, &minute, NULL, &millisecond));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetTime(&hour, &minute, &second, NULL));
    LE_ASSERT(LE_FAULT == le_data_GetTime(&hour, &minute, &second, &millisecond));

    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetDateTime(NULL, &month, &day, &hour, &minute, &second, &millisecond));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetDateTime(&year, NULL, &day, &hour, &minute, &second, &millisecond));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetDateTime(&year, &month, NULL, &hour, &minute, &second, &millisecond));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetDateTime(&year, &month, &day, NULL, &minute, &second, &millisecond));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetDateTime(&year, &month, &day, &hour, NULL, &second, &millisecond));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetDateTime(&year, &month, &day, &hour, &minute, NULL, &millisecond));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_GetDateTime(&year, &month, &day, &hour, &minute, &second, NULL));
    LE_ASSERT(LE_FAULT == le_data_GetDateTime(&year, &month, &day, &hour, &minute, &second, &millisecond));

    LE_ASSERT(le_data_GetDefaultRouteStatus());

    // Read the default profile index
    defaultProfileIndex = le_data_GetCellularProfileIndex();

    // Set RAT to GSM
    le_mrcTest_SetRatInUse(LE_MRC_RAT_GSM);
    // Valid profile index values: 1 - 16
    result = le_data_SetCellularProfileIndex(0);
    LE_ASSERT(LE_BAD_PARAMETER == result);
    profileIndex = le_data_GetCellularProfileIndex();
    LE_ASSERT(defaultProfileIndex == profileIndex);

    for (loop = 17; loop < 200; loop++)
    {
        result = le_data_SetCellularProfileIndex(loop);
        LE_ASSERT(LE_BAD_PARAMETER == result);
        profileIndex = le_data_GetCellularProfileIndex();
        LE_ASSERT(defaultProfileIndex == profileIndex);
    }

    for (loop = 1; loop < 17; loop++)
    {
        result = le_data_SetCellularProfileIndex(loop);
        LE_ASSERT(LE_OK == result);
        profileIndex = le_data_GetCellularProfileIndex();
        LE_ASSERT(loop == profileIndex);
    }


    result = le_data_SetCellularProfileIndex(LE_MDC_DEFAULT_PROFILE);
    LE_ASSERT(LE_OK == result);
    profileIndex = le_data_GetCellularProfileIndex();
    LE_ASSERT(defaultProfileIndex == profileIndex);

    // Set RAT to CDMA
    le_mrcTest_SetRatInUse(LE_MRC_RAT_CDMA);
    // Valid profile index values: 101 - 107
    for (loop = 0; loop < 101; loop++)
    {
        result = le_data_SetCellularProfileIndex(loop);
        LE_ASSERT(LE_BAD_PARAMETER == result);
        profileIndex = le_data_GetCellularProfileIndex();
        LE_ASSERT(defaultProfileIndex == profileIndex);
    }

    for (loop = 108; loop < 200; loop++)
    {
        result = le_data_SetCellularProfileIndex(loop);
        LE_ASSERT(LE_BAD_PARAMETER == result);
        profileIndex = le_data_GetCellularProfileIndex();
        LE_ASSERT(defaultProfileIndex == profileIndex);
    }

    for (loop = 101; loop < 108; loop++)
    {
        result = le_data_SetCellularProfileIndex(loop);
        LE_ASSERT(LE_OK == result);
        profileIndex = le_data_GetCellularProfileIndex();
        LE_ASSERT(loop == profileIndex);
    }

    result = le_data_SetCellularProfileIndex(LE_MDC_DEFAULT_PROFILE);
    LE_ASSERT(LE_OK == result);
    profileIndex = le_data_GetCellularProfileIndex();
    LE_ASSERT(defaultProfileIndex == profileIndex);

    // Start threads in order to simulate multi-users of data connection service
    for (i = 0; i < CLIENTS_NB; i++)
    {
        char threadString[20] = {0};
        char semString[20]    = {0};
        snprintf(threadString, 20, "app%dHandler", i);
        snprintf(semString, 20, "app%dSem", i);
        AppCtx[i].appId = i;
        // Create a semaphore to coordinate the test
        AppCtx[i].appSemaphore = le_sem_Create(semString, 0);
        AppCtx[i].appThreadRef = le_thread_Create(threadString, AppHandler, &AppCtx[i]);
        le_thread_Start(AppCtx[i].appThreadRef);
    }

    // Wait for threads start
    SynchronizeTest();

    // Technology list is 1=Wifi, 2=Cellular and Wifi is not configured:
    // DCS should connect through cellular technology when a connection is requested
    memset(ExpectedIntf, '\0', sizeof(ExpectedIntf));
    snprintf(ExpectedIntf, sizeof(ExpectedIntf), MDC_INTERFACE_NAME);
    ExpectedConnectionStatus = true;

    // Each applications requests a data connection: the API has therefore to be called
    // by the application threads
    result = le_data_SetCellularProfileIndex(PA_MDC_MIN_INDEX_3GPP2_PROFILE);
    for (i = 0; i < CLIENTS_NB; i++)
    {
        le_event_QueueFunctionToThread(AppCtx[i].appThreadRef, DcsRequest, &AppCtx[i], NULL);

        // Wait for the handlers call
        SynchronizeTest();
    }

    LE_INFO("Clients started");

    LE_ASSERT(LE_BUSY == le_data_SetCellularProfileIndex(LE_MDC_DEFAULT_PROFILE));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_AddRoute("216.58.206.45.228"));
    LE_ASSERT(LE_BAD_PARAMETER == le_data_DelRoute("216.58.206.45.228"));
    LE_ASSERT_OK(le_data_AddRoute("216.58.206.45"));
    LE_ASSERT_OK(le_data_DelRoute("216.58.206.45"));

    // Test time APIs
    LE_ASSERT_OK(le_data_GetDate(&year, &month, &day));
    LE_ASSERT_OK(le_data_GetTime(&hour, &minute, &second, &millisecond));

    // Configure Wifi to be able to use it
    le_cfg_IteratorRef_t wifiTestIteratorRef = (le_cfg_IteratorRef_t)0x01234567;
    le_cfgTest_SetStringNodeValue(wifiTestIteratorRef, CFG_NODE_SSID, "TestSSID");

    LE_INFO("Simulate cellular disconnection");

    // Note: interface name is not available when cellular is disconnected
    memset(ExpectedIntf, '\0', sizeof(ExpectedIntf));
    ExpectedConnectionStatus = false;
    // Simulate a cellular disconnection
    le_dcsTest_SimulateConnEvent(LE_DCS_EVENT_DOWN);

    // Wait for the handlers call
    SynchronizeTest();    // To catch the connection event simulated above
    SynchronizeTest();    // To catch the internally generated down event upon the le_dcs_Stop call
                          // for the current tech before the next tech is tried

    LE_INFO("Wait for Wifi connection");

    // Wifi should now be used to connect
    memset(ExpectedIntf, '\0', sizeof(ExpectedIntf));
    snprintf(ExpectedIntf, sizeof(ExpectedIntf), WIFI_INTERFACE_NAME);
    ExpectedConnectionStatus = true;

    // Wait for the handlers call
    SynchronizeTest();

    // Disconnection request
    ExpectedConnectionStatus = false;
    // Each application releases the data connection: the API has therefore to be called
    // by the application threads
    for (i = 0; i < CLIENTS_NB; i++)
    {
        le_event_QueueFunctionToThread(AppCtx[i].appThreadRef, DcsRelease, &AppCtx[i], NULL);
    }

    // All data connection released, wait for the disconnection notifications
    LE_INFO("Wait for Wifi disconnection");
    SynchronizeTest();

    // Each application removes the data connection status handler: the API has therefore
    // to be called by the application threads
    for (i = 0; i < CLIENTS_NB; i++)
    {
        le_event_QueueFunctionToThread(AppCtx[i].appThreadRef, DcsRemoveHandler,
                                       &AppCtx[i], NULL);
    }
    // Wait for handlers removal
    SynchronizeTest();

    // Simulate a wifi disconnection
    LE_INFO("Simulate Wifi disconnection");
    le_wifiClientTest_SimulateEvent(LE_WIFICLIENT_EVENT_DISCONNECTED);
    // Wait for the semaphore timeout to check that handlers are not called
    le_clk_Time_t timeToWait = {SHORT_TIMEOUT, 0};
    for (i = 0; i < CLIENTS_NB; i++)
    {
        LE_ASSERT(LE_TIMEOUT == le_sem_WaitWithTimeOut(AppCtx[i].appSemaphore, timeToWait));
    }

    LE_ASSERT(LE_BAD_PARAMETER == le_data_AddRoute("216.58.206.45.228"));
}

//--------------------------------------------------------------------------------------------------
/**
 * This thread is used to launch the data connection service unit tests
 */
//--------------------------------------------------------------------------------------------------
static void* DcsUnitTestThread
(
    void* contextPtr
)
{
    LE_INFO("DCS UT Thread Started");

    LE_INFO("======== Test technologies list ========");
    Testle_data_Technologies();

    LE_INFO("======== Test Data Connection service ========");
    Testle_data_Service();

    LE_INFO("======== Test Data Connection success! ========");
    exit(EXIT_SUCCESS);

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Main of the test
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // To reactivate for all DEBUG logs
    // le_log_SetFilterLevel(LE_LOG_DEBUG);

    LE_INFO("======== Start UnitTest of Data Connection service ========");

    // Create and start the unit test thread
    le_thread_Start(le_thread_Create("DCS UT Thread", DcsUnitTestThread, NULL));
}
