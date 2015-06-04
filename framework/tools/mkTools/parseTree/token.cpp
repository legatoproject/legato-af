//--------------------------------------------------------------------------------------------------
/**
 * @file token.cpp
 *
 * Implementation of Token_t functions.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


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
    DefFile_t* fileObjPtr,
    size_t lineNum,
    size_t columnNum
)
//--------------------------------------------------------------------------------------------------
:   Content_t(Content_t::TOKEN, fileObjPtr),
    type(tokenType),
    line(lineNum),
    column(columnNum),
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

        case GROUP_NAME:
            return "group name";

        case IPC_AGENT:
            return "IPC agent";

        case INTEGER:
            return "integer";

        case BOOLEAN:
            return "Boolean value";

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
    std::stringstream msg;

    msg << filePtr->path << ":" << line << ":" << column << ": error: " << message;

    throw mk::Exception_t(msg.str());
}




} // namespace parseTree
