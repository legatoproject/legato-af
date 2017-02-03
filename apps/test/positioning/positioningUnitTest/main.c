/**
 * This module implements the unit tests for positioning API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Test 2D location data acquisition
 *
 * Verify that le_pos_Get2DLocation() behaves as expected in failure and success
 *
 */
//--------------------------------------------------------------------------------------------------
static void Test_le_pos_Get2DLocation
(
    void
)
{
    le_result_t result;
    int32_t latitude, longitude, accuracy;
    gnssSimuLocation_t gnssLocation;

    // test for NULL pointers
    result = le_pos_Get2DLocation(NULL, NULL, NULL);
    LE_ASSERT(result == LE_FAULT);

    // test for gnss failure
    gnssLocation.result = LE_FAULT;
    le_gnssSimu_SetLocation(gnssLocation);
    result = le_pos_Get2DLocation(&latitude, &longitude, &accuracy);
    LE_ASSERT(result == LE_FAULT);

    // test for gnss out of range
    gnssLocation.latitude = INT32_MAX;
    gnssLocation.longitude = INT32_MAX;
    gnssLocation.accuracy = INT32_MAX;
    gnssLocation.result = LE_OUT_OF_RANGE;

    le_gnssSimu_SetLocation(gnssLocation);

    result = le_pos_Get2DLocation(&latitude, &longitude, &accuracy);
    LE_ASSERT(
        (latitude == INT32_MAX) &&
        (longitude == INT32_MAX) &&
        (accuracy == INT32_MAX) &&
        (result == LE_OUT_OF_RANGE)
    );

    // test for normal beahaviour
    /*
     * location address:
     *      1 Avenue du Bas Meudon
     *      92130 Issy-les-Moulineaux
     *      France
     *
     * WGS84 coordinates:
     *      latitude = 48.82309144610534
     *      longitude = 2.24932461977005
     *
     */
    gnssLocation.latitude = 48823091;
    gnssLocation.longitude = 2249324;
    gnssLocation.accuracy = 10;
    gnssLocation.result = LE_OK;

    le_gnssSimu_SetLocation(gnssLocation);

    result = le_pos_Get2DLocation(&latitude, &longitude, &accuracy);
    LE_ASSERT(
        (latitude == gnssLocation.latitude) &&
        (longitude == gnssLocation.longitude) &&
        (accuracy == gnssLocation.accuracy / 100) &&
        (result == LE_OK)
    );
}

//--------------------------------------------------------------------------------------------------
/**
 * Tested API: le_pos_Get3DLocation()
 *
 * Verify that le_pos_Get3DLocation() behaves as expected in failure and success
 *
 */
//--------------------------------------------------------------------------------------------------
static void Test_le_pos_Get3DLocation
(
    void
)
{
    le_result_t result;
    int32_t latitude, longitude, hAccuracy, altitude, vAccuracy;
    gnssSimuLocation_t gnssLocation;
    gnssSimuAltitude_t gnssAltitude;

    // test for NULL pointers
    result = le_pos_Get3DLocation(NULL, NULL, NULL, NULL, NULL);
    LE_ASSERT(result == LE_FAULT);

    // test for gnss failure
    gnssLocation.result = LE_FAULT;
    le_gnssSimu_SetLocation(gnssLocation);
    gnssAltitude.result = LE_FAULT;
    le_gnssSimu_SetAltitude(gnssAltitude);
    result = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy,
                &altitude, &vAccuracy);
    LE_ASSERT(result == LE_FAULT);

    // test for gnss out of range
    gnssLocation.latitude = INT32_MAX;
    gnssLocation.longitude = INT32_MAX;
    gnssLocation.accuracy = INT32_MAX;
    gnssLocation.result = LE_OUT_OF_RANGE;

    le_gnssSimu_SetLocation(gnssLocation);

    gnssAltitude.altitude = INT32_MAX;
    gnssAltitude.accuracy = INT32_MAX;
    gnssAltitude.result = LE_OUT_OF_RANGE;

    le_gnssSimu_SetAltitude(gnssAltitude);


    result = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy,
                &altitude, &vAccuracy);
    LE_ASSERT(
        (latitude == INT32_MAX) &&
        (longitude == INT32_MAX) &&
        (hAccuracy == INT32_MAX) &&
        (altitude == INT32_MAX) &&
        (vAccuracy == INT32_MAX) &&
        (result == LE_OUT_OF_RANGE)
    );

    // test for normal beahaviour
    /*
     * location address:
     *      1 Avenue du Bas Meudon
     *      92130 Issy-les-Moulineaux
     *      France
     *
     * WGS84 coordinates:
     *      latitude = 48.82309144610534
     *      longitude = 2.24932461977005
     *
     */
    gnssLocation.latitude = 48823091;
    gnssLocation.longitude = 2249324;
    gnssLocation.accuracy = 10;
    gnssLocation.result = LE_OK;

    le_gnssSimu_SetLocation(gnssLocation);

    /*
     * altitude address:
     *      1 Avenue du Bas Meudon
     *      92130 Issy-les-Moulineaux
     *      France
     *
     * altitude:
     *      32m
     *
     */
    gnssAltitude.altitude = 32000;
    gnssAltitude.accuracy = 10;
    gnssAltitude.result = LE_OK;

    le_gnssSimu_SetAltitude(gnssAltitude);

    result = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy,
                &altitude, &vAccuracy);
    LE_ASSERT(
        (latitude == gnssLocation.latitude) &&
        (longitude == gnssLocation.longitude) &&
        (hAccuracy == gnssLocation.accuracy / 100) &&
        (altitude == gnssAltitude.altitude / 1000) &&
        (vAccuracy == gnssAltitude.accuracy / 10) &&
        (result == LE_OK)
    );
}

