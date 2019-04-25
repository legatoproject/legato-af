//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the WatchdogAction_t class.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"

namespace model
{


//--------------------------------------------------------------------------------------------------
/**
 * Assignment operator.  Validates and stores the WatchdogAction_t value.
 *
 * @throws mk::Exception_t if input is not one of the valid action strings
 *
 * @return  Reference of this object.
 */
//--------------------------------------------------------------------------------------------------
WatchdogAction_t& WatchdogAction_t::operator =
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
        throw mk::Exception_t(
            mk::format(LE_I18N("Unknown watchdog action '%s'."), action)
        );
    }
    else
    {
        value = action;
        isSet = true;
    }

    return *this;
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
        throw mk::Exception_t(LE_I18N("Fetching watchdog action that has not been set."));
    }

    return value;
}


} // namespace model
