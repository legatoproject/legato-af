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
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

//--------------------------------------------------------------------------------------------------
/** @file pa_fwupdate.h
 *
 * Legato @ref c_pa_fwupdate include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
 * Download the firmware image file to the modem.
 *
 * @return
 *      - LE_OK on success
 *      - LE_FAULT on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_Download
(
    int fd   ///< [IN] File descriptor of the image, opened to the start of the image.
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
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_OVERFLOW if version string to big to fit in provided buffer
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
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_OVERFLOW if version string to big to fit in provided buffer
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
 * Program a synchronization between active and update systems
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    the feature is not supported
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_DualSysSync
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Program a swap between active and update systems
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    the feature is not supported
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_DualSysSwap
(
    bool isSyncReq      ///< [IN] Indicate if a synchronization is requested after the swap
);

//--------------------------------------------------------------------------------------------------
/**
 * Function which indicates if Active and Update systems are synchronized
 *
 * @return
 *      - LE_OK            on success
 *      - LE_UNSUPPORTED   the feature is not supported
 *      - LE_FAULT         else
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_DualSysGetSyncState
(
    bool *isSync ///< Indicates if both systems are synchronized
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
 *      - LE_FAULT         else
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_DualSysCheckSync
(
    bool *isSyncReq ///< Indicates if synchronization is requested
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
 * request to the modem to delete NVUP files
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    the feature is not supported
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_NvupDelete
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Write a NVUP file in UD system
 *
 * @return
 *      - LE_OK             on success
 *      - LE_UNSUPPORTED    the feature is not supported
 *      - LE_FAULT          on failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_fwupdate_NvupWrite
(
    size_t length,                      ///< [IN] data length
    uint8_t* data,                      ///< [IN] input data
    bool isEnd                          ///< [IN] flag to indicate the end of the file
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

#endif // LEGATO_PA_FWUPDATE_INCLUDE_GUARD
