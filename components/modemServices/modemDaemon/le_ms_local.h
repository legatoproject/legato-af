/**
 * @file le_ms_local.h
 *
 * Modem service definitions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_MS_LOCAL_INCLUDE_GUARD
#define LEGATO_MS_LOCAL_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 8

//--------------------------------------------------------------------------------------------------
/**
 * By default eCall is disabled
 */
//--------------------------------------------------------------------------------------------------
#ifndef INCLUDE_ECALL
#define INCLUDE_ECALL 0
#endif

//--------------------------------------------------------------------------------------------------
/**
 *  Thread name maintained by watchdog in modem service.
 */
//--------------------------------------------------------------------------------------------------
#define WDOG_THREAD_NAME_MDC_COMMAND_EVENT "CommandEventThread"
#define WDOG_THREAD_NAME_MRC_COMMAND_PROCESS "ProcessMrcCommandHandler"
#define WDOG_THREAD_NAME_SMS_COMMAND_SENDING "ProcessSmsSendingCommandHandler"

//--------------------------------------------------------------------------------------------------
/**
 * Enum for all watchdogs used by modem services
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    MS_WDOG_MAIN_LOOP,
    MS_WDOG_MDC_LOOP,
    MS_WDOG_SMS_LOOP,
    MS_WDOG_MRC_LOOP,
    MS_WDOG_RIPIN_LOOP,
#if INCLUDE_ECALL
    MS_WDOG_ECALL_LOOP,
#endif
    MS_WDOG_COUNT
}
MS_Watchdog_t;

#endif /* LEGATO_MS_LOCAL_INCLUDE_GUARD */
