/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * @page c_fwupdate Modem Firmware Update
 *
 * @ref le_fwupdate_interface.h "Click here for the API Reference documentation."
 *
 * <HR>
 *
 * @ref le_fwupdate_image <br>
 * @ref le_fwupdate_version <br>
 *
 * Firmware update allows the various firmware images to be updated from the application processor.
 * This includes the modem bootloader, modem firmware, and linux image, where the linux image 
 * consists of the linux bootloader, linux kernel, and linux rootfs.
 * 
 * Firmware update is useful when the image comes from an alternative source, rather than as an
 * over-the-air update through the AirVantage service.
 *
 * @todo
 *  - support resuming partial download
 *  - support detailed update status
 * 
 *
 * @section le_fwupdate_image Update Firmware Image
 *
 * The firmware update process is started by calling le_fwupdate_Download().  This function takes
 * a file descriptor, rather than a file, to provide flexibility in the source of the image.  In
 * particular, this can be used to stream the image, rather than having to save it on the file
 * system before doing the update.
 *
 * If the image is successfully downloaded to the modem, the modem will reset in order to apply 
 * the update. This will reset both the modem and application processors.  After the application 
 * processor has restarted, the @ref le_fwupdate_version APIs can be used to determine whether 
 * the firmware has been updated to the new version.
 *
 *
 * @section le_fwupdate_version Query Firmware Version
 *
 * le_fwupdate_GetFirmwareVersion() is used to query the current firmware version.  
 * le_fwupdate_GetBootloaderVersion() is used to query the current bootloader version.  
 * In both cases, the version is returned as a human readable string.
 * 
 * The linux kernel version can be queried using standard linux methods, such as the uname
 * command, or the uname() API.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc 2014. Use of this work is subject to license.
 */
/**
 * @file le_fwupdate_interface.h
 *
 * Legato @ref c_fwupdate include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */

#ifndef LE_FWUPDATE_INTERFACE_H_INCLUDE_GUARD
#define LE_FWUPDATE_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_fwupdate_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_fwupdate_StopClient
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of a version string
 */
//--------------------------------------------------------------------------------------------------
#define LE_FWUPDATE_MAX_VERS_LEN 256

//--------------------------------------------------------------------------------------------------
/**
 * Get the firmware version string
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_NOT_POSSIBLE for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_GetFirmwareVersion
(
    char* version,
        ///< [OUT]
        ///< Firmware version string

    size_t versionNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the bootloader version string
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_FOUND if the version string is not available
 *      - LE_NOT_POSSIBLE for any other errors
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_fwupdate_GetBootloaderVersion
(
    char* version,
        ///< [OUT]
        ///< Bootloader version string

    size_t versionNumElements
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Download the firmware image file to the modem.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE on failure
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
);


#endif // LE_FWUPDATE_INTERFACE_H_INCLUDE_GUARD

