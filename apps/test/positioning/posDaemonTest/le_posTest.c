 /**
  * This module implements the le_pos's unit tests.
  *
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <semaphore.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>

#include "interfaces.h"
#include "main.h"


#ifdef ENABLE_SIMUL

#define NAVIGATION_HANDLER_CALL_THRESHOLD   5

static uint32_t NavigationHandlerCallCount = 0;

#endif

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Profile State Change Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TenMeterNavigationHandler(le_pos_SampleRef_t positionSampleRef, void* contextPtr)
{
    int32_t  val, val1, accuracy;
    uint32_t uval, uAccuracy;
    // Date parameters
    uint16_t year;
    uint16_t month;
    uint16_t day;
    // Time parameters
    uint16_t hours;
    uint16_t minutes;
    uint16_t seconds;
    uint16_t milliseconds;


    if(positionSampleRef == NULL)
    {
        LE_ERROR("New Position sample is NULL!");
    }
    else
    {
        LE_INFO("New Position sample %p", positionSampleRef);
    }

#ifdef ENABLE_SIMUL
    NavigationHandlerCallCount++;
#endif

    le_pos_sample_Get2DLocation(positionSampleRef, &val, &val1, &accuracy);
    LE_INFO("Check le_pos_sample_Get2DLocation passed, lat.%d, long.%d,
            accuracy.%d", val, val1, accuracy);

    le_pos_sample_GetDate(positionSampleRef, &year, &month, &day);
    LE_INFO("Check le_pos_sample_GetDate passed, year.%d, month.%d, day.%d", year, month, day);

    le_pos_sample_GetTime(positionSampleRef, &hours, &minutes, &seconds, &milliseconds);
    LE_INFO("Check le_pos_sample_GetTime passed, hours.%d, minutes.%d, seconds.%d, milliseconds.%d",
            hours, minutes, seconds, milliseconds);

    le_pos_sample_GetAltitude(positionSampleRef, &val, &accuracy);
    LE_INFO("Check le_pos_sample_GetAltitude passed, alt.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetHorizontalSpeed(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("Check le_pos_sample_GetHorizontalSpeed passed, hSpeed.%u, accuracy.%u", uval,
            uAccuracy);

    le_pos_sample_GetVerticalSpeed(positionSampleRef, &val, &accuracy);
    LE_INFO("Check le_pos_sample_GetVerticalSpeed passed, vSpeed.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetHeading(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("Check le_pos_sample_GetHeading passed, heading.%u, accuracy.%u", uval, uAccuracy);

    le_pos_sample_GetDirection(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("Check le_pos_sample_GetDirection passed, direction.%u, accuracy.%u", uval, uAccuracy);

    le_pos_sample_Release(positionSampleRef);

#ifdef ENABLE_SIMUL
    if (NavigationHandlerCallCount == NAVIGATION_HANDLER_CALL_THRESHOLD)
    {
        LE_INFO("Reached feedback threshold (%d), test PASS.", NAVIGATION_HANDLER_CALL_THRESHOLD);
        exit(0);
    }
    else
    {
        LE_INFO("NavigationHandlerCallCount=%d", NavigationHandlerCallCount);
    }
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Profile State Change Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TwentyMeterNavigationHandler(le_pos_SampleRef_t positionSampleRef, void* contextPtr)
{
    int32_t  val, val1, accuracy;
    uint32_t uval, uAccuracy;

    if(positionSampleRef == NULL)
    {
        LE_ERROR("New Position sample is NULL!");
    }
    else
    {
        LE_INFO("New Position sample %p", positionSampleRef);
    }

#ifdef ENABLE_SIMUL
    NavigationHandlerCallCount++;
#endif

    le_pos_sample_Get2DLocation(positionSampleRef, &val, &val1, &accuracy);
    LE_INFO("Check le_pos_sample_Get2DLocation passed, lat.%d, long.%d, accuracy.%d", val, val1,
            accuracy);

    le_pos_sample_GetAltitude(positionSampleRef, &val, &accuracy);
    LE_INFO("Check le_pos_sample_GetAltitude passed, alt.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetHorizontalSpeed(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("Check le_pos_sample_GetHorizontalSpeed passed, hSpeed.%u, accuracy.%u", uval, uAccuracy);

    le_pos_sample_GetVerticalSpeed(positionSampleRef, &val, &accuracy);
    LE_INFO("Check le_pos_sample_GetVerticalSpeed passed, vSpeed.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetHeading(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("Check le_pos_sample_GetHeading passed, heading.%u, accuracy.%u", uval, uAccuracy);

    le_pos_sample_GetDirection(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("Check le_pos_sample_GetDirection passed, direction.%u, accuracy.%u", uval, uAccuracy);

    le_pos_sample_Release(positionSampleRef);

#ifdef ENABLE_SIMUL
    if (NavigationHandlerCallCount == 5)
    {
        exit(0);
    }
#endif
}

//--------------------------------------------------------------------------------------------------
//                                       Test Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Test: Fix On Demand.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_pos_Fix()
{
    int32_t           latitude;
    int32_t           longitude;
    int32_t           altitude;
    int32_t           hAccuracy;
    int32_t           vAccuracy;
    uint32_t          hSpeed;
    uint32_t          hSpeedAccuracy;
    int32_t           vSpeed;
    int32_t           vSpeedAccuracy;
    uint32_t          heading;
    uint32_t          headingAccuracy=0;
    uint32_t          direction;
    uint32_t          directionAccuracy=0;
    le_pos_FixState_t fixState;
    le_result_t       res;

    res = le_pos_GetFixState(&fixState);
    CU_ASSERT_EQUAL(res, LE_OK);
    LE_INFO("position fix state %d", fixState);

    res = le_pos_Get2DLocation(&latitude, &longitude, &hAccuracy);
    CU_ASSERT_EQUAL(res, LE_OK);
    LE_INFO("Check le_pos_Get2DLocation latitude.%d, longitude.%d, hAccuracy.%d", latitude,
            longitude, hAccuracy);

    res = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);
    CU_ASSERT_EQUAL(res, LE_OK);
    LE_INFO("Check le_pos_Get3DLocation latitude.%d, longitude.%d, hAccuracy.%d, altitude.%d,
            vAccuracy.%d", latitude, longitude, hAccuracy, altitude, vAccuracy);

    res = le_pos_GetMotion(&hSpeed, &hSpeedAccuracy, &vSpeed, &vSpeedAccuracy);
#if ENABLE_SIMUL
    CU_ASSERT_EQUAL(res, LE_OUT_OF_RANGE); // No vertical speed available with gnss-AT
#else
    CU_ASSERT_EQUAL(res, LE_OK);
#endif
    LE_INFO("Check le_pos_GetMotion hSpeed.%u, hSpeedAccuracy.%u, vSpeed.%d, vSpeedAccuracy.%d",
            hSpeed, hSpeedAccuracy, vSpeed, vSpeedAccuracy);

    res = le_pos_GetHeading(&heading, NULL);
    CU_ASSERT_EQUAL(res, LE_OK);
    LE_INFO("Check le_pos_GetHeading heading.%u, headingAccuracy.%u", heading, headingAccuracy);

    res = le_pos_GetDirection(&direction, NULL);
    CU_ASSERT_EQUAL(res, LE_OK);
    LE_INFO("Check le_pos_GetDirection direction.%u, directionAccuracy.%u", direction,
            directionAccuracy);

    sleep (6);

    res = le_pos_Get2DLocation(&latitude, &longitude, &hAccuracy);
    CU_ASSERT_EQUAL(res, LE_OK);
    LE_INFO("Check le_pos_Get2DLocation latitude.%d, longitude.%d, hAccuracy.%d", latitude,
            longitude, hAccuracy);

    res = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);
    CU_ASSERT_EQUAL(res, LE_OK);
    LE_INFO("Check le_pos_Get3DLocation latitude.%d, longitude.%d, hAccuracy.%d, altitude.%d,
            vAccuracy.%d", latitude, longitude, hAccuracy, altitude, vAccuracy);

    res = le_pos_GetMotion(&hSpeed, &hSpeedAccuracy, &vSpeed, &vSpeedAccuracy);
#if ENABLE_SIMUL
    CU_ASSERT_EQUAL(res, LE_OUT_OF_RANGE); // No vertical speed available with gnss-AT
#else
    CU_ASSERT_EQUAL(res, LE_OK);
#endif
    LE_INFO("Check le_pos_GetMotion hSpeed.%u, hSpeedAccuracy.%u, vSpeed.%d, vSpeedAccuracy.%d",
            hSpeed, hSpeedAccuracy, vSpeed, vSpeedAccuracy);

    res = le_pos_GetHeading(&heading, NULL);
    CU_ASSERT_EQUAL(res, LE_OK);
    LE_INFO("Check le_pos_GetHeading heading.%u, headingAccuracy.%u", heading, headingAccuracy);

    res = le_pos_GetDirection(&direction, NULL);
    CU_ASSERT_EQUAL(res, LE_OK);
    LE_INFO("Check le_pos_GetDirection direction.%u, directionAccuracy.%u", direction,
            directionAccuracy);

}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Navigation.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_pos_Navigation()
{
    le_pos_MovementHandlerRef_t   navigHandlerRef;

    navigHandlerRef = le_pos_AddMovementHandler(10, 0, TenMeterNavigationHandler, NULL);
    CU_ASSERT_PTR_NOT_NULL(navigHandlerRef);

    navigHandlerRef = le_pos_AddMovementHandler(20, 0, TwentyMeterNavigationHandler, NULL);
    CU_ASSERT_PTR_NOT_NULL(navigHandlerRef);
}





