//--------------------------------------------------------------------------------------------------
/**
 * Base class for various configurable limits that can have boolean values.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef BOOL_LIMIT_H_INCLUDE_GUARD
#define BOOL_LIMIT_H_INCLUDE_GUARD


class BoolLimit_t : public Limit_t
{
    public:

        BoolLimit_t(bool defaultValue): value(defaultValue) {}
        virtual ~BoolLimit_t() {}

    protected:

        bool value;

    public:

        bool Get() const;

        void Set(bool b) { value = b; isSet = true; }
};


#endif // BOOL_LIMIT_H_INCLUDE_GUARD
