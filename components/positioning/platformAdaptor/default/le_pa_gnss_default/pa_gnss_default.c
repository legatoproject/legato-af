/**
 * @file pa_gnss_default.c
 *
 * Default implementation of @ref c_pa_gnss.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_gnss.h"

//--------------------------------------------------------------------------------------------------
/**
 * Init this component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the PA GNSS Module.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_Init
(
    void
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release the PA GNSS Module.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t  __attribute__((weak)) pa_gnss_Release
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the GNSS constellation bit mask
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED request not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_SetConstellation
(
    le_gnss_ConstellationBitMask_t constellationMask  ///< [IN] GNSS constellation used in solution.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
le_result_t __attribute__((weak)) pa_gnss_GetConstellation
(
    le_gnss_ConstellationBitMask_t *constellationMaskPtr ///< [OUT] GNSS constellation used
                                                         ///< in solution
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start the GNSS acquisition.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_Start
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the GNSS acquisition.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_Stop
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_SetAcquisitionRate
(
    uint32_t rate     ///< [IN] rate in milliseconds
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the rate of GNSS fix reception
 *
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_GetAcquisitionRate
(
    uint32_t* ratePtr     ///< [IN] rate in milliseconds
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

// -------------------------------------------------------------------------------------------------
/**
 * Get the minimum NMEA rate supported on this platform
 *
 * @return LE_OK               Function succeeded.
 * @return LE_FAULT            Function failed.
 */
