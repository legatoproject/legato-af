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
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file pa.h
 *
 * Legato @ref c_pa include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_PA_INCLUDE_GUARD
#define LEGATO_PA_INCLUDE_GUARD


#include "legato.h"
#include "le_mdm_defs.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function Initialize the platform adapter layer.
 *
 * @return LE_NOT_POSSIBLE The function failed to Initialize the platform adapter layer.
 * @return LE_OK           The function succeeded.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_Init
(
    void
);


#endif // LEGATO_PA_INCLUDE_GUARD
