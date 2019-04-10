
//--------------------------------------------------------------------------------------------------
/**
 * @file value.h
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "value.h"



namespace json
{

namespace data
{



//--------------------------------------------------------------------------------------------------
/**
 * Construct a data union.  This is needed to support C++11s unrestricted unions.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Data_t::Data_t
(
)
//--------------------------------------------------------------------------------------------------
:   boolean(false)
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Clean up the union, note that we depend on the enclosing Value_ts destructor to perform the real
 * deletion.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Data_t::~Data_t
(
)
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 * Create a new blank value.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::Null)
//--------------------------------------------------------------------------------------------------
{
    data.number = 0;
}



//--------------------------------------------------------------------------------------------------
/**
 * Create a new value that's a duplicate of an existing object.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
    const Object_t& object
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::Object)
//--------------------------------------------------------------------------------------------------
{
    new(&data.object) Object_t(object);
}



//--------------------------------------------------------------------------------------------------
/**
 * Move the value from an existing object into a new value.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
    Object_t&& object
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::Object)
//--------------------------------------------------------------------------------------------------
{
    new(&data.object) Object_t(object);
}



//--------------------------------------------------------------------------------------------------
/**
 * Copy an array into a new value.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
    const Array_t& array
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::Array)
//--------------------------------------------------------------------------------------------------
{
    new(&data.array) Array_t(array);
}



//--------------------------------------------------------------------------------------------------
/**
 * Move an array of values into a new value object.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
    Array_t&& array
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::Array)
//--------------------------------------------------------------------------------------------------
{
    new(&data.array) Array_t(array);
}



//--------------------------------------------------------------------------------------------------
/**
 * Copy a C string into a new value object.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
    const char* string
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::String)
//--------------------------------------------------------------------------------------------------
{
    new(&data.string) String_t(string);
}



//--------------------------------------------------------------------------------------------------
/**
 * Create a new value object that's a copy of an existing string.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
    const String_t& string
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::String)
//--------------------------------------------------------------------------------------------------
{
    new(&data.string) String_t(string);
}



//--------------------------------------------------------------------------------------------------
/**
 * Move the data from an existing string into a new value object.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
    String_t&& string
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::String)
//--------------------------------------------------------------------------------------------------
{
    new(&data.string) String_t(string);
}



//--------------------------------------------------------------------------------------------------
/**
 * Create a new value from a boolean value.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
    bool boolean
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::Bool)
//--------------------------------------------------------------------------------------------------
{
    data.boolean = boolean;
}



//--------------------------------------------------------------------------------------------------
/**
 * Create a new value from an integer.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
    int number
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::Number)
//--------------------------------------------------------------------------------------------------
{
    data.number = number;
}



//--------------------------------------------------------------------------------------------------
/**
 * Create a new value from a floating point value.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
    double number
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::Number)
//--------------------------------------------------------------------------------------------------
{
    data.number = number;
}



//--------------------------------------------------------------------------------------------------
/**
 * Create a new value from an initializer list, allowing the value to be constructed by structure
 * style initialization.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
    const std::initializer_list<Value_t>& arrayList
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::Array)
//--------------------------------------------------------------------------------------------------
{
    new(&data.array) Array_t(arrayList);
}



//--------------------------------------------------------------------------------------------------
/**
 * Create a new value that's a copy of an existing value.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
    const Value_t& value
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::Null)
//--------------------------------------------------------------------------------------------------
{
    operator =(value);
}



//--------------------------------------------------------------------------------------------------
/**
 * Move the data from an existing value into a newly created value.
 */
//--------------------------------------------------------------------------------------------------
Value_t::Value_t
(
    Value_t&& value
)
//--------------------------------------------------------------------------------------------------
:   type(Type_t::Null)
//--------------------------------------------------------------------------------------------------
{
    operator =(value);
}



//--------------------------------------------------------------------------------------------------
/**
 * Destroy and free the memory from a value.
 */
//--------------------------------------------------------------------------------------------------
Value_t::~Value_t
(
)
//--------------------------------------------------------------------------------------------------
{
    Reset();
}



//--------------------------------------------------------------------------------------------------
/**
 * Overwrite a value from the data of another value.
 *
 * @return A reference to this value.
 */
//--------------------------------------------------------------------------------------------------
Value_t& Value_t::operator =
(
    const Value_t& value
)
//--------------------------------------------------------------------------------------------------
{
    if (&value != this)
    {
        Reset();

        switch (value.type)
        {
            case Type_t::Null:
                break;

            case Type_t::Object:
                new(&data.object) Object_t(value.data.object);
                break;

            case Type_t::Array:
                new(&data.array) Array_t(value.data.array);
                break;

            case Type_t::String:
                new(&data.string) String_t(value.data.string);
                break;

            case Type_t::Bool:
                data.boolean = value.data.boolean;
                break;

            case Type_t::Number:
                data.number = value.data.number;
                break;
        }

        type = value.type;
    }

    return *this;
}



