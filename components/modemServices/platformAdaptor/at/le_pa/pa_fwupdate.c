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
