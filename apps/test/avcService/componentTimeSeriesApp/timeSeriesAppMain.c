//--------------------------------------------------------------------------------------------------
/**
 * @file timeSeriesAppMain.c
 *
 * This component is used for testing the AirVantage Time Series feature.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"

#include "le_print.h"

static double temperatureCount = 20;
static int humidityCount = 0;

#define HUMIDITY_INCREMENT           1000
#define HUMIDITY_SCALE               .001

#define SLEEP_MSEC                   100                        // sample every 100 milli second.
#define SAMPLE_RATE                  .01                        // sample every 100 milli second.

#define SLEEP_USEC                   (1000*SLEEP_MSEC)

#define TEMPERATURE_INCREMENT        0.01
#define TEMPERATURE_SCALE            100


//--------------------------------------------------------------------------------------------------
/**
 * Print time series status
 */
//--------------------------------------------------------------------------------------------------

void PrintTimeSeriesStatus
(
    le_avdata_AssetInstanceRef_t instRef,
    char* FieldName
)
{
    bool isTimeSeries = false;
    int numDataPoint = 0;

    le_avdata_GetTimeSeriesStatus(instRef, FieldName, &isTimeSeries, &numDataPoint);
    LE_FATAL_IF(isTimeSeries == false, "Time series not enabled on %s.", FieldName);
    LE_WARN("Number of %s data points recorded = %d", FieldName, numDataPoint);
}


//--------------------------------------------------------------------------------------------------
/**
 * Init the component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_result_t result = LE_OK;
    int i;
    bool isObserve = false;
    le_avdata_AssetInstanceRef_t instZeroRef;
    instZeroRef = le_avdata_Create("myHouse");
    uint64_t utcMilliSec;
    struct timeval tv;

    // Start Time series, sampling every 100 msec i.e., a time stamp factor of .01.
    result = le_avdata_StartTimeSeries(instZeroRef, "Humidity", HUMIDITY_SCALE, SAMPLE_RATE );
    result = le_avdata_StartTimeSeries(instZeroRef, "Temperature", TEMPERATURE_SCALE, SAMPLE_RATE );

    if (result != LE_OK)
    {
        LE_FATAL("Not able to start time series data.");
    }

    // Collect 1000 data points.
    for (i = 0; i < 1000; i++)
    {
        result = le_avdata_SetInt(instZeroRef, "Humidity", humidityCount);
        humidityCount = humidityCount + HUMIDITY_INCREMENT;

        if (result == LE_NO_MEMORY)
        {
            // Ideally the data should be pushed here. I am pushing at the next stage
            // for testing.
            LE_WARN("Humidity data is written to buffer but no space for next one!");
        }

        if (result == LE_OVERFLOW)
        {
            result = le_avdata_IsObserve(instZeroRef, "Humidity", &isObserve);
            if (result == LE_OK)
            {
                PrintTimeSeriesStatus(instZeroRef, "Humidity");
                LE_FATAL_IF(le_avdata_PushTimeSeries(instZeroRef, "Humidity", true) != LE_OK,
                            "Failed to push humidity history");
                sleep(1);
            }
        }

        gettimeofday(&tv, NULL);
        utcMilliSec = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;

        result = le_avdata_RecordFloat(instZeroRef, "Temperature", temperatureCount, utcMilliSec);
        temperatureCount = temperatureCount + TEMPERATURE_INCREMENT;

        if (result == LE_NO_MEMORY)
        {
            // Ideally the data should be pushed here. I am pushing at the next stage
            // for testing.
            LE_WARN("Temperature data is written to buffer but no space for next one!");
        }

        if (result == LE_OVERFLOW)
        {
            result = le_avdata_IsObserve(instZeroRef, "Temperature", &isObserve);
            if (result == LE_OK)
            {
                PrintTimeSeriesStatus(instZeroRef, "Temperature");
                LE_FATAL_IF(le_avdata_PushTimeSeries(instZeroRef, "Temperature", true) != LE_OK,
                            "Failed to push temperature history");
                sleep(1);
            }
        }

        usleep(SLEEP_USEC);
    }

    // Job is done! Push data and get out!
    result = le_avdata_IsObserve(instZeroRef, "Humidity", &isObserve);
    if (result == LE_OK)
    {
        PrintTimeSeriesStatus(instZeroRef, "Humidity");
        LE_FATAL_IF(le_avdata_PushTimeSeries(instZeroRef, "Humidity", true) != LE_OK,
                    "Failed to push humidity history");

        sleep(1);
    }

    result = le_avdata_IsObserve(instZeroRef, "Temperature", &isObserve);
    if (result == LE_OK)
    {
        PrintTimeSeriesStatus(instZeroRef, "Temperature");
        LE_FATAL_IF(le_avdata_PushTimeSeries(instZeroRef, "Temperature", true) != LE_OK,
                    "Failed to push temperature history");
    }
}

