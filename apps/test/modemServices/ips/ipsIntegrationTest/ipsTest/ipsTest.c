 /**
  * This module implements the le_ips's tests.
  *
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

/*
 * Instruction to execute this test
 * 1) install application test.
 * 2) Start log trace 'logread -f | grep 'ips'
 * 3) Start application 'app start ipsTest'
 * 4) Execute application 'app runProc ipsTest --exe=ipsTest -- [command..] see PrintUsage()'
 * 5) check trace for the following INFO  trace:
 */

#include "legato.h"
#include "interfaces.h"

/* Waiting time for Threshold Events */
#define WAIT_TIME_EVENT 480
/* Number of Threshold Events to wait */
#define NB_EVENTS 10

/* Default AR/WP Input Voltage thresholds */
#define DEFAULT_IPS_HI_CRITICAL_THRESHOLD   4400
#define DEFAULT_IPS_NORMAL_THRESHOLD        3600
#define DEFAULT_ISP_WARNING_THRESHOLD       3400
#define DEFAULT_IPS_CRITICAL_THRESHOLD      3200

#define TEST_IPS_HI_CRITICAL_THRESHOLD      4000
#define TEST_IPS_NORMAL_THRESHOLD           3700
#define TEST_IPS_WARNING_THRESHOLD          3600
#define TEST_IPS_CRITICAL_THRESHOLD         3400

/* Simulated external battery level */
#define TEST_IPS_EXT_BATTERY_LEVEL          57

/* Value for managed Current value displaying and remaining event */
static int WaitForNbEvents;

/* Handler reference for Event waiting */
static le_ips_ThresholdEventHandlerRef_t Ref;

/* Semaphore use for Critical Event waiting */
static sem_t SemaphoreCriticalEvent;

//--------------------------------------------------------------------------------------------------
/**
 * Helper.
 */