//--------------------------------------------------------------------------------------------------
/**
 * Tested API: le_pos_GetDate()
 *
 * Verify that le_pos_GetDate() behaves as expected in failure and success
 *
 */
//--------------------------------------------------------------------------------------------------
static void Test_le_pos_GetDate
(
    void
)
{
    le_result_t result;
    uint16_t year, month, day;
    gnssSimuDate_t gnssDate;

    // test for NULL pointers
    result = le_pos_GetDate(NULL, NULL, NULL);
    LE_ASSERT(result == LE_FAULT);

    // test for gnss failure
    gnssDate.result = LE_FAULT;
    le_gnssSimu_SetDate(gnssDate);
    result = le_pos_GetDate(&year, &month, &day);
    LE_ASSERT(result == LE_FAULT);

    // test for gnss out of range
    gnssDate.year = 0;
    gnssDate.month = 0;
    gnssDate.day = 0;
    gnssDate.result = LE_OUT_OF_RANGE;

    le_gnssSimu_SetDate(gnssDate);

    result = le_pos_GetDate(&year, &month, &day);
    LE_ASSERT(
        (year == 0) &&
        (month == 0) &&
        (day == 0) &&
        (result == LE_OUT_OF_RANGE)
    );

    // test for normal beahaviour
    gnssDate.year = 2016;
    gnssDate.month = 12;
    gnssDate.day = 12;
    gnssDate.result = LE_OK;

    le_gnssSimu_SetDate(gnssDate);

    result = le_pos_GetDate(&year, &month, &day);
    LE_ASSERT(
        (year == gnssDate.year) &&
        (month == gnssDate.month) &&
        (day == gnssDate.day) &&
        (result == gnssDate.result)
    );
}

//--------------------------------------------------------------------------------------------------
/**
 * Tested API: le_pos_GetDirection()
 *
 * Verify that le_pos_GetDirection() behaves as expected in failure and success
 *
 */
//--------------------------------------------------------------------------------------------------
static void Test_le_pos_GetDirection
(
    void
)
{
    le_result_t result;
    uint32_t direction, accuracy;
    gnssSimuDirection_t gnssDirection;

    // test for NULL pointers
    result = le_pos_GetDirection(NULL, NULL);
    LE_ASSERT(result == LE_FAULT);

    // test for gnss failure
    gnssDirection.result = LE_FAULT;
    le_gnssSimu_SetDirection(gnssDirection);
    result = le_pos_GetDirection(&direction, &accuracy);
    LE_ASSERT(result == LE_FAULT);

    // test for gnss out of range
    gnssDirection.direction = UINT32_MAX;
    gnssDirection.accuracy = UINT32_MAX;
    gnssDirection.result = LE_OUT_OF_RANGE;

    le_gnssSimu_SetDirection(gnssDirection);

    result = le_pos_GetDirection(&direction, &accuracy);
    LE_ASSERT(
        (direction == UINT32_MAX) &&
        (accuracy == UINT32_MAX) &&
        (result == LE_OUT_OF_RANGE)
    );

    // test for normal beahaviour
    gnssDirection.direction = 100;
    gnssDirection.accuracy = 10;
    gnssDirection.result = LE_OK;

    le_gnssSimu_SetDirection(gnssDirection);

    result = le_pos_GetDirection(&direction, &accuracy);
    LE_ASSERT(
        (direction == gnssDirection.direction/10) &&
        (accuracy == gnssDirection.accuracy / 10) &&
        (result == LE_OK)
    );
}

