//--------------------------------------------------------------------------------------------------
/**
 * Base class for various configurable limits that can have integer values that are non-negative
 * (positive or zero).
 *
 * Copyright (C) Sierra Wireless, Inc. 2014.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef NON_NEGATIVE_INT_LIMIT_H_INCLUDE_GUARD
#define NON_NEGATIVE_INT_LIMIT_H_INCLUDE_GUARD

namespace legato
{


class NonNegativeIntLimit_t : public Limit_t
{
    public:

        NonNegativeIntLimit_t(size_t defaultValue): m_Value(defaultValue) {}
        NonNegativeIntLimit_t(const NonNegativeIntLimit_t& original);
        virtual ~NonNegativeIntLimit_t() {}

    protected:

        size_t m_Value;

    public:

        virtual NonNegativeIntLimit_t& operator =(const NonNegativeIntLimit_t& original);
        virtual NonNegativeIntLimit_t& operator =(NonNegativeIntLimit_t&& original);

        virtual void operator =(int value);
        virtual void operator =(size_t value) { m_IsSet = true; m_Value = value; }

        virtual size_t Get() const;
};


}

#endif // NON_NEGATIVE_INT_LIMIT_H_INCLUDE_GUARD
