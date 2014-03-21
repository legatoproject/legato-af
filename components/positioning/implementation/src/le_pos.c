//--------------------------------------------------------------------------------------------------
/**
 * @file le_pos.c
 *
 * This file contains the source code of the high level Positioning API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------


#include "legato.h"
#include "le_pos.h"
#include "pa_gnss.h"
#include "le_cfg_interface.h"
#include "cfgEntries.h"

#include <math.h>

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
// TODO move to configuration module? or retrieve it from a speedometer?
//--------------------------------------------------------------------------------------------------
#define SUPPOSED_AVERAGE_SPEED       50  // 50 km/h
#define DEFAULT_ACQUISITION_RATE     5   // 5 seconds


// To compute the estimated horizontal error, we assume that the GNSS's User Equivalent Range Error
// (UERE) is equivalent for all the satellites, for civil application UERE is approximatively 7
// meters.
#define GNSS_UERE                                 7
#define GNSS_ESTIMATED_VERTICAL_ERROR_FACTOR      GNSS_UERE * 1.5

#define POSITIONING_SAMPLE_MAX         1

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
    int32_t         hSpeedAccuracy;  ///< horizontal speed accuracy
    bool            vSpeedValid;     ///< if true, vertical speed is set
    int32_t         vSpeed;          ///< vertical speed
    bool            vSpeedAccuracyValid; ///< if true, vertical speed accuracy is set
    int32_t         vSpeedAccuracy;  ///< vertical speed accuracy
    bool            headingValid;    ///< if true, heading is set
    int32_t         heading;         ///< heading
    bool            headingAccuracyValid; ///< if true, heading accuracy is set
    int32_t         headingAccuracy; ///< heading accuracy
    bool            directionValid;  ///< if true, direction is set
    int32_t         direction;       ///< direction
    bool            directionAccuracyValid; ///< if true, direction accuracy is set
    int32_t         directionAccuracy; ///< direction accuracy
    le_dls_Link_t   link;            ///< Object node link
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
    uint32_t                     horizontalMagnitude; ///< The horizontal magnitude in metres for this handler.
    uint32_t                     verticalMagnitude;   ///< The vertical magnitude in metres for this handler.
    int32_t                      lastLat;             ///< The altitude associated with the last handler's notification.
    int32_t                      lastLong;            ///< The longitude associated with the last handler's notification.
    int32_t                      lastAlt;             ///< The altitude associated with the last handler's notification.
    le_dls_Link_t                link;                ///< Object node link
}
le_pos_SampleHandler_t;

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Create and initialize the position samples list.
 *
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t PosSampleList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Create and initialize the position sample's handlers list.
 *
 */
//--------------------------------------------------------------------------------------------------
le_dls_List_t PosSampleHandlerList = LE_DLS_LIST_INIT;

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
static le_event_HandlerRef_t PAHandlerRef = NULL;


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
            posSampleHandlerNodePtr = (le_pos_SampleHandler_t*)CONTAINER_OF(linkPtr, le_pos_SampleHandler_t, link);
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
    uint32_t horizontalMagnitude,  // The horizontal magnitude in metres.
    uint32_t verticalMagnitude     // The vertical magnitude in metres.
)
{
    uint32_t  rate;
    uint32_t  metersec = averageSpeed*1000/3600; // convert speed in m/sec

    if ((horizontalMagnitude<metersec) || (verticalMagnitude<metersec))
    {
        return 1;
    }

    rate = 0;
    while (! (((horizontalMagnitude >= (metersec + metersec*rate)) && (verticalMagnitude >= (metersec + metersec*rate))) &&
             ((horizontalMagnitude < (metersec + metersec*(rate+1))) || (verticalMagnitude < (metersec + metersec*(rate+1))))))
    {
        rate++;
    }
    return rate + 2;
}

