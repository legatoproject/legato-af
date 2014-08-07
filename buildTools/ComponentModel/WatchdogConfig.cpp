//--------------------------------------------------------------------------------------------------
/**
 *  These classes handle the validation and output of configuration data for (currently) the
 *  watchdog configurations:
 *      watchdogTimeout
 *      watchdogAction
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include "WatchdogConfig.h"


//--------------------------------------------------------------------------------------------------
/** Validates and stores watchdogTimeout value
 *
 *  @throws legato::Exception if timeout is not 0 or positive.
 */
//--------------------------------------------------------------------------------------------------
void WatchdogTimeoutConfig::Set(int32_t milliseconds)
{
    if (milliseconds >= 0)
    {
        m_WatchdogTimeout = milliseconds;
        m_IsValid = true;
    }
    else
    {
        throw legato::Exception("WatchdogTimeout must be a positive number of milliseconds.");
    }
}

//--------------------------------------------------------------------------------------------------
/** Validates and stores the timeout value for 'never' timeout (-1)
 *
 *  @throws legato::Exception if string is not 'never'
 */
//--------------------------------------------------------------------------------------------------
void WatchdogTimeoutConfig::Set(const std::string &never)
{
    if (never != "never")
    {
        throw legato::Exception("WatchdogTimeout must be a positive number of milliseconds or 'never'.");
    }
    else
    {
        // -1 is the numerical value of the define LE_WDOG_TIMEOUT_NEVER used in le_wdog to disable
        // timing out.
        m_WatchdogTimeout = -1;
        m_IsValid = true;
    }
}

//--------------------------------------------------------------------------------------------------
/** Validates and stores the watchdogAction value
 *
 *  @throws legato::Exception if input is not one of the valid action strings
 */
//--------------------------------------------------------------------------------------------------
void WatchdogActionConfig::Set(std::string action)
{
    if (   (action != "ignore")
        && (action != "restart")
        && (action != "stop")
        && (action != "restartApp")
        && (action != "stopApp")
        && (action != "reboot")
        && (action != "pauseApp")
       )
    {
        throw legato::Exception(std::string("Unknown watchdog action '") + action + "'.");
    }
    else
    {
        m_WatchdogAction = action;
        m_IsValid = true;
    }
}
