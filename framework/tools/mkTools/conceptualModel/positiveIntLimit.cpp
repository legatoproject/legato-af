//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the Positive Integer Limit class.
 *
 *  Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


namespace model
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
        throw mk::Exception_t("Default value must be positive. Set to zero.");
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Validates and stores the limit value.
 *
 *  @throws mk::Exception_t if value out of range.
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
        throw mk::Exception_t("Limit must be greater than zero.");
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
 *  @throws mk::Exception_t if value out of range.
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
        throw mk::Exception_t("Limit must be greater than zero.");
    }
}


}  // namespace model
