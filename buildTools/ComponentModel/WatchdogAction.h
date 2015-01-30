//--------------------------------------------------------------------------------------------------
/**
 *  Class whose objects can be used to store watchdogAction settings.
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef WATCHDOG_ACTION_H_INCLUDE_GUARD
#define WATCHDOG_ACTION_H_INCLUDE_GUARD

namespace legato
{


class WatchdogAction_t: public Limit_t
{
    public:

        WatchdogAction_t() {}
        WatchdogAction_t(const WatchdogAction_t& original);
        virtual ~WatchdogAction_t() {}

    private:

        std::string m_Value;

    public:

        WatchdogAction_t& operator =(const WatchdogAction_t& original);
        WatchdogAction_t& operator =(WatchdogAction_t&& original);

        void operator =(const std::string &action);

        const std::string& Get() const;
};


} // namespace legato

#endif  // WATCHDOG_ACTION_H_INCLUDE_GUARD
