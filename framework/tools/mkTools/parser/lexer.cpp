//--------------------------------------------------------------------------------------------------
/**
 * @file lexer.cpp Lexical Analyzer (Lexer) for the mk* tools.
 *
 * @warning Don't use isalpha() or isalnum() in this file, because their results are
 locale-dependent.
 *
 * Copyright (C) Sierra Wireless Inc.
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
Lexer_t::LexerContext_t::LexerContext_t
(
    parseTree::DefFileFragment_t* filePtr
)
//--------------------------------------------------------------------------------------------------
:   filePtr(filePtr),
    inputStream(filePtr->path),
    line(1),
    column(0)
//--------------------------------------------------------------------------------------------------
{
    // Make sure the file exists and we were able to open it.
    if (!file::FileExists(filePtr->path))
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("File not found: '%s'."), filePtr->path)
        );
    }
    if (!inputStream.is_open())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for reading."), filePtr->path)
        );
    }

    // Read in the first character.
    nextChar = inputStream.get();

    if (!inputStream.good())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to read from file '%s'."), filePtr->path)
        );
    }
}

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
:   beVerbose(false)
//--------------------------------------------------------------------------------------------------
{
    // Setup the lexer context for the top-level file
    context.emplace(fileObjPtr);

    // then move to the first token
    NextToken();
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
            return (context.top().nextChar == EOF);

        case parseTree::Token_t::OPEN_CURLY:
            return (context.top().nextChar == '{');

        case parseTree::Token_t::CLOSE_CURLY:
            return (context.top().nextChar == '}');

        case parseTree::Token_t::OPEN_PARENTHESIS:
            return (context.top().nextChar == '(');

        case parseTree::Token_t::CLOSE_PARENTHESIS:
            return (context.top().nextChar == ')');

        case parseTree::Token_t::COLON:
            return (context.top().nextChar == ':');

        case parseTree::Token_t::EQUALS:
            return (context.top().nextChar == '=');

        case parseTree::Token_t::DOT:
            return (context.top().nextChar == '.');

        case parseTree::Token_t::STAR:
            return (context.top().nextChar == '*');

        case parseTree::Token_t::ARROW:
            return ((context.top().nextChar == '-') && (context.top().inputStream.peek() == '>'));

        case parseTree::Token_t::WHITESPACE:
            return IsWhitespace(context.top().nextChar);

        case parseTree::Token_t::COMMENT:
            if (context.top().nextChar == '/')
            {
                int secondChar = context.top().inputStream.peek();
                return ((secondChar == '/') || (secondChar == '*'));
            }
            else
            {
                return false;
            }

        case parseTree::Token_t::FILE_PERMISSIONS:
        case parseTree::Token_t::SERVER_IPC_OPTION:
        case parseTree::Token_t::CLIENT_IPC_OPTION:
            return (context.top().nextChar == '[');

        case parseTree::Token_t::ARG:
            // Can be anything in a FILE_PATH, plus the equals sign (=).
            if (context.top().nextChar == '=')
            {
                return true;
            }
            // *** FALL THROUGH ***

        case parseTree::Token_t::FILE_PATH:
            // Can be anything in a FILE_NAME, plus the forward slash (/).
            // If it starts with a slash, it could be a comment or a file path.
            if (context.top().nextChar == '/')
            {
                // If it's not a comment, then it's a file path.
                int secondChar = context.top().inputStream.peek();
                return ((secondChar != '/') && (secondChar != '*'));
            }
            // *** FALL THROUGH ***

        case parseTree::Token_t::FILE_NAME:
            return (   IsFileNameChar(context.top().nextChar)
                       || (context.top().nextChar == '\'')   // Could be in single-quotes.
                       || (context.top().nextChar == '"') ); // Could be in quotes.

        case parseTree::Token_t::IPC_AGENT:
            // Can start with the same characters as a NAME or GROUP_NAME, plus '<'.
            if (context.top().nextChar == '<')
            {
                return true;
            }
            // *** FALL THROUGH ***

        case parseTree::Token_t::NAME:
        case parseTree::Token_t::GROUP_NAME:
        case parseTree::Token_t::DOTTED_NAME:
            return (   islower(context.top().nextChar)
                       || isupper(context.top().nextChar)
                       || (context.top().nextChar == '_') );

        case parseTree::Token_t::INTEGER:
            return (isdigit(context.top().nextChar));

        case parseTree::Token_t::SIGNED_INTEGER:
            return (   (context.top().nextChar == '+')
                       || (context.top().nextChar == '-')
                       || isdigit(context.top().nextChar));

        case parseTree::Token_t::BOOLEAN:
            return IsMatchBoolean();

        case parseTree::Token_t::FLOAT:
            throw mk::Exception_t(LE_I18N("Internal error: FLOAT lookahead not implemented."));

        case parseTree::Token_t::STRING:
            throw mk::Exception_t(LE_I18N("Internal error: STRING lookahead not implemented."));

        case parseTree::Token_t::MD5_HASH:
            return isxdigit(context.top().nextChar);

        case parseTree::Token_t::DIRECTIVE:
            return context.top().nextChar == '#';
    }

    throw mk::Exception_t(LE_I18N("Internal error: IsMatch(): Invalid token type requested."));
}



//--------------------------------------------------------------------------------------------------
/**
 * Pull a single token from the file being parsed, leaving the point immediately after the token.
 */
