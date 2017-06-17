/** @file watchdogAction.h
 *
 * Provides the functions and types relating to watchdog action
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef WATCHDOG_ACTION_H_INCLUDE_GUARD
#define WATCHDOG_ACTION_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Enumerated values for watchdog action (and related error values)
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    WATCHDOG_ACTION_NOT_FOUND,        ///< No action was found for watchdog timeout.
    WATCHDOG_ACTION_ERROR,            ///< There was an error reading the timeout action.
    WATCHDOG_ACTION_HANDLED,          ///< Already been handled. No further action required.
    WATCHDOG_ACTION_IGNORE,           ///< Watchdog timedout but no further action is required.
    WATCHDOG_ACTION_RESTART,          ///< The process should be restarted.
    WATCHDOG_ACTION_STOP,             ///< The process should be terminated.
    WATCHDOG_ACTION_RESTART_APP,      ///< The application should be restarted.
    WATCHDOG_ACTION_STOP_APP,         ///< The application should be terminated.
    WATCHDOG_ACTION_REBOOT            ///< The system should be rebooted.
}
wdog_action_WatchdogAction_t;

//--------------------------------------------------------------------------------------------------
/**
 * Translates a watchdog action string to a watchdogAction_t enum.
 *
 * @return
 *      The watchdog action enum corresponding to the action string given or
 *      WATCHDOG_ACTION_ERROR if the string does not represent a valid action.
 */
//--------------------------------------------------------------------------------------------------
wdog_action_WatchdogAction_t wdog_action_EnumFromString
(
    char* actionString  ///< [IN] the action string to translate
);

#endif // WATCHDOG_ACTION_H_INCLUDE_GUARD
