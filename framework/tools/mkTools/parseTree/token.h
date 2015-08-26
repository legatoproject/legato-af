//--------------------------------------------------------------------------------------------------
/**
 * @file token.h Lexical token definitions.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_TOKEN_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_TOKEN_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of all possible types of lexical tokens in .Xdef files.
 */
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Lexical token in a .Xdef file.
 */
//--------------------------------------------------------------------------------------------------
struct Token_t: public Content_t
{
    enum Type_t
    {
        END_OF_FILE,        ///< The end of the file being parsed.
        OPEN_CURLY,         ///< '{'
        CLOSE_CURLY,        ///< '}'
        OPEN_PARENTHESIS,   ///< '('
        CLOSE_PARENTHESIS,  ///< ')'
        COLON,              ///< ':'
        EQUALS,             ///< '='
        DOT,                ///< '.'
        STAR,               ///< '*'
        ARROW,              ///< "->"
        WHITESPACE,         ///< Any combination of contiguous spaces, tabs, newlines and returns.
        COMMENT,            ///< A comment.
        FILE_PERMISSIONS,   ///< File permissions, in square brackets (e.g., "[rw]").
        SERVER_IPC_OPTION,  ///< Server-side IPC option, in square brackets (e.g., "[async]").
        CLIENT_IPC_OPTION,  ///< Client-side IPC option, in square brackets (e.g., "[types-only]").
        ARG,                ///< A command-line argument.
        FILE_PATH,          ///< A file system path.
        FILE_NAME,          ///< The name of a file or another name having the same constraints.
        NAME,               ///< Name safe to use as a program identifier in C-like languages.
        GROUP_NAME,         ///< Name safe to use as a user group name in Unix.
        IPC_AGENT,          ///< App or user name in a binding (e.g., "appName" or "<userName>").
        INTEGER,            ///< Integer number, possibly with a 'K' suffix.
        SIGNED_INTEGER,     ///< Like Integer, but supports both positive and negative values.
        BOOLEAN,            ///< Either "true" or "false".
        FLOAT,              ///< Standard  C style floating point number.
        STRING              ///< String value quoted with a ' or a ".
    };

    Type_t type;        ///< The type of token.

    size_t line;        ///< The line number it was found in (1 = first line).
    size_t column;      ///< The column number it was found in (0 = first column).

    std::string text;   ///< The text of the token copied verbatim from the file.

    Token_t* nextPtr;   ///< Ptr to the next token, closer to the end of the file.
    Token_t* prevPtr;   ///< Ptr to the previous token, closer to the beginning of the file.

    Token_t(Type_t tokenType, DefFile_t* fileObjPtr, size_t lineNum, size_t columnNum);

    // Get a human-readable string describing the type.
    static std::string TypeName(Type_t type);
    std::string TypeName() { return TypeName(type); }

    // Throw an exception with the file, line and column at the front.
    void ThrowException(const std::string& message) const __attribute__ ((noreturn));
};


#endif // LEGATO_MKTOOLS_TOKEN_H_INCLUDE_GUARD
