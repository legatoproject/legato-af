/**
 * @file le_gnss_local.h
 *
 * Local GNSS Definitions
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_GNSS_LOCAL_INCLUDE_GUARD
#define LEGATO_GNSS_LOCAL_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the GNSS
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void gnss_Init
(
    void
);


#endif // LEGATO_GNSS_LOCAL_INCLUDE_GUARD

