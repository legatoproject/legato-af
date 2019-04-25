//--------------------------------------------------------------------------------------------------
/**
 * Class whose objects can be used to store faultAction settings.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef FAULT_ACTION_H_INCLUDE_GUARD
#define FAULT_ACTION_H_INCLUDE_GUARD



class FaultAction_t : public Limit_t
{
    public:

        virtual ~FaultAction_t() {}

    private:

        std::string value;

    public:

        FaultAction_t& operator =(const std::string &action);

        const std::string& Get() const;
};


#endif  // FAULT_ACTION_H_INCLUDE_GUARD
