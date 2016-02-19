//--------------------------------------------------------------------------------------------------
/**
 * @file lexer.cpp Lexical Analyzer (Lexer) for the mk* tools.
 *
 * @warning Don't use isalpha() or isalnum() in this file, because their results are
            locale-dependent.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


namespace parser
{


//--------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
//--------------------------------------------------------------------------------------------------
Lexer_t::Lexer_t
(
    parseTree::DefFile_t* fileObjPtr
)
//--------------------------------------------------------------------------------------------------
:   beVerbose(false),
    filePtr(fileObjPtr),
    inputStream(filePtr->path),
    line(1),
    column(0)
//--------------------------------------------------------------------------------------------------
{
    // Make sure the file exists and we were able to open it.
    if (!file::FileExists(filePtr->path))
    {
        throw mk::Exception_t("File not found: '" + filePtr->path + "'.");
    }
    if (!inputStream.is_open())
    {
        throw mk::Exception_t("Failed to open file '" + filePtr->path + "' for reading.");
    }

    // Read in the first character.
    nextChar = inputStream.get();

    if (!inputStream.good())
    {
        throw mk::Exception_t("Failed to read from file '" + filePtr->path + "'.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks whether or not a given character is an accepted whitespace character.
 *
 * @return true if whitespace, false if something else.
 */
//--------------------------------------------------------------------------------------------------
static inline bool IsWhitespace
(
    const char character
)
//--------------------------------------------------------------------------------------------------
{
    // NOTE: Can't use isspace() because it accepts vertical tabs and form feeds, which are
    //       not allowed in def files.
    return (   (character == ' ')
            || (character == '\t')
            || (character == '\n')
            || (character == '\r')   );
}


//--------------------------------------------------------------------------------------------------
/**
 * Check whether a given character is valid within a FILE_NAME token (excluding " and ').
 *
 * @return true if valid, false if not.
 */
//--------------------------------------------------------------------------------------------------
static inline bool IsFileNameChar
(
    const char character
)
//--------------------------------------------------------------------------------------------------
{
    // NOTE: Don't use isalpha() or isalnum(), because their results are locale-dependent.
    return (   islower(character)
            || isupper(character)
            || isdigit(character)
            || (character == '.')
            || (character == '_')
            || (character == '$')  // Start of environment variable.
            || (character == '-')
            || (character == ':')
            || (character == ';')
            || (character == '+')
            || (character == '=')
            || (character == '?') );
}


//--------------------------------------------------------------------------------------------------
/**
 * Check whether a given character is valid within a FILE_PATH token (excluding " and ').
 *
 * @return true if valid, false if not.
 */
//--------------------------------------------------------------------------------------------------
static inline bool IsFilePathChar
(
    const char character
)
//--------------------------------------------------------------------------------------------------
{
    // Can be anything in a FILE_NAME, plus the forward slash (/).
    return (   IsFileNameChar(character)
            || (character == '/') );
}


//--------------------------------------------------------------------------------------------------
/**
 * Check whether a given character is valid within an ARG token (excluding " and ').
 *
 * @return true if valid, false if not.
 */
