//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the Priority class.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2014. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include <limits.h>
#include <string.h>

namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Enumerations of selected priority levels.
 *
 * Real-time priorities are numbered from 1 to 32.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PRIORITY_IDLE = -3,
    PRIORITY_LOW = -2,
    PRIORITY_MEDIUM = -1,
    PRIORITY_HIGH = -0,
}
PriorityLevel_t;



//--------------------------------------------------------------------------------------------------
/**
 * Number translation function.  Converts a string representation of a number into an actual number.
 *
 * @return  The number.
 */
//--------------------------------------------------------------------------------------------------
static int GetNumber
(
    const char* stringPtr
)
//--------------------------------------------------------------------------------------------------
{
    char* endPtr = NULL;

    errno = 0;
    long int number = strtol(stringPtr, &endPtr, 0);
    if ((errno == ERANGE) || (number < INT_MIN) || (number > INT_MAX))
    {
        std::string msg = "Number '";
        msg += stringPtr;
        msg += "' is out of range (magnitude too large).";
        throw legato::Exception(msg);
    }

    if (*endPtr != '\0')
    {
        std::stringstream msg;
        msg << "Unexpected character '" << *endPtr << "' in number '" << stringPtr << "'.";
        throw legato::Exception(msg.str());
    }

    return (int)number;
}




//--------------------------------------------------------------------------------------------------
/**
 * Copy constructor.
 */
//--------------------------------------------------------------------------------------------------
Priority_t::Priority_t
(
    const Priority_t& original
)
//--------------------------------------------------------------------------------------------------
:   Limit_t(original),
    m_Value(original.m_Value),
    m_NumericalValue(original.m_NumericalValue)
//--------------------------------------------------------------------------------------------------
{
}


//--------------------------------------------------------------------------------------------------
/**
 * Copy assignment operator.
 */
//--------------------------------------------------------------------------------------------------
Priority_t& Priority_t::operator =
(
    const Priority_t& original
)
//--------------------------------------------------------------------------------------------------
{
    if (this != &original)
    {
        Limit_t::operator =(original);
        m_Value = original.m_Value;
        m_NumericalValue = original.m_NumericalValue;
    }

    return *this;
}


//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator.
 */
//--------------------------------------------------------------------------------------------------
Priority_t& Priority_t::operator =
(
    Priority_t&& original
)
//--------------------------------------------------------------------------------------------------
{
    if (this != &original)
    {
        m_Value = std::move(original.m_Value);
        m_NumericalValue = std::move(original.m_NumericalValue);
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
void Priority_t::operator =
(
    const std::string& value
)
//--------------------------------------------------------------------------------------------------
{
    const char* priority = value.c_str();

    if ((priority[0] == 'r') && (priority[1] == 't'))
    {
        int number = GetNumber(priority + 2);
        if ((number < 1) || (number > 32))
        {
            throw legato::Exception("Real-time priority level must be between rt1 and rt32,"
                                    " inclusive.");
        }

        m_NumericalValue = number;
    }
    else if (strcmp(priority, "idle") == 0)
    {
        m_NumericalValue = PRIORITY_IDLE;
    }
    else if (strcmp(priority, "low") == 0)
    {
        m_NumericalValue = PRIORITY_LOW;
    }
    else if (strcmp(priority, "medium") == 0)
    {
        m_NumericalValue = PRIORITY_MEDIUM;
    }
    else if (strcmp(priority, "high") == 0)
    {
        m_NumericalValue = PRIORITY_HIGH;
    }
    else
    {
        throw legato::Exception(std::string("Unrecognized priority level '") + priority + "'.");
    }

    m_Value = value;
    m_IsSet = true;
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
const std::string& Priority_t::Get
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    if (!m_IsSet)
    {
        throw legato::Exception("Fetching priority value that has not been set.");
    }

    return m_Value;
}


//--------------------------------------------------------------------------------------------------
/**
 * Comparison operator.
 *
 * @return true iff both priorities are set and this priority is higher than the other priority.
 */
//--------------------------------------------------------------------------------------------------
bool Priority_t::operator >
(
    const Priority_t& other
)
//--------------------------------------------------------------------------------------------------
{
    return (m_IsSet && other.m_IsSet && (m_NumericalValue > other.m_NumericalValue));
}


//--------------------------------------------------------------------------------------------------
/**
 * @return true if the priority is set to a real-time level.
 **/
//--------------------------------------------------------------------------------------------------
bool Priority_t::IsRealTime
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    return (m_IsSet && (m_NumericalValue > 0));
}


}  // namespace legato
