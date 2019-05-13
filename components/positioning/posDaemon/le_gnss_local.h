/**
 * @file le_gnss_local.h
 *
 * Local GNSS Definitions
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_GNSS_LOCAL_INCLUDE_GUARD
#define LEGATO_GNSS_LOCAL_INCLUDE_GUARD

#include "legato.h"

/// Typically, we don't expect more than this number of concurrent activation requests.
/// Ideally should be a prime number.
#ifndef LE_CONFIG_POSITIONING_ACTIVATION_MAX
#define LE_CONFIG_POSITIONING_ACTIVATION_MAX 13
#endif // LE_CONFIG_POSITIONING_ACTIVATION_MAX

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the GNSS
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t gnss_Init
(
    void
);

#endif // LEGATO_GNSS_LOCAL_INCLUDE_GUARD
