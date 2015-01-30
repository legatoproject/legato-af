//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the WatchdogTimeout class.
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
WatchdogTimeout_t::WatchdogTimeout_t
(
    const WatchdogTimeout_t& original
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
WatchdogTimeout_t& WatchdogTimeout_t::operator =
(
    const WatchdogTimeout_t& original
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
WatchdogTimeout_t& WatchdogTimeout_t::operator =
(
    WatchdogTimeout_t&& original
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
 * Assignment operator.  Validates and stores watchdogTimeout value
 *
 * @throws legato::Exception if timeout is not 0 or positive.
 */
//--------------------------------------------------------------------------------------------------
void WatchdogTimeout_t::operator =
(
    int32_t milliseconds
)
//--------------------------------------------------------------------------------------------------
{
    if (milliseconds >= 0)
    {
        m_Value = milliseconds;
        m_IsSet = true;
    }
    else
    {
        throw legato::Exception("WatchdogTimeout must be a positive number of milliseconds.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Assignment operator.  Validates and stores the timeout value for 'never' timeout (-1)
 *
 * @throws legato::Exception if string is not 'never'
 */
//--------------------------------------------------------------------------------------------------
void WatchdogTimeout_t::operator =
(
    const std::string &never
)
//--------------------------------------------------------------------------------------------------
{
    if (never != "never")
    {
        throw legato::Exception("WatchdogTimeout must be a positive number of milliseconds or"
                                " 'never'.");
    }
    else
    {
        // -1 is the numerical value of the define LE_WDOG_TIMEOUT_NEVER used in le_wdog to disable
        // timing out.
        m_Value = -1;
        m_IsSet = true;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the timeout value.
 *
 * @throw   legato::Exception if the timeout is not set.
 *
 * @return  A non-negative timeout in milliseconds or -1 if the watchdog is disabled.
 */
//--------------------------------------------------------------------------------------------------
int32_t WatchdogTimeout_t::Get
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    if (!m_IsSet)
    {
        throw legato::Exception("Fetching watchdog timeout that has not been set.");
    }

    return m_Value;
}





} // namespace legato
