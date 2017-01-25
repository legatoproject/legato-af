/** @file updateCtrl.h
 *
 * Includes for updateDaemon.c to talk to updateCtrl.c
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef _UPDATECTRL_INCLUDE_GUARD_
#define _UPDATECTRL_INCLUDE_GUARD_
//--------------------------------------------------------------------------------------------------
/**
 * Set a callback to call for when all probation locks are removed.
 */
//--------------------------------------------------------------------------------------------------
void updateCtrl_SetProbationExpiryCallBack
(
    void (*f)(void)
);

//--------------------------------------------------------------------------------------------------
/**
 * Set a callback to call to set the status to "good".
 */
//--------------------------------------------------------------------------------------------------
void updateCtrl_Initialize
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Called from the probation timout handler to determine whether it is OK to mark the system "good".
 *
 * @return
 *      - true    If probation is locked
 *      - false   Probation is not locked
 */
//--------------------------------------------------------------------------------------------------
bool updateCtrl_IsProbationLocked
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Are there any defers in effect
 *
 * @return
 *      - true    If probation is locked
 *      - false   Probation is not locked
 */
//--------------------------------------------------------------------------------------------------
bool updateCtrl_HasDefers
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Set a callback to call to set the status to "good".
 */
//--------------------------------------------------------------------------------------------------
void updateDaemon_MarkGood
(
    void
);

#endif //_UPDATECTRL_INCLUDE_GUARD_
