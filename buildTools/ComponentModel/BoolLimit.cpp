//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the Boolean Limit class.
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"


namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the limit value.
 *
 * @throw   legato::Exception if the limit is not set.
 *
 * @return  The value.
 */
//--------------------------------------------------------------------------------------------------
bool BoolLimit_t::Get
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (!m_IsSet)
    {
        throw legato::Exception("Fetching boolean limit that has not been set.");
    }

    return m_Value;
}



}  // namespace legato
