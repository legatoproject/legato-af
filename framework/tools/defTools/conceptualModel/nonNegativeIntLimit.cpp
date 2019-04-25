//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Non-Negative Integer Limit class.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"


namespace model
{


//--------------------------------------------------------------------------------------------------
/**
 * Validates and stores the limit value.
 *
 *  @throws mk::Exception_t if value out of range.
 *
 *  @return  Reference of this object.
 */
//--------------------------------------------------------------------------------------------------
NonNegativeIntLimit_t& NonNegativeIntLimit_t::operator =
(
    int value
)
//--------------------------------------------------------------------------------------------------
{
    if (value < 0)
    {
        throw mk::Exception_t(LE_I18N("Limit must not be negative."));
    }
    else
    {
        operator =((size_t)value);
    }

    return *this;
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
    return value;
}



}  // namespace model
