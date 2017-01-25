//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the WatchdogAction_t class.
 *
 *  Copyright (C) Sierra Wireless, Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace model
{


//--------------------------------------------------------------------------------------------------
/**
 * Assignment operator.  Validates and stores the WatchdogAction_t value.
 *
 * @throws mk::Exception_t if input is not one of the valid action strings
 */
//--------------------------------------------------------------------------------------------------
void WatchdogAction_t::operator =
(
    const std::string& action
)
//--------------------------------------------------------------------------------------------------
{
    if (   (action != "ignore")
        && (action != "restart")
        && (action != "stop")
        && (action != "restartApp")
        && (action != "stopApp")
        && (action != "reboot")
       )
    {
        throw mk::Exception_t("Unknown watchdog action '" + action + "'.");
    }
    else
    {
        value = action;
        isSet = true;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the watchdog action.
 *
 * @throw   mk::Exception_t if the action is not set.
 *
 * @return  The action string as it should appear in the configuration tree.
 */
//--------------------------------------------------------------------------------------------------
const std::string& WatchdogAction_t::Get
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    if (!isSet)
    {
        throw mk::Exception_t("Fetching watchdog action that has not been set.");
    }

    return value;
}


} // namespace model
