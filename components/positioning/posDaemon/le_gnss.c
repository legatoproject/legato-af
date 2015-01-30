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
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the GNSS
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void gnss_Init
(
    void
)
{
    LE_FATAL_IF((pa_gnss_Init() != LE_OK),"Failed to initialize the PA GNSS module");

    LE_DEBUG("GNSS ready.");
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to load an 'Extended Ephemeris' file into the GNSS device.
 *
 * @return LE_FAULT         The function failed to inject the 'Extended Ephemeris' file.
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
 * This function clears ephemeris, position and time data before performing a restart.
 *
 * @return LE_FAULT         The function failed.
 * @return LE_OK            The function succeeded.
 *
 */
//--------------------------------------------------------------------------------------------------

le_result_t le_gnss_ForceColdRestart
(
    void
)
{
    return pa_gnss_ForceRestart(PA_GNSS_COLD_RESTART);
}

