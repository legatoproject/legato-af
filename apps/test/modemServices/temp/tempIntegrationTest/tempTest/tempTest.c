/**
 * This module implements the le_temp's unit tests.
 *
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

/*
 * Instruction to execute this test
 * 1) install application test.
 * 2) Start log trace 'logread -f | grep 'temp'
 * 3) Start application 'app start tempTest'
 * 4) Start sequence execInApp tempTest tempTest <sequence number>
 *                     "Sequence <id>",
      "Sequence <id>"
                "    : Display Help"
                "  0 : Get temperature"
                "  1 : Set Get Platform Thresholds"
                "  2 : Set Get Radio Thresholds"
                "  3 : Configure Platform Thresolds event" or set them manualy by AT commands
                "  4 : Configure Radio Thresolds event" or set them manualy by AT commands
         Restart target
         Start log trace 'logread -f | grep 'temp'
         Start application 'app start tempTest'
         Start sequence execInApp tempTest tempTest <sequence number>
                "  5 : Test Thresolds event, (use CTR+C to exit before first Critical Event)
         Change temeparture to check differrent events.
                "  6 : Restore Radio temperature Thresolds"
                "  7 : Restore Platform temperature Thresolds"
 * 5) check temperature INFO traces value.
 */

#include "legato.h"
#include "interfaces.h"

#include <time.h>
#include <semaphore.h>

// Default Radio temperature thresholds.
#define DEFAULT_RADIO_HIWARNING_THRESHOLD      110
#define DEFAULT_RADIO_HICRITICAL_THRESHOLD     140

// Default platform temperature thresholds.
#define DEFAULT_PLATFORM_HICRITICAL_THRESHOLD  140
#define DEFAULT_PLATFORM_HIWARNING_THRESHOLD   90
#define DEFAULT_PLATFORM_LOWARNING_THRESHOLD   -40
#define DEFAULT_PLATFORM_LOCRITICAL_THRESHOLD  -45

/* Waiting time for setting temperature Thresholds */
#define WAIT_TIME 30

/* Waiting time for Threshold Events */
#define WAIT_TIME_EVENT 480

/* Semaphore use for Critical Event waiting */
static sem_t SemaphoreCriticalEvent;

/* Value for managed Current value displaying */
static int PoolTemp = 0;

/* Value for managed Current value displaying */
static int TimeCounter = 0;


//--------------------------------------------------------------------------------------------------
/**
 * Helper.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage()
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
                    "Sequence <id>",
                    "    : Display Help",
                    "  0 : Get temperature",
                    "  1 : Set Get Platform Thresholds",
                    "  2 : Set Get Radio Thresholds",
                    "  3 : Configure Platform Thresolds event",
                    "  4 : Configure Radio Thresolds event",
                    "  5 : Test Thresolds event, (use CTR+C to exit before first Critical Event)",
                    "  6 : Restore Radio temperature Thresolds",
                    "  7 : Restore Platform temperature Thresolds"
    };

    for(idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        if(sandboxed)
        {
            LE_INFO("%s", usagePtr[idx]);
        }
        else
        {
            fprintf(stderr, "%s\r\n", usagePtr[idx]);
        }
    }
}



/*
 * Restore default Radio temperature thresholds.
 */
static void SetDefaultRadioThreshold
(
    void
)
{

    if ( le_temp_SetRadioThresholds(DEFAULT_RADIO_HIWARNING_THRESHOLD,
                    DEFAULT_RADIO_HICRITICAL_THRESHOLD ) == LE_OK)
    {
        LE_INFO("======== Restore Radio Threshold warning Done ========");
    }
    else
    {
        LE_INFO("======== Restore Radio Threshold warning Failed ========");
    }
}

/*
 * Restore default Platform temperature thresholds.
 */
