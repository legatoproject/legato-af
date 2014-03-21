//--------------------------------------------------------------------------------------------------
/**
 * Implementation of functions that are common to both the Component Parser and the
 * Application Parser.
 *
 * Copyright (C) 201 Sierra Wireless Inc., all rights reserved.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"

extern "C"
{
    #include "ParserCommonInternals.h"
}

#include <limits.h>


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
)
//--------------------------------------------------------------------------------------------------
{
    int permissions = 0;
    int i;

    // Check each character and set the appropriate flag.
    // NOTE: We can assume that the first character is '[', the last character is ']', and
    //       the only characters in between are 'r', 'w', 'x', and/or 'p' because that's
    //       enforced by the lexer.
    for (i = 1; stringPtr[i] != ']'; i++)
    {
        switch (stringPtr[i])
        {
            case 'r':
                permissions |= legato::PERMISSION_READABLE;
                break;

            case 'w':
                permissions |= legato::PERMISSION_WRITEABLE;
                break;

            case 'x':
                permissions |= legato::PERMISSION_EXECUTABLE;
                break;

            case 'p':
                permissions |= legato::PERMISSION_PERSISTENT | legato::PERMISSION_WRITEABLE;
                break;

            default:
                fprintf(stderr,
                        "Unexpected character '%c' in permissions string '%s' (lexer bug).\n",
                        stringPtr[i],
                        stringPtr);
                exit(EXIT_FAILURE);
        }
    }

    std::cerr << "** WARNING: File permissions not fully supported yet." << std::endl;

    return permissions;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    char* endPtr = NULL;

    errno = 0;
    long int number = strtol(stringPtr, &endPtr, 0);
    if ((errno == ERANGE) || (number < INT_MIN) || (number > INT_MAX))
    {
        std::string msg = "Number '";
        msg += stringPtr;
        msg += "' is out of range (magnitude too large).";
        throw legato::Exception(msg);
    }

    if (*endPtr != '\0')
    {
        if ((endPtr[0] == 'K') && (endPtr[1] == '\0'))
        {
            number *= 1024;
            if ((number < INT_MIN) || (number > INT_MAX))
            {
                std::string msg = "Number '";
                msg += stringPtr;
                msg += "' is out of range (magnitude too large).";
                throw legato::Exception(msg);
            }
        }
        else
        {
            fprintf(stderr,
                    "Unexpected character '%c' in number '%s' (lexer bug).\n",
                    *endPtr,
                    stringPtr);
            exit(EXIT_FAILURE);
        }
    }

    return (int)number;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    size_t startPos = 0;

    size_t quotePos = string.find('"');
    if (quotePos == string.npos)
    {
        return string;
    }
    else
    {
        std::string result;

        while (quotePos != string.npos)
        {
            result += string.substr(startPos, quotePos - startPos);
            startPos = quotePos + 1;
            quotePos = string.find('"', startPos);
        }
        result += string.substr(startPos);

        return result;
    }
}
