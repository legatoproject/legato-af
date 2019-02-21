//--------------------------------------------------------------------------------------------------
/**
 * @file format.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_FORMAT_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_FORMAT_INCLUDE_GUARD

namespace mk
{

namespace detail
{

//--------------------------------------------------------------------------------------------------
/**
 * Type an argument needs to be converted to so it can be safely passed to printf.
 *
 * Pass most types straight through, but convert std::string to char *
 */
//--------------------------------------------------------------------------------------------------
template<typename T>
struct printf_type
{
    typedef const T& type;
};

template<>
struct printf_type<std::string>
{
    typedef const char *type;
};

//--------------------------------------------------------------------------------------------------
/**
 * Convert argument to the correct type.
 *
 * This is partially specialized so std::string will automatically be converted to const char *.
 * add any other partial template specializations as required here -- final type needs to be
 * a type understood by printf.
 */
//--------------------------------------------------------------------------------------------------
template<typename T, typename R=typename printf_type<T>::type>
inline R to_printf_type
(
    const T &t
)
//--------------------------------------------------------------------------------------------------
{
    return t;
}

//--------------------------------------------------------------------------------------------------
/**
 * Explicit specialization of to_printf_type for std::string
 */
//--------------------------------------------------------------------------------------------------
template<>
inline const char * to_printf_type<std::string>
(
    const std::string &t
)
//--------------------------------------------------------------------------------------------------
{
    return t.c_str();
}

static const int bufferSize = 128;

}


//--------------------------------------------------------------------------------------------------
/**
 * Format a series of arguments according to a printf-style format string.
 *
 * This will convert any arguments that need to be converted (e.g. std::string to const char *),
 * then call snprintf() to format the string.  Finally it will construct a std::string object with
 * the result and return it.
 */
//--------------------------------------------------------------------------------------------------
template<typename... T>
inline std::string format
(
    const char *format, const T&... args
)
//--------------------------------------------------------------------------------------------------
{
    char buffer[detail::bufferSize];

    int stringSize = snprintf(buffer, detail::bufferSize, format, detail::to_printf_type(args)...);

    if (stringSize < detail::bufferSize)
    {
        return std::string(buffer);
    }
    else
    {
        std::unique_ptr<char[]> allocBuffer(new char[stringSize + 1]);

        snprintf(allocBuffer.get(), stringSize + 1, format, detail::to_printf_type(args)...);

        return std::string(allocBuffer.get());
    }
}

} // namespace mk

#endif // LEGATO_DEFTOOLS_FORMAT_INCLUDE_GUARD
