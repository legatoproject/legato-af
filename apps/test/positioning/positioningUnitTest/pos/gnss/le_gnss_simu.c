//--------------------------------------------------------------------------------------------------
/**
 * @file le_gnss_simu.c
 *
 * This file contains the simulation functions for the GNSS API.
 * Refer to le_gnss_interface.h for info about the used functions
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "le_gnss_simu.h"

//--------------------------------------------------------------------------------------------------
/**
 * Time structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint16_t hours;               ///< The Hours.
    uint16_t minutes;             ///< The Minutes.
    uint16_t seconds;             ///< The Seconds.
    uint16_t milliseconds;        ///< The Milliseconds.
}
pa_Gnss_Time_t;

//--------------------------------------------------------------------------------------------------
/**
 * Date structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint16_t year;                ///< The Year.
    uint16_t month;               ///< The Month.
    uint16_t day;                 ///< The Day.
}
pa_Gnss_Date_t;

//--------------------------------------------------------------------------------------------------
/**
 * Satellite Vehicle information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint16_t                satId;        ///< Satellite in View ID number.
    le_gnss_Constellation_t satConst;     ///< GNSS constellation type.
    bool                    satUsed;      ///< TRUE if satellite in View is used for fix Navigation.
    bool                    satTracked;   ///< TRUE if satellite in View is tracked for Navigation.
    uint8_t                 satSnr;       ///< Satellite in View Signal To Noise Ratio [dBHz].
    uint16_t                satAzim;      ///< Satellite in View Azimuth [degrees].
                                          ///< Range: 0 to 360
    uint8_t                 satElev;      ///< Satellite in View Elevation [degrees].
                                          ///< Range: 0 to 90
}
Pa_Gnss_SvInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Satellite Measurement information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint16_t satId;          ///< Satellite in View ID number.
    int32_t  satLatency;     ///< Satellite latency measurement (age of measurement)
                             ///< Units: Milliseconds.
}
Pa_Gnss_SvMeasurement_t;

//--------------------------------------------------------------------------------------------------
/**
 * Position structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {

    le_gnss_FixState_t      fixState;       ///< Position Fix state

    bool                    latitudeValid;  ///< if true, latitude is set
    int32_t                 latitude;       ///< The Latitude in degrees, positive North,
                                            ///  with 6 decimal places.

    bool                    longitudeValid; ///< if true, longitude is set
    int32_t                 longitude;      ///< The Longitude in degrees, positive East,
                                            ///  with 6 decimal places.

    bool                    altitudeValid;  ///< if true, altitude is set
    int32_t                 altitude;       ///< The Altitude in meters, above Mean Sea Level,
                                            ///  with 3 decimal places.

    bool                    altitudeOnWgs84Valid; ///< if true, altitudeOnWgs84 is set
    int32_t                 altitudeOnWgs84;///< The altitudeOnWgs84 in meters, between WGS-84 earth
                                            ///  ellipsoid and mean sea level with 3 decimal places.

    bool                    hSpeedValid;    ///< if true, horizontal speed is set
    uint32_t                hSpeed;        ///< The horizontal Speed in m/sec, with 2 decimal places
                                            ///  (125 = 1.25m/sec).

    bool                    vSpeedValid;    ///< if true, vertical speed is set
    uint32_t                vSpeed;         ///< The vertical Speed in m/sec, with 2 decimal places
                                            ///  (125 = 1.25m/sec).

    bool                    directionValid; ///< if true, direction is set
    uint32_t                direction;      ///< Direction in degrees, where 0 is True North, with 1
                                            ///  decimal place (308  = 30.8 degrees).

    bool                    headingValid;   ///< if true, heading is set
    uint32_t                heading;        ///< heading in degrees, where 0 is True North, with 1
                                            ///  decimal place (308  = 30.8 degrees).

    bool                    hdopValid;      ///< if true, horizontal dilution is set
    uint32_t                hdop;           ///< The horizontal dilution of precision (DOP)

    bool                    pdopValid;      ///< if true, position dilution is set
    uint32_t                pdop;           ///< The position dilution of precision (DOP)

    bool                    vdopValid;      ///< if true, vertical dilution is set
    uint32_t                vdop;           ///< The vertical dilution of precision (DOP)

    bool                    gdopValid;      ///< if true, geometric dilution is set
    uint32_t                gdop;           ///< The geometric dilution of precision (DOP)

    bool                    tdopValid;      ///< if true, time dilution is set
    uint32_t                tdop;           ///< The time dilution of precision (DOP)

    bool                    hUncertaintyValid;  ///< if true, horizontal uncertainty is set
    uint32_t                hUncertainty;       ///< The horizontal uncertainty in meters,
                                                ///  with 2 decimal places

    bool                    vUncertaintyValid;  ///< if true, vertical uncertainty is set
    uint32_t                vUncertainty;       ///< The vertical uncertainty in meters,
                                                ///  with 1 decimal place

    bool                    hSpeedUncertaintyValid;///< if true, horizontal speed uncertainty is set
    uint32_t                hSpeedUncertainty;      ///< The horizontal speed uncertainty in m/sec,
                                                    ///  with 1 decimal place

    bool                    vSpeedUncertaintyValid; ///< if true, vertical speed uncertainty is set
    uint32_t                vSpeedUncertainty;      ///< The vertical speed uncertainty in m/sec,
                                                    ///  with 1 decimal place

    bool                    magneticDeviationValid; ///< if true, magnetic deviation is set
    int32_t                 magneticDeviation;      ///< The magnetic deviation in degrees,
                                                    ///  with 1 decimal place

    bool                    directionUncertaintyValid; ///< if true, direction uncertainty is set
    uint32_t                directionUncertainty;      ///< The direction uncertainty in degrees,
                                                       ///  with 1 decimal place
    // UTC time
    bool                    timeValid;           ///< if true, time is set
    pa_Gnss_Time_t          time;                ///< The time of the fix
    uint64_t                epochTime;           ///< Epoch time in milliseconds since Jan. 1, 1970
    bool                    dateValid;           ///< if true, date is set
    pa_Gnss_Date_t          date;                ///< The date of the fix

    // Leap Seconds
    bool                    leapSecondsValid;    ///< if true, leapSeconds is set
    uint8_t                 leapSeconds;         ///< UTC leap seconds in advance in seconds
    // GPS time
    bool                    gpsTimeValid;        ///< if true, GPS time is set
    uint32_t                gpsWeek;             ///< GPS week number from midnight, Jan. 6, 1980.
    uint32_t                gpsTimeOfWeek;     ///< Amount of time in milliseconds into the GPS week.
    // Time accuracy
    bool                    timeAccuracyValid;      ///< if true, timeAccuracy is set
    uint32_t                timeAccuracy;           ///< Estimated Accuracy for time in milliseconds
    // Position measurement latency
    bool                    positionLatencyValid;   ///< if true, positionLatency is set
    uint32_t                positionLatency;       ///< Position measurement latency in milliseconds
    // Satellite Vehicles information
    bool                    satsInViewCountValid;   ///< if true, satsInView is set
    uint8_t                 satsInViewCount;        ///< Satellites in View count.
    bool                    satsTrackingCountValid; ///< if true, satsTrackingCount is set
    uint8_t                 satsTrackingCount;      ///< Tracking satellites in View.
    bool                    satsUsedCountValid;     ///< if true, satsUsedCount is set
    uint8_t                 satsUsedCount;          ///< Satellites in View used for Navigation.
    bool                    satInfoValid;           ///< if true, satInfo is set
    Pa_Gnss_SvInfo_t        satInfo[LE_GNSS_SV_INFO_MAX_LEN];
                                                    ///< Satellite Vehicle information.
    bool                    satMeasValid;           ///< if true, satInfo is set
    Pa_Gnss_SvMeasurement_t satMeas[LE_GNSS_SV_INFO_MAX_LEN];
                                                    ///< Satellite measurement information.
}
pa_Gnss_Position_t;

//--------------------------------------------------------------------------------------------------
/**
 * Maintains simulated location data
 *
 */
