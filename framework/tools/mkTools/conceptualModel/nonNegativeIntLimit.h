//--------------------------------------------------------------------------------------------------
/**
 * Base class for various configurable limits that can have integer values that are non-negative
 * (positive or zero).
 *
 * Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef NON_NEGATIVE_INT_LIMIT_H_INCLUDE_GUARD
#define NON_NEGATIVE_INT_LIMIT_H_INCLUDE_GUARD


class NonNegativeIntLimit_t : public Limit_t
{
    public:

        NonNegativeIntLimit_t(size_t defaultValue): value(defaultValue) {}

    protected:

        size_t value;

    public:

        virtual void operator =(int value);
        virtual void operator =(size_t value) { isSet = true; this->value = value; }

        virtual size_t Get() const;
};


#endif // NON_NEGATIVE_INT_LIMIT_H_INCLUDE_GUARD
