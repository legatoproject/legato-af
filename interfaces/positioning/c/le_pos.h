/**
 * @page c_pos Positioning Service
 *
 *
 * @ref le_pos.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * This file contains the the prototype definitions of the high level Positioning APIs.
 *
 * @ref le_pos_fix <br>
 * @ref le_pos_navigation <br>
 * @ref le_pos_configdb <br>
 *
 * You need to know the location and the current movement information to precisely track machine position.
 *
 * This module provides an API to retrieve the position's information.
 *
 * @section le_pos_fix Fix On Demand
 * The @c le_pos_Get2DLocation() function gets the last updated latitude, longitude
 * and the horizontal accuracy values:
 *
 * - latitude is in degrees, positive North.
 * - longitude is in degrees, positive East.
 * - horizontal accuracy is in metres.
 *
 * The latitude and longitude are given in degrees with 6 decimal places like:
 * - Latitude +48858300  = 48.858300 degrees North
 * - Longitude +2294400 =  2.294400 degrees East
 *
 * The @c le_pos_Get3DLocation() function gets the last updated latitude, longitude,
 * altitude and their associated accuracy values.
 * - altitude is in metres, above Mean Sea Level, with 3 decimal places (3047 = 3.047 metres).
 * - horizontal and vertical accuracies are in metres.
 *
 * The @c le_pos_GetMotion() function gets the last updated horizontal and vertical
 * speed values and the associated accuracy values:
 *
 * - horizontal speed is in m/sec.
 * - vertical speed is in m/sec, positive up.
 *
 * The @c le_pos_GetHeading() function gets the last updated heading value in
 * degrees (where 0 is True North) and its associated accuracy value. Heading is the direction that
 * the vehicle/person is facing.
 *
 * The @c le_pos_GetDirection() function gets the last updated direction value in
 * degrees (where 0 is True North) and its associated accuracy value. Direction of movement is the
 * direction that the vehicle/person is actually moving.
 *
 * @section le_pos_navigation Navigation
 * To be notified when the device is in motion, you must register an handler function
 * to get the new position's data. The @c le_pos_AddMovementHandler() API registers
 * that handler. The horizontal and vertical change is measured in metres so only movement over
 * the threshhold will trigger notification (0 means we don't care about changes).
 *
 * The handler will give a reference to the position sample object that has triggered the
 * notification. You can then access parameters using accessor functions, and release
 * the object when done with it.
 *
 * The accessor functions are:
 * - le_pos_sample_Get2DLocation()
 * - le_pos_sample_GetAltitude()
 * - le_pos_sample_GetHorizontalSpeed()
 * - le_pos_sample_GetVerticalSpeed()
 * - le_pos_sample_GetHeading()
 * - le_pos_sample_GetDirection()
 *
 * @c le_pos_sample_Release() releases the object.
 *
 * You can uninstall the handler function by calling the le_pos_RemoveMovementHandler() API.
 * @note The le_pos_RemoveMovementHandler() API does not delete the Position Object. The caller has
 *       to delete it by calling the le_pos_sample_Release() function.
 *
 *
 * In the following code sample, the function InstallGeoFenceHandler() installs an handler function
 * that triggers an alarm if the device leaves a designated location.
 *
 * @code
 *
 * #define GEOFENCE_CENTRE_LATITUDE            +48070380  // Latitude 48.070380 degrees North
 * #define GEOFENCE_CENTRE_LONGITUDE           -11310000  // Longitude 11.310000 degrees West
 * #define MAX_METRES_FROM_GEOFENCE_CENTRE     150
 *
 * static le_event_HandlerRef_t GeoFenceHandlerRef;
 *
 * void InstallGeoFenceHandler()
 * {
 *     // I set my home latitude and longitude where my device is located
 *     SetHomeLocation(HOME_LATITUDE, HOME_LONGITUDE);
 *
 *     MyPositionUpdateHandlerRef = le_pos_AddMovementHandler(50, // 50 metres
 *                                                            0,  // I don't care about changes of altitude.
 *                                                            MyPositionUpdateHandler,
 *                                                            NULL);
 * }
 *
 *
 * static void MyPositionUpdateHandler
 * (
 *     le_pos_SampleRef_t     positionSampleRef,
 *     void*                  contextPtr
 * )
 * {
 *     int32_t metresFromCentre;
 *
 *     int32_t accuracyMetres;
 *     int32_t latitude;
 *     int32_t longitude;
 *
 *     le_pos_sample_Get2DLocation(positionSampleRef, &latitude, &longitude, &accuracyMetres);
 *
 *     metresFromCentre = le_pos_ComputeDistance(GEOFENCE_CENTRE_LATITUDE, GEOFENCE_CENTRE_LONGITUDE
 *                                               latitude, longitude);
 *
 *     if (metresFromCentre > MAX_METRES_FROM_GEOFENCE_CENTRE)
 *     {
 *         // Check it doesn't just look like we are outside the fence because of bad accuracy.
 *         if (  (accuracyMetres > metresFromCentre) ||
 *               ((metresFromCentre - accuracyMetres) < MAX_METRES_FROM_GEOFENCE_CENTRE)  )
 *         {
 *             // Could be outside the fence, but we also could be inside the fence.
 *         }
 *         else
 *         {
 *             // Definitely outside the fence!
 *             RaiseAlarm(positionSampleRef);
 *         }
 *     }
 *
 *     le_pos_sample_Release(positionSampleRef);
 * }
 *
 * @endcode
 *
 * @section le_pos_configdb Positioning configuration tree
 *
 * The configuration database path for Positioning is:
 * @verbatim
   /
       positioning/
           acquisitionRate<int> == 5
  @endverbatim
 *
 *  - 'acquisitionRate' is the fix acquisition rate in seconds.
 *
 * @note
 *  If there is no configuration for 'acquisitionRate', it will be automatically set to 5 seconds.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file le_pos.h
 *
 * Legato @ref c_pos include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_POS_INCLUDE_GUARD
#define LEGATO_POS_INCLUDE_GUARD

#include "legato.h"



//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for dealing with Position samples.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_pos_Sample* le_pos_SampleRef_t;

//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for Movement notification's Handler.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_pos_MovementHandler* le_pos_MovementHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used for movement notifications.
 *
 * @param positionSampleRef  Position's sample reference.
 * @param contextPtr         Context information the movement handler may require.
 */
