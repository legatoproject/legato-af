//--------------------------------------------------------------------------------------------------
/**
 * @file le_pos.c
 *
 * This file contains the source code of the high level Positioning API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------


#include "legato.h"
#include "interfaces.h"
#include "le_gnss_local.h"
#ifdef LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
#include "posCfgEntries.h"
#endif // LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
#include "watchdogChain.h"

#include <math.h>

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
// TODO move to configuration module? or retrieve it from a speedometer?
//--------------------------------------------------------------------------------------------------
#define SUPPOSED_AVERAGE_SPEED       50     // 50 km/h
#define DEFAULT_ACQUISITION_RATE     1000   // one second
#define DEFAULT_POWER_STATE          true


// To compute the estimated horizontal error, we assume that the GNSS's User Equivalent Range Error
// (UERE) is equivalent for all the satellites, for civil application UERE is approximatively 7
// meters.
#define GNSS_UERE                                 7
#define GNSS_ESTIMATED_VERTICAL_ERROR_FACTOR      (GNSS_UERE * 1.5)

#define POSITIONING_SAMPLE_MAX          1

/// Expected number of sample handlers
#define HIGH_POS_SAMPLE_HANDLER_COUNT   1

#define CHECK_VALIDITY(_par_,_max_) (((_par_) == (_max_))? false : true)

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 8

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration for time conversion
 */
//--------------------------------------------------------------------------------------------------
#define SEC_TO_MSEC            1000
#define HOURS_TO_SEC           3600

//--------------------------------------------------------------------------------------------------
/**
 * Default acquisition rate is 1 sample/sec
 */
//--------------------------------------------------------------------------------------------------
#define DEFAULT_ACQUISITION_RATE 1000

//--------------------------------------------------------------------------------------------------
/**
 * Count of the number of activation requests that have not been released yet.
 *
 * See le_posCtrl_Request().
 */
//--------------------------------------------------------------------------------------------------
static int CurrentActivationsCount = 0;

//--------------------------------------------------------------------------------------------------
/**
 *  Position data type.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    ALTITUDE,       ///< Altitude in meters, above Mean Sea Level
    H_ACCURACY,     ///< Horizontal position accuracy
    V_ACCURACY      ///< Vertical position accuracy
}
le_pos_DistanceValueType_t;


//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Position Sample structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_pos_Sample
{
    le_pos_FixState_t fixState;      ///< Position Fix state
    bool            latitudeValid;   ///< if true, latitude is set
    int32_t         latitude;        ///< altitude
    bool            longitudeValid;  ///< if true, longitude is set
    int32_t         longitude;       ///< longitude
    bool            hAccuracyValid;  ///< if true, horizontal accuracy is set
    int32_t         hAccuracy;       ///< horizontal accuracy
    bool            altitudeValid;   ///< if true, altitude is set
    int32_t         altitude;        ///< altitude
    bool            vAccuracyValid;  ///< if true, vertical accuracy is set
    int32_t         vAccuracy;       ///< vertical accuracy
    bool            hSpeedValid;     ///< if true, horizontal speed is set
    uint32_t        hSpeed;          ///< horizontal speed
    bool            hSpeedAccuracyValid; ///< if true, horizontal speed accuracy is set
    uint32_t        hSpeedAccuracy;  ///< horizontal speed accuracy
    bool            vSpeedValid;     ///< if true, vertical speed is set
    int32_t         vSpeed;          ///< vertical speed
    bool            vSpeedAccuracyValid; ///< if true, vertical speed accuracy is set
    int32_t         vSpeedAccuracy;  ///< vertical speed accuracy
    bool            headingValid;    ///< if true, heading is set
    uint32_t        heading;         ///< heading
    bool            headingAccuracyValid; ///< if true, heading accuracy is set
    uint32_t        headingAccuracy; ///< heading accuracy
    bool            directionValid;  ///< if true, direction is set
    uint32_t        direction;       ///< direction
    bool            directionAccuracyValid; ///< if true, direction accuracy is set
    uint32_t        directionAccuracy; ///< direction accuracy
    bool            dateValid;          ///< if true, date is set
    uint16_t        year;               ///< UTC Year A.D. [e.g. 2014].
    uint16_t        month;              ///< UTC Month into the year [range 1...12].
    uint16_t        day;                ///< UTC Days into the month [range 1...31].
    bool            timeValid;          ///< if true, time is set
    uint16_t        hours;              ///< UTC Hours into the day [range 0..23].
    uint16_t        minutes;            ///< UTC Minutes into the hour [range 0..59].
    uint16_t        seconds;            ///< UTC Seconds into the minute [range 0..59].
    uint16_t        milliseconds;       ///< UTC Milliseconds into the second [range 0..999].
    bool            leapSecondsValid;   ///< if true, leapSeconds is set
    uint8_t         leapSeconds;        ///< UTC leap seconds in advance in seconds

    le_dls_Link_t   link;               ///< Object node link
}
le_pos_Sample_t;

//--------------------------------------------------------------------------------------------------
/**
 * Position Sample's Handler structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_pos_SampleHandler
{
    le_pos_MovementHandlerFunc_t handlerFuncPtr;      ///< The handler function address.
    void*                        handlerContextPtr;   ///< The handler function context.
    uint32_t                     acquisitionRate;     ///< The acquisition rate for this handler.
    uint32_t                     horizontalMagnitude; ///< The horizontal magnitude in meters for
                                                      ///  this handler.
    uint32_t                     verticalMagnitude;   ///< The vertical magnitude in meters for this
                                                      ///  handler.
    int32_t                      lastLat;             ///< The altitude associated with the last
                                                      ///  handler's notification.
    int32_t                      lastLong;            ///< The longitude associated with the last
                                                      ///  handler's notification.
    int32_t                      lastAlt;             ///< The altitude associated with the last
                                                      ///  handler's notification.
    le_msg_SessionRef_t          sessionRef;          ///< Store message session reference.
    le_dls_Link_t                link;                ///< Object node link
}
le_pos_SampleHandler_t;


//--------------------------------------------------------------------------------------------------
/**
 * Position control Client request object structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_posCtrl_ActivationRef_t      posCtrlActivationRef;   ///< Positioning control reference.
    le_msg_SessionRef_t             sessionRef;             ///< Client session identifier.
    le_dls_Link_t                   link;                   ///< Object node link.
}
ClientRequest_t;

//--------------------------------------------------------------------------------------------------
/**
 * Position sample request object structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_pos_SampleRef_t              positionSampleRef;  ///< Store position smaple reference.
    le_pos_Sample_t*                posSampleNodePtr;   ///< Position sample node pointer.
    le_msg_SessionRef_t             sessionRef;         ///< Client session identifier.
    le_dls_Link_t                   link;               ///< Object node link.
}
PosSampleRequest_t;

//--------------------------------------------------------------------------------------------------
/**
 * Position structure for move calculation.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
        int32_t  latitude;          ///< Latitude.
        int32_t  longitude;         ///< Longitude.
        int32_t  altitude;          ///< Altitude.
        int32_t  vAccuracy;         ///< Vertical accuracy.
        int32_t  hAccuracy;         ///< Horizontal accuracy.
        bool     locationValid;     ///< If true, location is set.
        bool     altitudeValid;     ///< If true, altitude is set.
}
PositionParam_t;

//--------------------------------------------------------------------------------------------------
/**
 * Static safe Reference Map for service activation requests.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(PositioningClient, LE_CONFIG_POSITIONING_ACTIVATION_MAX);

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for service activation requests.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t ActivationRequestRefMap;


//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Create and initialize the position samples list.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t PosSampleList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Create and initialize the position sample's handlers list.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t PosSampleHandlerList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Static pool for position samples
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(PosSample, POSITIONING_SAMPLE_MAX, sizeof(le_pos_Sample_t));


//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for position samples.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   PosSamplePoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Static memory pool for position sample requests.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(PosSampleRequest,
                          POSITIONING_SAMPLE_MAX,
                          sizeof(PosSampleRequest_t));


//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for position sample request.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   PosSampleRequestPoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of expected sample handlers
 */
//--------------------------------------------------------------------------------------------------
#define HIGH_SAMPLE_HANDLER_COUNT       1


//--------------------------------------------------------------------------------------------------
/**
 * Define static pool for sample handlers
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(PosSampleHandler,
                          HIGH_SAMPLE_HANDLER_COUNT,
                          sizeof(le_pos_SampleHandler_t));


//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for position sample's handlers.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   PosSampleHandlerPoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Static safe Reference Map for Positioning Sample objects.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(PosSampleMap, POSITIONING_SAMPLE_MAX);


//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Positioning Sample objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t PosSampleMap;


//--------------------------------------------------------------------------------------------------
/**
 * Static pool for positioning client handlers
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(PosCtrlHandler,
                          LE_CONFIG_POSITIONING_ACTIVATION_MAX,
                          sizeof(ClientRequest_t));

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Positioning Client Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PosCtrlHandlerPoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Number of Handler functions that own position samples.
 *
 */
//--------------------------------------------------------------------------------------------------
static int32_t NumOfHandlers;

//--------------------------------------------------------------------------------------------------
/**
 * PA handler's reference.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_gnss_PositionHandlerRef_t GnssHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * The acquisition rate in milliseconds.
 */
//--------------------------------------------------------------------------------------------------
static uint32_t AcqRate = DEFAULT_ACQUISITION_RATE;

//--------------------------------------------------------------------------------------------------
/**
 * Verify GNSS device availability. TODO
 *
 */
//--------------------------------------------------------------------------------------------------
static bool IsGNSSAvailable
(
    void
)
{
    return true;
}

//--------------------------------------------------------------------------------------------------
/**
 * The resolution for the positioning distance parameters.
 */
//--------------------------------------------------------------------------------------------------
static le_pos_Resolution_t DistanceResolution = LE_POS_RES_METER;

