/**
 * @file le_ms.c
 *
 * This file contains the source code of Modem Services Initialization.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */


#include "legato.h"
#include "pa.h"
#include "interfaces.h"
#include "le_mrc_local.h"
#include "le_sim_local.h"
#include "le_sms_local.h"
#include "le_mdc_local.h"
#include "le_mcc_local.h"
#include "le_ips_local.h"
#include "le_ecall_local.h"
#include "le_temp_local.h"
#include "le_antenna_local.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the Modem Services.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_pa_Init();   // Initialize the platform adaptor first.

    le_mrc_Init();
    le_sim_Init();
    le_sms_Init();
    le_mdc_Init();
    le_mcc_Init();
    le_ips_Init();
    le_temp_Init();
    le_antenna_Init();
    le_ecall_Init();
}