//--------------------------------------------------------------------------------------------------
/**
 * Calculate the distance in metres between two fix points (use Haversine formula).
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
    // distance = R.c.1000 (in metres)
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

    LE_DEBUG("Computed distance is %e metres (double)", (double)(R * c * 1000));
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
            posSampleHandlerNodePtr = (le_pos_SampleHandler_t*)CONTAINER_OF(linkPtr, le_pos_SampleHandler_t, link);
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
    pa_Gnss_Position_Ref_t position
)
{
    le_pos_SampleHandler_t* posSampleHandlerNodePtr;
    le_dls_Link_t*          linkPtr;
    le_pos_Sample_t*        posSampleNodePtr=NULL;

    if(!NumOfHandlers)
    {
        return;
    }

    LE_DEBUG("Handler Function called with pa_position %p", position);

    linkPtr = le_dls_Peek(&PosSampleHandlerList);
    if (linkPtr != NULL)
    {
        do
        {
            bool hflag, vflag;
            // Get the node from the list
            posSampleHandlerNodePtr = (le_pos_SampleHandler_t*)CONTAINER_OF(linkPtr, le_pos_SampleHandler_t, link);

            if (   (posSampleHandlerNodePtr->horizontalMagnitude != 0)
                &&
                   ( (!position->longitudeValid) || (!position->latitudeValid) )
               )
            {
                LE_DEBUG("Longitude or Latitude are not relevant");
                return;
            }

            if (((posSampleHandlerNodePtr->verticalMagnitude != 0) && (!position->altitudeValid)))
            {
                LE_DEBUG("Altitude is not relevant");
                return;
            }

            uint32_t horizontalMove = ComputeDistance(posSampleHandlerNodePtr->lastLat,
                                                      posSampleHandlerNodePtr->lastLong,
                                                      position->latitude,
                                                      position->longitude);
            uint32_t verticalMove = abs(position->altitude-posSampleHandlerNodePtr->lastAlt);

            LE_DEBUG("horizontalMove.%d, verticalMove.%d", horizontalMove, verticalMove);

            if (!position->vUncertaintyValid) {
                vflag = false;
            } else {
                vflag = IsBeyondMagnitude(posSampleHandlerNodePtr->verticalMagnitude,
                                          verticalMove,
                                          position->vUncertainty/10); // uncertainty in meters with 1 decimal place
            }

            if (!position->hUncertaintyValid) {
                hflag = false;
            } else {
                hflag = IsBeyondMagnitude(posSampleHandlerNodePtr->horizontalMagnitude,
                                          horizontalMove,
                                          position->hUncertainty/10); // uncertainty in meters with 1 decimal place
            }

            LE_DEBUG("Vertical IsBeyondMagnitude.%d", vflag);
            LE_DEBUG("Horizontal IsBeyondMagnitude.%d", hflag);

            if ( ( (posSampleHandlerNodePtr->verticalMagnitude != 0)   && (vflag) )    ||
                 ( (posSampleHandlerNodePtr->horizontalMagnitude != 0) && (hflag) ) )
            {
                if(posSampleNodePtr == NULL)
                {
                    // Create the position sample node.
                    posSampleNodePtr = (le_pos_Sample_t*)le_mem_ForceAlloc(PosSamplePoolRef);
                    posSampleNodePtr->latitudeValid = position->latitudeValid;
                    posSampleNodePtr->latitude = position->latitude;
                    posSampleNodePtr->longitudeValid = position->longitudeValid;
                    posSampleNodePtr->longitude = position->longitude;
                    posSampleNodePtr->hAccuracyValid = position->hUncertaintyValid;
                    posSampleNodePtr->hAccuracy = position->hUncertainty;
                    posSampleNodePtr->altitudeValid = position->altitudeValid;
                    posSampleNodePtr->altitude = position->altitude;
                    posSampleNodePtr->vAccuracyValid = position->vUncertaintyValid;
                    posSampleNodePtr->vAccuracy = position->vUncertainty;
                    posSampleNodePtr->hSpeedValid = position->hSpeedValid;
                    posSampleNodePtr->hSpeed = position->hSpeed;
                    posSampleNodePtr->hSpeedAccuracyValid = position->hSpeedUncertaintyValid;
                    posSampleNodePtr->hSpeedAccuracy = position->hSpeedUncertainty;
                    posSampleNodePtr->vSpeedValid = position->vSpeedValid;
                    posSampleNodePtr->vSpeed = position->vSpeed;
                    posSampleNodePtr->vSpeedAccuracyValid = position->vSpeedUncertaintyValid;
                    posSampleNodePtr->vSpeedAccuracy = position->vSpeedUncertainty;
                    posSampleNodePtr->headingValid = position->headingValid;
                    posSampleNodePtr->heading = position->heading;
                    posSampleNodePtr->headingAccuracyValid = position->headingUncertaintyValid;
                    posSampleNodePtr->headingAccuracy = position->headingUncertainty;
                    posSampleNodePtr->directionValid = position->trackValid;
                    posSampleNodePtr->direction = position->track;
                    posSampleNodePtr->directionAccuracyValid = position->trackUncertaintyValid;
                    posSampleNodePtr->directionAccuracy = position->trackUncertainty;
                    posSampleNodePtr->link = LE_DLS_LINK_INIT;

                    // Add the node to the queue of the list by passing in the node's link.
                    le_dls_Queue(&PosSampleList, &(posSampleNodePtr->link));
                }

                // Save the infirmation reported to the handler function
                posSampleHandlerNodePtr->lastLat = position->latitude;
                posSampleHandlerNodePtr->lastLong = position->longitude;
                posSampleHandlerNodePtr->lastAlt = position->altitude;

                LE_DEBUG("Report sample %p to the corresponding handler (handler %p)",
                         posSampleNodePtr,
                         posSampleHandlerNodePtr->handlerFuncPtr);

                uint8_t i;
                for(i=0 ; i<NumOfHandlers-1 ; i++)
                {
                    le_mem_AddRef((void *)posSampleNodePtr);
                }

                // Call the client's handler
                posSampleHandlerNodePtr->handlerFuncPtr(le_ref_CreateRef(PosSampleMap, posSampleNodePtr),
                                                        posSampleHandlerNodePtr->handlerContextPtr);
            }

            // Move to the next node.
            linkPtr = le_dls_PeekNext(&PosSampleHandlerList, linkPtr);
        } while (linkPtr != NULL);
    }

    le_mem_Release(position);
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

    char configPath[LIMIT_MAX_PATH_BYTES];
    snprintf(configPath, sizeof(configPath), "%s/%s",
             LEGATO_CONFIG_TREE_ROOT_DIR,
             CFG_NODE_POSITIONING);

    le_cfg_iteratorRef_t posCfg = le_cfg_CreateReadTxn(configPath);

    int32_t rate = le_cfg_GetInt(posCfg,CFG_NODE_RATE);

    // Set the new value for acquitision rate
    if (pa_gnss_SetAcquisitionRate(rate) != LE_OK)
    {
        LE_WARN("Failed to set GNSS's acquisition rate to %d seconds!",rate);
    }

    LE_DEBUG("New acquisition rate (%d) for positioning",rate);

    le_cfg_DeleteIterator(posCfg);
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
    // Default configutation
    int32_t rate = DEFAULT_ACQUISITION_RATE;

    // Check that the app has a configuration value.
    le_cfg_iteratorRef_t posCfg = le_cfg_CreateReadTxn(CFG_POSITIONING_PATH);

    if ( le_cfg_IsEmpty(posCfg,CFG_NODE_RATE) )
    {
        LE_WARN("No rate configuration set for positioning, Initialize the default one");
        le_cfg_DeleteIterator(posCfg);

        // Add the default acquisition rate,.
        posCfg = le_cfg_CreateWriteTxn(CFG_POSITIONING_PATH);
        // enter default the value
        le_cfg_SetInt(posCfg,CFG_NODE_RATE,rate);

        le_result_t result = le_cfg_CommitWrite(posCfg);
        LE_FATAL_IF(result!=LE_OK,"Could not commit changes, result == %s.",LE_RESULT_TXT(result));

        posCfg = le_cfg_CreateReadTxn(CFG_POSITIONING_PATH);
    }
    else
    {
        rate = le_cfg_GetInt(posCfg,CFG_NODE_RATE);
    }

    LE_DEBUG("Set acquisition rate to value %d", rate);
    // TODO: how to stop/start device in order to limit power consumption?
    LE_FATAL_IF((pa_gnss_SetAcquisitionRate(rate) != LE_OK),
                "Failed to set GNSS's acquisition rate!");

    // Add a configDb handler to check if the acquition rate change.
    le_cfg_AddChangeHandler(CFG_POSITIONING_RATE_PATH, AcquitisionRateUpdate,NULL);

    le_cfg_DeleteIterator(posCfg);
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
void le_pos_Init
(
    void
)
{
    // start the configDB client.
    le_cfg_Initialize();

    // Create a pool for Position Sample objects
    PosSamplePoolRef = le_mem_CreatePool("PosSamplePoolRef", sizeof(le_pos_Sample_t));
    le_mem_ExpandPool(PosSamplePoolRef,POSITIONING_SAMPLE_MAX);
    le_mem_SetDestructor(PosSamplePoolRef, PosSampleDestructor);

    // Create a pool for Position Sample Handler objects
    PosSampleHandlerPoolRef = le_mem_CreatePool("PosSampleHandlerPoolRef", sizeof(le_pos_SampleHandler_t));
    le_mem_SetDestructor(PosSampleHandlerPoolRef, PosSampleHandlerDestructor);

    // Create the reference HashMap for positioning sample
    PosSampleMap = le_ref_CreateMap("PosSampleMap", POSITIONING_SAMPLE_MAX);

    NumOfHandlers = 0;
    PAHandlerRef = NULL;

    // TODO define a policy for positioning device selection
    if (IsGNSSAvailable() == true)
    {
        if (pa_gnss_Init() != LE_OK)
        {
            LE_EMERG("Failed to initialize the PA GNSS module");
        } else
        {
            LoadPositioningFromConfigDb();

            LE_FATAL_IF((pa_gnss_Start() != LE_OK),
                        "Failed to start GNSS's acquisition!");
        }
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
    uint32_t                     horizontalMagnitude, ///< [IN] The horizontal magnitude in metres.
                                                      ///       0 means that I don't care about
                                                      ///       changes in the latitude and longitude.
    uint32_t                     verticalMagnitude,   ///< [IN] The vertical magnitude in metres.
                                                      ///       0 means that I don't care about
                                                      ///       changes in the altitude.
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

    rate = ComputeCommonSmallestRate(posSampleHandlerNodePtr->acquisitionRate);

    LE_DEBUG("Computed Acquisistion rate is %d sec for an average speed of %d km/h",
             rate,
             SUPPOSED_AVERAGE_SPEED);

    // update configDB with the new rate.
    {
        // Add the default acquisition rate,.
        le_cfg_iteratorRef_t posCfg = le_cfg_CreateWriteTxn(CFG_POSITIONING_PATH);
        // enter default the value
        le_cfg_SetInt(posCfg,CFG_NODE_RATE,rate);

        le_result_t result = le_cfg_CommitWrite(posCfg);
        LE_FATAL_IF(result!=LE_OK,"Could not commit changes, result == %s.",LE_RESULT_TXT(result));
    }

    posSampleHandlerNodePtr->horizontalMagnitude = horizontalMagnitude;
    posSampleHandlerNodePtr->verticalMagnitude = verticalMagnitude;

    LE_FATAL_IF((pa_gnss_SetAcquisitionRate(rate) != LE_OK),
                "Failed to set GNSS's acquisition rate!");

    le_dls_Queue(&PosSampleHandlerList, &(posSampleHandlerNodePtr->link));

    // Start acquisition
    if (NumOfHandlers == 0)
    {
//         LE_ERROR("Add PA handler");
        LE_FATAL_IF(((PAHandlerRef=pa_gnss_AddPositionDataHandler(PosSampleHandlerfunc)) == NULL),
                    "Failed to add PA GNSS's handler!");

// TODO: how to stop/start device in order to limit power consumption?
//         LE_FATAL_IF((pa_gnss_Start() != LE_OK),
//                     "Failed to start GNSS's acquisition!");
    }

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
            posSampleHandlerNodePtr = (le_pos_SampleHandler_t*)CONTAINER_OF(linkPtr, le_pos_SampleHandler_t, link);
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

    if(NumOfHandlers == 0)
    {
        pa_gnss_RemovePositionDataHandler(PAHandlerRef);
        pa_gnss_Stop();
        PAHandlerRef = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the position sample's 2D location (latitude, longitude,
 * horizontal accuracy).
 *
 * @return LE_FAULT         The function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX).
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note latitudePtr, longitudePtr, horizontalAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_Get2DLocation
(
    le_pos_SampleRef_t  positionSampleRef,    ///< [IN] The position sample's reference.
    int32_t*            latitudePtr,          ///< [OUT] The latitude in degrees.
    int32_t*            longitudePtr,         ///< [OUT] The longitude in degrees.
    int32_t*            horizontalAccuracyPtr ///< [OUT] The horizontal's accuracy estimate in metres.
)
{
    le_result_t result = LE_OK;
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL) {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (latitudePtr) {
        if (positionSamplePtr->latitudeValid) {
            *latitudePtr = positionSamplePtr->latitude;
        } else {
            *latitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (longitudePtr) {
        if (positionSamplePtr->longitudeValid) {
            *longitudePtr = positionSamplePtr->longitude;
        } else {
            *longitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (horizontalAccuracyPtr) {
        if (positionSamplePtr->hAccuracyValid) {
            *horizontalAccuracyPtr = positionSamplePtr->hAccuracy/10;
        } else {
            *horizontalAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the position sample's altitude.
 *
 * @return LE_FAULT         The function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX).
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note altitudePtr, altitudeAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetAltitude
(
    le_pos_SampleRef_t  positionSampleRef,   ///< [IN] The position sample's reference.
    int32_t*            altitudePtr,         ///< [OUT] The altitude.
    int32_t*            altitudeAccuracyPtr  ///< [OUT] The altitude's accuracy estimate.
)
{
    le_result_t result = LE_OK;
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL) {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (altitudePtr) {
        if (positionSamplePtr->altitudeValid) {
            *altitudePtr = positionSamplePtr->altitude;
        } else {
            *altitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (altitudeAccuracyPtr) {
        if (positionSamplePtr->vAccuracyValid) {
            *altitudeAccuracyPtr = positionSamplePtr->vAccuracy;
        } else {
            *altitudeAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the position sample's horizontal speed.
 *
 * @return LE_FAULT         The function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX, UINT32_MAX).
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note hSpeedPtr, hSpeedAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetHorizontalSpeed
(
    le_pos_SampleRef_t  positionSampleRef, ///< [IN] The position sample's reference.
    uint32_t*           hSpeedPtr,         ///< [OUT] The horizontal speed.
    int32_t*            hSpeedAccuracyPtr  ///< [OUT] The horizontal speed's accuracy estimate.
)
{
    le_result_t result = LE_OK;
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL) {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (hSpeedPtr) {
        if (positionSamplePtr->hSpeedValid) {
            *hSpeedPtr = positionSamplePtr->hSpeed;
        } else {
            *hSpeedPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (hSpeedAccuracyPtr) {
        if (positionSamplePtr->hSpeedAccuracyValid) {
            *hSpeedAccuracyPtr = positionSamplePtr->hSpeedAccuracy;
        } else {
            *hSpeedAccuracyPtr = INT32_MAX;
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

    if ( positionSamplePtr == NULL) {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (vSpeedPtr) {
        if (positionSamplePtr->vSpeedValid) {
            *vSpeedPtr = positionSamplePtr->vSpeed;
        } else {
            *vSpeedPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vSpeedAccuracyPtr) {
        if (positionSamplePtr->vSpeedAccuracyValid) {
            *vSpeedAccuracyPtr = positionSamplePtr->vSpeedAccuracy;
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
 * the vehicle/person is facing.
 *
 * @return LE_FAULT         The function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX).
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note headingPtr, headingAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetHeading
(
    le_pos_SampleRef_t positionSampleRef, ///< [IN] The position sample's reference.
    int32_t*           headingPtr,        ///< [OUT] The heading in degrees (where 0 is True North).
    int32_t*           headingAccuracyPtr ///< [OUT] The heading's accuracy estimate.
)
{
    le_result_t result = LE_OK;
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL) {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (headingPtr) {
        if (positionSamplePtr->headingValid) {
            *headingPtr = positionSamplePtr->heading;
        } else {
            *headingPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (headingAccuracyPtr) {
        if (positionSamplePtr->headingAccuracyValid) {
            *headingAccuracyPtr = positionSamplePtr->headingAccuracy;
        } else {
            *headingAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the position sample's direction. Direction of movement is the
 * direction that the vehicle/person is actually moving.
 *
 * @return LE_FAULT         The function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX).
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note directionPtr, directionAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetDirection
(
    le_pos_SampleRef_t  positionSampleRef,    ///< [IN] The position sample's reference.
    int32_t*            directionPtr,         ///< [OUT] The direction.
    int32_t*            directionAccuracyPtr  ///< [OUT] The direction's accuracy estimate.
)
{
    le_result_t result = LE_OK;
    le_pos_Sample_t* positionSamplePtr = le_ref_Lookup(PosSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL) {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (directionPtr) {
        if (positionSamplePtr->directionValid) {
            *directionPtr = positionSamplePtr->direction;
        } else {
            *directionPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (directionAccuracyPtr) {
        if (positionSamplePtr->directionAccuracyValid) {
            *directionAccuracyPtr = positionSamplePtr->directionAccuracy;
        } else {
            *directionAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
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

    if ( positionSamplePtr == NULL) {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return;
    }
    le_ref_DeleteRef(PosSampleMap,positionSampleRef);
    le_mem_Release(positionSamplePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the 2D location's data (Latitude, Longitude, Horizontal
 * accuracy).
 *
 * @return LE_FAULT         The function failed to get the 2D location's data
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX).
 * @return LE_OK            The function succeeded.
 *
 * @note latitudePtr, longitudePtr, hAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_Get2DLocation
(
    int32_t*       latitudePtr,    ///< [OUT] The Latitude in degrees, positive North.
    int32_t*       longitudePtr,   ///< [OUT] The Longitude in degrees, positive East.
    int32_t*       hAccuracyPtr    ///< [OUT] The horizontal position's accuracy.
)
{
    pa_Gnss_Position_t  data;

    // Register a handler function for Call Event indications
    if (pa_gnss_GetLastPositionData(&data) != LE_OK)
    {
        return LE_FAULT;
    }
    else
    {
        le_result_t result = LE_OK;
        if (latitudePtr) {
            if (data.latitudeValid) {
                *latitudePtr = data.latitude;
            } else {
                *latitudePtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        if (longitudePtr) {
            if (data.longitudeValid) {
                *longitudePtr = data.longitude;
            } else {
                *longitudePtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        if (hAccuracyPtr) {
            if (data.hUncertaintyValid) {
                *hAccuracyPtr = data.hUncertainty/10;
            } else {
                *hAccuracyPtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        return result;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the 3D location's data (Latitude, Longitude, Altitude,
 * Horizontal accuracy, Vertical accuracy).
 *
 * @return LE_FAULT         The function failed to get the 3D location's data
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX).
 * @return LE_OK            The function succeeded.
 *
 * @note latitudePtr, longitudePtr,hAccuracyPtr, altitudePtr, vAccuracyPtr can be set to NULL
 *       if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_Get3DLocation
(
    int32_t*       latitudePtr,    ///< [OUT] The Latitude in degrees, positive North.
    int32_t*       longitudePtr,   ///< [OUT] The Longitude in degrees, positive East.
    int32_t*       hAccuracyPtr,   ///< [OUT] The horizontal position's accuracy.
    int32_t*       altitudePtr,    ///< [OUT] The Altitude in metres, above Mean Sea Level.
    int32_t*       vAccuracyPtr    ///< [OUT] The vertical position's accuracy.
)
{
    pa_Gnss_Position_t  data;

    // Register a handler function for Call Event indications
    if (pa_gnss_GetLastPositionData(&data) != LE_OK)
    {
        return LE_FAULT;
    }
    else
    {
        le_result_t result = LE_OK;

        if (latitudePtr) {
            if (data.latitudeValid) {
                *latitudePtr = data.latitude;
            } else {
                *latitudePtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        if (longitudePtr) {
            if (data.longitudeValid) {
                *longitudePtr = data.longitude;
            } else {
                *longitudePtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        if (hAccuracyPtr) {
            if (data.hUncertaintyValid) {
                *hAccuracyPtr = data.hUncertainty/10;
            } else {
                *hAccuracyPtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        if (altitudePtr) {
            if (data.altitudeValid) {
                *altitudePtr = data.altitude;
            } else {
                *altitudePtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        if (vAccuracyPtr) {
            if (data.vUncertaintyValid) {
                *vAccuracyPtr = data.vUncertainty/10;
            } else {
                *vAccuracyPtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        return result;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the motion's data (Horizontal Speed, Horizontal Speed's
 * accuracy, Vertical Speed, Vertical Speed's accuracy).
 *
 * @return LE_FAULT         The function failed to get the motion's data.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX, UINT32_MAX).
 * @return LE_OK            The function succeeded.
 *
 * @note hSpeedPtr, hSpeedAccuracyPtr, vSpeedPtr, vSpeedAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetMotion
(
    uint32_t*   hSpeedPtr,          ///< [OUT] The Horizontal Speed in m/sec.
    int32_t*    hSpeedAccuracyPtr,  ///< [OUT] The Horizontal Speed's accuracy in m/sec.
    int32_t*    vSpeedPtr,          ///< [OUT] The Vertical Speed in m/sec, positive up.
    int32_t*    vSpeedAccuracyPtr   ///< [OUT] The Vertical Speed's accuracy in m/sec.
)
{
    pa_Gnss_Position_t  data;

    // Register a handler function for Call Event indications
    if (pa_gnss_GetLastPositionData(&data) != LE_OK)
    {
        return LE_FAULT;
    }
    else
    {
        le_result_t result = LE_OK;
        // TODO Need speedometer to get these values !
        if (hSpeedPtr) {
            if (data.hSpeedValid) {
                *hSpeedPtr = data.hSpeed;
            } else {
                *hSpeedPtr = UINT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        if (hSpeedAccuracyPtr) {
            if (data.hSpeedUncertaintyValid) {
                *hSpeedAccuracyPtr = data.hSpeedUncertainty;
            } else {
                *hSpeedAccuracyPtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        if (vSpeedPtr) {
            if (data.vSpeedValid) {
                *vSpeedPtr = data.vSpeed;
            } else {
                *vSpeedPtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        if (vSpeedAccuracyPtr) {
            if (data.vSpeedUncertaintyValid) {
                *vSpeedAccuracyPtr = data.vSpeedUncertainty;
            } else {
                *vSpeedAccuracyPtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        return result;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the heading indication.
 *
 * @return LE_FAULT         The function failed to get the heading indication.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX).
 * @return LE_OK            The function succeeded.
 *
 * @note headingPtr, headingAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetHeading
(
    int32_t*  headingPtr,        ///< [OUT] The heading in degrees(where 0 is True North).
    int32_t*  headingAccuracyPtr ///< [OUT] The heading's accuracy.
)
{
    pa_Gnss_Position_t  data;

    // Register a handler function for Call Event indications
    if (pa_gnss_GetLastPositionData(&data) != LE_OK)
    {
        return LE_FAULT;
    }
    else
    {
        le_result_t result = LE_OK;
        // if no compass is available, the heading is the last computed direction
        if (headingPtr) {
            if (data.headingValid) {
                *headingPtr = data.heading;
            } else {
                *headingPtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        if (headingAccuracyPtr) {
            if (data.headingUncertaintyValid) {
                *headingAccuracyPtr = data.headingUncertainty;
            } else {
                *headingAccuracyPtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        return result;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the direction indication. Direction of movement is the
 * direction that the vehicle/person is actually moving.
 *
 * @return LE_FAULT         The function failed to get the direction indication.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is not valid (set to INT32_MAX).
 * @return LE_OK            The function succeeded.
 *
 * @note directionPtr, directionAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetDirection
(
    int32_t*    directionPtr,         ///< [OUT] The direction indication in degrees.
    int32_t*    directionAccuracyPtr  ///< [OUT] The direction's accuracy.
)
{
    pa_Gnss_Position_t  data;

    // Register a handler function for Call Event indications
    if (pa_gnss_GetLastPositionData(&data) != LE_OK)
    {
        return LE_FAULT;
    }
    else
    {
        le_result_t result = LE_OK;

        if (directionPtr) {
            if (data.trackValid) {
                *directionPtr = data.track;
            } else {
                *directionPtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        if (directionAccuracyPtr) {
            if (data.trackUncertaintyValid) {
                *directionAccuracyPtr = data.trackUncertainty;
            } else {
                *directionAccuracyPtr = INT32_MAX;
                result = LE_OUT_OF_RANGE;
            }
        }
        return result;
    }
}

