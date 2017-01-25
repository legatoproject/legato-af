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
#include "posCfgEntries.h"

#include <math.h>

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
// TODO move to configuration module? or retrieve it from a speedometer?
//--------------------------------------------------------------------------------------------------
#define SUPPOSED_AVERAGE_SPEED       50  // 50 km/h
#define DEFAULT_ACQUISITION_RATE     5000   // 5 seconds
#define DEFAULT_POWER_STATE          true


// To compute the estimated horizontal error, we assume that the GNSS's User Equivalent Range Error
// (UERE) is equivalent for all the satellites, for civil application UERE is approximatively 7
// meters.
#define GNSS_UERE                                 7
#define GNSS_ESTIMATED_VERTICAL_ERROR_FACTOR      GNSS_UERE * 1.5

#define POSITIONING_SAMPLE_MAX         1


/// Typically, we don't expect more than this number of concurrent activation requests.
#define POSITIONING_ACTIVATION_MAX      13      // Ideally should be a prime number.


#define CHECK_VALIDITY(_par_,_max_) ((_par_) == (_max_))? false : true

//--------------------------------------------------------------------------------------------------
/**
 * Count of the number of activation requests that have not been released yet.
 *
 * See le_posCtrl_Request().
 */
//--------------------------------------------------------------------------------------------------
static int CurrentActivationsCount = 0;


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
    le_pos_FixState_t fixState;        ///< Position Fix state
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
    le_dls_Link_t                link;                ///< Object node link
}
le_pos_SampleHandler_t;


//--------------------------------------------------------------------------------------------------
/**
 * Position control Client request objet structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_posCtrl_ActivationRef_t      posCtrlActivationRef;   ///< Potistioning control reference.
    le_msg_SessionRef_t             sessionRef;             ///< Client session identifier.
    le_dls_Link_t                   link;                   ///< Object node link.
}
ClientRequest_t;


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
 * Memory Pool for position samples.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   PosSamplePoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for position sample's handlers.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   PosSampleHandlerPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Positioning Sample objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t PosSampleMap;

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
static uint32_t AcqRate = 1000; // in milliseconds.

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
            if ( posSampleNodePtr == (le_pos_Sample_t*)obj)
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
                                                                    le_pos_SampleHandler_t, link);
            // Check the node.
            if ( posSampleHandlerNodePtr == (le_pos_SampleHandler_t*)obj)
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
 * Calculate the GNSS's Acquisistion rate.
 *
 */
