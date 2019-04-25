//--------------------------------------------------------------------------------------------------
/**
 * Class whose objects can be used to store watchdogTimeout settings.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef WATCHDOG_TIMEOUT_H_INCLUDE_GUARD
#define WATCHDOG_TIMEOUT_H_INCLUDE_GUARD



class WatchdogTimeout_t: public Limit_t
{
    public:

        virtual ~WatchdogTimeout_t() {}

    private:

        int32_t value;

    public:

        WatchdogTimeout_t& operator =(int32_t milliseconds);
        WatchdogTimeout_t& operator =(const std::string &never);  // For setting "never" as the timeout.

        int32_t Get() const;
};


#endif  // WATCHDOG_TIMEOUT_H_INCLUDE_GUARD
