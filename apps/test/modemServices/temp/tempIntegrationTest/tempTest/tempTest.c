/**
 * This module implements the le_temp's integration test (work on AR8652).
 *
 * Instruction to execute this test
 * 1) install application test.
 * 2) Start log trace 'logread -f | grep 'temp'
 * 3) Start application 'app start tempTest'
 * 4) Start sequence app runProc tempTest --exe=tempTest -- <sequence number>
 *
 *       "Sequence <id>"
 *                 "    : Display Help"
 *                 "  0 : Get temperature"
 *                 "  1 : Set Get Power Controller Thresholds"
 *                 "  2 : Set Get Power Amplifier Thresholds"
 *                 "  3 : Configure Power Controller Thresolds event"
 *                 "  4 : Configure Power Amplifier Thresolds event"
 *          Restart target
 *          Start log trace 'logread -f | grep 'temp'
 *          Start application 'app start tempTest'
 *          Start sequence app runProc tempTest --exe=tempTest -- <sequence number>
 *                 "  5 : Test Thresolds event, (use CTR+C to exit before first Critical Event)
 *          Change temeparture to check differrent events.
 *                 "  6 : Set default Power Amplifier temperature Thresolds"
 *                 "  7 : Set default Power Controller temperature Thresolds"
 * 5) check temperature INFO traces value.
 *
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

#include <time.h>
#include <semaphore.h>

// Default Power Amplifier temperature thresholds.
#define MY_PA_HI_NORMAL_THRESHOLD      110
#define MY_PA_HI_CRITICAL_THRESHOLD    140

// Default Power Controller temperature thresholds.
#define MY_PC_HI_CRITICAL_THRESHOLD  140
#define MY_PC_HI_NORMAL_THRESHOLD    90
#define MY_PC_LO_NORMAL_THRESHOLD    -40
#define MY_PC_LO_CRITICAL_THRESHOLD  -45

// Waiting time to reach temperature Thresholds
#define WAIT_TIME 30

// Waiting time for Threshold Events
#define WAIT_TIME_EVENT 480

// Semaphore used for Critical Event waiting
static sem_t SemaphoreCriticalEvent;

// Variables for value displaying
static int PoolingPause = 0;
static int TimeCounter = 0;


//--------------------------------------------------------------------------------------------------
/**
 * Helper.
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage
(
    void
)
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
                    "Sequence <id>",
                    "    : Display Help",
                    "  0 : Get temperature",
                    "  1 : Set Get Power Controller Thresholds",
                    "  2 : Set Get Power Amplifier Thresholds",
                    "  3 : Configure Power Controller Thresolds event",
                    "  4 : Configure Power Amplifier Thresolds event",
                    "  5 : Test Thresolds event, (use CTR+C to exit before first Critical Event)",
                    "  6 : Set default Power Amplifier temperature Thresolds",
                    "  7 : Set default Power Controller temperature Thresolds"
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

//-------------------------------------------------------------------------------------------------
/**
 * Set default default Power Amplifier temperature thresholds.
 */