//--------------------------------------------------------------------------------------------------
static uint32_t CalculateAcquisitionRate
(
    uint32_t averageSpeed,         // km/h
    uint32_t horizontalMagnitude,  // The horizontal magnitude in meters.
    uint32_t verticalMagnitude     // The vertical magnitude in meters.
)
{
    uint32_t  rate;
    uint32_t  metersec = averageSpeed*1000/3600; // convert speed in m/sec

    if ((horizontalMagnitude<metersec) || (verticalMagnitude<metersec))
    {
        return 1;
    }

    rate = 0;
    while (! (((horizontalMagnitude >= (metersec + metersec*rate)) &&
               (verticalMagnitude >= (metersec + metersec*rate))) &&
             ((horizontalMagnitude < (metersec + metersec*(rate+1))) ||
              (verticalMagnitude < (metersec + metersec*(rate+1))))))
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
        if ( (accuracy > move) || ((move - accuracy) < magnitude) )
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
                                                                  le_pos_SampleHandler_t, link);
            // Check the node.
            if ( posSampleHandlerNodePtr->acquisitionRate < rate )
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
    // Positioning sample parameters
    le_pos_SampleHandler_t* posSampleHandlerNodePtr;
    le_dls_Link_t*          linkPtr;
    le_pos_Sample_t*        posSampleNodePtr=NULL;

    if (!NumOfHandlers)
    {
        return;
    }

    LE_DEBUG("Handler Function called with sample %p", positionSampleRef);

    // Get Location
    result = le_gnss_GetLocation( positionSampleRef
                                , &latitude
                                , &longitude
                                , &hAccuracy);
    if ((result == LE_OK)
    ||((result == LE_OUT_OF_RANGE)&&(latitude != INT32_MAX)&&(longitude != INT32_MAX)))
    {
        locationValid = true;
        LE_DEBUG("Position lat.%d, long.%d, hAccuracy.%d"
                    , latitude, longitude, hAccuracy/10);
    }
    else
    {
        locationValid = false;
        LE_DEBUG("Position unknown [%d,%d,%d]"
                , latitude, longitude, hAccuracy);
    }

    // Get altitude
    result = le_gnss_GetAltitude( positionSampleRef
                                , &altitude
                                , &vAccuracy);
    if ((result == LE_OK)
    ||((result == LE_OUT_OF_RANGE)&&(altitude != INT32_MAX)))
    {
        altitudeValid = true;
        LE_DEBUG("Altitude.%d, vAccuracy.%d"
                , altitude/1000, vAccuracy/10);
    }
    else
    {
        altitudeValid = false;
        LE_DEBUG("Altitude unknown [%d,%d]"
                , altitude, vAccuracy);
    }
    // Positioning sample
    linkPtr = le_dls_Peek(&PosSampleHandlerList);
    if (linkPtr != NULL)
    {
        do
        {
            bool hflag, vflag;
            // Get the node from the list
            posSampleHandlerNodePtr = (le_pos_SampleHandler_t*)CONTAINER_OF(linkPtr,
                                                                 le_pos_SampleHandler_t, link);

            if ((posSampleHandlerNodePtr->horizontalMagnitude != 0) && !locationValid)
            {
                LE_DEBUG("Longitude or Latitude are not relevant");
                return;
            }

            if (((posSampleHandlerNodePtr->verticalMagnitude != 0) && (!altitudeValid)))
            {
                LE_DEBUG("Altitude is not relevant");
                return;
            }

            // Compute horizontal and vertical move
            LE_DEBUG("Last Position lat.%d, long.%d"
                    , posSampleHandlerNodePtr->lastLat, posSampleHandlerNodePtr->lastLong);
            uint32_t horizontalMove = ComputeDistance(posSampleHandlerNodePtr->lastLat,
                                                      posSampleHandlerNodePtr->lastLong,
                                                      latitude,
                                                      longitude);
            uint32_t verticalMove = abs(altitude-posSampleHandlerNodePtr->lastAlt);

            LE_DEBUG("horizontalMove.%d, verticalMove.%d", horizontalMove, verticalMove);

            if (vAccuracy != INT32_MAX)
            {
                vflag = false;
            } else
            {
                vflag = IsBeyondMagnitude(posSampleHandlerNodePtr->verticalMagnitude,
                                          verticalMove,
                                          vAccuracy/10); // Accuracy in meters with 1 decimal place
            }

            if (hAccuracy != INT32_MAX)
            {
                hflag = false;
            } else
            {
                hflag = IsBeyondMagnitude(posSampleHandlerNodePtr->horizontalMagnitude,
                                          horizontalMove,
                                          hAccuracy/10); // Accuracy in meters with 1 decimal place
            }

            LE_DEBUG("Vertical IsBeyondMagnitude.%d", vflag);
            LE_DEBUG("Horizontal IsBeyondMagnitude.%d", hflag);

            // Movement is detected in the following cases:
            // - Vertical distance is beyond the magnitude
            // - Horizontal distance is beyond the magnitude
            // - We don't care about vertical & horizontal distance (magnitudes equal to 0)
            //   therefore that movement handler is called each positioning acquisition rate
            if ( ( (posSampleHandlerNodePtr->verticalMagnitude != 0)   && (vflag) )    ||
                 ( (posSampleHandlerNodePtr->horizontalMagnitude != 0) && (hflag) )    ||
                 ( (posSampleHandlerNodePtr->verticalMagnitude == 0)
                    && (posSampleHandlerNodePtr->horizontalMagnitude == 0) )    )
            {
                if (posSampleNodePtr == NULL)
                {
                    // Create the position sample node.
                    posSampleNodePtr = (le_pos_Sample_t*)le_mem_ForceAlloc(PosSamplePoolRef);
                    posSampleNodePtr->latitudeValid = CHECK_VALIDITY(latitude,INT32_MAX);
                    posSampleNodePtr->latitude = latitude;

                    posSampleNodePtr->longitudeValid = CHECK_VALIDITY(longitude,INT32_MAX);
                    posSampleNodePtr->longitude = longitude;

                    posSampleNodePtr->hAccuracyValid = CHECK_VALIDITY(hAccuracy,INT32_MAX);
                    posSampleNodePtr->hAccuracy = hAccuracy;

                    posSampleNodePtr->altitudeValid = CHECK_VALIDITY(altitude,INT32_MAX);
                    posSampleNodePtr->altitude = altitude;

                    posSampleNodePtr->vAccuracyValid = CHECK_VALIDITY(vAccuracy,INT32_MAX);
                    posSampleNodePtr->vAccuracy = vAccuracy;

                    // Get horizontal speed
                    le_gnss_GetHorizontalSpeed( positionSampleRef
                                                , &hSpeed
                                                , &hSpeedAccuracy);
                    posSampleNodePtr->hSpeedValid = CHECK_VALIDITY(hSpeed,UINT32_MAX);
                    posSampleNodePtr->hSpeed = hSpeed;
                    posSampleNodePtr->hSpeedAccuracyValid =
                                                        CHECK_VALIDITY(hSpeedAccuracy,UINT32_MAX);
                    posSampleNodePtr->hSpeedAccuracy = hSpeedAccuracy;

                    // Get vertical speed
                    le_gnss_GetVerticalSpeed( positionSampleRef
                                            , &vSpeed
                                            , &vSpeedAccuracy);
                    posSampleNodePtr->vSpeedValid = CHECK_VALIDITY(vSpeed,INT32_MAX);
                    posSampleNodePtr->vSpeed = vSpeed;
                    posSampleNodePtr->vSpeedAccuracyValid =
                                                        CHECK_VALIDITY(vSpeedAccuracy,INT32_MAX);
                    posSampleNodePtr->vSpeedAccuracy = vSpeedAccuracy;

                    // Heading not supported by GNSS engine
                    posSampleNodePtr->headingValid = false;
                    posSampleNodePtr->heading = UINT32_MAX;
                    posSampleNodePtr->headingAccuracyValid = false;
                    posSampleNodePtr->headingAccuracy = UINT32_MAX;

                    // Get direction
                    le_gnss_GetDirection( positionSampleRef
                                        , &direction
                                        , &directionAccuracy);

                    posSampleNodePtr->directionValid = CHECK_VALIDITY(direction,UINT32_MAX);
                    posSampleNodePtr->direction = direction;
                    posSampleNodePtr->directionAccuracyValid =
                                                    CHECK_VALIDITY(directionAccuracy,UINT32_MAX);
                    posSampleNodePtr->directionAccuracy = directionAccuracy;

                    // Get UTC time
                    if (LE_OK == le_gnss_GetDate(positionSampleRef
                                            , &year
                                            , &month
                                            , &day))
                    {
                        posSampleNodePtr->dateValid = true;
                    }
                    else
                    {
                        posSampleNodePtr->dateValid = false;
                    }
                    posSampleNodePtr->year = year;
                    posSampleNodePtr->month = month;
                    posSampleNodePtr->day = day;

                    if (LE_OK == le_gnss_GetTime(positionSampleRef
                                        , &hours
                                        , &minutes
                                        , &seconds
                                        , &milliseconds))
                    {
                        posSampleNodePtr->timeValid = true;
                    }
                    else
                    {
                        posSampleNodePtr->timeValid = false;
                    }
                    posSampleNodePtr->hours = hours;
                    posSampleNodePtr->minutes = minutes;
                    posSampleNodePtr->seconds = seconds;
                    posSampleNodePtr->milliseconds = milliseconds;

                    posSampleNodePtr->link = LE_DLS_LINK_INIT;

                    // Add the node to the queue of the list by passing in the node's link.
                    le_dls_Queue(&PosSampleList, &(posSampleNodePtr->link));
                }

                // Save the information reported to the handler function
                posSampleHandlerNodePtr->lastLat = latitude;
                posSampleHandlerNodePtr->lastLong = longitude;
                posSampleHandlerNodePtr->lastAlt = altitude;

                LE_DEBUG("Report sample %p to the corresponding handler (handler %p)",
                         posSampleNodePtr,
                         posSampleHandlerNodePtr->handlerFuncPtr);

                uint8_t i;
                for(i=0 ; i<NumOfHandlers-1 ; i++)
                {
                    le_mem_AddRef((void *)posSampleNodePtr);
                }

                // Call the client's handler
                posSampleHandlerNodePtr->handlerFuncPtr(
                                                le_ref_CreateRef(PosSampleMap, posSampleNodePtr),
                                                posSampleHandlerNodePtr->handlerContextPtr);
            }

            // Move to the next node.
            linkPtr = le_dls_PeekNext(&PosSampleHandlerList, linkPtr);
        } while (linkPtr != NULL);
    }

    // Release provided Position sample reference
    le_gnss_ReleaseSampleRef(positionSampleRef);
}

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

    LE_DEBUG("New acquisition rate (%d) for positioning",AcqRate);

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
    LE_DEBUG("Set acquisition rate to value %d", AcqRate);

    // Add a configDb handler to check if the acquition rate change.
    le_cfg_AddChangeHandler(CFG_POSITIONING_RATE_PATH, AcquitisionRateUpdate,NULL);

    le_cfg_CancelTxn(posCfg);
}