//--------------------------------------------------------------------------------------------------
static void PrintUsage()
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
                    "app runProc ipsTest --exe=ipsTest -- <option> ..:",
                    "<option>:"
                    "  ALL  : Execute all test",
                    "  HANDLER : Install a Input Voltage monitoring handler to monitor events.",
                    "    Wait for some events or stop application by CTR+Z",
                    "  RESTORE : Restore Default Input Voltage threshold values",
                    "  SET <critical> <warning> <normal> <high critical>: Set the"
                    " Platform warning and critical input voltage thresholds in [mV].",
                    "      Reboot Required"
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
    LE_INFO("======== Test IPS implementation Test HELP ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_GetInputVoltage()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_GetInputVoltage
(
    void
)
{
    uint32_t voltage = 0;

    LE_INFO("======== Testle_ips_GetInputVoltage Test ========");
    LE_ASSERT_OK(le_ips_GetInputVoltage(&voltage));
    LE_ASSERT(voltage != 0);
    LE_INFO("le_ips_GetInputVoltage returns %d mV => %d,%03d V",
             voltage, voltage/1000, voltage%1000);
    printf("le_ips_GetInputVoltage returns %d mV => %d,%03d V\n",
            voltage, voltage/1000, voltage%1000);
    LE_INFO("======== Testle_ips_GetInputVoltage Test PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_GetPowerSource()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_GetPowerSource
(
    void
)
{
    le_ips_PowerSource_t powerSource;

    LE_INFO("======== Testle_ips_GetPowerSource Test ========");
    LE_ASSERT_OK(le_ips_GetPowerSource(&powerSource));
    LE_ASSERT(LE_IPS_POWER_SOURCE_EXTERNAL == powerSource);
    LE_INFO("======== Testle_ips_GetPowerSource Test PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_GetBatteryLevel()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_GetBatteryLevel
(
    void
)
{
    uint8_t batteryLevel;

    LE_INFO("======== Testle_ips_GetBatteryLevel Test ========");
    LE_ASSERT_OK(le_ips_GetBatteryLevel(&batteryLevel));
    LE_ASSERT(0 == batteryLevel);
    LE_INFO("======== Testle_ips_GetBatteryLevel Test PASSED ========");
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_SetBatteryLevel()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_SetBatteryLevel
(
    void
)
{
    uint8_t batteryLevel;
    le_ips_PowerSource_t powerSource;

    LE_INFO("======== Testle_ips_SetBatteryLevel Test ========");
    LE_ASSERT_OK(le_ips_SetBatteryLevel(TEST_IPS_EXT_BATTERY_LEVEL));
    LE_ASSERT_OK(le_ips_GetBatteryLevel(&batteryLevel));
    LE_ASSERT(TEST_IPS_EXT_BATTERY_LEVEL == batteryLevel);
    LE_ASSERT_OK(le_ips_GetPowerSource(&powerSource));
    LE_ASSERT(LE_IPS_POWER_SOURCE_BATTERY == powerSource);
    LE_INFO("======== Testle_ips_SetBatteryLevel Test PASSED ========");
}


//--------------------------------------------------------------------------------------------------
/**
 *  Restore default QMI input voltage threshold values.
 *
 */
//--------------------------------------------------------------------------------------------------
static void RestoreVoltageThresholds
(
    void
)
{
    LE_INFO("======== RestoreVoltageThresholds ========");
    LE_INFO("Restore Default QMI thresholds le_ips_SetVoltageThresholds (%d, %d, %d, %d) in [mV]",
        DEFAULT_IPS_CRITICAL_THRESHOLD, DEFAULT_ISP_WARNING_THRESHOLD,
        DEFAULT_IPS_NORMAL_THRESHOLD, DEFAULT_IPS_HI_CRITICAL_THRESHOLD);
    LE_ASSERT(le_ips_SetVoltageThresholds( DEFAULT_IPS_CRITICAL_THRESHOLD,
        DEFAULT_ISP_WARNING_THRESHOLD, DEFAULT_IPS_NORMAL_THRESHOLD,
        DEFAULT_IPS_HI_CRITICAL_THRESHOLD) == LE_OK);
    LE_INFO("!!!!!!! YOU MUST REBOOT THE MODULE (for AR7, AR8 and WP85 platforms) !!!!!!!");
    LE_INFO("======== RestoreVoltageThresholds DONE ========");
    exit(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_SetVoltageThresholds()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_SetVoltageThresholds
(
    uint16_t criticalInVolt,
    uint16_t warningInVolt,
    uint16_t normalInVolt,
    uint16_t hiCriticalInVolt
)
{
    LE_INFO("======== Testle_ips_SetVoltageThresholds Test ========");
    LE_ASSERT(le_ips_SetVoltageThresholds( criticalInVolt, warningInVolt,
        normalInVolt, hiCriticalInVolt) == LE_OK);

    LE_INFO("le_ips_SetVoltageThresholds (%d, %d, %d, %d) in [mV]",
        criticalInVolt, warningInVolt, normalInVolt, hiCriticalInVolt);
    LE_INFO("!!!!!!! YOU MUST REBOOT THE MODULE (for AR7, AR8 and WP85 platforms) !!!!!!!");

    LE_INFO("======== Testle_ips_SetVoltageThresholds Test PASSED ========");
}


//--------------------------------------------------------------------------------------------------
/**
 * Input voltage Threshold events function handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void ThresholdEventHandlerFunc
(
    le_ips_ThresholdStatus_t event,
    void* contextPtr
)
{
    WaitForNbEvents--;
    LE_INFO("Input Voltage monitoring event %d Remaining %d", event, WaitForNbEvents);

    switch(event)
    {
        ///< High Critical input voltage threshold is reached.
        case LE_IPS_VOLTAGE_HI_CRITICAL:
        {
            LE_INFO("LE_IPS_VOLTAGE_HI_CRITICAL");
        }
        break;

        ///< Normal input voltage threshold is reached.
        case LE_IPS_VOLTAGE_NORMAL:
        {
            LE_INFO("LE_IPS_VOLTAGE_NORMAL");
        }
        break;

        ///< Low Warning input voltage threshold is reached.
        case LE_IPS_VOLTAGE_WARNING:
        {
            LE_INFO("LE_IPS_VOLTAGE_WARNING");
        }
        break;

        ///< Low Critical input voltage threshold is reached.
        case LE_IPS_VOLTAGE_CRITICAL:
        {
            LE_INFO("LE_IPS_VOLTAGE_CRITICAL");
        }
        break;

        default:
        {
            LE_ERROR("Unknown Event");
        }
        break;
    }

    if( WaitForNbEvents == 0)
    {
        sem_post(&SemaphoreCriticalEvent);
    }
}


//--------------------------------------------------------------------------------------------------
/*
 * Event Thread
 */
//--------------------------------------------------------------------------------------------------
static void* EventThread
(
    void* context
)
{
    le_ips_ConnectService();

    LE_INFO("======== Testle_ips_AddThresholdEventHandler Test ========");
    Ref = le_ips_AddThresholdEventHandler(ThresholdEventHandlerFunc, NULL);
    LE_ASSERT(Ref != NULL);
    LE_INFO("Ref  0x%p", Ref);
    LE_INFO("======== Testle_ips_AddThresholdEventHandler DONE ========");

    // Run the event loop
    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/*
 * Input voltage Display Thread
 */
//--------------------------------------------------------------------------------------------------
static void* DisplayIpsThread
(
    void* context
)
{
    uint32_t voltage = 0;

    le_ips_ConnectService();

    LE_INFO("Thread Start");

    do
    {
        le_ips_GetInputVoltage(&voltage);
        LE_INFO("le_ips_GetInputVoltage return %d mV => %d,%03d V",
            voltage, voltage/1000, voltage%1000);
        sleep(2);
    }
    while (WaitForNbEvents > 0);

    // Run the event loop
    le_event_RunLoop();
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_AddThresholdEventHandler()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_AddThresholdEventHandler
(
    void
)
{
    struct timespec ts;

    WaitForNbEvents = NB_EVENTS;

    le_thread_Ref_t thread2 = le_thread_Create("EventThread2",EventThread,NULL);
    le_thread_Start(thread2);

    le_thread_Ref_t thread = le_thread_Create("ipsTestDisplay",DisplayIpsThread,NULL);
    le_thread_Start(thread);

    sem_init(&SemaphoreCriticalEvent,0,0);

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        LE_ERROR("Cannot get current time");
        return;
    }

    LE_INFO("!!!!!!! WAIT FOR FIRST IPS EVENT in %d second !!!!!!!", WAIT_TIME_EVENT);

    ts.tv_sec += WAIT_TIME_EVENT; // Wait for 480 seconds.
    ts.tv_nsec += 0;

    if ( sem_timedwait(&SemaphoreCriticalEvent,&ts) == -1 )
    {
        LE_WARN("errno %d", errno);
        if ( errno == ETIMEDOUT)
        {
            LE_WARN("Timeout for Event");
            return;
        }
    }

    WaitForNbEvents = 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_AddThresholdEventHandler(), le_ips_RemoveThresholdEventHandler()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_ThresholdEventHandler
(
    void
)
{
    le_ips_ThresholdEventHandlerRef_t handlerRef;

    WaitForNbEvents = NB_EVENTS;
    LE_INFO("======== Testle_ips_ThresholdEventHandler Test ========");
    handlerRef = le_ips_AddThresholdEventHandler(ThresholdEventHandlerFunc, NULL);
    LE_ASSERT(handlerRef != NULL);
    LE_INFO("handlerRef  0x%p", handlerRef);
    le_ips_RemoveThresholdEventHandler(handlerRef);
    LE_INFO("======== Testle_ips_ThresholdEventHandler Test PASSED ========");
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_GetVoltageThresholds(), le_ips_SetVoltageThresholds()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_SetGetVoltageThresholds
(
    void
)
{
    le_result_t res;
    uint16_t criticalInVoltOri, warningInVoltOri, normalInVoltOri, hiCriticalInVoltOri;
    uint16_t criticalInVolt, warningInVolt, normalInVolt, hiCriticalInVolt;

    LE_INFO("======== Testle_ips_GetVoltageThresholds Test ========");
    LE_ASSERT(le_ips_GetVoltageThresholds( &criticalInVoltOri, &warningInVoltOri,
        &normalInVoltOri, &hiCriticalInVoltOri) == LE_OK);
    LE_INFO("le_ips_GetVoltageThresholds (%d, %d, %d, %d) in [mV]",
        criticalInVoltOri, warningInVoltOri, normalInVoltOri, hiCriticalInVoltOri);

    res = le_ips_SetVoltageThresholds( TEST_IPS_CRITICAL_THRESHOLD,
        TEST_IPS_WARNING_THRESHOLD, TEST_IPS_NORMAL_THRESHOLD, TEST_IPS_HI_CRITICAL_THRESHOLD);
    if(res != LE_OK)
    {
        RestoreVoltageThresholds();
        LE_ASSERT(false);
    }

    res = le_ips_GetVoltageThresholds( &criticalInVolt, &warningInVolt,
            &normalInVolt, &hiCriticalInVolt);

    if( (res != LE_OK)
                    || (criticalInVolt != TEST_IPS_CRITICAL_THRESHOLD)
                    || (warningInVolt != TEST_IPS_WARNING_THRESHOLD)
                    || (normalInVolt != TEST_IPS_NORMAL_THRESHOLD)
                    || (hiCriticalInVolt != TEST_IPS_HI_CRITICAL_THRESHOLD))
    {
        RestoreVoltageThresholds();
        LE_ASSERT(false);
    }

    res = le_ips_SetVoltageThresholds( criticalInVoltOri, warningInVoltOri,
        normalInVoltOri, hiCriticalInVoltOri);
    if (res != LE_OK)
    {
        RestoreVoltageThresholds();
        LE_ASSERT( res == LE_OK);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: le_ips_GetVoltageThresholds()
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_ips_GetVoltageThresholds
(
    void
)
{
    uint16_t criticalInVolt, warningInVolt, normalInVolt, hiCriticalInVolt;

    LE_INFO("======== Testle_ips_GetVoltageThresholds Test ========");
    LE_ASSERT(le_ips_GetVoltageThresholds( &criticalInVolt, &warningInVolt,
        &normalInVolt, &hiCriticalInVolt) == LE_OK);

    LE_INFO("le_ips_GetVoltageThresholds (%d, %d, %d, %d) in [mV]",
        criticalInVolt, warningInVolt, normalInVolt, hiCriticalInVolt);

    LE_ASSERT(criticalInVolt != 0);
    LE_ASSERT(warningInVolt != 0);
    LE_ASSERT(normalInVolt != 0);
    LE_ASSERT(hiCriticalInVolt != 0);

    LE_ASSERT(criticalInVolt < warningInVolt);
    LE_ASSERT(warningInVolt < normalInVolt);
    LE_ASSERT(normalInVolt < hiCriticalInVolt);

    LE_INFO("======== Testle_ips_GetVoltageThresholds Test PASSED ========");
}


COMPONENT_INIT
{
    LE_INFO("======== Start IPS implementation Test========");

    const char *  testNumberStr = NULL;
    int nbarg = 0;
    nbarg = le_arg_NumArgs();
    LE_INFO(" nbargument %d ", nbarg );

    testNumberStr = le_arg_GetArg(0);
    if (NULL == testNumberStr)
    {
        LE_ERROR("testNumberStr is NULL");
        exit(EXIT_FAILURE);
    }
    if ( nbarg  == 1 )
    {
        if(strcmp(testNumberStr, "ALL") == 0)
        {
            Testle_ips_GetInputVoltage();
            Testle_ips_GetPowerSource();
            Testle_ips_GetBatteryLevel();
            Testle_ips_GetVoltageThresholds();
            Testle_ips_SetGetVoltageThresholds();
            Testle_ips_ThresholdEventHandler();
            Testle_ips_SetBatteryLevel();
            LE_INFO("======== Test IPS implementation Test SUCCESS ========");
        }
        else if(strcmp(testNumberStr, "HANDLER") == 0)
        {
            Testle_ips_AddThresholdEventHandler();
        }
        else if(strcmp(testNumberStr, "RESTORE") == 0)
        {
            RestoreVoltageThresholds();
        }
        else
        {
            PrintUsage();
        }
    }
    else if ( nbarg  == 5 )
    {
        if(strcmp(testNumberStr, "SET") == 0)
        {
            const char *  critStr = le_arg_GetArg(1);
            const char *  warnStr = le_arg_GetArg(2);
            const char *  normStr = le_arg_GetArg(3);
            const char *  hiCritStr = le_arg_GetArg(4);
            uint16_t critical, warning, normal, hiCritical;

            if (NULL == critStr)
            {
                LE_ERROR("critStr is NULL");
                exit(EXIT_FAILURE);
            }
            if (NULL == warnStr)
            {
                LE_ERROR("warnStr is NULL");
                exit(EXIT_FAILURE);
            }
            if (NULL == normStr)
            {
                LE_ERROR("normStr is NULL");
                exit(EXIT_FAILURE);
            }
            if (NULL == hiCritStr)
            {
                LE_ERROR("hiCritStr is NULL");
                exit(EXIT_FAILURE);
            }
            critical = atoi(critStr);
            warning = atoi(warnStr);
            normal = atoi(normStr);
            hiCritical = atoi(hiCritStr);

            Testle_ips_SetVoltageThresholds(critical, warning, normal, hiCritical);
        }
        else
        {
            PrintUsage();
        }
    }
    else
    {
        PrintUsage();
    }

    exit(EXIT_SUCCESS);
}