//--------------------------------------------------------------------------------------------------
/**
 * le_pos_Getheading() is not supported, test that is does return an error code
 * and UINT32_MAX as heading and accuracy values
 *
 */
//--------------------------------------------------------------------------------------------------
static void Test_le_pos_GetHeading
(
    void
)
{
    le_result_t result;
    uint32_t heading, accuracy;

    result = le_pos_GetHeading(&heading, &accuracy);
    LE_ASSERT(
        (heading == UINT32_MAX) &&
        (accuracy == UINT32_MAX) &&
        (result == LE_OUT_OF_RANGE)
    );
}

//--------------------------------------------------------------------------------------------------
/**
 * Tested API: le_pos_GetMotion()
 *
 * Verify that le_pos_GetMotion() behaves as expected in failure and success
 *
 */
//--------------------------------------------------------------------------------------------------
static void Test_le_pos_GetMotion
(
    void
)
{
    le_result_t result;
    uint32_t hSpeed, hAccuracy;
    int32_t vSpeed, vAccuracy;
    gnssSimuHSpeed_t gnssHSpeed;
    gnssSimuVSpeed_t gnssVSpeed;

    // test for NULL pointers
    result = le_pos_GetMotion(NULL, NULL, NULL, NULL);
    LE_ASSERT(result == LE_FAULT);

    // test for gnss failure
    gnssHSpeed.result = LE_FAULT;
    gnssVSpeed.result = LE_FAULT;
    le_gnssSimu_SetHSpeed(gnssHSpeed);
    le_gnssSimu_SetVSpeed(gnssVSpeed);
    result = le_pos_GetMotion(&hSpeed, &hAccuracy, &vSpeed, &vAccuracy);
    LE_ASSERT(result == LE_FAULT);

    // test for gnss out of range
    gnssHSpeed.speed = UINT32_MAX;
    gnssHSpeed.accuracy = UINT32_MAX;
    gnssHSpeed.result = LE_OUT_OF_RANGE;

    le_gnssSimu_SetHSpeed(gnssHSpeed);

    gnssVSpeed.speed = INT32_MAX;
    gnssVSpeed.accuracy = INT32_MAX;
    gnssVSpeed.result = LE_OUT_OF_RANGE;

    le_gnssSimu_SetVSpeed(gnssVSpeed);

    result = le_pos_GetMotion(&hSpeed, &hAccuracy, &vSpeed, &vAccuracy);
    LE_ASSERT(
        (hSpeed == UINT32_MAX) &&
        (hAccuracy == UINT32_MAX) &&
        (vSpeed == INT32_MAX) &&
        (vAccuracy == INT32_MAX) &&
        (result == LE_OUT_OF_RANGE)
    );

    // test for normal beahaviour
    gnssHSpeed.speed = 3600;
    gnssHSpeed.accuracy = 10;
    gnssHSpeed.result = LE_OK;

    le_gnssSimu_SetHSpeed(gnssHSpeed);

    gnssVSpeed.speed = 300;
    gnssVSpeed.accuracy = 10;
    gnssVSpeed.result = LE_OK;

    le_gnssSimu_SetVSpeed(gnssVSpeed);

    result = le_pos_GetMotion(&hSpeed, &hAccuracy, &vSpeed, &vAccuracy);
    LE_ASSERT(
        (hSpeed == gnssHSpeed.speed/100) &&
        (hAccuracy == gnssHSpeed.accuracy / 10) &&
        (vSpeed == gnssVSpeed.speed/100) &&
        (vAccuracy == gnssVSpeed.accuracy / 10) &&
        (result == LE_OK)
    );
}

//--------------------------------------------------------------------------------------------------
/**
 * Tested API: le_pos_GetTime()
 *
 * Verify that le_pos_GetTime() behaves as expected in failure and success
 *
 */
