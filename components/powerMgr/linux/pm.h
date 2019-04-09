/**
 * @file pm.h
 *
 * Local power manager functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef COMPONENTS_POWERMGR_PM_H_
#define COMPONENTS_POWERMGR_PM_H_


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether any process is holding a wakelock.
 *
 * @return
 *     - true if any process is holding a wakelock
 *     - false otherwise.
 *
 */
//--------------------------------------------------------------------------------------------------
bool pm_CheckWakeLock
(
    void
);

#endif /* COMPONENTS_POWERMGR_PM_H_ */
