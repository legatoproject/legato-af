/**
 * @file fwupdateServer.c
 *
 * Implementation of FW Update API
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */
#include "legato.h"
#include "interfaces.h"
#include "pa_fwupdate.h"
#include "watchdogChain.h"
#include "fwupdate_local.h"

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 8

//--------------------------------------------------------------------------------------------------
//                                       Static declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Wakeup source to keep system awake during download/update.
 */
//--------------------------------------------------------------------------------------------------
static le_pm_WakeupSourceRef_t WakeupSource = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Obtain a wake lock - prevent system from suspending during FOTA operations
 */
//--------------------------------------------------------------------------------------------------
static void SetWakeLock
(
    void
)
{
    le_result_t result;

    if (WakeupSource)
    {
        LE_ERROR("Wakeup source and wakelock already exist.");
        return;
    }

    LE_DEBUG("Connecting to PowerManager");
    result = le_pm_TryConnectService();
    if (LE_OK != result)
    {
        LE_WARN("PowerManager is unavailable, %s", LE_RESULT_TXT(result));
        return;
    }

    WakeupSource = le_pm_NewWakeupSource(0, "FWUpdate");
    if (NULL == WakeupSource)
    {
        LE_ERROR("Can't create wakeup source");
        le_pm_DisconnectService();
        return;
    }

    result = le_pm_StayAwake(WakeupSource);
    LE_ERROR_IF((LE_OK != result), "Can't StayAwake, err %s", LE_RESULT_TXT(result));
}

//--------------------------------------------------------------------------------------------------
/**
 * Release a wake lock
 */
//--------------------------------------------------------------------------------------------------
static void ReleaseWakeLock
(
    void
)
{
    if (WakeupSource)
    {
        LE_DEBUG("Disconnecting from PowerManager");
        // on disconnect, PM removes wakeup sources for this client.
        le_pm_DisconnectService();
        WakeupSource = NULL;
    }
}


//==================================================================================================
//                                       Public API Functions
//==================================================================================================


