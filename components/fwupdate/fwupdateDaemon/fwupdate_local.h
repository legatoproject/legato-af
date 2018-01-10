/**
 * @file fwupdate_local.h
 *
 * Local fwupdate Definitions
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_ECALL_LOCAL_INCLUDE_GUARD
#define LEGATO_ECALL_LOCAL_INCLUDE_GUARD

#include "legato.h"


// Max watchdog timeout. This value should be matched with fwupdateService.adef definition.
#define FWUPDATE_MAX_WDOG_TIMEOUT       960

// Watchdog kick interval
#define FWUPDATE_WDOG_KICK_INTERVAL     FWUPDATE_MAX_WDOG_TIMEOUT/2

// Watchdog timer for fwupdate
#define FWUPDATE_WDOG_TIMER    0


#endif // LEGATO_FWUPDATE_LOCAL_INCLUDE_GUARD
