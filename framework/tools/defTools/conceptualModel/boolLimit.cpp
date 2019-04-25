//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Boolean Limit class.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"

namespace model
{


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the limit value.
 *
 * @throw   mk::Exception_t if the limit is not set.
 *
 * @return  The value.
 */
//--------------------------------------------------------------------------------------------------
bool BoolLimit_t::Get
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    if (!isSet)
    {
        throw mk::Exception_t(LE_I18N("Fetching boolean limit that has not been set."));
    }

    return value;
}



}  // namespace model