//--------------------------------------------------------------------------------------------------
parseTree::Token_t* Lexer_t::PullRaw
(
    parseTree::Token_t::Type_t type    ///< The type of token to pull from the file.
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Token_t* tokenPtr = new parseTree::Token_t(type, context.top().filePtr,
                                                          context.top().line, context.top().column);

    switch (type)
    {
        case parseTree::Token_t::END_OF_FILE:

            if (context.top().nextChar != EOF)
            {
                ThrowException(
                    mk::format(LE_I18N("Expected end-of-file, but found '%c'."),
                               (char)context.top().nextChar)
                );
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

        case parseTree::Token_t::DOTTED_NAME:

            PullDottedName(tokenPtr);
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

        case parseTree::Token_t::MD5_HASH:

            PullMd5(tokenPtr);
            break;

        case parseTree::Token_t::DIRECTIVE:

            PullDirective(tokenPtr);
            break;
    }

    return tokenPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Pull a token from the file being parsed, moving the point to the start of the next important
 * token.
 */
//--------------------------------------------------------------------------------------------------
parseTree::Token_t* Lexer_t::Pull
(
    parseTree::Token_t::Type_t type
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Token_t* tokenPtr = PullRaw(type);

    NextToken();

    return tokenPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Move to the start of the next interesting token in the input stream.
 *
 * Interesting is currently non-whitespace, non-comment.  Any uninteresting tokens are still added
 * to the token list for the file, but not returned by Lexer_t::Pull()
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::NextToken
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    for (;;)
    {
        if (IsMatch(parseTree::Token_t::WHITESPACE))
        {
            (void)PullRaw(parseTree::Token_t::WHITESPACE);
        }
        else if (IsMatch(parseTree::Token_t::COMMENT))
        {
            (void)PullRaw(parseTree::Token_t::COMMENT);
        }
        else if (IsMatch(parseTree::Token_t::DIRECTIVE))
        {
            ProcessDirective();
        }
        else if (IsMatch(parseTree::Token_t::END_OF_FILE))
        {
            // If not processing the top-level file, back to the next higher level.
            if (context.size() > 1)
            {
                (void)PullRaw(parseTree::Token_t::END_OF_FILE);

                context.pop();
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Process a single directive.
 *
 * The directives supported by mk* tools are:
 *   - #include "file": Include another file in this one.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::ProcessDirective
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Token_t *directivePtr = PullRaw(parseTree::Token_t::DIRECTIVE);

    // Skip whitespace between directive and any arguments
    if (IsMatch(parseTree::Token_t::WHITESPACE))
    {
        (void)PullRaw(parseTree::Token_t::WHITESPACE);
    }

    if (directivePtr->text == std::string("#include"))
    {
        ProcessIncludeDirective();
    }
    else
    {
        ThrowException(
            mk::format(LE_I18N("Unrecognized processing directive '%s'"),
                       directivePtr->text)
        );
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Process an include directive.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::ProcessIncludeDirective
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Token_t* includePathPtr = PullRaw(parseTree::Token_t::FILE_PATH);
    std::set<std::string> substitutedVars;

    auto filePath = path::Unquote(envVars::DoSubstitution(includePathPtr->text,
                                                          &substitutedVars));

    for (auto substitutedVar: substitutedVars)
    {
        // No need to check if variable is already in usedVars list as insert will simply
        // fail in that case, leaving the original definition intact).
        usedVars.insert(std::make_pair(substitutedVar, includePathPtr));
    }

    auto curDir = path::GetContainingDir(context.top().filePtr->path);

    // First search for include file in the including file's directory, then in the LEGATO_ROOT
    // directory
    auto includePath = file::FindFile(filePath, { curDir });
    if (includePath == "")
    {
        includePath = file::FindFile(filePath, { envVars::Get("LEGATO_ROOT") });
    }

    if (includePath == "")
    {
        ThrowException(
            mk::format(LE_I18N("File '%s' not found."), filePath)
        );
    }

    // Construct a new file fragment for the included file and move parsing to that fragment
    auto fileFragmentPtr = new parseTree::DefFileFragment_t(includePath);

    context.top().filePtr->includedFiles.insert(std::make_pair(includePathPtr, fileFragmentPtr));
    context.emplace(fileFragmentPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Check if a valid boolean value (true, false, on, or off) is waiting in the input stream.
 *
 * @return true if a valid boolean is waiting.  false otherwise.
 **/
//--------------------------------------------------------------------------------------------------
bool Lexer_t::IsMatchBoolean
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    char lookaheadBuff[6]; // Longest boolean value is "false" (5 chars, plus one after).
    size_t n = 0;

    bool result = false;

    switch (context.top().nextChar)
    {
        case 't':

            n = Lookahead(lookaheadBuff, 4);

            if (   (n == 4)
                   && (strncmp(lookaheadBuff, "true", 4) == 0))
            {
                result = true;
            }
            break;

        case 'f':

            n = Lookahead(lookaheadBuff, 5);

            if (   (n == 5)
                   && (strncmp(lookaheadBuff, "false", 5) == 0))
            {
                result = true;
            }
            break;

        case 'o':

            n = Lookahead(lookaheadBuff, 3); // "off" is 3 bytes long.

            if (   ((n >= 2) && (strncmp(lookaheadBuff, "on", 2) == 0))
                   || ((n == 3) && (strncmp(lookaheadBuff, "off", 3) == 0))  )
            {
                result = true;
            }
            break;
    }

    return result;
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
        if (context.top().nextChar != *charPtr)
        {
            UnexpectedChar(mk::format(LE_I18N("Unexpected character %%s. Expected '%s'"),
                                      tokenString));
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
    auto startPos = context.top().inputStream.tellg();

    while (IsWhitespace(context.top().nextChar))
    {
        AdvanceOneCharacter(tokenPtr);
    }

    if (startPos == context.top().inputStream.tellg())
    {
        ThrowException(LE_I18N("Expected whitespace."));
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
    if (context.top().nextChar != '/')
    {
        ThrowException(LE_I18N("Expected '/' at start of comment."));
    }

    // Eat the leading '/'.
    AdvanceOneCharacter(tokenPtr);

    // Figure out which kind of comment it is.
    if (context.top().nextChar == '/')
    {
        // C++ style comment, terminated by either new-line or end-of-file.
        AdvanceOneCharacter(tokenPtr);
        while ((context.top().nextChar != '\n') && (context.top().nextChar != EOF))
        {
            AdvanceOneCharacter(tokenPtr);
        }
    }
    else if (context.top().nextChar == '*')
    {
        // C style comment, terminated by "*/" digraph.
        AdvanceOneCharacter(tokenPtr);
        for (;;)
        {
            if (context.top().nextChar == '*')
            {
                AdvanceOneCharacter(tokenPtr);

                if (context.top().nextChar == '/')
                {
                    AdvanceOneCharacter(tokenPtr);

                    break;
                }
            }
            else if (context.top().nextChar == EOF)
            {
                ThrowException(
                    mk::format(LE_I18N("Unexpected end-of-file before end of comment.\n"
                                       "%s: note: Comment starts here."), tokenPtr->GetLocation())
                );
            }
            else
            {
                AdvanceOneCharacter(tokenPtr);
            }
        }
    }
    else
    {
        ThrowException(LE_I18N("Expected '/' or '*' at start of comment."));
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
    if (!isdigit(context.top().nextChar))
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of integer."));
    }

    while (isdigit(context.top().nextChar))
    {
        AdvanceOneCharacter(tokenPtr);
    }

    if (context.top().nextChar == 'K')
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
    if (   (context.top().nextChar == '-')
           || (context.top().nextChar == '+'))
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
    if (context.top().nextChar == 't')
    {
        PullConstString(tokenPtr, "true");
    }
    else if (context.top().nextChar == 'f')
    {
        PullConstString(tokenPtr, "false");
    }
    else if (context.top().nextChar == 'o')
    {
        AdvanceOneCharacter(tokenPtr);

        if (context.top().nextChar == 'n')
        {
            AdvanceOneCharacter(tokenPtr);
        }
        else if (context.top().nextChar == 'f')
        {
            AdvanceOneCharacter(tokenPtr);

            if (context.top().nextChar != 'f')
            {
                ThrowException(LE_I18N("Unexpected boolean value.  Only 'true', 'false', "
                                       "'on', or 'off' allowed."));
            }

            AdvanceOneCharacter(tokenPtr);
        }
    }
    else
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of boolean value.  "
                               "Only 'true', 'false', 'on', or 'off' allowed."));
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
    if (   (isdigit(context.top().nextChar) == false)
           && (context.top().nextChar != '+')
           && (context.top().nextChar != '-'))
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of floating point value."));
    }

    AdvanceOneCharacter(tokenPtr);

    while (isdigit(context.top().nextChar))
    {
        AdvanceOneCharacter(tokenPtr);
    }

    if (context.top().nextChar == '.')
    {
        AdvanceOneCharacter(tokenPtr);

        while (isdigit(context.top().nextChar))
        {
            AdvanceOneCharacter(tokenPtr);
        }
    }

    if (   (context.top().nextChar == 'e')
           || (context.top().nextChar == 'E'))
    {
        AdvanceOneCharacter(tokenPtr);

        if (   (isdigit(context.top().nextChar) == false)
               && (context.top().nextChar != '+')
               && (context.top().nextChar != '-'))
        {
            UnexpectedChar(LE_I18N("Unexpected character %s in exponent part of"
                                   " floating point value."));
        }

        AdvanceOneCharacter(tokenPtr);

        while (isdigit(context.top().nextChar))
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
    if (   (context.top().nextChar == '"')
           || (context.top().nextChar == '\''))
    {
        PullQuoted(tokenPtr, context.top().nextChar);
    }
    else
    {
        ThrowException(LE_I18N("Expected string literal."));
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
    if (context.top().nextChar != '[')
    {
        ThrowException(LE_I18N("Expected '[' at start of file permissions."));
    }

    // Eat the leading '['.
    AdvanceOneCharacter(tokenPtr);

    // Must be something between the square brackets.
    if (context.top().nextChar == ']')
    {
        ThrowException(LE_I18N("Empty file permissions."));
    }

    // Continue until terminated by ']'.
    do
    {
        // Check for end-of-file or illegal character in file permissions.
        if (context.top().nextChar == EOF)
        {
            ThrowException(LE_I18N("Unexpected end-of-file before end of file permissions."));
        }
        else if ((context.top().nextChar != 'r') && (context.top().nextChar != 'w') && (context.top().nextChar != 'x'))
        {
            UnexpectedChar(LE_I18N("Unexpected character %s inside file permissions."));
        }

        AdvanceOneCharacter(tokenPtr);

    } while (context.top().nextChar != ']');

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
        ThrowException(
            mk::format(LE_I18N("Invalid server-side IPC option: '%s'"), tokenPtr->text)
        );
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

    // Check that it's one of the valid client-side options.
    if (   (tokenPtr->text != "[manual-start]")
           && (tokenPtr->text != "[types-only]")
           && (tokenPtr->text != "[optional]") )
    {
        ThrowException(
            mk::format(LE_I18N("Invalid client-side IPC option: '%s'"), tokenPtr->text)
        );
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
    if (context.top().nextChar != '[')
    {
        ThrowException(LE_I18N("Expected '[' at start of IPC option."));
    }

    // Eat the leading '['.
    AdvanceOneCharacter(tokenPtr);

    // Must be something between the square brackets.
    if (context.top().nextChar == ']')
    {
        ThrowException(LE_I18N("Empty IPC option."));
    }

    // Continue until terminated by ']'.
    do
    {
        // Check for end-of-file or illegal character in option.
        if (context.top().nextChar == EOF)
        {
            ThrowException(LE_I18N("Unexpected end-of-file before end of IPC option."));
        }
        else if ((context.top().nextChar != '-') && !islower(context.top().nextChar))
        {
            UnexpectedChar(LE_I18N("Unexpected character %s inside option."));
        }

        AdvanceOneCharacter(tokenPtr);

    } while (context.top().nextChar != ']');

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
    if (context.top().nextChar == '"')
    {
        PullQuoted(tokenPtr, '"');
    }
    else if (context.top().nextChar == '\'')
    {
        PullQuoted(tokenPtr, '\'');
    }
    else
    {
        auto startPos = context.top().inputStream.tellg();

        while (IsArgChar(context.top().nextChar))
        {
            if (context.top().nextChar == '$')
            {
                PullEnvVar(tokenPtr);
            }
            else
            {
                if (context.top().nextChar == '/')
                {
                    // Check for comment start.
                    int secondChar = context.top().inputStream.peek();
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
        if (startPos == context.top().inputStream.tellg())
        {
            if (isprint(context.top().nextChar))
            {
                ThrowException(
                    mk::format(LE_I18N("Invalid character '%c' in argument."),
                               (char)context.top().nextChar)
                );
            }
            else
            {
                ThrowException(LE_I18N("Invalid (non-printable) character in argument."));
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
    if (context.top().nextChar == '"')
    {
        PullQuoted(tokenPtr, '"');
    }
    else if (context.top().nextChar == '\'')
    {
        PullQuoted(tokenPtr, '\'');
    }
    else
    {
        auto startPos = context.top().inputStream.tellg();

        while (IsFilePathChar(context.top().nextChar))
        {
            if (context.top().nextChar == '$')
            {
                PullEnvVar(tokenPtr);
            }
            else
            {
                if (context.top().nextChar == '/')
                {
                    // Check for comment start.
                    int secondChar = context.top().inputStream.peek();
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
        if (startPos == context.top().inputStream.tellg())
        {
            if (isprint(context.top().nextChar))
            {
                ThrowException(
                    mk::format(LE_I18N("Invalid character '%c' in file path."),
                               (char)context.top().nextChar)
                );
            }
            else
            {
                ThrowException(LE_I18N("Invalid (non-printable) character in file path."));
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
    if (context.top().nextChar == '"')
    {
        PullQuoted(tokenPtr, '"');
    }
    else if (context.top().nextChar == '\'')
    {
        PullQuoted(tokenPtr, '\'');
    }
    else
    {
        auto startPos = context.top().inputStream.tellg();

        while (IsFileNameChar(context.top().nextChar))
        {
            if (context.top().nextChar == '$')
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
        if (startPos == context.top().inputStream.tellg())
        {
            if (isprint(context.top().nextChar))
            {
                ThrowException(
                    mk::format(LE_I18N("Invalid character '%c' in name."),
                               (char)context.top().nextChar)
                );
            }
            else
            {
                ThrowException(LE_I18N("Invalid (non-printable) character in name."));
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
    if (   islower(context.top().nextChar)
           || isupper(context.top().nextChar)
           || (context.top().nextChar == '_') )
    {
        AdvanceOneCharacter(tokenPtr);
    }
    else
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of name. "
                               "Names must start with a letter ('a'-'z' or 'A'-'Z')"
                               " or an underscore ('_')."));
    }

    while (   islower(context.top().nextChar)
              || isupper(context.top().nextChar)
              || isdigit(context.top().nextChar)
              || (context.top().nextChar == '_') )
    {
        AdvanceOneCharacter(tokenPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull a dotted name from the input file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullDottedName
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    do
    {
        PullName(tokenPtr);

        if (context.top().nextChar == '.')
        {
            AdvanceOneCharacter(tokenPtr);
        }
    }
    while (   islower(context.top().nextChar)
              || isupper(context.top().nextChar)
              || (context.top().nextChar == '_'));
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
    if (   islower(context.top().nextChar)
           || isupper(context.top().nextChar)
           || (context.top().nextChar == '_') )
    {
        AdvanceOneCharacter(tokenPtr);
    }
    else
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of group name. "
                               "Group names must start with a letter "
                               "('a'-'z' or 'A'-'Z') or an underscore ('_')."));
    }

    while (   islower(context.top().nextChar)
              || isupper(context.top().nextChar)
              || isdigit(context.top().nextChar)
              || (context.top().nextChar == '_')
              || (context.top().nextChar == '-') )
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
    auto firstChar = context.top().nextChar;

    // User names are enclosed in angle brackets (e.g., "<username>").
    if (firstChar == '<')
    {
        AdvanceOneCharacter(tokenPtr);

        while (   islower(context.top().nextChar)
                  || isupper(context.top().nextChar)
                  || isdigit(context.top().nextChar)
                  || (context.top().nextChar == '_')
                  || (context.top().nextChar == '-') )
        {
            AdvanceOneCharacter(tokenPtr);
        }

        if (context.top().nextChar != '>')
        {
            UnexpectedChar(LE_I18N("Unexpected character %s in user name.  "
                                   "Must be terminated with '>'."));
        }
        else
        {
            AdvanceOneCharacter(tokenPtr);
        }
    }
    // App names have the same rules as C programming language identifiers.
    else if (   islower(context.top().nextChar)
                || isupper(context.top().nextChar)
                || (context.top().nextChar == '_') )
    {
        AdvanceOneCharacter(tokenPtr);

        while (   islower(context.top().nextChar)
                  || isupper(context.top().nextChar)
                  || isdigit(context.top().nextChar)
                  || (context.top().nextChar == '_') )
        {
            AdvanceOneCharacter(tokenPtr);
        }
    }
    else
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of IPC agent name. "
                               "App names must start with a letter "
                               "('a'-'z' or 'A'-'Z') or an underscore ('_').  User names must be "
                               "inside angle brackets ('<username>')."));
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

    while (context.top().nextChar != quoteChar)
    {
        // Don't allow end of file or end of line characters inside the quoted string.
        if (context.top().nextChar == EOF)
        {
            ThrowException(LE_I18N("Unexpected end-of-file before end of quoted string."));
        }
        if ((context.top().nextChar == '\n') || (context.top().nextChar == '\r'))
        {
            ThrowException(LE_I18N("Unexpected end-of-line before end of quoted string."));
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
    if (context.top().nextChar == '{')
    {
        AdvanceOneCharacter(tokenPtr->text);
        hasCurlies = true;
    }

    // Pull the first character of the environment variable name.
    if (   islower(context.top().nextChar)
           || isupper(context.top().nextChar)
           || (context.top().nextChar == '_') )
    {
        AdvanceOneCharacter(tokenPtr->text);
    }
    else
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of"
                               " environment variable name.  "
                               "Must start with a letter ('a'-'z' or 'A'-'Z') or"
                               " an underscore ('_')."));
    }

    // Pull the rest of the environment variable name.
    while (   islower(context.top().nextChar)
              || isupper(context.top().nextChar)
              || isdigit(context.top().nextChar)
              || (context.top().nextChar == '_') )
    {
        AdvanceOneCharacter(tokenPtr->text);
    }

    // If there was an opening curly brace, match the closing one now.
    if (hasCurlies)
    {
        if (context.top().nextChar == '}')
        {
            AdvanceOneCharacter(tokenPtr->text);
        }
        else if (context.top().nextChar == EOF)
        {
            ThrowException(LE_I18N("Unexpected end-of-file inside environment variable name."));
        }
        else
        {
            ThrowException(
                mk::format(LE_I18N("'}' expected.  '%c' found."), (char)context.top().nextChar)
            );
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull an MD5 hash from the input file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullMd5
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    // There are always exactly 32 hexadecimal digits in an md5 sum.
    for (int i = 0; i < 32; i++)
    {
        if (   (!isdigit(context.top().nextChar))
               && (context.top().nextChar != 'a')
               && (context.top().nextChar != 'b')
               && (context.top().nextChar != 'c')
               && (context.top().nextChar != 'd')
               && (context.top().nextChar != 'e')
               && (context.top().nextChar != 'f')  )
        {
            if (IsWhitespace(context.top().nextChar))
            {
                ThrowException(LE_I18N("MD5 hash too short."));
            }

            UnexpectedChar(LE_I18N("Unexpected character %s in MD5 hash."));
        }

        AdvanceOneCharacter(tokenPtr);
    }

    // Make sure it isn't too long.
    if (   isdigit(context.top().nextChar)
           || (context.top().nextChar == 'a')
           || (context.top().nextChar == 'b')
           || (context.top().nextChar == 'c')
           || (context.top().nextChar == 'd')
           || (context.top().nextChar == 'e')
           || (context.top().nextChar == 'f')  )
    {
        ThrowException(LE_I18N("MD5 hash too long."));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull a processing directive (e.g. include, conditional) from the file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullDirective
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    // advance past the '#'
    if (context.top().nextChar == '#')
    {
        AdvanceOneCharacter(tokenPtr);
    }
    else
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of processing directive.  "
                               "Must start with '#' character."));
    }

    if (   islower(context.top().nextChar)
           || isupper(context.top().nextChar))
    {
        AdvanceOneCharacter(tokenPtr);
    }
    else
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of processing directive.  "
                               "Must start with a letter ('a'-'z' or 'A'-'Z')."));
    }

    while (   islower(context.top().nextChar)
              || isupper(context.top().nextChar))
    {
        AdvanceOneCharacter(tokenPtr);
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
    std::string charAsString;

    if (isprint(unexpectedChar))
    {
        charAsString = "'" + std::string(1, unexpectedChar) + "'";
    }
    else
    {
        charAsString = LE_I18N("<unprintable>");
    }

    return mk::format((LE_I18N("%s:%d:%d: error: ") + message).c_str(),
                      context.top().filePtr->path, lineNum, columnNum, charAsString);
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
                                                     LE_I18N("Unexpected character %s"
                                                             " at beginning of name."
                                                             " Names must start"
                                                             " with a letter ('a'-'z' or 'A'-'Z')"
                                                             " or an underscore ('_').")));
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
                                                         LE_I18N("Unexpected character %s.  "
                                                                 "Names may only contain letters"
                                                                 " ('a'-'z' or 'A'-'Z'),"
                                                                 " numbers ('0'-'9')"
                                                                 " and underscores ('_').")));
        }

        charPtr++;
    }

    // Everything looks fine.  Convert token type now.
    tokenPtr->type = parseTree::Token_t::NAME;
}

//--------------------------------------------------------------------------------------------------
/**
 * Find if an environment or build variable has been used by the lexer.
 *
 * @return Pointer to the first token in which the variable was used, or NULL if not used.
 */
//--------------------------------------------------------------------------------------------------
parseTree::Token_t *Lexer_t::FindVarUse
(
    const std::string &name
)
//--------------------------------------------------------------------------------------------------
{
    auto varUse = usedVars.find(name);

    if (varUse == usedVars.end())
    {
        // Variable not used by lexer
        return NULL;
    }
    else
    {
        return varUse->second;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Lookahead n bytes in the input stream, copying those bytes into a buffer provided.
 *
 * This does not remove the bytes from the input stream.  It just "peeks" ahead at what's waiting.
 *
 * The first byte copied into the buffer will be 'nextChar'.
 *
 * If the end of the file is reached before the buffer is full, the EOF character will be copied
 * into the buffer as the last byte.
 *
 * @return the number of bytes copied into the buffer.  Could be less than the number requested
 *         if the end of file is reached.  Will never be more than the number requested.
 **/
//--------------------------------------------------------------------------------------------------
size_t Lexer_t::Lookahead
(
    char* buffPtr,  ///< Buffer to copy bytes into.
    size_t n        ///< Number of bytes to copy into the buffer.
)
//--------------------------------------------------------------------------------------------------
{
    if (n > 0)
    {
        size_t i = 1;
        *buffPtr = context.top().nextChar;

        while ((*buffPtr != EOF) && (i < n))
        {
            buffPtr++;
            *buffPtr = context.top().inputStream.get();
            i++;
        }

        n = i;

        // Push back everything we pulled out of the input stream.
        while (i > 1)
        {
            i--;
            context.top().inputStream.putback(*buffPtr);
            buffPtr--;
        }
    }

    return n;
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
    string += context.top().nextChar;

    if (context.top().nextChar == '\n')
    {
        context.top().line++;
        context.top().column = 0;
    }
    else
    {
        context.top().column++;
    }

    context.top().nextChar = context.top().inputStream.get();

    if (context.top().inputStream.bad())
    {
        ThrowException(LE_I18N("Failed to fetch next character from file."));
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
    std::string formattedMessage;

    formattedMessage = mk::format(LE_I18N("%s:%d:%d: error: %s"),
                                  context.top().filePtr->path,
                                  context.top().line,
                                  context.top().column,
                                  message);

    throw mk::Exception_t(formattedMessage);
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
    throw mk::Exception_t(UnexpectedCharErrorMsg(context.top().nextChar,
                                                 context.top().line,
                                                 context.top().column,
                                                 message));
}



} // namespace parser
