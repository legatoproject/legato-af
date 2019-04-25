//--------------------------------------------------------------------------------------------------
/**
 * Base class for various configurable limits that can have integer values that are positive
 * (not negative and not zero).
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef POSITIVE_INT_LIMIT_H_INCLUDE_GUARD
#define POSITIVE_INT_LIMIT_H_INCLUDE_GUARD



struct PositiveIntLimit_t : public NonNegativeIntLimit_t
{
    PositiveIntLimit_t(void) {}
    explicit PositiveIntLimit_t(size_t defaultValue);

    virtual PositiveIntLimit_t& operator =(int value);
    virtual PositiveIntLimit_t& operator =(size_t value);
};


#endif // POSITIVE_INT_LIMIT_H_INCLUDE_GUARD
