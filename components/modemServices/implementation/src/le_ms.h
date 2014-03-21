/** @file le_ms.h
 *
 * This file contains the declarations of the Modem Services Initialization.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_MS_INCLUDE_GUARD
#define LEGATO_MS_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the Modem Services.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_ms_Init
(
    void
);

#endif // LEGATO_MS_INCLUDE_GUARD