//--------------------------------------------------------------------------------------------------
static gnssSimuLocation_t GnssLocation;

//--------------------------------------------------------------------------------------------------
/**
 * Maintains simulated altitude data
 *
 */
//--------------------------------------------------------------------------------------------------
static gnssSimuAltitude_t GnssAltitude;

//--------------------------------------------------------------------------------------------------
/**
 * Maintains simulated direction data
 *
 */
//--------------------------------------------------------------------------------------------------
static gnssSimuDirection_t GnssDirection;

//--------------------------------------------------------------------------------------------------
/**
 * Maintains simulated horizontal data
 *
 */
//--------------------------------------------------------------------------------------------------
static gnssSimuHSpeed_t GnssHSpeed;

//--------------------------------------------------------------------------------------------------
/**
 * Maintains simulated vertical data
 *
 */
//--------------------------------------------------------------------------------------------------
static gnssSimuVSpeed_t GnssVSpeed;

//--------------------------------------------------------------------------------------------------
/**
 * Maintains simulated date data
 *
 */
//--------------------------------------------------------------------------------------------------
static gnssSimuDate_t GnssDate;

//--------------------------------------------------------------------------------------------------
/**
 * Maintains simulated time data
 *
 */
//--------------------------------------------------------------------------------------------------
static gnssSimuTime_t GnssTime;

//--------------------------------------------------------------------------------------------------
/**
 * Maintains simulated DOP data
 *
 */
//--------------------------------------------------------------------------------------------------
static gnssSimuDop_t GnssDop;

//--------------------------------------------------------------------------------------------------
/**
 * Maintains simulated position state
 *
 */
//--------------------------------------------------------------------------------------------------
static gnssSimuPositionState_t GnssSimuPositionSate;

//--------------------------------------------------------------------------------------------------
/**
 * Sample reference
 *
 */
//--------------------------------------------------------------------------------------------------
static le_gnss_SampleRef_t Sample;

//--------------------------------------------------------------------------------------------------
/**
 * Memory pool for call event data.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PositionSamplePoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Event for new Pos state
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t PosEventId = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * The computed position Sample data.
 */
//--------------------------------------------------------------------------------------------------
static le_gnss_PositionSample_t PositionSampleData;

//--------------------------------------------------------------------------------------------------
/**
 * The computed position data.
 */
//--------------------------------------------------------------------------------------------------
static pa_Gnss_Position_t PositionData;

//--------------------------------------------------------------------------------------------------
/**
 * le_gnssSimu_SetLocation: update simulated location data
 *
 */
