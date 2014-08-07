/** @file le_info.h
 *
 * Legato @ref c_info include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_INFO_INCLUDE_GUARD
#define LEGATO_INFO_INCLUDE_GUARD

#include "legato.h"
#include "le_mdm_defs.h"



//--------------------------------------------------------------------------------------------------
// Symbol and Enum definitions.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Maximum IMEI length (15 digits)
 *
 */
//--------------------------------------------------------------------------------------------------
#define LE_INFO_IMEI_MAX_LEN     (15+1)


//--------------------------------------------------------------------------------------------------
// APIs.
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Retrieve the International Mobile Equipment Identity (IMEI).
 *
 * @return LE_FAULT       Function failed to retrieve the IMEI.
 * @return LE_OVERFLOW     IMEI length exceed the maximum length.
 * @return LE_OK          Function succeeded.
 *
 * @note If the caller passes a bad pointer into this function, it's a fatal error; the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_info_GetImei
(
    char*            imeiPtr,  ///< [OUT] IMEI string.
    size_t           len      ///< [IN] Length of IMEI string.
);




#endif // LEGATO_INFO_INCLUDE_GUARD