//--------------------------------------------------------------------------------------------------
static inline bool IsArgChar
(
    const char character
)
//--------------------------------------------------------------------------------------------------
{
    // Can be anything in a FILE_PATH, plus the equals sign (=).
    return (   IsFilePathChar(character)
            || (character == '=') );
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if the next sequence of text in the file could match a given type of token.
 */
//--------------------------------------------------------------------------------------------------
bool Lexer_t::IsMatch
(
    parseTree::Token_t::Type_t type    ///< The type of token to pull from the file.
)
//--------------------------------------------------------------------------------------------------
{
    switch (type)
    {
        case parseTree::Token_t::END_OF_FILE:
            return (nextChar == EOF);

        case parseTree::Token_t::OPEN_CURLY:
            return (nextChar == '{');

        case parseTree::Token_t::CLOSE_CURLY:
            return (nextChar == '}');

        case parseTree::Token_t::OPEN_PARENTHESIS:
            return (nextChar == '(');

        case parseTree::Token_t::CLOSE_PARENTHESIS:
            return (nextChar == ')');

        case parseTree::Token_t::COLON:
            return (nextChar == ':');

        case parseTree::Token_t::EQUALS:
            return (nextChar == '=');

        case parseTree::Token_t::DOT:
            return (nextChar == '.');

        case parseTree::Token_t::STAR:
            return (nextChar == '*');

        case parseTree::Token_t::ARROW:
            return ((nextChar == '-') && (inputStream.peek() == '>'));

        case parseTree::Token_t::WHITESPACE:
            return IsWhitespace(nextChar);

        case parseTree::Token_t::COMMENT:
            if (nextChar == '/')
            {
                int secondChar = inputStream.peek();
                return ((secondChar == '/') || (secondChar == '*'));
            }
            else
            {
                return false;
            }

        case parseTree::Token_t::FILE_PERMISSIONS:
        case parseTree::Token_t::SERVER_IPC_OPTION:
        case parseTree::Token_t::CLIENT_IPC_OPTION:
            return (nextChar == '[');

        case parseTree::Token_t::ARG:
            // Can be anything in a FILE_PATH, plus the equals sign (=).
            if (nextChar == '=')
            {
                return true;
            }
            // *** FALL THROUGH ***

        case parseTree::Token_t::FILE_PATH:
            // Can be anything in a FILE_NAME, plus the forward slash (/).
            // If it starts with a slash, it could be a comment or a file path.
            if (nextChar == '/')
            {
                // If it's not a comment, then it's a file path.
                int secondChar = inputStream.peek();
                return ((secondChar != '/') && (secondChar != '*'));
            }
            // *** FALL THROUGH ***

        case parseTree::Token_t::FILE_NAME:
            return (   IsFileNameChar(nextChar)
                    || (nextChar == '\'')   // Could be in single-quotes.
                    || (nextChar == '"') ); // Could be in quotes.

        case parseTree::Token_t::IPC_AGENT:
            // Can start with the same characters as a NAME or GROUP_NAME, plus '<'.
            if (nextChar == '<')
            {
                return true;
            }
            // *** FALL THROUGH ***

        case parseTree::Token_t::NAME:
        case parseTree::Token_t::GROUP_NAME:
            return (   islower(nextChar)
                    || isupper(nextChar)
                    || (nextChar == '_') );

        case parseTree::Token_t::INTEGER:
            return (isdigit(nextChar));

        case parseTree::Token_t::SIGNED_INTEGER:
            return (   (nextChar == '+')
                    || (nextChar == '-')
                    || isdigit(nextChar));

        case parseTree::Token_t::BOOLEAN:
            throw mk::Exception_t("Internal bug: BOOLEAN lookahead not implemented.");

        case parseTree::Token_t::FLOAT:
            throw mk::Exception_t("Internal bug: FLOAT lookahead not implemented.");

        case parseTree::Token_t::STRING:
            throw mk::Exception_t("Internal bug: STRING lookahead not implemented.");
    }

    throw mk::Exception_t("Internal bug: IsMatch(): Invalid token type requested.");
}



//--------------------------------------------------------------------------------------------------
/**
 * Pull a token from the file being parsed.
 */
//--------------------------------------------------------------------------------------------------
parseTree::Token_t* Lexer_t::Pull
(
    parseTree::Token_t::Type_t type    ///< The type of token to pull from the file.
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Token_t* tokenPtr = new parseTree::Token_t(type, filePtr, line, column);

    switch (type)
    {
        case parseTree::Token_t::END_OF_FILE:

            if (nextChar != EOF)
            {
                ThrowException("Expected end-of-file, but found '" + std::string(&nextChar) + "'.");
            }
            break;

        case parseTree::Token_t::OPEN_CURLY:

            PullConstString(tokenPtr, "{");
            break;

        case parseTree::Token_t::CLOSE_CURLY:

            PullConstString(tokenPtr, "}");
            break;

        case parseTree::Token_t::OPEN_PARENTHESIS:

            PullConstString(tokenPtr, "(");
            break;

        case parseTree::Token_t::CLOSE_PARENTHESIS:

            PullConstString(tokenPtr, ")");
            break;

        case parseTree::Token_t::COLON:

            PullConstString(tokenPtr, ":");
            break;

        case parseTree::Token_t::EQUALS:

            PullConstString(tokenPtr, "=");
            break;

        case parseTree::Token_t::DOT:

            PullConstString(tokenPtr, ".");
            break;

        case parseTree::Token_t::STAR:

            PullConstString(tokenPtr, "*");
            break;

        case parseTree::Token_t::ARROW:

            PullConstString(tokenPtr, "->");
            break;

        case parseTree::Token_t::WHITESPACE:

            PullWhitespace(tokenPtr);
            break;

        case parseTree::Token_t::COMMENT:

            PullComment(tokenPtr);
            break;

        case parseTree::Token_t::FILE_PERMISSIONS:

            PullFilePermissions(tokenPtr);
            break;

        case parseTree::Token_t::SERVER_IPC_OPTION:

            PullServerIpcOption(tokenPtr);
            break;

        case parseTree::Token_t::CLIENT_IPC_OPTION:

            PullClientIpcOption(tokenPtr);
            break;

        case parseTree::Token_t::ARG:

            PullArg(tokenPtr);
            break;

        case parseTree::Token_t::FILE_PATH:

            PullFilePath(tokenPtr);
            break;

        case parseTree::Token_t::FILE_NAME:

            PullFileName(tokenPtr);
            break;

        case parseTree::Token_t::NAME:

            PullName(tokenPtr);
            break;

        case parseTree::Token_t::GROUP_NAME:

            PullGroupName(tokenPtr);
            break;

        case parseTree::Token_t::IPC_AGENT:

            PullIpcAgentName(tokenPtr);
            break;

        case parseTree::Token_t::INTEGER:

            PullInteger(tokenPtr);
            break;

        case parseTree::Token_t::SIGNED_INTEGER:

            PullSignedInteger(tokenPtr);
            break;

        case parseTree::Token_t::BOOLEAN:

            PullBoolean(tokenPtr);
            break;

        case parseTree::Token_t::FLOAT:

            PullFloat(tokenPtr);
            break;

        case parseTree::Token_t::STRING:

            PullString(tokenPtr);
            break;
    }

    return tokenPtr;
}



//--------------------------------------------------------------------------------------------------
/**
 * Pulls a constant string token from the input stream.
 *
 * @throw mk::Exception_t if the next token in the stream does not match the string exactly.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullConstString
(
    parseTree::Token_t* tokenPtr,
    const char* tokenString
)
//--------------------------------------------------------------------------------------------------
{
    const char* charPtr = tokenString;

    while (*charPtr != '\0')
    {
        if (nextChar != *charPtr)
        {
            UnexpectedChar(" Expected '" + std::string(tokenString) + "'");
        }

        AdvanceOneCharacter(tokenPtr);
        charPtr++;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Pull a sequence of whitespace characters from the file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullWhitespace
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto startPos = inputStream.tellg();

    while (IsWhitespace(nextChar))
    {
        AdvanceOneCharacter(tokenPtr);
    }

    if (startPos == inputStream.tellg())
    {
        ThrowException("Expected whitespace.");
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Pull a comment from the file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullComment
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (nextChar != '/')
    {
        ThrowException("Expected '/' at start of comment.");
    }

    // Eat the leading '/'.
    AdvanceOneCharacter(tokenPtr);

    // Figure out which kind of comment it is.
    if (nextChar == '/')
    {
        // C++ style comment, terminated by either new-line or end-of-file.
        AdvanceOneCharacter(tokenPtr);
        while ((nextChar != '\n') && (nextChar != EOF))
        {
            AdvanceOneCharacter(tokenPtr);
        }
    }
    else if (nextChar == '*')
    {
        // C style comment, terminated by "*/" digraph.
        AdvanceOneCharacter(tokenPtr);
        for (;;)
        {
            if (nextChar == '*')
            {
                AdvanceOneCharacter(tokenPtr);

                if (nextChar == '/')
                {
                    AdvanceOneCharacter(tokenPtr);

                    break;
                }
            }
            else if (nextChar == EOF)
            {
                std::stringstream msg;
                msg << "Unexpected end-of-file before end of comment starting at line ";
                msg << tokenPtr->line << " character " << tokenPtr->column << ".";
                ThrowException(msg.str());
            }
            else
            {
                AdvanceOneCharacter(tokenPtr);
            }
        }
    }
    else
    {
        ThrowException("Expected '/' or '*' at start of comment.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull an integer (possibly ending in a K suffix) from the input file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullInteger
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (!isdigit(nextChar))
    {
        UnexpectedChar("at beginning of integer.");
    }

    while (isdigit(nextChar))
    {
        AdvanceOneCharacter(tokenPtr);
    }

    if (nextChar == 'K')
    {
        AdvanceOneCharacter(tokenPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull an integer (possibly ending in a K suffix) from the input file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullSignedInteger
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (   (nextChar == '-')
        || (nextChar == '+'))
    {
        AdvanceOneCharacter(tokenPtr);
    }

    PullInteger(tokenPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull a boolean value from the input file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullBoolean
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (nextChar == 't')
    {
        PullConstString(tokenPtr, "true");
    }
    else if (nextChar == 'f')
    {
        PullConstString(tokenPtr, "false");
    }
    else if (nextChar == 'o')
    {
        AdvanceOneCharacter(tokenPtr);

        if (nextChar == 'n')
        {
            AdvanceOneCharacter(tokenPtr);
        }
        else if (nextChar == 'f')
        {
            AdvanceOneCharacter(tokenPtr);

            if (nextChar != 'f')
            {
                ThrowException("Unexpected boolean value.  Only 'true', 'false', "
                               "'on', or 'off' allowed.");
            }

            AdvanceOneCharacter(tokenPtr);
        }
    }
    else
    {
        UnexpectedChar("at beginning of boolean value.  "
                       "Only 'true', 'false', 'on', or 'off' allowed.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull a floating point value from the input file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullFloat
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (   (isdigit(nextChar) == false)
        && (nextChar != '+')
        && (nextChar != '-'))
    {
        UnexpectedChar("at beginning of floating point value.");
    }

    AdvanceOneCharacter(tokenPtr);

    while (isdigit(nextChar))
    {
        AdvanceOneCharacter(tokenPtr);
    }

    if (nextChar == '.')
    {
        AdvanceOneCharacter(tokenPtr);

        while (isdigit(nextChar))
        {
            AdvanceOneCharacter(tokenPtr);
        }
    }

    if (   (nextChar == 'e')
        || (nextChar == 'E'))
    {
        AdvanceOneCharacter(tokenPtr);

        if (   (isdigit(nextChar) == false)
            && (nextChar != '+')
            && (nextChar != '-'))
        {
            UnexpectedChar("in exponent part of floating point value.");
        }

        AdvanceOneCharacter(tokenPtr);

        while (isdigit(nextChar))
        {
            AdvanceOneCharacter(tokenPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull a string literal from the input file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullString
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (   (nextChar == '"')
        || (nextChar == '\''))
    {
        PullQuoted(tokenPtr, nextChar);
    }
    else
    {
        ThrowException("Expected string literal.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull file permissions (e.g., "[rw]") from the file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullFilePermissions
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (nextChar != '[')
    {
        ThrowException("Expected '[' at start of file permissions.");
    }

    // Eat the leading '['.
    AdvanceOneCharacter(tokenPtr);

    // Must be something between the square brackets.
    if (nextChar == ']')
    {
        ThrowException("Empty file permissions.");
    }

    // Continue until terminated by ']'.
    do
    {
        // Check for end-of-file or illegal character in file permissions.
        if (nextChar == EOF)
        {
            ThrowException("Unexpected end-of-file before end of file permissions.");
        }
        else if ((nextChar != 'r') && (nextChar != 'w') && (nextChar != 'x'))
        {
            UnexpectedChar("inside file permissions.");
        }

        AdvanceOneCharacter(tokenPtr);

    } while (nextChar != ']');

    // Eat the trailing ']'.
    AdvanceOneCharacter(tokenPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull a server-side IPC option (e.g., "[manual-start]") from the file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullServerIpcOption
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    PullIpcOption(tokenPtr);

    // Check that it's one of the valid server-side options.
    if (   (tokenPtr->text != "[manual-start]")
        && (tokenPtr->text != "[async]") )
    {
        ThrowException("Invalid server-side IPC option: '" + tokenPtr->text + "'");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull a client-side IPC option (e.g., "[manual-start]") from the file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullClientIpcOption
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    PullIpcOption(tokenPtr);

    // Check that it's one of the valid server-side options.
    if (   (tokenPtr->text != "[manual-start]")
        && (tokenPtr->text != "[types-only]") )
    {
        ThrowException("Invalid client-side IPC option: '" + tokenPtr->text + "'");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull an IPC option (e.g., "[manual-start]") from the file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullIpcOption
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (nextChar != '[')
    {
        ThrowException("Expected '[' at start of IPC option.");
    }

    // Eat the leading '['.
    AdvanceOneCharacter(tokenPtr);

    // Must be something between the square brackets.
    if (nextChar == ']')
    {
        ThrowException("Empty IPC option.");
    }

    // Continue until terminated by ']'.
    do
    {
        // Check for end-of-file or illegal character in option.
        if (nextChar == EOF)
        {
            ThrowException("Unexpected end-of-file before end of IPC option.");
        }
        else if ((nextChar != '-') && !islower(nextChar))
        {
            UnexpectedChar("inside option.");
        }

        AdvanceOneCharacter(tokenPtr);

    } while (nextChar != ']');

    // Eat the trailing ']'.
    AdvanceOneCharacter(tokenPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull a command-line argument from the input file and store it in the token.
 *
 * Performs environment variable substitution before storing the result in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullArg
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (nextChar == '"')
    {
        PullQuoted(tokenPtr, '"');
    }
    else if (nextChar == '\'')
    {
        PullQuoted(tokenPtr, '\'');
    }
    else
    {
        auto startPos = inputStream.tellg();

        while (IsArgChar(nextChar))
        {
            if (nextChar == '$')
            {
                PullEnvVar(tokenPtr);
            }
            else
            {
                if (nextChar == '/')
                {
                    // Check for comment start.
                    int secondChar = inputStream.peek();
                    if ((secondChar == '/') || (secondChar == '*'))
                    {
                        break;
                    }
                }

                AdvanceOneCharacter(tokenPtr);
            }
        }

        // If no characters were matched, then the first character is an invalid argument
        // character.
        if (startPos == inputStream.tellg())
        {
            if (isprint(nextChar))
            {
                std::stringstream msg;
                msg << "Invalid character '" << nextChar << "' in argument.";
                ThrowException(msg.str());
            }
            else
            {
                ThrowException("Invalid (non-printable) character in argument.");
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull a file path from the input file and store it in the token.
 *
 * Performs environment variable substitution before storing the result in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullFilePath
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (nextChar == '"')
    {
        PullQuoted(tokenPtr, '"');
    }
    else if (nextChar == '\'')
    {
        PullQuoted(tokenPtr, '\'');
    }
    else
    {
        auto startPos = inputStream.tellg();

        while (IsFilePathChar(nextChar))
        {
            if (nextChar == '$')
            {
                PullEnvVar(tokenPtr);
            }
            else
            {
                if (nextChar == '/')
                {
                    // Check for comment start.
                    int secondChar = inputStream.peek();
                    if ((secondChar == '/') || (secondChar == '*'))
                    {
                        break;
                    }
                }

                AdvanceOneCharacter(tokenPtr);
            }
        }

        // If no characters were matched, then the first character is an invalid file path
        // character.
        if (startPos == inputStream.tellg())
        {
            if (isprint(nextChar))
            {
                std::stringstream msg;
                msg << "Invalid character '" << nextChar << "' in file path.";
                ThrowException(msg.str());
            }
            else
            {
                ThrowException("Invalid (non-printable) character in file path.");
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull a file name from the input file and store it in the token.
 *
 * Performs environment variable substitution before storing the result in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullFileName
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (nextChar == '"')
    {
        PullQuoted(tokenPtr, '"');
    }
    else if (nextChar == '\'')
    {
        PullQuoted(tokenPtr, '\'');
    }
    else
    {
        auto startPos = inputStream.tellg();

        while (IsFileNameChar(nextChar))
        {
            if (nextChar == '$')
            {
                PullEnvVar(tokenPtr);
            }
            else
            {
                AdvanceOneCharacter(tokenPtr);
            }
        }

        // If no characters were matched, then the first character is an invalid file name
        // character.
        if (startPos == inputStream.tellg())
        {
            if (isprint(nextChar))
            {
                ThrowException("Invalid character '" + std::string(&nextChar) + "' in name.");
            }
            else
            {
                ThrowException("Invalid (non-printable) character in name.");
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull a name from the input file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullName
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (   islower(nextChar)
        || isupper(nextChar)
        || (nextChar == '_') )
    {
        AdvanceOneCharacter(tokenPtr);
    }
    else
    {
        UnexpectedChar("at beginning of name. Names must start with a letter ('a'-'z' or 'A'-'Z')"
                       " or an underscore ('_').");
    }

    while (   islower(nextChar)
           || isupper(nextChar)
           || isdigit(nextChar)
           || (nextChar == '_') )
    {
        AdvanceOneCharacter(tokenPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull a group name from the input file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullGroupName
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (   islower(nextChar)
        || isupper(nextChar)
        || (nextChar == '_') )
    {
        AdvanceOneCharacter(tokenPtr);
    }
    else
    {
        UnexpectedChar("at beginning of group name. Group names must start with a letter "
                       "('a'-'z' or 'A'-'Z') or an underscore ('_').");
    }

    while (   islower(nextChar)
           || isupper(nextChar)
           || isdigit(nextChar)
           || (nextChar == '_')
           || (nextChar == '-') )
    {
        AdvanceOneCharacter(tokenPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull the name of an IPC agent (user or app) from the input file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullIpcAgentName
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto firstChar = nextChar;

    // User names are enclosed in angle brackets (e.g., "<username>").
    if (firstChar == '<')
    {
        AdvanceOneCharacter(tokenPtr);

        while (   islower(nextChar)
               || isupper(nextChar)
               || isdigit(nextChar)
               || (nextChar == '_')
               || (nextChar == '-') )
        {
            AdvanceOneCharacter(tokenPtr);
        }

        if (nextChar != '>')
        {
            UnexpectedChar("in user name.  Must be terminated with '>'.");
        }
        else
        {
            AdvanceOneCharacter(tokenPtr);
        }
    }
    // App names have the same rules as C programming language identifiers.
    else if (   islower(nextChar)
             || isupper(nextChar)
             || (nextChar == '_') )
    {
        AdvanceOneCharacter(tokenPtr);

        while (   islower(nextChar)
               || isupper(nextChar)
               || isdigit(nextChar)
               || (nextChar == '_') )
        {
            AdvanceOneCharacter(tokenPtr);
        }
    }
    else
    {
        UnexpectedChar("at beginning of IPC agent name. App names must start with a letter "
                       "('a'-'z' or 'A'-'Z') or an underscore ('_').  User names must be inside"
                       " angle brackets ('<username>').");
    }

}


//--------------------------------------------------------------------------------------------------
/**
 * Pull into a token's text everything up to and including the first occurrence of a given
 * quote character.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullQuoted
(
    parseTree::Token_t* tokenPtr,
    char quoteChar
)
//--------------------------------------------------------------------------------------------------
{
    // Eat the leading quote.
    AdvanceOneCharacter(tokenPtr);

    while (nextChar != quoteChar)
    {
        // Don't allow end of file or end of line characters inside the quoted string.
        if (nextChar == EOF)
        {
            ThrowException("Unexpected end-of-file before end of quoted string.");
        }
        if ((nextChar == '\n') || (nextChar == '\r'))
        {
            ThrowException("Unexpected end-of-line before end of quoted string.");
        }

        AdvanceOneCharacter(tokenPtr);
    }

    // Eat the trailing quote.
    AdvanceOneCharacter(tokenPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Pulls an environment variable out of the input file stream and appends it to a given token.
 *
 * NOTE: Environment variable substitution is not done here because we want to preserve the
 *       token text exactly as it appeared in the file.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullEnvVar
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Get the '$'.
    AdvanceOneCharacter(tokenPtr->text);

    // If the next character is a curly brace, remember that we need to look for the closing curly.
    bool hasCurlies = false;    // true if ${ENV_VAR} style.  false if $ENV_VAR style.
    if (nextChar == '{')
    {
        AdvanceOneCharacter(tokenPtr->text);
        hasCurlies = true;
    }

    // Pull the first character of the environment variable name.
    if (   islower(nextChar)
        || isupper(nextChar)
        || (nextChar == '_') )
    {
        AdvanceOneCharacter(tokenPtr->text);
    }
    else
    {
        UnexpectedChar("at beginning of environment variable name.  Must start with a letter"
                       " ('a'-'z' or 'A'-'Z') or an underscore ('_').");
    }

    // Pull the rest of the environment variable name.
    while (   islower(nextChar)
           || isupper(nextChar)
           || isdigit(nextChar)
           || (nextChar == '_') )
    {
        AdvanceOneCharacter(tokenPtr->text);
    }

    // If there was an opening curly brace, match the closing one now.
    if (hasCurlies)
    {
        if (nextChar == '}')
        {
            AdvanceOneCharacter(tokenPtr->text);
        }
        else if (nextChar == EOF)
        {
            ThrowException("Unexpected end-of-file inside environment variable name.");
        }
        else
        {
            ThrowException("'}' expected.  '" + std::string(&nextChar) + "' found.");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Advance the current file position by one character, appending the character into a given token's
 * text value and updating the line and column numbers.
 *
 * @throw mk::Exception_t if this can't be done.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::AdvanceOneCharacter
(
    parseTree::Token_t* tokenPtr    ///< Token object to add the character to.
)
//--------------------------------------------------------------------------------------------------
{
    AdvanceOneCharacter(tokenPtr->text);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate an "Unexpected character" error message.
 *
 * @return The message.
 */
//--------------------------------------------------------------------------------------------------
std::string Lexer_t::UnexpectedCharErrorMsg
(
    char unexpectedChar,
    size_t lineNum,
    size_t columnNum,
    const std::string& message  ///< Additional message to append.
)
//--------------------------------------------------------------------------------------------------
{
    std::stringstream msg;

    msg << filePtr->path << ":" << lineNum << ":" << columnNum << ": error: ";

    if (isprint(unexpectedChar))
    {
        msg << "Unexpected character '" << unexpectedChar << "' ";
    }
    else
    {
        msg << "Unexpected non-printable character ";
    }

    msg << message;

    return msg.str();
}


//--------------------------------------------------------------------------------------------------
/**
 * Attempt to convert a given token to a NAME token.
 *
 * @throw mk::Exception_t if the token contains characters that are not allowed in a NAME.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::ConvertToName
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    const char* charPtr = tokenPtr->text.c_str();

    if (   (!islower(charPtr[0]))
        && (!isupper(charPtr[0]))
        && (charPtr[0] != '_') )
    {
        throw mk::Exception_t(UnexpectedCharErrorMsg(charPtr[0],
                                                          tokenPtr->line,
                                                          tokenPtr->column,
                                                          "at beginning of name. Names must start"
                                                          " with a letter ('a'-'z' or 'A'-'Z')"
                                                          " or an underscore ('_')."));
    }

    charPtr++;

    while (*charPtr != '\0')
    {
        if (   (!islower(*charPtr))
            && (!isupper(*charPtr))
            && (!isdigit(*charPtr))
            && (*charPtr != '_') )
        {
            throw mk::Exception_t(UnexpectedCharErrorMsg(charPtr[0],
                                                              tokenPtr->line,
                                                              tokenPtr->column,
                                                              "Names may only contain letters"
                                                              " ('a'-'z' or 'A'-'Z'),"
                                                              " numbers ('0'-'9')"
                                                              " and underscores ('_')."));
        }

        charPtr++;
    }

    // Everything looks fine.  Convert token type now.
    tokenPtr->type = parseTree::Token_t::NAME;
}


//--------------------------------------------------------------------------------------------------
/**
 * Advance the current file position by one character, appending the character into a given string
 * and updating the line and column numbers.
 *
 * @throw mk::Exception_t if this can't be done.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::AdvanceOneCharacter
(
    std::string& string     ///< String to add the character to.
)
//--------------------------------------------------------------------------------------------------
{
    string += nextChar;

    if (nextChar == '\n')
    {
        line++;
        column = 0;
    }
    else
    {
        column++;
    }

    nextChar = inputStream.get();

    if (inputStream.bad())
    {
        ThrowException("Failed to fetch next character from file.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Throws an exception containing the file path, line number, and column number, in the same
 * style as a compiler would.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::ThrowException
(
    const std::string& message
)
//--------------------------------------------------------------------------------------------------
{
    std::stringstream msg;

    msg << filePtr->path << ":" << line << ":" << column << ": error: " << message;

    throw mk::Exception_t(msg.str());
}



//--------------------------------------------------------------------------------------------------
/**
 * Throws an unexpected character exception containing the file path, line number, column number,
 * and information about the unexpected character.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::UnexpectedChar
(
    const std::string& message  ///< Additional info to append to the exception message.
)
//--------------------------------------------------------------------------------------------------
{
    throw mk::Exception_t(UnexpectedCharErrorMsg(nextChar, line, column, message));
}



} // namespace parser