//--------------------------------------------------------------------------------------------------
/**
 * Move the data from another value into this value, overwriting what this value may have previously
 * held.
 *
 * @return A reference to this value.
 */
//--------------------------------------------------------------------------------------------------
Value_t& Value_t::operator =
(
    Value_t&& value
)
//--------------------------------------------------------------------------------------------------
{
    if (&value != this)
    {
        Reset();

        switch (value.type)
        {
            case Type_t::Null:
                break;

            case Type_t::Object:
                new(&data.object) Object_t(std::move(value.data.object));
                break;

            case Type_t::Array:
                new(&data.array) Array_t(std::move(value.data.array));
                break;

            case Type_t::String:
                new(&data.string) String_t(std::move(value.data.string));
                break;

            case Type_t::Bool:
                data.boolean = value.data.boolean;
                break;

            case Type_t::Number:
                data.number = value.data.number;
                break;
        }

        type = value.type;
    }

    return *this;
}



//--------------------------------------------------------------------------------------------------
/**
 * Compare this value to another value.
 *
 * @return A true if the values are equal, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool Value_t::operator ==
(
    const Value_t& other
)
//--------------------------------------------------------------------------------------------------
{
    if (&other == this)
    {
        return true;
    }

    if (type != other.type)
    {
        return false;
    }

    bool result = false;

    switch (type)
    {
        case Type_t::Null:
            result = true;
            break;

        case Type_t::Object:
            result = data.object == other.data.object;
            break;

        case Type_t::Array:
            result = data.array == other.data.array;
            break;

        case Type_t::String:
            result = data.string == other.data.string;
            break;

        case Type_t::Bool:
            result = data.boolean == other.data.boolean;
            break;

        case Type_t::Number:
            result = data.number == other.data.number;
            break;
    }

    return result;
}



//--------------------------------------------------------------------------------------------------
/**
 * Compare this value to another value.
 *
 * @return True if the values are not equal, false if they are equal.
 */
//--------------------------------------------------------------------------------------------------
bool Value_t::operator !=
(
    const Value_t& other
)
//--------------------------------------------------------------------------------------------------
{
    return !(*this == other);
}



//--------------------------------------------------------------------------------------------------
/**
 * Compare this value to another value.
 *
 * @return True if this value is "greater," than the other value, false if it is not.
 */
//--------------------------------------------------------------------------------------------------
bool Value_t::operator >
(
    const Value_t& other
)
//--------------------------------------------------------------------------------------------------
{
    if (&other == this)
    {
        return false;
    }

    if (type != other.type)
    {
        return type > other.type;
    }

    bool result = false;

    switch (type)
    {
        case Type_t::Null:
            result = false;
            break;

        case Type_t::Object:
            result = data.object > other.data.object;
            break;

        case Type_t::Array:
            result = data.array > other.data.array;
            break;

        case Type_t::String:
            result = data.string > other.data.string;
            break;

        case Type_t::Bool:
            result = data.boolean > other.data.boolean;
            break;

        case Type_t::Number:
            result = data.number > other.data.number;
            break;
    }

    return result;
}



//--------------------------------------------------------------------------------------------------
/**
 * Compare this value to another value.
 *
 * @return True if this value is "lesser," than the other value, false if it is not.
 */
//--------------------------------------------------------------------------------------------------
bool Value_t::operator <
(
    const Value_t& other
)
//--------------------------------------------------------------------------------------------------
{
    if (   (*this > other)
        || (*this == other))
    {
        return false;
    }

    return true;
}



//--------------------------------------------------------------------------------------------------
/**
 * Compare this value to another value.
 *
 * @return True if this value is "greater or equal," than the other value, false if it is not.
 */
//--------------------------------------------------------------------------------------------------
bool Value_t::operator >=
(
    const Value_t& other
)
//--------------------------------------------------------------------------------------------------
{
    return !(*this < other);
}



//--------------------------------------------------------------------------------------------------
/**
 * Compare this value to another value.
 *
 * @return True if this value is "lesser than or equal," than the other value, false if it is not.
 */
//--------------------------------------------------------------------------------------------------
bool Value_t::operator <=
(
    const Value_t& other
)
//--------------------------------------------------------------------------------------------------
{
    return !(*this > other);
}



//--------------------------------------------------------------------------------------------------
/**
 * Check if this value is "valid."
 *
 * @return A true if the value is null, false if not.
 */
//--------------------------------------------------------------------------------------------------
Value_t::operator bool
(
)
const
//--------------------------------------------------------------------------------------------------
{
    return type != Type_t::Null;
}