//--------------------------------------------------------------------------------------------------
typedef void(*le_pos_MovementHandlerFunc_t)
(
    le_pos_SampleRef_t    positionSampleRef,
    void*                 contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Register an handler for movement notifications.
 *
 * @return  Handler reference, only needed for to remove the handler.
 *
 * @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_pos_MovementHandlerRef_t le_pos_AddMovementHandler
(
    uint32_t                     horizontalMagnitude, ///< [IN]  Horizontal magnitude in metres.
                                                      ///       0 means that I don't care about
                                                      ///       changes in the latitude and longitude.
    uint32_t                     verticalMagnitude,   ///< [IN] Vertical magnitude in metres.
                                                      ///       0 means that I don't care about
                                                      ///       changes in the altitude.
    le_pos_MovementHandlerFunc_t handlerPtr,          ///< [IN] Handler function.
    void*                        contextPtr           ///< [IN] The context pointer
);

//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler for movement notifications.
 *
 * @note Doesn't return on failure, no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void le_pos_RemoveMovementHandler
(
    le_pos_MovementHandlerRef_t    handlerRef ///< [IN] Handler reference.
);

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
    le_pos_SampleRef_t  positionSampleRef,    ///< [IN] Position sample's reference.
    int32_t*            latitudePtr,          ///< [OUT] Latitude in degrees.
    int32_t*            longitudePtr,         ///< [OUT] Longitude in degrees.
    int32_t*            horizontalAccuracyPtr ///< [OUT] Horizontal's accuracy estimate in metres.
);

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
    le_pos_SampleRef_t  positionSampleRef,   ///< [IN] Position sample's reference.
    int32_t*            altitudePtr,         ///< [OUT] Altitude in metres.
    int32_t*            altitudeAccuracyPtr  ///< [OUT] Altitude's accuracy estimate in metres.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's horizontal speed.
 *
 * @return LE_FAULT         Function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX, UINT32_MAX).
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
    le_pos_SampleRef_t  positionSampleRef, ///< [IN] Position sample's reference.
    uint32_t*           hSpeedPtr,         ///< [OUT] Horizontal speed.
    int32_t*            hSpeedAccuracyPtr  ///< [OUT] Horizontal speed's accuracy estimate.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's vertical speed.
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
    le_pos_SampleRef_t  positionSampleRef, ///< [IN] Position sample's reference.
    int32_t*            vSpeedPtr,         ///< [OUT] Vertical speed.
    int32_t*            vSpeedAccuracyPtr  ///< [OUT] Vertical speed's accuracy estimate.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's heading. Heading is the direction that
 * the vehicle/person is facing.
 *
 * @return LE_FAULT         Function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 * @return LE_OK            TFunction succeeded.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note headingPtr, headingAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetHeading
