//--------------------------------------------------------------------------------------------------
/** @file supervisor/supervisor.h
 *
 * API for working with Legato supervisor.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#ifndef LEGATO_SRC_SUPERVISOR_INCLUDE_GUARD
#define LEGATO_SRC_SUPERVISOR_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Reboot the system.
 */
//--------------------------------------------------------------------------------------------------
void framework_Reboot
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Reports if the Legato framework is stopping.
 *
 * @return
 *     true if the framework is stopping or rebooting
 *     false otherwise
 */
//--------------------------------------------------------------------------------------------------
bool framework_IsStopping
(
    void
);

#endif // LEGATO_SRC_SUPERVISOR_INCLUDE_GUARD
