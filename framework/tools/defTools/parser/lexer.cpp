//--------------------------------------------------------------------------------------------------
/**
 * @file lexer.cpp Lexical Analyzer (Lexer) for def files.
 *
 * @warning Don't use isalpha() or isalnum() in this file, because their results are
 locale-dependent.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"


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
    column(0),
    ifNestDepth(0)
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

    // Read in the first characters.
    Buffer(2);

    if (inputStream.bad())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to read from file '%s'."), filePtr->path)
        );
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Ensure at least n elements are present in the lookahead character buffer
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::LexerContext_t::Buffer
(
    size_t n
)
{
    while (inputStream.good() && (nextChars.size() < n))
    {
        nextChars.push_back(inputStream.get());
    }
}


void Lexer_t::LexerContext_t::setCurPos()
{
    curPos = inputStream.tellg();
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
            return (context.top().nextChars[0] == EOF);

        case parseTree::Token_t::OPEN_CURLY:
            return (context.top().nextChars[0] == '{');

        case parseTree::Token_t::CLOSE_CURLY:
            return (context.top().nextChars[0] == '}');

        case parseTree::Token_t::OPEN_PARENTHESIS:
            return (context.top().nextChars[0] == '(');

        case parseTree::Token_t::CLOSE_PARENTHESIS:
            return (context.top().nextChars[0] == ')');

        case parseTree::Token_t::COLON:
            return (context.top().nextChars[0] == ':');

        case parseTree::Token_t::EQUALS:
            return (context.top().nextChars[0] == '=');

        case parseTree::Token_t::DOT:
            return (context.top().nextChars[0] == '.');

        case parseTree::Token_t::STAR:
            return (context.top().nextChars[0] == '*');

        case parseTree::Token_t::ARROW:
            return ((context.top().nextChars[0] == '-') && (context.top().nextChars[1] == '>'));

        case parseTree::Token_t::WHITESPACE:
            return IsWhitespace(context.top().nextChars[0]);

        case parseTree::Token_t::COMMENT:
            if (context.top().nextChars[0] == '/')
            {
                int secondChar = context.top().nextChars[1];
                return ((secondChar == '/') || (secondChar == '*'));
            }
            else
            {
                return false;
            }

        case parseTree::Token_t::FILE_PERMISSIONS:
        case parseTree::Token_t::SERVER_IPC_OPTION:
        case parseTree::Token_t::CLIENT_IPC_OPTION:
        case parseTree::Token_t::OPTIONAL_OPEN_SQUARE:
        case parseTree::Token_t::PROVIDE_HEADER_OPTION:
            return (context.top().nextChars[0] == '[');

        case parseTree::Token_t::ARG:
            // Can be anything in a FILE_PATH, plus the equals sign (=).
            if (context.top().nextChars[0] == '=')
            {
                return true;
            }
            // *** FALL THROUGH ***

        case parseTree::Token_t::FILE_PATH:
            // Can be anything in a FILE_NAME, plus the forward slash (/).
            // If it starts with a slash, it could be a comment or a file path.
            if (context.top().nextChars[0] == '/')
            {
                // If it's not a comment, then it's a file path.
                int secondChar = context.top().nextChars[1];
                return ((secondChar != '/') && (secondChar != '*'));
            }
            // *** FALL THROUGH ***

        case parseTree::Token_t::FILE_NAME:
            return (   IsFileNameChar(context.top().nextChars[0])
                       || (context.top().nextChars[0] == '\'')   // Could be in single-quotes.
                       || (context.top().nextChars[0] == '"') ); // Could be in quotes.

        case parseTree::Token_t::IPC_AGENT:
            // Can start with the same characters as a NAME or GROUP_NAME, plus '<'.
            if (context.top().nextChars[0] == '<')
            {
                return true;
            }
            // *** FALL THROUGH ***

        case parseTree::Token_t::NAME:
        case parseTree::Token_t::GROUP_NAME:
        case parseTree::Token_t::DOTTED_NAME:
            return (   islower(context.top().nextChars[0])
                       || isupper(context.top().nextChars[0])
                       || (context.top().nextChars[0] == '_') );

        case parseTree::Token_t::INTEGER:
            return (isdigit(context.top().nextChars[0]));

        case parseTree::Token_t::SIGNED_INTEGER:
            return (   (context.top().nextChars[0] == '+')
                       || (context.top().nextChars[0] == '-')
                       || isdigit(context.top().nextChars[0]));

        case parseTree::Token_t::BOOLEAN:
            return IsMatchBoolean();

        case parseTree::Token_t::FLOAT:
            throw mk::Exception_t(LE_I18N("Internal error: FLOAT lookahead not implemented."));

        case parseTree::Token_t::STRING:
            throw mk::Exception_t(LE_I18N("Internal error: STRING lookahead not implemented."));

        case parseTree::Token_t::MD5_HASH:
            // expect to find at least two hexadecimal characters
            return (isxdigit(context.top().nextChars[0])
                    && isxdigit(context.top().nextChars[1]));

        case parseTree::Token_t::DIRECTIVE:
            return context.top().nextChars[0] == '#';
    }

    throw mk::Exception_t(LE_I18N("Internal error: IsMatch(): Invalid token type requested."));
}


//--------------------------------------------------------------------------------------------------
/**
 * Skip over a single token, regardless of the type.
 *
 * This only skips non-whitespace tokens.  To skip whitespace or comments, use NextToken().
 *
 * @note This function does minimal validation on the pulled token, so may allow invalid tokens.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::SkipToNextDirective
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Still need to add skipped text to list of tokens.  Treat it as a single comment for now.
    // Might be nice to actually tokenize but add as "inactive" or similar, but for now
    // can't tokenize without also parsing (see LE-6223).
    parseTree::Token_t* phonyTokenPtr = new parseTree::Token_t(parseTree::Token_t::COMMENT,
                                                               context.top().filePtr,
                                                               context.top().line,
                                                               context.top().column,
                                                               context.top().curPos);

    while (true)
    {
        switch (context.top().nextChars[0])
        {
            case '#':
                // Found a directive
                return;

            case '/':
            {
                int secondChar = context.top().nextChars[1];
                if (secondChar == '/' ||
                    secondChar == '*')
                {
                    // Found a comment.  Pull the whole thing as it may contain embedded directives
                    // that should be ignored.
                    PullComment(phonyTokenPtr);
                }
                else
                {
                    AdvanceOneCharacter(phonyTokenPtr);
                }

                break;
            }

            case '"':
            case '\'':
                // Found a quoted string.  Pull the whole thing as it may contain embedded
                // directives that should be ignored.
                PullQuoted(phonyTokenPtr, context.top().nextChars[0]);
                break;

            default:
                AdvanceOneCharacter(phonyTokenPtr);
                break;
        }
    }
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
                                                          context.top().line, context.top().column,
                                                          context.top().curPos);

    switch (type)
    {
        case parseTree::Token_t::END_OF_FILE:

            if (context.top().nextChars[0] != EOF)
            {
                ThrowException(
                    mk::format(LE_I18N("Expected end-of-file, but found '%c'."),
                               (char)context.top().nextChars[0])
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

        case parseTree::Token_t::OPTIONAL_OPEN_SQUARE:

            PullOptional(tokenPtr);
            break;

        case parseTree::Token_t::PROVIDE_HEADER_OPTION:

            PullProvideHeader(tokenPtr);
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
 * Pull a token or directive from the file being parsed, moving the point to the start of the next
 * token or directive.
 */
