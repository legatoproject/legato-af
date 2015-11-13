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
 * Maintains the GNSS device state.
 */
//--------------------------------------------------------------------------------------------------
static le_gnss_State_t GnssState = LE_GNSS_STATE_UNINITIALIZED;

//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the GNSS
 *
 * @return LE_FAULT  The function failed.
 * @return LE_NOT_PERMITTED If the GNSS device is already initialized.
 * @return LE_OK     The function succeed.
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

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Set the GNSS constellation bit mask
 *
 * @return LE_FAULT         The function failed.
 * @return LE_UNSUPPORTED   If the request is not supported.
 * @return LE_NOT_PERMITTED If the GNSS device is not initialized, disabled or active.
 * @return LE_OK            The function succeeded.
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
    if (constellationMaskPtr == NULL)
    {
        LE_KILL_CLIENT("Pointer is NULL !");
        return LE_FAULT;
    }
    return (pa_gnss_GetConstellation(constellationMaskPtr));
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
 * @return LE_FAULT         The function failed to disable the 'Extended Ephemeris' file.
 * @return LE_OK            The function succeeded.
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
 * @return LE_FAULT         The function failed to inject the 'Extended Ephemeris' file.
 * @return LE_TIMEOUT       A time-out occurred.
 * @return LE_FORMAT_ERROR  'Extended Ephemeris' file format error.
 * @return LE_OK            The function succeeded.
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
 * @return LE_FAULT         The function failed to get the validity
 * @return LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_gnss_GetExtendedEphemerisValidity
(
    le_clk_Time_t *startTimePtr,    ///< [OUT] Start time
    le_clk_Time_t *stopTimePtr      ///< [OUT] Stop time
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

    return pa_gnss_GetExtendedEphemerisValidityTimes(startTimePtr,stopTimePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function starts the GNSS device.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_DUPLICATE     If the GNSS device is already started.
 * @return LE_NOT_PERMITTED If the GNSS device is not initialized or disabled.
 * @return LE_OK            The function succeeded.
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
 * @return LE_FAULT         The function failed.
 * @return LE_DUPLICATE     If the GNSS device is already stopped.
 * @return LE_NOT_PERMITTED If the GNSS device is not initialized or disabled.
 * @return LE_OK            The function succeeded.
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
 * @return LE_FAULT         The function failed.
 * @return LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 * @return LE_OK            The function succeeded.
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
 * @return LE_FAULT         The function failed.
 * @return LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 * @return LE_OK            The function succeeded.
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
 * @return LE_FAULT         The function failed.
 * @return LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 * @return LE_OK            The function succeeded.
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
 * @return LE_FAULT         The function failed.
 * @return LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 * @return LE_OK            The function succeeded.
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
 * @return LE_BUSY          The position is not fixed and TTFF can't be measured.
 * @return LE_NOT_PERMITTED If the GNSS device is not enabled or not started.
 * @return LE_OK            Function succeeded.
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
 * @return LE_FAULT         The function failed.
 * @return LE_DUPLICATE     If the GNSS device is already enabled.
 * @return LE_NOT_PERMITTED If the GNSS device is not initialized.
 * @return LE_OK            The function succeeded.
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
 * @return LE_FAULT         The function failed.
 * @return LE_DUPLICATE     If the GNSS device is already disabled.
 * @return LE_NOT_PERMITTED If the GNSS device is not initialized or started.
 * @return LE_OK            The function succeeded.
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
    uint8_t  suplCertificateId,      ///< [IN] Certificate ID of the SUPL certificate.
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
    uint8_t  suplCertificateId  ///< [IN]  Certificate ID of the SUPL certificate.
                                ///< Certificate ID range is 0 to 9
)
{
    return pa_gnss_DeleteSuplCertificate(suplCertificateId);
}
