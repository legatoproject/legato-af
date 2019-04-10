
//--------------------------------------------------------------------------------------------------
/**
 * @file jsonOut.cpp
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "value.h"
#include "jsonOut.h"


namespace json
{

namespace data
{



//--------------------------------------------------------------------------------------------------
/**
 * Copy a string and return a version that is safe for writing as JSON.
 *
 * @return The copied and escaped string.
 */
//--------------------------------------------------------------------------------------------------
static std::string EscapeString
(
    const std::string& string  ///< [IN] The string to copy and escape.
)
//--------------------------------------------------------------------------------------------------
{
    std::string newCopy;

    if (!string.empty())
    {
        newCopy.reserve(string.size());

        for (auto nextChar : string)
        {
            switch (nextChar)
            {
                case '\t': newCopy += "\\t";    break;
                case '\r': newCopy += "\\r";    break;
                case '\n': newCopy += "\\n";    break;
                case '\"': newCopy += "\\\"";   break;
                default:   newCopy += nextChar; break;
            }
        }
    }

    return newCopy;
}



//--------------------------------------------------------------------------------------------------
/**
 * Write a json object as properly formated json text to the given output stream.
 *
 * @return A reference to the input stream for further pipelining.
 */
//--------------------------------------------------------------------------------------------------
std::ostream& operator <<
(
    std::ostream& out,      ///< [IN] The output stream to write to.
    const Object_t& object  ///< [IN] The object to write as json.
)
//--------------------------------------------------------------------------------------------------
{
    out << "{ ";

    if (!object.empty())
    {
        auto last = --object.end();

        for (auto iter = object.begin(); iter != object.end(); ++iter)
        {
            out << "\"" << EscapeString(iter->first) << "\" : " << iter->second;

            if (iter != last)
            {
                out << ", ";
            }
            else
            {
                out << " ";
            }
        }
    }

    out << "}";

    return out;
}



//--------------------------------------------------------------------------------------------------
/**
 * Write a json array as properly formated json text to the given output stream.
 *
 * @return A reference to the input stream for further pipelining.
 */
//--------------------------------------------------------------------------------------------------
std::ostream& operator <<
(
    std::ostream& out,    ///< [IN] The output stream to write to.
    const Array_t& array  ///< [IN] The array to write as json.
)
//--------------------------------------------------------------------------------------------------
{
    out << "[ ";

    for (size_t i = 0; i < array.size(); ++i)
    {
        out << array[i];

        if (i < (array.size() - 1))
        {
            out << ", ";
        }
        else
        {
            out << " ";
        }
    }

    out << "]";

    return out;
}



//--------------------------------------------------------------------------------------------------
/**
 * Write a json value as properly formated json text to the given output stream.
 *
 * @return A reference to the input stream for further pipelining.
 */
//--------------------------------------------------------------------------------------------------
std::ostream& operator <<
(
    std::ostream& out,    ///< [IN] The output stream to write to.
    const Value_t& value  ///< [IN] The value to write as json.
)
//--------------------------------------------------------------------------------------------------
{
    switch (value.Type())
    {
        case Type_t::Null:
            out << "null";
            break;

        case Type_t::Object:
            out << value.AsObject();
            break;

        case Type_t::Array:
            out << value.AsArray();
            break;

        case Type_t::String:
            out << "\"" << EscapeString(value.AsString()) << "\"";
            break;

        case Type_t::Bool:
            out << (value.AsBoolean() ? "true" : "false");
            break;

        case Type_t::Number:
            out << value.AsNumber();
            break;
    }

    return out;
}



} // namespace data

} // namespace json