//--------------------------------------------------------------------------------------------------
/**
* handler function to release Positioning service
*
*/
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void* contextPtr
)
{
    LE_ERROR("SessionRef (%p) has been closed", sessionRef);

    if (!sessionRef)
    {
        LE_ERROR("ERROR sessionRef is NULL");
    }

    // Shearch for all positioning control request references used by the current client session
    //  that has been closed.
    le_ref_IterRef_t iterRef = le_ref_GetIterator(ActivationRequestRefMap);
    le_result_t result = le_ref_NextNode(iterRef);
    while ( result == LE_OK )
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
    PosSamplePoolRef = le_mem_CreatePool("PosSamplePoolRef", sizeof(le_pos_Sample_t));
    le_mem_ExpandPool(PosSamplePoolRef,POSITIONING_SAMPLE_MAX);
    le_mem_SetDestructor(PosSamplePoolRef, PosSampleDestructor);

    // Initialize the event client close function handler.
    le_msg_ServiceRef_t msgService = le_posCtrl_GetServiceRef();
    le_msg_AddServiceCloseHandler( msgService, CloseSessionEventHandler, NULL);

    // Create a pool for Position Sample Handler objects
    PosSampleHandlerPoolRef = le_mem_CreatePool("PosSampleHandlerPoolRef",
                                                sizeof(le_pos_SampleHandler_t));
    le_mem_SetDestructor(PosSampleHandlerPoolRef, PosSampleHandlerDestructor);


    // Create the reference HashMap for positioning sample
    PosSampleMap = le_ref_CreateMap("PosSampleMap", POSITIONING_SAMPLE_MAX);

    NumOfHandlers = 0;
    GnssHandlerRef = NULL;

    // Create safe reference map for request references. The size of the map should be based on
    // the expected number of simultaneous data requests, so take a reasonable guess.
    ActivationRequestRefMap = le_ref_CreateMap("Positioning Client", POSITIONING_ACTIVATION_MAX);

    // Create a pool for Position control client objects.
    PosCtrlHandlerPoolRef = le_mem_CreatePool("PosCtrlHandlerPoolRef", sizeof(ClientRequest_t));
    le_mem_ExpandPool(PosCtrlHandlerPoolRef,POSITIONING_ACTIVATION_MAX);

    // TODO define a policy for positioning device selection
    if (IsGNSSAvailable() == true)
    {
        gnss_Init();
        LoadPositioningFromConfigDb();
    }
    else
    {
        LE_CRIT("GNSS module not available");
    }

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
            LE_WARN("Failed to set GNSS's acquisition rate (%d)", AcqRate);
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

    LE_DEBUG("le_posCtrl_Request ref (%p), SessionRef (%p)" ,reqRef, msgSession);

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
    if ( posPtr == NULL )
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
                                                      ///     0 means that I don't care about
                                                      ///     changes in the latitude and longitude.
    uint32_t                     verticalMagnitude,   ///< [IN] The vertical magnitude in meters.
                                                      ///     0 means that I don't care about
                                                      ///     changes in the altitude.
    le_pos_MovementHandlerFunc_t handlerPtr,          ///< [IN] The handler function.
    void*                        contextPtr           ///< [IN] The context pointer
)
{
    uint32_t                 rate = 0;
    le_pos_SampleHandler_t*  posSampleHandlerNodePtr=NULL;

    LE_FATAL_IF((handlerPtr == NULL), "handlerPtr pointer is NULL !");

    // Create the position sample handler node.
    posSampleHandlerNodePtr = (le_pos_SampleHandler_t*)le_mem_ForceAlloc(PosSampleHandlerPoolRef);
    posSampleHandlerNodePtr->handlerFuncPtr = handlerPtr;
    posSampleHandlerNodePtr->handlerContextPtr = contextPtr;
    posSampleHandlerNodePtr->acquisitionRate = CalculateAcquisitionRate(SUPPOSED_AVERAGE_SPEED,
                                                                        horizontalMagnitude,
                                                                        verticalMagnitude);

    AcqRate = ComputeCommonSmallestRate(posSampleHandlerNodePtr->acquisitionRate);

    LE_DEBUG("Computed Acquisistion rate is %d sec for an average speed of %d km/h",
             rate,
             SUPPOSED_AVERAGE_SPEED);

    // update configDB with the new rate.
    {
        // Add the default acquisition rate,.
        le_cfg_IteratorRef_t posCfg = le_cfg_CreateWriteTxn(CFG_POSITIONING_PATH);
        // enter default the value
        le_cfg_SetInt(posCfg,CFG_NODE_RATE,rate);
        le_cfg_CommitTxn(posCfg);
    }

    posSampleHandlerNodePtr->horizontalMagnitude = horizontalMagnitude;
    posSampleHandlerNodePtr->verticalMagnitude = verticalMagnitude;

    // Start acquisition
    if (NumOfHandlers == 0)
    {
        if ((GnssHandlerRef=le_gnss_AddPositionHandler(PosSampleHandlerfunc, NULL)) == NULL)
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
                                                             le_pos_SampleHandler_t, link);
            // Check the node.
            if ( (le_pos_MovementHandlerRef_t)posSampleHandlerNodePtr == handlerRef )
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
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (latitudePtr)
    {
        if (positionSamplePtr->latitudeValid)
        {
            *latitudePtr = positionSamplePtr->latitude;
        } else {
            *latitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (longitudePtr)
    {
        if (positionSamplePtr->longitudeValid)
        {
            *longitudePtr = positionSamplePtr->longitude;
        } else {
            *longitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (horizontalAccuracyPtr)
    {
        if (positionSamplePtr->hAccuracyValid)
        {
            *horizontalAccuracyPtr = positionSamplePtr->hAccuracy/10; // Update resolution
        } else {
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
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (positionSamplePtr->timeValid)
    {
        result = LE_OK;
        if (hoursPtr)
        {
            *hoursPtr = positionSamplePtr->hours;
        }
        if (minutesPtr)
        {
            *minutesPtr = positionSamplePtr->minutes;
        }
        if (secondsPtr)
        {
            *secondsPtr = positionSamplePtr->seconds;
        }
        if (millisecondsPtr)
        {
            *millisecondsPtr = positionSamplePtr->milliseconds;
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
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (positionSamplePtr->dateValid)
    {
        result = LE_OK;
        if (yearPtr)
        {
            *yearPtr = positionSamplePtr->year;
        }
        if (monthPtr)
        {
            *monthPtr = positionSamplePtr->month;
        }
        if (dayPtr)
        {
            *dayPtr = positionSamplePtr->day;
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
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (altitudePtr)
    {
        if (positionSamplePtr->altitudeValid)
    {
            *altitudePtr = positionSamplePtr->altitude/1000; // Update resolution
        } else {
            *altitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (altitudeAccuracyPtr)
    {
        if (positionSamplePtr->vAccuracyValid)
        {
            *altitudeAccuracyPtr = positionSamplePtr->vAccuracy/10; // Update resolution
        } else {
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
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX,
 *                          UINT32_MAX).
 * @return LE_OK            Function succeeded.
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
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (hSpeedPtr)
    {
        if (positionSamplePtr->hSpeedValid)
        {
            *hSpeedPtr = positionSamplePtr->hSpeed/100; // Update resolution
        } else {
            *hSpeedPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (hSpeedAccuracyPtr)
    {
        if (positionSamplePtr->hSpeedAccuracyValid)
        {
            *hSpeedAccuracyPtr = positionSamplePtr->hSpeedAccuracy/10; // Update resolution
        } else {
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
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (vSpeedPtr)
    {
        if (positionSamplePtr->vSpeedValid)
        {
            *vSpeedPtr = positionSamplePtr->vSpeed/100; // Update resolution
        } else {
            *vSpeedPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vSpeedAccuracyPtr)
    {
        if (positionSamplePtr->vSpeedAccuracyValid)
        {
            *vSpeedAccuracyPtr = positionSamplePtr->vSpeedAccuracy/10; // Update resolution
        } else {
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
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (headingPtr)
    {
        if (positionSamplePtr->headingValid)
        {
            *headingPtr = positionSamplePtr->heading;
        } else {
            *headingPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (headingAccuracyPtr)
    {
        if (positionSamplePtr->headingAccuracyValid)
        {
            *headingAccuracyPtr = positionSamplePtr->headingAccuracy;
        } else {
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
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (directionPtr)
    {
        if (positionSamplePtr->directionValid)
        {
            *directionPtr = positionSamplePtr->direction/10; // Update resolution
        } else {
            *directionPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (directionAccuracyPtr)
    {
        if (positionSamplePtr->directionAccuracyValid)
        {
            *directionAccuracyPtr = positionSamplePtr->directionAccuracy/10; // Update resolution
        } else {
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
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (statePtr)
    {
        *statePtr = positionSamplePtr->fixState;
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
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return;
    }
    le_ref_DeleteRef(PosSampleMap,positionSampleRef);
    le_mem_Release(positionSamplePtr);
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
    int32_t     latitude;
    int32_t     longitude;
    int32_t     hAccuracy;
    le_result_t gnssResult = LE_OK;
    le_result_t posResult = LE_OK;
    le_gnss_SampleRef_t positionSampleRef = le_gnss_GetLastSampleRef();

    // At least one valid pointer
    if (( latitudePtr == NULL)&&
        ( longitudePtr == NULL)&&
        ( hAccuracyPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid reference provided!");
        return LE_FAULT;
    }

    // Get Location
    gnssResult = le_gnss_GetLocation( positionSampleRef
                                , &latitude
                                , &longitude
                                , &hAccuracy);
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
                *hAccuracyPtr = hAccuracy/10; // Update resolution
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
    int32_t     latitude;
    int32_t     longitude;
    int32_t     hAccuracy;
    int32_t     altitude;
    int32_t     vAccuracy;
    le_result_t gnssResult = LE_OK;
    le_result_t posResult = LE_OK;
    le_gnss_SampleRef_t positionSampleRef = le_gnss_GetLastSampleRef();

    // At least one valid pointer
    if (( latitudePtr == NULL)&&
        ( longitudePtr == NULL)&&
        ( hAccuracyPtr == NULL)&&
        ( altitudePtr == NULL)&&
        ( vAccuracyPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid reference provided!");
        return LE_FAULT;
    }

    // Get Location
    gnssResult = le_gnss_GetLocation( positionSampleRef
                                , &latitude
                                , &longitude
                                , &hAccuracy);
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
                *hAccuracyPtr = hAccuracy/10; // Update resolution
            }
        }
    }
    else
    {
        posResult = LE_FAULT;
    }

    // Get altitude
    gnssResult = le_gnss_GetAltitude( positionSampleRef
                                    , &altitude
                                    , &vAccuracy);

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
                *altitudePtr = altitude/1000; // Update resolution
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
                *vAccuracyPtr = vAccuracy/10; // Update resolution
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
    le_result_t posResult = LE_OK;
    le_gnss_SampleRef_t positionSampleRef = le_gnss_GetLastSampleRef();

    // All pointers must be valid
    if (( hoursPtr == NULL)||
        ( minutesPtr == NULL)||
        ( secondsPtr == NULL)||
        ( millisecondsPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid reference provided!");
        return LE_FAULT;
    }

    // Get UTC time
    posResult = le_gnss_GetTime(positionSampleRef
                            , hoursPtr
                            , minutesPtr
                            , secondsPtr
                            , millisecondsPtr);

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
    le_result_t posResult = LE_OK;
    le_gnss_SampleRef_t positionSampleRef = le_gnss_GetLastSampleRef();

    // All pointers must be valid
    if (( yearPtr == NULL)||
        ( monthPtr == NULL)||
        ( dayPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid reference provided!");
        return LE_FAULT;
    }

    // Get date
    posResult = le_gnss_GetDate(positionSampleRef
                                , yearPtr
                                , monthPtr
                                , dayPtr);

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
    // Horizontal speed
    uint32_t hSpeed;
    uint32_t hSpeedAccuracy;
    // Vertical speed
    int32_t vSpeed;
    int32_t vSpeedAccuracy;
    le_result_t gnssResult = LE_OK;
    le_result_t posResult = LE_OK;
    le_gnss_SampleRef_t positionSampleRef = le_gnss_GetLastSampleRef();

    // At least one valid pointer
    if (( hSpeedPtr == NULL)&&
        ( hSpeedAccuracyPtr == NULL)&&
        ( vSpeedPtr == NULL)&&
        ( vSpeedAccuracyPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid reference provided!");
        return LE_FAULT;
    }

   // Get horizontal speed
    gnssResult = le_gnss_GetHorizontalSpeed( positionSampleRef
                                            , &hSpeed
                                            , &hSpeedAccuracy);
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
    gnssResult = le_gnss_GetVerticalSpeed( positionSampleRef
                                        , &vSpeed
                                        , &vSpeedAccuracy);

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
    if (( headingPtr == NULL)&&
        ( headingAccuracyPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid reference provided!");
        return LE_FAULT;
    }

    // Heading indication not supported by GNSS feature.
    *headingPtr = UINT32_MAX;
    *headingAccuracyPtr = UINT32_MAX;
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
    // Direction
    uint32_t direction;
    uint32_t directionAccuracy;
    le_result_t gnssResult = LE_OK;
    le_result_t posResult = LE_OK;
    le_gnss_SampleRef_t positionSampleRef = le_gnss_GetLastSampleRef();

    // At least one valid pointer
    if (( directionPtr == NULL)&&
        ( directionAccuracyPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid reference provided!");
        return LE_FAULT;
    }

    // Get direction
    gnssResult = le_gnss_GetDirection( positionSampleRef
                                    , &direction
                                    , &directionAccuracy);

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
    le_gnss_GetPositionState( positionSampleRef, &gnssState);

    *statePtr = (le_pos_FixState_t)gnssState;

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
    if (!acquisitionRate)
    {
        LE_WARN("Invalid acquisition rate (%d)", acquisitionRate);
        return LE_OUT_OF_RANGE;
    }

    le_cfg_IteratorRef_t posCfg = le_cfg_CreateWriteTxn(CFG_POSITIONING_PATH);
    le_cfg_SetInt(posCfg,CFG_NODE_RATE,acquisitionRate);
    le_cfg_CommitTxn(posCfg);

    return LE_OK;
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
    uint32_t  acquisitionRate;

    le_cfg_IteratorRef_t posCfg = le_cfg_CreateReadTxn(CFG_POSITIONING_PATH);
    acquisitionRate = le_cfg_GetInt(posCfg, CFG_NODE_RATE, DEFAULT_ACQUISITION_RATE);
    le_cfg_CancelTxn(posCfg);

    LE_DEBUG("acquisition rate (%d) for positioning",acquisitionRate);
    return acquisitionRate;
}

