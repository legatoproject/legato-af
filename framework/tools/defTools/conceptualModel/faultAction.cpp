//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the FaultAction_t class.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"
#include <string.h>

namespace model
{


//--------------------------------------------------------------------------------------------------
/**
 * Assignment operator.  Validates and stores the FaultAction_t value.
 *
 * @throws mk::Exception_t if input is not one of the valid action strings
 *
 * @return  Reference of this object.
 */
//--------------------------------------------------------------------------------------------------
FaultAction_t& FaultAction_t::operator =
(
    const std::string& action
)
//--------------------------------------------------------------------------------------------------
{
    const char* actionStr = action.c_str();

    if (   (strcmp(actionStr, "ignore") == 0)
        || (strcmp(actionStr, "restart") == 0)
        || (strcmp(actionStr, "restartApp") == 0)
        || (strcmp(actionStr, "stopApp") == 0)
        || (strcmp(actionStr, "reboot") == 0)
        || (strcmp(actionStr, "pauseApp") == 0)
       )
    {
        value = action;
        isSet = true;
    }
    else
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Unknown fault action '%s'."), action)
        );
    }

    return *this;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the limit value.
 *
 * @throw   mk::Exception_t if the limit is not set.
 *
 * @return  The value.
 */
//--------------------------------------------------------------------------------------------------
const std::string& FaultAction_t::Get
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    if (!isSet)
    {
        throw mk::Exception_t(LE_I18N("Fetching fault action limit that has not been set."));
    }

    return value;
}



} // namespace model
