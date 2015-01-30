//--------------------------------------------------------------------------------------------------
/**
 * Helper functions related to the C programming language.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"

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
)
//--------------------------------------------------------------------------------------------------
{
    std::string result;
    size_t safeOriginalCharCount = 0;

    for (auto c : name)
    {
        // Copy characters that are safe to be used in a C identifier into the result string.
        // Replace all other characters with an underscore.
        if (   ((c >= '0') && (c <= '9'))
            || ((c >= 'A') && (c <= 'Z'))
            || ((c >= 'a') && (c <= 'z'))
            || (c == '_')
           )
        {
            safeOriginalCharCount++;
        }
        else
        {
            c = '_';
        }

        result.push_back(c);
    }

    // Double check that the name is not empty.
    if (safeOriginalCharCount == 0)
    {
        throw legato::Exception("Name '" + name
                                + "' contained no characters safe for use in a C identifier.");
    }

    return result;
}



}