//-------------------------------------------------------------------------------------------------
static void SetDefaultPaThreshold
(
    void
)
{
    le_temp_SensorRef_t paSensorRef = le_temp_Request("POWER_AMPLIFIER");
    if ((le_temp_SetThreshold(paSensorRef, "HI_NORMAL_THRESHOLD", MY_PA_HI_NORMAL_THRESHOLD) != LE_OK) ||
        (le_temp_SetThreshold(paSensorRef, "HI_CRITICAL_THRESHOLD",MY_PA_HI_CRITICAL_THRESHOLD) != LE_OK))
    {
        LE_INFO("======== Set default Power Amplifier Threshold Failed ========");
    }
    else
    {
         LE_ASSERT(le_temp_StartMonitoring() == LE_OK);
         LE_INFO("======== Set default Power Amplifier Threshold Done ========");
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Set default default Power Controller temperature thresholds.
 */
//-------------------------------------------------------------------------------------------------
static void SetDefaultPcThreshold
(
    void
)
{
    le_temp_SensorRef_t pcSensorRef = le_temp_Request("POWER_CONTROLLER");
    if ((le_temp_SetThreshold(pcSensorRef, "LO_CRITICAL_THRESHOLD", MY_PC_LO_CRITICAL_THRESHOLD) != LE_OK) ||
        (le_temp_SetThreshold(pcSensorRef, "LO_NORMAL_THRESHOLD", MY_PC_LO_NORMAL_THRESHOLD) != LE_OK) ||
        (le_temp_SetThreshold(pcSensorRef, "HI_NORMAL_THRESHOLD", MY_PC_HI_NORMAL_THRESHOLD) != LE_OK) ||
        (le_temp_SetThreshold(pcSensorRef, "HI_CRITICAL_THRESHOLD", MY_PC_HI_CRITICAL_THRESHOLD) != LE_OK))
    {
        LE_INFO("======== Set default Power Controller Threshold Failed ========");
    }
    else
    {
         LE_ASSERT(le_temp_StartMonitoring() == LE_OK);
         LE_INFO("======== Set default Power Controller Threshold Done ========");
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Thread for temperature displaying.
 */
//-------------------------------------------------------------------------------------------------
static void* DisplayTempThread
(
    void* context
)
{
    int32_t paTemp = 0;
    int32_t pcTemp = 0;

    le_temp_ConnectService();

    LE_INFO("DisplayTempThread Start");

    do
    {
        if (PoolingPause == 2)
        {
            le_temp_SensorRef_t paSensorRef = le_temp_Request("POWER_AMPLIFIER");
            le_temp_GetTemperature(paSensorRef, &paTemp);
            le_temp_SensorRef_t pcSensorRef = le_temp_Request("POWER_CONTROLLER");
            le_temp_GetTemperature(pcSensorRef, &pcTemp);
            LE_INFO("(count.%d) Get Power Amplifier Temp pa.%d, Power Controller Temp pc.%d",
                    TimeCounter++, paTemp, pcTemp);
            TimeCounter++;
        }
        sleep(1);
    }
    while (PoolingPause > 0);

    le_event_RunLoop();
    return NULL;
}

//-------------------------------------------------------------------------------------------------
/**
 * Threshold handler.
 */
//-------------------------------------------------------------------------------------------------
static void ThresholdEventHandlerFunc
(
    le_temp_SensorRef_t  sensorRef,
    const char*          thresholdPtr,
    void*                contextPtr
)
{
    char sensorName[LE_TEMP_SENSOR_NAME_MAX_BYTES] = {0};

    LE_ASSERT(le_temp_GetSensorName(sensorRef, sensorName, sizeof(sensorName)) == LE_OK);
    LE_ASSERT(!strlen(sensorName));

    LE_INFO("%s threshold event for %s sensor", thresholdPtr, sensorName);
}

//-------------------------------------------------------------------------------------------------
/**
 * Event Thread
 */
//-------------------------------------------------------------------------------------------------
static void* EventThread(void* context)
{
    le_temp_ThresholdEventHandlerRef_t ref;

    le_temp_ConnectService();

    ref = le_temp_AddThresholdEventHandler(ThresholdEventHandlerFunc, NULL);
    LE_ASSERT(ref != NULL);
    LE_INFO("EventThread to add Threshold event handler with ref.%p", ref);

    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Test:
 * - le_temp_GetTemperature()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_temp_GetTemperatures
(
)
{
    int32_t temp = 0;

    le_temp_SensorRef_t paSensorRef = le_temp_Request("POWER_AMPLIFIER");
    LE_ASSERT(le_temp_GetTemperature(paSensorRef, &temp) == LE_OK);
    LE_INFO("le_temp_GetTemperature return %d degrees Celsius for Power Amplifier sensor", temp);

    le_temp_SensorRef_t pcSensorRef = le_temp_Request("POWER_CONTROLLER");
    LE_ASSERT(le_temp_GetTemperature(pcSensorRef, &temp) == LE_OK);
    LE_INFO("le_temp_GetTemperature return %d degrees Celsius Power Controller sensor", temp);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *  - le_temp_GetThresholds()
 *  - le_temp_SetThresholds()
 */
//--------------------------------------------------------------------------------------------------
static void Testle_temp_SetGetPcThresholds
(
    void
)
{
    int32_t oldLoCriticalTemp = 0;
    int32_t oldLoNormalTemp = 0;
    int32_t oldHiNormalTemp = 0;
    int32_t oldHiCriticalTemp = 0;

    int32_t loCriticalTemp = 0;
    int32_t loNormalTemp = 0;
    int32_t hiNormalTemp = 0;
    int32_t hiCriticalTemp = 0;

    int32_t refLoCriticalTemp = 0;
    int32_t refLoNormalTemp = 0;
    int32_t refHiNormalTemp = 0;
    int32_t refHiCriticalTemp = 0;

    le_temp_SensorRef_t pcSensorRef = le_temp_Request("POWER_CONTROLLER");
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "LO_CRITICAL_THRESHOLDL", &oldLoCriticalTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "LO_NORMAL_THRESHOLD", &oldLoNormalTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "HI_NORMAL_THRESHOLD", &oldHiNormalTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "HI_CRITICAL_THRESHOLD", &oldHiCriticalTemp) == LE_OK);
    LE_INFO("le_temp_GetThreshold for PC (lo_crit.%d, lo_norm.%d, hi_norm.%d, hi_crit.%d)",
            oldLoCriticalTemp,
            oldLoNormalTemp,
            oldHiNormalTemp,
            oldHiCriticalTemp);

    refLoCriticalTemp = oldLoCriticalTemp + 10;
    refLoNormalTemp = oldLoNormalTemp + 20;
    refHiNormalTemp = oldHiNormalTemp - 20;
    refHiCriticalTemp = oldHiCriticalTemp -10;

    LE_ASSERT(le_temp_SetThreshold(pcSensorRef, "LO_CRITICAL_THRESHOLD", refLoCriticalTemp) == LE_OK);
    LE_ASSERT(le_temp_SetThreshold(pcSensorRef, "LO_NORMAL_THRESHOLD", refLoNormalTemp) == LE_OK);
    LE_ASSERT(le_temp_SetThreshold(pcSensorRef, "HI_NORMAL_THRESHOLD", refHiNormalTemp) == LE_OK);
    LE_ASSERT(le_temp_SetThreshold(pcSensorRef, "HI_CRITICAL_THRESHOLD", refHiCriticalTemp) == LE_OK);
    LE_INFO("le_temp_SetThreshold for PC (lo_crit.%d, lo_norm.%d, hi_norm.%d, hi_crit.%d)",
            refLoCriticalTemp,
            refLoNormalTemp,
            refHiNormalTemp,
            refHiCriticalTemp);

    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "LO_CRITICAL_THRESHOLD", &loCriticalTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "LO_NORMAL_THRESHOLD", &loNormalTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "HI_NORMAL_THRESHOLD", &hiNormalTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "HI_CRITICAL_THRESHOLD", &hiCriticalTemp) == LE_OK);
    LE_INFO("le_temp_GetThreshold for PC (lo_crit.%d, lo_norm.%d, hi_norm.%d, hi_crit.%d)",
            loCriticalTemp,
            loNormalTemp,
            hiNormalTemp,
            hiCriticalTemp);

    LE_ASSERT(loCriticalTemp == refLoCriticalTemp);
    LE_ASSERT(loNormalTemp == refLoNormalTemp);
    LE_ASSERT(hiNormalTemp == refHiNormalTemp);
    LE_ASSERT(hiCriticalTemp == refHiCriticalTemp);

    LE_ASSERT(le_temp_SetThreshold(pcSensorRef, "LO_CRITICAL_THRESHOLD", oldLoCriticalTemp) == LE_OK);
    LE_ASSERT(le_temp_SetThreshold(pcSensorRef, "LO_NORMAL_THRESHOLD", oldLoNormalTemp) == LE_OK);
    LE_ASSERT(le_temp_SetThreshold(pcSensorRef, "HI_NORMAL_THRESHOLD", oldHiNormalTemp) == LE_OK);
    LE_ASSERT(le_temp_SetThreshold(pcSensorRef, "HI_CRITICAL_THRESHOLD", oldHiCriticalTemp) == LE_OK);
    LE_INFO("Restore Initial thresold values for PC (lo_crit.%d, lo_norm.%d, hi_norm.%d, hi_crit.%d)",
            oldLoCriticalTemp,
            oldLoNormalTemp,
            oldHiNormalTemp,
            oldHiCriticalTemp);

    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "LO_CRITICAL_THRESHOLD", &loCriticalTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "LO_NORMAL_THRESHOLD", &loNormalTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "HI_NORMAL_THRESHOLD", &hiNormalTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "HI_CRITICAL_THRESHOLD", &hiCriticalTemp) == LE_OK);
    LE_INFO("le_temp_GetThreshold for PC (lo_crit.%d, lo_norm.%d, hi_norm.%d, hi_crit.%d)",
            loCriticalTemp,
            loNormalTemp,
            hiNormalTemp,
            hiCriticalTemp);

    LE_ASSERT(loCriticalTemp == oldLoCriticalTemp);
    LE_ASSERT(loNormalTemp == oldLoNormalTemp);
    LE_ASSERT(hiNormalTemp == oldHiNormalTemp);
    LE_ASSERT(hiCriticalTemp == oldHiCriticalTemp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *  - le_temp_GetThresholds()
 *  - le_temp_SetThresholds()
 */
//--------------------------------------------------------------------------------------------------
static void Testle_temp_SetGetPaThresholds
(
)
{
    int32_t oldNormalTemp = 0;
    int32_t oldCriticalTemp = 0;
    int32_t normalTemp = 0;
    int32_t criticalTemp = 0;
    int32_t refNormalTemp = 0;
    int32_t refCriticalTemp = 0;

    le_temp_SensorRef_t paSensorRef = le_temp_Request("POWER_AMPLIFIER");

    LE_ASSERT(le_temp_GetThreshold(paSensorRef, "HI_NORMAL_THRESHOLD", &oldNormalTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(paSensorRef, "HI_CRITICAL_THRESHOLD", &oldCriticalTemp) == LE_OK);
    LE_INFO("le_temp_GetThreshold for PA (hi_norm.%d, hi_crit.%d)",
            oldNormalTemp,
            oldCriticalTemp);

    refNormalTemp = oldNormalTemp - 30;
    refCriticalTemp = oldCriticalTemp - 20;

    LE_ASSERT(le_temp_SetThreshold(paSensorRef, "HI_NORMAL_THRESHOLD", refNormalTemp) == LE_OK);
    LE_ASSERT(le_temp_SetThreshold(paSensorRef, "HI_CRITICAL_THRESHOLD", refCriticalTemp) == LE_OK);
    LE_INFO("le_temp_SetThreshold for PA (hi_norm.%d, hi_crit.%d)",
            refNormalTemp,
            refCriticalTemp);

    LE_ASSERT(le_temp_GetThreshold(paSensorRef, "HI_NORMAL_THRESHOLD", &normalTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(paSensorRef, "HI_CRITICAL_THRESHOLD", &criticalTemp) == LE_OK);
    LE_INFO("le_temp_GetThreshold for PA (hi_norm.%d, hi_crit.%d)",
            normalTemp,
            criticalTemp);

    LE_ASSERT(normalTemp == refNormalTemp);
    LE_ASSERT(criticalTemp == refCriticalTemp);

    LE_ASSERT(le_temp_SetThreshold(paSensorRef, "HI_NORMAL_THRESHOLD", oldNormalTemp) == LE_OK);
    LE_ASSERT(le_temp_SetThreshold(paSensorRef, "HI_CRITICAL_THRESHOLD", oldCriticalTemp) == LE_OK);
    LE_INFO("Restore Initial thresold values for PA (hi_norm.%d, hi_crit.%d)",
            oldNormalTemp,
            oldCriticalTemp);


    LE_ASSERT(le_temp_GetThreshold(paSensorRef, "HI_NORMAL_THRESHOLD", &normalTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(paSensorRef, "HI_CRITICAL_THRESHOLD", &criticalTemp) == LE_OK);
    LE_INFO("le_temp_GetThreshold for PA (hi_norm.%d, hi_crit.%d)",
            normalTemp,
            criticalTemp);

    LE_ASSERT(normalTemp == oldNormalTemp);
    LE_ASSERT(criticalTemp == oldCriticalTemp);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *  - le_temp_GetTemperature()
 *  - le_temp_SetThreshold()
 */
//--------------------------------------------------------------------------------------------------
static void Testle_temp_SetPcThresholdEvent
(
    void
)
{
    int32_t normTemp = 0;
    int32_t critTemp = 0;
    int32_t loNormTemp = 0;
    int32_t loCritTemp = 0;
    int32_t temperature = 0;

    TimeCounter = 0;
    PoolingPause = 1;
    LE_INFO("Set PoolingPause %d", PoolingPause);

    le_thread_Ref_t thread = le_thread_Create("tempTest", DisplayTempThread, NULL);
    le_thread_Start(thread);

    LE_INFO("!!! YOU HAVE %d SECOND TO SET THE MODULE AT THE TEMP REFERENCE !!!", WAIT_TIME);
    TimeCounter = 0;

    PoolingPause = 2;
    LE_INFO("Set PoolingPause %d", PoolingPause);

    sleep(WAIT_TIME);

    // Get current Power Controller Temperature
    le_temp_SensorRef_t pcSensorRef = le_temp_Request("POWER_CONTROLLER");
    LE_ASSERT(le_temp_GetTemperature(pcSensorRef, &temperature) == LE_OK);
    LE_INFO("le_temp_GetTemperature returns %d degrees Celsius for PC", temperature);

    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "LO_CRITICAL_THRESHOLD", &loCritTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "LO_NORMAL_THRESHOLD", &loNormTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "HI_NORMAL_THRESHOLD", &normTemp) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "HI_CRITICAL_THRESHOLD", &critTemp) == LE_OK);
    LE_INFO("le_temp_GetThreshold for PC (lo_crit.%d, lo_norm.%d, hi_norm.%d, hi_crit.%d)",
            loCritTemp,
            loNormTemp,
            normTemp,
            critTemp);

    // Set Normal Platform threshold Temperature
    critTemp = temperature + 20;
    normTemp = temperature + 10;
    loNormTemp = temperature - 10;
    loCritTemp = temperature - 20;

    LE_ASSERT(le_temp_SetThreshold(pcSensorRef, "LO_CRITICAL_THRESHOLD", loCritTemp) == LE_OK);
    LE_ASSERT(le_temp_SetThreshold(pcSensorRef, "LO_NORMAL_THRESHOLD", loNormTemp) == LE_OK);
    LE_ASSERT(le_temp_SetThreshold(pcSensorRef, "HI_NORMAL_THRESHOLD", normTemp) == LE_OK);
    LE_ASSERT(le_temp_SetThreshold(pcSensorRef, "HI_CRITICAL_THRESHOLD", critTemp) == LE_OK);

    PoolingPause = 0;

    LE_ASSERT(le_temp_StartMonitoring() == LE_OK);

    LE_INFO("!!! YOU MUST REBOOT THE MODULE !!!");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *  - le_temp_GetTemperature()
 *  - le_temp_SetThreshold()
 */
//--------------------------------------------------------------------------------------------------
static void Testle_temp_SetPaThresholdEvent
(
    void
)
{
    int32_t normTemp = 0;
    int32_t critTemp = 0;
    int32_t temperature = 0;

    TimeCounter = 0;
    PoolingPause = 1;
    LE_INFO("Set PoolingPause %d", PoolingPause);

    le_thread_Ref_t thread = le_thread_Create("tempTest", DisplayTempThread, NULL);
    le_thread_Start(thread);

    LE_INFO("!!! YOU HAVE %d SECOND TO SET THE MODULE AT THE TEMP REFERENCE !!!", WAIT_TIME);
    TimeCounter = 0;

    PoolingPause = 2;
    LE_INFO("Set PoolingPause %d", PoolingPause);

    sleep(WAIT_TIME);

    // Get current PA Temperature
    le_temp_SensorRef_t paSensorRef = le_temp_Request("POWER_AMPLIFIER");
    LE_ASSERT(le_temp_GetTemperature(paSensorRef, &temperature) == LE_OK);
    LE_INFO("le_temp_GetTemperature returns %d degree Celsius for PA", temperature);

    // Set Normal threshold Tempeartaure
    normTemp = temperature + 10;
    critTemp = temperature + 20;

    LE_ASSERT(le_temp_SetThreshold(paSensorRef, "HI_NORMAL_THRESHOLD", normTemp) == LE_OK);
    LE_ASSERT(le_temp_SetThreshold(paSensorRef, "HI_CRITICAL_THRESHOLD", critTemp) == LE_OK);
    LE_INFO("Temperature threshold are set to (%d, %d) in degree Celsius",
            normTemp,
            critTemp);

    PoolingPause = 0;

    LE_ASSERT(le_temp_StartMonitoring() == LE_OK);

    LE_INFO("!!! YOU MUST REBOOT THE MODULE !!!");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test:
 *  - le_temp_AddThresholdEventHandler()
 *  - le_temp_rEMOVEThresholdEventHandler()
 */
//--------------------------------------------------------------------------------------------------
static void Testle_temp_ThresholdEvent
(
    void
)
{
    struct timespec ts;
    le_temp_ThresholdEventHandlerRef_t ref;
    int32_t loNormThreshold = 0;
    int32_t loCritThreshold = 0;
    int32_t hiNormThreshold = 0;
    int32_t hiCritThresholdp = 0;

    le_temp_SensorRef_t pcSensorRef = le_temp_Request("POWER_CONTROLLER");
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "LO_CRITICAL_THRESHOLDL", &loCritThreshold) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "LO_NORMAL_THRESHOLD", &loNormThreshold) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "HI_NORMAL_THRESHOLD", &hiNormThreshold) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(pcSensorRef, "HI_CRITICAL_THRESHOLD", &hiCritThresholdp) == LE_OK);
    LE_INFO("le_temp_GetThreshold for PC (lo_crit.%d, lo_norm.%d, hi_norm.%d, hi_crit.%d)",
            loCritThreshold,
            loNormThreshold,
            hiNormThreshold,
            hiCritThresholdp);

    le_temp_SensorRef_t paSensorRef = le_temp_Request("POWER_AMPLIFIER");
    LE_ASSERT(le_temp_GetThreshold(paSensorRef, "HI_NORMAL_THRESHOLD", &hiNormThreshold) == LE_OK);
    LE_ASSERT(le_temp_GetThreshold(paSensorRef, "HI_CRITICAL_THRESHOLD", &hiCritThresholdp) == LE_OK);
    LE_INFO("le_temp_GetThreshold for PA (hi_norm.%d, hi_crit.%d)",
            hiNormThreshold,
            hiCritThresholdp);

    TimeCounter = 0;
    PoolingPause = 1;
    LE_INFO("Set PoolingPause %d", PoolingPause);

    ref = le_temp_AddThresholdEventHandler(ThresholdEventHandlerFunc, NULL);
    LE_ASSERT(ref != NULL);
    LE_INFO("ref  0x%p", ref);
    le_temp_RemoveThresholdEventHandler(ref);

    le_thread_Ref_t thread = le_thread_Create("TempTest",DisplayTempThread,NULL);
    le_thread_Start(thread);

    le_thread_Ref_t thread2 = le_thread_Create("EventThread",EventThread,NULL);
    le_thread_Start(thread2);

    sem_init(&SemaphoreCriticalEvent,0,0);

    PoolingPause = 2;
    LE_INFO("Set PoolingPause %d", PoolingPause);

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        LE_ERROR("Cannot get current time");
        return;
    }

    LE_INFO("!!! YOU MUST WARM UP OR COLD DOWN THE MODULE in %d second !!!", WAIT_TIME_EVENT);

    ts.tv_sec += WAIT_TIME_EVENT; // Wait for 480 seconds.
    ts.tv_nsec += 0;

    if ( sem_timedwait(&SemaphoreCriticalEvent,&ts) == -1 )
    {
        LE_WARN("errno %d", errno);
        if ( errno == ETIMEDOUT)
        {
            LE_WARN("Timeout for Warning Event");
            return;
        }
    }

    PoolingPause = 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * App init.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int testNumberInt = 0;

    if (le_arg_NumArgs() == 1)
    {
        const char* testNumberIntPtr = le_arg_GetArg(0);
        if (NULL == testNumberIntPtr)
        {
            LE_ERROR("testNumberIntPtr is NULL");
            exit(EXIT_FAILURE);
        }
        testNumberInt = atoi(testNumberIntPtr);
        LE_DEBUG("Test Sequence. %d", testNumberInt);

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
                LE_INFO("======== Testle_temp_SetGetPcThresholds Test ========");
                Testle_temp_SetGetPcThresholds();
                LE_INFO("======== Testle_temp_SetGetPcThresholds Test PASSED ========");
            }
            break;

            case 2:
            {
                LE_INFO("======== Testle_temp_SetGetPaThresholds Test ========");
                Testle_temp_SetGetPaThresholds();
                LE_INFO("======== Testle_temp_SetGetPaThresholds Test PASSED ========");
            }
            break;

            case 3:
            {
                LE_INFO("======== Testle_temp_SetPcThresholdEvent Test ========");
                Testle_temp_SetPcThresholdEvent();
                LE_INFO("======== Testle_temp_SetPcThresholdEvent Test PASSED ========");
            }
            break;

            case 4:
            {
                LE_INFO("======== Testle_temp_SetPaThresholdEvent Test ========");
                Testle_temp_SetPaThresholdEvent();
                LE_INFO("======== Testle_temp_SetPaThresholdEvent Test PASSED ========");
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
                SetDefaultPaThreshold();
            }
            break;

            case 7:
            {
                SetDefaultPcThreshold();
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

