//--------------------------------------------------------------------------------------------------
/**
 *  These classes handle the validation and output of configuration data for (currently) the
 *  watchdog configurations:
 *      watchdogTimeout
 *      watchdogAction
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef WATCHDOGCONFIG_H_INCLUDE_GUARD
#define WATCHDOGCONFIG_H_INCLUDE_GUARD

#include <ostream>

using namespace std;


//--------------------------------------------------------------------------------------------------
/** Config - base class that implements the IsValid method
 */
//--------------------------------------------------------------------------------------------------
class Config
{
    public:

        Config() { m_IsValid = false; }
        bool IsValid() const { return m_IsValid; }

    protected:

        bool m_IsValid;
};

//--------------------------------------------------------------------------------------------------
/** Validates and outputs watchdogTimeout config
 *
 *  @throws legato::Exception if Set() is given invalid input
 */
//--------------------------------------------------------------------------------------------------
class WatchdogTimeoutConfig: public Config
{
    public:

        void Set(int32_t milliseconds);
        void Set(const std::string &never);
        int32_t Get() const { return m_WatchdogTimeout; }

    private:

        int32_t m_WatchdogTimeout;
};

//--------------------------------------------------------------------------------------------------
/** Validates and outputs watchdogAction config
 *
 *  @throws legato::Exception if Set() is given invalid input
 */
//--------------------------------------------------------------------------------------------------
class WatchdogActionConfig: public Config
{
    public:

        // Could validate the string here too and throw or some such if it wasn't correct
        void Set(std::string action);
        std::string Get() const { return m_WatchdogAction; }

    private:

        std::string m_WatchdogAction;
};

#endif  // WATCHDOGCONFIG_H_INCLUDE_GUARD