//--------------------------------------------------------------------------------------------------
/**
 * Treat this value as if it is an object and attempt to access a named sub value.
 *
 * @return The requested named value, otherwise an exception is thrown.
 */
//--------------------------------------------------------------------------------------------------
const Value_t& Value_t::operator []
(
    const char* name
)
const
//--------------------------------------------------------------------------------------------------
{
    return operator [](std::string(name));
}



//--------------------------------------------------------------------------------------------------
/**
 * Treat this value as if it is an object and attempt to access a named sub value.
 *
 * @return The requested named value, otherwise an exception is thrown.
 */
//--------------------------------------------------------------------------------------------------
Value_t& Value_t::operator []
(
    const char* name
)
//--------------------------------------------------------------------------------------------------
{
    return operator [](std::string(name));
}



//--------------------------------------------------------------------------------------------------
/**
 * Treat this value as if it is an object and attempt to access a named sub value.
 *
 * @return The requested named value, otherwise an exception is thrown.
 */
//--------------------------------------------------------------------------------------------------
const Value_t& Value_t::operator []
(
    const std::string& name
)
const
//--------------------------------------------------------------------------------------------------
{
    Expect(Type_t::Object);

    const auto& found = data.object.find(name);

    if (found == data.object.end())
    {
        throw std::runtime_error("Field, '" + name + "' not found in object.");
    }

    return found->second;
}



//--------------------------------------------------------------------------------------------------
/**
 * Treat this value as if it is an object and attempt to access a named sub value.
 *
 * @return The requested named value, otherwise an exception is thrown.
 */
//--------------------------------------------------------------------------------------------------
Value_t& Value_t::operator []
(
    const std::string& name
)
//--------------------------------------------------------------------------------------------------
{
    Expect(Type_t::Object);

    const auto& found = data.object.find(name);

    if (found == data.object.end())
    {
        throw std::runtime_error("Field, '" + name + "' not found in object.");
    }

    return found->second;
}



//--------------------------------------------------------------------------------------------------
/**
 * Treat this value as if it is an array and attempt to access an indexed sub value.
 *
 * @return The requested named value, otherwise an exception is thrown.
 */
//--------------------------------------------------------------------------------------------------
const Value_t& Value_t::operator []
(
    size_t index
)
const
//--------------------------------------------------------------------------------------------------
{
    Expect(Type_t::Array);

    if (index > data.array.size())
    {
        throw std::runtime_error("Index out of bounds for array.");
    }

    return data.array[index];
}



//--------------------------------------------------------------------------------------------------
/**
 * Treat this value as if it is an array and attempt to access an indexed sub value.
 *
 * @return The requested named value, otherwise an exception is thrown.
 */
//--------------------------------------------------------------------------------------------------
Value_t& Value_t::operator []
(
    size_t index
)
//--------------------------------------------------------------------------------------------------
{
    Expect(Type_t::Array);

    if (index > data.array.size())
    {
        throw std::runtime_error("Index out of bounds for array.");
    }

    return data.array[index];
}



//--------------------------------------------------------------------------------------------------
/**
 * Read the type id for this value.
 *
 * @return The type id for the currently stored data.
 */
//--------------------------------------------------------------------------------------------------
Type_t Value_t::Type
(
)
const
//--------------------------------------------------------------------------------------------------
{
    return type;
}



//--------------------------------------------------------------------------------------------------
/**
 * Is this value NULL?.
 *
 * @return A true if the value is NULL, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool Value_t::IsNull
(
)
const
//--------------------------------------------------------------------------------------------------
{
    return type == Type_t::Null;
}



//--------------------------------------------------------------------------------------------------
/**
 * Is this value holding an object?
 *
 * @return True if the value is an object, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool Value_t::IsObject
(
)
const
//--------------------------------------------------------------------------------------------------
{
    return type == Type_t::Object;
}



//--------------------------------------------------------------------------------------------------
/**
 * Is this value holding an array?
 *
 * @return True if the value is an array, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool Value_t::IsArray
(
)
const
//--------------------------------------------------------------------------------------------------
{
    return type == Type_t::Array;
}



//--------------------------------------------------------------------------------------------------
/**
 * Is this value holding a string?
 *
 * @return True if the value is a string, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool Value_t::IsString
(
)
const
//--------------------------------------------------------------------------------------------------
{
    return type == Type_t::String;
}



//--------------------------------------------------------------------------------------------------
/**
 * Is this value holding a boolean value?
 *
 * @return True if the value is a boolean value, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool Value_t::IsBoolean
(
)
const
//--------------------------------------------------------------------------------------------------
{
    return type == Type_t::Bool;
}



//--------------------------------------------------------------------------------------------------
/**
 * Is this value holding a numeric value?
 *
 * @return True if the value is a numeric value, false if not.
 */
