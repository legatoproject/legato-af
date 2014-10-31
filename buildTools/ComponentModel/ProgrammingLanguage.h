//--------------------------------------------------------------------------------------------------
/**
 * Helper functions and definitions related to programming languages.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013, 2014.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef PROGRAMMING_LANGUAGE_H_INCLUDE_GUARD
#define PROGRAMMING_LANGUAGE_H_INCLUDE_GUARD

namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of programming languages supported by the mk tools.
 */
//--------------------------------------------------------------------------------------------------
enum ProgrammingLanguage_t
{
    LANG_C,    ///< The C programming language.
    LANG_CXX   ///< The C++ programming language.
};



//--------------------------------------------------------------------------------------------------
/**
 * Convert a name into one that is safe for use inside identifiers in C by replacing all unsafe
 * characters.
 *
 * @return  The C-safe name.
 *
 * @throw   legato::Exception if all the characters in the original name had to be replaced.
 */
//--------------------------------------------------------------------------------------------------
std::string GetCSafeName
(
    const std::string& name
);


}

#endif // PROGRAMMING_LANGUAGE_H_INCLUDE_GUARD
