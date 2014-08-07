/** @file le_mrc_local.h
 *
 * This file contains the declaration of the MRC Initialization.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_MRC_LOCAL_INCLUDE_GUARD
#define LEGATO_MRC_LOCAL_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the MRC component.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_mrc_Init
(
    void
);


#endif // LEGATO_MRC_LOCAL_INCLUDE_GUARD
