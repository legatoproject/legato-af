//--------------------------------------------------------------------------------------------------
/**
 * Base class for various configurable limits that can have integer values that are positive
 * (not negative and not zero).
 *
 * Copyright (C) Sierra Wireless, Inc. 2014.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef POSITIVE_INT_LIMIT_H_INCLUDE_GUARD
#define POSITIVE_INT_LIMIT_H_INCLUDE_GUARD

namespace legato
{


class PositiveIntLimit_t : public NonNegativeIntLimit_t
{
    public:

        PositiveIntLimit_t(size_t defaultValue);
        PositiveIntLimit_t(const PositiveIntLimit_t& original);
        virtual ~PositiveIntLimit_t() {}

    public:

        virtual PositiveIntLimit_t& operator =(const PositiveIntLimit_t& original);
        virtual PositiveIntLimit_t& operator =(PositiveIntLimit_t&& original);

        virtual void operator =(int value);
        virtual void operator =(size_t value);
};


}

#endif // POSITIVE_INT_LIMIT_H_INCLUDE_GUARD
