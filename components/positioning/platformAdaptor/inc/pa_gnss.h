/**
 * @page c_pa_gnss Platform Adapter Global Navigation Satellite System API
 *
 * @ref pa_gnss.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */


/** @file pa_gnss.h
 *
 * Legato @ref c_pa_gnss include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */

#ifndef LEGATO_PA_GNSS_INCLUDE_GUARD
#define LEGATO_PA_GNSS_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------

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
 * Position structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct {
    bool               latitudeValid; ///< if true, latitude is set
    int32_t            latitude;  ///< The Latitude in degrees, positive North,
                                  ///  with 6 decimal places (+48858300  = 48.858300 degrees North).

    bool               longitudeValid; ///< if true, longitude is set
    int32_t            longitude; ///< The Longitude in degrees, positive East,
                                  ///  with 6 decimal places.

    bool               altitudeValid; ///< if true, altitude is set
    int32_t            altitude;  ///< The Altitude in meters, above Mean Sea Level,
                                  ///  with 3 decimal places.

    bool               hSpeedValid; ///< if true, horizontal speed is set
    uint32_t           hSpeed;    ///< The horizontal Speed in m/sec, with 2 decimal places
                                  ///  (125 = 1.25m/sec).

    bool               vSpeedValid; ///< if true, vertical speed is set
    uint32_t           vSpeed;    ///< The vertical Speed in m/sec, with 2 decimal places
                                  ///  (125 = 1.25m/sec).

    bool               trackValid; ///< if true, Track is set
    uint32_t           track;     ///< Track (direction) in degrees, where 0 is True North, with 1
                                  ///  decimal place (308  = 30.8 degrees).

    bool               headingValid; ///< if true, heading is set
    uint32_t           heading;     ///< heading in degrees, where 0 is True North, with 1
                                  ///  decimal place (308  = 30.8 degrees).

    bool               hdopValid; ///< if true, horizontal dilution is set
    uint16_t           hdop;      ///< The horizontal Dilution of Precision (DOP)

    bool               hUncertaintyValid; ///< if true, horizontal uncertainty is set
    uint32_t           hUncertainty;  ///< The horizontal uncertainty in meters,
                                      ///  with 1 decimal place

    bool               vUncertaintyValid; ///< if true, vertical uncertainty is set
    uint32_t           vUncertainty;  ///< The vertical uncertainty in meters,
                                      ///  with 1 decimal place

    bool               hSpeedUncertaintyValid; ///< if true, horizontal speed uncertainty is set
    uint32_t           hSpeedUncertainty;  ///< The horizontal speed uncertainty in m/sec,
                                           ///  with 1 decimal place

    bool               vSpeedUncertaintyValid; ///< if true, vertical speed uncertainty is set
    uint32_t           vSpeedUncertainty;  ///< The vertical speed uncertainty in m/sec,
                                           ///  with 1 decimal place

    bool               headingUncertaintyValid; ///< if true, heading uncertainty is set
    uint32_t           headingUncertainty;  ///< The heading uncertainty in degrees,
                                            /// with 1 decimal place

    bool               trackUncertaintyValid; ///< if true, track uncertainty is set
    uint32_t           trackUncertainty;  ///< The track uncertainty in degrees,
                                          ///  with 1 decimal place

    bool               vdopValid; ///< if true, vertical dilition is set
    uint16_t           vdop;      ///< The vertical Dilution of Precision (DOP)

    bool               timeValid; ///< if true, time is set
    pa_Gnss_Time_t     time;  ///< The time of the fix

    bool               dateValid; ///< if true, date is set
    pa_Gnss_Date_t     date;  ///< The date of the fix
}
pa_Gnss_Position_t;

/** Reference to a position structure. */
typedef pa_Gnss_Position_t* pa_Gnss_Position_Ref_t;

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the PA GNSS Module.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release the PA GNSS Module.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_Release
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start the GNSS acquisition.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_Start
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the GNSS acquisition.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_Stop
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the rate of GNSS fix reception
 *
 * @return LE_NOT_POSSIBLE  The function failed.
 * @return LE_TIMEOUT       No response was received.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_SetAcquisitionRate
(
    uint32_t rate     ///< [IN] rate in seconds
);

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for handler functions used to get GNSS position data.
 *
 * @param position The new position.
 */
//--------------------------------------------------------------------------------------------------
typedef void(*pa_gnss_PositionDataHandlerFunc_t)(pa_Gnss_Position_Ref_t position);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler for GNSS position data notifications.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t pa_gnss_AddPositionDataHandler
(
    pa_gnss_PositionDataHandlerFunc_t handler ///< [IN] The handler function.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove a handler for GNSS position data notifications.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void pa_gnss_RemovePositionDataHandler
(
    le_event_HandlerRef_t    handlerRef ///< [IN] The handler reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the location's data.
 *
 * @return LE_NOT_POSSIBLE  The function cannot get internal position information
 * @return LE_OK            The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_GetLastPositionData
(
    pa_Gnss_Position_Ref_t  positionRef   ///< [OUT] Reference to a position struct
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to load xtra.bin file into the gnss.
 *
 * @return LE_FAULT         The function failed to load the xtra.bin file
 * @return LE_NOT_FOUND     The file path does not exist
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_LoadXtra
(
    const char* xtraFilePathPtr    ///< [IN] xtra.bin file path
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the validity of the last xtra.bin injected
 *
 * @return LE_FAULT         The function failed to get the validity
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_GetXtraValidityTimes
(
    le_clk_Time_t *startTimePtr,    ///< [OUT] Start time
    le_clk_Time_t *stopTimePtr      ///< [OUT] Stop time
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the status of Xtra session
 *
 * @return LE_FAULT         The function failed to get the status
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_GetXtraSessionStatus
(
    bool *enablePtr ///< [OUT] enable/disable
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to enable Xtra session.
 *
 * @return LE_FAULT         The function failed to enable.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_EnableXtraSession
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to disable Xtra session.
 *
 * @return LE_FAULT         The function failed to disable.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_gnss_DisableXtraSession
(
    void
);

#endif // LEGATO_PA_GNSS_INCLUDE_GUARD
