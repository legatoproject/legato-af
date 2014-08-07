
//--------------------------------------------------------------------------------------------------
/**
 * Definition of the basic legato::Exception class.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
*/
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_EXCEPTION_INCLUDE_GUARD
#define LEGATO_EXCEPTION_INCLUDE_GUARD


namespace legato
{


//----------------------------------------------------------------------------------------------
/**
 *  A basic string exception object used to throw exceptions with an accompanying error message.
 */
//----------------------------------------------------------------------------------------------
class Exception : public std::runtime_error
{
    public:

        // Constructor
        Exception
        (
            const std::string& message  /// Description of the problem that was encountered.
        );
};


class DependencyException : public Exception
{
    public:
        DependencyException(const std::string&);
};


}




#endif
