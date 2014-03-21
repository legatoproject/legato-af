/**
 * @page c_info Modem Information API
 *
 * @ref le_info.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * This file contains prototype definitions for Modem Information APIs.
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file le_info.h
 *
 * Legato @ref c_info include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
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
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_info_GetImei
(
    char*            imeiPtr,  ///< [OUT] The IMEI string.
    size_t           len      ///< [IN] The length of IMEI string.
);




#endif // LEGATO_INFO_INCLUDE_GUARD
