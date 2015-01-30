//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the Non-Negative Integer Limit class.
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"


namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Copy constructor.
 */
//--------------------------------------------------------------------------------------------------
NonNegativeIntLimit_t::NonNegativeIntLimit_t
(
    const NonNegativeIntLimit_t& original
)
//--------------------------------------------------------------------------------------------------
:   Limit_t(original),
    m_Value(original.m_Value)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy assignment operator.
 */
//--------------------------------------------------------------------------------------------------
NonNegativeIntLimit_t& NonNegativeIntLimit_t::operator =
(
    const NonNegativeIntLimit_t& original
)
//--------------------------------------------------------------------------------------------------
{
    if (this != &original)
    {
        Limit_t::operator =(original);
        m_Value = original.m_Value;
    }

    return *this;
}


//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator.
 */
//--------------------------------------------------------------------------------------------------
NonNegativeIntLimit_t& NonNegativeIntLimit_t::operator =
(
    NonNegativeIntLimit_t&& original
)
//--------------------------------------------------------------------------------------------------
{
    if (this != &original)
    {
        m_Value = std::move(original.m_Value);
        m_IsSet = std::move(original.m_IsSet);
    }

    return *this;
}



//--------------------------------------------------------------------------------------------------
/**
 * Validates and stores the limit value.
 *
 *  @throws legato::Exception if value out of range.
 */
//--------------------------------------------------------------------------------------------------
void NonNegativeIntLimit_t::operator =
(
    int value
)
//--------------------------------------------------------------------------------------------------
{
    if (value < 0)
    {
        throw legato::Exception("Limit must not be negative.");
    }
    else
    {
        operator =((size_t)value);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the limit value.
 *
 * @return  The value (guaranteed to be greater than or equal to 0).
 */
//--------------------------------------------------------------------------------------------------
size_t NonNegativeIntLimit_t::Get
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    return m_Value;
}



}  // namespace legato
