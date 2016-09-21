/**
 * @file fwupdateServer.c
 *
 * Implementation of FW Update API
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */
#include "legato.h"
#include "interfaces.h"
#include "pa_fwupdate.h"

//==================================================================================================
//                                       Private Functions
//==================================================================================================
//--------------------------------------------------------------------------------------------------
/**
 * Function to be treated at device init
 */
//--------------------------------------------------------------------------------------------------
void SyncAtStartupCheck
(
    void* param1,   ///< Not used
    void *param2    ///< Not used
)
{
    bool sync;
    le_result_t result;
    /* Check if a SYNC operation needs to be made */
    result = pa_fwupdate_DualSysCheckSync (&sync);
    LE_DEBUG ("SyncAtStartupCheck pa_fwupdate_DualSysCheckSync %d sync %d", result, sync);
    if ((result == LE_OK) && sync)
    {
        /* Make a sync operation */
        result = pa_fwupdate_DualSysSync();
        if (result != LE_OK)
        {
            LE_ERROR ("FW update component init: Sync failure %d", result);
        }
    }
}


//==================================================================================================
//                                       Public API Functions
//==================================================================================================


//--------------------------------------------------------------------------------------------------
/**
 * Download the firmware image file to the modem.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if an input parameter is not valid
 *      - LE_FAULT on failure
 *
 * @note
 *      The process exits, if an invalid file descriptor (e.g. negative) is given.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_Download
(
    int fd
        ///< [IN]
        ///< File descriptor of the image, opened to the start of the image.
)
{
    // fd must be positive
    if (fd < 0)
    {
        LE_KILL_CLIENT("'fd' is negative");
        return LE_BAD_PARAMETER;
    }

    // Pass the fd to the PA layer, which will handle the details.
    return pa_fwupdate_Download(fd);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the firmware version string
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_FAULT for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_GetFirmwareVersion
(
    char* version,
        ///< [OUT]
        ///< Firmware version string

    size_t versionNumElements
        ///< [IN]
)
{
    return pa_fwupdate_GetFirmwareVersion(version, versionNumElements);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the bootloader version string
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_FAULT for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_GetBootloaderVersion
(
    char* version,
        ///< [OUT]
        ///< Bootloader version string

    size_t versionNumElements
        ///< [IN]
)
{
    return pa_fwupdate_GetBootloaderVersion(version, versionNumElements);
}

//--------------------------------------------------------------------------------------------------
/**
 * Function which indicates if Active and Update systems are synchronized
 *
 * @return
 *      - LE_OK            On success
 *      - LE_UNSUPPORTED   The feature is not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_DualSysSyncState
(
    bool *isSync ///< [OUT] true if both systems are synchronized, false otherwise
)
{
    le_result_t result = LE_FAULT;
    /* Get the dual system synchronization state from PA */
    result = pa_fwupdate_DualSysGetSyncState( isSync );

    LE_DEBUG ("le_fwupdate_DualSysSyncState: result %d, isSync %d", result, *isSync);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Request a full system reset with a systems swap.
 *
 * After the reset, the UPDATE and ACTIVE systems will be swapped.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_UNSUPPORTED   The feature is not supported
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_DualSysSwap
(
    void
)
{
    le_result_t result = pa_fwupdate_DualSysSwap (false);

    if (result == LE_OK)
    {
        /* make a system reset */
        pa_fwupdate_Reset();
        /* at this point the system is reseting */
    }

    LE_DEBUG ("le_fwupdate_DualSysSwap result %d", result);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Request a full system reset with a systems SYNC.
 *
 * After the reset, the UPDATE system will be synchronised with the ACTIVE one.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_UNSUPPORTED   The feature is not supported
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_DualSysSync
(
    void
)
{
    le_result_t result = pa_fwupdate_DualSysSync();
    if (result == LE_FAULT)
    {
        LE_DEBUG ("le_fwupdate_DualSysSync sync failure --> pass SW update to NORMAL");
        result = pa_fwupdate_SetState (PA_FWUPDATE_STATE_NORMAL);
    }
    LE_DEBUG ("le_fwupdate_DualSysSync result %d", result);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Request a full system reset with a systems SWAP and systems SYNC.
 *
 * After the reset, the UPDATE and ACTIVE systems will be swapped and synchronized.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_UNSUPPORTED   The feature is not supported
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_DualSysSwapAndSync
(
    void
)
{
    le_result_t result;

    /* Program the SWAP */
    result = pa_fwupdate_DualSysSwap (true);
    if (result == LE_OK)
    {
        /* make a system reset */
        pa_fwupdate_Reset();
        /* at this point the system is reseting */
    }
    
    LE_DEBUG ("le_fwupdate_DualSysSwapAndSync result %d", result);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization function for FwUpdate Daemon
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_event_QueueFunction (SyncAtStartupCheck, NULL, NULL);
}

