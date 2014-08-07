/** @file paQmiPosTest.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */


#include "legato.h"
#include "pa_gnss.h"

#define MAX_NUM_ACQS        20
static int NumAcqs = 0;
static pa_Gnss_Position_t currentPos;

static void GetCurrentHandler
(
    le_timer_Ref_t timerRef
)
{
    static int numChecks = 0;

    LE_INFO("Getting current position.");
    pa_Gnss_Position_t position;
    LE_ASSERT(pa_gnss_GetLastPositionData(&position) == LE_OK);

    if ( (position.latitude ==              currentPos.latitude) &&
         (position.longitude ==             currentPos.longitude) &&
         (position.altitude ==              currentPos.altitude) &&
         (position.hSpeed ==                currentPos.hSpeed) &&
         (position.vSpeed ==                currentPos.vSpeed) &&
         (position.track ==                 currentPos.track) &&
         (position.heading ==               currentPos.heading) &&
         (position.hdop ==                  currentPos.hdop) &&
         (position.hUncertainty ==          currentPos.hUncertainty) &&
         (position.vUncertainty ==          currentPos.vUncertainty) &&
         (position.hSpeedUncertainty ==     currentPos.hSpeedUncertainty) &&
         (position.vSpeedUncertainty ==     currentPos.vSpeedUncertainty) &&
         (position.headingUncertainty ==    currentPos.headingUncertainty) &&
         (position.trackUncertainty ==      currentPos.trackUncertainty) &&
         (position.vdop ==                  currentPos.vdop) &&
         (position.time.hours ==            currentPos.time.hours) &&
         (position.time.minutes ==          currentPos.time.minutes) &&
         (position.time.seconds ==          currentPos.time.seconds) &&
         (position.time.milliseconds ==     currentPos.time.milliseconds) &&
         (position.date.day ==              currentPos.date.day) &&
         (position.date.month ==            currentPos.date.month) &&
         (position.date.year ==             currentPos.date.year)
        )
    {
        LE_INFO("Comparison of last position is correct.");
    }
    else
    {
        LE_FATAL("Comparison of last position is incorrect.");
    }

    numChecks++;

    if (numChecks >= 2*MAX_NUM_ACQS)
    {
        LE_INFO("Test complete.");
        exit(EXIT_SUCCESS);
    }
}


static void PositionEventHandler
(
    pa_Gnss_Position_Ref_t  position  ///< Data received with the handler
)
{
    NumAcqs++;
    LE_INFO("Got position event. %d", NumAcqs);

    printf("hours %d, minutes %d, seconds %d, ms %d, year %d, month %d, day %d\n",
           position->time.hours,
           position->time.minutes,
           position->time.seconds,
           position->time.milliseconds,
           position->date.year,
           position->date.month,
           position->date.day);

    printf("latitude %d, longitude %d, altitude %d,"
           "hSpeed %d, vSpeed %d,"
           "track %d, heading %d,"
           "hdop %d, vdop %d"
           "hUncertainty %d, vUncertainty %d"
           "headingUncertainty %d, trackUncertainty %d\n",
           position->latitude,position->longitude,position->altitude,
           position->hSpeed,position->vSpeed,
           position->track,position->heading,
           position->hdop,position->vdop,
           position->hUncertainty,position->vUncertainty,
           position->headingUncertainty,position->trackUncertainty);

    // Save the current position for comparison later.
    currentPos.latitude =              position->latitude;
    currentPos.longitude =             position->longitude;
    currentPos.altitude =              position->altitude;
    currentPos.hSpeed =                position->hSpeed;
    currentPos.vSpeed =                position->vSpeed;
    currentPos.track =                 position->track;
    currentPos.heading =               position->heading;
    currentPos.hdop =                  position->hdop;
    currentPos.hUncertainty =          position->hUncertainty;
    currentPos.vUncertainty =          position->vUncertainty;
    currentPos.hSpeedUncertainty =     position->hSpeedUncertainty;
    currentPos.vSpeedUncertainty =     position->vSpeedUncertainty;
    currentPos.headingUncertainty =    position->headingUncertainty;
    currentPos.trackUncertainty =      position->trackUncertainty;
    currentPos.vdop =                  position->vdop;
    currentPos.time.hours =            position->time.hours;
    currentPos.time.minutes =          position->time.minutes;
    currentPos.time.seconds =          position->time.seconds;
    currentPos.time.milliseconds =     position->time.milliseconds;
    currentPos.date.day =              position->date.day;
    currentPos.date.month =            position->date.month;
    currentPos.date.year =             position->date.year;

    le_mem_Release(position);

    if (NumAcqs >= MAX_NUM_ACQS)
    {
        LE_ASSERT(pa_gnss_Stop() == LE_OK);
        LE_DEBUG("************** Stopped acquisitions. *************");
    }
}


COMPONENT_INIT
{
    LE_INFO("======== Begin Positioning Platform Adapter's QMI implementation Test  ========");

    LE_ASSERT(pa_gnss_Init() == LE_OK);

    // The modem seems to need time to initialize.
    sleep(1);

    LE_ASSERT(pa_gnss_SetAcquisitionRate(3) == LE_OK);

    pa_gnss_AddPositionDataHandler(PositionEventHandler);

    LE_ASSERT(pa_gnss_Start() == LE_OK);

    // Set a timer that gets the current position.
    le_timer_Ref_t GetCurrentTimer = le_timer_Create("GetCurrentPos");

    le_clk_Time_t interval = {2, 0};
    LE_ASSERT(le_timer_SetInterval(GetCurrentTimer, interval) == LE_OK);
    LE_ASSERT(le_timer_SetRepeat(GetCurrentTimer, 0) == LE_OK);
    LE_ASSERT(le_timer_SetHandler(GetCurrentTimer, GetCurrentHandler) == LE_OK);

    LE_ASSERT(le_timer_Start(GetCurrentTimer) == LE_OK);
}
