//--------------------------------------------------------------------------------------------------
/**
 * @file token.cpp
 *
 * Implementation of Token_t functions.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"


namespace parseTree
{


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
Token_t::Token_t
(
    Type_t tokenType,
    DefFileFragment_t* fileObjPtr,
    size_t lineNum,
    size_t columnNum,
    int curPosition
)
//--------------------------------------------------------------------------------------------------
:   Content_t(Content_t::TOKEN, fileObjPtr),
    type(tokenType),
    line(lineNum),
    column(columnNum),
    curPos(curPosition),
    nextPtr(NULL),
    prevPtr(fileObjPtr->lastTokenPtr)
//--------------------------------------------------------------------------------------------------
{
    fileObjPtr->lastTokenPtr = this;
    if (prevPtr != NULL)
    {
        prevPtr->nextPtr = this;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a human-readable name of a given token type.
 *
 * @return The name.
 */
//--------------------------------------------------------------------------------------------------
std::string Token_t::TypeName
(
    Type_t type
)
//--------------------------------------------------------------------------------------------------
{
    switch (type)
    {
        case END_OF_FILE:
            return "end-of-file";

        case OPEN_CURLY:
            return "{";

        case CLOSE_CURLY:
            return "}";

        case OPEN_PARENTHESIS:
            return "(";

        case CLOSE_PARENTHESIS:
            return ")";

        case COLON:
            return ":";

        case EQUALS:
            return "=";

        case DOT:
            return ".";

        case STAR:
            return "*";

        case ARROW:
            return "->";

        case WHITESPACE:
            return "whitespace";

        case COMMENT:
            return "comment";

        case FILE_PERMISSIONS:
            return "file permissions";

        case SERVER_IPC_OPTION:
            return "server-side IPC option";

        case CLIENT_IPC_OPTION:
            return "client-side IPC option";

        case ARG:
            return "argument";

        case FILE_PATH:
            return "file path";

        case FILE_NAME:
            return "file name";

        case NAME:
            return "name";

        case DOTTED_NAME:
            return "dotted name";

        case GROUP_NAME:
            return "group name";

        case IPC_AGENT:
            return "IPC agent";

        case INTEGER:
            return "integer";

        case SIGNED_INTEGER:
            return "signed integer";

        case BOOLEAN:
            return "Boolean value";

        case FLOAT:
            return "floating point number";

        case STRING:
            return "string";

        case MD5_HASH:
            return "MD5 hash";

        case DIRECTIVE:
            return "directive";

        case OPTIONAL_OPEN_SQUARE:
            return "optional";

        case PROVIDE_HEADER_OPTION:
            return "provide-header";

        default:
        {
            std::stringstream typeName;
            typeName << "<out-of-range:" << type << ">";
            return typeName.str();
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Get a string containing a description of the location of the content item.
 *
 * E.g., "/home/userName/workdir/myApp.adef:123:12"
 *
 * where "123" is the line number in the file, and "12" is the column.
 */
//--------------------------------------------------------------------------------------------------
std::string Token_t::GetLocation
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    std::stringstream msg;

    msg << filePtr->path + ":" << line << ":" << column;

    return msg.str();
}


//--------------------------------------------------------------------------------------------------
/**
 * Throws an exception containing the file path, line number, and column number, in the same
 * style as a compiler would.
 */
//--------------------------------------------------------------------------------------------------
void Token_t::ThrowException
(
    const std::string& message
)
const
//--------------------------------------------------------------------------------------------------
{
    throw mk::Exception_t(
        mk::format(LE_I18N("%s: error: %s"), GetLocation(), message)
    );
}



//--------------------------------------------------------------------------------------------------
/**
 * Prints a warning message containing the file path, line number, and column number, in the same
 * style as a compiler would.
 */
//--------------------------------------------------------------------------------------------------
void Token_t::PrintWarning
(
    const std::string& message
)
const
//--------------------------------------------------------------------------------------------------
{
    std::cerr << LE_I18N("** WARNING: ") << std::endl
              << mk::format(LE_I18N("%s: warning: %s"), GetLocation(), message)
              << std::endl;
}




} // namespace parseTree