//--------------------------------------------------------------------------------------------------
void le_gnssSimu_SetLocation
(
    gnssSimuLocation_t gnssLocation
)
{
    GnssLocation = gnssLocation;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_gnssSimu_SetAltitude: update simulated altitude data
 *
 */
//--------------------------------------------------------------------------------------------------
void le_gnssSimu_SetAltitude
(
    gnssSimuAltitude_t gnssAltitude
)
{
    GnssAltitude = gnssAltitude;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_gnssSimu_SetDate: update simulated date data
 *
 */
//--------------------------------------------------------------------------------------------------
void le_gnssSimu_SetDate
(
    gnssSimuDate_t gnssDate
)
{
    GnssDate = gnssDate;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_gnssSimu_SetDirection: update simulated direction data
 *
 */
//--------------------------------------------------------------------------------------------------
void le_gnssSimu_SetDirection
(
    gnssSimuDirection_t gnssDirection
)
{
    GnssDirection = gnssDirection;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_gnssSimu_SetHSpeed: update simulated horizontal speed data
 *
 */
//--------------------------------------------------------------------------------------------------
void le_gnssSimu_SetHSpeed
(
    gnssSimuHSpeed_t gnssHSpeed
)
{
    GnssHSpeed = gnssHSpeed;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_gnssSimu_SetVSpeed: update simulated vertical speed data
 *
 */
//--------------------------------------------------------------------------------------------------
void le_gnssSimu_SetVSpeed
(
    gnssSimuVSpeed_t gnssVSpeed
)
{
    GnssVSpeed = gnssVSpeed;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_gnssSimu_SetTime: update simulated time data
 *
 */
//--------------------------------------------------------------------------------------------------
void le_gnssSimu_SetTime
(
    gnssSimuTime_t gnssTime
)
{
    GnssTime = gnssTime;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_gnssSimu_SetDop: update simulated DOP data
 *
 */
//--------------------------------------------------------------------------------------------------
void le_gnssSimu_SetDop
(
    gnssSimuDop_t gnssdop
)
{
    GnssDop = gnssdop;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_gnssSimu_SetSampleRef: set sample reference
 *
 */
//--------------------------------------------------------------------------------------------------
void le_gnssSimu_SetSampleRef
(
    le_gnss_SampleRef_t sample
)
{
    Sample = sample;
}

//--------------------------------------------------------------------------------------------------
/**
 * le_gnssSimu_SetPositionState: set position state
 *
 */
//--------------------------------------------------------------------------------------------------
void le_gnssSimu_SetPositionState
(
    gnssSimuPositionState_t state
)
{
    GnssSimuPositionSate = state;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the GNSS position information.
 */
//--------------------------------------------------------------------------------------------------
static void InitializeValidGnssPosition
(
    pa_Gnss_Position_t* posDataPtr       ///< [IN/OUT] Pointer to the position data.
)
{
    if (NULL == posDataPtr)
    {
        LE_ERROR("posDataPtr is NULL !");
        return;
    }

    posDataPtr->fixState = LE_GNSS_STATE_FIX_NO_POS;

    posDataPtr->latitudeValid = true;
    posDataPtr->latitude=48823091;

    posDataPtr->longitudeValid = true;
    posDataPtr->longitude=2249324;

    posDataPtr->altitudeValid = true;
    posDataPtr->altitude=32000;

    posDataPtr->altitudeOnWgs84Valid = true;
    posDataPtr->altitudeOnWgs84=32;

    posDataPtr->hSpeedValid=true;
    posDataPtr->hSpeed=3600;

    posDataPtr->vSpeedValid=true;
    posDataPtr->vSpeed=300;

    posDataPtr->directionValid = true;
    posDataPtr->direction=100;

    posDataPtr->headingValid = false;

    posDataPtr->hdopValid = false;

    posDataPtr->vdopValid = false;

    posDataPtr->gdopValid = false;

    posDataPtr->tdopValid = false;

    posDataPtr->hUncertaintyValid = false;

    posDataPtr->vUncertaintyValid = false;

    posDataPtr->hSpeedUncertaintyValid = false;

    posDataPtr->vSpeedUncertaintyValid = false;

    posDataPtr->magneticDeviationValid = false;

    posDataPtr->directionUncertaintyValid = false;

    // Date
    posDataPtr->dateValid = true;
    posDataPtr->date.year = 2016;
    posDataPtr->date.month = 12;
    posDataPtr->date.day = 12;

    // Time
    posDataPtr->timeValid = true;
    posDataPtr->time.hours = 120;
    posDataPtr->time.minutes = 15;
    posDataPtr->time.seconds = 54;
    posDataPtr->time.milliseconds = 1245;

    // Leap Seconds
    posDataPtr->leapSecondsValid = false;

    // Epoch time
    posDataPtr->epochTime = false;

    // GPS time
    posDataPtr->gpsTimeValid = false;

    // Time accuracy
    posDataPtr->timeAccuracyValid = false;

    // Position measurement latency
    posDataPtr->positionLatencyValid = false;

    // Satellites information
    posDataPtr->satsInViewCountValid = false;
    posDataPtr->satsTrackingCountValid = false;
    posDataPtr->satsUsedCountValid = false;
    posDataPtr->satInfoValid = false;

    // Satellite latency measurement
    posDataPtr->satMeasValid = false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample information from GNSS position information.
 */
//--------------------------------------------------------------------------------------------------

static void GetPosSampleData
(
    le_gnss_PositionSample_t* posSampleDataPtr,  ///< [OUT] Pointer to the position sample report.
    pa_Gnss_Position_t* paPosDataPtr             ///< [IN] The PA position data report.
)
{
    uint8_t i;

    if (NULL == posSampleDataPtr)
    {
        LE_ERROR("posSampleDataPtr is NULL !");
        return;
    }
    if (NULL == paPosDataPtr)
    {
        LE_ERROR("paPosDataPtr is NULL !");
        return;
    }

    // Position information
    posSampleDataPtr->fixState = paPosDataPtr->fixState;
    posSampleDataPtr->latitudeValid = paPosDataPtr->latitudeValid;
    posSampleDataPtr->latitude = paPosDataPtr->latitude;
    posSampleDataPtr->longitudeValid = paPosDataPtr->longitudeValid;
    posSampleDataPtr->longitude = paPosDataPtr->longitude;
    posSampleDataPtr->hAccuracyValid = paPosDataPtr->hUncertaintyValid;
    posSampleDataPtr->hAccuracy = paPosDataPtr->hUncertainty;
    posSampleDataPtr->altitudeValid = paPosDataPtr->altitudeValid;
    posSampleDataPtr->altitude = paPosDataPtr->altitude;
    posSampleDataPtr->altitudeOnWgs84Valid = paPosDataPtr->altitudeOnWgs84Valid;
    posSampleDataPtr->altitudeOnWgs84 = paPosDataPtr->altitudeOnWgs84;
    posSampleDataPtr->vAccuracyValid = paPosDataPtr->vUncertaintyValid;
    posSampleDataPtr->vAccuracy = paPosDataPtr->vUncertainty;
    posSampleDataPtr->hSpeedValid = paPosDataPtr->hSpeedValid;
    posSampleDataPtr->hSpeed = paPosDataPtr->hSpeed;
    posSampleDataPtr->hSpeedAccuracyValid = paPosDataPtr->hSpeedUncertaintyValid;
    posSampleDataPtr->hSpeedAccuracy = paPosDataPtr->hSpeedUncertainty;
    posSampleDataPtr->vSpeedValid = paPosDataPtr->vSpeedValid;
    posSampleDataPtr->vSpeed = paPosDataPtr->vSpeed;
    posSampleDataPtr->vSpeedAccuracyValid = paPosDataPtr->vSpeedUncertaintyValid;
    posSampleDataPtr->vSpeedAccuracy = paPosDataPtr->vSpeedUncertainty;
    posSampleDataPtr->directionValid = paPosDataPtr->directionValid;
    posSampleDataPtr->direction = paPosDataPtr->direction;
    posSampleDataPtr->directionAccuracyValid = paPosDataPtr->directionUncertaintyValid;
    posSampleDataPtr->directionAccuracy = paPosDataPtr->directionUncertainty;
    posSampleDataPtr->magneticDeviationValid = paPosDataPtr->magneticDeviationValid;
    posSampleDataPtr->magneticDeviation = paPosDataPtr->magneticDeviation;
    // Date
    posSampleDataPtr->dateValid = paPosDataPtr->dateValid;
    posSampleDataPtr->year = paPosDataPtr->date.year;
    posSampleDataPtr->month = paPosDataPtr->date.month;
    posSampleDataPtr->day = paPosDataPtr->date.day;
    // UTC time
    posSampleDataPtr->timeValid = paPosDataPtr->timeValid;
    posSampleDataPtr->hours = paPosDataPtr->time.hours;
    posSampleDataPtr->minutes = paPosDataPtr->time.minutes;
    posSampleDataPtr->seconds = paPosDataPtr->time.seconds;
    posSampleDataPtr->milliseconds = paPosDataPtr->time.milliseconds;
    // Leap Seconds
    posSampleDataPtr->leapSecondsValid = paPosDataPtr->leapSecondsValid;
    posSampleDataPtr->leapSeconds = paPosDataPtr->leapSeconds;
    // Epoch time
    posSampleDataPtr->epochTime = paPosDataPtr->epochTime;
    // GPS time
    posSampleDataPtr->gpsTimeValid = paPosDataPtr->gpsTimeValid;
    posSampleDataPtr->gpsWeek = paPosDataPtr->gpsWeek;
    posSampleDataPtr->gpsTimeOfWeek = paPosDataPtr->gpsTimeOfWeek;
    // Time accuracy
    posSampleDataPtr->timeAccuracyValid = paPosDataPtr->timeAccuracyValid;
    posSampleDataPtr->timeAccuracy = paPosDataPtr->timeAccuracy;
    // Position measurement latency
    posSampleDataPtr->positionLatencyValid = paPosDataPtr->positionLatencyValid;
    posSampleDataPtr->positionLatency = paPosDataPtr->positionLatency;
    // DOP parameters
    posSampleDataPtr->hdopValid = paPosDataPtr->hdopValid;
    posSampleDataPtr->hdop = paPosDataPtr->hdop;
    posSampleDataPtr->vdopValid = paPosDataPtr->vdopValid;
    posSampleDataPtr->vdop = paPosDataPtr->vdop;
    posSampleDataPtr->pdopValid = paPosDataPtr->pdopValid;
    posSampleDataPtr->pdop = paPosDataPtr->pdop;
    posSampleDataPtr->gdopValid = paPosDataPtr->gdopValid;
    posSampleDataPtr->gdop = paPosDataPtr->gdop;
    posSampleDataPtr->tdopValid = paPosDataPtr->tdopValid;
    posSampleDataPtr->tdop = paPosDataPtr->tdop;

    // Satellites information
    posSampleDataPtr->satsInViewCountValid = paPosDataPtr->satsInViewCountValid;
    posSampleDataPtr->satsInViewCount = paPosDataPtr->satsInViewCount;
    posSampleDataPtr->satsTrackingCountValid = paPosDataPtr->satsTrackingCountValid;
    posSampleDataPtr->satsTrackingCount = paPosDataPtr->satsTrackingCount;
    posSampleDataPtr->satsUsedCountValid = paPosDataPtr->satsUsedCountValid;
    posSampleDataPtr->satsUsedCount = paPosDataPtr->satsUsedCount;
    posSampleDataPtr->satInfoValid = paPosDataPtr->satInfoValid;
    for(i=0; i<LE_GNSS_SV_INFO_MAX_LEN; i++)
    {
        posSampleDataPtr->satInfo[i].satId = paPosDataPtr->satInfo[i].satId;
        posSampleDataPtr->satInfo[i].satConst = paPosDataPtr->satInfo[i].satConst;
        posSampleDataPtr->satInfo[i].satUsed = paPosDataPtr->satInfo[i].satUsed;
        posSampleDataPtr->satInfo[i].satTracked = paPosDataPtr->satInfo[i].satTracked;
        posSampleDataPtr->satInfo[i].satSnr = paPosDataPtr->satInfo[i].satSnr;
        posSampleDataPtr->satInfo[i].satAzim = paPosDataPtr->satInfo[i].satAzim;
        posSampleDataPtr->satInfo[i].satElev = paPosDataPtr->satInfo[i].satElev;
    }

    // Satellite latency measurement
    posSampleDataPtr->satMeasValid = paPosDataPtr->satMeasValid;
    for(i=0; i<LE_GNSS_SV_INFO_MAX_LEN; i++)
    {
        posSampleDataPtr->satMeas[i].satId = paPosDataPtr->satMeas[i].satId;
        posSampleDataPtr->satMeas[i].satLatency = paPosDataPtr->satMeas[i].satLatency;
    }

    // Node Link
    posSampleDataPtr->link = LE_DLS_LINK_INIT;

}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the GNSS
 *
 * @return
 *  - LE_FAULT  The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is already initialized.
 *  - LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gnss_Init
(
    void
)
{
    InitializeValidGnssPosition(&PositionData);
    GetPosSampleData(&PositionSampleData, &PositionData);
    PosEventId = le_event_CreateIdWithRefCounting("PosEventId");
    PositionSamplePoolRef = le_mem_CreatePool("PositionSamplePoolRef",
                            sizeof(le_gnss_PositionSample_t));
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler for position notifications.
 *
 *  - A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_gnss_PositionHandlerRef_t le_gnss_AddPositionHandler
(
    le_gnss_PositionHandlerFunc_t handlerPtr,          ///< [IN] The handler function.
    void*                         contextPtr           ///< [IN] The context pointer
)
{
    le_event_HandlerRef_t        handlerRef;

    // Create an event Id for new Network Registration State notification if not already done
    if (handlerPtr == NULL)
    {
        LE_ERROR("Handler function is NULL!");
        return NULL;
    }

    handlerRef = le_event_AddHandler("PosEventHandler", PosEventId,
                 (le_event_HandlerFunc_t)handlerPtr);

    return (le_gnss_PositionHandlerRef_t)handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove a handler for position notifications.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_gnss_RemovePositionHandler
(
    le_gnss_PositionHandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * To report the event for handler.
 */
//--------------------------------------------------------------------------------------------------
void le_gnssSimu_ReportEvent
(
    void
)
{

    // Build the data for the user's event handler.
    le_gnss_PositionSample_t* posSampleDataPtr = (le_gnss_PositionSample_t*)
                                                 le_mem_ForceAlloc(PositionSamplePoolRef);
    memcpy(posSampleDataPtr, &PositionSampleData, sizeof(le_gnss_PositionSample_t));
    le_event_ReportWithRefCounting(PosEventId, posSampleDataPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the position sample's fix state
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetPositionState
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    le_gnss_FixState_t* statePtr
        ///< [OUT]
        ///< Position fix state.
)
{
    // Check input pointers
    if (NULL == statePtr)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }
    *statePtr = GnssSimuPositionSate.state;

    return GnssSimuPositionSate.result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the location's data (Latitude, Longitude, Horizontal accuracy).
 *
 * @return
 *  - LE_FAULT         Function failed to get the location's data
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note latitudePtr, longitudePtr, hAccuracyPtr, altitudePtr, vAccuracyPtr can be set to NULL
 *       if not needed.
 *
 * @note The latitude and longitude values are based on the WGS84 standard coordinate system.
 *
 * @note The latitude and longitude values are given in degrees with 6 decimal places like:
 *       Latitude +48858300 = 48.858300 degrees North
 *       Longitude +2294400 = 2.294400 degrees East
 *       (The latitude and longitude values are given in degrees, minutes, seconds in NMEA frame)
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetLocation
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    int32_t* latitudePtr,
        ///< [OUT]
        ///< WGS84 Latitude in degrees, positive North [resolution 1e-6].

    int32_t* longitudePtr,
        ///< [OUT]
        ///< WGS84 Longitude in degrees, positive East [resolution 1e-6].

    int32_t* hAccuracyPtr
        ///< [OUT]
        ///< Horizontal position's accuracy in meters [resolution 1e-2].
)
{
    if (latitudePtr)
    {
        *latitudePtr = GnssLocation.latitude;
    }
    if (longitudePtr)
    {
        *longitudePtr = GnssLocation.longitude;
    }
    if (hAccuracyPtr)
    {
        *hAccuracyPtr = GnssLocation.accuracy;
    }

    return GnssLocation.result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's altitude.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note Altitude is in meters, above Mean Sea Level, with 3 decimal places (3047 = 3.047 meters).
 *
 * @note For a 2D position fix, the altitude will be indicated as invalid and set to INT32_MAX
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note altitudePtr, altitudeAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetAltitude
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    int32_t* altitudePtr,
        ///< [OUT]
        ///< Altitude in meters, above Mean Sea Level [resolution 1e-3].

    int32_t* vAccuracyPtr
        ///< [OUT]
        ///< Vertical position's accuracy in meters [resolution 1e-1].
)
{
    if (altitudePtr)
    {
        *altitudePtr = GnssAltitude.altitude;
    }
    if (vAccuracyPtr)
    {
        *vAccuracyPtr = GnssAltitude.accuracy;
    }

    return GnssAltitude.result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's time.
 *
 * @return
 *  - LE_FAULT         Function failed to get the time.
 *  - LE_OUT_OF_RANGE  The retrieved time is invalid (all fields are set to 0).
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetTime
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint16_t* hoursPtr,
        ///< [OUT]
        ///< UTC Hours into the day [range 0..23].

    uint16_t* minutesPtr,
        ///< [OUT]
        ///< UTC Minutes into the hour [range 0..59].

    uint16_t* secondsPtr,
        ///< [OUT]
        ///< UTC Seconds into the minute [range 0..59].

    uint16_t* millisecondsPtr
        ///< [OUT]
        ///< UTC Milliseconds into the second [range 0..999].
)
{
    // Check input pointers
    if ((NULL == hoursPtr)
        || (NULL == minutesPtr)
        || (NULL == secondsPtr)
        || (NULL == millisecondsPtr))
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    *hoursPtr = GnssTime.hrs;
    *minutesPtr = GnssTime.min;
    *secondsPtr = GnssTime.sec;
    *millisecondsPtr = GnssTime.msec;

    return GnssTime.result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's GPS time.
 *
 * @return
 *  - LE_FAULT         Function failed to get the time.
 *  - LE_OUT_OF_RANGE  The retrieved time is invalid (all fields are set to 0).
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetGpsTime
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* gpsWeekPtr,
        ///< [OUT] GPS week number from midnight, Jan. 6, 1980.

    uint32_t* gpsTimeOfWeekPtr
        ///< [OUT] Amount of time in milliseconds into the GPS week.
)
{
    // Check input pointers
    if ((NULL == gpsWeekPtr) || (NULL == gpsTimeOfWeekPtr))
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's time accurary.
 *
 * @return
 *  - LE_FAULT         Function failed to get the time.
 *  - LE_OUT_OF_RANGE  The retrieved time accuracy is invalid (set to UINT16_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetTimeAccuracy
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* timeAccuracyPtr
        ///< [OUT] Estimated time accuracy in nanoseconds
)
{
    // Check input pointers
    if (NULL == timeAccuracyPtr)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's UTC leap seconds in advance
 *
 * @return
 *  - LE_FAULT         Function failed to get the leap seconds.
 *  - LE_OUT_OF_RANGE  The retrieved time accuracy is invalid (set to UINT8_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note The leap seconds in advance is the accumulated time in seconds since the start of GPS Epoch
 * time (Jan 6, 1980). This value has to be added to the UTC time (since Jan. 1, 1970)
 *
 * @note Insertion of each UTC leap second is usually decided about six months in advance by the
 * International Earth Rotation and Reference Systems Service (IERS).
 *
 * @note If the caller is passing an invalid position sample reference or a null pointer into this
 *       function, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetGpsLeapSeconds
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint8_t* leapSecondsPtr
        ///< [OUT] UTC leap seconds in advance in seconds
)
{
    // Check input pointers
    if (NULL == leapSecondsPtr)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's date.
 *
 * @return
 *  - LE_FAULT         Function failed to get the date.
 *  - LE_OUT_OF_RANGE  The retrieved date is invalid (all fields are set to 0).
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetDate
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint16_t* yearPtr,
        ///< [OUT]
        ///< UTC Year A.D. [e.g. 2014].

    uint16_t* monthPtr,
        ///< [OUT]
        ///< UTC Month into the year [range 1...12].

    uint16_t* dayPtr
        ///< [OUT]
        ///< UTC Days into the month [range 1...31].
)
{
    // Check input pointers
    if ((NULL == yearPtr) || (NULL == monthPtr) || (NULL == dayPtr))
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    *yearPtr = GnssDate.year;
    *monthPtr = GnssDate.month;
    *dayPtr = GnssDate.day;

    return GnssDate.result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's horizontal speed.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to UINT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note hSpeedPtr, hSpeedAccuracyPtr can be set to NULL if not needed.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetHorizontalSpeed
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint32_t* hspeedPtr,
        ///< [OUT]
        ///< Horizontal speed in meters/second [resolution 1e-2].

    uint32_t* hspeedAccuracyPtr
        ///< [OUT]
        ///< Horizontal speed's accuracy estimate
        ///< in meters/second [resolution 1e-1].
)
{
    if (hspeedPtr)
    {
        *hspeedPtr = GnssHSpeed.speed;
    }

    if (hspeedAccuracyPtr)
    {
        *hspeedAccuracyPtr = GnssHSpeed.accuracy;
    }

    return GnssHSpeed.result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's vertical speed.
 *
 * @return
 *  - LE_FAULT         The function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX).
 *  - LE_OK            The function succeeded.
 *
 * @note vSpeedPtr, vSpeedAccuracyPtr can be set to NULL if not needed.
 *
 * @note For a 2D position Fix, the vertical speed will be indicated as invalid
 * and set to INT32_MAX.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetVerticalSpeed
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    int32_t* vspeedPtr,
        ///< [OUT]
        ///< Vertical speed in meters/second [resolution 1e-2].

    int32_t* vspeedAccuracyPtr
        ///< [OUT]
        ///< Vertical speed's accuracy estimate
        ///< in meters/second [resolution 1e-1].
)
{
    if (vspeedPtr)
    {
        *vspeedPtr = GnssVSpeed.speed;
    }

    if (vspeedAccuracyPtr)
    {
        *vspeedAccuracyPtr = GnssVSpeed.accuracy;
    }

    return GnssVSpeed.result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's direction. Direction of movement is the direction that the vehicle or
 * person is actually moving.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to UINT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note Direction is given in degrees with 1 decimal place: 1755 = 175,5 degrees.
 *       Direction ranges from 0 to 359.9 degrees, where 0 is True North.
 *
 * @note directionPtr, directionAccuracyPtr can be set to NULL if not needed.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetDirection
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint32_t* directionPtr,
        ///< [OUT]
        ///< Direction in degrees [resolution 1e-1].
        ///< Range: 0 to 359.9, where 0 is True North.

    uint32_t* directionAccuracyPtr
        ///< [OUT]
        ///< Direction's accuracy estimate in degrees [resolution 1e-1].
)
{
    if (directionPtr)
    {
        *directionPtr = GnssDirection.direction;
    }

    if (directionAccuracyPtr)
    {
        *directionAccuracyPtr = GnssDirection.accuracy;
    }

    return GnssDirection.result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the Satellites Vehicle information.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid.
 *  - LE_OK            Function succeeded.
 *
 * @note satId[] can be set to 0 if that information list index is not configured, so
 * all satellite parameters (satConst[], satSnr[],satAzim[], satElev[]) are fixed to 0.
 *
 * @note For LE_OUT_OF_RANGE returned code, invalid value depends on field type:
 * UINT16_MAX for satId, LE_GNSS_SV_CONSTELLATION_UNDEFINED for satConst, false for satUsed,
 * UINT8_MAX for satSnr, UINT16_MAX for satAzim, UINT8_MAX for satElev.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetSatellitesInfo
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint16_t* satIdPtr,
        ///< [OUT]
        ///< Satellites in View ID number, referring
        ///< to NMEA standard.

    size_t* satIdNumElementsPtr,
        ///< [INOUT]

    le_gnss_Constellation_t* satConstPtr,
        ///< [OUT]
        ///< GNSS constellation type.

    size_t* satConstNumElementsPtr,
        ///< [INOUT]

    bool* satUsedPtr,
        ///< [OUT]
        ///< TRUE if satellite in View Used
        ///< for Navigation.

    size_t* satUsedNumElementsPtr,
        ///< [INOUT]

    uint8_t* satSnrPtr,
        ///< [OUT]
        ///< Satellites in View Signal To Noise Ratio
        ///< [dBHz].

    size_t* satSnrNumElementsPtr,
        ///< [INOUT]

    uint16_t* satAzimPtr,
        ///< [OUT]
        ///< Satellites in View Azimuth [degrees].
        ///< Range: 0 to 360
        ///< If Azimuth angle
        ///< is currently unknown, the value is
        ///< set to UINT16_MAX.

    size_t* satAzimNumElementsPtr,
        ///< [INOUT]

    uint8_t* satElevPtr,
        ///< [OUT]
        ///< Satellites in View Elevation [degrees].
        ///< Range: 0 to 90
        ///< If Elevation angle
        ///< is currently unknown, the value is
        ///< set to UINT8_MAX.

    size_t* satElevNumElementsPtr
        ///< [INOUT]
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the SBAS constellation category
 *
 */
//--------------------------------------------------------------------------------------------------
le_gnss_SbasConstellationCategory_t le_gnss_GetSbasConstellationCategory
(
    uint16_t  satId      ///< [IN] Satellites in View ID number, referring to NMEA standard.
)
{
    // TODO: implement this function
    return LE_GNSS_SBAS_UNKNOWN;
};

//--------------------------------------------------------------------------------------------------
/**
 * Get the Satellites Vehicle status.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid.
 *  - LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetSatellitesStatus
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint8_t* satsInViewCountPtr,
        ///< [OUT]
        ///< Number of satellites expected to be in view.

    uint8_t* satsTrackingCountPtr,
        ///< [OUT]
        ///< Number of satellites in view, when tracking.

    uint8_t* satsUsedCountPtr
        ///< [OUT]
        ///< Number of satellites in view used for Navigation.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the DOP parameters (Dilution Of Precision) for the fixed position
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to UINT16_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @deprecated This function is deprecated, le_gnss_GetDilutionOfPrecision() should be used for
 *             new code.
 *
 * @note The DOP values are given with 3 decimal places like: DOP value 2200 = 2.200
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetDop
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    uint16_t* hdopPtr,
        ///< [OUT]
        ///< Horizontal Dilution of Precision [resolution 1e-3].

    uint16_t* vdopPtr,
        ///< [OUT]
        ///< Vertical Dilution of Precision [resolution 1e-3].

    uint16_t* pdopPtr
        ///< [OUT]
        ///< Position Dilution of Precision [resolution 1e-3].
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the DOP parameter (Dilution Of Precision) for the fixed position.
 *
 * @return
 *  - LE_FAULT         Function failed to find the DOP value.
 *  - LE_OUT_OF_RANGE  The retrieved parameter is invalid (set to INT16_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note This function replaces the deprecated function le_gnss_GetDop().
 *
 * @note The DOP value is given with 3 decimal places like: DOP value 2200 = 2.200
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetDilutionOfPrecision
(
    le_gnss_SampleRef_t positionSampleRef,      ///< [IN] Position sample's reference.
    le_gnss_DopType_t dopType,                  ///< [IN] Dilution of Precision type.
    uint16_t* dopPtr                            ///< [OUT] Dilution of Precision corresponding to
                                                ///< the dopType. [resolution 1e-3].
)
{
    if (NULL == dopPtr)
    {
        return LE_OUT_OF_RANGE;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's altitude with respect to the WGS-84 ellipsoid
 *
 * @return
 *  - LE_FAULT         Function failed to get the altitude.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note altitudeOnWgs84 is in meters, between WGS-84 earth ellipsoid and mean sea level
 *       with 3 decimal places (3047 = 3.047 meters).
 *
 * @note For a 2D position fix, the altitude with respect to the WGS-84 ellipsoid will be indicated
 *       as invalid and set to INT32_MAX.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetAltitudeOnWgs84
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* altitudeOnWgs84Ptr
        ///< [OUT] Altitude in meters, between WGS-84 earth ellipsoid
        ///<       and mean sea level [resolution 1e-3].
)
{
    if (NULL == altitudeOnWgs84Ptr)
    {
        LE_KILL_CLIENT("altitudeOnWgs84Ptr is NULL !");
        return LE_FAULT;
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's magnetic deviation. It is the difference between the bearing to
 * true north and the bearing shown on a magnetic compass. The deviation is positive when the
 * magnetic north is east of true north.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
 *
 * @note magneticDeviation is in degrees, with 1 decimal places (47 = 4.7 degree).
 *
 * @note If the caller is passing an invalid Position sample reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetMagneticDeviation
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* magneticDeviationPtr
        ///< [OUT] MagneticDeviation in degrees [resolution 1e-1].
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function gets the last updated position sample object reference.
 *
 * @return A reference to last Position's sample.
 *
 * @note
 *      On failure, the process exits, so you don't have to worry about checking the returned
 *      reference for validity.
 */
//--------------------------------------------------------------------------------------------------
le_gnss_SampleRef_t le_gnss_GetLastSampleRef
(
    void
)
{
    return Sample;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release the position sample.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_gnss_ReleaseSampleRef
(
    le_gnss_SampleRef_t    positionSampleRef    ///< [IN] The position sample's reference.
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the GNSS constellation bit mask
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_UNSUPPORTED   If the request is not supported.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized, disabled or active.
 *  - LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetConstellation
(
    le_gnss_ConstellationBitMask_t constellationMask  ///< [IN] GNSS constellation used in solution.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the GNSS constellation bit mask
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetConstellation
(
    le_gnss_ConstellationBitMask_t *constellationMaskPtr ///< [OUT] GNSS constellation used in
                                                         ///< solution
)
{
    if (NULL == constellationMaskPtr)
    {
        LE_KILL_CLIENT("constellationMaskPtr is NULL !");
        return LE_FAULT;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the area for the GNSS constellation
 *
 * @return
 *  - LE_OK            The function succeeded.
 *  - LE_FAULT         The function failed.
 *  - LE_UNSUPPORTED   If the request is not supported.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized, disabled or active.
 *  - LE_BAD_PARAMETER Invalid constellation area.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetConstellationArea
(
    le_gnss_Constellation_t satConstellation,       ///< [IN] GNSS constellation used in solution.
    le_gnss_ConstellationArea_t constellationArea   ///< [IN] GNSS constellation area.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the area for the GNSS constellation
 *
 * @return
 *  - LE_OK            On success
 *  - LE_FAULT         On failure
 *  - LE_UNSUPPORTED   Request not supported
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized, disabled or active.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetConstellationArea
(
    le_gnss_Constellation_t satConstellation,         ///< [IN] GNSS constellation used in solution.
    le_gnss_ConstellationArea_t* constellationAreaPtr ///< [OUT] GNSS constellation area.
)
{
    if (NULL == constellationAreaPtr)
    {
        LE_KILL_CLIENT("constellationAreaPtr is NULL !");
        return LE_FAULT;
    }
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function enables the use of the 'Extended Ephemeris' file into the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed to enable the 'Extended Ephemeris' file.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_EnableExtendedEphemerisFile
(
    void
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function disables the use of the 'Extended Ephemeris' file into the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed to disable the 'Extended Ephemeris' file.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_DisableExtendedEphemerisFile
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to load an 'Extended Ephemeris' file into the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed to inject the 'Extended Ephemeris' file.
 *  - LE_TIMEOUT       A time-out occurred.
 *  - LE_FORMAT_ERROR  'Extended Ephemeris' file format error.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_LoadExtendedEphemerisFile
(
    int32_t       fd      ///< [IN] extended ephemeris file descriptor
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the validity of the last injected Extended Ephemeris.
 *
 * @return
 *  - LE_FAULT         The function failed to get the validity
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetExtendedEphemerisValidity
(
    uint64_t *startTimePtr,    ///< [OUT] Start time in seconds (since Jan. 1, 1970)
    uint64_t *stopTimePtr      ///< [OUT] Stop time in seconds (since Jan. 1, 1970)
)
{
    if (NULL == startTimePtr)
    {
        LE_KILL_CLIENT("startTimePtr is NULL !");
        return LE_FAULT;
    }
    if (NULL == stopTimePtr)
    {
        LE_KILL_CLIENT("stopTimePtr is NULL !");
        return LE_FAULT;
    }

    *startTimePtr = 1480349409;
    *stopTimePtr = 1480349444;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to inject the UTC time into the GNSS device.
 *
 * @return
 *  - LE_OK            The function succeeded.
 *  - LE_FAULT         The function failed to inject the UTC time.
 *  - LE_TIMEOUT       A time-out occurred.
 *
 * @note It is mandatory to enable the 'Extended Ephemeris' file injection into the GNSS device with
 * le_gnss_EnableExtendedEphemerisFile() before injecting time with this API.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_InjectUtcTime
(
    uint64_t timeUtc,   ///< [IN] UTC time since Jan. 1, 1970 in milliseconds
    uint32_t timeUnc    ///< [IN] Time uncertainty in milliseconds
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function starts the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_DUPLICATE     If the GNSS device is already started.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized or disabled.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_Start
(
    void
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function stops the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_DUPLICATE     If the GNSS device is already stopped.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized or disabled.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_Stop
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function performs a "HOT" restart of the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_ForceHotRestart
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function performs a "WARM" restart of the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_ForceWarmRestart
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function performs a "COLD" restart of the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_ForceColdRestart
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function performs a "FACTORY" restart of the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_ForceFactoryRestart
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the TTFF in milliseconds
 *
 * @return
 *  - LE_BUSY          The position is not fixed and TTFF can't be measured.
 *  - LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 *  - LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetTtff
(
    uint32_t* ttffPtr     ///< [OUT] TTFF in milliseconds
)
{
    if (NULL == ttffPtr)
    {
        LE_KILL_CLIENT("ttffPtr is NULL !");
        return LE_FAULT;
    }
    *ttffPtr = 1000;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function enables the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_DUPLICATE     If the GNSS device is already enabled.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_Enable
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function disables the GNSS device.
 *
 * @return
 *  - LE_FAULT         The function failed.
 *  - LE_DUPLICATE     If the GNSS device is already disabled.
 *  - LE_NOT_PERMITTED If the GNSS device is not initialized or started.
 *  - LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_Disable
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the GNSS device acquisition rate.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED request not supported
 *  - LE_TIMEOUT a time-out occurred
 *  - LE_NOT_PERMITTED If the GNSS device is not in "ready" state.
 *
 * @warning This function may be subject to limitations depending on the platform. Please refer to
 *          the @ref platformConstraintsGnss page.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetAcquisitionRate
(
    uint32_t  rate      ///< Acquisition rate in milliseconds.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the GNSS device acquisition rate.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_NOT_PERMITTED If the GNSS device is not in "ready" state.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetAcquisitionRate
(
    uint32_t* ratePtr      ///< Acquisition rate in milliseconds.
)
{
    if (NULL == ratePtr)
    {
        LE_KILL_CLIENT("ratePtr is NULL !");
        return LE_FAULT;
    }
    *ratePtr = 1000;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the SUPL Assisted-GNSS mode.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED request not supported
 *  - LE_TIMEOUT a time-out occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetSuplAssistedMode
(
    le_gnss_AssistedMode_t  assistedMode      ///< [IN] Assisted-GNSS mode.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the SUPL Assisted-GNSS mode.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetSuplAssistedMode
(
    le_gnss_AssistedMode_t* assistedModePtr      ///< [OUT] Assisted-GNSS mode.
)
{
    if (NULL == assistedModePtr)
    {
        LE_KILL_CLIENT("assistedModePtr is NULL !");
        return LE_FAULT;
    }

    *assistedModePtr = LE_GNSS_STANDALONE_MODE;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the SUPL server URL.
 * That server URL is a NULL-terminated string with a maximum string length (including NULL
 * terminator) equal to 256. Optionally the port number is specified after a colon.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_BUSY service is busy
 *  - LE_TIMEOUT a time-out occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetSuplServerUrl
(
    const char*  suplServerUrlPtr      ///< [IN] SUPL server URL.
)
{
    if (NULL == suplServerUrlPtr)
    {
        LE_KILL_CLIENT("suplServerUrlPtr is NULL !");
        return LE_FAULT;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function injects the SUPL certificate to be used in A-GNSS sessions.
 *
 * @return
 *  - LE_OK on success
 *  - LE_BAD_PARAMETER on invalid parameter
 *  - LE_FAULT on failure
 *  - LE_BUSY service is busy
 *  - LE_TIMEOUT a time-out occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_InjectSuplCertificate
(
    uint8_t  suplCertificateId,      ///< [IN] ID of the SUPL certificate.
                                     ///< Certificate ID range is 0 to 9
    uint16_t suplCertificateLen,     ///< [IN] SUPL certificate size in Bytes.
    const char*  suplCertificatePtr  ///< [IN] SUPL certificate contents.
)
{
    if (NULL == suplCertificatePtr)
    {
        LE_KILL_CLIENT("suplCertificatePtr is NULL !");
        return LE_BAD_PARAMETER;
    }
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes the SUPL certificate.
 *
 * @return
 *  - LE_OK on success
 *  - LE_BAD_PARAMETER on invalid parameter
 *  - LE_FAULT on failure
 *  - LE_BUSY service is busy
 *  - LE_TIMEOUT a time-out occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_DeleteSuplCertificate
(
    uint8_t  suplCertificateId  ///< [IN]  ID of the SUPL certificate.
                                ///< Certificate ID range is 0 to 9
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the enabled NMEA sentences using a bit mask.
 *
 * @return
 *  - LE_OK             Success
 *  - LE_BAD_PARAMETER  Bit mask exceeds the maximal value
 *  - LE_FAULT          Failure
 *  - LE_BUSY           Service is busy
 *  - LE_TIMEOUT        Timeout occurred
 *  - LE_NOT_PERMITTED  GNSS device is not in "ready" state
 *
 * @warning This function may be subject to limitations depending on the platform. Please refer to
 *          the @ref platformConstraintsGnss page.
 *
 * @warning The settings are platform dependent. Please refer to
 *          @ref platformConstraintsGnss_SettingConfiguration section for full details.
 *
 * @deprecated LE_GNSS_NMEA_MASK_PQXFI is deprecated. LE_GNSS_NMEA_MASK_PTYPE should be used
 *             instead. Setting LE_GNSS_NMEA_MASK_PTYPE will also set LE_GNSS_NMEA_MASK_PQXFI.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetNmeaSentences
(
    le_gnss_NmeaBitMask_t nmeaMask ///< [IN] Bit mask for enabled NMEA sentences.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the bit mask for the enabled NMEA sentences.
 *
 * @return
 *  - LE_OK             Success
 *  - LE_FAULT          Failure
 *  - LE_BUSY           Service is busy
 *  - LE_TIMEOUT        Timeout occurred
 *  - LE_NOT_PERMITTED  GNSS device is not in "ready" state
 *
 * @note If the caller is passing an null pointer to this function, it is a fatal error
 *       and the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetNmeaSentences
(
    le_gnss_NmeaBitMask_t* nmeaMaskPtr ///< [OUT] Bit mask for enabled NMEA sentences.
)
{
    if (NULL == nmeaMaskPtr)
    {
        LE_KILL_CLIENT("nmeaMaskPtr is NULL !");
        return LE_FAULT;
    }

    *nmeaMaskPtr = LE_GNSS_NMEA_MASK_GPGGA;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference stub
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_gnss_GetServiceRef
(
    void
)
{
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference stub
 *
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_gnss_GetClientSessionRef
(
    void
)
{
    return NULL;
}
