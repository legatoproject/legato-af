/** @file le_sms_local.h
 *
 * This file contains the declaration of the SMS Operatrions Initialization.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_SMS_OPS_LOCAL_INCLUDE_GUARD
#define LEGATO_SMS_OPS_LOCAL_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the SMS operations component
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_Init
(
    void
);


#endif // LEGATO_SMS_OPS_LOCAL_INCLUDE_GUARD
