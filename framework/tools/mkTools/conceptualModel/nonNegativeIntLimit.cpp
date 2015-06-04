//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the Non-Negative Integer Limit class.
 *
 *  Copyright (C) Sierra Wireless, Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


namespace model
{


//--------------------------------------------------------------------------------------------------
/**
 * Validates and stores the limit value.
 *
 *  @throws mk::Exception_t if value out of range.
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
        throw mk::Exception_t("Limit must not be negative.");
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
    return value;
}



}  // namespace model
