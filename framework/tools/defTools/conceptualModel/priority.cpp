//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the Priority class.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"
#include <limits.h>
#include <string.h>

namespace model
{


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
        throw mk::Exception_t(
            mk::format(LE_I18N("Number '%s' is out of range (magnitude too large)."),
                       stringPtr)
        );
    }

    if (*endPtr != '\0')
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Unexpected character '%c' in number '%s'"), *endPtr, stringPtr)
        );
    }

    return (int)number;
}



//--------------------------------------------------------------------------------------------------
/**
 * Validates and stores the limit value.
 *
 *  @throws mk::Exception_t if value out of range.
 *
 *  @return  Reference of this object.
 */
//--------------------------------------------------------------------------------------------------
Priority_t& Priority_t::operator =
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
            throw mk::Exception_t(LE_I18N("Real-time priority level must be between rt1 and rt32,"
                                          " inclusive."));
        }

        numericalValue = number;
    }
    else if (strcmp(priority, "idle") == 0)
    {
        numericalValue = IDLE;
    }
    else if (strcmp(priority, "low") == 0)
    {
        numericalValue = LOW;
    }
    else if (strcmp(priority, "medium") == 0)
    {
        numericalValue = MEDIUM;
    }
    else if (strcmp(priority, "high") == 0)
    {
        numericalValue = HIGH;
    }
    else
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Unrecognized priority level '%s'."), priority)
        );
    }

    this->value = value;
    isSet = true;
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
const std::string& Priority_t::Get
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    if (!isSet)
    {
        throw mk::Exception_t(LE_I18N("Fetching priority value that has not been set."));
    }

    return value;
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the priority numerical value.
 *
 * @throw   mk::Exception_t if the limit is not set.
 *
 * @return  The value.
 */
//--------------------------------------------------------------------------------------------------
int Priority_t::GetNumericalValue
(
    void
) const
{
    if (!isSet)
    {
        throw mk::Exception_t(LE_I18N("Fetching priority value that has not been set."));
    }

    return numericalValue;
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
    return (isSet && other.isSet && (numericalValue > other.numericalValue));
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
    return (isSet && (numericalValue > 0));
}


}  // namespace model
