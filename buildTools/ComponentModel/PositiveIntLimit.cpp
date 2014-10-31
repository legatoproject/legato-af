//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the Positive Integer Limit class.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"


namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Constructor.
 */
//--------------------------------------------------------------------------------------------------
PositiveIntLimit_t::PositiveIntLimit_t
(
    size_t defaultValue
)
//--------------------------------------------------------------------------------------------------
:   NonNegativeIntLimit_t(defaultValue)
//--------------------------------------------------------------------------------------------------
{
    if (defaultValue == 0)
    {
        throw legato::Exception("Default value must be positive. Set to zero.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy constructor.
 */
//--------------------------------------------------------------------------------------------------
PositiveIntLimit_t::PositiveIntLimit_t
(
    const PositiveIntLimit_t& original
)
//--------------------------------------------------------------------------------------------------
:   NonNegativeIntLimit_t(original)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy assignment operator.
 */
//--------------------------------------------------------------------------------------------------
PositiveIntLimit_t& PositiveIntLimit_t::operator =
(
    const PositiveIntLimit_t& original
)
//--------------------------------------------------------------------------------------------------
{
    NonNegativeIntLimit_t::operator =(original);

    return *this;
}


//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator.
 */
//--------------------------------------------------------------------------------------------------
PositiveIntLimit_t& PositiveIntLimit_t::operator =
(
    PositiveIntLimit_t&& original
)
//--------------------------------------------------------------------------------------------------
{
    NonNegativeIntLimit_t::operator =(original);

    return *this;
}



//--------------------------------------------------------------------------------------------------
/**
 * Validates and stores the limit value.
 *
 *  @throws legato::Exception if value out of range.
 */
//--------------------------------------------------------------------------------------------------
void PositiveIntLimit_t::operator =
(
    int value
)
//--------------------------------------------------------------------------------------------------
{
    if (value <= 0)
    {
        throw legato::Exception("Limit must be greater than zero.");
    }
    else
    {
        NonNegativeIntLimit_t::operator =((size_t)value);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Validates and stores the limit value.
 *
 *  @throws legato::Exception if value out of range.
 */
//--------------------------------------------------------------------------------------------------
void PositiveIntLimit_t::operator =
(
    size_t value
)
//--------------------------------------------------------------------------------------------------
{
    if (value > 0)
    {
        NonNegativeIntLimit_t::operator =(value);
    }
    else
    {
        throw legato::Exception("Limit must be greater than zero.");
    }
}


}  // namespace legato
