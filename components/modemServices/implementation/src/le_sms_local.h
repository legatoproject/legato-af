/** @file le_sms_local.h
 *
 * This file contains the declaration of the SMS Operatrions Initialization.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_SMS_OPS_LOCAL_INCLUDE_GUARD
#define LEGATO_SMS_OPS_LOCAL_INCLUDE_GUARD

#include "legato.h"
#include "le_mdm_defs.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the SMS operations component
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
void le_sms_msg_Init
(
    void
);


#endif // LEGATO_SMS_OPS_LOCAL_INCLUDE_GUARD
