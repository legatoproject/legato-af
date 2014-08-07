/**
 * @file pa_fwupdate.c
 *
 * AT implementation of @ref c_pa_fwupdate API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 *
 */


#include "legato.h"
#include "pa_fwupdate.h"
#include "pa_fwupdate_local.h"



//==================================================================================================
//  PRIVATE FUNCTIONS
//==================================================================================================



//==================================================================================================
//  MODULE/COMPONENT FUNCTIONS
//==================================================================================================


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the fwupdate module
 *
 * @return
 *      - LE_OK if successful.
 *      - LE_FAULT if unsuccessful.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_Init
(
    void
)
{
    return LE_OK;
}


//==================================================================================================
//  PUBLIC API FUNCTIONS
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
le_result_t pa_fwupdate_GetFirmwareVersion
(
    char* versionPtr,        ///< [OUT] Firmware version string
    size_t versionSize       ///< [IN] Size of version buffer
)
{
    return LE_NOT_FOUND;
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
le_result_t pa_fwupdate_GetBootloaderVersion
(
    char* versionPtr,        ///< [OUT] Firmware version string
    size_t versionSize       ///< [IN] Size of version buffer
)
{
    return LE_NOT_FOUND;
}


//--------------------------------------------------------------------------------------------------
/**
 * Download the firmware image file to the modem.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE on failure
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_fwupdate_Download
(
    int fd   ///< [IN] File descriptor of the image, opened to the start of the image.
)
{
    LE_INFO("Called ...");

    return LE_NOT_POSSIBLE;
}

