//--------------------------------------------------------------------------------------------------
/**
 * Base class for various configurable limits that can have integer values that are non-negative
 * (positive or zero).
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef NON_NEGATIVE_INT_LIMIT_H_INCLUDE_GUARD
#define NON_NEGATIVE_INT_LIMIT_H_INCLUDE_GUARD


class NonNegativeIntLimit_t : public Limit_t
{
    public:

        NonNegativeIntLimit_t(void) {}
        explicit NonNegativeIntLimit_t(size_t defaultValue): value(defaultValue) {}

    protected:

        size_t value;

    public:

        virtual NonNegativeIntLimit_t& operator =(int value);
        virtual NonNegativeIntLimit_t& operator =(size_t value) { isSet = true; this->value = value; return *this;}

        virtual size_t Get() const;
};


#endif // NON_NEGATIVE_INT_LIMIT_H_INCLUDE_GUARD
