/** @file le_sms_local.h
 *
 * This file contains the declaration of the SMS Operatrions Initialization.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_SMS_OPS_LOCAL_INCLUDE_GUARD
#define LEGATO_SMS_OPS_LOCAL_INCLUDE_GUARD

#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the SMS operations component
 *
 * @return LE_FAULT  The function failed.
 * @return LE_OK     The function succeed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_sms_Init
(
    void
);


#endif // LEGATO_SMS_OPS_LOCAL_INCLUDE_GUARD
