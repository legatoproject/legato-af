//--------------------------------------------------------------------------------------------------
/**
 *  Class whose objects can be used to store faultAction settings.
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef FAULT_ACTION_H_INCLUDE_GUARD
#define FAULT_ACTION_H_INCLUDE_GUARD

namespace legato
{


class FaultAction_t : public Limit_t
{
    public:

        FaultAction_t() {}
        FaultAction_t(const FaultAction_t& original);
        virtual ~FaultAction_t() {}

    private:

        std::string m_Value;

    public:

        void operator =(const FaultAction_t& original);
        void operator =(FaultAction_t&& original);

        void operator =(const std::string &action);

        const std::string& Get() const;
};


} // namespace legato

#endif  // FAULT_ACTION_H_INCLUDE_GUARD