// -------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_GetMinNmeaRate
(
    uint32_t* minNmeaRatePtr    ///< [OUT] Minimum NMEA rate in milliseconds.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

// -------------------------------------------------------------------------------------------------
/**
 * Get the maximum NMEA rate supported on this platform
 *
 * @return LE_OK               Function succeeded.
 * @return LE_FAULT            Function failed.
 */
// -------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_GetMaxNmeaRate
(
    uint32_t* maxNmeaRatePtr    ///< [OUT] Maximum NMEA rate in milliseconds.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns a bitmask containing all NMEA sentences supported on this platform
 *
 * @return LE_OK               Function succeeded.
 * @return LE_FAULT            The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_GetSupportedNmeaSentences
(
    le_gnss_NmeaBitMask_t* nmeaMaskPtr    ///< [OUT] Supported NMEA sentences
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns a bitmask containing all satellite constellations supported on this platform
 *
 * @return LE_OK               Function succeeded.
 * @return LE_FAULT            The function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_GetSupportedConstellations
(
    le_gnss_ConstellationBitMask_t* constellationMaskPtr    ///< [OUT] Supported GNSS constellations
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler for GNSS position data notifications.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t __attribute__((weak)) pa_gnss_AddPositionDataHandler
(
    pa_gnss_PositionDataHandlerFunc_t handler ///< [IN] The handler function.
)
{
    LE_ERROR("Unsupported function called");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove a handler for GNSS position data notifications.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
void __attribute__((weak)) pa_gnss_RemovePositionDataHandler
(
    le_event_HandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
    LE_ERROR("Unsupported function called");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to register an handler for NMEA frames notifications.
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_event_HandlerRef_t __attribute__((weak)) pa_gnss_AddNmeaHandler
(
    pa_gnss_NmeaHandlerFunc_t handler ///< [IN] The handler function.
)
{
    LE_ERROR("Unsupported function called");
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove a handler for NMEA frames notifications.
 */
//--------------------------------------------------------------------------------------------------
void __attribute__((weak)) pa_gnss_RemoveNmeaHandler
(
    le_event_HandlerRef_t    handlerRef ///< [IN] The handler reference.
)
{
    LE_ERROR("Unsupported function called");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to load an 'Extended Ephemeris' file into the GNSS device.
 *
 * @return LE_FAULT         The function failed to inject the 'Extended Ephemeris' file.
 * @return LE_TIMEOUT       A time-out occurred.
 * @return LE_FORMAT_ERROR  'Extended Ephemeris' file format error.
 * @return LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_LoadExtendedEphemerisFile
(
    int32_t       fd      ///< [IN] extended ephemeris file descriptor
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the validity of the last injected Extended Ephemeris.
 *
 * @return LE_FAULT         The function failed to get the validity
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_GetExtendedEphemerisValidity
(
    uint64_t *startTimePtr,    ///< [OUT] Start time in seconds (since Jan. 1, 1970)
    uint64_t *stopTimePtr      ///< [OUT] Stop time in seconds (since Jan. 1, 1970)
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function enables the use of the 'Extended Ephemeris' file into the GNSS device.
 *
 * @return LE_FAULT         The function failed to enable the 'Extended Ephemeris' file.
 * @return LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_EnableExtendedEphemerisFile
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function disables the use of the 'Extended Ephemeris' file into the GNSS device.
 *
 * @return LE_FAULT         The function failed to disable the 'Extended Ephemeris' file.
 * @return LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_DisableExtendedEphemerisFile
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to inject UTC time into the GNSS device.
 *
 * @return
 *  - LE_OK            The function succeeded.
 *  - LE_FAULT         The function failed to inject the UTC time.
 *  - LE_TIMEOUT       A time-out occurred.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_InjectUtcTime
(
    uint64_t timeUtc,      ///< [IN] UTC time since Jan. 1, 1970 in milliseconds
    uint32_t timeUnc       ///< [IN] Time uncertainty in milliseconds
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function delete GNSS assistance data for warm/cold/factory start
 *
 * @return LE_FAULT           The function failed.
 * @return LE_OK              The function succeeded.
 * @return LE_UNSUPPORTED     Restart type not supported.
 * @return LE_BAD_PARAMETER   Bad input parameter.
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_DeleteAssistData
(
    le_gnss_StartMode_t mode    ///< [IN] Start mode
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the GNSS engine.
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_ForceEngineStop
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the TTFF in milliseconds.
 *
 * @return LE_BUSY          The position is not fixed and TTFF can't be measured.
 * @return LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_GetTtff
(
    uint32_t* ttffPtr     ///< [OUT] TTFF in milliseconds
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function enables the GNSS device.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_Enable
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function disables the GNSS device.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_Disable
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
le_result_t __attribute__((weak)) pa_gnss_SetSuplAssistedMode
(
    le_gnss_AssistedMode_t  assistedMode      ///< [IN] Assisted-GNSS mode.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
le_result_t __attribute__((weak)) pa_gnss_GetSuplAssistedMode
(
    le_gnss_AssistedMode_t *assistedModePtr      ///< [OUT] Assisted-GNSS mode.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
le_result_t __attribute__((weak)) pa_gnss_SetSuplServerUrl
(
    const char*  suplServerUrlPtr      ///< [IN] SUPL server URL.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
le_result_t __attribute__((weak)) pa_gnss_InjectSuplCertificate
(
    uint8_t  suplCertificateId,      ///< [IN] ID of the SUPL certificate.
                                     ///< Certificate ID range is 0 to 9
    uint16_t suplCertificateLen,     ///< [IN] SUPL certificate size in Bytes.
    const char*  suplCertificatePtr  ///< [IN] SUPL certificate contents.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
le_result_t __attribute__((weak)) pa_gnss_DeleteSuplCertificate
(
    uint8_t  suplCertificateId  ///< [IN]  ID of the SUPL certificate.
                                ///< Certificate ID range is 0 to 9
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the enabled NMEA sentences bit mask
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_BUSY service is busy
 *  - LE_TIMEOUT a time-out occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_SetNmeaSentences
(
    le_gnss_NmeaBitMask_t nmeaMask ///< [IN] Bit mask for enabled NMEA sentences.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the enabled NMEA sentences bit mask
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_BUSY service is busy
 *  - LE_TIMEOUT a time-out occurred
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_GetNmeaSentences
(
    le_gnss_NmeaBitMask_t* nmeaMaskPtr ///< [OUT] Bit mask for enabled NMEA sentences.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the GNSS minimum elevation.
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED request not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_SetMinElevation
(
    uint8_t  minElevation      ///< [IN] Minimum elevation in degrees [range 0..90].
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets leap seconds information
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_TIMEOUT on indication timeout
 *  - LE_UNSUPPORTED Not supported on this platform
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_GetLeapSeconds
(
    uint64_t* gpsTimePtr,              ///< [OUT] The number of milliseconds of GPS time since
                                       ///<       Jan. 6, 1980
    int32_t* currentLeapSecondsPtr,    ///< [OUT] Current UTC leap seconds value in milliseconds
    uint64_t* changeEventTimePtr,      ///< [OUT] The number of milliseconds since Jan. 6, 1980
                                       ///<       to the next leap seconds change event
    int32_t* nextLeapSecondsPtr        ///< [OUT] UTC leap seconds value to be applied at the
                                       ///<       change event time in milliseconds
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the GNSS minimum elevation.
 *
 * @return
*  - LE_OK on success
*  - LE_BAD_PARAMETER if minElevationPtr is NULL
*  - LE_FAULT on failure
*  - LE_UNSUPPORTED request not supported
*/
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_GetMinElevation
(
   uint8_t*  minElevationPtr     ///< [OUT] Minimum elevation in degrees [range 0..90].
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the area for the GNSS constellation
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED request not supported
 *  - LE_BAD_PARAMETER on invalid constellation area
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_SetConstellationArea
(
    le_gnss_Constellation_t satConstellation,        ///< [IN] GNSS constellation used in solution.
    le_gnss_ConstellationArea_t constellationArea    ///< [IN] GNSS constellation area.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the area for the GNSS constellation
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_UNSUPPORTED request not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_GetConstellationArea
(
    le_gnss_Constellation_t satConstellation,         ///< [IN] GNSS constellation used in solution.
    le_gnss_ConstellationArea_t* constellationAreaPtr ///< [OUT] GNSS constellation area.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Enables the EXT_GPS_LNA_EN signal
 *
 * @return LE_OK               Function succeeded.
 * @return LE_UNSUPPORTED      Function not supported on this platform
 *
 * @note The EXT_GPS_LNA_EN signal will be set high when the GNSS state is active
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_EnableExternalLna
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disables the EXT_GPS_LNA_EN signal
 *
 * @return LE_OK               Function succeeded.
 * @return LE_UNSUPPORTED      Function not supported on this platform
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t __attribute__((weak)) pa_gnss_DisableExternalLna
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert a location data parameter from/to multi-coordinate system
 *
 * @return
 *  - LE_OK on success
 *  - LE_FAULT on failure
 *  - LE_BAD_PARAMETER if locationDataDstPtr is NULL
 *  - LE_UNSUPPORTED request not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t __attribute__((weak)) pa_gnss_ConvertDataCoordinateSystem
(
    le_gnss_CoordinateSystem_t coordinateSrc,     ///< [IN] Coordinate system to convert from.
    le_gnss_CoordinateSystem_t coordinateDst,     ///< [IN] Coordinate system to convert to.
    le_gnss_LocationDataType_t locationDataType,  ///< [IN] Type of location data to convert.
    int64_t locationDataSrc,                      ///< [IN] Data to convert.
    int64_t* locationDataDstPtr                   ///< [OUT] Converted Data.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}
