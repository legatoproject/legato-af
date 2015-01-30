//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the FaultAction_t class.
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include "string.h"

namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Copy constructor.
 */
//--------------------------------------------------------------------------------------------------
FaultAction_t::FaultAction_t
(
    const FaultAction_t& original
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
void FaultAction_t::operator =
(
    const FaultAction_t& original
)
//--------------------------------------------------------------------------------------------------
{
    if (this != &original)
    {
        Limit_t::operator =(original);
        m_Value = original.m_Value;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator.
 */
//--------------------------------------------------------------------------------------------------
void FaultAction_t::operator =
(
    FaultAction_t&& original
)
//--------------------------------------------------------------------------------------------------
{
    if (this != &original)
    {
        m_Value = std::move(original.m_Value);
        m_IsSet = std::move(original.m_IsSet);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Assignment operator.  Validates and stores the FaultAction_t value.
 *
 * @throws legato::Exception if input is not one of the valid action strings
 */
//--------------------------------------------------------------------------------------------------
void FaultAction_t::operator =
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
        m_Value = action;
        m_IsSet = true;
    }
    else
    {
        throw legato::Exception("Unknown fault action '" + action + "'.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the limit value.
 *
 * @throw   legato::Exception if the limit is not set.
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
    if (!m_IsSet)
    {
        throw legato::Exception("Fetching fault action limit that has not been set.");
    }

    return m_Value;
}



} // namespace legato