//--------------------------------------------------------------------------------------------------
/**
 * Download the firmware image file into the update partition. The function can also be used to
 * resume the download if the @c le_fwupdate_InitDownload function is not called before.
 *
 * @return
 *      - LE_OK              On success
 *      - LE_BAD_PARAMETER   If an input parameter is not valid
 *      - LE_TIMEOUT         After 900 seconds without data received
 *      - LE_NOT_PERMITTED   The systems are not synced
 *      - LE_UNAVAILABLE     The flash access is not granted for SW update
 *      - LE_CLOSED          File descriptor has been closed before all data have been received
 *      - LE_OUT_OF_RANGE    Storage is too small
 *      - LE_FAULT           On failure
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

    SetWakeLock();

    // Pass the fd to the PA layer, which will handle the details.
    le_result_t result = pa_fwupdate_Download(fd);

    ReleaseWakeLock();

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Download initialization:
 *  - for single and dual systems, it resets resume position,
 *  - for dual systems, it synchronizes the systems if needed.
 *
 * @note
 *      When invoked, resuming a previous download is not possible, a full update package has to be
 *      downloaded.
 *
 * @return
 *      - LE_OK         On success
 *      - LE_FAULT      On failure
 *      - LE_IO_ERROR   Dual systems platforms only -- The synchronization fails due to
 *                      unrecoverable ECC errors. In this case, the update without synchronization
 *                      is forced, but the whole system must be updated to ensure that the new
 *                      update system will be workable
 *                      ECC stands for Error-Correction-Code: some errors may be corrected. If this
 *                      correction fails, a unrecoverable error is registered and the data become
 *                      corrupted.
 *      - LE_NO_MEMORY  On memory allocation failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_InitDownload
(
    void
)
{
    return pa_fwupdate_InitDownload();
}

//--------------------------------------------------------------------------------------------------
/**
 * Return the downloaded update package write position.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER Invalid parameter
 *      - LE_FAULT on failure
 */;
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_GetResumePosition
(
    size_t *positionPtr     ///< [OUT] Update package read position
)
{
    return pa_fwupdate_GetResumePosition(positionPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Return the update status, which is either the last status of the systems swap if it failed, or
 * the status of the secondary bootloader (SBL).
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER Invalid parameter
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_GetUpdateStatus
(
    le_fwupdate_UpdateStatus_t *statusPtr, ///< [OUT] Returned update status
    char *statusLabelPtr,                  ///< [OUT] Points to the string matching the status
    size_t statusLabelLength               ///< [IN]  Maximum label length
)
{
    le_result_t result = LE_BAD_PARAMETER;
    pa_fwupdate_UpdateStatus_t paStatus;

    // Check the parameters
    if (NULL == statusPtr)
    {
        LE_ERROR("Invalid parameter.");
        return result;
    }

    // Get the update status from the PA
    result = pa_fwupdate_GetUpdateStatus(&paStatus, statusLabelPtr, statusLabelLength);

    if (LE_OK == result)
    {
        switch (paStatus)
        {
            case PA_FWUPDATE_UPDATE_STATUS_OK:
                *statusPtr = LE_FWUPDATE_UPDATE_STATUS_OK;
                break;
            case PA_FWUPDATE_UPDATE_STATUS_PARTITION_ERROR:
                *statusPtr = LE_FWUPDATE_UPDATE_STATUS_PARTITION_ERROR;
                break;
            case PA_FWUPDATE_UPDATE_STATUS_DWL_ONGOING:
                *statusPtr = LE_FWUPDATE_UPDATE_STATUS_DWL_ONGOING;
                break;
            case PA_FWUPDATE_UPDATE_STATUS_DWL_FAILED:
                *statusPtr = LE_FWUPDATE_UPDATE_STATUS_DWL_FAILED;
                break;
            case PA_FWUPDATE_UPDATE_STATUS_DWL_TIMEOUT:
                *statusPtr = LE_FWUPDATE_UPDATE_STATUS_DWL_TIMEOUT;
                break;
            case PA_FWUPDATE_UPDATE_STATUS_UNKNOWN:
                *statusPtr = LE_FWUPDATE_UPDATE_STATUS_UNKNOWN;
                break;
            default:
                LE_ERROR("Invalid PA status (%d)!", paStatus);
                *statusPtr = LE_FWUPDATE_UPDATE_STATUS_UNKNOWN;
                break;
        }
    }
    else
    {
        LE_ERROR("Unable to determine the FW update status!");
        *statusPtr = LE_FWUPDATE_UPDATE_STATUS_UNKNOWN;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the firmware version string
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_OVERFLOW if version string to big to fit in provided buffer
 *      - LE_FAULT for any other errors
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_GetFirmwareVersion
(
    char* versionPtr,
        ///< [OUT]
        ///< Firmware version string

    size_t versionNumElements
        ///< [IN]
)
{
    // Check input parameters
    if (versionPtr == NULL)
    {
        LE_KILL_CLIENT("versionPtr is NULL !");
        return LE_FAULT;
    }
    if (versionNumElements == 0)
    {
        LE_ERROR("parameter error");
        return LE_FAULT;
    }
    return pa_fwupdate_GetFirmwareVersion(versionPtr, versionNumElements);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the bootloader version string
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_OVERFLOW if version string to big to fit in provided buffer
 *      - LE_FAULT for any other errors
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_GetBootloaderVersion
(
    char* versionPtr,
        ///< [OUT]
        ///< Bootloader version string

    size_t versionNumElements
        ///< [IN]
)
{
    // Check input parameters
    if (versionPtr == NULL)
    {
        LE_KILL_CLIENT("versionPtr is NULL !");
        return LE_FAULT;
    }
    if (versionNumElements == 0)
    {
        LE_ERROR("parameter error");
        return LE_FAULT;
    }
    return pa_fwupdate_GetBootloaderVersion(versionPtr, versionNumElements);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the custom system version string at the specified index
 *
 * @return
 *      - LE_OK on success
 *      - LE_OUT_OF_RANGE if the index specified is greater than the number of versions available,
 *                        or greater than the number of versions allowed to be returned.
 *      - LE_OVERFLOW if the version string cannot fit in provided buffer
 *      - LE_NOT_FOUND if opening a version containing file fails
 *      - LE_FAULT if reading a version containing file fails
 *      - LE_UNAVAILABLE if the configTree is unavailable
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_GetSystemVersion
(
    uint8_t index,
        ///< [IN] Index to retrieve

    char* versionNamePtr,
        ///< [OUT] System version name string

    size_t versionNameNumElements,
        ///< [IN] Buffer size

    char* versionPtr,
        ///< [OUT] System version string

    size_t versionNumElements
        ///< [IN] Buffer size
)
{
#if LE_CONFIG_ENABLE_CONFIG_TREE
    // The index must be less than the maximum number of system versions allowed
    if (index >= LE_FWUPDATE_MAX_NUM_VERSIONS)
    {
        LE_ERROR("The index requested exceeds the maximum number of versions allowed");
        return LE_OUT_OF_RANGE;
    }

    // Read the system versions from configtree
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn("framework/systemVersions");

    if (le_cfg_GoToFirstChild(iterRef) != LE_OK)
    {
        le_cfg_CancelTxn(iterRef);
        return LE_OUT_OF_RANGE;
    }

    // Loop until the specified version at index is reached
    for (uint8_t i = 0; i < index; i++)
    {
        // If specified version at index cannot be reached, return
        if (le_cfg_GoToNextSibling(iterRef) != LE_OK)
        {
            le_cfg_CancelTxn(iterRef);
            return LE_OUT_OF_RANGE;
        }
    }

    // Get the version name
    if (le_cfg_GetNodeName(iterRef, "", versionNamePtr, versionNameNumElements) == LE_OVERFLOW)
    {
        LE_ERROR("Version name buffer size is too small: %" PRIuS, versionNameNumElements);
        le_cfg_CancelTxn(iterRef);
        return LE_OVERFLOW;
    }

    // Get the version
    if (le_cfg_GetString(iterRef, "", versionPtr, versionNumElements, "") == LE_OVERFLOW)
    {
        LE_ERROR("Version buffer size is too small: %" PRIuS, versionNumElements);
        le_cfg_CancelTxn(iterRef);
        return LE_OVERFLOW;
    }

    le_cfg_CancelTxn(iterRef);

    // Check if version is a file path
    char* fileHeader = "file:";
    if (strncmp(versionPtr, fileHeader, strlen(fileHeader)) == 0)
    {
        char* path = strtok(versionPtr, ":");
        // The path is the second token after splitting the string
        path = strtok(NULL, " ");

        FILE* fp;
        fp = fopen(path, "r");
        if (fp == NULL)
        {
            LE_ERROR("Failed to open %s", path);
            return LE_NOT_FOUND;
        }

        if (fgets(versionPtr, versionNumElements, fp) == NULL)
        {
            LE_ERROR("Failed to read %s", path);
            if (fclose(fp) != 0)
            {
                LE_ERROR("Failed to close %s", path);
            }
            return LE_FAULT;
        }

        if (fclose(fp) != 0)
        {
            LE_ERROR("Failed to close %s", path);
        }

        // Remove trailing newline
        strtok(versionPtr, "\n");
    }

    return LE_OK;
#else
    // If the configTree is not enabled, system versions cannot be gotten
    return LE_UNAVAILABLE;
#endif
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the app bootloader version string
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_OVERFLOW if version string to big to fit in provided buffer
 *      - LE_BAD_PARAMETER bad parameter
 *      - LE_UNSUPPORTED not supported
 *      - LE_FAULT for any other errors
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_GetAppBootloaderVersion
(
    char* versionPtr, ///< [OUT] App Bootloader version string
    size_t bufferSize ///< [IN] Buffer size
)
{
    // Check input parameters
    if (versionPtr == NULL)
    {
        LE_KILL_CLIENT("versionPtr is NULL !");
        return LE_BAD_PARAMETER;
    }
    if (bufferSize == 0)
    {
        LE_ERROR("buffer size is 0");
        return LE_BAD_PARAMETER;
    }
    return pa_fwupdate_GetAppBootloaderVersion(versionPtr, bufferSize);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the status of the system.
 *
 * Dual System: Indicates if Active and Update systems are synchronized
 * Single System: This api is not supported on single system.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_UNSUPPORTED   The feature is not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_IsSystemMarkedGood
(
    bool* isSystemGoodPtr ///< [OUT] true if the system is marked good, false otherwise
)
{
    if (isSystemGoodPtr == NULL)
    {
        LE_KILL_CLIENT("isSystemGoodPtr is NULL.");
        return LE_FAULT;
    }

    /* Get the system synchronization state from PA */
    le_result_t result = pa_fwupdate_GetSystemState( isSystemGoodPtr );

    LE_DEBUG ("result %d, isSystemGood %d", result, *isSystemGoodPtr);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Request to install the package. Calling this API will result in a system reset.
 *
 * Dual System: After reset, the UPDATE and ACTIVE systems will be swapped.
 * Single System: After reset, the image in the FOTA partition will be installed on the device.
 *
 * @note On success, a device reboot will be initiated.
 *
 *
 * @return
 *      - LE_BUSY          Download is ongoing, install is not allowed
 *      - LE_UNSUPPORTED   The feature is not supported
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_Install
(
    void
)
{
    le_result_t result;

    SetWakeLock();

    /* request system install */
    result = pa_fwupdate_Install(false);

    ReleaseWakeLock();

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Mark the current system as good.
 *
 * Dual System: Requests a system Sync. The UPDATE system will be synchronised with the ACTIVE one.
 * Single System: This api is not supported on single system.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_UNSUPPORTED   The feature is not supported
 *      - LE_UNAVAILABLE   The flash access is not granted for SW update
 *      - LE_FAULT         On failure
 *      - LE_IO_ERROR      Dual systems platforms only -- The synchronization fails due to
 *                         unrecoverable ECC errors
 *                         ECC stands for Error-Correction-Code: some errors may be corrected. If
 *                         this correction fails, an unrecoverable error is registered and the data
 *                         become corrupted.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_MarkGood
(
    void
)
{
    le_result_t result = pa_fwupdate_MarkGood();
    LE_DEBUG ("result %d", result);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Request to install the package and marks the system as good. Calling this API will result in a
 * system reset.
 *
 * Dual System: Request a full system reset with a systems SWAP and systems SYNC. After the reset,
 * the UPDATE and ACTIVE systems will be swapped and synchronized.
 * Single System: Installs the package from the FOTA partition.
 *
 *
 * @note On success, a device reboot is initiated without returning any value.
 *
 * @return
 *      - LE_FAULT         On failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_InstallAndMarkGood
(
    void
)
{
    le_result_t result;

    SetWakeLock();

    /* request the swap and sync */
    result = pa_fwupdate_Install(true);
    /* the previous function returns only if there has been an error */
    LE_ERROR(" !!! Error %s", LE_RESULT_TXT(result));

    ReleaseWakeLock();

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialization function for FwUpdate Daemon
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("FW update is ready");

    // Monitor main loop with the watchdog
    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(FWUPDATE_WDOG_TIMER, watchdogInterval);
}

