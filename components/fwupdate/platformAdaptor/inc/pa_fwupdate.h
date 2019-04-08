/**
 * @page c_pa_fwupdate Firmware Update Platform Adapter API
 *
 * @ref pa_fwupdate.h "API Reference"
 *
 * <HR>
 *
 * @section pa_fwupdate_toc Table of Contents
 *
 *  - @ref pa_fwupdate_intro
 *  - @ref pa_fwupdate_rational
 *
 *
 * @section pa_fwupdate_intro Introduction
 *
 * These APIs are on the top of the platform-dependent adapter layer. They are independent of the
 * implementation. They guarantee the portability on different kind of platform without any changes
 * for the components developped upon these APIs.
 *
 *
 * @section pa_fwupdate_rational Rational
 * These functions are all blocking functions, so that they return when the modem has answered or
 * when a timeout has occured due to an interrupted communication with the modem.
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/** @file pa_fwupdate.h
 *
 * Legato @ref c_pa_fwupdate include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PA_FWUPDATE_INCLUDE_GUARD
#define LEGATO_PA_FWUPDATE_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Enumerate all SW update states (used by pa_fwupdate_SetState)
 */
//--------------------------------------------------------------------------------------------------
typedef enum pa_fwupdate_state
{
    PA_FWUPDATE_STATE_NORMAL = 1, ///< Normal state
    PA_FWUPDATE_STATE_SYNC,       ///< Synchronization state
    PA_FWUPDATE_STATE_INVALID     ///< Invalid entry;
}
pa_fwupdate_state_t;

