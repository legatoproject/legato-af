//--------------------------------------------------------------------------------------------------
/**
 * @file le_gnss_simu.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------


#ifndef _LE_GNSS_STUB_H
#define _LE_GNSS_STUB_H

#include "le_gnss_interface.h"

//--------------------------------------------------------------------------------------------------
/**
 * Simulated gnss
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * gnssSimuLocation_t: a structure that holds simulated location data
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    int32_t latitude;
    int32_t longitude;
    int32_t accuracy;
    le_result_t result;
}
gnssSimuLocation_t;

//--------------------------------------------------------------------------------------------------
/**
 * gnssSimuAltitude_t: a structure that holds simulated altitude data
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    int32_t altitude;
    int32_t accuracy;
    le_result_t result;
}
gnssSimuAltitude_t;

//--------------------------------------------------------------------------------------------------
/**
 * gnssSimuDate_t: a structure that holds simulated date data
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint16_t year;
    uint16_t month;
    uint16_t day;
    le_result_t result;
}
gnssSimuDate_t;

//--------------------------------------------------------------------------------------------------
/**
 * gnssSimuDirection_t: a structure that holds simulated direction data
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint32_t direction;
    uint32_t accuracy;
    le_result_t result;
}
gnssSimuDirection_t;

//--------------------------------------------------------------------------------------------------
/**
 * gnssSimuHSpeed_t: a structure that holds simulated horizontal speed data
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint32_t speed;
    uint32_t accuracy;
    le_result_t result;
}
gnssSimuHSpeed_t;

//--------------------------------------------------------------------------------------------------
/**
 * gnssSimuVSpeed_t: a structure that holds simulated vertical speed data
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    int32_t speed;
    int32_t accuracy;
    le_result_t result;
}
gnssSimuVSpeed_t;

//--------------------------------------------------------------------------------------------------
/**
 * gnssSimuDop_t: a structure that holds simulated DOP data
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint32_t pDop;
    uint32_t hDop;
    uint32_t vDop;
    uint32_t gDop;
    uint32_t tDop;
    le_result_t result;
}
gnssSimuDop_t;

//--------------------------------------------------------------------------------------------------
/**
 * gnssSimuTime_t: a structure that holds simulated time data
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint16_t hrs;
    uint16_t min;
    uint16_t sec;
    uint16_t msec;
    le_result_t result;
}
gnssSimuTime_t;

//--------------------------------------------------------------------------------------------------
/**
 * gnssSimuPositionState_t: a structure that holds simulated position state data
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    le_gnss_FixState_t state;
    le_result_t result;
}
gnssSimuPositionState_t;

void le_gnssSimu_SetLocation(gnssSimuLocation_t gnssLocation);
void le_gnssSimu_SetAltitude(gnssSimuAltitude_t gnssAltitude);
void le_gnssSimu_SetDate(gnssSimuDate_t gnssDate);
void le_gnssSimu_SetDirection(gnssSimuDirection_t gnssDirection);
void le_gnssSimu_SetHSpeed(gnssSimuHSpeed_t gnssHSpeed);
void le_gnssSimu_SetVSpeed(gnssSimuVSpeed_t gnssVSpeed);
void le_gnssSimu_SetTime(gnssSimuTime_t gnssTime);
void le_gnssSimu_SetDop(gnssSimuDop_t dop);
void le_gnssSimu_SetSampleRef(le_gnss_SampleRef_t sample);
void le_gnssSimu_SetPositionState(gnssSimuPositionState_t state);
void le_gnssSimu_ReportEvent(void);         ///to report the event for handler

//--------------------------------------------------------------------------------------------------
/**
 * Satellite Vehicle information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint16_t                satId;         ///< Satellite in View ID number [PRN].
    le_gnss_Constellation_t satConst;      ///< GNSS constellation type.
    bool                    satUsed;       ///< TRUE if satellite in View Used for Navigation.
    bool                    satTracked;    ///< TRUE if satellite in View is tracked for Navigation.
    uint8_t                 satSnr;        ///< Satellite in View Signal To Noise Ratio [dBHz].
    uint16_t                satAzim;       ///< Satellite in View Azimuth [degrees].
                                           ///< Range: 0 to 360
    uint8_t                 satElev;       ///< Satellite in View Elevation [degrees].
                                           ///< Range: 0 to 90
}
le_gnss_SvInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Satellite measurement information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint16_t satId;          ///< Satellite in View ID number.
    int32_t  satLatency;     ///< Satellite latency measurement (age of measurement)
                             ///< Units: Milliseconds.
}
le_gnss_SvMeas_t;

//--------------------------------------------------------------------------------------------------
/**
 * Position Sample structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_gnss_PositionSample
{
    le_gnss_FixState_t fixState;        ///< Position Fix state
    bool               latitudeValid;   ///< if true, latitude is set
    int32_t            latitude;        ///< altitude
    bool               longitudeValid;  ///< if true, longitude is set
    int32_t            longitude;       ///< longitude
    bool               hAccuracyValid;  ///< if true, horizontal accuracy is set
    int32_t            hAccuracy;       ///< horizontal accuracy
    bool               altitudeValid;   ///< if true, altitude is set
    int32_t            altitude;        ///< altitude
    bool               altitudeAssumedValid; ///< if true, altitude assumed is set
    bool               altitudeAssumed;  ///< if false, the altitude is calculated
                                         ///< if true the altitude is assumed.
    bool               altitudeOnWgs84Valid;///< if true, altitude with respect to the WGS-84 is set
    int32_t            altitudeOnWgs84; ///< altitude with respect to the WGS-84 ellipsoid
    bool               vAccuracyValid;  ///< if true, vertical accuracy is set
    int32_t            vAccuracy;       ///< vertical accuracy
    bool               hSpeedValid;     ///< if true, horizontal speed is set
    uint32_t           hSpeed;          ///< horizontal speed
    bool               hSpeedAccuracyValid; ///< if true, horizontal speed accuracy is set
    int32_t            hSpeedAccuracy;  ///< horizontal speed accuracy
    bool               vSpeedValid;     ///< if true, vertical speed is set
    int32_t            vSpeed;          ///< vertical speed
    bool               vSpeedAccuracyValid; ///< if true, vertical speed accuracy is set
    int32_t            vSpeedAccuracy;  ///< vertical speed accuracy
    bool               directionValid;  ///< if true, direction is set
    uint32_t           direction;       ///< direction
    bool               directionAccuracyValid; ///< if true, direction accuracy is set
    uint32_t           directionAccuracy; ///< direction accuracy
    bool               dateValid;       ///< if true, date is set
    uint16_t           year;            ///< UTC Year A.D. [e.g. 2014].
    uint16_t           month;           ///< UTC Month into the year [range 1...12].
    uint16_t           day;             ///< UTC Days into the month [range 1...31].
    bool               timeValid;       ///< if true, time is set
    uint16_t           hours;           ///< UTC Hours into the day [range 0..23].
    uint16_t           minutes;         ///< UTC Minutes into the hour [range 0..59].
    uint16_t           seconds;         ///< UTC Seconds into the minute [range 0..59].
    uint16_t           milliseconds;    ///< UTC Milliseconds into the second [range 0..999].
    bool               gpsTimeValid;    ///< if true, GPS time is set
    uint32_t           gpsWeek;         ///< GPS week number from midnight, Jan. 6, 1980.
    uint32_t           gpsTimeOfWeek;   ///< Amount of time in milliseconds into the GPS week.
    bool               timeAccuracyValid; ///< if true, timeAccuracy is set
    uint32_t           timeAccuracy;      ///< Estimated Accuracy for time in milliseconds
    bool               positionLatencyValid; ///< if true, positionLatency is set
    uint32_t           positionLatency;      ///< Position measurement latency in milliseconds
    bool               hdopValid;       ///< if true, horizontal dilution is set
    uint16_t           hdop;            ///< The horizontal dilution of precision (DOP)
    bool               vdopValid;       ///< if true, vertical dilution is set
    uint16_t           vdop;            ///< The vertical dilution of precision (DOP)
    bool               pdopValid;       ///< if true, position dilution is set
    uint16_t           pdop;            ///< The Position dilution of precision (DOP)
    bool               gdopValid;       ///< if true, geometric dilution is set
    uint16_t           gdop;            ///< The geometric dilution of precision (DOP)
    bool               tdopValid;       ///< if true, time dilution is set
    uint16_t           tdop;            ///< The time dilution of precision (DOP)
    bool               magneticDeviationValid; ///< if true, magnetic deviation is set
    int32_t            magneticDeviation;  ///< The magnetic deviation

    // Leap Seconds
    bool               leapSecondsValid;
    uint16_t           leapSeconds;

    // Epoch time
    uint16_t           epochTime;

    // Satellite Vehicles information
    bool               satsInViewCountValid;   ///< if true, satsInViewCount is set
    uint8_t            satsInViewCount;        ///< Satellites in View count.
    bool               satsTrackingCountValid; ///< if true, satsTrackingCount is set
    uint8_t            satsTrackingCount;      ///< Tracking satellites in View count.
    bool               satsUsedCountValid;     ///< if true, satsUsedCount is set
    uint8_t            satsUsedCount;          ///< Satellites in View used for Navigation.
    bool               satInfoValid;           ///< if true, satInfo is set
    le_gnss_SvInfo_t   satInfo[LE_GNSS_SV_INFO_MAX_LEN];
    bool               satMeasValid;           ///< if true, satMeas is set
    le_gnss_SvMeas_t   satMeas[LE_GNSS_SV_INFO_MAX_LEN];
                                               ///< Satellite Vehicle measurement information.
    le_dls_Link_t      link;                   ///< Object node link
}
le_gnss_PositionSample_t;

//--------------------------------------------------------------------------------------------------
/**
 * Position Sample's Handler structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_gnss_PositionHandler
{
    le_gnss_PositionHandlerFunc_t handlerFuncPtr;      ///< The handler function address.
    void*                         handlerContextPtr;   ///< The handler function context.
    le_msg_SessionRef_t           sessionRef;          ///< Store message session reference.
    le_dls_Link_t                 link;                ///< Object node link
}
le_gnss_PositionHandler_t;

le_result_t gnss_Init
(
    void
);

le_gnss_PositionHandlerRef_t le_gnss_AddPositionHandler
(
    le_gnss_PositionHandlerFunc_t handlerPtr,          ///< [IN] The handler function.
    void*                        contextPtr           ///< [IN] The context pointer
);

void le_gnss_RemovePositionHandler
(
    le_gnss_PositionHandlerRef_t    handlerRef ///< [IN] The handler reference.
);

le_result_t le_gnss_GetPositionState
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN]
        ///< Position sample's reference.

    le_gnss_FixState_t* statePtr
        ///< [OUT]
        ///< Position fix state.
);

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
);

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
);

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
);

le_result_t le_gnss_GetGpsTime
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* gpsWeekPtr,
        ///< [OUT] GPS week number from midnight, Jan. 6, 1980.

    uint32_t* gpsTimeOfWeekPtr
        ///< [OUT] Amount of time in milliseconds into the GPS week.
);

le_result_t le_gnss_GetTimeAccuracy
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* timeAccuracyPtr
        ///< [OUT] Estimated time accuracy in nanoseconds
);

le_result_t le_gnss_GetGpsLeapSeconds
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint8_t* leapSecondsPtr
        ///< [OUT] UTC leap seconds in advance in seconds
);

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
);

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
);

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
);

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
);

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
);

le_gnss_SbasConstellationCategory_t le_gnss_GetSbasConstellationCategory
(
    uint16_t  satId      ///< [IN] Satellites in View ID number, referring to NMEA standard.
);

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
);

le_result_t le_gnss_GetDilutionOfPrecision
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    le_gnss_DopType_t dopType,
        ///< [IN] Dilution of Precision type.

    uint16_t* dopPtr
        ///< [OUT] Dilution of Precision corresponding to
        ///<       the dopType. [resolution 1e-3].
);

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
);

le_result_t le_gnss_GetAltitudeOnWgs84
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* altitudeOnWgs84Ptr
        ///< [OUT] Altitude in meters, between WGS-84 earth ellipsoid
        ///<       and mean sea level [resolution 1e-3].
);

le_result_t le_gnss_GetMagneticDeviation
(
    le_gnss_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* magneticDeviationPtr
        ///< [OUT] MagneticDeviation in degrees [resolution 1e-1].
);

le_gnss_SampleRef_t le_gnss_GetLastSampleRef
(
    void
);

void le_gnss_ReleaseSampleRef
(
    le_gnss_SampleRef_t    positionSampleRef    ///< [IN] The position sample's reference.
);

le_result_t le_gnss_SetConstellation
(
    le_gnss_ConstellationBitMask_t constellationMask  ///< [IN] GNSS constellation used in solution.
);

le_result_t le_gnss_GetConstellation
(
    le_gnss_ConstellationBitMask_t *constellationMaskPtr ///< [OUT] GNSS constellation used in
                                                         ///< solution
);

le_result_t le_gnss_EnableExtendedEphemerisFile
(
    void
);

le_result_t le_gnss_DisableExtendedEphemerisFile
(
    void
);

le_result_t le_gnss_LoadExtendedEphemerisFile
(
    int32_t       fd      ///< [IN] extended ephemeris file descriptor
);

le_result_t le_gnss_GetExtendedEphemerisValidity
(
    uint64_t *startTimePtr,    ///< [OUT] Start time in seconds (since Jan. 1, 1970)
    uint64_t *stopTimePtr      ///< [OUT] Stop time in seconds (since Jan. 1, 1970)
);

le_result_t le_gnss_InjectUtcTime
(
    uint64_t timeUtc,   ///< [IN] UTC time since Jan. 1, 1970 in milliseconds
    uint32_t timeUnc    ///< [IN] Time uncertainty in milliseconds
);

le_result_t le_gnss_Start
(
    void
);

le_result_t le_gnss_Stop
(
    void
);

le_result_t le_gnss_ForceHotRestart
(
    void
);

le_result_t le_gnss_ForceWarmRestart
(
    void
);

le_result_t le_gnss_ForceColdRestart
(
    void
);

le_result_t le_gnss_ForceFactoryRestart
(
    void
);

le_result_t le_gnss_GetTtff
(
    uint32_t* ttffPtr     ///< [OUT] TTFF in milliseconds
);

le_result_t le_gnss_Enable
(
    void
);

le_result_t le_gnss_Disable
(
    void
);

le_result_t le_gnss_SetAcquisitionRate
(
    uint32_t  rate      ///< Acquisition rate in milliseconds.
);

le_result_t le_gnss_GetAcquisitionRate
(
    uint32_t* ratePtr      ///< Acquisition rate in milliseconds.
);

le_result_t le_gnss_SetSuplAssistedMode
(
    le_gnss_AssistedMode_t  assistedMode      ///< [IN] Assisted-GNSS mode.
);

le_result_t le_gnss_GetSuplAssistedMode
(
    le_gnss_AssistedMode_t * assistedModePtr      ///< [OUT] Assisted-GNSS mode.
);

le_result_t le_gnss_SetSuplServerUrl
(
    const char*  suplServerUrlPtr      ///< [IN] SUPL server URL.
);

le_result_t le_gnss_InjectSuplCertificate
(
    uint8_t  suplCertificateId,      ///< [IN] ID of the SUPL certificate.
                                     ///< Certificate ID range is 0 to 9
    uint16_t suplCertificateLen,     ///< [IN] SUPL certificate size in Bytes.
    const char*  suplCertificatePtr  ///< [IN] SUPL certificate contents.
);

le_result_t le_gnss_DeleteSuplCertificate
(
    uint8_t  suplCertificateId  ///< [IN]  ID of the SUPL certificate.
                                ///< Certificate ID range is 0 to 9
);

le_result_t le_gnss_SetNmeaSentences
(
    le_gnss_NmeaBitMask_t nmeaMask ///< [IN] Bit mask for enabled NMEA sentences.
);

le_result_t le_gnss_GetNmeaSentences
(
    le_gnss_NmeaBitMask_t* nmeaMaskPtr ///< [OUT] Bit mask for enabled NMEA sentences.
);

le_msg_ServiceRef_t le_gnss_GetServiceRef
(
    void
);

le_msg_SessionRef_t le_gnss_GetClientSessionRef
(
    void
);

#endif /* le_gnss_stub.h */
