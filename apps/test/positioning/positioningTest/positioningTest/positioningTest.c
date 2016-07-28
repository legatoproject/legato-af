 /**
  * This module implements the le_posCtrl's tests.
  *
  * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include <stdio.h>

#include "interfaces.h"

void Testle_pos_GetInfo();

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Navigation notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NavigationHandler(le_pos_SampleRef_t positionSampleRef, void* contextPtr)
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

    le_pos_sample_Get2DLocation(positionSampleRef, &val, &val1, &accuracy);
    LE_INFO("Check le_pos_sample_Get2DLocation passed, lat.%d, long.%d, accuracy.%d", val, val1, accuracy);

    le_pos_sample_GetDate(positionSampleRef, &year, &month, &day);
    LE_INFO("Check le_pos_sample_GetDate passed, year.%d, month.%d, day.%d", year, month, day);

    le_pos_sample_GetTime(positionSampleRef, &hours, &minutes, &seconds, &milliseconds);
    LE_INFO("Check le_pos_sample_GetTime passed, hours.%d, minutes.%d, seconds.%d, milliseconds.%d", hours, minutes, seconds, milliseconds);

    le_pos_sample_GetAltitude(positionSampleRef, &val, &accuracy);
    LE_INFO("Check le_pos_sample_GetAltitude passed, alt.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetHorizontalSpeed(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("Check le_pos_sample_GetHorizontalSpeed passed, hSpeed.%d, accuracy.%d", uval, uAccuracy);

    le_pos_sample_GetVerticalSpeed(positionSampleRef, &val, &accuracy);
    LE_INFO("Check le_pos_sample_GetVerticalSpeed passed, vSpeed.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetHeading(positionSampleRef, &val, &accuracy);
    LE_INFO("Check le_pos_sample_GetHeading passed, heading.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetDirection(positionSampleRef, &val, &accuracy);
    LE_INFO("Check le_pos_sample_GetDirection passed, direction.%d, accuracy.%d", val, accuracy);

    le_pos_sample_Release(positionSampleRef);
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

    le_pos_sample_Get2DLocation(positionSampleRef, &val, &val1, &accuracy);
    LE_INFO("Check le_pos_sample_Get2DLocation passed, lat.%d, long.%d, accuracy.%d", val, val1, accuracy);

    le_pos_sample_GetAltitude(positionSampleRef, &val, &accuracy);
    LE_INFO("Check le_pos_sample_GetAltitude passed, alt.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetHorizontalSpeed(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("Check le_pos_sample_GetHorizontalSpeed passed, hSpeed.%d, accuracy.%d", uval, uAccuracy);

    le_pos_sample_GetVerticalSpeed(positionSampleRef, &val, &accuracy);
    LE_INFO("Check le_pos_sample_GetVerticalSpeed passed, vSpeed.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetHeading(positionSampleRef, &val, &accuracy);
    LE_INFO("Check le_pos_sample_GetHeading passed, heading.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetDirection(positionSampleRef, &val, &accuracy);
    LE_INFO("Check le_pos_sample_GetDirection passed, direction.%d, accuracy.%d", val, accuracy);

    le_pos_sample_Release(positionSampleRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Add Position Handler
 *
*/
//--------------------------------------------------------------------------------------------------
static void* NavigationThread
(
    void* context
)
{
    le_pos_MovementHandlerRef_t     navigationHandlerRef;


    le_pos_ConnectService();

    LE_INFO("======== Navigation Handler thread  ========");
    navigationHandlerRef = le_pos_AddMovementHandler(0, 0, NavigationHandler, NULL);
    LE_ASSERT(navigationHandlerRef != NULL);

    navigationHandlerRef = le_pos_AddMovementHandler(20, 0, TwentyMeterNavigationHandler, NULL);
    LE_ASSERT(navigationHandlerRef != NULL);

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Get position Fix info.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_pos_GetInfo()
{
    int32_t     latitude;
    int32_t     longitude;
    int32_t     altitude;
    int32_t     hAccuracy;
    int32_t     vAccuracy;
    uint32_t    hSpeed;
    uint32_t    hSpeedAccuracy;
    int32_t     vSpeed;
    int32_t     vSpeedAccuracy;
    int32_t     heading;
    int32_t     headingAccuracy=0;
    int32_t     direction;
    int32_t     directionAccuracy=0;
    uint16_t    year = 0;
    uint16_t    month = 0;
    uint16_t    day = 0;
    uint16_t    hours = 0;
    uint16_t    minutes = 0;
    uint16_t    seconds = 0;
    uint16_t    milliseconds = 0;

    le_result_t res;

    res= le_pos_Get2DLocation(&latitude, &longitude, &hAccuracy);
    LE_INFO("le_pos_Get2DLocation %s"
            , (res==LE_OK)?"OK":(res==LE_OUT_OF_RANGE)?"parameter(s) out of range":"ERROR");
    LE_ASSERT((res ==LE_OK)||(res == LE_OUT_OF_RANGE));
    LE_INFO("Check le_pos_Get2DLocation latitude.%d, longitude.%d, hAccuracy.%d"
            , latitude, longitude, hAccuracy);

    res= le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);
    LE_INFO("le_pos_Get3DLocation %s"
            , (res==LE_OK)?"OK":(res==LE_OUT_OF_RANGE)?"parameter(s) out of range":"ERROR");
    LE_ASSERT((res ==LE_OK)||(res == LE_OUT_OF_RANGE));
    LE_INFO("Check le_pos_Get3DLocation latitude.%d, longitude.%d, hAccuracy.%d, altitude.%d"
            ", vAccuracy.%d"
            , latitude, longitude, hAccuracy, altitude, vAccuracy);

    res= le_pos_GetDate(&year, &month, &day);
    LE_INFO("le_pos_GetDate %s"
            , (res==LE_OK)?"OK":(res==LE_OUT_OF_RANGE)?"parameter(s) out of range":"ERROR");
    LE_ASSERT((res ==LE_OK)||(res == LE_OUT_OF_RANGE));
    LE_INFO("Check le_pos_GetDate year.%d, month.%d, day.%d"
            , year, month, day);

    res= le_pos_GetTime(&hours, &minutes, &seconds, &milliseconds);
    LE_INFO("le_pos_GetTime %s"
            , (res==LE_OK)?"OK":(res==LE_OUT_OF_RANGE)?"parameter(s) out of range":"ERROR");
    LE_ASSERT((res ==LE_OK)||(res == LE_OUT_OF_RANGE));
    LE_INFO("Check le_pos_GetTime hours.%d, minutes.%d, seconds.%d, milliseconds.%d"
            , hours, minutes, seconds, milliseconds);

    res= le_pos_GetMotion(&hSpeed, &hSpeedAccuracy, &vSpeed, &vSpeedAccuracy);
    LE_INFO("le_pos_GetMotion %s"
            , (res==LE_OK)?"OK":(res==LE_OUT_OF_RANGE)?"parameter(s) out of range":"ERROR");
    LE_ASSERT((res ==LE_OK)||(res == LE_OUT_OF_RANGE));
    LE_INFO("Check le_pos_GetMotion hSpeed.%d, hSpeedAccuracy.%d, vSpeed.%d, vSpeedAccuracy.%d"
            , hSpeed, hSpeedAccuracy, vSpeed, vSpeedAccuracy);

    res= le_pos_GetHeading(&heading, &headingAccuracy);
    LE_INFO("le_pos_GetHeading %s"
            , (res==LE_OK)?"OK":(res==LE_OUT_OF_RANGE)?"parameter(s) out of range":"ERROR");
    LE_ASSERT((res ==LE_OK)||(res == LE_OUT_OF_RANGE));
    LE_INFO("Check le_pos_GetHeading heading.%d, headingAccuracy.%d"
            , heading, headingAccuracy);

    res= le_pos_GetDirection(&direction, &directionAccuracy);
    LE_INFO("le_pos_GetDirection %s"
            , (res==LE_OK)?"OK":(res==LE_OUT_OF_RANGE)?"parameter(s) out of range":"ERROR");
    LE_ASSERT((res ==LE_OK)||(res == LE_OUT_OF_RANGE));
    LE_INFO("Check le_pos_GetDirection direction.%d, directionAccuracy.%d"
            , direction, directionAccuracy);

}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Get position Fix info.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_pos_TestAcquisitionRate()
{
    LE_ASSERT(le_pos_SetAcquisitionRate(3000) == LE_OK);
    LE_ASSERT(le_pos_GetAcquisitionRate() == 3000);

    LE_ASSERT(le_pos_SetAcquisitionRate(0) == LE_OUT_OF_RANGE);

    LE_ASSERT(le_pos_SetAcquisitionRate(5000) == LE_OK);
    LE_ASSERT(le_pos_GetAcquisitionRate() == 5000);
}

//--------------------------------------------------------------------------------------------------
/**
 * App init.
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_posCtrl_ActivationRef_t      activationRef;
    le_thread_Ref_t                 navigationThreadRef;


    LE_INFO("======== Positioning Test started  ========");

    Testle_pos_TestAcquisitionRate();
    // Add Position Handler Test
    navigationThreadRef = le_thread_Create("NavigationThread",NavigationThread,NULL);
    le_thread_Start(navigationThreadRef);

    LE_INFO("Get initial position");
    Testle_pos_GetInfo();
    LE_INFO("Request activation of the positioning service");
    LE_ASSERT((activationRef = le_posCtrl_Request()) != NULL);
    LE_INFO("Wait 120 seconds for a 3D fix");
    sleep(120);
    Testle_pos_GetInfo();
    sleep(1);
    LE_INFO("Release the positioning service");
    le_posCtrl_Release(activationRef);
    LE_INFO("======== Positioning Test finished ========");

    // Stop Navigation thread
    le_thread_Cancel(navigationThreadRef);

    exit(EXIT_SUCCESS);
}
