/**
 * @file pa_info.c
 *
 * AT implementation of @ref c_pa_info API.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


#include "legato.h"
#include "pa.h"
#include "pa_info.h"







//--------------------------------------------------------------------------------------------------
/**
 * This function get the International Mobile Equipment Identity (IMEI).
 *
 * @return  LE_NOT_POSSIBLE  The function failed to get the value.
 * @return  LE_TIMEOUT       No response was received from the Modem.
 * @return  LE_OK            The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_info_GetIMEI
(
    pa_info_Imei_t imei   ///< [OUT] IMEI value
)
{
    //To do

    return LE_OK;
}