//--------------------------------------------------------------------------------------------------
parseTree::Token_t* Lexer_t::PullTokenOrDirective
(
    parseTree::Token_t::Type_t type
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Token_t* tokenPtr = PullRaw(type);

    NextTokenOrDirective();

    return tokenPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Move to the start of the next interesting token in the input stream.
 *
 * Interesting is currently non-whitespace, non-comment, non-directive.  Any uninteresting tokens
 * are still added to the token list for the file, but not returned by Lexer_t::Pull().  This
 * function will expand directives in place.
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
        NextTokenOrDirective();

        if (IsMatch(parseTree::Token_t::DIRECTIVE))
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
 * Move to the start of the next interesting token or directive in the input stream.
 *
 * Interesting is currently non-whitespace, non-comment, non-directive.  Any uninteresting tokens
 * are still added to the token list for the file, but not returned by Lexer_t::Pull().
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::NextTokenOrDirective
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
        else
        {
            return;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Reset lexer back to state immedately after the given token.
 *
 * No pointers can be retained to tokens which are reset, as these tokens will be deleted.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::ResetTo(parseTree::Token_t* resetTokenPtr)
{
    // Start at the last token from the file and work backwards
    auto lastTokenPtr = resetTokenPtr->nextPtr;
    while (lastTokenPtr->nextPtr != NULL)
    {
        lastTokenPtr = lastTokenPtr->nextPtr;
    }

    // But don't rewind past whitespace or comments
    auto firstTokenPtr = resetTokenPtr;
    while (firstTokenPtr->nextPtr->type == parseTree::Token_t::COMMENT ||
           firstTokenPtr->nextPtr->type == parseTree::Token_t::WHITESPACE)
    {
        firstTokenPtr = firstTokenPtr->nextPtr;
    }

    while (lastTokenPtr != firstTokenPtr)
    {
        auto prevTokenPtr = lastTokenPtr->prevPtr;

        // Shouldn't backtrack into another file
        if (lastTokenPtr->filePtr != context.top().filePtr)
        {
            resetTokenPtr->ThrowException(LE_I18N("Internal Error: Attempting to reset lookahead "
                                                  "across file boundary"));
        }

        // Re-add the text to the buffer
        context.top().nextChars.insert(context.top().nextChars.begin(),
                                       lastTokenPtr->text.begin(),
                                       lastTokenPtr->text.end());

        // Reset column & line numbers
        context.top().line = lastTokenPtr->line;
        context.top().column = lastTokenPtr->column;

        // And delete this token and remove it from the token list
        delete lastTokenPtr;
        prevTokenPtr->nextPtr = NULL;
        lastTokenPtr = prevTokenPtr;
    }

    // Finally advance to the next token
    NextToken();
}

//--------------------------------------------------------------------------------------------------
/**
 * Process a single directive.
 *
 * The directives supported by mk* tools are:
 *   - #include "file": Include another file in this one.
 *   - #if, #elif, #else and #endif: Conditionally process sections of a file.
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
    else if (directivePtr->text == std::string("#if"))
    {
        ProcessIfDirective();
    }
    else if (directivePtr->text == std::string("#elif"))
    {
        ProcessElifDirective();
    }
    else if (directivePtr->text == std::string("#else"))
    {
        ProcessElseDirective();
    }
    else if (directivePtr->text == std::string("#endif"))
    {
        ProcessEndifDirective();
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

    auto filePath = path::Unquote(DoSubstitution(includePathPtr, &substitutedVars));

    MarkVarsUsed(substitutedVars, includePathPtr);

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
 * Process an if directive.
 *
 * This skips to the start of the active section of the conditional block.  A conditional block
 * looks like:
 *
 *     #if <condition>                // Initial #if statement
 *         // #if block
 *         // statements ...
 *     #elif <condition>              // Zero or more
 *         // #elif block             // #elif blocks
 *         // statements ...
 *     #else                          // Optional
 *         // #else block             // #else block
 *         // statements
 *     #endif                         // Final #endif
 *
 *  - Skip nothing if condition on #if block is true, otherwise
 *  - Skip to start of first #elif block whose condition evaluates to true, otherwise
 *  - Skip to start of #else block, if one exists, finally
 *  - Skip to statement after #endif if there's no #else block.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::ProcessIfDirective
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    bool haveElif;
    bool skip;

    // Now inside the if statement, so increase the nesting depth.
    context.top().ifNestDepth++;

    do
    {
        // If the expression is false, skip this section.
        skip = !PullAndEvalBoolExpression();

        if (skip)
        {
            parseTree::Token_t *nextToken;

            nextToken = SkipConditional(true /*allowElse*/,
                                        false /*don't skipElse*/);

            if (nextToken->text == "#endif")
            {
                // Hit the end of the conditional without any active sections, so just mark as
                // left the conditional.
                context.top().ifNestDepth--;

                return;
            }

            haveElif = (nextToken->text == "#elif");
        }
        else
        {
            haveElif = false;
        }
    } while (haveElif);
}