(
    le_pos_SampleRef_t positionSampleRef, ///< [IN] Position sample's reference.
    int32_t*           headingPtr,        ///< [OUT] Heading in degrees (where 0 is True North).
    int32_t*           headingAccuracyPtr ///< [OUT] Heading's accuracy estimate in degrees.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the position sample's direction. Direction of movement is the
 * direction that the vehicle/person is actually moving.
 *
 * @return LE_FAULT         Function failed to find the positionSample.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 *
 * @note directionPtr, directionAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_sample_GetDirection
(
    le_pos_SampleRef_t positionSampleRef,   ///< [IN] Position sample's reference.
    int32_t*           directionPtr,        ///< [OUT] Direction in degrees (where 0 is True North).
    int32_t*           directionAccuracyPtr ///< [OUT] Direction's accuracy estimate in degrees.
);

//--------------------------------------------------------------------------------------------------
/**
 * Release the position sample.
 *
 * @note If the caller is passing an invalid Position reference into this function,
 *       it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_pos_sample_Release
(
    le_pos_SampleRef_t    positionSampleRef    ///< [IN] Position sample's reference.
);

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
    int32_t*       latitudePtr,    ///< [OUT] Latitude in degrees, positive North.
    int32_t*       longitudePtr,   ///< [OUT] Longitude in degrees, positive East.
    int32_t*       hAccuracyPtr    ///< [OUT] Horizontal position's accuracy in metres.
);

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
    int32_t*       latitudePtr,    ///< [OUT] Latitude in degrees, positive North.
    int32_t*       longitudePtr,   ///< [OUT] Longitude in degrees, positive East.
    int32_t*       hAccuracyPtr,   ///< [OUT] Horizontal position's accuracy in metres.
    int32_t*       altitudePtr,    ///< [OUT] Altitude in metres, above Mean Sea Level.
    int32_t*       vAccuracyPtr    ///< [OUT] Vertical position's accuracy in metres.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the motion's data (Horizontal Speed, Horizontal Speed's
 * accuracy, Vertical Speed, Vertical Speed's accuracy).
 *
 * @return LE_FAULT         Function failed to get the motion's data.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX, UINT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note hSpeedPtr, hSpeedAccuracyPtr, vSpeedPtr, vSpeedAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetMotion
(
    uint32_t*   hSpeedPtr,          ///< [OUT] Horizontal Speed in m/sec.
    int32_t*    hSpeedAccuracyPtr,  ///< [OUT] Horizontal Speed's accuracy in m/sec.
    int32_t*    vSpeedPtr,          ///< [OUT] Vertical Speed in m/sec, positive up.
    int32_t*    vSpeedAccuracyPtr   ///< [OUT] Vertical Speed's accuracy in m/sec.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the heading indication.
 *
 * @return LE_FAULT         Function failed to get the heading indication.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note headingPtr, headingAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetHeading
(
    int32_t*  headingPtr,        ///< [OUT] Heading in degrees (where 0 is True North).
    int32_t*  headingAccuracyPtr ///< [OUT] Heading's accuracy in degrees.
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the direction indication. Direction of movement is the
 * direction that the vehicle/person is actually moving.
 *
 * @return LE_FAULT         Function failed to get the direction indication.
 * @return LE_OUT_OF_RANGE  One of the retrieved parameter is invalid (set to INT32_MAX).
 * @return LE_OK            Function succeeded.
 *
 * @note directionPtr, directionAccuracyPtr can be set to NULL if not needed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_pos_GetDirection
(
    int32_t* directionPtr,        ///< [OUT] Direction indication in degrees (where 0 is True North).
    int32_t* directionAccuracyPtr ///< [OUT] Direction's accuracy in degrees.
);

#endif // LEGATO_POS_INCLUDE_GUARD
