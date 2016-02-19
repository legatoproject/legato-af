/**
 * This module implements the unit tests for temperature API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_temp_simu.h"
#include "le_temp_local.h"

#define NB_CLIENT 2

// Task context
typedef struct
{
    uint32_t appId;
    le_thread_Ref_t                     appThreadRef;
    le_temp_ThresholdEventHandlerRef_t  eventHandler;
    le_temp_ThresholdStatus_t           expectedStatus;
} AppContext_t;

static le_sem_Ref_t    ThreadSemaphore;
static AppContext_t AppCtx[NB_CLIENT];
static le_clk_Time_t TimeToWait ={ 0, 1000000 };
static le_temp_ThresholdStatus_t ExpectedStatus;

//--------------------------------------------------------------------------------------------------
/**
 * Synchronize test threads and tasks
 *
 */
//--------------------------------------------------------------------------------------------------
static void SynchTest( void )
{
    int i=0;

    for (;i<NB_CLIENT;i++)
    {
        LE_ASSERT(le_sem_WaitWithTimeOut(ThreadSemaphore, TimeToWait) == LE_OK);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Temperature event handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void ThresholdEventHandler
(
    le_temp_ThresholdStatus_t event,
    void* contextPtr
)
{
    le_temp_ThresholdStatus_t* expectedStatusPtr = contextPtr;

    LE_ASSERT( *expectedStatusPtr == event );

    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test tasks: this function handles the task and run an eventLoop
 *
 */
//--------------------------------------------------------------------------------------------------
static void* AppHandler
(
    void* ctxPtr
)
{
    AppContext_t * appCtxPtr = (AppContext_t*) ctxPtr;

    LE_DEBUG("App id: %d", appCtxPtr->appId);

    // Subscribe to temperature event handler
    appCtxPtr->eventHandler = le_temp_AddThresholdEventHandler(NULL, &ExpectedStatus);
    LE_ASSERT(appCtxPtr->eventHandler == NULL);

    appCtxPtr->eventHandler=le_temp_AddThresholdEventHandler(ThresholdEventHandler,&ExpectedStatus);
    LE_ASSERT(appCtxPtr->eventHandler != NULL);

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);

    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the temperature event handler
 *
 * API tested:
 *  le_temp_AddThresholdEventHandler
 *  call of the handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_Temp_AddHandlers
(
    void
)
{
    int i;

    // int app context
    memset(AppCtx, 0, NB_CLIENT*sizeof(AppContext_t));

    // Start tasks: simulate multi-user of le_sim
    // each thread subcribes to state handler using le_sim_AddNewStateHandler
    for (i=0; i < NB_CLIENT; i++)
    {
        char string[20]={0};
        snprintf(string,20,"app%dhandler", i);
        AppCtx[i].appId = i;
        AppCtx[i].appThreadRef = le_thread_Create(string, AppHandler, &AppCtx[i]);
        le_thread_Start(AppCtx[i].appThreadRef);
    }

    // Wait that the tasks have started before continuing the test
    SynchTest();

    ExpectedStatus = 0;

    for (; ExpectedStatus <= LE_TEMP_PLATFORM_LOW_CRITICAL; ExpectedStatus++)
    {
        pa_tempSimu_TriggerEventReport(ExpectedStatus);

        // wait the handlers' calls
        SynchTest();
    }

    // Check that no more call of the semaphore
    LE_ASSERT(le_sem_GetValue(ThreadSemaphore) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove temperature event handlers
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    AppContext_t * appCtxPtr = (AppContext_t*) param1Ptr;

    le_temp_RemoveThresholdEventHandler( NULL );
    le_temp_RemoveThresholdEventHandler( appCtxPtr->eventHandler );

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the temperature remove handlers
 *
 * API tested:
 *  le_temp_RemoveThresholdEventHandler
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_Temp_RemoveHandlers
(
    void
)
{
    int i;

    // Remove handlers: add le_temp_RemoveThresholdEventHandler to the eventLoop of tasks
    for (i=0; i<NB_CLIENT; i++)
    {
        le_event_QueueFunctionToThread( AppCtx[i].appThreadRef,
                                        RemoveHandler,
                                        &AppCtx[i],
                                        NULL );
    }

    // Wait for the tasks
    SynchTest();

    // trigger an event report
    ExpectedStatus = 0;
    pa_tempSimu_TriggerEventReport(ExpectedStatus);

    // Wait for the semaphore timeout to check that handlers are not called
    LE_ASSERT( le_sem_WaitWithTimeOut(ThreadSemaphore, TimeToWait) == LE_TIMEOUT );
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread running an event loop for le_temp and pa_temp_simu (useful for event report)
 *
 */
//--------------------------------------------------------------------------------------------------
static void* TempThread
(
    void* ctxPtr
)
{
    pa_temp_Init();

    le_temp_Init();

    le_sem_Post(ThreadSemaphore);

    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Create and start a thread to run le_temp and pa_temp_simu (useful for event report)
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_Temp_Init
(
    void
)
{
    // Create a semaphore to coordinate the test
    ThreadSemaphore = le_sem_Create("HandlerSem",0);

    le_thread_Start(le_thread_Create("PaTempThread", TempThread, NULL));

    le_sem_Wait(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test APIs with bad parameters
 *
 * Tested API:
 *  - le_temp_GetPlatformTemperature
 *  - le_temp_GetPlatformThresholds
 *  - le_temp_GetRadioTemperature
 *  - le_temp_GetRadioThresholds
 *  - le_temp_SetPlatformThresholds
 *  - le_temp_SetRadioThresholds
 *
 * exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_Temp_TestBadParameters
(
    void
)
{
    pa_tempSimu_SetReturnCode(LE_OK);
    LE_ASSERT(le_temp_GetPlatformTemperature(NULL) == LE_FAULT);
    LE_ASSERT(le_temp_GetPlatformThresholds(NULL,NULL,NULL,NULL) == LE_FAULT);
    LE_ASSERT(le_temp_GetRadioTemperature(NULL) == LE_FAULT);
    LE_ASSERT(le_temp_GetRadioThresholds(NULL,NULL) == LE_FAULT);
    LE_ASSERT(le_temp_SetPlatformThresholds(2,2,3,5) == LE_BAD_PARAMETER);
    LE_ASSERT(le_temp_SetPlatformThresholds(1,0,3,5) == LE_BAD_PARAMETER);
    LE_ASSERT(le_temp_SetPlatformThresholds(1,2,0,5) == LE_BAD_PARAMETER);
    LE_ASSERT(le_temp_SetPlatformThresholds(1,2,3,4) == LE_BAD_PARAMETER);
    LE_ASSERT(le_temp_SetRadioThresholds(3,4) == LE_BAD_PARAMETER);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test APIs with bad return code
 *
 * Tested API:
 *  - le_temp_GetPlatformTemperature
 *  - le_temp_GetPlatformThresholds
 *  - le_temp_GetRadioTemperature
 *  - le_temp_GetRadioThresholds
 *  - le_temp_SetPlatformThresholds
 *  - le_temp_SetRadioThresholds
 *
 * exit if failed
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_Temp_TestBadReturnCode
(
    void
)
{
    int32_t platformTemp=0;
    int32_t lowCriticalTemp=1, lowCriticalTempTmp=0;;
    int32_t lowWarningTemp=3, lowWarningTempTmp=0;
    int32_t hiWarningTemp=5, hiWarningTempTmp=0;
    int32_t hiCriticalTemp=7, hiCriticalTempTmp=0;
    int32_t radioTemp=0;

    pa_tempSimu_SetReturnCode(LE_FAULT);
    LE_ASSERT(le_temp_GetPlatformTemperature(&platformTemp) == LE_FAULT);
    LE_ASSERT(le_temp_GetPlatformThresholds(&lowCriticalTempTmp,
                                            &lowWarningTempTmp,
                                            &hiWarningTempTmp,
                                            &hiCriticalTempTmp) == LE_FAULT);
    LE_ASSERT(le_temp_GetRadioTemperature(&radioTemp) == LE_FAULT);
    LE_ASSERT(le_temp_GetRadioThresholds(&hiWarningTempTmp, &hiCriticalTempTmp) == LE_FAULT);
    LE_ASSERT(le_temp_SetPlatformThresholds(lowCriticalTemp,
                                            lowWarningTemp,
                                            hiWarningTemp,
                                            hiCriticalTemp) == LE_FAULT);
    LE_ASSERT(le_temp_SetRadioThresholds(hiWarningTemp, hiCriticalTemp) == LE_FAULT);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test APIs with correct parameters
 *
 * Tested API:
 *  - le_temp_GetPlatformTemperature
 *  - le_temp_GetPlatformThresholds
 *  - le_temp_GetRadioTemperature
 *  - le_temp_GetRadioThresholds
 *  - le_temp_SetPlatformThresholds
 *  - le_temp_SetRadioThresholds
 *
 * exit if failed
 */
//--------------------------------------------------------------------------------------------------
void Testle_Temp_TestCorrectUsage
(
    void
)
{
    int32_t platformTemp=0;
    int32_t lowCriticalTemp=1, lowCriticalTempTmp=0;;
    int32_t lowWarningTemp=3, lowWarningTempTmp=0;
    int32_t hiWarningTemp=5, hiWarningTempTmp=0;
    int32_t hiCriticalTemp=7, hiCriticalTempTmp=0;
    int32_t radioTemp=0;

    pa_tempSimu_SetReturnCode(LE_OK);

    LE_ASSERT(le_temp_GetPlatformTemperature(&platformTemp) == LE_OK);
    LE_ASSERT(platformTemp==PA_SIMU_TEMP_DEFAULT_PLATFORM_TEMP);

    LE_ASSERT(le_temp_GetPlatformThresholds(&lowCriticalTempTmp,
                                            &lowWarningTempTmp,
                                            &hiWarningTempTmp,
                                            &hiCriticalTempTmp) == LE_OK);
    LE_ASSERT(lowCriticalTempTmp == PA_SIMU_TEMP_DEFAULT_PLATFORM_LOW_CRIT);
    LE_ASSERT(lowWarningTempTmp == PA_SIMU_TEMP_DEFAULT_PLATFORM_LOW_WARN);
    LE_ASSERT(hiWarningTempTmp == PA_SIMU_TEMP_DEFAULT_PLATFORM_HIGH_WARN);
    LE_ASSERT(hiCriticalTempTmp == PA_SIMU_TEMP_DEFAULT_PLATFORM_HIGH_CRIT);

    LE_ASSERT(le_temp_GetRadioTemperature(&radioTemp) == LE_OK);
    LE_ASSERT(radioTemp == PA_SIMU_TEMP_DEFAULT_RADIO_TEMP);

    LE_ASSERT(le_temp_GetRadioThresholds(&hiWarningTempTmp, &hiCriticalTempTmp) == LE_OK);
    LE_ASSERT(hiWarningTempTmp == PA_SIMU_TEMP_DEFAULT_RADIO_HIGH_WARN);
    LE_ASSERT(hiCriticalTempTmp == PA_SIMU_TEMP_DEFAULT_RADIO_HIGH_CRIT);

    LE_ASSERT(le_temp_SetPlatformThresholds(lowCriticalTemp,
                                            lowWarningTemp,
                                            hiWarningTemp,
                                            hiCriticalTemp) == LE_OK);

    // Check if the values are correctly set in the PA
    lowCriticalTempTmp = lowWarningTempTmp = hiWarningTempTmp = hiCriticalTempTmp = 0;
    pa_temp_GetPlatformThresholds(&lowCriticalTempTmp,
                                  &lowWarningTempTmp,
                                  &hiWarningTempTmp,
                                  &hiCriticalTempTmp);
    LE_ASSERT(lowCriticalTemp == lowCriticalTempTmp);
    LE_ASSERT(lowWarningTemp == lowWarningTempTmp);
    LE_ASSERT(hiWarningTemp == hiWarningTempTmp);
    LE_ASSERT(hiCriticalTemp == hiCriticalTempTmp);

    LE_ASSERT(le_temp_SetRadioThresholds(hiWarningTemp, hiCriticalTemp) == LE_OK);
    hiWarningTempTmp = hiCriticalTempTmp = 0;
    pa_temp_GetRadioThresholds(&hiWarningTempTmp, &hiCriticalTempTmp);
    LE_ASSERT(hiWarningTemp == hiWarningTempTmp);
    LE_ASSERT(hiCriticalTemp == hiCriticalTempTmp);
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    Testle_Temp_Init();

    LE_INFO("======== Start UnitTest of TEMP API ========");

    LE_INFO("======== Test invalid parameters ========");
    Testle_Temp_TestBadParameters();

    LE_INFO("======== Test failed return code ========");
    Testle_Temp_TestBadReturnCode();

    LE_INFO("======== Test correct usage ========");
    Testle_Temp_TestCorrectUsage();

    LE_INFO("======== Test temperature event report ========");
    Testle_Temp_AddHandlers();
    Testle_Temp_RemoveHandlers();

    LE_INFO("======== UnitTest of TEMP API ends with SUCCESS ========");

    return 0;
}