//--------------------------------------------------------------------------------------------------
static void Test_le_pos_GetTime
(
    void
)
{
    le_result_t result;
    uint16_t hrs, min, sec, msec;
    gnssSimuTime_t gnssTime;

    // test for NULL pointers
    result = le_pos_GetTime(NULL, NULL, NULL, NULL);
    LE_ASSERT(result == LE_FAULT);

    // test for gnss failure
    gnssTime.result = LE_FAULT;
    le_gnssSimu_SetTime(gnssTime);
    result = le_pos_GetTime(&hrs, &min, &sec, &msec);
    LE_ASSERT(result == LE_FAULT);

    // test for gnss out of range
    gnssTime.hrs = 0;
    gnssTime.min = 0;
    gnssTime.sec = 0;
    gnssTime.msec = 0;
    gnssTime.result = LE_OUT_OF_RANGE;

    le_gnssSimu_SetTime(gnssTime);

    result = le_pos_GetTime(&hrs, &min, &sec, &msec);
    LE_ASSERT(
        (hrs == 0) &&
        (min == 0) &&
        (sec == 0) &&
        (msec == 0) &&
        (result == LE_OUT_OF_RANGE)
    );

    // test for normal beahaviour
    gnssTime.hrs = 120;
    gnssTime.min = 15;
    gnssTime.sec = 54;
    gnssTime.msec = 1245;
    gnssTime.result = LE_OK;

    le_gnssSimu_SetTime(gnssTime);

    result = le_pos_GetTime(&hrs, &min, &sec, &msec);
    LE_ASSERT(
        (hrs == gnssTime.hrs) &&
        (min == gnssTime.min) &&
        (sec == gnssTime.sec) &&
        (msec == gnssTime.msec) &&
        (result == gnssTime.result)
    );
}

//--------------------------------------------------------------------------------------------------
/**
 * Tested API: le_pos_GetFixState()
 *
 * Verify that le_pos_GetFixState() behaves as expected in failure and success
 *
 */
//--------------------------------------------------------------------------------------------------
static void Test_le_pos_GetFixState
(
    void
)
{
    le_result_t result;
    le_pos_FixState_t state;
    le_gnss_SampleRef_t sample;
    gnssSimuPositionState_t gnssPositionState;
    int var = 0;

    // test for NULL pointers
    result = le_pos_GetFixState(NULL);
    LE_ASSERT(result == LE_FAULT);

    // test for gnss failure
    le_gnssSimu_SetSampleRef(NULL);
    result = le_pos_GetFixState(&state);
    LE_ASSERT(result == LE_FAULT);

    // test for normal beahaviour
    sample = (le_gnss_SampleRef_t) &var;
    le_gnssSimu_SetSampleRef(sample);

    gnssPositionState.state = LE_GNSS_STATE_FIX_NO_POS;
    gnssPositionState.result = LE_OK;

    le_gnssSimu_SetPositionState(gnssPositionState);

    result = le_pos_GetFixState(&state);
    LE_ASSERT((state == LE_POS_STATE_NO_FIX) && (result == LE_OK));

    sample = (le_gnss_SampleRef_t) &var;
    le_gnssSimu_SetSampleRef(sample);

    gnssPositionState.state = LE_GNSS_STATE_FIX_2D;
    gnssPositionState.result = LE_OK;

    le_gnssSimu_SetPositionState(gnssPositionState);

    result = le_pos_GetFixState(&state);
    LE_ASSERT((state == LE_POS_STATE_FIX_2D) && (result == LE_OK));

    sample = (le_gnss_SampleRef_t) &var;
    le_gnssSimu_SetSampleRef(sample);

    gnssPositionState.state = LE_GNSS_STATE_FIX_3D;
    gnssPositionState.result = LE_OK;

    le_gnssSimu_SetPositionState(gnssPositionState);

    result = le_pos_GetFixState(&state);
    LE_ASSERT((state == LE_POS_STATE_FIX_3D) && (result == LE_OK));

    sample = (le_gnss_SampleRef_t) &var;
    le_gnssSimu_SetSampleRef(sample);

    gnssPositionState.state = LE_GNSS_STATE_FIX_ESTIMATED;
    gnssPositionState.result = LE_OK;

    le_gnssSimu_SetPositionState(gnssPositionState);

    result = le_pos_GetFixState(&state);
    LE_ASSERT((state == LE_POS_STATE_FIX_ESTIMATED) && (result == LE_OK));
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // To reactivate for all DEBUG logs
#ifdef DEBUG
    LE_INFO("DEBUG MODE");
    le_log_SetFilterLevel(LE_LOG_DEBUG);
#endif

    // tests
    Test_le_pos_Get2DLocation();
    Test_le_pos_Get3DLocation();
    Test_le_pos_GetDirection();
    Test_le_pos_GetHeading();
    Test_le_pos_GetMotion();
    Test_le_pos_GetDate();
    Test_le_pos_GetTime();
    Test_le_pos_GetFixState();

    exit(0);
}
