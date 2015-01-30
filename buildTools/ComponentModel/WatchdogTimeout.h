//--------------------------------------------------------------------------------------------------
/**
 *  Class whose objects can be used to store watchdogTimeout settings.
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef WATCHDOG_TIMEOUT_H_INCLUDE_GUARD
#define WATCHDOG_TIMEOUT_H_INCLUDE_GUARD

namespace legato
{


class WatchdogTimeout_t: public Limit_t
{
    public:

        WatchdogTimeout_t() {}
        WatchdogTimeout_t(const WatchdogTimeout_t& original);
        virtual ~WatchdogTimeout_t() {}

    private:

        int32_t m_Value;

    public:

        WatchdogTimeout_t& operator =(const WatchdogTimeout_t& original);
        WatchdogTimeout_t& operator =(WatchdogTimeout_t&& original);

        void operator =(int32_t milliseconds);
        void operator =(const std::string &never);  // For setting "never" as the timeout.

        int32_t Get() const;
};


} // namespace legato

#endif  // WATCHDOG_TIMEOUT_H_INCLUDE_GUARD
