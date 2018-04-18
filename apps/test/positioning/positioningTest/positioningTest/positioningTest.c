 /**
  * This module implements the le_posCtrl's tests.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include <stdio.h>

#include "interfaces.h"

//--------------------------------------------------------------------------------------------------
/**
 * Movement Handler References
 */
//--------------------------------------------------------------------------------------------------
static le_pos_MovementHandlerRef_t  NavigationHandlerRef;
static le_pos_MovementHandlerRef_t  FiftyNavigationHandlerRef;


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
    le_pos_FixState_t fixState;

    if (NULL == positionSampleRef)
    {
        LE_ERROR("New Position sample is NULL!");
    }
    else
    {
        LE_INFO("New Position sample %p", positionSampleRef);
    }

    le_pos_sample_GetFixState(positionSampleRef, &fixState);
    LE_INFO("GetFixState: %d", fixState);

    le_pos_sample_Get2DLocation(positionSampleRef, &val, &val1, &accuracy);
    LE_INFO("Get2DLocation: lat.%d, long.%d, accuracy.%d", val, val1, accuracy);

    le_pos_sample_GetDate(positionSampleRef, &year, &month, &day);
    LE_INFO("GetDate: year.%d, month.%d, day.%d", year, month, day);

    le_pos_sample_GetTime(positionSampleRef, &hours, &minutes, &seconds, &milliseconds);
    LE_INFO("GetTime: hours.%d, minutes.%d, seconds.%d, milliseconds.%d", hours, minutes, seconds,
            milliseconds);

    le_pos_sample_GetAltitude(positionSampleRef, &val, &accuracy);
    LE_INFO("GetAltitude: alt.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetHorizontalSpeed(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("GetHorizontalSpeed: hSpeed.%u, accuracy.%u", uval, uAccuracy);

    le_pos_sample_GetVerticalSpeed(positionSampleRef, &val, &accuracy);
    LE_INFO("GetVerticalSpeed: vSpeed.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetHeading(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("GetHeading: heading.%u, accuracy.%u", uval, uAccuracy);

    le_pos_sample_GetDirection(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("GetDirection: direction.%u, accuracy.%u", uval, uAccuracy);

    le_pos_sample_Release(positionSampleRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for Profile State Change Notifications.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FiftyMeterNavigationHandler
(
    le_pos_SampleRef_t positionSampleRef,
    void* contextPtr
)
{
    int32_t  val, val1, accuracy;
    uint32_t uval, uAccuracy;
    le_pos_FixState_t fixState;

    if (NULL == positionSampleRef)
    {
        LE_ERROR("New Position sample is NULL!");
    }
    else
    {
        LE_INFO("New Position sample %p", positionSampleRef);
    }

    le_pos_sample_GetFixState(positionSampleRef, &fixState);
    LE_INFO("GetFixState: %d", fixState);

    le_pos_sample_Get2DLocation(positionSampleRef, &val, &val1, &accuracy);
    LE_INFO("Get2DLocation: lat.%d, long.%d, accuracy.%d", val, val1, accuracy);

    le_pos_sample_GetAltitude(positionSampleRef, &val, &accuracy);
    LE_INFO("GetAltitude: alt.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetHorizontalSpeed(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("GetHorizontalSpeed: hSpeed.%u, accuracy.%u", uval, uAccuracy);

    le_pos_sample_GetVerticalSpeed(positionSampleRef, &val, &accuracy);
    LE_INFO("GetVerticalSpeed: vSpeed.%d, accuracy.%d", val, accuracy);

    le_pos_sample_GetHeading(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("GetHeading: heading.%u, accuracy.%u", uval, uAccuracy);

    le_pos_sample_GetDirection(positionSampleRef, &uval, &uAccuracy);
    LE_INFO("GetDirection: direction.%u, accuracy.%u", uval, uAccuracy);

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
    le_pos_ConnectService();

    LE_INFO("======== Navigation Handler thread  ========");

    // Test the registration of an handler for movement notifications.
    // The movement notification range can be set to an horizontal and a vertical magnitude of 50
    // meters each.
    FiftyNavigationHandlerRef = le_pos_AddMovementHandler(50, 50, FiftyMeterNavigationHandler, NULL);
    LE_ASSERT(NULL != FiftyNavigationHandlerRef);

    // le_pos_AddMovementHandler calculates an acquisitionRate (look at le_pos_AddMovementHandler
    // and CalculateAcquisitionRate())
    // Test that the acquisitionRate is 4000 msec.
    LE_ASSERT(4000 == le_pos_GetAcquisitionRate());

    // Test the registration of an handler for movement notifications with horizontal or vertical
    // magnitude of 0 meters. (It will set an acquisition rate of 1sec).
    NavigationHandlerRef = le_pos_AddMovementHandler(0, 0, NavigationHandler, NULL);
    LE_ASSERT(NULL != NavigationHandlerRef);

    // Test that the acquisitionRate is 1000 msec
    // (because final acquisitionRate is the smallest calculated).
    LE_ASSERT(1000 == le_pos_GetAcquisitionRate());

    le_event_RunLoop();
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: le_pos_SetDistanceResolution
 *
 * The results can also be cheched visualy
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_pos_DistanceResolutionUpdate
(
    void
)
{
    int32_t           latitude;
    int32_t           longitude;
    int32_t           altitude, altitudeSav;
    int32_t           hAccuracy, hAccuracySav;
    int32_t           vAccuracy, vAccuracySav;
    le_result_t       res;

    LE_ASSERT(LE_BAD_PARAMETER == le_pos_SetDistanceResolution(LE_POS_RES_UNKNOWN));

    // get the default values (in meters)
    LE_ASSERT_OK(le_pos_SetDistanceResolution(LE_POS_RES_METER));
    res = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);
    LE_ASSERT((LE_OK == res) || (LE_OUT_OF_RANGE == res));
    LE_INFO("Meter resolution: hAccuracy %d, altitude %d, vAccuracy %d",
            hAccuracy, altitude, vAccuracy);
    altitudeSav = altitude;
    hAccuracySav = hAccuracy;
    vAccuracySav = vAccuracy;

    // test decmeter resolution
    LE_ASSERT_OK(le_pos_SetDistanceResolution(LE_POS_RES_DECIMETER));
    res = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);
    LE_ASSERT((LE_OK == res) || (LE_OUT_OF_RANGE == res));
    LE_INFO("Decimeter: hAccuracy %d, altitude %d, vAccuracy %d", hAccuracy, altitude, vAccuracy);

    if ((UINT32_MAX != hAccuracy) && (UINT32_MAX != hAccuracySav))
    {
        LE_ASSERT(hAccuracy != hAccuracySav);
    }
    if ((UINT32_MAX != vAccuracy) && (UINT32_MAX != vAccuracySav))
    {
        LE_ASSERT(vAccuracy != vAccuracySav);
    }
    if ((UINT32_MAX != altitude) && (UINT32_MAX != altitudeSav))
    {
        LE_ASSERT(altitude != altitudeSav);
    }
    altitudeSav = altitude;
    hAccuracySav = hAccuracy;
    vAccuracySav = vAccuracy;

    // test centimeter resolution
    LE_ASSERT_OK(le_pos_SetDistanceResolution(LE_POS_RES_CENTIMETER));
    res = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);
    LE_ASSERT((LE_OK == res) || (LE_OUT_OF_RANGE == res));
    LE_INFO("Centimeter: hAccuracy.%d, altitude.%d, vAccuracy.%d", hAccuracy, altitude, vAccuracy);

    if ((UINT32_MAX != hAccuracy) && (UINT32_MAX != hAccuracySav))
    {
        LE_ASSERT(hAccuracy != hAccuracySav);
    }
    if ((UINT32_MAX != vAccuracy) && (UINT32_MAX != vAccuracySav))
    {
        LE_ASSERT(vAccuracy != vAccuracySav);
    }
    if ((UINT32_MAX != altitude) && (UINT32_MAX != altitudeSav))
    {
        LE_ASSERT(altitude != altitudeSav);
    }
    altitudeSav = altitude;
    hAccuracySav = hAccuracy;
    vAccuracySav = vAccuracy;

    // test millimeter resolution
    LE_ASSERT_OK(le_pos_SetDistanceResolution(LE_POS_RES_MILLIMETER));
    res = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);
    LE_ASSERT((LE_OK == res) || (LE_OUT_OF_RANGE == res));
    LE_INFO("Millimeter: hAccuracy.%d, altitude.%d, vAccuracy.%d", hAccuracy, altitude, vAccuracy);

    if ((UINT32_MAX != hAccuracy) && (UINT32_MAX != hAccuracySav))
    {
        LE_ASSERT(hAccuracy != hAccuracySav);
    }
    if ((UINT32_MAX != vAccuracy) && (UINT32_MAX != vAccuracySav))
    {
        LE_ASSERT(vAccuracy != vAccuracySav);
    }
    if ((UINT32_MAX != altitude) && (UINT32_MAX != altitudeSav))
    {
        LE_ASSERT(altitude != altitudeSav);
    }
    altitudeSav = altitude;
    hAccuracySav = hAccuracy;
    vAccuracySav = vAccuracy;

    // test meter resolution
    LE_ASSERT_OK(le_pos_SetDistanceResolution(LE_POS_RES_METER));
    res = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);
    LE_ASSERT((LE_OK == res) || (LE_OUT_OF_RANGE == res));
    LE_INFO("Meter: hAccuracy.%d, altitude.%d, vAccuracy.%d", hAccuracy, altitude, vAccuracy);

    if ((UINT32_MAX != hAccuracy) && (UINT32_MAX != hAccuracySav))
    {
        LE_ASSERT(hAccuracy != hAccuracySav);
    }
    if ((UINT32_MAX != vAccuracy) && (UINT32_MAX != vAccuracySav))
    {
        LE_ASSERT(vAccuracy != vAccuracySav);
    }
    if ((UINT32_MAX != altitude) && (UINT32_MAX != altitudeSav))
    {
        LE_ASSERT(altitude != altitudeSav);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Get position Fix info.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_pos_GetInfo
(
    void
)
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
    uint16_t          year = 0;
    uint16_t          month = 0;
    uint16_t          day = 0;
    uint16_t          hours = 0;
    uint16_t          minutes = 0;
    uint16_t          seconds = 0;
    uint16_t          milliseconds = 0;
    le_pos_FixState_t fixState;
    le_result_t       res;

    LE_ASSERT_OK(le_pos_GetFixState(&fixState));
    LE_INFO("position fix state %d", fixState);

    res = le_pos_Get2DLocation(&latitude, &longitude, &hAccuracy);
    LE_INFO("le_pos_Get2DLocation %s",
            (LE_OK == res) ? "OK" : (LE_OUT_OF_RANGE == res) ? "parameter(s) out of range":"ERROR");
    LE_ASSERT((LE_OK == res) || (LE_OUT_OF_RANGE == res));
    LE_INFO("Check le_pos_Get2DLocation latitude.%d, longitude.%d, hAccuracy.%d",
            latitude, longitude, hAccuracy);

    res = le_pos_Get3DLocation(&latitude, &longitude, &hAccuracy, &altitude, &vAccuracy);
    LE_INFO("le_pos_Get3DLocation %s",
            (LE_OK == res) ? "OK" : (LE_OUT_OF_RANGE == res) ? "parameter(s) out of range":"ERROR");
    LE_ASSERT((LE_OK == res) || (LE_OUT_OF_RANGE == res));
    LE_INFO("Check le_pos_Get3DLocation latitude.%d, longitude.%d, hAccuracy.%d, altitude.%d"
            ", vAccuracy.%d",
            latitude, longitude, hAccuracy, altitude, vAccuracy);

    res = le_pos_GetDate(&year, &month, &day);
    LE_INFO("le_pos_GetDate %s",
            (LE_OK == res) ? "OK" : (LE_OUT_OF_RANGE == res) ? "parameter(s) out of range":"ERROR");
    LE_ASSERT((LE_OK == res) || (LE_OUT_OF_RANGE == res));
    LE_INFO("Check le_pos_GetDate year.%d, month.%d, day.%d",
            year, month, day);

    res = le_pos_GetTime(&hours, &minutes, &seconds, &milliseconds);
    LE_INFO("le_pos_GetTime %s",
            (LE_OK == res) ? "OK" : (LE_OUT_OF_RANGE == res) ? "parameter(s) out of range":"ERROR");
    LE_ASSERT((LE_OK == res) || (LE_OUT_OF_RANGE == res));
    LE_INFO("Check le_pos_GetTime hours.%d, minutes.%d, seconds.%d, milliseconds.%d",
            hours, minutes, seconds, milliseconds);

    res = le_pos_GetMotion(&hSpeed, &hSpeedAccuracy, &vSpeed, &vSpeedAccuracy);
    LE_INFO("le_pos_GetMotion %s",
            (LE_OK == res) ? "OK" : (LE_OUT_OF_RANGE == res) ? "parameter(s) out of range":"ERROR");
    LE_ASSERT((LE_OK == res) || (LE_OUT_OF_RANGE == res));
    LE_INFO("Check le_pos_GetMotion hSpeed.%u, hSpeedAccuracy.%u, vSpeed.%d, vSpeedAccuracy.%d",
            hSpeed, hSpeedAccuracy, vSpeed, vSpeedAccuracy);

    res = le_pos_GetHeading(&heading, &headingAccuracy);
    LE_INFO("le_pos_GetHeading %s",
            (LE_OK == res) ? "OK" : (LE_OUT_OF_RANGE == res) ? "parameter(s) out of range":"ERROR");
    LE_ASSERT((LE_OK == res) || (LE_OUT_OF_RANGE == res));
    LE_INFO("Check le_pos_GetHeading heading.%u, headingAccuracy.%u",
            heading, headingAccuracy);

    res = le_pos_GetDirection(&direction, &directionAccuracy);
    LE_INFO("le_pos_GetDirection %s",
            (LE_OK == res) ? "OK" : (LE_OUT_OF_RANGE == res) ? "parameter(s) out of range":"ERROR");
    LE_ASSERT((LE_OK == res) || (LE_OUT_OF_RANGE == res));
    LE_INFO("Check le_pos_GetDirection direction.%u, directionAccuracy.%u",
            direction, directionAccuracy);

    // Test NULL pointer (regression test for LE-4708)
    res = le_pos_GetDirection(&direction, NULL);
    LE_INFO("le_pos_GetDirection %s",
            (LE_OK == res) ? "OK" : (LE_OUT_OF_RANGE == res) ? "parameter(s) out of range":"ERROR");
    LE_ASSERT((LE_OK == res) || (LE_OUT_OF_RANGE == res));
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Get position Fix info.
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_pos_TestAcquisitionRate
(
    void
)
{
    LE_ASSERT_OK(le_pos_SetAcquisitionRate(3000));
    LE_ASSERT(3000 == le_pos_GetAcquisitionRate());

    LE_ASSERT(LE_OUT_OF_RANGE == le_pos_SetAcquisitionRate(0));

    LE_ASSERT_OK(le_pos_SetAcquisitionRate(1000));
    LE_ASSERT(1000 == le_pos_GetAcquisitionRate());
}

//--------------------------------------------------------------------------------------------------
/**
 * Setting/Getting enabled GPS NMEA sentences mask
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_pos_ActivateGpsNmeaSentences
(
    void
)
{
    int i = 0;
    le_gnss_NmeaBitMask_t nmeaMask;
    le_gnss_NmeaBitMask_t saveNmeaMask;

    LE_INFO("Start Test TestLeGnssNmeaSentences");

    // Test 1: bit mask too big, error
    nmeaMask = (LE_GNSS_NMEA_SENTENCES_MAX << 1) | 1;
    LE_ASSERT(LE_BAD_PARAMETER == le_gnss_SetNmeaSentences(nmeaMask));

    // Test 2: test all bits from the bit mask
    le_gnss_NmeaBitMask_t nmeaSentencesList[] = {
        LE_GNSS_NMEA_MASK_GPGGA,
        LE_GNSS_NMEA_MASK_GPGSA,
        LE_GNSS_NMEA_MASK_GPGSV,
        LE_GNSS_NMEA_MASK_GPRMC,
        LE_GNSS_NMEA_MASK_GPVTG,
        LE_GNSS_NMEA_MASK_GLGSV,
        LE_GNSS_NMEA_MASK_GNGNS,
        LE_GNSS_NMEA_MASK_GNGSA,
        LE_GNSS_NMEA_MASK_GAGGA,
        LE_GNSS_NMEA_MASK_GAGSA,
        LE_GNSS_NMEA_MASK_GAGSV,
        LE_GNSS_NMEA_MASK_GARMC,
        LE_GNSS_NMEA_MASK_GAVTG,
        0
    };

    for (i = 0; nmeaSentencesList[i]; i++)
    {
        LE_ASSERT_OK(le_gnss_SetNmeaSentences(nmeaSentencesList[i]));
        LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
        LE_ASSERT(nmeaMask == nmeaSentencesList[i]);
    }

    // @deprecated, PQXFI is deprecated. PTYPE is used instead.
    LE_ASSERT_OK(le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_PQXFI));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT(nmeaMask == (LE_GNSS_NMEA_MASK_PQXFI | LE_GNSS_NMEA_MASK_PTYPE));

    LE_ASSERT_OK(le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_PTYPE));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT(nmeaMask == (LE_GNSS_NMEA_MASK_PQXFI | LE_GNSS_NMEA_MASK_PTYPE));

    LE_ASSERT_OK(le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_PSTIS));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT((LE_GNSS_NMEA_MASK_GPGRS == nmeaMask) || (0 == nmeaMask));

    LE_ASSERT_OK(le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_GPGRS));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT((LE_GNSS_NMEA_MASK_GPGRS == nmeaMask) || (0 == nmeaMask));

    LE_ASSERT_OK(le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_GPGLL));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT((LE_GNSS_NMEA_MASK_GPGLL == nmeaMask) || (0 == nmeaMask));

    LE_ASSERT_OK(le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_DEBUG));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT((LE_GNSS_NMEA_MASK_DEBUG == nmeaMask) || (0 == nmeaMask));

    LE_ASSERT_OK(le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_GPDTM));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT((LE_GNSS_NMEA_MASK_GPDTM == nmeaMask) || (0 == nmeaMask));

    LE_ASSERT_OK(le_gnss_SetNmeaSentences(LE_GNSS_NMEA_MASK_GAGNS));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT((LE_GNSS_NMEA_MASK_GAGNS == nmeaMask) || (0 == nmeaMask));
    // Test 3: test bit mask combinations
    saveNmeaMask = LE_GNSS_NMEA_MASK_GPGGA | LE_GNSS_NMEA_MASK_GPGSA | LE_GNSS_NMEA_MASK_GPGSV |
                   LE_GNSS_NMEA_MASK_GPRMC | LE_GNSS_NMEA_MASK_GPVTG | LE_GNSS_NMEA_MASK_GLGSV |
                   LE_GNSS_NMEA_MASK_GNGNS | LE_GNSS_NMEA_MASK_GNGSA | LE_GNSS_NMEA_MASK_GAGGA |
                   LE_GNSS_NMEA_MASK_GAGSA | LE_GNSS_NMEA_MASK_GAGSV | LE_GNSS_NMEA_MASK_GARMC |
                   LE_GNSS_NMEA_MASK_GAVTG | LE_GNSS_NMEA_MASK_PQXFI | LE_GNSS_NMEA_MASK_PTYPE;

    LE_ASSERT_OK(le_gnss_SetNmeaSentences(saveNmeaMask));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT(nmeaMask == saveNmeaMask);

    LE_ASSERT_OK(le_gnss_SetNmeaSentences(saveNmeaMask | LE_GNSS_NMEA_MASK_GPGRS));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT(((saveNmeaMask | LE_GNSS_NMEA_MASK_GPGRS) == nmeaMask) || (saveNmeaMask == nmeaMask));

    LE_ASSERT_OK(le_gnss_SetNmeaSentences(saveNmeaMask | LE_GNSS_NMEA_MASK_GPGLL));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT(((saveNmeaMask | LE_GNSS_NMEA_MASK_GPGLL) == nmeaMask) || (saveNmeaMask == nmeaMask));

    LE_ASSERT_OK(le_gnss_SetNmeaSentences(saveNmeaMask | LE_GNSS_NMEA_MASK_DEBUG));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT(((saveNmeaMask | LE_GNSS_NMEA_MASK_DEBUG) == nmeaMask) || (saveNmeaMask == nmeaMask));

    LE_ASSERT_OK(le_gnss_SetNmeaSentences(saveNmeaMask | LE_GNSS_NMEA_MASK_GPDTM));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT(((saveNmeaMask | LE_GNSS_NMEA_MASK_GPDTM) == nmeaMask) || (saveNmeaMask == nmeaMask));

    LE_ASSERT_OK(le_gnss_SetNmeaSentences(saveNmeaMask | LE_GNSS_NMEA_MASK_GAGNS));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT(((saveNmeaMask | LE_GNSS_NMEA_MASK_GAGNS) == nmeaMask) || (saveNmeaMask == nmeaMask));
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset the GPS NMEA sentences mask
 *
 */
//--------------------------------------------------------------------------------------------------
static void Testle_pos_ResetGpsNmeaSentences
(
    void
)
{
    le_gnss_NmeaBitMask_t gpsNmeaMask = 0;
    le_gnss_NmeaBitMask_t nmeaMask;
    LE_ASSERT_OK(le_gnss_SetNmeaSentences(gpsNmeaMask));
    LE_ASSERT_OK(le_gnss_GetNmeaSentences(&nmeaMask));
    LE_ASSERT(nmeaMask == gpsNmeaMask);
}

//--------------------------------------------------------------------------------------------
/**
 * Remove the movement handlers
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveMovementHandlers
(
    void
)
{
    le_pos_RemoveMovementHandler(NavigationHandlerRef);
    NavigationHandlerRef = NULL;

    le_pos_RemoveMovementHandler(FiftyNavigationHandlerRef);
    FiftyNavigationHandlerRef = NULL;
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
    Testle_pos_DistanceResolutionUpdate();
    Testle_pos_GetInfo();
    sleep(1);
    LE_INFO("Release the positioning service");
    le_posCtrl_Release(activationRef);
    LE_INFO("======== Positioning Test finished ========");

    //Remove the movement handlers
    RemoveMovementHandlers();
    // Stop Navigation thread
    le_thread_Cancel(navigationThreadRef);

    Testle_pos_ActivateGpsNmeaSentences();
    Testle_pos_ResetGpsNmeaSentences();

    exit(EXIT_SUCCESS);
}
