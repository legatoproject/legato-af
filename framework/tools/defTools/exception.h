
//--------------------------------------------------------------------------------------------------
/**
 * @file exception.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_EXCEPTION_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_EXCEPTION_INCLUDE_GUARD


namespace mk
{


//----------------------------------------------------------------------------------------------
/**
 *  A basic string exception object used to throw exceptions with an accompanying error message.
 */
//----------------------------------------------------------------------------------------------
class Exception_t : public std::runtime_error
{
    public:

        Exception_t(const std::string& message);
};


} // namespace mk


#endif // LEGATO_DEFTOOLS_EXCEPTION_INCLUDE_GUARD