static void SetDefaultPlatformThreshold
(
    void
)
{
    if ( le_temp_SetPlatformThresholds(DEFAULT_PLATFORM_LOCRITICAL_THRESHOLD,
                    DEFAULT_PLATFORM_LOWARNING_THRESHOLD,
                    DEFAULT_PLATFORM_HIWARNING_THRESHOLD,
                    DEFAULT_PLATFORM_HICRITICAL_THRESHOLD) == LE_OK)
    {
        LE_INFO("======== Restore Platform Threshold Done ========");
    }
    else
    {
        LE_INFO("======== Restore Platform Threshold Failed ========");
    }
}
static void* DisplayTempThread(void* context)
{
    int32_t radioTemp = 0;
    int32_t platformTemp = 0;

    le_temp_ConnectService();

    LE_INFO("Thread Start");

    do
    {
        if (PoolTemp == 2)
        {
            le_temp_GetRadioTemperature(&radioTemp);
            le_temp_GetPlatformTemperature(&platformTemp);
            LE_INFO("(%d) Get Radio Temp (%d), Platform Temp %d",
                TimeCounter++, radioTemp, platformTemp);
            TimeCounter++;
        }
        sleep(1);
    }
    while (PoolTemp > 0);

    // Run the event loop
    le_event_RunLoop();
    return NULL;
}


static void ThresholdEventHandlerFunc
(
    le_temp_ThresholdStatus_t event,
    void* contextPtr
)
{
    switch(event)
    {
        case LE_TEMP_PLATFORM_HI_CRITICAL:
            LE_INFO("ThresholdEventHandlerFunc event LE_TEMP_PLATFORM_HI_CRITICAL");
            sem_post(&SemaphoreCriticalEvent);
            break;

        case LE_TEMP_PLATFORM_HI_WARNING:
            LE_INFO("ThresholdEventHandlerFunc event LE_TEMP_PLATFORM_HI_WARNING");
            break;

        case LE_TEMP_PLATFORM_LOW_CRITICAL:
            LE_INFO("ThresholdEventHandlerFunc event LE_TEMP_PLATFORM_LOW_CRITICAL");
            sem_post(&SemaphoreCriticalEvent);
            break;

        case LE_TEMP_PLATFORM_LOW_WARNING:
            LE_INFO("ThresholdEventHandlerFunc event LE_TEMP_PLATFORM_LOW_WARNING");

            break;

        case LE_TEMP_PLATFORM_NORMAL:
            LE_INFO("ThresholdEventHandlerFunc event LE_TEMP_PLATFORM_NORMAL");
            break;

        case LE_TEMP_RADIO_HI_CRITICAL:
            LE_INFO("ThresholdEventHandlerFunc event LE_TEMP_RADIO_HI_CRITICAL");
            sem_post(&SemaphoreCriticalEvent);
            break;

        case LE_TEMP_RADIO_HI_WARNING:
            LE_INFO("ThresholdEventHandlerFunc event LE_TEMP_RADIO_HI_WARNING");
            break;

        case LE_TEMP_RADIO_NORMAL:
            LE_INFO("ThresholdEventHandlerFunc event LE_TEMP_RADIO_NORMAL");
            break;

        default:
            LE_ERROR("ThresholdEventHandlerFunc event Unknown %d", event);
            break;
    }
}

/*
 * Event Thread
 */
