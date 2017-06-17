/** @file watchdogAction.c
 *
 * Contains the implementation of the config node string for looking up watchdog action and
 * a function to convert from the action string to the enumerated value for that action
 * (or to an error value).
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "watchdogAction.h"
#include "le_basics.h"
#include <string.h>

//--------------------------------------------------------------------------------------------------
/**
 * A structure that associated a watchdog action string with the corresponding enum
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char* actionString;
    wdog_action_WatchdogAction_t action;
}
NameToAction_t;

//--------------------------------------------------------------------------------------------------
/**
 * Table associating watchdog action strings to enums
 */
//--------------------------------------------------------------------------------------------------
static NameToAction_t NameToActionTable[] =
{
     // WATCHDOG_ACTION_NONE - not searched for. This is the default.
    {"",            WATCHDOG_ACTION_NOT_FOUND},
    {"ignore",      WATCHDOG_ACTION_IGNORE},
    {"restart",     WATCHDOG_ACTION_RESTART},
    {"stop",        WATCHDOG_ACTION_STOP},
    {"restartApp",  WATCHDOG_ACTION_RESTART_APP},
    {"stopApp",     WATCHDOG_ACTION_STOP_APP},
    {"reboot",      WATCHDOG_ACTION_REBOOT}
};

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
)
{
    int c;
    wdog_action_WatchdogAction_t result = WATCHDOG_ACTION_ERROR;
    if (actionString)
    {
        for (c = 0; c < NUM_ARRAY_MEMBERS(NameToActionTable); ++c)
        {
            if (strcmp(NameToActionTable[c].actionString, actionString) == 0)
            {
                result = NameToActionTable[c].action;
                break;
            }
        }
    }
    return result;
}
