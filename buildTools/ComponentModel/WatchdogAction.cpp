//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the WatchdogAction_t class.
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"

namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Copy constructor.
 */
//--------------------------------------------------------------------------------------------------
WatchdogAction_t::WatchdogAction_t
(
    const WatchdogAction_t& original
)
//--------------------------------------------------------------------------------------------------
:   Limit_t(original),
    m_Value(original.m_Value)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy assignment operator.
 */
//--------------------------------------------------------------------------------------------------
WatchdogAction_t& WatchdogAction_t::operator =
(
    const WatchdogAction_t& original
)
//--------------------------------------------------------------------------------------------------
{
    if (this != &original)
    {
        Limit_t::operator =(original);
        m_Value = original.m_Value;
    }

    return *this;
}


//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator.
 */
//--------------------------------------------------------------------------------------------------
WatchdogAction_t& WatchdogAction_t::operator =
(
    WatchdogAction_t&& original
)
//--------------------------------------------------------------------------------------------------
{
    if (this != &original)
    {
        m_Value = std::move(original.m_Value);
        m_IsSet = std::move(original.m_IsSet);
    }

    return *this;
}


//--------------------------------------------------------------------------------------------------
/**
 * Assignment operator.  Validates and stores the WatchdogAction_t value.
 *
 * @throws legato::Exception if input is not one of the valid action strings
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
        throw legato::Exception(std::string("Unknown watchdog action '") + action + "'.");
    }
    else
    {
        m_Value = action;
        m_IsSet = true;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the watchdog action.
 *
 * @throw   legato::Exception if the action is not set.
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
    if (!m_IsSet)
    {
        throw legato::Exception("Fetching watchdog action that has not been set.");
    }

    return m_Value;
}


} // namespace legato
