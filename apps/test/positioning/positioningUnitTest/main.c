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
 * Navigation Handler Reference
 */
//--------------------------------------------------------------------------------------------------
static le_pos_MovementHandlerRef_t  NavigationHandlerRef;
static le_pos_MovementHandlerRef_t  FiftyNavigationHandlerRef;
//--------------------------------------------------------------------------------------------------
/**
 * Server Service Reference
 */
//--------------------------------------------------------------------------------------------------
static le_msg_ServiceRef_t _ServerServiceRef;

//--------------------------------------------------------------------------------------------------
/**
 * Client Session Reference for the current message received from a client
 */
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t _ClientSessionRef;

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_pos_GetServiceRef
(
    void
)
{
    return _ServerServiceRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_pos_GetClientSessionRef
(
    void
)
{
    return _ClientSessionRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Registers a function to be called whenever one of this service's sessions is closed by
 * the client.  (STUBBED FUNCTION)

 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionEventHandlerRef_t MyAddServiceCloseHandler
(
    le_msg_ServiceRef_t             serviceRef, ///< [in] Reference to the service.
    le_msg_SessionEventHandler_t    handlerFunc,///< [in] Handler function.
    void*                           contextPtr  ///< [in] Opaque pointer value to pass to handler.
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_gnss_GetServiceRef
(
    void
)
{
    return _ServerServiceRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_gnss_GetClientSessionRef
(
    void
)
{
    return _ClientSessionRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Activation reference.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_posCtrl_ActivationRef_t   ActivationRef;

//--------------------------------------------------------------------------------------------------
/**
 * Thread and semaphore reference.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_sem_Ref_t                 ThreadSemaphore;
static le_sem_Ref_t                 InitSemaphore;
static le_clk_Time_t                TimeToWait = { 5, 0 };
static le_thread_Ref_t              NavigationThreadRef;

//--------------------------------------------------------------------------------------------------
/**
 * Mutex used to protect access to le_gnss functions when used in different threads.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.
/// Locks the mutex.
#define LOCK    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0);
/// Unlocks the mutex.
#define UNLOCK  LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);

//--------------------------------------------------------------------------------------------------
/**
 * Synchronize test thread (i.e. main) and tasks
 *
 */
//--------------------------------------------------------------------------------------------------
static void SynchTest
(
    void
)
{
   LE_ASSERT(LE_OK == le_sem_WaitWithTimeOut(ThreadSemaphore, TimeToWait));
}

//--------------------------------------------------------------------------------------------------
/**
 * Test 2D location data acquisition
 *
 * Verify that le_pos_Get2DLocation() behaves as expected in failure and success
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_pos_Get2DLocation
(
    void
)
{
    le_result_t result;
    int32_t latitude, longitude, accuracy;
    gnssSimuLocation_t gnssLocation;

    // test for NULL pointers
    result = le_pos_Get2DLocation(NULL, NULL, NULL);
    LE_ASSERT(LE_FAULT == result);

    // test for gnss failure
    gnssLocation.result = LE_FAULT;
    le_gnssSimu_SetLocation(gnssLocation);
    result = le_pos_Get2DLocation(&latitude, &longitude, &accuracy);
    LE_ASSERT(LE_FAULT == result);

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
        (LE_OUT_OF_RANGE == result)
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
        (LE_OK == result)
    );
}

//--------------------------------------------------------------------------------------------------
/**
 * Tested API: le_pos_SetDistanceResolution()
 *
 * Verify that le_pos_Get3DLocation() gives distance values in the correct resolution
 */
//--------------------------------------------------------------------------------------------------
static void Testle_pos_GetDistanceResolution
(
    void
)
{
    le_result_t result;
    int32_t latitude, longitude, hAccuracy, altitude, vAccuracy;
    gnssSimuLocation_t gnssLocation;
    gnssSimuAltitude_t gnssAltitude;

    gnssLocation.latitude = 48823091;
    gnssLocation.longitude = 2249324;
    gnssLocation.accuracy = 200;   // horizontal accuracy in centimeters
    gnssLocation.result = LE_OK;

    le_gnssSimu_SetLocation(gnssLocation);

    gnssAltitude.altitude = -32000;  // altitude in millimeters (-32 m)
    gnssAltitude.accuracy = 10;      // vertical accuracy in decimeters
    gnssAltitude.result = LE_OK;

    le_gnssSimu_SetAltitude(gnssAltitude);

    LE_ASSERT(LE_BAD_PARAMETER == le_pos_SetDistanceResolution(LE_POS_RES_UNKNOWN));

    LE_ASSERT_OK(le_pos_SetDistanceResolution(LE_POS_RES_DECIMETER));

    result = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);

    LE_INFO("hAccuracy %d, altitude %d, vAccuracy %d, result %d",
            hAccuracy, altitude, vAccuracy, result);
    LE_ASSERT(
        (48823091 == latitude) &&
        (2249324 == longitude) &&
        (20 == hAccuracy) &&
        (-320 == altitude) &&
        (10 == vAccuracy) &&
        (LE_OK == result)
        );

    LE_ASSERT_OK(le_pos_SetDistanceResolution(LE_POS_RES_CENTIMETER));

    result = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy,
                &altitude, &vAccuracy);

    LE_INFO("hAccuracy %d, altitude %d, vAccuracy %d, result %d",
            hAccuracy, altitude, vAccuracy, result);
    LE_ASSERT(
        (48823091 == latitude) &&
        (2249324 == longitude) &&
        (200 == hAccuracy) &&
        (-3200 == altitude) &&
        (100 == vAccuracy) &&
        (LE_OK == result)
        );

    LE_ASSERT_OK(le_pos_SetDistanceResolution(LE_POS_RES_MILLIMETER));

    result = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy,
                &altitude, &vAccuracy);

    LE_INFO("hAccuracy %d, altitude %d, vAccuracy %d, result %d",
            hAccuracy, altitude, vAccuracy, result);
    LE_ASSERT(
        (48823091 == latitude) &&
        (2249324 == longitude) &&
        (2000 == hAccuracy) &&
        (-32000 == altitude) &&
        (1000 == vAccuracy) &&
        (LE_OK == result)
        );

    LE_ASSERT_OK(le_pos_SetDistanceResolution(LE_POS_RES_METER));

    result = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy,
                &altitude, &vAccuracy);

    LE_INFO("hAccuracy %d, altitude %d, vAccuracy %d, result %d",
            hAccuracy, altitude, vAccuracy, result);
    LE_ASSERT(
        (48823091 == latitude) &&
        (2249324 == longitude) &&
        (2 == hAccuracy) &&
        (-32 == altitude) &&
        (1 == vAccuracy) &&
        (LE_OK == result)
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
static void Testle_pos_Get3DLocation
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
    LE_ASSERT(LE_FAULT == result);

    // test for gnss failure
    gnssLocation.result = LE_FAULT;
    le_gnssSimu_SetLocation(gnssLocation);
    gnssAltitude.result = LE_FAULT;
    le_gnssSimu_SetAltitude(gnssAltitude);
    result = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy,
                &altitude, &vAccuracy);
    LE_ASSERT(LE_FAULT == result);

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
        (LE_OUT_OF_RANGE == result)
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
        (LE_OK == result)
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
static void Testle_pos_GetDate
(
    void
)
{
    le_result_t result;
    uint16_t year, month, day;
    gnssSimuDate_t gnssDate;

    // test for NULL pointers
    result = le_pos_GetDate(NULL, NULL, NULL);
    LE_ASSERT(LE_FAULT == result);

    // test for gnss failure
    gnssDate.result = LE_FAULT;
    le_gnssSimu_SetDate(gnssDate);
    result = le_pos_GetDate(&year, &month, &day);
    LE_ASSERT(LE_FAULT == result);

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
        (LE_OUT_OF_RANGE == result)
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
static void Testle_pos_GetDirection
(
    void
)
{
    le_result_t result;
    uint32_t direction, accuracy;
    gnssSimuDirection_t gnssDirection;

    // test for NULL pointers
    result = le_pos_GetDirection(NULL, NULL);
    LE_ASSERT(LE_FAULT == result);

    // test for gnss failure
    gnssDirection.result = LE_FAULT;
    le_gnssSimu_SetDirection(gnssDirection);
    result = le_pos_GetDirection(&direction, &accuracy);
    LE_ASSERT(LE_FAULT == result);

    // test for gnss out of range
    gnssDirection.direction = UINT32_MAX;
    gnssDirection.accuracy = UINT32_MAX;
    gnssDirection.result = LE_OUT_OF_RANGE;

    le_gnssSimu_SetDirection(gnssDirection);

    result = le_pos_GetDirection(&direction, &accuracy);
    LE_ASSERT(
        (direction == UINT32_MAX) &&
        (accuracy == UINT32_MAX) &&
        (LE_OUT_OF_RANGE == result)
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
        (LE_OK == result)
    );
}

//--------------------------------------------------------------------------------------------------
/**
 * le_pos_Getheading() is not supported, test that is does return an error code
 * and UINT32_MAX as heading and accuracy values
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_pos_GetHeading
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
        (LE_OUT_OF_RANGE == result)
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
static void Testle_pos_GetMotion
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
    LE_ASSERT(LE_FAULT == result);

    // test for gnss failure
    gnssHSpeed.result = LE_FAULT;
    gnssVSpeed.result = LE_FAULT;
    le_gnssSimu_SetHSpeed(gnssHSpeed);
    le_gnssSimu_SetVSpeed(gnssVSpeed);
    result = le_pos_GetMotion(&hSpeed, &hAccuracy, &vSpeed, &vAccuracy);
    LE_ASSERT(LE_FAULT == result);

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
        (LE_OUT_OF_RANGE == result)
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
        (LE_OK == result)
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
static void Testle_pos_GetTime
(
    void
)
{
    le_result_t result;
    uint16_t hrs, min, sec, msec;
    gnssSimuTime_t gnssTime;

    // test for NULL pointers
    result = le_pos_GetTime(NULL, NULL, NULL, NULL);
    LE_ASSERT(LE_FAULT == result);

    // test for gnss failure
    gnssTime.result = LE_FAULT;
    le_gnssSimu_SetTime(gnssTime);
    result = le_pos_GetTime(&hrs, &min, &sec, &msec);
    LE_ASSERT(LE_FAULT == result);

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
        (LE_OUT_OF_RANGE == result)
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
static void Testle_pos_GetFixState
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
    LE_ASSERT(LE_FAULT == result);

    // test for gnss failure
    le_gnssSimu_SetSampleRef(NULL);
    result = le_pos_GetFixState(&state);
    LE_ASSERT(LE_FAULT == result);

    // test for normal beahaviour
    sample = (le_gnss_SampleRef_t) &var;
    le_gnssSimu_SetSampleRef(sample);

    gnssPositionState.state = LE_GNSS_STATE_FIX_NO_POS;
    gnssPositionState.result = LE_OK;

    le_gnssSimu_SetPositionState(gnssPositionState);

    result = le_pos_GetFixState(&state);
    LE_ASSERT((LE_POS_STATE_NO_FIX == state) && (LE_OK == result));

    sample = (le_gnss_SampleRef_t) &var;
    le_gnssSimu_SetSampleRef(sample);

    gnssPositionState.state = LE_GNSS_STATE_FIX_2D;
    gnssPositionState.result = LE_OK;

    le_gnssSimu_SetPositionState(gnssPositionState);

    result = le_pos_GetFixState(&state);
    LE_ASSERT((LE_POS_STATE_FIX_2D == state) && (LE_OK == result));

    sample = (le_gnss_SampleRef_t) &var;
    le_gnssSimu_SetSampleRef(sample);

    gnssPositionState.state = LE_GNSS_STATE_FIX_3D;
    gnssPositionState.result = LE_OK;

    le_gnssSimu_SetPositionState(gnssPositionState);

    result = le_pos_GetFixState(&state);
    LE_ASSERT((LE_POS_STATE_FIX_3D == state) && (LE_OK == result));

    sample = (le_gnss_SampleRef_t) &var;
    le_gnssSimu_SetSampleRef(sample);

    gnssPositionState.state = LE_GNSS_STATE_FIX_ESTIMATED;
    gnssPositionState.result = LE_OK;

    le_gnssSimu_SetPositionState(gnssPositionState);

    result = le_pos_GetFixState(&state);
    LE_ASSERT((LE_POS_STATE_FIX_ESTIMATED == state) && (LE_OK == result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Tested API: le_pos_GetAcquisitionRate()
 *
 * Verify that le_pos_GetAcquisitionRate() behaves as expected in failure and success
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_pos_GetAcquisitionRate
(
    void
)
{
    le_result_t result;
    uint32_t  acquisitionRate =5000, acqRate;
    result = le_pos_SetAcquisitionRate(acquisitionRate);
    LE_ASSERT(LE_OK == result);

    acqRate = le_pos_GetAcquisitionRate();
    LE_ASSERT(acqRate == acquisitionRate);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Navigation notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FiftyNavigationHandler
(
    le_pos_SampleRef_t positionSampleRef,
    void* contextPtr
)
{
    int32_t latitude, longitude, horizontalAccuracy;
    uint16_t hrs, min, sec, msec;
    uint16_t year, month, day;
    uint32_t heading, headingAccuracy;
    uint32_t direction, directionAccuracy;
    int32_t altitude, altitudeAccuracy;
    uint32_t hSpeed, hSpeedAccuracy;
    int32_t vSpeed, vSpeedAccuracy;
    le_pos_FixState_t state;

    LE_ASSERT(NULL != positionSampleRef);

    Testle_pos_GetFixState();
    Testle_pos_Get2DLocation();
    Testle_pos_GetDate();
    Testle_pos_GetTime();
    Testle_pos_GetHeading();
    Testle_pos_GetDirection();

    // test for NULL pointers
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_Get2DLocation(NULL, NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetTime(NULL, NULL, NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetDate(NULL, NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetDirection(NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetAltitude(NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetHorizontalSpeed(NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetVerticalSpeed(NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetFixState(NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetHeading(NULL, NULL, NULL)));

    // test for Normal Behaviour
    LE_ASSERT_OK(le_pos_sample_Get2DLocation(positionSampleRef, &latitude,
                                             &longitude, &horizontalAccuracy));
    LE_ASSERT_OK(le_pos_sample_GetTime(positionSampleRef, &hrs, &min, &sec, &msec));
    LE_ASSERT_OK(le_pos_sample_GetDate(positionSampleRef, &year, &month, &day));
    LE_ASSERT_OK(le_pos_sample_GetDirection(positionSampleRef, &direction, &directionAccuracy));
    LE_ASSERT_OK(le_pos_sample_GetAltitude(positionSampleRef, &altitude, &altitudeAccuracy));
    LE_ASSERT_OK(le_pos_sample_GetHorizontalSpeed(positionSampleRef, &hSpeed, &hSpeedAccuracy));
    LE_ASSERT_OK(le_pos_sample_GetVerticalSpeed(positionSampleRef, &vSpeed, &vSpeedAccuracy));
    LE_ASSERT_OK(le_pos_sample_GetFixState(positionSampleRef, &state));
    LE_ASSERT(LE_OUT_OF_RANGE == (le_pos_sample_GetHeading(positionSampleRef, &heading,
                                                           &headingAccuracy)));
    le_pos_sample_Release(positionSampleRef);
    le_sem_Post(ThreadSemaphore);
}
//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Navigation notification.
 *
 */
//--------------------------------------------------------------------------------------------------
static void NavigationHandler
(
    le_pos_SampleRef_t positionSampleRef,
    void* contextPtr
)
{
    int32_t latitude, longitude, horizontalAccuracy;
    uint16_t hrs, min, sec, msec;
    uint16_t year, month, day;
    uint32_t heading, headingAccuracy;
    uint32_t direction, directionAccuracy;
    int32_t altitude, altitudeAccuracy;
    uint32_t hSpeed, hSpeedAccuracy;
    int32_t vSpeed, vSpeedAccuracy;
    le_pos_FixState_t state;

    LE_ASSERT(NULL != positionSampleRef);

    Testle_pos_GetFixState();
    Testle_pos_Get2DLocation();
    Testle_pos_GetDate();
    Testle_pos_GetTime();
    Testle_pos_GetHeading();
    Testle_pos_GetDirection();

    // test for NULL pointers
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_Get2DLocation(NULL, NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetTime(NULL, NULL, NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetDate(NULL, NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetDirection(NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetAltitude(NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetHorizontalSpeed(NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetVerticalSpeed(NULL, NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetFixState(NULL, NULL)));
    LE_ASSERT(LE_BAD_PARAMETER == (le_pos_sample_GetHeading(NULL, NULL, NULL)));

    // test for Normal Behaviour
    LE_ASSERT_OK(le_pos_sample_Get2DLocation(positionSampleRef, &latitude,
                                             &longitude, &horizontalAccuracy));
    LE_ASSERT_OK(le_pos_sample_GetTime(positionSampleRef, &hrs, &min, &sec, &msec));
    LE_ASSERT_OK(le_pos_sample_GetDate(positionSampleRef, &year, &month, &day));
    LE_ASSERT_OK(le_pos_sample_GetDirection(positionSampleRef, &direction, &directionAccuracy));
    LE_ASSERT_OK(le_pos_sample_GetAltitude(positionSampleRef, &altitude, &altitudeAccuracy));
    LE_ASSERT_OK(le_pos_sample_GetHorizontalSpeed(positionSampleRef, &hSpeed, &hSpeedAccuracy));
    LE_ASSERT_OK(le_pos_sample_GetVerticalSpeed(positionSampleRef, &vSpeed, &vSpeedAccuracy));
    LE_ASSERT_OK(le_pos_sample_GetFixState(positionSampleRef, &state));
    LE_ASSERT(LE_OUT_OF_RANGE == (le_pos_sample_GetHeading(positionSampleRef, &heading,
                                                           &headingAccuracy)));
    le_pos_sample_Release(positionSampleRef);
    le_sem_Post(ThreadSemaphore);
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
    // test for NULL
    NavigationHandlerRef = le_pos_AddMovementHandler(0, 0, NULL, NULL);
    LE_ASSERT(NavigationHandlerRef == NULL);

    // test for Normal Behaviour
    LOCK
    // Test the registration of an handler for movement notifications.
    // The movement notification range can be set to an horizontal and a vertical magnitude of 50
    // meters each.
    FiftyNavigationHandlerRef = le_pos_AddMovementHandler(50, 50, FiftyNavigationHandler, NULL);
    LE_ASSERT(FiftyNavigationHandlerRef != NULL);

    // le_pos_AddMovementHandler calculates an acquisitionRate (look at le_pos_AddMovementHandler
    // and CalculateAcquisitionRate().
    // Test that the acquisitionRate is 4000 msec.
    LE_ASSERT(4000 == le_pos_GetAcquisitionRate());

    // Test the registration of an handler for movement notifications with horizontal or vertical
    // magnitude of 0 meters. (It will set an acquisition rate of 1sec).
    NavigationHandlerRef = le_pos_AddMovementHandler(0, 0, NavigationHandler, NULL);
    LE_ASSERT(NavigationHandlerRef != NULL);
    // Test that the acquisitionRate is 1000 msec
    // (because final acquisitionRate is the smallest calculated).
    LE_ASSERT(1000 == le_pos_GetAcquisitionRate());

    UNLOCK

    le_sem_Post(ThreadSemaphore);
    le_event_RunLoop();
}

//--------------------------------------------------------------------------------------------------
/**
 * Tested API: le_pos_AddMovementHandler()
 *
 * Verify that le_pos_AddMovementHandler() behaves as expected in failure and success
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_pos_AddMovementHandler
(
    void
)
{
    ThreadSemaphore = le_sem_Create("HandlerSem", 0);
    NavigationThreadRef = le_thread_Create("NavigationThread", NavigationThread, NULL);
    le_thread_Start(NavigationThreadRef);

    LE_INFO("Request activation of the positioning service");
    ActivationRef = le_posCtrl_Request();
    LE_ASSERT(NULL != ActivationRef);

    // Wait that the tasks have started before continuing the test
    SynchTest();

    // The tasks have subscribe to event event handler:
    le_gnssSimu_ReportEvent();

    // wait the handlers' calls
    SynchTest();
}

//--------------------------------------------------------------------------------------------
/**
 * Test: this function handles the remove position handler
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    // test for NULL
    le_pos_RemoveMovementHandler(NULL);

    // test for Remove Handler when HandleRef is initialized
    le_pos_RemoveMovementHandler(NavigationHandlerRef);

    NavigationHandlerRef = NULL;

    // test for Remove Handler when HandleRef is initialized
    le_pos_RemoveMovementHandler(FiftyNavigationHandlerRef);

    FiftyNavigationHandlerRef = NULL;

    // Semaphore is used to synchronize the task execution with the core test
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Tested API: le_pos_RemoveMovementHandler()
 *
 * Verify that le_pos_RemoveMovementHandler() behaves as expected in failure and success
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_pos_RemoveMovementHandler
(
    void
)
{
    le_event_QueueFunctionToThread(NavigationThreadRef, RemoveHandler, NULL, NULL);
    SynchTest();
    // Provoke event to make sure handler not called anymore
    le_gnssSimu_ReportEvent();
    // No semaphore post is waiting, we are expecting a timeout
    LE_ASSERT(LE_TIMEOUT == le_sem_WaitWithTimeOut(ThreadSemaphore, TimeToWait));

    le_thread_Cancel(NavigationThreadRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * UnitTestInit thread: this function initializes the test and runs an eventLoop
 *
 */
//--------------------------------------------------------------------------------------------------
static void* UnitTestInit
(
    void* contexPtr
)
{
    Testle_pos_AddMovementHandler();
    Testle_pos_RemoveMovementHandler();
    le_sem_Post(InitSemaphore);
    le_event_RunLoop();
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
#if LE_CONFIG_DEBUG
    LE_INFO("DEBUG MODE");
    le_log_SetFilterLevel(LE_LOG_DEBUG);
#endif

    // tests
    Testle_pos_Get2DLocation();
    Testle_pos_Get3DLocation();
    Testle_pos_GetDistanceResolution();
    Testle_pos_GetDirection();
    Testle_pos_GetHeading();
    Testle_pos_GetMotion();
    Testle_pos_GetDate();
    Testle_pos_GetTime();
    Testle_pos_GetFixState();
    Testle_pos_GetAcquisitionRate();
    // Create a semaphore to coordinate Initialization
    InitSemaphore = le_sem_Create("InitSem",0);
    le_thread_Start(le_thread_Create("UnitTestInit", UnitTestInit, NULL));
    le_sem_Wait(InitSemaphore);

    LE_INFO("Release the positioning service");
    le_posCtrl_Release(ActivationRef);
    exit(0);
}