//--------------------------------------------------------------------------------------------------
/**
 * Pos Sample destructor.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PosSampleDestructor
(
    void* obj
)
{
    le_pos_Sample_t *posSampleNodePtr;
    le_dls_Link_t   *linkPtr;

    LE_FATAL_IF((obj == NULL), "Position Sample Object does not exist!");

    linkPtr = le_dls_Peek(&PosSampleList);
    if (linkPtr != NULL)
    {
        do
        {
            // Get the node from the list
            posSampleNodePtr = (le_pos_Sample_t*)CONTAINER_OF(linkPtr, le_pos_Sample_t, link);
            // Check the node.
            if (posSampleNodePtr == (le_pos_Sample_t*)obj)
            {
                // Remove the node.
                le_dls_Remove(&PosSampleList, linkPtr);
                linkPtr=NULL;
            }
            else
            {
                // Move to the next node.
                linkPtr = le_dls_PeekNext(&PosSampleList, linkPtr);
            }
        } while (linkPtr != NULL);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Pos Sample's Handler destructor.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PosSampleHandlerDestructor
(
    void* obj
)
{
    le_pos_SampleHandler_t *posSampleHandlerNodePtr;
    le_dls_Link_t          *linkPtr;

    linkPtr = le_dls_Peek(&PosSampleHandlerList);
    if (linkPtr != NULL)
    {
        do
        {
            // Get the node from the list
            posSampleHandlerNodePtr = (le_pos_SampleHandler_t*)CONTAINER_OF(linkPtr,
                                                                            le_pos_SampleHandler_t,
                                                                            link);
            // Check the node.
            if (posSampleHandlerNodePtr == (le_pos_SampleHandler_t*)obj)
            {
                // Remove the node.
                le_dls_Remove(&PosSampleHandlerList, linkPtr);
                linkPtr=NULL;
            }
            else
            {
                // Move to the next node.
                linkPtr = le_dls_PeekNext(&PosSampleHandlerList, linkPtr);
            }
        } while (linkPtr != NULL);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Calculate the GNSS's Acquisistion rate in seconds.
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t CalculateAcquisitionRate
(
    uint32_t averageSpeed,         // km/h.
    uint32_t horizontalMagnitude,  // The horizontal magnitude in meters.
    uint32_t verticalMagnitude     // The vertical magnitude in meters.
)
{
    uint32_t  rate;
    uint32_t  metersec = averageSpeed * SEC_TO_MSEC/HOURS_TO_SEC;

    LE_DEBUG("metersec %"PRIu32" (m/sec), h_Magnitude %"PRIu32", v_Magnitude %"PRIu32"",
                  metersec,
                  horizontalMagnitude,
                  verticalMagnitude);

    if ((horizontalMagnitude < metersec) || (verticalMagnitude < metersec))
    {
        return 1;
    }

    rate = 0;
    while (! (((horizontalMagnitude >= (metersec + metersec*rate)) &&
               (verticalMagnitude >= (metersec + metersec*rate)))
             &&
             ((horizontalMagnitude < (metersec + metersec*(rate+1))) ||
              (verticalMagnitude < (metersec + metersec*(rate+1)))))
          )
    {
        rate++;
    }
    return rate + 2;
}

//--------------------------------------------------------------------------------------------------
/**
 * Calculate the distance in meters between two fix points (use Haversine formula).
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t ComputeDistance
(
    uint32_t latitude1,
    uint32_t longitude1,
    uint32_t latitude2,
    uint32_t longitude2
)
{
    // Haversine formula:
    // a = sin²(Δφ/2) + cos(φ1).cos(φ2).sin²(Δλ/2)
    // c = 2.atan2(√a, √(1−a))
    // distance = R.c.1000 (in meters)
    // where φ is latitude, λ is longitude, R is earth’s radius (mean radius = 6,371km)
    #define PI 3.14159265

    double R = 6371; // km
    double dLat = ((double)latitude2-(double)latitude1)/1000000.0*PI/180;
    double dLon = ((double)longitude2-(double)longitude1)/1000000.0*PI/180;
    double lat1 = ((double)latitude1)/1000000.0*PI/180;
    double lat2 = ((double)latitude2)/1000000.0*PI/180;
    double a, c;

    a = sin(dLat/2) * sin(dLat/2) + sin(dLon/2) * sin(dLon/2) * cos(lat1) * cos(lat2);
    c = 2 * atan2(sqrt(a), sqrt(1-a));

    LE_DEBUG("Computed distance is %e meters (double)", (double)(R * c * 1000));
    return (uint32_t)(R * c * 1000);
}

//--------------------------------------------------------------------------------------------------
/**
 * Verify if the covered distance is beyond the magnitude.
 *
 */