//--------------------------------------------------------------------------------------------------
/**
 * Update status
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum pa_fwupdate_UpdateStatus
{
    PA_FWUPDATE_UPDATE_STATUS_OK,              ///< Last update succeeded
    PA_FWUPDATE_UPDATE_STATUS_PARTITION_ERROR, ///< At least, one partition is corrupted
    PA_FWUPDATE_UPDATE_STATUS_DWL_ONGOING,     ///< Downloading in progress
    PA_FWUPDATE_UPDATE_STATUS_DWL_FAILED,      ///< Last downloading failed
    PA_FWUPDATE_UPDATE_STATUS_DWL_TIMEOUT,     ///< Last downloading stopped due to timeout
    PA_FWUPDATE_UPDATE_STATUS_UNKNOWN          ///< Unknown status. It has to be the last one.
}
pa_fwupdate_UpdateStatus_t;

//--------------------------------------------------------------------------------------------------
/**
 * Sub system ID: 3 sub system are defined:
 *     - MODEM = sbl, tz, rpm, modem
 *     - LK    = aboot
 *     - LINUX = boot, system, lefwkro, customer0
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum pa_fwupdate_SubSysId
{
    PA_FWUPDATE_SUBSYSID_NONE = -1,   ///< Sub System ID is not defined or does not exist
    PA_FWUPDATE_SUBSYSID_MODEM = 0,
    PA_FWUPDATE_SUBSYSID_LK,
    PA_FWUPDATE_SUBSYSID_LINUX,
    PA_FWUPDATE_SUBSYSID_MAX,
}
pa_fwupdate_SubSysId_t;

//--------------------------------------------------------------------------------------------------
/**
 * System ID: The dual system platforms have 2 systems: 1 and 2. Some partitions are present in both
 * systems. Some others are shared (common) between the both systems.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum pa_fwupdate_System
{
    PA_FWUPDATE_SYSTEM_NONE = 0,
    PA_FWUPDATE_SYSTEM_1 = 1,
    PA_FWUPDATE_SYSTEM_2 = 2,
}
pa_fwupdate_System_t;


//--------------------------------------------------------------------------------------------------
/**
 * MTD partition table of the allowed partition managed.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char*                  name;          // Partition name
    char*                  systemName[PA_FWUPDATE_SYSTEM_2];
                                          // Real MTD name for system 1 and system 2
    pa_fwupdate_SubSysId_t systemMask;    // System owning the partion (modem, lk or linux)
    bool                   isLogical;     // True if it is a "logical" partition
}
pa_fwupdate_MtdPartition_t;

//--------------------------------------------------------------------------------------------------
/**
 * Internal generic UBI volume name to volume name and volume id suffix translation table
 * The array is terminated by the two fields volumeName and suffixName set to NULL.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char*               volumeName; // Generic UBI volume name
    char*               suffixName; // Expected volume name suffix to fetch the UBI volume
    uint8_t             volumeId;   // Volume ID corresponding to this name
}
pa_fwupdate_UbiVolume_t;

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
LE_SHARED le_result_t pa_fwupdate_Download
(
    int fd   ///< [IN] File descriptor of the image, opened to the start of the image.
);

//--------------------------------------------------------------------------------------------------
/**
 * Return the update package write position.
 *
 * @note This is actually the position within the update package, not the one once the update
 * package is processed (unzipping, extracting, ... ).
 *
 * @return
 *      - LE_OK            on success
 *      - LE_BAD_PARAMETER bad parameter
 *      - LE_FAULT         on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_GetResumePosition
(
    size_t *positionPtr     ///< [OUT] Update package read position
);


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
LE_SHARED le_result_t pa_fwupdate_GetUpdateStatus
(
    pa_fwupdate_UpdateStatus_t *statusPtr, ///< [OUT] Returned update status
    char *statusLabelPtr,                  ///< [OUT] String matching the status
    size_t statusLabelLength               ///< [IN] Maximum length of the status description
);


//--------------------------------------------------------------------------------------------------
/**
 * Read the image file from the modem.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_Read
(
    int* fdPtr         ///< [OUT] File descriptor for the image, ready for reading.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the firmware version string
 *
 * @return
 *      - LE_OK            on success
 *      - LE_NOT_FOUND     if the version string is not available
 *      - LE_OVERFLOW      if version string to big to fit in provided buffer
 *      - LE_BAD_PARAMETER bad parameter
 *      - LE_FAULT for any other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_GetFirmwareVersion
(
    char* versionPtr,        ///< [OUT] Firmware version string
    size_t versionSize       ///< [IN] Size of version buffer
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the bootloader version string
 *
 * @return
 *      - LE_OK            on success
 *      - LE_NOT_FOUND     if the version string is not available
 *      - LE_OVERFLOW      if version string to big to fit in provided buffer
 *      - LE_BAD_PARAMETER bad parameter
 *      - LE_FAULT for any other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_GetBootloaderVersion
(
    char* versionPtr,        ///< [OUT] Firmware version string
    size_t versionSize       ///< [IN] Size of version buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the app bootloader version string
 *
 * @return
 *      - LE_OK            on success
 *      - LE_NOT_FOUND     if the version string is not available
 *      - LE_OVERFLOW      if version string to big to fit in provided buffer
 *      - LE_BAD_PARAMETER bad parameter
 *      - LE_UNSUPPORTED   not supported
 *      - LE_FAULT         for any other errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_GetAppBootloaderVersion
(
    char* versionPtr,        ///< [OUT] App Bootloader version string
    size_t versionSize       ///< [IN] Size of version buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Mark the current system as good.
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    the feature is not supported
 *      - LE_UNAVAILABLE    the flash access is not granted for SW update
 *      - LE_FAULT          on failure
 *      - LE_IO_ERROR       if SYNC fails due to unrecoverable ECC errors
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_MarkGood
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Install the downloaded package.
 *
 * @note On success, a device reboot is initiated without returning any value.
 *
 * @return
 *      - LE_BUSY           download is ongoing
 *      - LE_UNSUPPORTED    the feature is not supported
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_Install
(
    bool isMarkGoodReq      ///< [IN] Indicate if a mark good operation is requested after install
);

//--------------------------------------------------------------------------------------------------
/**
 * Function which indicates if the system is marked good.
 *
 * @note This operation is not supported on single system. On dual system this function indicates if
 *       the Active and Update systems are synchronized.
 *
 *
 * @return
 *      - LE_OK            on success
 *      - LE_UNSUPPORTED   the feature is not supported
 *      - LE_BAD_PARAMETER bad parameter
 *      - LE_FAULT         on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_GetSystemState
(
    bool *isSystemGoodPtr ///< [OUT] Indicates if the system is marked good
);

//--------------------------------------------------------------------------------------------------
/**
 * Function to reset the device. This function does not return any error code.
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_fwupdate_Reset
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Function which indicates if a Sync operation is needed (swap & sync operation)
 *
 * @return
 *      - LE_OK            on success
 *      - LE_UNSUPPORTED   the feature is not supported
 *      - LE_BAD_PARAMETER bad parameter
 *      - LE_FAULT         else
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_DualSysCheckSync
(
    bool *isSyncReqPtr ///< [OUT] Indicates if synchronization is requested
);

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
LE_SHARED le_result_t pa_fwupdate_SetState
(
    pa_fwupdate_state_t state   ///< [IN] state to set
);

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
LE_SHARED le_result_t pa_fwupdate_NvupApply
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the resume context
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    the feature is not supported
 *      - LE_FAULT          on failure
 *      - LE_NO_MEMORY      on memory allocation failure
 *      - LE_IO_ERROR       if SYNC fails due to unrecoverable ECC errors. In this case, the update
 *                          without sync is forced, but the whole system must be updated to ensure
 *                          that the new dual will be workable
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_InitDownload
(
    void
);

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
LE_SHARED le_result_t pa_fwupdate_DisableSyncBeforeUpdate
(
    bool isDisabled  ///< [IN] State of sync check : true (disable) or false (enable)
);

//--------------------------------------------------------------------------------------------------
/**
 * Define a new "system" by setting the 3 groups.  This system will become the current system in
 * in use after the reset performed by this service, if no error are reported.
 *
 * @note On success, a device reboot is initiated without returning any value.
 *
 * @return
 *      - LE_BAD_PARAMETER   If an input parameter is not valid
 *      - LE_FAULT           On failure
 *      - LE_UNSUPPORTED     The feature is not supported
 *
 * @note
 *      The process exits, if an invalid file descriptor (e.g. negative) is given.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_SetSystem
(
    pa_fwupdate_System_t systemArray[PA_FWUPDATE_SUBSYSID_MAX]
                         ///< [IN] System array for "modem/lk/linux" partition groups
);

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
LE_SHARED le_result_t pa_fwupdate_GetSystem
(
    pa_fwupdate_System_t systemArray[PA_FWUPDATE_SUBSYSID_MAX]
                         ///< [OUT] System array for "modem/lk/linux" partition groups
);

//--------------------------------------------------------------------------------------------------
/**
 * Start the bad image indication
 *
 * @return
 *      - LE_OK             on success
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_StartBadImageIndication
(
    le_event_Id_t eventId       ///< the event Id to use to report the bad image
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the bad image indication
 *
 * @return
 *      - LE_OK             on success
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_fwupdate_StopBadImageIndication
(
    void
);

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
LE_SHARED le_result_t pa_fwupdate_RequestUpdate
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Release the flash access after a SW update
 *
 * @return
 *      - LE_OK           on success
 *      - LE_FAULT        on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_CompleteUpdate
(
    void
);

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
LE_SHARED le_result_t pa_fwupdate_GetMtdPartitionTab
(
    pa_fwupdate_MtdPartition_t **mtdPartPtr
);

#endif // LEGATO_PA_FWUPDATE_INCLUDE_GUARD

