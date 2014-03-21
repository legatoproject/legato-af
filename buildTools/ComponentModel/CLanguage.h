//--------------------------------------------------------------------------------------------------
/**
 * Helper functions related to the C programming language.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef C_LANGUAGE_H_INCLUDE_GUARD
#define C_LANGUAGE_H_INCLUDE_GUARD

namespace legato
{


//--------------------------------------------------------------------------------------------------
/**
 * Convert a name into one that is safe for use inside indentifiers in C by replacing all unsafe
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

#endif // C_LANGUAGE_H_INCLUDE_GUARD