//--------------------------------------------------------------------------------------------------
static bool IsBeyondMagnitude
(
    uint32_t magnitude,
    uint32_t move,
    uint32_t accuracy
)
{
    if (move > magnitude)
    {
        // Check that it doesn't just look like we are beyond the magnitude because of bad accuracy.
        if ((accuracy > move) || ((move - accuracy) < magnitude))
        {
            // Could be beyond the magnitude, but we also could be inside the fence.
            return false;
        }
        else
        {
            // Definitely beyond the magnitude!
            return true;
        }
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Calculate the smallest acquisition rate to use for all the registered handlers.
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t ComputeCommonSmallestRate
(
    uint32_t rate
)
{
    le_pos_SampleHandler_t *posSampleHandlerNodePtr;
    le_dls_Link_t          *linkPtr;

    linkPtr = le_dls_Peek(&PosSampleHandlerList);
    if (linkPtr != NULL)
    {
        do
        {
            // Get the node from the list
            posSampleHandlerNodePtr = (le_pos_SampleHandler_t*)CONTAINER_OF(linkPtr,
                                                                            le_pos_SampleHandler_t,
                                                                            link);
            // Check the node.
            if (posSampleHandlerNodePtr->acquisitionRate < rate)
            {
                // Remove the node.
                rate = posSampleHandlerNodePtr->acquisitionRate;
                linkPtr=NULL;
            }
            else
            {
                // Move to the next node.
                linkPtr = le_dls_PeekNext(&PosSampleHandlerList, linkPtr);
            }
        } while (linkPtr != NULL);
    }

    return rate;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert the value in the selected resolution.
 *
 */
//--------------------------------------------------------------------------------------------------
static int32_t ConvertDistance
(
    int32_t value,
    le_pos_DistanceValueType_t type
)
{
    int32_t resValue = 0;
    switch(type)
    {
        case ALTITUDE:
            // altitude is got to the function in millimeters
            switch(DistanceResolution)
            {
                case LE_POS_RES_DECIMETER:
                    resValue = value/100;
                    break;
                case LE_POS_RES_CENTIMETER:
                    resValue = value/10;
                    break;
                case LE_POS_RES_MILLIMETER:
                    resValue = value;
                    break;
                case LE_POS_RES_METER:
                default:
                    // treat unknown resolution as meters by default
                    resValue = value/1000;
                    break;
            };
            break;

        case H_ACCURACY:
            // hAccuracy is got to the function in centimeters-
            switch(DistanceResolution)
            {
                case LE_POS_RES_DECIMETER:
                    resValue = value/10;
                    break;
                case LE_POS_RES_CENTIMETER:
                    resValue = value;
                    break;
                case LE_POS_RES_MILLIMETER:
                    resValue = value*10;
                    break;
                case LE_POS_RES_METER:
                default:
                    // treat unknown resolution as meters by default
                    resValue = value/100;
                    break;
           };
            break;

        case V_ACCURACY:
            // Vaccuracy in got to the function in decimeters.
            switch(DistanceResolution)
            {
                case LE_POS_RES_DECIMETER:
                    resValue = value;
                    break;
                case LE_POS_RES_CENTIMETER:
                    resValue = value*10;
                    break;
                case LE_POS_RES_MILLIMETER:
                    resValue = value*100;
                    break;
                case LE_POS_RES_METER:
                default:
                    // treat unknown resolution as meters by default
                    resValue = value/10;
                    break;
            };
            break;

        default:
          LE_ERROR("Wrong type");
          break;
    }

    return resValue;
}

//--------------------------------------------------------------------------------------------------
/**
 * Compute horizontal and vertical move
 *
 * @return LE_FAULT         Function failed.
 * @return LE_OK            Function succeeded.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ComputeMove
(
  le_pos_SampleHandler_t *posSampleHandlerNodePtr,  ///< [IN]  The handler reference.
  const PositionParam_t  *posParamPtr,              ///< [IN]  The position structure for the move
                                                    ///        calculation.
  bool                   *hflagPtr,                 ///< [OUT] True if the horizontal distance is
                                                    ///        beyond the magnitude.
  bool                   *vflagPtr                  ///< [OUT] True if the vertical distance is
)                                                   ///        beyond the magnitude.
{
    if (NULL == posSampleHandlerNodePtr)
    {
        LE_ERROR("posSampleHandlerNodePtr is Null");
        return LE_FAULT;
    }

    if ((posSampleHandlerNodePtr->horizontalMagnitude != 0) && !(posParamPtr->locationValid))
    {
        LE_ERROR("Longitude or Latitude are not relevant");
        return LE_FAULT;
    }

    if (((posSampleHandlerNodePtr->verticalMagnitude != 0) && (!posParamPtr->altitudeValid)))
    {
        LE_ERROR("Altitude is not relevant");
        return LE_FAULT;
    }

    // Compute horizontal and vertical move
    LE_DEBUG("Last Position lat.%"PRIi32", long.%"PRIi32,
                 posSampleHandlerNodePtr->lastLat, posSampleHandlerNodePtr->lastLong);

    // Save the current latitude values into lastLat.
    if (posSampleHandlerNodePtr->lastLat == 0)
    {
        posSampleHandlerNodePtr->lastLat = posParamPtr->latitude;
    }

    // Save the current longitude values into lastLong.
    if (posSampleHandlerNodePtr->lastLong == 0)
    {
        posSampleHandlerNodePtr->lastLong = posParamPtr->longitude;
    }

    // Save the current altitude values into lastAlt.
    if (posSampleHandlerNodePtr->lastAlt == 0)
    {
        posSampleHandlerNodePtr->lastAlt = posParamPtr->altitude;
    }

    uint32_t horizontalMove = ComputeDistance(posSampleHandlerNodePtr->lastLat,
                                              posSampleHandlerNodePtr->lastLong,
                                              posParamPtr->latitude,
                                              posParamPtr->longitude);

    uint32_t verticalMove = abs(posParamPtr->altitude - posSampleHandlerNodePtr->lastAlt);

    LE_DEBUG("horizontalMove.%"PRIu32", verticalMove.%"PRIu32, horizontalMove, verticalMove);

    if (INT32_MAX == posParamPtr->vAccuracy)
    {
        *vflagPtr = false;
    }
    else
    {
        // Vertical accuracy is in meters with 1 decimal place
        *vflagPtr = IsBeyondMagnitude(posSampleHandlerNodePtr->verticalMagnitude,
                                   verticalMove,
                                   posParamPtr->vAccuracy/10);
    }

    if (INT32_MAX == posParamPtr->hAccuracy)
    {
        *hflagPtr = false;
    }
    else
    {
        // Accuracy is in meters with 2 decimal places
        *hflagPtr = IsBeyondMagnitude(posSampleHandlerNodePtr->horizontalMagnitude,
                                   horizontalMove,
                                   posParamPtr->hAccuracy/100);
    }
    LE_DEBUG("Vertical IsBeyondMagnitude.%d", *vflagPtr);
    LE_DEBUG("Horizontal IsBeyondMagnitude.%d", *hflagPtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The main position Sample Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PosSampleHandlerfunc
(
    le_gnss_SampleRef_t positionSampleRef,
    void* contextPtr
)
{
    le_result_t result;
    // Location parameters
    bool        locationValid = false;
    int32_t     latitude;
    int32_t     longitude;
    int32_t     hAccuracy;
    bool        altitudeValid = false;
    int32_t     altitude;
    int32_t     vAccuracy;
    // Horizontal speed
    uint32_t hSpeed;
    uint32_t hSpeedAccuracy;
    // Vertical speed
    int32_t vSpeed;
    int32_t vSpeedAccuracy;
    // Direction
    uint32_t direction;
    uint32_t directionAccuracy;
    // Date parameters
    uint16_t year;
    uint16_t month;
    uint16_t day;
    // Time parameters
    uint16_t hours;
    uint16_t minutes;
    uint16_t seconds;
    uint16_t milliseconds;
    // Leap seconds in advance
    uint8_t leapSeconds;
    PositionParam_t posParam;
    // the position fix state
    le_gnss_FixState_t gnssState;

    // Positioning sample parameters
    le_pos_SampleHandler_t* posSampleHandlerNodePtr;
    le_dls_Link_t*          linkPtr;
    PosSampleRequest_t*     posSampleRequestPtr=NULL;

    if (NULL == positionSampleRef)
    {
        LE_ERROR("positionSampleRef is Null");
        return;
    }

    if (!NumOfHandlers)
    {
        LE_DEBUG("No positioning Sample handler, exit Handler Function");
        // Release provided Position sample reference
        le_gnss_ReleaseSampleRef(positionSampleRef);
        return;
    }

    LE_DEBUG("Handler Function called with sample %p", positionSampleRef);

    // Get Location
    result = le_gnss_GetLocation(positionSampleRef, &latitude, &longitude, &hAccuracy);
    if ((LE_OK == result) ||
        ((LE_OUT_OF_RANGE == result) && (INT32_MAX != latitude) && (INT32_MAX != longitude)))
    {
        locationValid = true;
        LE_DEBUG("Position lat.%"PRIi32", long.%"PRIi32", hAccuracy.%"PRIi32,
                 latitude, longitude, ConvertDistance(hAccuracy, H_ACCURACY));
    }
    else
    {
        locationValid = false;
        LE_DEBUG("Position unknown [%"PRIi32",%"PRIi32",%"PRIi32"]", latitude, longitude, hAccuracy);
    }

    // Get altitude
    result = le_gnss_GetAltitude(positionSampleRef, &altitude, &vAccuracy);

    if ((LE_OK == result) ||
        ((LE_OUT_OF_RANGE != result) && (INT32_MAX != altitude)))
    {
        altitudeValid = true;
        LE_DEBUG("Altitude.%"PRIi32", vAccuracy.%"PRIi32, ConvertDistance(altitude, ALTITUDE),
                                                          ConvertDistance(vAccuracy, V_ACCURACY));
    }
    else
    {
        altitudeValid = false;
        LE_DEBUG("Altitude unknown [%"PRIi32",%"PRIi32"]", altitude, vAccuracy);
    }

    // Positioning sample
    linkPtr = le_dls_Peek(&PosSampleHandlerList);

    if (NULL == linkPtr)
    {
        // Release provided Position sample reference
        le_gnss_ReleaseSampleRef(positionSampleRef);
        return;
    }

    posParam.latitude = latitude;
    posParam.longitude = longitude;
    posParam.altitude = altitude;
    posParam.vAccuracy = vAccuracy;
    posParam.hAccuracy = hAccuracy;
    posParam.locationValid = locationValid;
    posParam.altitudeValid = altitudeValid;

    do
    {
        bool hflag, vflag;
        // Get the node from the list
        posSampleHandlerNodePtr = (le_pos_SampleHandler_t*)CONTAINER_OF(linkPtr,
                                                                        le_pos_SampleHandler_t,
                                                                        link);

        if (LE_FAULT == ComputeMove(posSampleHandlerNodePtr, &posParam, &hflag, &vflag))
        {
            // Release provided Position sample reference
            le_gnss_ReleaseSampleRef(positionSampleRef);
            return;
        }

        // Movement is detected in the following cases:
        // - Vertical distance is beyond the magnitude
        // - Horizontal distance is beyond the magnitude
        // - We don't care about vertical & horizontal distance (magnitudes equal to 0)
        //   therefore that movement handler is called each positioning acquisition rate
        if (((0 != posSampleHandlerNodePtr->verticalMagnitude) && (vflag)) ||
             ((0 != posSampleHandlerNodePtr->horizontalMagnitude) && (hflag)) ||
             ((0 == posSampleHandlerNodePtr->verticalMagnitude)
             && (0 == posSampleHandlerNodePtr->horizontalMagnitude)))
        {
            // Create the position sample node.
            posSampleRequestPtr = le_mem_ForceAlloc(PosSampleRequestPoolRef);
            posSampleRequestPtr->posSampleNodePtr
                                = (le_pos_Sample_t*)le_mem_ForceAlloc(PosSamplePoolRef);
            posSampleRequestPtr->posSampleNodePtr->latitudeValid
                                = CHECK_VALIDITY(latitude,INT32_MAX);
            posSampleRequestPtr->posSampleNodePtr->latitude = latitude;

            posSampleRequestPtr->posSampleNodePtr->longitudeValid
                                = CHECK_VALIDITY(longitude,INT32_MAX);
            posSampleRequestPtr->posSampleNodePtr->longitude = longitude;

            posSampleRequestPtr->posSampleNodePtr->hAccuracyValid
                                = CHECK_VALIDITY(hAccuracy,INT32_MAX);
            posSampleRequestPtr->posSampleNodePtr->hAccuracy = hAccuracy;

            posSampleRequestPtr->posSampleNodePtr->altitudeValid
                                = CHECK_VALIDITY(altitude,INT32_MAX);
            posSampleRequestPtr->posSampleNodePtr->altitude = altitude;

            posSampleRequestPtr->posSampleNodePtr->vAccuracyValid
                                = CHECK_VALIDITY(vAccuracy,INT32_MAX);
            posSampleRequestPtr->posSampleNodePtr->vAccuracy = vAccuracy;

            // Get horizontal speed
            le_gnss_GetHorizontalSpeed(positionSampleRef, &hSpeed, &hSpeedAccuracy);
            posSampleRequestPtr->posSampleNodePtr->hSpeedValid
                                    = CHECK_VALIDITY(hSpeed,UINT32_MAX);
            posSampleRequestPtr->posSampleNodePtr->hSpeed = hSpeed;
            posSampleRequestPtr->posSampleNodePtr->hSpeedAccuracyValid =
                                                CHECK_VALIDITY(hSpeedAccuracy,UINT32_MAX);
            posSampleRequestPtr->posSampleNodePtr->hSpeedAccuracy = hSpeedAccuracy;

            // Get vertical speed
            le_gnss_GetVerticalSpeed(positionSampleRef, &vSpeed, &vSpeedAccuracy);
            posSampleRequestPtr->posSampleNodePtr->vSpeedValid
                                = CHECK_VALIDITY(vSpeed,INT32_MAX);
            posSampleRequestPtr->posSampleNodePtr->vSpeed = vSpeed;
            posSampleRequestPtr->posSampleNodePtr->vSpeedAccuracyValid =
                                                CHECK_VALIDITY(vSpeedAccuracy,INT32_MAX);
            posSampleRequestPtr->posSampleNodePtr->vSpeedAccuracy = vSpeedAccuracy;

            // Heading not supported by GNSS engine
            posSampleRequestPtr->posSampleNodePtr->headingValid = false;
            posSampleRequestPtr->posSampleNodePtr->heading = UINT32_MAX;
            posSampleRequestPtr->posSampleNodePtr->headingAccuracyValid = false;
            posSampleRequestPtr->posSampleNodePtr->headingAccuracy = UINT32_MAX;

            // Get direction
            le_gnss_GetDirection(positionSampleRef, &direction, &directionAccuracy);

            posSampleRequestPtr->posSampleNodePtr->directionValid
                                = CHECK_VALIDITY(direction,UINT32_MAX);
            posSampleRequestPtr->posSampleNodePtr->direction = direction;
            posSampleRequestPtr->posSampleNodePtr->directionAccuracyValid =
                                            CHECK_VALIDITY(directionAccuracy,UINT32_MAX);
            posSampleRequestPtr->posSampleNodePtr->directionAccuracy = directionAccuracy;

            // Get UTC time
            if (LE_OK == le_gnss_GetDate(positionSampleRef, &year, &month, &day))
            {
                posSampleRequestPtr->posSampleNodePtr->dateValid = true;
            }
            else
            {
                posSampleRequestPtr->posSampleNodePtr->dateValid = false;
            }
            posSampleRequestPtr->posSampleNodePtr->year = year;
            posSampleRequestPtr->posSampleNodePtr->month = month;
            posSampleRequestPtr->posSampleNodePtr->day = day;

            if (LE_OK == le_gnss_GetTime(positionSampleRef, &hours, &minutes, &seconds,
                                         &milliseconds))
            {
                posSampleRequestPtr->posSampleNodePtr->timeValid = true;
            }
            else
            {
                posSampleRequestPtr->posSampleNodePtr->timeValid = false;
            }
            posSampleRequestPtr->posSampleNodePtr->hours = hours;
            posSampleRequestPtr->posSampleNodePtr->minutes = minutes;
            posSampleRequestPtr->posSampleNodePtr->seconds = seconds;
            posSampleRequestPtr->posSampleNodePtr->milliseconds = milliseconds;

            // Get UTC leap seconds in advance
            if (LE_OK == le_gnss_GetGpsLeapSeconds(positionSampleRef, &leapSeconds))
            {
               posSampleRequestPtr->posSampleNodePtr->leapSecondsValid = true;
            }
            else
            {
                posSampleRequestPtr->posSampleNodePtr->leapSecondsValid = false;
            }
            posSampleRequestPtr->posSampleNodePtr->leapSeconds = leapSeconds;

            // Get position fix state
            if (LE_OK != le_gnss_GetPositionState(positionSampleRef, &gnssState))
            {
                posSampleRequestPtr->posSampleNodePtr->fixState = LE_POS_STATE_UNKNOWN;
                LE_ERROR("Failed to get a position fix");
            }
            else
            {
                posSampleRequestPtr->posSampleNodePtr->fixState = (le_pos_FixState_t)gnssState;
            }

            posSampleRequestPtr->posSampleNodePtr->link = LE_DLS_LINK_INIT;

            // Add the node to the queue of the list by passing in the node's link.
            le_dls_Queue(&PosSampleList, &(posSampleRequestPtr->posSampleNodePtr->link));

            // Save the information reported to the handler function
            posSampleHandlerNodePtr->lastLat = latitude;
            posSampleHandlerNodePtr->lastLong = longitude;
            posSampleHandlerNodePtr->lastAlt = altitude;

            LE_DEBUG("Report sample %p to the corresponding handler (handler %p)",
                     posSampleRequestPtr->posSampleNodePtr,
                     posSampleHandlerNodePtr->handlerFuncPtr);

            le_pos_SampleRef_t reqRef = le_ref_CreateRef(PosSampleMap, posSampleRequestPtr);

            // Get the message session reference from handler function
            posSampleRequestPtr->sessionRef = posSampleHandlerNodePtr->sessionRef;

            // Store posSample reference which will be used in close session handler
            posSampleRequestPtr->positionSampleRef = reqRef;

            // Call the client's handler
            posSampleHandlerNodePtr->handlerFuncPtr(reqRef,
                                            posSampleHandlerNodePtr->handlerContextPtr);
        }

        // Move to the next node.
        linkPtr = le_dls_PeekNext(&PosSampleHandlerList, linkPtr);
    } while (NULL != linkPtr);

    // Release provided Position sample reference
    le_gnss_ReleaseSampleRef(positionSampleRef);
}

#ifdef LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
//--------------------------------------------------------------------------------------------------
/**
 * Handler function when an Acquition Rate change in configDB
 */
//--------------------------------------------------------------------------------------------------
static void AcquitisionRateUpdate
(
    void* contextPtr
)
{
    LE_DEBUG("Acquisition Rate changed");

    le_cfg_IteratorRef_t posCfg = le_cfg_CreateReadTxn(CFG_POSITIONING_PATH);

    AcqRate = le_cfg_GetInt(posCfg, CFG_NODE_RATE, DEFAULT_ACQUISITION_RATE);

    LE_DEBUG("New acquisition rate (%"PRIu32") for positioning", AcqRate);

    le_cfg_CancelTxn(posCfg);
}

//--------------------------------------------------------------------------------------------------
/**
 * Load the configuration tree
 */
//--------------------------------------------------------------------------------------------------
static void LoadPositioningFromConfigDb
(
    void
)
{
    // Check that the app has a configuration value.
    le_cfg_IteratorRef_t posCfg = le_cfg_CreateReadTxn(CFG_POSITIONING_PATH);

    // Default configuration
    AcqRate = le_cfg_GetInt(posCfg, CFG_NODE_RATE, DEFAULT_ACQUISITION_RATE);
    LE_DEBUG("Set acquisition rate to value %"PRIu32, AcqRate);

    // Add a configDb handler to check if the acquition rate change.
    le_cfg_AddChangeHandler(CFG_POSITIONING_RATE_PATH, AcquitisionRateUpdate,NULL);

    le_cfg_CancelTxn(posCfg);
}
#endif // LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING

//--------------------------------------------------------------------------------------------------
/**
* handler function to release Positioning service for le_posCtrl APIs
*
*/
//--------------------------------------------------------------------------------------------------
static void PosCtrlCloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    LE_ERROR("SessionRef (%p) has been closed", sessionRef);

    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    // Search for all positioning control request references used by the current client session
    // that has been closed.
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ActivationRequestRefMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (result == LE_OK)
    {
        ClientRequest_t * posCtrlHandlerPtr =
                        (ClientRequest_t *) le_ref_GetValue(iterRef);

        // Check if the session reference saved matchs with the current session reference.
        if (posCtrlHandlerPtr->sessionRef == sessionRef)
        {
            le_posCtrl_ActivationRef_t saferef =
                            (le_posCtrl_ActivationRef_t) le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release le_posCtrl_Release 0x%p, Session 0x%p",
                saferef, sessionRef);

            // Release positioning clent control request reference found.
            le_posCtrl_Release(saferef);
        }

        // Get the next value in the reference mpa
        result = le_ref_NextNode(iterRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
* handler function to release Positioning service for le_pos APIs
*
*/
//--------------------------------------------------------------------------------------------------
static void PosCloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    LE_ERROR("SessionRef (%p) has been closed", sessionRef);

    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
        return;
    }

    // Search for all position sample request references used by the current client session
    // that has been closed.
    le_ref_IterRef_t iterRef = le_ref_GetIterator(PosSampleMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while (result == LE_OK)
    {
        PosSampleRequest_t *posSampleRequestPtr = (PosSampleRequest_t*)le_ref_GetValue(iterRef);

        // Check if the session reference saved matchs with the current session reference.
        if (posSampleRequestPtr->sessionRef == sessionRef)
        {
            le_pos_SampleRef_t safeRef = (le_pos_SampleRef_t) le_ref_GetSafeRef(iterRef);
            LE_DEBUG("Release le_pos_sample_Release 0x%p, Session 0x%p\n", safeRef, sessionRef);

            // Release positioning clent control request reference found.
            le_pos_sample_Release(safeRef);
        }

        // Get the next value in the reference mpa
        result = le_ref_NextNode(iterRef);
    }
}

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Positioning component.
 *
 * @note The process exits on failure
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Create a pool for Position Sample objects
    PosSamplePoolRef = le_mem_InitStaticPool(PosSample,
                                             POSITIONING_SAMPLE_MAX,
                                             sizeof(le_pos_Sample_t));
    le_mem_SetDestructor(PosSamplePoolRef, PosSampleDestructor);

    // Create a pool for Position sample request objects
    PosSampleRequestPoolRef = le_mem_InitStaticPool(PosSampleRequest,
                                                    POSITIONING_SAMPLE_MAX,
                                                    sizeof(PosSampleRequest_t));

    // Initialize the event client close function handler.
    le_msg_ServiceRef_t posMsgService = le_pos_GetServiceRef();
    le_msg_AddServiceCloseHandler(posMsgService, PosCloseSessionEventHandler, NULL);

    // Initialize the event client close function handler.
    le_msg_ServiceRef_t posCtrlMsgService = le_posCtrl_GetServiceRef();
    le_msg_AddServiceCloseHandler(posCtrlMsgService, PosCtrlCloseSessionEventHandler, NULL);

    // Create a pool for Position Sample Handler objects
    PosSampleHandlerPoolRef = le_mem_InitStaticPool(PosSampleHandler,
                                                    HIGH_POS_SAMPLE_HANDLER_COUNT,
                                                    sizeof(le_pos_SampleHandler_t));
    le_mem_SetDestructor(PosSampleHandlerPoolRef, PosSampleHandlerDestructor);

    // Create the reference HashMap for positioning sample
    PosSampleMap = le_ref_InitStaticMap(PosSampleMap, POSITIONING_SAMPLE_MAX);

    NumOfHandlers = 0;
    GnssHandlerRef = NULL;

    // Create safe reference map for request references. The size of the map should be based on
    // the expected number of simultaneous data requests, so take a reasonable guess.
    ActivationRequestRefMap = le_ref_InitStaticMap(PositioningClient,
            LE_CONFIG_POSITIONING_ACTIVATION_MAX);

    // Create a pool for Position control client objects.
    PosCtrlHandlerPoolRef = le_mem_InitStaticPool(PosCtrlHandler,
                                                  LE_CONFIG_POSITIONING_ACTIVATION_MAX,
                                                  sizeof(ClientRequest_t));

    // TODO define a policy for positioning device selection
    if (IsGNSSAvailable() == true)
    {
        gnss_Init();
#ifdef LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
        LoadPositioningFromConfigDb();
#endif // LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
    }
    else
    {
        LE_CRIT("GNSS module not available");
    }

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);
    LE_DEBUG("Positioning service started.");
}

//--------------------------------------------------------------------------------------------------
/**
 * Request activation of the positioning service.
 *
 * @return
 *      - Reference to the service activation request (to be used later for releasing the request).
 *      - NULL if the service request could not be processed.
 */
//--------------------------------------------------------------------------------------------------
le_posCtrl_ActivationRef_t le_posCtrl_Request
(
    void
)
{
    ClientRequest_t * clientRequestPtr = le_mem_ForceAlloc(PosCtrlHandlerPoolRef);

    // On le_mem_ForceAlloc failure, the process exits, so function doesn't need to checking the
    // returned pointer for validity.

    // Need to return a unique reference that will be used by Release.
    le_posCtrl_ActivationRef_t reqRef =
                    le_ref_CreateRef(ActivationRequestRefMap, clientRequestPtr);

    if (CurrentActivationsCount == 0)
    {

        if (le_gnss_SetAcquisitionRate(AcqRate) != LE_OK)
        {
            LE_WARN("Failed to set GNSS's acquisition rate (%"PRIu32")", AcqRate);
        }

        // Start the GNSS acquisition.
        if (le_gnss_Start() != LE_OK)
        {
            le_ref_DeleteRef(ActivationRequestRefMap, reqRef);
            le_mem_Release(clientRequestPtr);
            return NULL;
        }
    }
    CurrentActivationsCount++;

    // Save the client session "msgSession" associated with the request reference "reqRef"
    le_msg_SessionRef_t msgSession = le_posCtrl_GetClientSessionRef();
    clientRequestPtr->sessionRef = msgSession;
    clientRequestPtr->posCtrlActivationRef = reqRef;

    LE_DEBUG("le_posCtrl_Request ref (%p), SessionRef (%p)", reqRef, msgSession);

    return reqRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Release the Positioning services.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_posCtrl_Release
(
    le_posCtrl_ActivationRef_t ref
        ///< [IN]
        ///< Reference to a positioning service activation request.
)
{
    void* posPtr = le_ref_Lookup(ActivationRequestRefMap, ref);
    if (posPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid positioning service activation reference %p", ref);
    }
    else
    {
        if (CurrentActivationsCount > 0)
        {
            CurrentActivationsCount--;
            if (CurrentActivationsCount == 0)
            {
                le_gnss_Stop();
            }
        }
        le_ref_DeleteRef(ActivationRequestRefMap, ref);
        LE_DEBUG("Remove Position Ctrl (%p)",ref);
        le_mem_Release(posPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler for movement notifications.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_pos_MovementHandlerRef_t le_pos_AddMovementHandler
(
    uint32_t                     horizontalMagnitude, ///< [IN] The horizontal magnitude in meters.
                                                      ///       0 means that I don't care about
                                                      ///       changes in the latitude and
                                                      ///       longitude.
    uint32_t                     verticalMagnitude,   ///< [IN] The vertical magnitude in meters.
                                                      ///       0 means that I don't care about
                                                      ///       changes in the altitude.
    le_pos_MovementHandlerFunc_t handlerPtr,          ///< [IN] The handler function.
    void*                        contextPtr           ///< [IN] The context pointer
)
{
    le_pos_SampleHandler_t*  posSampleHandlerNodePtr=NULL;

    if (NULL == handlerPtr)
    {
        LE_KILL_CLIENT("handlerPtr pointer is NULL!");
        return NULL;
    }

    // Create the position sample handler node.
    posSampleHandlerNodePtr = (le_pos_SampleHandler_t*)le_mem_ForceAlloc(PosSampleHandlerPoolRef);
    posSampleHandlerNodePtr->link = LE_DLS_LINK_INIT;
    posSampleHandlerNodePtr->handlerFuncPtr = handlerPtr;
    posSampleHandlerNodePtr->handlerContextPtr = contextPtr;
    posSampleHandlerNodePtr->acquisitionRate =
                                        CalculateAcquisitionRate(SUPPOSED_AVERAGE_SPEED,
                                                                 horizontalMagnitude,
                                                                 verticalMagnitude) * SEC_TO_MSEC;
    posSampleHandlerNodePtr->sessionRef = le_pos_GetClientSessionRef();
    AcqRate = ComputeCommonSmallestRate(posSampleHandlerNodePtr->acquisitionRate);

    LE_DEBUG("Calculated acquisition rate %"PRIu32" msec for an average speed of %d km/h",
             posSampleHandlerNodePtr->acquisitionRate,
             SUPPOSED_AVERAGE_SPEED);
    LE_DEBUG("Smallest computed acquisition rate %"PRIu32" msec", AcqRate);

#ifdef LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
    // update configDB with the new rate.
    {
        // Add the default acquisition rate,.
        le_cfg_IteratorRef_t posCfg = le_cfg_CreateWriteTxn(CFG_POSITIONING_PATH);
        // enter default the value
        le_cfg_SetInt(posCfg, CFG_NODE_RATE, AcqRate);
        le_cfg_CommitTxn(posCfg);
    }
#endif // LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING

    posSampleHandlerNodePtr->horizontalMagnitude = horizontalMagnitude;
    posSampleHandlerNodePtr->verticalMagnitude = verticalMagnitude;

    // Initialization of lastLat, lastLong and lastAlt.
    posSampleHandlerNodePtr->lastLat = 0;
    posSampleHandlerNodePtr->lastLong = 0;
    posSampleHandlerNodePtr->lastAlt = 0;

    // Start acquisition
    if (0 == NumOfHandlers)
    {
        if (NULL == (GnssHandlerRef=le_gnss_AddPositionHandler(PosSampleHandlerfunc, NULL)))
        {
            LE_ERROR("Failed to add PA GNSS's handler!");
            le_mem_Release(posSampleHandlerNodePtr);
            return NULL;
        }
    }

    le_dls_Queue(&PosSampleHandlerList, &(posSampleHandlerNodePtr->link));
    NumOfHandlers++;

    return (le_pos_MovementHandlerRef_t)posSampleHandlerNodePtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove a handler for movement notifications.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_pos_RemoveMovementHandler
(
    le_pos_MovementHandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
    le_pos_SampleHandler_t* posSampleHandlerNodePtr;
    le_dls_Link_t*          linkPtr;

    linkPtr = le_dls_Peek(&PosSampleHandlerList);
    if (linkPtr != NULL)
    {
        do
        {
            // Get the node from MsgList
            posSampleHandlerNodePtr = (le_pos_SampleHandler_t*)CONTAINER_OF(linkPtr,
                                                                            le_pos_SampleHandler_t,
                                                                            link);
            // Check the node.
            if ((le_pos_MovementHandlerRef_t)posSampleHandlerNodePtr == handlerRef)
            {
                // Remove the node.
                le_mem_Release(posSampleHandlerNodePtr);
                NumOfHandlers--;
                linkPtr=NULL;
            }
            else
            {
                // Move to the next node.
                linkPtr = le_dls_PeekNext(&PosSampleHandlerList, linkPtr);
            }
        } while (linkPtr != NULL);
    }

    if (NumOfHandlers == 0)
    {
        le_gnss_RemovePositionHandler(GnssHandlerRef);
        GnssHandlerRef = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's 2D location (latitude, longitude,
 * horizontal accuracy).
 *
 * @return LE_FAULT         Function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 * @return LE_OK            Function succeeded.
 * @return LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note latitudePtr, longitudePtr, horizontalAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_Get2DLocation
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* latitudePtr,
        ///< [OUT] WGS84 Latitude in degrees, positive North [resolution 1e-6].

    int32_t* longitudePtr,
        ///< [OUT] WGS84 Longitude in degrees, positive East [resolution 1e-6].

    int32_t* horizontalAccuracyPtr
        ///< [OUT] Horizontal position's accuracy in meters.
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = le_ref_Lookup(PosSampleMap,positionSampleRef);
    if (NULL == posSampleRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", positionSampleRef);
        return LE_BAD_PARAMETER;
    }

    if (posSampleRequestPtr->posSampleNodePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (latitudePtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->latitudeValid)
        {
            *latitudePtr = posSampleRequestPtr->posSampleNodePtr->latitude;
        }
        else
        {
            *latitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (longitudePtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->longitudeValid)
        {
            *longitudePtr = posSampleRequestPtr->posSampleNodePtr->longitude;
        }
        else
        {
            *longitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (horizontalAccuracyPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->hAccuracyValid)
        {
            // Update resolution
            *horizontalAccuracyPtr =
               ConvertDistance(posSampleRequestPtr->posSampleNodePtr->hAccuracy, H_ACCURACY);
        }
        else
        {
            *horizontalAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the position sample's time.
 *
 * @return LE_FAULT         Function failed to get the time.
 * @return LE_OUT_OF_RANGE  The retrieved time is invalid (all fields are set to 0).
 * @return LE_OK            Function succeeded.
 * @return LE_BAD_PARAMETER Invalid reference provided.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetTime
(
    le_pos_SampleRef_t  positionSampleRef,  ///< [IN] The position sample's reference.
    uint16_t* hoursPtr,                     ///< UTC Hours into the day [range 0..23].
    uint16_t* minutesPtr,                   ///< UTC Minutes into the hour [range 0..59].
    uint16_t* secondsPtr,                   ///< UTC Seconds into the minute [range 0..59].
    uint16_t* millisecondsPtr               ///< UTC Milliseconds into the second [range 0..999].
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = le_ref_Lookup(PosSampleMap,positionSampleRef);
    if (NULL == posSampleRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", positionSampleRef);
        return LE_BAD_PARAMETER;
    }

    if (posSampleRequestPtr->posSampleNodePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (posSampleRequestPtr->posSampleNodePtr->timeValid)
    {
        result = LE_OK;
        if (hoursPtr)
        {
            *hoursPtr = posSampleRequestPtr->posSampleNodePtr->hours;
        }
        if (minutesPtr)
        {
            *minutesPtr = posSampleRequestPtr->posSampleNodePtr->minutes;
        }
        if (secondsPtr)
        {
            *secondsPtr = posSampleRequestPtr->posSampleNodePtr->seconds;
        }
        if (millisecondsPtr)
        {
            *millisecondsPtr = posSampleRequestPtr->posSampleNodePtr->milliseconds;
        }
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        if (hoursPtr)
        {
            *hoursPtr = 0;
        }
        if (minutesPtr)
        {
            *minutesPtr = 0;
        }
        if (secondsPtr)
        {
            *secondsPtr = 0;
        }
        if (millisecondsPtr)
        {
            *millisecondsPtr = 0;
        }
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the position sample's date.
 *
 * @return LE_FAULT         Function failed to get the date.
 * @return LE_OUT_OF_RANGE  The retrieved date is invalid (all fields are set to 0).
 * @return LE_OK            Function succeeded.
 * @return LE_BAD_PARAMETER Invalid reference provided.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetDate
(
    le_pos_SampleRef_t  positionSampleRef,  ///< [IN] The position sample's reference.
    uint16_t* yearPtr,                      ///< UTC Year A.D. [e.g. 2014].
    uint16_t* monthPtr,                     ///< UTC Month into the year [range 1...12].
    uint16_t* dayPtr                        ///< UTC Days into the month [range 1...31].
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = le_ref_Lookup(PosSampleMap,positionSampleRef);
    if (NULL == posSampleRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", positionSampleRef);
        return LE_BAD_PARAMETER;
    }

    if (posSampleRequestPtr->posSampleNodePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (posSampleRequestPtr->posSampleNodePtr->dateValid)
    {
        result = LE_OK;
        if (yearPtr)
        {
            *yearPtr = posSampleRequestPtr->posSampleNodePtr->year;
        }
        if (monthPtr)
        {
            *monthPtr = posSampleRequestPtr->posSampleNodePtr->month;
        }
        if (dayPtr)
        {
            *dayPtr = posSampleRequestPtr->posSampleNodePtr->day;
        }
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        if (yearPtr)
        {
            *yearPtr = 0;
        }
        if (monthPtr)
        {
            *monthPtr = 0;
        }
        if (dayPtr)
        {
            *dayPtr = 0;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's altitude.
 *
 * @return LE_FAULT         Function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 * @return LE_OK            Function succeeded.
 * @return LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note altitudePtr, altitudeAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetAltitude
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    int32_t* altitudePtr,
        ///< [OUT] Altitude in meters, above Mean Sea Level.

    int32_t* altitudeAccuracyPtr
        ///< [OUT] Vertical position's accuracy in meters.
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = le_ref_Lookup(PosSampleMap,positionSampleRef);
    if (NULL == posSampleRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", positionSampleRef);
        return LE_BAD_PARAMETER;
    }

    if (posSampleRequestPtr->posSampleNodePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (altitudePtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->altitudeValid)
        {
            // Update resolution
            *altitudePtr =
               ConvertDistance(posSampleRequestPtr->posSampleNodePtr->altitude, ALTITUDE);
        }
        else
        {
            *altitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (altitudeAccuracyPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->vAccuracyValid)
        {
            // Update resolution
            *altitudeAccuracyPtr =
               ConvertDistance(posSampleRequestPtr->posSampleNodePtr->vAccuracy, V_ACCURACY);
        }
        else
        {
            *altitudeAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's horizontal speed.
 *
 * @return LE_FAULT         Function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid
 *                          (set to INT32_MAX, UINT32_MAX).
 * @return LE_OK            Function succeeded.
 * @return LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note hSpeedPtr, hSpeedAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetHorizontalSpeed
(
    le_pos_SampleRef_t positionSampleRef,
        ///< [IN] Position sample's reference.

    uint32_t* hSpeedPtr,
        ///< [OUT] The Horizontal Speed in m/sec.

    uint32_t* hSpeedAccuracyPtr
        ///< [OUT] The Horizontal Speed's accuracy in m/sec.
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = le_ref_Lookup(PosSampleMap,positionSampleRef);
    if (NULL == posSampleRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", positionSampleRef);
        return LE_BAD_PARAMETER;
    }

    if (posSampleRequestPtr->posSampleNodePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (hSpeedPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->hSpeedValid)
        {
            // Update resolution
            *hSpeedPtr = posSampleRequestPtr->posSampleNodePtr->hSpeed/100;
        }
        else
        {
            *hSpeedPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (hSpeedAccuracyPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->hSpeedAccuracyValid)
        {
            // Update resolution
            *hSpeedAccuracyPtr = posSampleRequestPtr->posSampleNodePtr->hSpeedAccuracy/10;
        }
        else
        {
            *hSpeedAccuracyPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the position sample's vertical speed.
 *
 * @return LE_FAULT         The function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX).
 * @return LE_OK            The function succeeded.
 * @return LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note vSpeedPtr, vSpeedAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetVerticalSpeed
(
    le_pos_SampleRef_t  positionSampleRef, ///< [IN] The position sample's reference.
    int32_t*            vSpeedPtr,         ///< [OUT] The vertical speed.
    int32_t*            vSpeedAccuracyPtr  ///< [OUT] The vertical speed's accuracy estimate.
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = le_ref_Lookup(PosSampleMap,positionSampleRef);
    if (NULL == posSampleRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", positionSampleRef);
        return LE_BAD_PARAMETER;
    }

    if (posSampleRequestPtr->posSampleNodePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (vSpeedPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->vSpeedValid)
        {
            *vSpeedPtr = posSampleRequestPtr->posSampleNodePtr->vSpeed/100; // Update resolution
        }
        else
        {
            *vSpeedPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vSpeedAccuracyPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->vSpeedAccuracyValid)
        {
            // Update resolution
            *vSpeedAccuracyPtr = posSampleRequestPtr->posSampleNodePtr->vSpeedAccuracy/10;
        }
        else
        {
            *vSpeedAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the position sample's heading. Heading is the direction that
 * the vehicle or person is facing.
 *
 * @return LE_FAULT         The function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to UINT32_MAX).
 * @return LE_OK            The function succeeded.
 * @return LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note Heading is given in degrees.
 *       Heading ranges from 0 to 359 degrees, where 0 is True North.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note headingPtr, headingAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetHeading
(
    le_pos_SampleRef_t positionSampleRef, ///< [IN]  The position sample's reference.
    uint32_t*          headingPtr,        ///< [OUT] The heading in degrees.
                                          ///<       Range: 0 to 359, where 0 is True North.
    uint32_t*          headingAccuracyPtr ///< [OUT] The heading's accuracy estimate.
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = le_ref_Lookup(PosSampleMap,positionSampleRef);
    if (NULL == posSampleRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", positionSampleRef);
        return LE_BAD_PARAMETER;
    }

    if (posSampleRequestPtr->posSampleNodePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (headingPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->headingValid)
        {
            *headingPtr = posSampleRequestPtr->posSampleNodePtr->heading;
        }
        else
        {
            *headingPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (headingAccuracyPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->headingAccuracyValid)
        {
            *headingAccuracyPtr = posSampleRequestPtr->posSampleNodePtr->headingAccuracy;
        }
        else
        {
            *headingAccuracyPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the position sample's direction. Direction of movement is the
 * direction that the vehicle or person is actually moving.
 *
 * @return LE_FAULT         The function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to UINT32_MAX).
 * @return LE_OK            The function succeeded.
 * @return LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note Direction is given in degrees.
 *       Direction ranges from 0 to 359 degrees, where 0 is True North.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note directionPtr, directionAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetDirection
(
    le_pos_SampleRef_t  positionSampleRef,    ///< [IN]  The position sample's reference.
    uint32_t*           directionPtr,         ///< [OUT] Direction indication in degrees.
                                              ///<       Range: 0 to 359, where 0 is True North.
    uint32_t*           directionAccuracyPtr  ///< [OUT] The direction's accuracy estimate.
)
{
    le_result_t result = LE_OK;
    PosSampleRequest_t* posSampleRequestPtr = le_ref_Lookup(PosSampleMap,positionSampleRef);
    if (NULL == posSampleRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", positionSampleRef);
        return LE_BAD_PARAMETER;
    }

    if (posSampleRequestPtr->posSampleNodePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (directionPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->directionValid)
        {
            // Update resolution
            *directionPtr = posSampleRequestPtr->posSampleNodePtr->direction/10;
        }
        else
        {
            *directionPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (directionAccuracyPtr)
    {
        if (posSampleRequestPtr->posSampleNodePtr->directionAccuracyValid)
        {
            // Update resolution
            *directionAccuracyPtr = posSampleRequestPtr->posSampleNodePtr->directionAccuracy/10;
        }
        else
        {
            *directionAccuracyPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the position sample's fix state.
 *
 * @return LE_FAULT         Function failed to get the position sample's fix state.
 * @return LE_OK            Function succeeded.
 * @return LE_BAD_PARAMETER Invalid reference provided.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetFixState
(
    le_pos_SampleRef_t  positionSampleRef,    ///< [IN] The position sample's reference.
    le_pos_FixState_t*  statePtr              ///< [OUT] Position fix state.
)
{
    PosSampleRequest_t* posSampleRequestPtr = le_ref_Lookup(PosSampleMap,positionSampleRef);
    if (NULL == posSampleRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", positionSampleRef);
        return LE_BAD_PARAMETER;
    }

    if (posSampleRequestPtr->posSampleNodePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (statePtr)
    {
        *statePtr = posSampleRequestPtr->posSampleNodePtr->fixState;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release the position sample.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_pos_sample_Release
(
    le_pos_SampleRef_t    positionSampleRef    ///< [IN] The position sample's reference.
)
{
    if (positionSampleRef == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", positionSampleRef);
        return;
    }
    PosSampleRequest_t* posSampleRequestPtr = le_ref_Lookup(PosSampleMap, positionSampleRef);
    if (NULL == posSampleRequestPtr)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", positionSampleRef);
        return;
    }

    if (posSampleRequestPtr->posSampleNodePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", positionSampleRef);
        return;
    }
    le_ref_DeleteRef(PosSampleMap, positionSampleRef);
    le_mem_Release(posSampleRequestPtr->posSampleNodePtr);
    le_mem_Release(posSampleRequestPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the 2D location's data (Latitude, Longitude, Horizontal
 * accuracy).
 *
 * @return LE_FAULT         Function failed to get the 2D location's data
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note latitudePtr, longitudePtr, hAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_Get2DLocation
(
    int32_t* latitudePtr,
        ///< [OUT] WGS84 Latitude in degrees, positive North [resolution 1e-6].

    int32_t* longitudePtr,
        ///< [OUT] WGS84 Longitude in degrees, positive East [resolution 1e-6].

    int32_t* hAccuracyPtr
        ///< [OUT] Horizontal position's accuracy in meters.
)
{
    // At least one valid pointer
    if ((latitudePtr == NULL)&&
        (longitudePtr == NULL)&&
        (hAccuracyPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid input parameters!");
        return LE_FAULT;
    }

    int32_t     latitude;
    int32_t     longitude;
    int32_t     hAccuracy;
    le_result_t gnssResult = LE_OK;
    le_result_t posResult = LE_OK;
    le_gnss_SampleRef_t positionSampleRef = le_gnss_GetLastSampleRef();

    // Get Location
    gnssResult = le_gnss_GetLocation(positionSampleRef, &latitude, &longitude, &hAccuracy);
    if ((gnssResult == LE_OK)||(gnssResult == LE_OUT_OF_RANGE))
    {
        if (latitudePtr)
        {
            if (latitude == INT32_MAX)
            {
                posResult = LE_OUT_OF_RANGE;
            }
            *latitudePtr = latitude;
        }
        if (longitudePtr)
        {
            if (longitude == INT32_MAX)
            {
                posResult = LE_OUT_OF_RANGE;
            }
            *longitudePtr = longitude;
        }
        if (hAccuracyPtr)
        {
            if (hAccuracy == INT32_MAX)
            {
                *hAccuracyPtr = hAccuracy;
                posResult = LE_OUT_OF_RANGE;
            }
            else
            {
                // Update resolution
                *hAccuracyPtr = ConvertDistance(hAccuracy, H_ACCURACY);
            }
        }
    }
    else
    {
        posResult = LE_FAULT;
    }

    // Release provided Position sample reference
    le_gnss_ReleaseSampleRef(positionSampleRef);

    return posResult;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the 3D location's data (Latitude, Longitude, Altitude,
 * Horizontal accuracy, Vertical accuracy).
 *
 * @return LE_FAULT         Function failed to get the 3D location's data
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note latitudePtr, longitudePtr,hAccuracyPtr, altitudePtr, vAccuracyPtr can be set to NULL
 *       if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_Get3DLocation
(
    int32_t* latitudePtr,
        ///< [OUT] WGS84 Latitude in degrees, positive North [resolution 1e-6].

    int32_t* longitudePtr,
        ///< [OUT] WGS84 Longitude in degrees, positive East [resolution 1e-6].

    int32_t* hAccuracyPtr,
        ///< [OUT] Horizontal position's accuracy in meters.

    int32_t* altitudePtr,
        ///< [OUT] Altitude in meters, above Mean Sea Level.

    int32_t* vAccuracyPtr
        ///< [OUT] Vertical position's accuracy in meters.
)
{
    // At least one valid pointer
    if ((latitudePtr == NULL)&&
        (longitudePtr == NULL)&&
        (hAccuracyPtr == NULL)&&
        (altitudePtr == NULL)&&
        (vAccuracyPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid input parameters!");
        return LE_FAULT;
    }

    int32_t     latitude;
    int32_t     longitude;
    int32_t     hAccuracy;
    int32_t     altitude;
    int32_t     vAccuracy;
    le_result_t gnssResult = LE_OK;
    le_result_t posResult = LE_OK;
    le_gnss_SampleRef_t positionSampleRef = le_gnss_GetLastSampleRef();

    // Get Location
    gnssResult = le_gnss_GetLocation(positionSampleRef, &latitude, &longitude, &hAccuracy);
    if ((gnssResult == LE_OK)||(gnssResult == LE_OUT_OF_RANGE))
    {
        if (latitudePtr)
        {
            if (latitude == INT32_MAX)
            {
                posResult = LE_OUT_OF_RANGE;
            }
            *latitudePtr = latitude;
        }
        if (longitudePtr)
        {
            if (longitude == INT32_MAX)
            {
                posResult = LE_OUT_OF_RANGE;
            }
            *longitudePtr = longitude;
        }
        if (hAccuracyPtr)
        {
            if (hAccuracy == INT32_MAX)
            {
                *hAccuracyPtr = hAccuracy;
                posResult = LE_OUT_OF_RANGE;
            }
            else
            {
                // Update resolution
                *hAccuracyPtr = ConvertDistance(hAccuracy, H_ACCURACY);
            }
        }
    }
    else
    {
        posResult = LE_FAULT;
    }

    // Get altitude
    gnssResult = le_gnss_GetAltitude(positionSampleRef, &altitude, &vAccuracy);

    if (((gnssResult == LE_OK)||(gnssResult == LE_OUT_OF_RANGE))
        &&(posResult != LE_FAULT))
    {
        if (altitudePtr)
        {
            if (altitude == INT32_MAX)
            {
                *altitudePtr = altitude;
                posResult = LE_OUT_OF_RANGE;
            }
            else
            {
                // Update resolution
                *altitudePtr = ConvertDistance(altitude, ALTITUDE);
            }
        }
        if (vAccuracyPtr)
        {
            if (vAccuracy == INT32_MAX)
            {
                *vAccuracyPtr = vAccuracy;
                posResult = LE_OUT_OF_RANGE;
            }
            else
            {
                *vAccuracyPtr = ConvertDistance(vAccuracy, V_ACCURACY);
            }
        }
    }
    else
    {
        posResult = LE_FAULT;
    }

    // Release provided Position sample reference
    le_gnss_ReleaseSampleRef(positionSampleRef);

    return posResult;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the time of the last updated location
 *
 * @return LE_FAULT         Function failed to get the time.
 * @return LE_OUT_OF_RANGE  The retrieved time is invalid (all fields are set to 0).
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetTime
(
    uint16_t* hoursPtr,
        ///< [OUT] UTC Hours into the day [range 0..23].

    uint16_t* minutesPtr,
        ///< [OUT] UTC Minutes into the hour [range 0..59].

    uint16_t* secondsPtr,
        ///< [OUT] UTC Seconds into the minute [range 0..59].

    uint16_t* millisecondsPtr
        ///< [OUT] UTC Milliseconds into the second [range 0..999].
)
{
    // All pointers must be valid
    if ((hoursPtr == NULL)||
        (minutesPtr == NULL)||
        (secondsPtr == NULL)||
        (millisecondsPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid input parameters!");
        return LE_FAULT;
    }

    le_result_t posResult = LE_OK;
    le_gnss_SampleRef_t positionSampleRef = le_gnss_GetLastSampleRef();

    // Get UTC time
    posResult = le_gnss_GetTime(positionSampleRef,
                                hoursPtr,
                                minutesPtr,
                                secondsPtr,
                                millisecondsPtr);

    // Release provided Position sample reference
    le_gnss_ReleaseSampleRef(positionSampleRef);

    return posResult;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the date of the last updated location
 *
 * @return LE_FAULT         Function failed to get the date.
 * @return LE_OUT_OF_RANGE  The retrieved date is invalid (all fields are set to 0).
 * @return LE_OK            Function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetDate
(
    uint16_t* yearPtr,
        ///< [OUT] UTC Year A.D. [e.g. 2014].

    uint16_t* monthPtr,
        ///< [OUT] UTC Month into the year [range 1...12].

    uint16_t* dayPtr
        ///< [OUT] UTC Days into the month [range 1...31].
)
{
    // All pointers must be valid
    if ((yearPtr == NULL)||
        (monthPtr == NULL)||
        (dayPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid input parameters!");
        return LE_FAULT;
    }

    le_result_t posResult = LE_OK;
    le_gnss_SampleRef_t positionSampleRef = le_gnss_GetLastSampleRef();

    // Get date
    posResult = le_gnss_GetDate(positionSampleRef, yearPtr, monthPtr, dayPtr);

    // Release provided Position sample reference
    le_gnss_ReleaseSampleRef(positionSampleRef);

    return posResult;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the motion's data (Horizontal Speed, Horizontal Speed's
 * accuracy, Vertical Speed, Vertical Speed's accuracy).
 *
 * @return LE_FAULT         The function failed to get the motion's data.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX,
 *                          UINT32_MAX).
 * @return LE_OK            The function succeeded.
 *
 * @note hSpeedPtr, hSpeedAccuracyPtr, vSpeedPtr, vSpeedAccuracyPtr can be set to NULL if not
 *       needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetMotion
(
    uint32_t*   hSpeedPtr,          ///< [OUT] The Horizontal Speed in m/sec.
    uint32_t*   hSpeedAccuracyPtr,  ///< [OUT] The Horizontal Speed's accuracy in m/sec.
    int32_t*    vSpeedPtr,          ///< [OUT] The Vertical Speed in m/sec, positive up.
    int32_t*    vSpeedAccuracyPtr   ///< [OUT] The Vertical Speed's accuracy in m/sec.
)
{
    // At least one valid pointer
    if ((hSpeedPtr == NULL)&&
        (hSpeedAccuracyPtr == NULL)&&
        (vSpeedPtr == NULL)&&
        (vSpeedAccuracyPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid input parameters!");
        return LE_FAULT;
    }

    // Horizontal speed
    uint32_t hSpeed;
    uint32_t hSpeedAccuracy;
    // Vertical speed
    int32_t vSpeed;
    int32_t vSpeedAccuracy;
    le_result_t gnssResult = LE_OK;
    le_result_t posResult = LE_OK;
    le_gnss_SampleRef_t positionSampleRef = le_gnss_GetLastSampleRef();

    // Get horizontal speed
    gnssResult = le_gnss_GetHorizontalSpeed(positionSampleRef, &hSpeed, &hSpeedAccuracy);
    if ((gnssResult == LE_OK)||(gnssResult == LE_OUT_OF_RANGE))
    {
        if (hSpeedPtr)
        {
            if (hSpeed != UINT32_MAX)
            {
                *hSpeedPtr = hSpeed/100; // Update resolution
            }
            else
            {
                *hSpeedPtr = hSpeed;
                posResult = LE_OUT_OF_RANGE;
            }
        }
        if (hSpeedAccuracyPtr)
        {
            if (hSpeedAccuracy != UINT32_MAX)
            {
                *hSpeedAccuracyPtr = hSpeedAccuracy/10; // Update resolution
            }
            else
            {
                *hSpeedAccuracyPtr = hSpeedAccuracy;
                posResult = LE_OUT_OF_RANGE;
            }
        }
    }
    else
    {
        posResult = LE_FAULT;
    }

    // Get vertical speed
    gnssResult = le_gnss_GetVerticalSpeed(positionSampleRef, &vSpeed, &vSpeedAccuracy);

   if (((gnssResult == LE_OK)||(gnssResult == LE_OUT_OF_RANGE))
        &&(posResult != LE_FAULT))
    {
        if (vSpeedPtr)
        {
            if (vSpeed != INT32_MAX)
            {
                *vSpeedPtr = vSpeed/100; // Update resolution
            }
            else
            {
                *vSpeedPtr = vSpeed;
                posResult = LE_OUT_OF_RANGE;
            }
        }
        if (vSpeedAccuracyPtr)
        {
            if (vSpeedAccuracy != INT32_MAX)
            {
                *vSpeedAccuracyPtr = vSpeedAccuracy/10; // Update resolution
            } else
            {
                *vSpeedAccuracyPtr = vSpeedAccuracy;
                posResult = LE_OUT_OF_RANGE;
            }
        }
    }
    else
    {
        posResult = LE_FAULT;
    }

    // Release provided Position sample reference
    le_gnss_ReleaseSampleRef(positionSampleRef);

    return posResult;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the heading indication.
 *
 * @return LE_FAULT         The function failed to get the heading indication.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to UINT32_MAX).
 * @return LE_OK            The function succeeded.
 *
 * @note Heading is given in degrees.
 *       Heading ranges from 0 to 359 degrees, where 0 is True North.
 *
 * @note headingPtr, headingAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetHeading
(
    uint32_t*  headingPtr,        ///< [OUT] The heading in degrees.
                                  ///<       Range: 0 to 359, where 0 is True North.
    uint32_t*  headingAccuracyPtr ///< [OUT] The heading's accuracy.
)
{

    // At least one valid pointer
    if ((headingPtr == NULL)&&
        (headingAccuracyPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid input parameters!");
        return LE_FAULT;
    }

    // Heading indication not supported by GNSS feature.
    if (headingPtr)
    {
        *headingPtr = UINT32_MAX;
    }
    if (headingAccuracyPtr)
    {
        *headingAccuracyPtr = UINT32_MAX;
    }

    return LE_OUT_OF_RANGE;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the direction indication. Direction of movement is the direction that the vehicle or person
 * is actually moving.
 *
 * @return LE_FAULT         Function failed to get the direction indication.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to UINT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note Direction is given in degrees.
 *       Direction ranges from 0 to 359 degrees, where 0 is True North.
 *
 * @note directionPtr, directionAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetDirection
(
    uint32_t* directionPtr,
        ///< [OUT] Direction indication in degrees. Range: 0 to 359, where 0 is True North.
    uint32_t* directionAccuracyPtr
        ///< [OUT] Direction's accuracy estimate in degrees.
)
{
    // At least one valid pointer
    if ((directionPtr == NULL)&&
        (directionAccuracyPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid input parameters!");
        return LE_FAULT;
    }

    // Direction
    uint32_t direction;
    uint32_t directionAccuracy;
    le_result_t gnssResult = LE_OK;
    le_result_t posResult = LE_OK;
    le_gnss_SampleRef_t positionSampleRef = le_gnss_GetLastSampleRef();

    // Get direction
    gnssResult = le_gnss_GetDirection(positionSampleRef, &direction, &directionAccuracy);

    if ((gnssResult == LE_OK)||(gnssResult == LE_OUT_OF_RANGE))
    {
        if (directionPtr)
        {
            if (direction != UINT32_MAX)
            {
                *directionPtr = direction/10; // Update resolution
            }
            else
            {
                *directionPtr = direction;
                posResult = LE_OUT_OF_RANGE;
            }
        }
        if (directionAccuracyPtr)
        {
            if (directionAccuracy != UINT32_MAX)
            {
                *directionAccuracyPtr = directionAccuracy/10; // Update resolution
            }
            else
            {
                *directionAccuracyPtr = directionAccuracy;
                posResult = LE_OUT_OF_RANGE;
            }
        }
    }
    else
    {
        posResult = LE_FAULT;
    }

    // Release provided Position sample reference
    le_gnss_ReleaseSampleRef(positionSampleRef);

    return posResult;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position fix state
 *
 * @return LE_FAULT         Function failed to get the fix state.
 * @return LE_OK            Function succeeded.
 *
 * @note In case the function fails to get the fix state a fatal error occurs,
 *       the function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetFixState
(
    le_pos_FixState_t* statePtr   ///< [OUT] Position fix state.
)
{
    le_gnss_FixState_t gnssState;
    le_gnss_SampleRef_t positionSampleRef = le_gnss_GetLastSampleRef();

    // At least one valid pointer
    if (NULL == statePtr)
    {
        LE_KILL_CLIENT("state pointer is NULL");
        return LE_FAULT;
    }

    // Check position sample's reference
    if (NULL == positionSampleRef)
    {
        LE_KILL_CLIENT("Invalid reference (%p)",positionSampleRef);
        return LE_FAULT;
    }

    // Get position fix state.
    if (LE_OK != le_gnss_GetPositionState(positionSampleRef, &gnssState))
    {
        *statePtr = LE_POS_STATE_UNKNOWN;
        LE_ERROR("Failed to get the position fix state");
    }
    else
    {
        *statePtr = (le_pos_FixState_t)gnssState;
    }

    // Release provided Position sample reference
    le_gnss_ReleaseSampleRef(positionSampleRef);

    return LE_OK;
}

// -------------------------------------------------------------------------------------------------
/**
 * Set the acquisition rate.
 *
 * @return
 *    LE_OUT_OF_RANGE    Invalid acquisition rate.
 *    LE_OK              The function succeeded.
 */
// -------------------------------------------------------------------------------------------------
le_result_t le_pos_SetAcquisitionRate
(
    uint32_t  acquisitionRate   ///< IN Acquisition rate in milliseconds.
)
{
#ifdef LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
    if (!acquisitionRate)
    {
        LE_WARN("Invalid acquisition rate (%"PRIu32")", acquisitionRate);
        return LE_OUT_OF_RANGE;
    }

    le_cfg_IteratorRef_t posCfg = le_cfg_CreateWriteTxn(CFG_POSITIONING_PATH);
    le_cfg_SetInt(posCfg,CFG_NODE_RATE,acquisitionRate);
    le_cfg_CommitTxn(posCfg);
    return LE_OK;
#else
    return LE_UNSUPPORTED;
#endif // LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
}

// -------------------------------------------------------------------------------------------------
/**
 * Retrieve the acquisition rate.
 *
 * @return
 *   Acquisition rate in milliseconds.
 */
// -------------------------------------------------------------------------------------------------
uint32_t le_pos_GetAcquisitionRate
(
    void
)
{
    uint32_t  acquisitionRate = DEFAULT_ACQUISITION_RATE;

#ifdef LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING
    le_cfg_IteratorRef_t posCfg = le_cfg_CreateReadTxn(CFG_POSITIONING_PATH);
    acquisitionRate = le_cfg_GetInt(posCfg, CFG_NODE_RATE, DEFAULT_ACQUISITION_RATE);
    le_cfg_CancelTxn(posCfg);

    LE_DEBUG("acquisition rate (%"PRIu32") for positioning",acquisitionRate);
#endif // LE_CONFIG_ENABLE_GNSS_ACQUISITION_RATE_SETTING

    return acquisitionRate;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the resolution for the positioning distance values.
 *
 * @return LE_OK               Function succeeded.
 * @return LE_BAD_PARAMETER    Invalid parameter provided.
 *
 * @note The positioning distance values are: the altitude above sea level, the horizontal
 *       position accuracy and the vertical position accuracy. The API sets the same resolution to
 *       all distance values. The resolution change request takes effect immediately.
 *
 * @warning The positioning distance values resolutions are platform dependent. Please refer to
 *          @ref platformConstraintsPositioning_SettingResolution section for full details.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_SetDistanceResolution
(
    le_pos_Resolution_t resolution    ///< IN Resolution.
)
{
    if (resolution >= LE_POS_RES_UNKNOWN)
    {
        LE_ERROR("Invalid resolution (%d)", resolution);
        return LE_BAD_PARAMETER;
    }

    LE_DEBUG("resolution %d saved", resolution);

    DistanceResolution = resolution;
    return LE_OK;
}