//--------------------------------------------------------------------------------------------------
bool Value_t::IsNumber
(
)
const
//--------------------------------------------------------------------------------------------------
{
    return type == Type_t::Number;
}



//--------------------------------------------------------------------------------------------------
/**
 * Get a reference to the underlying object value.
 *
 * @return The object this value is holding.
 */
//--------------------------------------------------------------------------------------------------
const Object_t& Value_t::AsObject
(
)
const
//--------------------------------------------------------------------------------------------------
{
    Expect(Type_t::Object);
    return data.object;
}



//--------------------------------------------------------------------------------------------------
/**
 * Get a reference to the underlying object value.
 *
 * @return The object this value is holding.
 */
//--------------------------------------------------------------------------------------------------
Object_t& Value_t::AsObject
(
)
//--------------------------------------------------------------------------------------------------
{
    Expect(Type_t::Object);
    return data.object;
}



//--------------------------------------------------------------------------------------------------
/**
 * Get a reference to the underlying array value.
 *
 * @return The array this value is holding.
 */
//--------------------------------------------------------------------------------------------------
const Array_t& Value_t::AsArray
(
)
const
//--------------------------------------------------------------------------------------------------
{
    Expect(Type_t::Array);
    return data.array;
}



//--------------------------------------------------------------------------------------------------
/**
 * Get a reference to the underlying array value.
 *
 * @return The array this value is holding.
 */
//--------------------------------------------------------------------------------------------------
Array_t& Value_t::AsArray
(
)
//--------------------------------------------------------------------------------------------------
{
    Expect(Type_t::Array);
    return data.array;
}



//--------------------------------------------------------------------------------------------------
/**
 * Get a reference to the underlying string value.
 *
 * @return The string this value is holding.
 */
//--------------------------------------------------------------------------------------------------
const String_t& Value_t::AsString
(
)
const
//--------------------------------------------------------------------------------------------------
{
    Expect(Type_t::String);
    return data.string;
}



//--------------------------------------------------------------------------------------------------
/**
 * Get a reference to the underlying string value.
 *
 * @return The string this value is holding.
 */
//--------------------------------------------------------------------------------------------------
String_t& Value_t::AsString
(
)
//--------------------------------------------------------------------------------------------------
{
    Expect(Type_t::String);
    return data.string;
}



//--------------------------------------------------------------------------------------------------
/**
 * Get a copy of the underlying boolean value.
 *
 * @return The boolean this value is holding.
 */
//--------------------------------------------------------------------------------------------------
bool Value_t::AsBoolean
(
)
const
//--------------------------------------------------------------------------------------------------
{
    Expect(Type_t::Bool);
    return data.boolean;
}



//--------------------------------------------------------------------------------------------------
/**
 * Get a copy of the underlying numeric value.
 *
 * @return The numeric this value is holding.
 */
//--------------------------------------------------------------------------------------------------
double Value_t::AsNumber
(
)
const
//--------------------------------------------------------------------------------------------------
{
    Expect(Type_t::Number);
    return data.number;
}



//--------------------------------------------------------------------------------------------------
/**
 * Convert a type id to a type string.
 *
 * @return A string that represents the type id.
 */
//--------------------------------------------------------------------------------------------------
std::string Value_t::TypeName
(
    Type_t type
)
const
//--------------------------------------------------------------------------------------------------
{
    const char* name = NULL;

    switch (type)
    {
        case Type_t::Null:
            name = "null";
            break;

        case Type_t::Object:
            name = "an object";
            break;

        case Type_t::Array:
            name = "an array";
            break;

        case Type_t::String:
            name = "a string";
            break;

        case Type_t::Bool:
            name = "a bool";
            break;

        case Type_t::Number:
            name = "a number";
            break;
    }

    return name;
}



//--------------------------------------------------------------------------------------------------
/**
 * Internal method to validate the values current type.  Simply pass in the expected type, and if
 * the values current type differs, an exception is thrown.
 */
//--------------------------------------------------------------------------------------------------
void Value_t::Expect
(
    Type_t expectedType
)
const
//--------------------------------------------------------------------------------------------------
{
    if (type != expectedType)
    {
        throw std::runtime_error("Expected value to be " + TypeName(expectedType) +
                                 " but was " + TypeName(type) + " instead.");
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Free the values current data and make the value NULL.
 */
//--------------------------------------------------------------------------------------------------
void Value_t::Reset
(
)
//--------------------------------------------------------------------------------------------------
{
    switch (type)
    {
        case Type_t::Null:
        case Type_t::Bool:
        case Type_t::Number:
            data.number = 0;
            break;

        case Type_t::Object:
            data.object.~Object_t();
            break;

        case Type_t::Array:
            data.array.~Array_t();
            break;

        case Type_t::String:
            data.string.~String_t();
            break;
    }

    type = Type_t::Null;
}



} // namespace data

} // namespace json
