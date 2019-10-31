/**
 * @file le_ms.c
 *
 * This file contains the source code of Modem Services Initialization.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#include "legato.h"
#include "interfaces.h"
#include "le_ms_local.h"
#include "le_mrc_local.h"
#include "le_sim_local.h"
#include "le_sms_local.h"
#include "le_mdc_local.h"
#include "le_mcc_local.h"
#include "le_ips_local.h"
#include "le_ecall_local.h"
#include "le_temp_local.h"
#include "le_antenna_local.h"
#include "le_riPin_local.h"
#include "le_lpt_local.h"
#include "sysResets.h"
#include "watchdogChain.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the Modem Services.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_wdogChain_Init(MS_WDOG_COUNT);

    le_mrc_Init();
    le_sim_Init();
    le_sms_Init();
    le_mdc_Init();
    le_mcc_Init();
    le_ips_Init();
    le_temp_Init();
    le_antenna_Init();
    le_riPin_Init();
    le_ecall_Init();
    if (LE_OK != sysResets_Init())
    {
        LE_ERROR("Failed to initialize system resets counter");
    }
    le_lpt_Init();

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_MonitorEventLoop(MS_WDOG_MAIN_LOOP, watchdogInterval);
}
