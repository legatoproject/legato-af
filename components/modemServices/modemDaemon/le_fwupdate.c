/**
 * @file le_fwupdate.c
 *
 * Implementation of FW Update API
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 *
 */
#include "legato.h"
#include "interfaces.h"
#include "pa_fwupdate.h"



//==================================================================================================
//                                       Private Functions
//==================================================================================================



//==================================================================================================
//                                       Public API Functions
//==================================================================================================


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
)
{
    return pa_fwupdate_GetBootloaderVersion(version, versionNumElements);
}


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
)
{
    // fd must be positive
    if (fd < 0)
    {
        LE_KILL_CLIENT("'fd' is negative");
        return LE_NOT_POSSIBLE;
    }

    // Pass the fd to the PA layer, which will handle the details.
    return pa_fwupdate_Download(fd);
}


