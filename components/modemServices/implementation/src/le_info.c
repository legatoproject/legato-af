/** @file le_info.c
 *
 * Legato @ref c_info implementation.
 *
 *
 * Copyright (C) Sierra Wireless, Inc. 2012. All rights reserved. Use of this work is subject to license.
 *
 */
#include "legato.h"
#include "le_info.h"
#include "pa_info.h"



//--------------------------------------------------------------------------------------------------
//                                       Public declarations
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to retrieve the International Mobile Equipment Identity (IMEI).
 *
 * @return LE_FAULT       The function failed to retrieve the IMEI.
 * @return LE_OVERFLOW    The IMEI length exceed the maximum length.
 * @return LE_OK          The function succeeded.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_info_GetImei
(
    char*            imeiPtr,  ///< [OUT] The IMEI string.
    size_t           len       ///< [IN] The length of IMEI string.
)
{
    pa_info_Imei_t imei;

    if (imeiPtr == NULL)
    {
        LE_KILL_CLIENT("imeiPtr is NULL !");
        return LE_FAULT;
    }

    if(pa_info_GetIMEI(imei) != LE_OK)
    {
        LE_ERROR("Failed to get the IMEI");
        imeiPtr[0] = '\0';
        return LE_FAULT;
    }
    else
    {
        return (le_utf8_Copy(imeiPtr, imei, len, NULL));
    }
}


