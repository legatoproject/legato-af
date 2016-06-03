//--------------------------------------------------------------------------------------------------
/**
 * @file le_gnss.c
 *
 * This file contains the source code of the GNSS API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------


#include "legato.h"
#include "interfaces.h"
#include "pa_gnss.h"


//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

#define GNSS_POSITION_SAMPLE_MAX         1

/// Typically, we don't expect more than this number of concurrent activation requests.
#define GNSS_POSITION_ACTIVATION_MAX      13      // Ideally should be a prime number.

//--------------------------------------------------------------------------------------------------
// Data structures.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration for GNSS device state
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_GNSS_STATE_UNINITIALIZED = 0,    ///< The GNSS device is not initialized
    LE_GNSS_STATE_READY,                ///< The GNSS device is ready
    LE_GNSS_STATE_ACTIVE,               ///< The GNSS device is active
    LE_GNSS_STATE_DISABLED,             ///< The GNSS device is disabled
    LE_GNSS_STATE_MAX                   ///< Do not use
}
le_gnss_State_t;


//--------------------------------------------------------------------------------------------------
/**
 * Satellite Vehicle information.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    uint16_t                satId;          ///< Satellite in View ID number [PRN].
    le_gnss_Constellation_t satConst;       ///< GNSS constellation type.
    bool                    satUsed;        ///< TRUE if satellite in View Used for Navigation.
    uint8_t                 satSnr;         ///< Satellite in View Signal To Noise Ratio [dBHz].
    uint16_t                satAzim;        ///< Satellite in View Azimuth [degrees].
                                            ///< Range: 0 to 360
    uint8_t                 satElev;        ///< Satellite in View Elevation [degrees].
                                            ///< Range: 0 to 90
}
le_gnss_SvInfo_t;

//--------------------------------------------------------------------------------------------------
/**
 * Position Sample structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_gnss_PositionSample
{
    le_gnss_FixState_t fixState;        ///< Position Fix state
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
    bool            directionValid;  ///< if true, direction is set
    int32_t         direction;       ///< direction
    bool            directionAccuracyValid; ///< if true, direction accuracy is set
    int32_t         directionAccuracy; ///< direction accuracy
    bool            dateValid;          ///< if true, date is set
    uint16_t        year;               ///< UTC Year A.D. [e.g. 2014].
    uint16_t        month;              ///< UTC Month into the year [range 1...12].
    uint16_t        day;                ///< UTC Days into the month [range 1...31].
    bool            timeValid;          ///< if true, time is set
    uint16_t        hours;              ///< UTC Hours into the day [range 0..23].
    uint16_t        minutes;            ///< UTC Minutes into the hour [range 0..59].
    uint16_t        seconds;            ///< UTC Seconds into the minute [range 0..59].
    uint16_t        milliseconds;       ///< UTC Milliseconds into the second [range 0..999].
    bool            gpsTimeValid;       ///< if true, GPS time is set
    uint32_t        gpsWeek;            ///< GPS week number from midnight, Jan. 6, 1980.
    uint32_t        gpsTimeOfWeek;      ///< Amount of time in milliseconds into the GPS week.
    bool            timeAccuracyValid;  ///< if true, timeAccuracy is set
    uint32_t        timeAccuracy;       ///< Estimated Accuracy for time in milliseconds
    bool            hdopValid;          ///< if true, horizontal dilution is set
    uint16_t        hdop;               ///< The horizontal Dilution of Precision (DOP)
    bool            vdopValid;          ///< if true, vertical dilition is set
    uint16_t        vdop;               ///< The vertical Dilution of Precision (DOP)
    bool            pdopValid;          ///< if true, position dilution is set
    uint16_t        pdop;               ///< The Position dilution of precision (DOP)
    // Satellite Vehicles information
    bool                satsInViewCountValid;    ///< if true, satsInViewCount is set
    uint8_t             satsInViewCount;         ///< Satellites in View count.
    bool                satsTrackingCountValid;  ///< if true, satsTrackingCount is set
    uint8_t             satsTrackingCount;       ///< Tracking satellites in View count.
    bool                satsUsedCountValid;      ///< if true, satsUsedCount is set
    uint8_t             satsUsedCount;           ///< Satellites in View used for Navigation.
    bool                satInfoValid;       ///< if true, satInfo is set
    le_gnss_SvInfo_t    satInfo[LE_GNSS_SV_INFO_MAX_LEN];
                                            ///< Satellite Vehicle information.
    le_dls_Link_t   link;               ///< Object node link
}
le_gnss_PositionSample_t;

//--------------------------------------------------------------------------------------------------
/**
 * Position Sample's Handler structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_gnss_PositionHandler
{
    le_gnss_PositionHandlerFunc_t handlerFuncPtr;      ///< The handler function address.
    void*                         handlerContextPtr;   ///< The handler function context.
    le_dls_Link_t                 link;                ///< Object node link
}
le_gnss_PositionHandler_t;

//--------------------------------------------------------------------------------------------------
// Static declarations.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Maintains the GNSS device state.
 */
