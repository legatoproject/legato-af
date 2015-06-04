//--------------------------------------------------------------------------------------------------
/**
 * Implementation of Process class methods.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include <limit.h>

namespace model
{

//--------------------------------------------------------------------------------------------------
/**
 * Check the validity of the process name.
 *
 * @throw mk::Exception_t if name is not valid.
 **/
//--------------------------------------------------------------------------------------------------
static void CheckName
(
    const std::string& name
)
//--------------------------------------------------------------------------------------------------
{
    if (name.length() > LIMIT_MAX_PROCESS_NAME_LEN)
    {
        throw mk::Exception_t("Process name '" + name + "' is too long."
                                   "  Must be a maximum of " +
                                   std::to_string(LIMIT_MAX_PROCESS_NAME_LEN) + " bytes.");
    }

    if (name.empty())
    {
        throw mk::Exception_t("Empty process name.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the process name.
 **/
//--------------------------------------------------------------------------------------------------
void Process_t::SetName
(
    const std::string& name
)
//--------------------------------------------------------------------------------------------------
{
    CheckName(name);

    this->name = name;
}


}
