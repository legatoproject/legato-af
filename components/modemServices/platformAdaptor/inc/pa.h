/**
 * @page c_pa Platform Adapter Modem APIs
 *
 * This file contains the prototypes definitions which are common to the Platform Adapter APIs.
 *
 * @section c_pa_apiList List of Platform Adapter Modem APIs
 *
 * The following platform adapter application programming interfaces (APIs) are provided:
 *
 *  - @subpage c_pa_sms
 *  - @subpage c_pa_sim
 *  - @subpage c_pa_mdc
 *  - @subpage c_pa_mrc
 *  - @subpage c_pa_mcc
 *  - @subpage c_pa_info
 *  - @subpage c_pa_ecall
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


/** @file pa.h
 *
 * Legato @ref c_pa include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_PA_INCLUDE_GUARD
#define LEGATO_PA_INCLUDE_GUARD


#include "legato.h"
#include "interfaces.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function initializes the platform adapter layer for modem services.
 *
 * @note This does NOT initialize positioning services access via QMI.
 *
 * @todo Clarify the separation of positioning services and modem services in the PA layer
 *       interface.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void le_pa_Init
(
    void
);


#endif // LEGATO_PA_INCLUDE_GUARD