//--------------------------------------------------------------------------------------------------
static le_gnss_State_t GnssState = LE_GNSS_STATE_UNINITIALIZED;


//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for position sample's handlers.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   PositionHandlerPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * PA handler's reference.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_HandlerRef_t PaHandlerRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Number of position Handler functions that own position samples.
 *
 */
//--------------------------------------------------------------------------------------------------
static int32_t NumOfPositionHandlers = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Create and initialize the position sample's handlers list.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t PositionHandlerList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for position samples.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t   PositionSamplePoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Create and initialize the position samples list.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t PositionSampleList = LE_DLS_LIST_INIT;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for Positioning Sample objects.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t PositionSampleMap;

//--------------------------------------------------------------------------------------------------
/**
 * Position Handler destructor.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PositionHandlerDestructor
(
    void* obj
)
{
    le_gnss_PositionHandler_t *positionHandlerNodePtr;
    le_dls_Link_t          *linkPtr;

    linkPtr = le_dls_Peek(&PositionHandlerList);
    if (linkPtr != NULL)
    {
        do
        {
            // Get the node from the list
            positionHandlerNodePtr =
                (le_gnss_PositionHandler_t*)CONTAINER_OF(linkPtr, le_gnss_PositionHandler_t, link);
            // Check the node.
            if ( positionHandlerNodePtr == (le_gnss_PositionHandler_t*)obj)
            {
                // Remove the node.
                le_dls_Remove(&PositionHandlerList, linkPtr);
                linkPtr=NULL;
            }
            else
            {
                // Move to the next node.
                linkPtr = le_dls_PeekNext(&PositionHandlerList, linkPtr);
            }
        } while (linkPtr != NULL);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Position Sample destructor.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PositionSampleDestructor
(
    void* obj
)
{
    le_gnss_PositionSample_t *positionSampleNodePtr;
    le_dls_Link_t   *linkPtr;

    LE_FATAL_IF((obj == NULL), "Position Sample Object does not exist!");

    linkPtr = le_dls_Peek(&PositionSampleList);
    if (linkPtr != NULL)
    {
        do
        {
            // Get the node from the list
            positionSampleNodePtr = (le_gnss_PositionSample_t*)
                                    CONTAINER_OF(linkPtr, le_gnss_PositionSample_t, link);
            // Check the node.
            if ( positionSampleNodePtr == (le_gnss_PositionSample_t*)obj)
            {
                // Remove the node.
                le_dls_Remove(&PositionSampleList, linkPtr);
                linkPtr=NULL;
            }
            else
            {
                // Move to the next node.
                linkPtr = le_dls_PeekNext(&PositionSampleList, linkPtr);
            }
        } while (linkPtr != NULL);
    }
}


//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the GNSS
 *
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
    le_result_t result = LE_FAULT;

    LE_DEBUG("gnss_Init");

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_UNINITIALIZED:
        {
            result = pa_gnss_Init();
            if(result == LE_OK)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_READY:
        case LE_GNSS_STATE_ACTIVE:
        case LE_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state [%d]", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    // Create a pool for Position  Handler objects
    PositionHandlerPoolRef = le_mem_CreatePool("PositionHandlerPoolRef"
                                                , sizeof(le_gnss_PositionHandler_t));
    le_mem_SetDestructor(PositionHandlerPoolRef, PositionHandlerDestructor);

    // Create a pool for Position Sample objects
    PositionSamplePoolRef = le_mem_CreatePool("PositionSamplePoolRef"
                                            , sizeof(le_gnss_PositionSample_t));
    le_mem_ExpandPool(PositionSamplePoolRef,GNSS_POSITION_SAMPLE_MAX);
    le_mem_SetDestructor(PositionSamplePoolRef, PositionSampleDestructor);

    // Create the reference HashMap for positioning sample
    PositionSampleMap = le_ref_CreateMap("PositionSampleMap", GNSS_POSITION_SAMPLE_MAX);

    // Initialize Handler context
    NumOfPositionHandlers = 0;
    PaHandlerRef = NULL;

    return result;
}



//--------------------------------------------------------------------------------------------------
/**
 * The PA position Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void PaPositionHandler
(
    pa_Gnss_Position_t* positionPtr
)
{

    le_gnss_PositionHandler_t*  positionHandlerNodePtr;
    le_dls_Link_t*              linkPtr;
    le_gnss_PositionSample_t*   positionSampleNodePtr=NULL;
    uint8_t i;

    if(!NumOfPositionHandlers)
    {
        return;
    }

    LE_DEBUG("Handler Function called with PA position %p", positionPtr);

    linkPtr = le_dls_Peek(&PositionHandlerList);
    if (linkPtr != NULL)
    {
        // Create the position sample node.
        positionSampleNodePtr =
                    (le_gnss_PositionSample_t*)le_mem_ForceAlloc(PositionSamplePoolRef);
        /* Copy GNSS information to the sample node */
        positionSampleNodePtr->fixState = positionPtr->fixState;
        positionSampleNodePtr->latitudeValid = positionPtr->latitudeValid;
        positionSampleNodePtr->latitude = positionPtr->latitude;
        positionSampleNodePtr->longitudeValid = positionPtr->longitudeValid;
        positionSampleNodePtr->longitude = positionPtr->longitude;
        positionSampleNodePtr->hAccuracyValid = positionPtr->hUncertaintyValid;
        positionSampleNodePtr->hAccuracy = positionPtr->hUncertainty;
        positionSampleNodePtr->altitudeValid = positionPtr->altitudeValid;
        positionSampleNodePtr->altitude = positionPtr->altitude;
        positionSampleNodePtr->vAccuracyValid = positionPtr->vUncertaintyValid;
        positionSampleNodePtr->vAccuracy = positionPtr->vUncertainty;
        positionSampleNodePtr->hSpeedValid = positionPtr->hSpeedValid;
        positionSampleNodePtr->hSpeed = positionPtr->hSpeed;
        positionSampleNodePtr->hSpeedAccuracyValid = positionPtr->hSpeedUncertaintyValid;
        positionSampleNodePtr->hSpeedAccuracy = positionPtr->hSpeedUncertainty;
        positionSampleNodePtr->vSpeedValid = positionPtr->vSpeedValid;
        positionSampleNodePtr->vSpeed = positionPtr->vSpeed;
        positionSampleNodePtr->vSpeedAccuracyValid = positionPtr->vSpeedUncertaintyValid;
        positionSampleNodePtr->vSpeedAccuracy = positionPtr->vSpeedUncertainty;
        positionSampleNodePtr->directionValid = positionPtr->directionValid;
        positionSampleNodePtr->direction = positionPtr->heading;
        positionSampleNodePtr->directionAccuracyValid = positionPtr->headingUncertaintyValid;
        positionSampleNodePtr->directionAccuracy = positionPtr->directionUncertainty;
        // Date
        positionSampleNodePtr->dateValid = positionPtr->dateValid;
        positionSampleNodePtr->year = positionPtr->date.year;
        positionSampleNodePtr->month = positionPtr->date.month;
        positionSampleNodePtr->day = positionPtr->date.day;
        // UTC time
        positionSampleNodePtr->timeValid = positionPtr->timeValid;
        positionSampleNodePtr->hours = positionPtr->time.hours;
        positionSampleNodePtr->minutes = positionPtr->time.minutes;
        positionSampleNodePtr->seconds = positionPtr->time.seconds;
        positionSampleNodePtr->milliseconds = positionPtr->time.milliseconds;
        // GPS time
        positionSampleNodePtr->gpsTimeValid = positionPtr->gpsTimeValid;
        positionSampleNodePtr->gpsWeek = positionPtr->gpsWeek;
        positionSampleNodePtr->gpsTimeOfWeek = positionPtr->gpsTimeOfWeek;
        // Time accuracy
        positionSampleNodePtr->timeAccuracyValid = positionPtr->timeAccuracyValid;
        positionSampleNodePtr->timeAccuracy = positionPtr->timeAccuracy;

        // DOP parameters
        positionSampleNodePtr->hdopValid = positionPtr->hdopValid;
        positionSampleNodePtr->hdop = positionPtr->hdop;
        positionSampleNodePtr->vdopValid = positionPtr->vdopValid;
        positionSampleNodePtr->vdop = positionPtr->vdop;
        positionSampleNodePtr->pdopValid = positionPtr->pdopValid;
        positionSampleNodePtr->pdop = positionPtr->pdop;
        // Satellites information
        positionSampleNodePtr->satsInViewCountValid = positionPtr->satsInViewCountValid;
        positionSampleNodePtr->satsInViewCount = positionPtr->satsInViewCount;
        positionSampleNodePtr->satsTrackingCountValid = positionPtr->satsTrackingCountValid;
        positionSampleNodePtr->satsTrackingCount = positionPtr->satsTrackingCount;
        positionSampleNodePtr->satsUsedCountValid = positionPtr->satsUsedCountValid;
        positionSampleNodePtr->satsUsedCount = positionPtr->satsUsedCount;
        positionSampleNodePtr->satInfoValid = positionPtr->satInfoValid;
        for(i=0; i<LE_GNSS_SV_INFO_MAX_LEN; i++)
        {
            positionSampleNodePtr->satInfo[i].satId = positionPtr->satInfo[i].satId;
            positionSampleNodePtr->satInfo[i].satConst = positionPtr->satInfo[i].satConst;
            positionSampleNodePtr->satInfo[i].satUsed = positionPtr->satInfo[i].satUsed;
            positionSampleNodePtr->satInfo[i].satSnr = positionPtr->satInfo[i].satSnr;
            positionSampleNodePtr->satInfo[i].satAzim = positionPtr->satInfo[i].satAzim;
            positionSampleNodePtr->satInfo[i].satElev = positionPtr->satInfo[i].satElev;
        }

        // Node Link
        positionSampleNodePtr->link = LE_DLS_LINK_INIT;

        // Add the node to the queue of the list by passing in the node's link.
        le_dls_Queue(&PositionSampleList, &(positionSampleNodePtr->link));

                    // Add reference for each subscribed handler
        for(i=0 ; i<NumOfPositionHandlers-1 ; i++)
        {
            le_mem_AddRef((void *)positionSampleNodePtr);
        }

        // Call Handler(s)
        do
        {
            // Get the node from the list
            positionHandlerNodePtr =
                (le_gnss_PositionHandler_t*)CONTAINER_OF(linkPtr, le_gnss_PositionHandler_t, link);

            LE_DEBUG("Report sample %p to the corresponding handler (handler %p)",
                     positionSampleNodePtr,
                     positionHandlerNodePtr->handlerFuncPtr);

            // Create a safe reference and call the client's handler
            void* safePositionSampleRef = le_ref_CreateRef(PositionSampleMap, positionSampleNodePtr);
            if(safePositionSampleRef != NULL)
            {
                positionHandlerNodePtr->handlerFuncPtr(safePositionSampleRef
                                                    , positionHandlerNodePtr->handlerContextPtr);
            }
            // Move to the next node.
            linkPtr = le_dls_PeekNext(&PositionHandlerList, linkPtr);
        } while (linkPtr != NULL);
    }

    le_mem_Release(positionPtr);
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
    void*                        contextPtr           ///< [IN] The context pointer
)
{
    le_gnss_PositionHandler_t*  positionHandlerPtr=NULL;

    LE_FATAL_IF((handlerPtr == NULL), "handlerPtr pointer is NULL !");

    // Create the position sample handler node.
    positionHandlerPtr = (le_gnss_PositionHandler_t*)le_mem_ForceAlloc(PositionHandlerPoolRef);
    positionHandlerPtr->handlerFuncPtr = handlerPtr;
    positionHandlerPtr->handlerContextPtr = contextPtr;

    LE_DEBUG("handler %p", handlerPtr);

    // Subscribe to PA position Data handler
    if (NumOfPositionHandlers == 0)
    {
        if ((PaHandlerRef=pa_gnss_AddPositionDataHandler(PaPositionHandler)) == NULL)
        {
            LE_ERROR("Failed to add PA position Data handler!");
            le_mem_Release(positionHandlerPtr);
            return NULL;
        }
    }

    // Update the position handler list with that new handler
    le_dls_Queue(&PositionHandlerList, &(positionHandlerPtr->link));
    NumOfPositionHandlers++;

    LE_DEBUG("Position handler %p added",
             positionHandlerPtr->handlerFuncPtr);

    return (le_gnss_PositionHandlerRef_t)positionHandlerPtr;

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
    le_gnss_PositionHandler_t* positionHandlerNodePtr;
    le_dls_Link_t*          linkPtr;

    linkPtr = le_dls_Peek(&PositionHandlerList);
    if (linkPtr != NULL)
    {
        do
        {
            // Get the node from MsgList
            positionHandlerNodePtr =
                (le_gnss_PositionHandler_t*)CONTAINER_OF(linkPtr, le_gnss_PositionHandler_t, link);
            // Check the node.
            if ( (le_gnss_PositionHandlerRef_t)positionHandlerNodePtr == handlerRef )
            {
                // Remove the node.
                le_mem_Release(positionHandlerNodePtr);
                NumOfPositionHandlers--;
                linkPtr=NULL;
            }
            else
            {
                // Move to the next node.
                linkPtr = le_dls_PeekNext(&PositionHandlerList, linkPtr);
            }
        } while (linkPtr != NULL);
    }

    if(NumOfPositionHandlers == 0)
    {
        pa_gnss_RemovePositionDataHandler(PaHandlerRef);
        PaHandlerRef = NULL;
    }
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
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr =
                                                le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Check input pointers
    if (statePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Get the position Fix state
    *statePtr = positionSamplePtr->fixState;

    return result;
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
 * @note latitudePtr, longitudePtr,hAccuracyPtr, altitudePtr, vAccuracyPtr can be set to NULL
 *       if not needed.
 *
 * @note: The latitude and longitude values are based on the WGS84 standard coordinate system.
 *
 * @note The latitude and longitude are given in degrees with 6 decimal places like:
 *    Latitude +48858300 = 48.858300 degrees North
 *    Longitude +2294400 = 2.294400 degrees East
 *
 * @note Altitude is in metres, above Mean Sea Level, with 3 decimal places (3047 = 3.047 metres).
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
        ///< Horizontal position's accuracy in metres [resolution 1e-2].
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

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
        }
        else
        {
            *latitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (longitudePtr)
    {
        if (positionSamplePtr->longitudeValid)
        {
            *longitudePtr = positionSamplePtr->longitude;
        }
        else
        {
            *longitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (hAccuracyPtr)
    {
        if (positionSamplePtr->hAccuracyValid)
        {
            *hAccuracyPtr = positionSamplePtr->hAccuracy;
        }
        else
        {
            *hAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
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
 * @note Altitude is in metres, above Mean Sea Level, with 3 decimal places (3047 = 3.047 metres).
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
        ///< Vertical position's accuracy in metres [resolution 1e-1].
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t * positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (altitudePtr)
    {
        if (positionSamplePtr->altitudeValid)
        {
            *altitudePtr = positionSamplePtr->altitude;
        }
        else
         {
            *altitudePtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vAccuracyPtr)
    {
        if (positionSamplePtr->vAccuracyValid)
        {
            *vAccuracyPtr = positionSamplePtr->vAccuracy;
        }
        else
        {
            *vAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    return result;
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
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Check input pointers
    if ((hoursPtr == NULL)
        || (minutesPtr == NULL)
        || (secondsPtr == NULL)
        || (millisecondsPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Get the position sample's time
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
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Check input pointers
    if ((gpsWeekPtr == NULL)
        || (gpsTimeOfWeekPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Get the position sample's GPS time
    if (positionSamplePtr->timeValid)
    {
        result = LE_OK;
        if (gpsWeekPtr)
        {
            *gpsWeekPtr = positionSamplePtr->gpsWeek;
        }
        if (gpsTimeOfWeekPtr)
        {
            *gpsTimeOfWeekPtr = positionSamplePtr->gpsTimeOfWeek;
        }
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        if (gpsWeekPtr)
        {
            *gpsWeekPtr = 0;
        }
        if (gpsTimeOfWeekPtr)
        {
            *gpsTimeOfWeekPtr = 0;
        }
    }

    return result;
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
        ///< [OUT] Estimated time accuracy in milliseconds
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Check input pointers
    if (timeAccuracyPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Get the position sample's time accuracy
    if (positionSamplePtr->timeAccuracyValid)
    {
        result = LE_OK;
        if (timeAccuracyPtr)
        {
            *timeAccuracyPtr = positionSamplePtr->timeAccuracy;
        }
    }
    else
    {
        result = LE_OUT_OF_RANGE;
        if (timeAccuracyPtr)
        {
            *timeAccuracyPtr = UINT16_MAX;
        }
    }

    return result;
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

    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr =
                                                le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Check input pointers
    if ((yearPtr == NULL)
        || (monthPtr == NULL)
        || (dayPtr == NULL))
    {
        LE_KILL_CLIENT("Invalid pointer provided!");
        return LE_FAULT;
    }

    // Get the position sample's date
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
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (hspeedPtr)
    {
        if (positionSamplePtr->hSpeedValid)
    {
            *hspeedPtr = positionSamplePtr->hSpeed;
        }
        else
        {
            *hspeedPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (hspeedAccuracyPtr)
    {
        if (positionSamplePtr->hSpeedAccuracyValid)
    {
            *hspeedAccuracyPtr = positionSamplePtr->hSpeedAccuracy;
        }
        else
        {
            *hspeedAccuracyPtr = UINT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
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
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (vspeedPtr)
    {
        if (positionSamplePtr->vSpeedValid)
        {
            *vspeedPtr = positionSamplePtr->vSpeed;
        }
        else
        {
            *vspeedPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vspeedAccuracyPtr)
    {
        if (positionSamplePtr->vSpeedAccuracyValid)
        {
            *vspeedAccuracyPtr = positionSamplePtr->vSpeedAccuracy;
        }
        else
        {
            *vspeedAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's direction. Direction of movement is the
 * direction that the vehicle/person is actually moving.
 *
 * @return
 *  - LE_FAULT         Function failed to find the positionSample.
 *  - LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 *  - LE_OK            Function succeeded.
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

    int32_t* directionPtr,
        ///< [OUT]
        ///< Direction in degrees [resolution 1e-1].
        ///< (where 0 is True North)

    int32_t* directionAccuracyPtr
        ///< [OUT]
        ///< Direction's accuracy estimate
        ///< in degrees [resolution 1e-1].
)
{
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (directionPtr)
    {
        if (positionSamplePtr->directionValid)
        {
            *directionPtr = positionSamplePtr->direction;
        }
        else
        {
            *directionPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (directionAccuracyPtr)
    {
        if (positionSamplePtr->directionAccuracyValid)
        {
            *directionAccuracyPtr = positionSamplePtr->directionAccuracy;
        }
        else
        {
            *directionAccuracyPtr = INT32_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
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
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);
    int i;

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (satIdPtr)
    {
        if (positionSamplePtr->satInfoValid)
        {
            for(i=0; i<*satIdNumElementsPtr; i++)
            {
                satIdPtr[i] = positionSamplePtr->satInfo[i].satId;
            }
        }
        else
        {
            for(i=0; i<*satIdNumElementsPtr; i++)
            {
                satIdPtr[i] = UINT16_MAX;
            }
            result = LE_OUT_OF_RANGE;
        }
    }

    if (satConstPtr)
    {
        if (positionSamplePtr->satInfoValid)
        {
            for(i=0; i<*satConstNumElementsPtr; i++)
            {
                satConstPtr[i] = positionSamplePtr->satInfo[i].satConst;
            }
        }
        else
        {
            for(i=0; i<*satConstNumElementsPtr; i++)
            {
                satConstPtr[i] = LE_GNSS_SV_CONSTELLATION_UNDEFINED;
            }
            result = LE_OUT_OF_RANGE;
        }
    }

    if (satUsedPtr)
    {
        if (positionSamplePtr->satInfoValid)
        {
            for(i=0; i<*satUsedNumElementsPtr; i++)
            {
                satUsedPtr[i] = positionSamplePtr->satInfo[i].satUsed;
            }
        }
        else
        {
            for(i=0; i<*satUsedNumElementsPtr; i++)
            {
                satUsedPtr[i] = false;
            }
            result = LE_OUT_OF_RANGE;
        }
    }

    if (satSnrPtr)
    {
        if (positionSamplePtr->satInfoValid)
        {
            for(i=0; i<*satSnrNumElementsPtr; i++)
            {
                satSnrPtr[i] = positionSamplePtr->satInfo[i].satSnr;
            }
        }
        else
        {
            for(i=0; i<*satSnrNumElementsPtr; i++)
            {
                satSnrPtr[i] = UINT8_MAX;
            }
            result = LE_OUT_OF_RANGE;
        }
    }

    if (satAzimPtr)
    {
        if (positionSamplePtr->satInfoValid)
        {
            for(i=0; i<*satAzimNumElementsPtr; i++)
            {
                satAzimPtr[i] = positionSamplePtr->satInfo[i].satAzim;
            }
        }
        else
        {
            for(i=0; i<*satAzimNumElementsPtr; i++)
            {
                satAzimPtr[i] = UINT16_MAX;
            }
            result = LE_OUT_OF_RANGE;
        }
    }

    if (satElevPtr)
    {
        if (positionSamplePtr->satInfoValid)
        {
            for(i=0; i<*satElevNumElementsPtr; i++)
            {
                satElevPtr[i] = positionSamplePtr->satInfo[i].satElev;
            }
        }
        else
        {
            for(i=0; i<*satElevNumElementsPtr; i++)
            {
                satElevPtr[i] = UINT8_MAX;
            }
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
}


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
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    // Satellites in View count
    if (satsInViewCountPtr)
    {
        if (positionSamplePtr->satsInViewCountValid)
        {
            *satsInViewCountPtr = positionSamplePtr->satsInViewCount;
        }
        else
        {
            *satsInViewCountPtr = UINT8_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    // Tracking satellites in View count
    if (satsTrackingCountPtr)
    {
        if (positionSamplePtr->satsTrackingCountValid)
        {
            *satsTrackingCountPtr = positionSamplePtr->satsTrackingCount;
        }
        else
        {
            *satsTrackingCountPtr = UINT8_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    // Number of satellites in View used for Navigation
    if (satsUsedCountPtr)
    {
        if (positionSamplePtr->satsUsedCountValid)
        {
            *satsUsedCountPtr = positionSamplePtr->satsUsedCount;
        }
        else
        {
            *satsUsedCountPtr = UINT8_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }

    return result;
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
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note The DOP values are given with 3 decimal places like: DOP value 2200 = 2.20
 *
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
    le_result_t result = LE_OK;
    le_gnss_PositionSample_t* positionSamplePtr
                                            = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    // Check position sample's reference
    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return LE_FAULT;
    }

    if (hdopPtr)
    {
        if (positionSamplePtr->hdopValid)
        {
            *hdopPtr = positionSamplePtr->hdop;
        }
        else
        {
            *hdopPtr = UINT16_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (vdopPtr)
    {
        if (positionSamplePtr->vdopValid)
        {
            *vdopPtr = positionSamplePtr->vdop;
        }
        else
        {
            *vdopPtr = UINT16_MAX;
            result = LE_OUT_OF_RANGE;
        }
    }
    if (pdopPtr)
    {
        if (positionSamplePtr->pdopValid)
        {
            *pdopPtr = positionSamplePtr->pdop;
        }
        else
        {
            *pdopPtr = UINT16_MAX;
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
void le_gnss_ReleaseSampleRef
(
    le_gnss_SampleRef_t    positionSampleRef    ///< [IN] The position sample's reference.
)
{
    le_gnss_PositionSample_t* positionSamplePtr = le_ref_Lookup(PositionSampleMap,positionSampleRef);

    if ( positionSamplePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!",positionSampleRef);
        return;
    }
    le_ref_DeleteRef(PositionSampleMap,positionSampleRef);
    le_mem_Release(positionSamplePtr);
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

    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Set GNSS constellation
            result = pa_gnss_SetConstellation(constellationMask);
        }
        break;

        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
    le_result_t result = LE_FAULT;

    if (constellationMaskPtr == NULL)
    {
        LE_KILL_CLIENT("Pointer is NULL !");
        return LE_FAULT;
    }

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Get GNSS constellation
            result = pa_gnss_GetConstellation(constellationMaskPtr);
        }
        break;

        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
    return (pa_gnss_EnableExtendedEphemerisFile());
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
    return (pa_gnss_DisableExtendedEphemerisFile());
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
    return (pa_gnss_LoadExtendedEphemerisFile(fd));
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
    if (!startTimePtr)
    {
        LE_KILL_CLIENT("startTimePtr is NULL !");
        return LE_FAULT;
    }

    if (!stopTimePtr)
    {
        LE_KILL_CLIENT("stopTimePtr is NULL !");
        return LE_FAULT;
    }

    return pa_gnss_GetExtendedEphemerisValidity(startTimePtr,stopTimePtr);
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
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Start GNSS
            result = pa_gnss_Start();
            // Update GNSS device state
            if(result == LE_OK)
            {
                GnssState = LE_GNSS_STATE_ACTIVE;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case LE_GNSS_STATE_ACTIVE:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_DUPLICATE;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Stop GNSS
            result = pa_gnss_Stop();
            // Update GNSS device state
            if(result == LE_OK)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        case LE_GNSS_STATE_READY:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_DUPLICATE;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Restart GNSS
            result = pa_gnss_ForceRestart(PA_GNSS_HOT_RESTART);
            // GNSS device state is updated ONLY if the restart is failed
            if(result == LE_FAULT)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_READY:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Restart GNSS
            result = pa_gnss_ForceRestart(PA_GNSS_WARM_RESTART);
            // GNSS device state is updated ONLY if the restart is failed
            if(result == LE_FAULT)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_READY:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Restart GNSS
            result = pa_gnss_ForceRestart(PA_GNSS_COLD_RESTART);
            // GNSS device state is updated ONLY if the restart is failed
            if(result == LE_FAULT)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_READY:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_ACTIVE:
        {
            // Restart GNSS
            result = pa_gnss_ForceRestart(PA_GNSS_FACTORY_RESTART);
            // GNSS device state is updated ONLY if the restart is failed
            if(result == LE_FAULT)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_READY:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_DISABLED:
        case LE_GNSS_STATE_UNINITIALIZED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        case LE_GNSS_STATE_READY:
        case LE_GNSS_STATE_ACTIVE:
        {
            result = pa_gnss_GetTtff(ttffPtr);
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_DISABLED:
        {
            // Enable GNSS device
            result = pa_gnss_Enable();
            // Update GNSS device state
            if(result == LE_OK)
            {
                GnssState = LE_GNSS_STATE_READY;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        case LE_GNSS_STATE_READY:
        case LE_GNSS_STATE_ACTIVE:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_DUPLICATE;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Disable GNSS device
            result = pa_gnss_Disable();
            // Update GNSS device state
            if(result == LE_OK)
            {
                GnssState = LE_GNSS_STATE_DISABLED;
            }
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_ACTIVE:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_NOT_PERMITTED;
        }
        break;
        case LE_GNSS_STATE_DISABLED:
        {
                        LE_ERROR("Bad state for that request [%d]", GnssState);

            result = LE_DUPLICATE;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_SetAcquisitionRate
(
    uint32_t  rate      ///< Acquisition rate in milliseconds.
)
{
    le_result_t result = LE_FAULT;

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Set the GNSS device acquisition rate
            result = pa_gnss_SetAcquisitionRate(rate);
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_ACTIVE:
        case LE_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
    le_result_t result = LE_FAULT;

    if (ratePtr == NULL)
    {
        LE_KILL_CLIENT("Pointer is NULL !");
        return LE_FAULT;
    }

    // Check the GNSS device state
    switch (GnssState)
    {
        case LE_GNSS_STATE_READY:
        {
            // Set the GNSS device acquisition rate
            result = pa_gnss_GetAcquisitionRate(ratePtr);
        }
        break;
        case LE_GNSS_STATE_UNINITIALIZED:
        case LE_GNSS_STATE_ACTIVE:
        case LE_GNSS_STATE_DISABLED:
        {
            LE_ERROR("Bad state for that request [%d]", GnssState);
            result = LE_NOT_PERMITTED;
        }
        break;
        default:
        {
            LE_ERROR("Unknown GNSS state %d", GnssState);
            result = LE_FAULT;
        }
        break;
    }

    return result;
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
    return pa_gnss_SetSuplAssistedMode(assistedMode);
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
    le_gnss_AssistedMode_t * assistedModePtr      ///< [OUT] Assisted-GNSS mode.
)
{
    if (assistedModePtr == NULL)
    {
        LE_KILL_CLIENT("assistedModePtr is NULL !");
        return LE_FAULT;
    }

    return pa_gnss_GetSuplAssistedMode(assistedModePtr);
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
    if (suplServerUrlPtr == NULL)
    {
        LE_KILL_CLIENT("suplServerUrlPtr is NULL !");
        return LE_FAULT;
    }

    return pa_gnss_SetSuplServerUrl(suplServerUrlPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function injects the SUPL certificate to be used in A-GNSS sessions.
 *
 * @return
 *  - LE_OK on success
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
    if (suplCertificatePtr == NULL)
    {
        LE_KILL_CLIENT("suplCertificatePtr is NULL !");
        return LE_FAULT;
    }

    return pa_gnss_InjectSuplCertificate( suplCertificateId
                                        , suplCertificateLen
                                        , suplCertificatePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes the SUPL certificate.
 *
 * @return
 *  - LE_OK on success
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
    return pa_gnss_DeleteSuplCertificate(suplCertificateId);
}