//--------------------------------------------------------------------------------------------------
/**
 * Process an else directive.
 *
 * Since #if/#elif processing skips over any inactive #else blocks, this will only be called
 * if this marks the end of an active #if/#elif block.  Skip straight to final #endif.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::ProcessElseDirective
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (context.top().ifNestDepth > 0)
    {
        (void)SkipConditional(false /*don't allowElse*/,
                              false /*don't skipElse*/);
    }
    else
    {
        ThrowException("Unexpected '#else' found.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Process an elif directive.
 *
 * Since #if/#elif processing skips over any inactive #elif blocks, this will only be called
 * if this marks the end of an active #if/#elif block.  Skip straight to final #endif.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::ProcessElifDirective
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (context.top().ifNestDepth > 0)
    {
        (void)SkipConditional(true /*allowElse*/,
                              true /*skipElse*/);
    }
    else
    {
        ThrowException("Unexpected '#elif' found.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Process an endif directive.
 *
 * Since #if/#elif processing skips over any inactive #elif blocks, this will only be called
 * if this marks the end of an active #if/#elif block.  Not really important for #endif though --
 * always just decrement the nesting depth.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::ProcessEndifDirective
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (context.top().ifNestDepth > 0)
    {
        --(context.top().ifNestDepth);
        return;
    }
    else
    {
        ThrowException("Unexpected '#endif' found.");
    }
}

//

//--------------------------------------------------------------------------------------------------
/**
 * Skip until the end of a conditional section.
 *
 * There are three cases where a section is skipped:
 *
 * Case 1: hit false expression; skip to start of next condition.
 *
 *     #if <false expression>  //<-- currently here
 *         // statements
 *     #elif //<-- skip to here, returning this #elif token
 *           //    (could also be #else or #endif directive)
 *
 * Case 2a: hit end of #if or #elif block terminated by an #elif; skip to #endif.
 *
 *     #if <true expression>
 *         // statements
 *     #elif //<-- currently here
 *         // statements
 *     #elif
 *         // statements
 *     #else
 *         // statements
 *     #endif //<-- skip to here, returning #endif token.  #elif and #else allowed to be skipped.
 *
 * Case 2b: Hit #if in section being skipped.  Skip straight to #endif.
 *
 *     #if <false condition>
 *         #if //<-- currently here
 *             // statements
 *         #else
 *             // statements
 *         #endif //<-- skip to here.
 *
 * Case 3: Hit end of #if or #elif block terminated by an #else.  Skip to #endif.
 *
 *     #if <true condition>
 *         // statements
 *     #else //<-- currently here
 *         // statements
 *     #endif //<-- skip to here.  #elif and #else are not allowed as an #else has already been
 *            //    seen.
 *
 * @returns the token that ends the conditional section
 */
//--------------------------------------------------------------------------------------------------
parseTree::Token_t *Lexer_t::SkipConditional
(
    bool allowElse,  ///< Is #else or #elif allowed?
    bool skipElse    ///< Skip over #else/#elif sections, straight to final #endif?
)
//--------------------------------------------------------------------------------------------------
{
    while (true)
    {
        if (IsMatch(parseTree::Token_t::DIRECTIVE))
        {
            parseTree::Token_t* directivePtr = PullTokenOrDirective(parseTree::Token_t::DIRECTIVE);

            if (directivePtr->text == std::string("#include"))
            {
                // Ignore #include directives.
            }
            else if (directivePtr->text == std::string("#if"))
            {
                // Skip contents of embedded '#if'
                SkipConditional(true, true);
            }
            else if (directivePtr->text == std::string("#else") ||
                     directivePtr->text == std::string("#elif"))
            {
                if (!allowElse)
                {
                    ThrowException("Unexpected processing directive '" + directivePtr->text + "'");
                }

                if (!skipElse)
                {
                    return directivePtr;
                }

                // If an "#else" has been found, no more else blocks are allowed.
                if (directivePtr->text == std::string("#else"))
                {
                    allowElse = false;
                }
            }
            else if (directivePtr->text == std::string("#endif"))
            {
                return directivePtr;
            }
            else
            {
                ThrowException("Unrecognized processing directive '" + directivePtr->text + "'");
            }
        }
        else if (IsMatch(parseTree::Token_t::END_OF_FILE))
        {
            ThrowException("Unexpected end-of-file inside conditional.");
        }
        else
        {
            SkipToNextDirective();
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Pull and evaluate a boolean expression which is recognizable by the preprocessor.
 */
//--------------------------------------------------------------------------------------------------
bool Lexer_t::PullAndEvalBoolExpression
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (IsMatch(parseTree::Token_t::FILE_PATH))
    {
        std::set<std::string> substitutedVarsOperand1;
        std::set<std::string> substitutedVarsOperand2;

        // A predicate or first operand of an infix operator
        parseTree::Token_t* namePtr = PullTokenOrDirective(parseTree::Token_t::FILE_PATH);

        if (IsMatch(parseTree::Token_t::EQUALS))
        {
            parseTree::Token_t* operand1Ptr = namePtr;

            // This is an infix equality operator -- check if left and right operands are equal
            (void)PullTokenOrDirective(parseTree::Token_t::EQUALS);

            parseTree::Token_t* operand2Ptr = PullTokenOrDirective(parseTree::Token_t::FILE_PATH);

            std::string operand1 = path::Unquote(DoSubstitution(operand1Ptr,
                                                                &substitutedVarsOperand1));
            MarkVarsUsed(substitutedVarsOperand1, operand1Ptr);

            std::string operand2 = path::Unquote(DoSubstitution(operand2Ptr,
                                                                &substitutedVarsOperand2));
            MarkVarsUsed(substitutedVarsOperand2, operand2Ptr);

            return operand1 == operand2;
        }
        else if (IsMatch(parseTree::Token_t::OPEN_PARENTHESIS))
        {
            bool result;

            // This is a predicate.  Evaluate the predicate.
            (void)PullTokenOrDirective(parseTree::Token_t::OPEN_PARENTHESIS);

            if (namePtr->text == std::string("file_exists"))
            {
                std::set<std::string> substitutedVars;
                parseTree::Token_t* fileNamePtr = PullTokenOrDirective(
                    parseTree::Token_t::FILE_PATH
                );
                std::string fileName = path::Unquote(DoSubstitution(fileNamePtr, &substitutedVars));
                auto curDir = path::GetContainingDir(context.top().filePtr->path);

                result = (file::FindFile(fileName, { curDir }) != "");

                MarkVarsUsed(substitutedVars, fileNamePtr);
            }
            else if (namePtr->text == std::string("dir_exists"))
            {
                std::set<std::string> substitutedVars;
                parseTree::Token_t* fileNamePtr = PullTokenOrDirective(
                    parseTree::Token_t::FILE_PATH
                );
                std::string fileName = path::Unquote(DoSubstitution(fileNamePtr, &substitutedVars));
                auto curDir = path::GetContainingDir(context.top().filePtr->path);

                result = (file::FindDirectory(fileName, { curDir }) != "");

                MarkVarsUsed(substitutedVars, fileNamePtr);
            }
            else
            {
                namePtr->ThrowException("Unknown predicate '" +
                                        namePtr->text +
                                        "'");
            }

            (void)PullTokenOrDirective(parseTree::Token_t::CLOSE_PARENTHESIS);

            return result;
        }
        else
        {
            UnexpectedChar("in conditional expression.");
        }
    }
    else
    {
        UnexpectedChar("in '#if' directive.");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Mark some variables as used by the preprocessor.
 *
 * This prevents these variables from being redefined at a later date to ensure all expansions
 * of a variable see the same value.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::MarkVarsUsed
(
    const std::set<std::string> &localUsedVars,
    parseTree::Token_t* usingTokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto const &substitutedVar: localUsedVars)
    {
        // No need to check if variable is already in usedVars list as insert will simply
        // fail in that case, leaving the original definition intact).
        usedVars.insert(std::make_pair(substitutedVar, usingTokenPtr));
    }
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
    static const char true_string[] = "true",
        false_string[] = "false",
        on_string[] = "on",
        off_string[] = "off";

    // Need at most 5 characters (for false)
    context.top().Buffer(5);
    return std::equal(true_string, true_string + sizeof(true_string) - 1,
                      context.top().nextChars.begin()) ||
        std::equal(false_string, false_string + sizeof(false_string) - 1,
                   context.top().nextChars.begin()) ||
        std::equal(on_string, on_string + sizeof(on_string) - 1,
                   context.top().nextChars.begin()) ||
        std::equal(off_string, off_string + sizeof(off_string) - 1,
                   context.top().nextChars.begin());
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
        if (context.top().nextChars[0] != *charPtr)
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
    size_t start_line = context.top().line,
        start_column = context.top().column;

    while (IsWhitespace(context.top().nextChars[0]))
    {
        AdvanceOneCharacter(tokenPtr);
    }

    if ((start_line == context.top().line) &&
        (start_column == context.top().column))
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
    if (context.top().nextChars[0] != '/')
    {
        ThrowException(LE_I18N("Expected '/' at start of comment."));
    }

    // Eat the leading '/'.
    AdvanceOneCharacter(tokenPtr);

    // Figure out which kind of comment it is.
    if (context.top().nextChars[0] == '/')
    {
        // C++ style comment, terminated by either new-line or end-of-file.
        AdvanceOneCharacter(tokenPtr);
        while ((context.top().nextChars[0] != '\n') && (context.top().nextChars[0] != EOF))
        {
            AdvanceOneCharacter(tokenPtr);
        }
    }
    else if (context.top().nextChars[0] == '*')
    {
        // C style comment, terminated by "*/" digraph.
        AdvanceOneCharacter(tokenPtr);
        for (;;)
        {
            if (context.top().nextChars[0] == '*')
            {
                AdvanceOneCharacter(tokenPtr);

                if (context.top().nextChars[0] == '/')
                {
                    AdvanceOneCharacter(tokenPtr);

                    break;
                }
            }
            else if (context.top().nextChars[0] == EOF)
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
    if (!isdigit(context.top().nextChars[0]))
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of integer."));
    }

    while (isdigit(context.top().nextChars[0]))
    {
        AdvanceOneCharacter(tokenPtr);
    }

    if (context.top().nextChars[0] == 'K')
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
    if (   (context.top().nextChars[0] == '-')
           || (context.top().nextChars[0] == '+'))
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
    if (context.top().nextChars[0] == 't')
    {
        PullConstString(tokenPtr, "true");
    }
    else if (context.top().nextChars[0] == 'f')
    {
        PullConstString(tokenPtr, "false");
    }
    else if (context.top().nextChars[0] == 'o')
    {
        AdvanceOneCharacter(tokenPtr);

        if (context.top().nextChars[0] == 'n')
        {
            AdvanceOneCharacter(tokenPtr);
        }
        else if (context.top().nextChars[0] == 'f')
        {
            AdvanceOneCharacter(tokenPtr);

            if (context.top().nextChars[0] != 'f')
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
    if (   (isdigit(context.top().nextChars[0]) == false)
           && (context.top().nextChars[0] != '+')
           && (context.top().nextChars[0] != '-'))
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of floating point value."));
    }

    AdvanceOneCharacter(tokenPtr);

    while (isdigit(context.top().nextChars[0]))
    {
        AdvanceOneCharacter(tokenPtr);
    }

    if (context.top().nextChars[0] == '.')
    {
        AdvanceOneCharacter(tokenPtr);

        while (isdigit(context.top().nextChars[0]))
        {
            AdvanceOneCharacter(tokenPtr);
        }
    }

    if (   (context.top().nextChars[0] == 'e')
           || (context.top().nextChars[0] == 'E'))
    {
        AdvanceOneCharacter(tokenPtr);

        if (   (isdigit(context.top().nextChars[0]) == false)
               && (context.top().nextChars[0] != '+')
               && (context.top().nextChars[0] != '-'))
        {
            UnexpectedChar(LE_I18N("Unexpected character %s in exponent part of"
                                   " floating point value."));
        }

        AdvanceOneCharacter(tokenPtr);

        while (isdigit(context.top().nextChars[0]))
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
    if (   (context.top().nextChars[0] == '"')
           || (context.top().nextChars[0] == '\''))
    {
        PullQuoted(tokenPtr, context.top().nextChars[0]);
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
    if (context.top().nextChars[0] != '[')
    {
        ThrowException(LE_I18N("Expected '[' at start of file permissions."));
    }

    // Eat the leading '['.
    AdvanceOneCharacter(tokenPtr);

    // Must be something between the square brackets.
    if (context.top().nextChars[0] == ']')
    {
        ThrowException(LE_I18N("Empty file permissions."));
    }

    // Continue until terminated by ']'.
    do
    {
        // Check for end-of-file or illegal character in file permissions.
        if (context.top().nextChars[0] == EOF)
        {
            ThrowException(LE_I18N("Unexpected end-of-file before end of file permissions."));
        }
        else if ((context.top().nextChars[0] != 'r') && (context.top().nextChars[0] != 'w') && (context.top().nextChars[0] != 'x'))
        {
            UnexpectedChar(LE_I18N("Unexpected character %s inside file permissions."));
        }

        AdvanceOneCharacter(tokenPtr);

    } while (context.top().nextChars[0] != ']');

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
           && (tokenPtr->text != "[async]")
           && (tokenPtr->text != "[direct]"))
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
    if (context.top().nextChars[0] != '[')
    {
        ThrowException(LE_I18N("Expected '[' at start of IPC option."));
    }

    // Eat the leading '['.
    AdvanceOneCharacter(tokenPtr);

    // Must be something between the square brackets.
    if (context.top().nextChars[0] == ']')
    {
        ThrowException(LE_I18N("Empty IPC option."));
    }

    // Continue until terminated by ']'.
    do
    {
        // Check for end-of-file or illegal character in option.
        if (context.top().nextChars[0] == EOF)
        {
            ThrowException(LE_I18N("Unexpected end-of-file before end of IPC option."));
        }
        else if ((context.top().nextChars[0] != '-') && !islower(context.top().nextChars[0]))
        {
            UnexpectedChar(LE_I18N("Unexpected character %s inside option."));
        }

        AdvanceOneCharacter(tokenPtr);

    } while (context.top().nextChars[0] != ']');

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
    if (context.top().nextChars[0] == '"')
    {
        PullQuoted(tokenPtr, '"');
    }
    else if (context.top().nextChars[0] == '\'')
    {
        PullQuoted(tokenPtr, '\'');
    }
    else
    {
        size_t start_line = context.top().line;
        size_t start_column = context.top().column;

        while (IsArgChar(context.top().nextChars[0]))
        {
            if (context.top().nextChars[0] == '$')
            {
                PullEnvVar(tokenPtr);
            }
            else
            {
                if (context.top().nextChars[0] == '/')
                {
                    // Check for comment start.
                    int secondChar = context.top().nextChars[1];
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
        if ((start_line == context.top().line) &&
            (start_column == context.top().column))
        {
            if (isprint(context.top().nextChars[0]))
            {
                ThrowException(
                    mk::format(LE_I18N("Invalid character '%c' in argument."),
                               (char)context.top().nextChars[0])
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
    if (context.top().nextChars[0] == '"')
    {
        PullQuoted(tokenPtr, '"');
    }
    else if (context.top().nextChars[0] == '\'')
    {
        PullQuoted(tokenPtr, '\'');
    }
    else
    {
        size_t start_line = context.top().line,
            start_column = context.top().column;

        while (IsFilePathChar(context.top().nextChars[0]))
        {
            if (context.top().nextChars[0] == '$')
            {
                PullEnvVar(tokenPtr);
            }
            else
            {
                if (context.top().nextChars[0] == '/')
                {
                    // Check for comment start.
                    int secondChar = context.top().nextChars[1];
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
        if (start_line == context.top().line &&
            start_column == context.top().column)
        {
            if (isprint(context.top().nextChars[0]))
            {
                ThrowException(
                    mk::format(LE_I18N("Invalid character '%c' in file path."),
                               (char)context.top().nextChars[0])
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
    if (context.top().nextChars[0] == '"')
    {
        PullQuoted(tokenPtr, '"');
    }
    else if (context.top().nextChars[0] == '\'')
    {
        PullQuoted(tokenPtr, '\'');
    }
    else
    {
        size_t start_line = context.top().line,
            start_column = context.top().column;

        while (IsFileNameChar(context.top().nextChars[0]))
        {
            if (context.top().nextChars[0] == '$')
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
        if ((start_line == context.top().line) &&
            (start_column == context.top().column))
        {
            if (isprint(context.top().nextChars[0]))
            {
                ThrowException(
                    mk::format(LE_I18N("Invalid character '%c' in name."),
                               (char)context.top().nextChars[0])
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
    if (   islower(context.top().nextChars[0])
           || isupper(context.top().nextChars[0])
           || (context.top().nextChars[0] == '_') )
    {
        AdvanceOneCharacter(tokenPtr);
    }
    else
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of name. "
                               "Names must start with a letter ('a'-'z' or 'A'-'Z')"
                               " or an underscore ('_')."));
    }

    while (   islower(context.top().nextChars[0])
              || isupper(context.top().nextChars[0])
              || isdigit(context.top().nextChars[0])
              || (context.top().nextChars[0] == '_') )
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

        if (context.top().nextChars[0] == '.')
        {
            AdvanceOneCharacter(tokenPtr);
        }
    }
    while (   islower(context.top().nextChars[0])
              || isupper(context.top().nextChars[0])
              || (context.top().nextChars[0] == '_'));
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
    if (   islower(context.top().nextChars[0])
           || isupper(context.top().nextChars[0])
           || (context.top().nextChars[0] == '_') )
    {
        AdvanceOneCharacter(tokenPtr);
    }
    else
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of group name. "
                               "Group names must start with a letter "
                               "('a'-'z' or 'A'-'Z') or an underscore ('_')."));
    }

    while (   islower(context.top().nextChars[0])
              || isupper(context.top().nextChars[0])
              || isdigit(context.top().nextChars[0])
              || (context.top().nextChars[0] == '_')
              || (context.top().nextChars[0] == '-') )
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
    auto firstChar = context.top().nextChars[0];

    // User names are enclosed in angle brackets (e.g., "<username>").
    if (firstChar == '<')
    {
        AdvanceOneCharacter(tokenPtr);

        while (   islower(context.top().nextChars[0])
                  || isupper(context.top().nextChars[0])
                  || isdigit(context.top().nextChars[0])
                  || (context.top().nextChars[0] == '_')
                  || (context.top().nextChars[0] == '-') )
        {
            AdvanceOneCharacter(tokenPtr);
        }

        if (context.top().nextChars[0] != '>')
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
    else if (   islower(context.top().nextChars[0])
                || isupper(context.top().nextChars[0])
                || (context.top().nextChars[0] == '_') )
    {
        AdvanceOneCharacter(tokenPtr);

        while (   islower(context.top().nextChars[0])
                  || isupper(context.top().nextChars[0])
                  || isdigit(context.top().nextChars[0])
                  || (context.top().nextChars[0] == '_') )
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

    while (context.top().nextChars[0] != quoteChar)
    {
        // Don't allow end of file or end of line characters inside the quoted string.
        if (context.top().nextChars[0] == EOF)
        {
            ThrowException(LE_I18N("Unexpected end-of-file before end of quoted string."));
        }
        if ((context.top().nextChars[0] == '\n') || (context.top().nextChars[0] == '\r'))
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
    if (context.top().nextChars[0] == '{')
    {
        AdvanceOneCharacter(tokenPtr->text);
        hasCurlies = true;
    }

    // Pull the first character of the environment variable name.
    if (   islower(context.top().nextChars[0])
           || isupper(context.top().nextChars[0])
           || (context.top().nextChars[0] == '_') )
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
    while (   islower(context.top().nextChars[0])
              || isupper(context.top().nextChars[0])
              || isdigit(context.top().nextChars[0])
              || (context.top().nextChars[0] == '_') )
    {
        AdvanceOneCharacter(tokenPtr->text);
    }

    // If there was an opening curly brace, match the closing one now.
    if (hasCurlies)
    {
        if (context.top().nextChars[0] == '}')
        {
            AdvanceOneCharacter(tokenPtr->text);
        }
        else if (context.top().nextChars[0] == EOF)
        {
            ThrowException(LE_I18N("Unexpected end-of-file inside environment variable name."));
        }
        else
        {
            ThrowException(
                mk::format(LE_I18N("'}' expected.  '%c' found."), (char)context.top().nextChars[0])
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
        if (   (!isdigit(context.top().nextChars[0]))
               && (context.top().nextChars[0] != 'a')
               && (context.top().nextChars[0] != 'b')
               && (context.top().nextChars[0] != 'c')
               && (context.top().nextChars[0] != 'd')
               && (context.top().nextChars[0] != 'e')
               && (context.top().nextChars[0] != 'f')  )
        {
            if (IsWhitespace(context.top().nextChars[0]))
            {
                ThrowException(LE_I18N("MD5 hash too short."));
            }

            UnexpectedChar(LE_I18N("Unexpected character %s in MD5 hash."));
        }

        AdvanceOneCharacter(tokenPtr);
    }

    // Make sure it isn't too long.
    if (   isdigit(context.top().nextChars[0])
           || (context.top().nextChars[0] == 'a')
           || (context.top().nextChars[0] == 'b')
           || (context.top().nextChars[0] == 'c')
           || (context.top().nextChars[0] == 'd')
           || (context.top().nextChars[0] == 'e')
           || (context.top().nextChars[0] == 'f')  )
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
    if (context.top().nextChars[0] == '#')
    {
        AdvanceOneCharacter(tokenPtr);
    }
    else
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of processing directive.  "
                               "Must start with '#' character."));
    }

    if (   islower(context.top().nextChars[0])
           || isupper(context.top().nextChars[0]))
    {
        AdvanceOneCharacter(tokenPtr);
    }
    else
    {
        UnexpectedChar(LE_I18N("Unexpected character %s at beginning of processing directive.  "
                               "Must start with a letter ('a'-'z' or 'A'-'Z')."));
    }

    while (   islower(context.top().nextChars[0])
              || isupper(context.top().nextChars[0]))
    {
        AdvanceOneCharacter(tokenPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull "optional" option (e.g., "[optional]") from the file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullOptional
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    PullIpcOption(tokenPtr);

    // Check that it's valid "optional".
    if (tokenPtr->text != "[optional]")
    {
        ThrowException(
            mk::format(LE_I18N("Invalid option: '%s'"), tokenPtr->text)
        );
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Pull "provide-header" option (e.g., "[provide-header]") from the file and store it in the token.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::PullProvideHeader
(
    parseTree::Token_t* tokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    PullIpcOption(tokenPtr);

    // Check that it's valid "provide-header".
    if (tokenPtr->text != "[provide-header]")
    {
        ThrowException(
            mk::format(LE_I18N("Invalid option: '%s'"), tokenPtr->text)
        );
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
 * Attempt to convert a given token to a DOTTED_NAME token.
 *
 * @throw mk::Exception_t if the token contains characters or character sequences that are not
 * allowed in a DOTTED_NAME.
 */
//--------------------------------------------------------------------------------------------------
void Lexer_t::ConvertToDottedName
(
    parseTree::Token_t* tokenPtr,
    size_t& dotCount
)
//--------------------------------------------------------------------------------------------------
{
    const char* charPtr = tokenPtr->text.c_str();

    dotCount = 0;

    if (   (!islower(charPtr[0]))
           && (!isupper(charPtr[0]))
           && (charPtr[0] != '_') )
    {
        throw mk::Exception_t(UnexpectedCharErrorMsg(charPtr[0],
                                                     tokenPtr->line,
                                                     tokenPtr->column,
                                                     LE_I18N("Unexpected character %s"
                                                             " at beginning of a dotted name."
                                                             " Dotted names must start"
                                                             " with a letter ('a'-'z' or 'A'-'Z')"
                                                             " or an underscore ('_').")));
    }

    charPtr++;

    while (*charPtr != '\0')
    {
        if (*charPtr == '.')
        {
            dotCount++;
            if (*(charPtr + 1) == '.')
            {
                throw mk::Exception_t(UnexpectedCharErrorMsg(charPtr[0],
                                                            tokenPtr->line,
                                                            tokenPtr->column,
                                                            LE_I18N("Can not have two consecutive"
                                                                    " dots, ('..') within a dotted"
                                                                    " name.")));
            }
        }
        else if (   (!islower(*charPtr))
                 && (!isupper(*charPtr))
                 && (!isdigit(*charPtr))
                 && (*charPtr != '_'))
        {
            throw mk::Exception_t(UnexpectedCharErrorMsg(charPtr[0],
                                                         tokenPtr->line,
                                                         tokenPtr->column,
                                                         LE_I18N("Unexpected character %s.  "
                                                                 "Dotted names may only contain"
                                                                 " letters ('a'-'z' or 'A'-'Z'),"
                                                                 " numbers ('0'-'9'),"
                                                                 " underscores ('_') and"
                                                                 " periods ('.').")));
        }

        charPtr++;
    }

    // Everything looks fine.  Convert token type now.
    tokenPtr->type = parseTree::Token_t::DOTTED_NAME;
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
    string += context.top().nextChars[0];

    if (context.top().nextChars[0] == '\n')
    {
        context.top().line++;
        context.top().column = 0;
    }
    else
    {
        context.top().column++;
    }

    context.top().setCurPos();

    context.top().nextChars.pop_front();
    context.top().Buffer(2);

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
    throw mk::Exception_t(UnexpectedCharErrorMsg(context.top().nextChars[0],
                                                 context.top().line,
                                                 context.top().column,
                                                 message));
}



} // namespace parser
