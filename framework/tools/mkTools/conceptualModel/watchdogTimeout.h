//--------------------------------------------------------------------------------------------------
/**
 *  Class whose objects can be used to store watchdogTimeout settings.
 *
 *  Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
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

        void operator =(int32_t milliseconds);
        void operator =(const std::string &never);  // For setting "never" as the timeout.

        int32_t Get() const;
};


#endif  // WATCHDOG_TIMEOUT_H_INCLUDE_GUARD