static void* EventThread(void* context)
{
    le_temp_ThresholdEventHandlerRef_t ref;

    le_temp_ConnectService();

    ref = le_temp_AddThresholdEventHandler(ThresholdEventHandlerFunc, NULL);
    LE_ASSERT(ref != NULL);
    LE_INFO("ref  0x%p", ref);

    LE_INFO("EventThread Start");

    // Run the event loop
    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Test:
 * - le_temp_GetRadioTemperature()
 * - le_temp_GetPlatformTemperature()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_temp_GetTemperatures
(
)
{
    int32_t temperature = 0;
    le_result_t res = LE_FAULT;

    res = le_temp_GetRadioTemperature(&temperature);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_temp_GetRadioTemperature return %d degree Celcus", temperature);

    res = le_temp_GetPlatformTemperature(&temperature);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_temp_GetPlatformTemperature return %d degree Celcus", temperature);

}


//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *  - le_temp_GetPlatformThresholds()
 *  - le_temp_SetPlatformThresholds()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_temp_SetGetPlatformThresholds
(
)
{
    le_result_t res = LE_FAULT;

    int32_t oldLoCriticalTemp = 0;
    int32_t oldLoWarningTemp = 0;
    int32_t oldHiWarningTemp = 0;
    int32_t oldHiCriticalTemp = 0;

    int32_t loCriticalTemp = 0;
    int32_t loWarningTemp = 0;
    int32_t hiWarningTemp = 0;
    int32_t hiCriticalTemp = 0;

    int32_t refLoCriticalTemp = 0;
    int32_t refLoWarningTemp = 0;
    int32_t refHiWarningTemp = 0;
    int32_t refHiCriticalTemp = 0;

    res = le_temp_GetPlatformThresholds(&oldLoCriticalTemp, &oldLoWarningTemp,
                    &oldHiWarningTemp, &oldHiCriticalTemp);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_temp_GetPlatformThresholds(%d, %d, %d, %d)", oldLoCriticalTemp, oldLoWarningTemp,
        oldHiWarningTemp, oldHiCriticalTemp);

    refLoCriticalTemp = oldLoCriticalTemp + 10;
    refLoWarningTemp = oldLoWarningTemp + 20;
    refHiWarningTemp = oldHiWarningTemp - 20;
    refHiCriticalTemp = oldHiCriticalTemp -10;

    LE_INFO("le_temp_SetPlatformThresholds(%d, %d, %d, %d)", refLoCriticalTemp, refLoWarningTemp,
           refHiWarningTemp, refHiCriticalTemp);
    res = le_temp_SetPlatformThresholds(refLoCriticalTemp, refLoWarningTemp,
                    refHiWarningTemp, refHiCriticalTemp);
    LE_ASSERT(res == LE_OK);


    res = le_temp_GetPlatformThresholds(&loCriticalTemp, &loWarningTemp,
                    &hiWarningTemp, &hiCriticalTemp);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_temp_GetPlatformThresholds(%d, %d, %d, %d)", loCriticalTemp, loWarningTemp,
        hiWarningTemp, hiCriticalTemp);
    LE_ASSERT(loCriticalTemp == refLoCriticalTemp);
    LE_ASSERT(loWarningTemp == refLoWarningTemp);
    LE_ASSERT(hiWarningTemp == refHiWarningTemp);
    LE_ASSERT(hiCriticalTemp == refHiCriticalTemp);

    // Test with critical threshold equal to the warning temperature.
    LE_INFO("le_temp_SetPlatformThresholds(%d, %d, %d, %d)", refLoCriticalTemp, refLoCriticalTemp,
        refHiWarningTemp, refHiCriticalTemp);
    res = le_temp_SetPlatformThresholds(refLoCriticalTemp, refLoCriticalTemp,
                    refHiWarningTemp, refHiCriticalTemp);
    LE_ASSERT(res == LE_BAD_PARAMETER);

    refLoCriticalTemp = oldLoCriticalTemp + 20  ;
    refLoWarningTemp = oldLoCriticalTemp + 10;
    refHiWarningTemp = oldHiWarningTemp;
    refHiCriticalTemp = oldHiCriticalTemp;

    LE_INFO("le_temp_SetPlatformThresholds(%d, %d, %d, %d)", refLoCriticalTemp, refLoCriticalTemp,
        refHiWarningTemp, refHiCriticalTemp);
    // Test with critical threshold temperature lesser than the warning temperature.
    res = le_temp_SetPlatformThresholds(refLoCriticalTemp, refLoCriticalTemp,
                    refHiWarningTemp, refHiCriticalTemp);
    LE_ASSERT(res == LE_BAD_PARAMETER);

    LE_INFO("Restore Initial thresold values (%d, %d, %d, %d)",
        oldLoCriticalTemp, oldLoWarningTemp,
        oldHiWarningTemp, oldHiCriticalTemp);
    res = le_temp_SetPlatformThresholds(oldLoCriticalTemp, oldLoWarningTemp,
                    oldHiWarningTemp, oldHiCriticalTemp);
    LE_ASSERT(res == LE_OK);

    res = le_temp_GetPlatformThresholds(&loCriticalTemp, &loWarningTemp,
                    &hiWarningTemp, &hiCriticalTemp);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_temp_GetPlatformThresholds(%d, %d, %d, %d)", loCriticalTemp, loWarningTemp,
        hiWarningTemp, hiCriticalTemp);

    LE_ASSERT(loCriticalTemp == oldLoCriticalTemp);
    LE_ASSERT(loWarningTemp == oldLoWarningTemp);
    LE_ASSERT(hiWarningTemp == oldHiWarningTemp);
    LE_ASSERT(hiCriticalTemp == oldHiCriticalTemp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *  - le_temp_SetRadioThresholds()
 *  - le_temp_GetRadioThresholds()
 */
//--------------------------------------------------------------------------------------------------
static void Testle_temp_SetGetRadioThresholds
(
)
{
    int32_t oldWarningTemp = 0;
    int32_t oldCriticalTemp = 0;
    int32_t warningTemp = 0;
    int32_t criticalTemp = 0;
    int32_t refwarningTemp = 0;
    int32_t refcriticalTemp = 0;
    le_result_t res = LE_FAULT;

    res = le_temp_GetRadioThresholds(&oldWarningTemp, &oldCriticalTemp);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_temp_GetRadioThresholds(%d, %d)", oldWarningTemp, oldCriticalTemp);

    refwarningTemp = oldWarningTemp - 30;
    refcriticalTemp = oldCriticalTemp - 20;

    res = le_temp_SetRadioThresholds(refwarningTemp, refcriticalTemp);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_temp_SetThreshold(%d, %d)", refwarningTemp, refcriticalTemp);

    res = le_temp_GetRadioThresholds(&warningTemp, &criticalTemp);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_temp_GetThreshold(%d, %d)", warningTemp, criticalTemp);
    LE_ASSERT(warningTemp == refwarningTemp);
    LE_ASSERT(criticalTemp == refcriticalTemp);

    // Test with critical threshold equal to the warning temperature.
    res = le_temp_SetRadioThresholds(warningTemp, warningTemp);
    LE_INFO("le_temp_SetThreshold(%d, %d)", warningTemp, warningTemp);
    LE_ASSERT(res == LE_BAD_PARAMETER);

    refwarningTemp = oldWarningTemp;
    refcriticalTemp = oldWarningTemp - 10;

    // Test with critical threshold temperature lesser than the warning temperature.
    res = le_temp_SetRadioThresholds(refwarningTemp, refcriticalTemp);
    LE_INFO("le_temp_SetThreshold(%d, %d)", refwarningTemp, refcriticalTemp);
    LE_ASSERT(res == LE_BAD_PARAMETER);

    LE_INFO("Restore Initial thresold values warning=%d, critical=%d",
        oldWarningTemp, oldCriticalTemp);

    res = le_temp_SetRadioThresholds(oldWarningTemp, oldCriticalTemp);
    LE_INFO("le_temp_SetThreshold(%d, %d)", oldWarningTemp, oldCriticalTemp);
    LE_ASSERT(res == LE_OK);
    res = le_temp_GetRadioThresholds(&warningTemp, &criticalTemp);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_temp_GetThreshold(%d, %d)", warningTemp, criticalTemp);
    LE_ASSERT(warningTemp == oldWarningTemp);
    LE_ASSERT(criticalTemp == oldCriticalTemp);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *  - le_temp_GetPlatformTemperature()
 *  - le_temp_SetPlatformThresholds()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_temp_PlatformThresholdEventSetting
(
)
{
    int32_t warningTemperature = 0;
    int32_t criticalTemperature = 0;
    int32_t lowarningTemperature = 0;
    int32_t locriticalTemperature = 0;

    int32_t temperature = 0;
    le_result_t res = LE_FAULT;

    TimeCounter = 0;
    PoolTemp = 1;
    LE_INFO("Set PoolTemp %d", PoolTemp);

    le_thread_Ref_t thread = le_thread_Create("tempTest",DisplayTempThread,NULL);
    le_thread_Start(thread);

    LE_INFO("!!!!!!! YOU HAVE %d SECOND TO SET THE MODULE AT"
        " THE TEMP REFERENCE !!!!!!!", WAIT_TIME);
    TimeCounter = 0;

    PoolTemp = 2;
    LE_INFO("Set PoolTemp %d", PoolTemp);

    sleep(WAIT_TIME);

    // Get current Platform Temperature
    res = le_temp_GetPlatformTemperature(&temperature);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_temp_GetPlatformTemperature return %d degree Celcus", temperature);

    res = le_temp_GetPlatformThresholds(&locriticalTemperature, &lowarningTemperature,
                    &warningTemperature, &criticalTemperature);
    LE_ASSERT(res == LE_OK);

    // Set Warning Platform threshold Temperature
    criticalTemperature = temperature + 20;
    warningTemperature = temperature + 10;
    lowarningTemperature = temperature - 10;
    locriticalTemperature = temperature - 20;

    //criticalTemperature = DEFAULT_PLATFORM_HICRITICAL_THRESHOLD; //temperature + 20;
    res = le_temp_SetPlatformThresholds(locriticalTemperature, lowarningTemperature,
                    warningTemperature, criticalTemperature);
    LE_ASSERT(res == LE_OK);

    PoolTemp = 0;
    LE_INFO("Set PoolTemp %d", PoolTemp);
    LE_INFO("!!!!!!! YOU MUST REBOOT THE MODULE !!!!!!!");
    sleep(2);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *  - le_temp_GetradioTemperature()
 *  - le_temp_SetRadioThresholds()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_temp_RadioThresholdEventSetting
(
)
{
    int32_t warningTemperature = 0;
    int32_t criticalTemperature = 0;
    int32_t temperature = 0;

    le_result_t res = LE_FAULT;

    TimeCounter = 0;
    PoolTemp = 1;
    LE_INFO("Set PoolTemp %d", PoolTemp);

    le_thread_Ref_t thread = le_thread_Create("tempTest",DisplayTempThread,NULL);
    le_thread_Start(thread);

    LE_INFO("!!!!!!! YOU HAVE %d SECOND TO SET THE MODULE AT"
           " THE TEMP REFERENCE !!!!!!!", WAIT_TIME);
    TimeCounter = 0;

    PoolTemp = 2;
    LE_INFO("Set PoolTemp %d", PoolTemp);

    sleep(WAIT_TIME);

    // Get current PA Tempeartaure
    res = le_temp_GetRadioTemperature(&temperature);
    LE_ASSERT(res == LE_OK);
    LE_INFO("le_temp_GetRadioTemperature return %d degree Celcus", temperature);

    // Set Warning threshold Tempeartaure
    warningTemperature = temperature + 10;
    criticalTemperature = temperature + 20;

    LE_INFO("temperature threshold are set tole_temp_SetThreshold(%d, %d) in degree Celcus",
        warningTemperature, criticalTemperature);
    res = le_temp_SetRadioThresholds(warningTemperature, criticalTemperature);
    LE_ASSERT(res == LE_OK);

    PoolTemp = 0;
    LE_INFO("Set PoolTemp %d", PoolTemp);
    LE_INFO("!!!!!!! YOU MUST REBOOT THE MODULE !!!!!!!");
    sleep(2);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *  - le_temp_AddThresholdEventHandler()
 *  - le_temp_SetThreshold()
 *  - le_temp_GetThreshold()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_temp_ThresholdEvent
(
)
{
    //  le_temp_ThresholdEventHandlerRef_t ref;
    struct timespec ts;
    le_temp_ThresholdEventHandlerRef_t ref;

    TimeCounter = 0;
    PoolTemp = 1;
    LE_INFO("Set PoolTemp %d", PoolTemp);

    ref = le_temp_AddThresholdEventHandler(ThresholdEventHandlerFunc, NULL);
    LE_ASSERT(ref != NULL);
    LE_INFO("ref  0x%p", ref);
    le_temp_RemoveThresholdEventHandler(ref);

    le_thread_Ref_t thread = le_thread_Create("tempTest",DisplayTempThread,NULL);
    le_thread_Start(thread);

    le_thread_Ref_t thread2 = le_thread_Create("EventThread",EventThread,NULL);
    le_thread_Start(thread2);

    sem_init(&SemaphoreCriticalEvent,0,0);

    PoolTemp = 2;
    LE_INFO("Set PoolTemp %d", PoolTemp);

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        LE_ERROR("Cannot get current time");
        return;
    }

    LE_INFO("!!!!!!! YOU MUST WARM UP OR COLD DOWN THE"
        " MODULE in %d second !!!!!!!", WAIT_TIME_EVENT);

    ts.tv_sec += WAIT_TIME_EVENT; // Wait for 480 seconds.
    ts.tv_nsec += 0;

    if ( sem_timedwait(&SemaphoreCriticalEvent,&ts) == -1 )
    {
        LE_WARN("errno %d", errno);
        if ( errno == ETIMEDOUT)
        {
            LE_WARN("Timeout for Warnig Event");
            return;
        }
    }

    PoolTemp = 0;
    LE_INFO("Set PoolTemp %d", PoolTemp);
    sleep(2);
}


COMPONENT_INIT
{
    const char *  testNumberStr = NULL;
    int testNumberInt = 0;
    int nbarg = 0;

    nbarg = le_arg_NumArgs();
    LE_INFO(" nbargument %d ", nbarg );
    if ( nbarg  == 1 )
    {
        testNumberStr = le_arg_GetArg(0);
        testNumberInt = atoi(testNumberStr);
        LE_DEBUG("Test Sequence %s => %d", testNumberStr, testNumberInt);

        LE_INFO("======== Start temperature (%d) test sequence ========", testNumberInt);

        switch (testNumberInt)
        {
            case 0:
            {
                LE_INFO("======== Testle_temp_GetTemperatures Test ========");
                Testle_temp_GetTemperatures();
                LE_INFO("======== Testle_temp_GetTemperatures Test PASSED ========");
            }
            break;

            case 1:
            {
                LE_INFO("======== Testle_temp_SetGetPlatformThresholds Test ========");
                Testle_temp_SetGetPlatformThresholds();
                LE_INFO("======== Testle_temp_SetGetPlatformThresholds Test PASSED ========");
            }
            break;

            case 2:
            {
                LE_INFO("======== Testle_temp_SetGetRadioThresholds Test ========");
                Testle_temp_SetGetRadioThresholds();
                LE_INFO("======== Testle_temp_SetGetRadioThresholds Test PASSED ========");
            }
            break;

            case 3:
            {
                LE_INFO("======== Testle_temp_PlatformThresholdEventSetting Test ========");
                Testle_temp_PlatformThresholdEventSetting();
                LE_INFO("======== Testle_temp_PlatformThresholdEventSetting Test PASSED ========");
            }
            break;

            case 4:
            {
                LE_INFO("======== Testle_temp_RadioThresholdEventSetting Test ========");
                Testle_temp_RadioThresholdEventSetting();
                LE_INFO("======== Testle_temp_RadioThresholdEventSetting Test PASSED ========");
            }
            break;

            case 5:
            {
                LE_INFO("======== Testle_temp_ThresholdEvent Test ========");
                Testle_temp_ThresholdEvent();
                LE_INFO("======== Testle_temp_ThresholdEvent Test PASSED ========");
            }
            break;

            case 6:
            {
                SetDefaultRadioThreshold();
            }
            break;

            case 7:
            {
                SetDefaultPlatformThreshold();
            }
            break;

            default:
            {
                PrintUsage();
            }
            break;
        }
        LE_INFO("======== Test temperature sequence (%d) Done ========", testNumberInt);
    }
    else
    {
        PrintUsage();
    }

    exit(EXIT_SUCCESS);
}

