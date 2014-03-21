//--------------------------------------------------------------------------------------------------
/**
 * Definitions of functions needed by both the Component Parser and the Application Parser.
 *
 * Not to be shared outside the parser.
 *
 * Copyright (C) 2013 Sierra Wireless Inc., all rights reserved.
 */
//--------------------------------------------------------------------------------------------------

#ifndef PARSER_COMMON_H_INCLUDE_GUARD
#define PARSER_COMMON_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * File permissions flags translation function.  Converts text like "[rwx]" into a number which
 * is a set of bit flags.
 *
 * @return  The corresponding permissions flags (defined in Permissions.h) or'd together.
 **/
//--------------------------------------------------------------------------------------------------
int yy_GetPermissionFlags
(
    const char* stringPtr   // Ptr to the string representation of the permissions.
);


//--------------------------------------------------------------------------------------------------
/**
 * Number translation function.  Converts a string representation of a number into an actual number.
 *
 * @return  The number.
 */
//--------------------------------------------------------------------------------------------------
int yy_GetNumber
(
    const char* stringPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Strips any quotation marks out of a given string.
 *
 * @return  The new string.
 */
//--------------------------------------------------------------------------------------------------
std::string yy_StripQuotes
(
    const std::string& string
);


#endif // PARSER_COMMON_H_INCLUDE_GUARD
