//--------------------------------------------------------------------------------------------------
/**
 * Base class for various configurable limits that can have boolean values.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef BOOL_LIMIT_H_INCLUDE_GUARD
#define BOOL_LIMIT_H_INCLUDE_GUARD

namespace legato
{


class BoolLimit_t : public Limit_t
{
    public:

        BoolLimit_t(bool defaultValue): m_Value(defaultValue) {}
        virtual ~BoolLimit_t() {}

    protected:

        bool m_Value;

    public:

        bool Get() const;

        void Set(bool value) { m_Value = value; m_IsSet = true; }
};


}

#endif // BOOL_LIMIT_H_INCLUDE_GUARD
