/*
 * ====================== WARNING ======================
 *
 * THE CONTENTS OF THIS FILE HAVE BEEN AUTO-GENERATED.
 * DO NOT MODIFY IN ANY WAY.
 *
 * ====================== WARNING ======================
 */

/**
 * @page c_info Modem Information API
 *
 * @ref info_interface.h "Click here for the API reference documentation."
 *
 * <HR>
 *
 * This file contains prototype definitions for Modem Information APIs.
 *
 * It's used to retrieve the International Mobile Equipment Identity (IMEI).
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */
/** @file info_interface.h
 *
 * Legato @ref c_info include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */

#ifndef INFO_INTERFACE_H_INCLUDE_GUARD
#define INFO_INTERFACE_H_INCLUDE_GUARD


#include "legato.h"

// User customizable include file
#include "userInclude.h"


//--------------------------------------------------------------------------------------------------
/**
 * Start the service for the client main thread
 */
//--------------------------------------------------------------------------------------------------
void le_info_StartClient
(
    const char* serviceInstanceName
        ///< [IN]
);

//--------------------------------------------------------------------------------------------------
/**
 * Stop the service for the current client thread
 */
//--------------------------------------------------------------------------------------------------
void le_info_StopClient
(
    void
);

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
    char* imei,
        ///< [OUT]
        ///< IMEI string.

    size_t imeiNumElements
        ///< [IN]
);


#endif // INFO_INTERFACE_H_INCLUDE_GUARD

