/**
 * @file le_ms.c
 *
 * This file contains the source code of Modem Services Initialization.
 *
 * Copyright (C) Sierra Wireless Inc.
 */


#include "pa.h"
#include "legato.h"
#include "interfaces.h"

#include "le_ms_local.h"
#include "le_mrc_local.h"
#include "le_sim_local.h"
#include "le_sms_local.h"
#include "le_mdc_local.h"
#include "le_temp_local.h"
#include "le_riPin_local.h"
#include "le_ips_local.h"
#include "sysResets.h"

#if !MK_CONFIG_MODEMSERVICE_NO_LPT
#include "le_lpt_local.h"
#endif

#if !MK_CONFIG_MODEMSERVICE_SIMPLE
#include "le_mcc_local.h"
#include "le_ecall_local.h"
#include "le_antenna_local.h"
#endif

#include "watchdogChain.h"

//--------------------------------------------------------------------------------------------------
/**
 *  Component initialization called only once.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT_ONCE
{
    le_mrc_InitPools();
    le_mdc_InitPools();
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the Modem Services.
 *
 * @note If the initialization failed, it is a fatal error, the function will not return.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    uint32_t immediateWatchdogs = (1 << MS_WDOG_MAIN_LOOP);
    // MS_WDOG_MDC_LOOP is started on demand.
    immediateWatchdogs |= (1 << MS_WDOG_SMS_LOOP);
    // MS_WDOG_MRC_LOOP is started on demand.
    immediateWatchdogs |= (1 << MS_WDOG_RIPIN_LOOP);
#if LE_CONFIG_ENABLE_ECALL
    immediateWatchdogs |= (1 << MS_WDOG_ECALL_LOOP);
#endif
    le_wdogChain_InitSome(MS_WDOG_COUNT, immediateWatchdogs);

    pa_Init();
    le_mrc_Init();
    le_sim_Init();
    le_sms_Init();
    le_mdc_Init();
#if !MK_CONFIG_MODEMSERVICE_NO_LPT
    le_lpt_Init();
#endif
#if !MK_CONFIG_MODEMSERVICE_SIMPLE
    le_mcc_Init();
#endif
    le_ips_Init();
    le_temp_Init();
    le_riPin_Init();

#if !MK_CONFIG_MODEMSERVICE_SIMPLE
    le_antenna_Init();
    le_ecall_Init();

    if (LE_OK != sysResets_Init())
    {
        LE_ERROR("Failed to initialize system resets counter");
    }
#endif

    LE_DEBUG("Modem Service Init done");

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_MonitorEventLoop(MS_WDOG_MAIN_LOOP, watchdogInterval);
}
