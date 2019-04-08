//--------------------------------------------------------------------------------------------------
/**
 * @file pa_fwupdate_default.c
 *
 * Default implementation of @ref pa_fwupdate interface
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "pa_fwupdate.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function starts a package download to the device.
 *
 * @warning This API is a blocking API. It needs to be called in a dedicated thread.
 *
 * @return
 *      - LE_OK              On success
 *      - LE_BAD_PARAMETER   If an input parameter is not valid
 *      - LE_TIMEOUT         After 900 seconds without data received
 *      - LE_NOT_POSSIBLE    The systems are not synced
 *      - LE_UNAVAILABLE     The flash access is not granted for SW update
 *      - LE_CLOSED          File descriptor has been closed before all data have been received
 *      - LE_OUT_OF_RANGE    Storage is too small
 *      - LE_FAULT           On failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_Download
(
    int fd   ///< [IN] File descriptor of the image, opened to the start of the image.
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Return the update package write position.
 *
 * @note This is actually the position within the update package, not the one once the update
 * package is processed (unzipping, extracting, ... ).
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 *      - LE_UNSUPPORTED not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_GetResumePosition
(
    size_t *positionPtr
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Return the last update status.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER Invalid parameter
 *      - LE_FAULT on failure
 *      - LE_UNSUPPORTED not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_GetUpdateStatus
(
    pa_fwupdate_UpdateStatus_t *statusPtr, ///< [OUT] Returned update status
    char *statusLabelPtr,                  ///< [OUT] String matching the status
    size_t statusLabelLength               ///< [IN] Maximum length of the status description
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the image file from the modem.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 *      - LE_UNSUPPORTED not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_Read
(
    int* fdPtr         ///< [OUT] File descriptor for the image, ready for reading.
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_GetFirmwareVersion
(
    char* versionPtr,        ///< [OUT] Firmware version string
    size_t versionSize       ///< [IN] Size of version buffer
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_GetBootloaderVersion
(
    char* versionPtr,        ///< [OUT] Firmware version string
    size_t versionSize       ///< [IN] Size of version buffer
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_GetAppBootloaderVersion
(
    char* versionPtr,        ///< [OUT] Firmware version string
    size_t versionSize       ///< [IN] Size of version buffer
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Installs the firmware package.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    the feature is not supported
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_Install
(
    bool isMarkGoodReq      ///< [IN] Indicate if a mark good operation is requested after install
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Mark the system as good.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    the feature is not supported
 *      - LE_UNAVAILABLE    the flash access is not granted for SW update
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_MarkGood
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Retrieves the state of the system.
 *
 * @return
 *      - LE_OK            on success
 *      - LE_UNSUPPORTED   the feature is not supported
 *      - LE_FAULT         else
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_GetSystemState
(
    bool *isSystemGoodPtr ///< [OUT] Indicates if the system is marked good
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to reset the device. This function does not return any error code.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_fwupdate_Reset
(
    void
)
{
    LE_ERROR("Unsupported function called");
}

//--------------------------------------------------------------------------------------------------
/**
 * Function which indicates if a Sync operation is needed (swap & sync operation)
 *
 * @return
 *      - LE_OK            on success
 *      - LE_UNSUPPORTED   the feature is not supported
 *      - LE_FAULT         else
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_DualSysCheckSync
(
    bool *isSyncReqPtr ///< [OUT] Indicates if synchronization is requested
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Request a full system reset with a systems SWAP and systems SYNC.
 *
 * After the reset, the UPDATE and ACTIVE systems will be swapped and synchronized.
 *
 * @return
 *      - LE_OK            on success
 *      - LE_UNSUPPORTED   the feature is not supported
 *      - LE_FAULT         on failure
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_InstallAndMarkGood
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * This API is to be called at the beginning of a SYNC operation.
 * It updates the SW update state field in SSDATA
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_SetState
(
    pa_fwupdate_state_t state   ///< [IN] state to set
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * request the modem to apply the NVUP files in UD system
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    the feature is not supported
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_NvupApply
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the resume context
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    the feature is not supported
 *      - LE_FAULT          on failure
 *      - LE_NO_MEMORY      on memory allocation failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_InitDownload
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Disable (true) or enable (false) the synchronisation check before performing an update.
 * The default behavior at startup is always to have the check enabled. It remains enabled
 * until this service is called with the value true. To re-enable the synchronization check
 * call this service with the value false.
 *
 * @note Upgrading some partitions without performing a sync before may let the whole system
 *       into a unworkable state. THIS IS THE RESPONSABILITY OF THE CALLER TO KNOW WHAT IMAGES
 *       ARE ALREADY FLASHED INTO THE UPDATE SYSTEM.
 *
 * @return
 *      - LE_OK              On success
 *      - LE_UNSUPPORTED     The feature is not supported
 *      - LE_FAULT           On failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_DisableSyncBeforeUpdate
(
    bool isDisabled  ///< [IN] State of sync check : true (disable) or false (enable)
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Define a new "system" by setting the 3 sub-systems.  This system will become the current system
 * in use after the reset performed by this service, if no error are reported.
 *
 * @return
 *      - LE_BAD_PARAMETER   If an input parameter is not valid
 *      - LE_FAULT           On failure
 *      - LE_UNSUPPORTED     The feature is not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_SetSystem
(
    pa_fwupdate_System_t systemArray[PA_FWUPDATE_SUBSYSID_MAX]
                         ///< [IN] System array for "modem/lk/linux" partition groups
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the current "system" in use.
 *
 * @return
 *      - LE_OK            On success
 *      - LE_FAULT         On failure
 *      - LE_UNSUPPORTED   The feature is not supported
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_GetSystem
(
    pa_fwupdate_System_t systemArray[PA_FWUPDATE_SUBSYSID_MAX]
                         ///< [OUT] System array for "modem/lk/linux" partition groups
)
{
    LE_ERROR("Unsupported function called");
    return LE_UNSUPPORTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * Start the bad image indication
 *
 * @return
 *      - LE_OK             on success
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_StartBadImageIndication
(
    le_event_Id_t eventId       ///< the event Id to use to report the bad image
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Stop the bad image indication
 *
 * @return
 *      - LE_OK             on success
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
void pa_fwupdate_StopBadImageIndication
(
    void
)
{
    LE_ERROR("Unsupported function called");
}

//--------------------------------------------------------------------------------------------------
/**
 * Request the flash access for a SW update
 *
 * @return
 *      - LE_OK            on success
 *      - LE_UNAVAILABLE   the flash access is not granted for SW update
 *      - LE_FAULT         on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_RequestUpdate
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Release the flash access after a SW update
 *
 * @return
 *      - LE_OK           on success
 *      - LE_FAULT        on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_CompleteUpdate
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the MTD partition table
 *
 * @return
 *      - LE_OK            on success
 *      - LE_BAD_PARAMETER if mtdPartPtr is NULL
 *      - LE_FAULT         on other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_GetMtdPartitionTab
(
    pa_fwupdate_MtdPartition_t **mtdPartPtr
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open SWIFOTA partition
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    the feature is not supported
 *      - LE_DUPLICATE      parition is already opened
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_OpenSwifota
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Close SWIFOTA partition
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    the feature is not supported
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_CloseSwifota
(
    void
)
{
    LE_ERROR("Unsupported function called");
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * Init this component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}


