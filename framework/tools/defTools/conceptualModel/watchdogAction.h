//--------------------------------------------------------------------------------------------------
/**
 * Class whose objects can be used to store watchdogAction settings.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef WATCHDOG_ACTION_H_INCLUDE_GUARD
#define WATCHDOG_ACTION_H_INCLUDE_GUARD



class WatchdogAction_t: public Limit_t
{
    public:

        virtual ~WatchdogAction_t() {}

    private:

        std::string value;

    public:

        WatchdogAction_t& operator =(const std::string &action);

        const std::string& Get() const;
};


#endif  // WATCHDOG_ACTION_H_INCLUDE_GUARD
