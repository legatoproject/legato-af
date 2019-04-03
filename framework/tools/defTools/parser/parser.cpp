//--------------------------------------------------------------------------------------------------
/**
 * @file parser.cpp  Functions used by multiple parsers.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"


namespace parser
{

//--------------------------------------------------------------------------------------------------
/**
 * Parse a simple section.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::SimpleSection_t* ParseSimpleSection
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr,    ///< The token containing the section name.
    parseTree::Token_t::Type_t tokenType        ///< Type of content token to expect.
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionPtr = new parseTree::SimpleSection_t(sectionNameTokenPtr);

    // Expect a ':' next.
    (void)lexer.Pull(parseTree::Token_t::COLON);

    // Expect the content token next.
    sectionPtr->AddContent(lexer.Pull(tokenType));

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a simple named item.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseSimpleNamedItem
(
    Lexer_t& lexer,
    parseTree::Token_t* nameTokenPtr,           ///< The token containing the name of the item.
    parseTree::Content_t::Type_t contentType,   ///< The named item type.
    parseTree::Token_t::Type_t tokenType        ///< Type of content token to expect.
)
//--------------------------------------------------------------------------------------------------
{
    auto itemPtr = parseTree::CreateTokenList(contentType, nameTokenPtr);

    // Expect an '=' next.
    (void)lexer.Pull(parseTree::Token_t::EQUALS);

    // Expect the content token next.
    itemPtr->AddContent(lexer.Pull(tokenType));

    return itemPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a section containing a list of tokens of the same type inside curly braces.
 *
 * This includes "cflags:", "cxxflags:", "ldflags:", "sources:", "groups", and more.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseTokenListSection
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr,///< The token containing the section name.
    parseTree::Token_t::Type_t tokenType    ///< Type of content token to expect.
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionPtr = new parseTree::TokenListSection_t(sectionNameTokenPtr);

    // Expect a ':' next.
    (void)lexer.Pull(parseTree::Token_t::COLON);

    // Expect a '{' next.
    (void)lexer.Pull(parseTree::Token_t::OPEN_CURLY);

    // Until we find a closing '}', keep calling the provided content parser function to parse
    // the next content item.
    while (!lexer.IsMatch(parseTree::Token_t::CLOSE_CURLY))
    {
        if (lexer.IsMatch(parseTree::Token_t::END_OF_FILE))
        {
            lexer.ThrowException(
                mk::format(LE_I18N("Unexpected end-of-file before end of %s section.\n"
                                   "%s: note: Section starts here."),
                           sectionNameTokenPtr->text,
                           sectionNameTokenPtr->GetLocation())
            );
        }

        sectionPtr->AddContent(lexer.Pull(tokenType));
    }

    // Pull out the '}' and make that the last token in the section.
    sectionPtr->lastTokenPtr = lexer.Pull(parseTree::Token_t::CLOSE_CURLY);

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a section which is a simple section or containing a list of tokens of the same type inside
 * curly braces.
 *
 * This includes only "preBuilt:". Simple section support for "preBuilt:" will be deprecated
 * and is supported now for backward compatibility.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseSimpleOrTokenListSection
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr,///< The token containing the section name.
    parseTree::Token_t::Type_t tokenType    ///< Type of content token to expect.
)
//--------------------------------------------------------------------------------------------------
{
    // Expect a ':' next.
    (void)lexer.Pull(parseTree::Token_t::COLON);

    // Check for '{' next.
    if (!lexer.IsMatch(parseTree::Token_t::OPEN_CURLY))
    {
        // 'preBuilt:' must use '{}'. Support without '{}' will be deprecated in a future release.
        // Add support now for backward comptability.
        sectionNameTokenPtr->PrintWarning(
            mk::format(LE_I18N("Use '{}' with '%s' section. Support without '{}' is deprecated."),
                               sectionNameTokenPtr->text)
        );

        // Section is simple if there is no '{'
        auto sectionPtr = new parseTree::SimpleSection_t(sectionNameTokenPtr);

        // Expect the content token next.
        sectionPtr->AddContent(lexer.Pull(tokenType));

        return sectionPtr;
    }

    auto sectionPtr = new parseTree::TokenListSection_t(sectionNameTokenPtr);

    // Expect a '{' next.
    (void)lexer.Pull(parseTree::Token_t::OPEN_CURLY);

    // Until we find a closing '}', keep calling the provided content parser function to parse
    // the next content item.
    while (!lexer.IsMatch(parseTree::Token_t::CLOSE_CURLY))
    {
        if (lexer.IsMatch(parseTree::Token_t::END_OF_FILE))
        {
            lexer.ThrowException(
                mk::format(LE_I18N("Unexpected end-of-file before end of %s section.\n"
                                   "%s: note: Section starts here."),
                                   sectionNameTokenPtr->text,
                                   sectionNameTokenPtr->GetLocation())
            );
        }

        sectionPtr->AddContent(lexer.Pull(tokenType));
    }

    // Pull out the '}' and make that the last token in the section.
    sectionPtr->lastTokenPtr = lexer.Pull(parseTree::Token_t::CLOSE_CURLY);

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a compound named item containing a list of tokens of the same type.
 *
 * This includes executables inside the "executables:" section.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseTokenListNamedItem
(
    Lexer_t& lexer,
    parseTree::Token_t* nameTokenPtr,           ///< The token containing the name of the item.
    parseTree::Content_t::Type_t contentType,   ///< The named item type.
    parseTree::Token_t::Type_t tokenType    ///< Type of content token to expect.
)
//--------------------------------------------------------------------------------------------------
{
    auto itemPtr = parseTree::CreateTokenList(contentType, nameTokenPtr);

    // Expect an '=' next.
    (void)lexer.Pull(parseTree::Token_t::EQUALS);

    // Expect a '(' next.
    (void)lexer.Pull(parseTree::Token_t::OPEN_PARENTHESIS);

    // Until we find a closing ')', keep pulling out content tokens and skipping whitespace and
    // comments after each.
    while (!lexer.IsMatch(parseTree::Token_t::CLOSE_PARENTHESIS))
    {
        if (lexer.IsMatch(parseTree::Token_t::END_OF_FILE))
        {
            lexer.ThrowException(
                mk::format(LE_I18N("Unexpected end-of-file before end of %s named '%s'.\n"
                                   "%s: note: %s starts here."),
                           itemPtr->TypeName(), nameTokenPtr->text,
                           nameTokenPtr->GetLocation(), itemPtr->TypeName())
            );
        }

        itemPtr->AddContent(lexer.Pull(tokenType));
    }

    // Pull out the ')' and make that the last token in the section.
    itemPtr->lastTokenPtr = lexer.Pull(parseTree::Token_t::CLOSE_PARENTHESIS);

    return itemPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a complex section (i.e., a section whose content contains compound items, not just tokens).
 *
 * Takes a pointer to a function that gets called to parse each item found in the section.
 * This item parser function returns a pointer to a parsed item to be added to the section's
 * content list, or throws an exception on error.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::CompoundItemList_t* ParseComplexSection
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr,///< The token containing the section name.
    std::function<parseTree::CompoundItem_t* (Lexer_t& lexer)> contentParserFunc
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionPtr = new parseTree::ComplexSection_t(sectionNameTokenPtr);

    // Expect a ':' next.
    (void)lexer.Pull(parseTree::Token_t::COLON);

    // Expect a '{' next.
    (void)lexer.Pull(parseTree::Token_t::OPEN_CURLY);

    // Until we find a closing '}', keep calling the provided content parser function to parse
    // the next content item.
    while (!lexer.IsMatch(parseTree::Token_t::CLOSE_CURLY))
    {
        if (lexer.IsMatch(parseTree::Token_t::END_OF_FILE))
        {
            std::stringstream msg;
            lexer.ThrowException(
                mk::format(LE_I18N("Unexpected end-of-file before end of %s section.\n"
                                   "%s: note: Section starts here."),
                           sectionNameTokenPtr->text,
                           sectionNameTokenPtr->GetLocation())
            );
        }

        sectionPtr->AddContent(contentParserFunc(lexer));
    }

    // Pull out the '}' and make that the last token in the section.
    sectionPtr->lastTokenPtr = lexer.Pull(parseTree::Token_t::CLOSE_CURLY);

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a named complex section.  That is a named section that contains compound items.
 *
 * Takes a pointer to a function that gets called to parse each item found in the section.
 * This item parser function returns a pointer to a parsed item to be added to the section's
 * content list, or throws an exception on error.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::CompoundItemList_t* ParseNamedComplexSection
(
    Lexer_t& lexer,
    parseTree::CompoundItemList_t* sectionPtr,  ///< The item containing the section name.
    std::function<parseTree::CompoundItem_t* (Lexer_t& lexer)> contentParserFunc
)
//--------------------------------------------------------------------------------------------------
{
    // Expect a '=' next.
    (void)lexer.Pull(parseTree::Token_t::EQUALS);

    // Expect a '{' next.
    (void)lexer.Pull(parseTree::Token_t::OPEN_CURLY);

    // Until we find a closing '}', keep calling the provided content parser function to parse
    // the next content item.
    while (!lexer.IsMatch(parseTree::Token_t::CLOSE_CURLY))
    {
        if (lexer.IsMatch(parseTree::Token_t::END_OF_FILE))
        {
            lexer.ThrowException(
                mk::format(LE_I18N("Unexpected end-of-file before end of %s section.\n"
                                   "%s: note: Section starts here."),
                           sectionPtr->firstTokenPtr->text,
                           sectionPtr->firstTokenPtr->GetLocation())
            );
        }

        sectionPtr->AddContent(contentParserFunc(lexer));
    }

    // Pull out the '}' and make that the last token in the section.
    sectionPtr->lastTokenPtr = lexer.Pull(parseTree::Token_t::CLOSE_CURLY);

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a compound section containing a list of simple named items whose content are all the same
 * type of token.
 *
 * This includes pools inside a "pools:" section.
 *
 * @return a pointer to the parse tree object created for this section.
 */
//--------------------------------------------------------------------------------------------------
parseTree::CompoundItemList_t* ParseSimpleNamedItemListSection
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr,///< The token containing the section name.
    parseTree::Content_t::Type_t namedItemType, ///< Type of named items to expect.
    parseTree::Token_t::Type_t tokenType        ///< Type of token to expect in the named items.
)
//--------------------------------------------------------------------------------------------------
{
    auto namedItemParser = [namedItemType, tokenType](Lexer_t& lexer)
        {
            return ParseSimpleNamedItem(lexer,
                                        lexer.Pull(parseTree::Token_t::NAME),
                                        namedItemType,
                                        tokenType);
        };

    return ParseComplexSection(lexer, sectionNameTokenPtr, namedItemParser);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a file.  Calls a provided section parser function for each section found in the file.
 *
 * The section parser function must return a pointer to a section (CompoundItem_t, which will be
 * added to the list of sections in the DefFile_t), or throw an exception on error.
 *
 * @throw mk::Exception_t if an error is encountered.
 */
//--------------------------------------------------------------------------------------------------
void ParseFile
(
    parseTree::DefFile_t* defFilePtr,   ///< Pointer to the definition file object to populate.
    bool beVerbose,                 ///< true if progress messages should be printed.
    parseTree::CompoundItem_t* (*sectionParserFunc)(Lexer_t& lexer) ///< Section parser function.
)
//--------------------------------------------------------------------------------------------------
{
    if (beVerbose)
    {
        std::cout << mk::format(LE_I18N("Parsing file: '%s'."), defFilePtr->path)
                  << std::endl;
    }

    // Create a Lexer for this file.
    Lexer_t lexer(defFilePtr);
    lexer.beVerbose = beVerbose;

    // Expect a list of any combination of sections.
    while (!lexer.IsMatch(parseTree::Token_t::END_OF_FILE))
    {
        if (lexer.IsMatch(parseTree::Token_t::NAME))
        {
            defFilePtr->sections.push_back(sectionParserFunc(lexer));
        }
        else
        {
            lexer.UnexpectedChar(LE_I18N("Unexpected character %s"));
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a bundled file or directory item from inside a "bundles:" section's "file" or "dir"
 * subsection.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::TokenList_t* ParseBundledItem
(
    Lexer_t& lexer,
    parseTree::Content_t::Type_t type
)
//--------------------------------------------------------------------------------------------------
{
    // Accept an optional set of permissions.
    parseTree::Token_t* permissionsPtr = NULL;
    if (lexer.IsMatch(parseTree::Token_t::FILE_PERMISSIONS))
    {
        permissionsPtr = lexer.Pull(parseTree::Token_t::FILE_PERMISSIONS);
    }

    // Expect a build host file system path followed by a target host file system path.
    parseTree::Token_t* buildHostPathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);
    parseTree::Token_t* targetPathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);

    // Create a new bundled item.
    parseTree::Token_t* firstPtr = (permissionsPtr != NULL ? permissionsPtr : buildHostPathPtr);
    auto bundledItemPtr = parseTree::CreateTokenList(type, firstPtr);

    // Add its contents.
    if (permissionsPtr != NULL)
    {
        bundledItemPtr->AddContent(permissionsPtr);
    }
    bundledItemPtr->AddContent(buildHostPathPtr);
    bundledItemPtr->AddContent(targetPathPtr);

    return bundledItemPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a subsection inside a "bundles:" section.
 *
 * @return Pointer to the subsection.
 */
//--------------------------------------------------------------------------------------------------
parseTree::CompoundItemList_t* ParseBundlesSubsection
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    // Expect the subsection name as the first token.
    parseTree::Token_t* nameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    // Figure out which type of content item to parse depending on what subsection it is.
    parseTree::Content_t::Type_t itemType;
    if (nameTokenPtr->text == "file" ||
        nameTokenPtr->text == "binary")
    {
        itemType = parseTree::Content_t::BUNDLED_FILE;
    }
    else if (nameTokenPtr->text == "dir")
    {
        itemType = parseTree::Content_t::BUNDLED_DIR;
    }
    else
    {
        lexer.ThrowException(
            mk::format(LE_I18N("Unexpected subsection name '%s' in 'bundles' section."),
                       nameTokenPtr->text)
        );
    }

    // Create a closure that knows which type of item should be parsed and how to parse it.
    auto itemParser = [itemType](Lexer_t& lexer)
        {
            return ParseBundledItem(lexer, itemType);
        };

    // Parse the subsection.
    return ParseComplexSection(lexer, nameTokenPtr, itemParser);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses an entry in the required "kernelModule(s):" section in a definition file.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
parseTree::RequiredModule_t* ParseRequiredModule
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    // Get the module file path token.
    parseTree::Token_t* moduleFilePathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);

    // Create parse tree node for this.
    auto modulePtr = new parseTree::RequiredModule_t(moduleFilePathPtr);

    // Add its contents.
    modulePtr->AddContent(moduleFilePathPtr);

    // Accept "optional" option.
    while (lexer.IsMatch(parseTree::Token_t::OPTIONAL_OPEN_SQUARE))
    {
        modulePtr->AddContent(lexer.Pull(parseTree::Token_t::OPTIONAL_OPEN_SQUARE));
    }

    return modulePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a required file/directory or device item from inside a "requires:" section's "file", "dir"
 * or "device" subsection.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::TokenList_t* ParseRequiredItem
(
    Lexer_t& lexer,
    parseTree::Content_t::Type_t type
)
//--------------------------------------------------------------------------------------------------
{
    // Accept an optional set of permissions.
    parseTree::Token_t* permissionsPtr = NULL;
    if (lexer.IsMatch(parseTree::Token_t::FILE_PERMISSIONS))
    {
        permissionsPtr = lexer.Pull(parseTree::Token_t::FILE_PERMISSIONS);
    }

    // Expect a source file system path followed by a destination file system path.
    parseTree::Token_t* srcPathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);
    parseTree::Token_t* destPathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);

    // Create a new required item.
    parseTree::Token_t* firstPtr = (permissionsPtr != NULL ? permissionsPtr : srcPathPtr);
    auto requiredItemPtr = parseTree::CreateTokenList(type, firstPtr);

    // Add its contents.
    if (permissionsPtr != NULL)
    {
        requiredItemPtr->AddContent(permissionsPtr);
    }
    requiredItemPtr->AddContent(srcPathPtr);
    requiredItemPtr->AddContent(destPathPtr);

    return requiredItemPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a single item from inside a "file:" subsection inside a "requires" subsection.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseRequiredFile
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    return ParseRequiredItem(lexer, parseTree::Content_t::REQUIRED_FILE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a single item from inside a "dir:" subsection inside a "requires" subsection.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseRequiredDir
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    return ParseRequiredItem(lexer, parseTree::Content_t::REQUIRED_DIR);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a single item from inside a "device:" subsection inside a "requires:" subsection.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseRequiredDevice
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    return ParseRequiredItem(lexer, parseTree::Content_t::REQUIRED_DEVICE);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a 'faultAction:' subsection.
 *
 * @return A pointer to the parse tree object created for the subsection.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseFaultAction
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr ///< The token containing the section name.
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionPtr = ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::NAME);

    // Double check that the name contains valid content.
    // TODO: Consider moving this to the lexer and creating a new FAULT_ACTION token.
    const std::string& content = sectionPtr->lastTokenPtr->text;

    if (   (content != "ignore")
        && (content != "restart")
        && (content != "restartApp")
        && (content != "stopApp")
        && (content != "reboot") )
    {
        lexer.ThrowException(
            mk::format(LE_I18N("Invalid fault action '%s'. Must be one of 'ignore',"
                               " 'restart', 'restartApp', 'stopApp', or 'reboot'."),
                       content)
        );
    }

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a section containing a scheduling priority.
 *
 * @return A pointer to the parse tree object created for the subsection.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParsePriority
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr ///< The token containing the section name.
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionPtr = ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::NAME);

    // Make sure the priority name is valid.
    // TODO: Consider creating a new THREAD_PRIORITY token for this.
    const std::string& content = sectionPtr->lastTokenPtr->text;

    if (   (content != "idle")
        && (content != "low")
        && (content != "medium")
        && (content != "high") )
    {
        if (   (content[0] == 'r')
            && (content[1] == 't')
            && isdigit(content[2])
            && (   (content[3] == '\0')
                || (   isdigit(content[3])
                    && (content[4] == '\0')
                    )
                )
            )
        {
            auto numberString = content.substr(2);

            std::stringstream numberStream(numberString);

            unsigned int number;

            numberStream >> number;

            if ((number < 1) || (number > 32))
            {
                lexer.ThrowException(
                    mk::format(LE_I18N("Real-time priority level %s out of range."
                                       "  Must be in the range 'rt1' through 'rt32'."), content)
                );
            }
        }
        else
        {
            lexer.ThrowException(
                mk::format(LE_I18N("Invalid priority '%s'."), content)
            );
        }
    }

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a 'watchdogAction:' subsection.
 *
 * @return A pointer to the parse tree object created for the subsection.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseWatchdogAction
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr ///< The token containing the section name.
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionPtr = ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::NAME);

    // Make sure the watchdog action is valid.
    // TODO: Consider creating a WATCHDOG_ACTION token for this.
    const std::string& content = sectionPtr->lastTokenPtr->text;

    if (   (content != "ignore")
        && (content != "restart")
        && (content != "restartApp")
        && (content != "stop")
        && (content != "stopApp")
        && (content != "reboot") )
    {
        lexer.ThrowException(
            mk::format(LE_I18N("Invalid watchdog action '%s'. Must be one of 'ignore',"
                               " 'restart', 'restartApp', 'stop', 'stopApp', or 'reboot'."),
                       content)
        );
    }

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a 'watchdogTimeout:' subsection.
 *
 * @return A pointer to the parse tree object created for the subsection.
 */
//--------------------------------------------------------------------------------------------------
parseTree::TokenList_t* ParseWatchdogTimeout
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr ///< The token containing the section name.
)
//--------------------------------------------------------------------------------------------------
{
    // NOTE: This simple section is different from others that always contain the same type
    //       of token because it can contain either an INTEGER or the NAME "never".

    auto sectionPtr = new parseTree::SimpleSection_t(sectionNameTokenPtr);

    // Expect a ':' next.
    (void)lexer.Pull(parseTree::Token_t::COLON);

    // Expect the content token next.  It could be the word "never" or an integer (number of ms).
    parseTree::Token_t* tokenPtr;
    if (lexer.IsMatch(parseTree::Token_t::NAME))
    {
        tokenPtr = lexer.Pull(parseTree::Token_t::NAME);

        if (tokenPtr->text != "never")
        {
            lexer.ThrowException(
                mk::format(LE_I18N("Invalid watchdog timeout value '%s'. Must be"
                                   " an integer or the word 'never'."), tokenPtr->text)
            );
        }
    }
    else if (lexer.IsMatch(parseTree::Token_t::INTEGER))
    {
        tokenPtr = lexer.Pull(parseTree::Token_t::INTEGER);
    }
    else
    {
        lexer.ThrowException(
            LE_I18N("Invalid watchdog timeout. Must be an integer or the word 'never'.")
        );
    }

    sectionPtr->AddContent(tokenPtr);

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check the section name passed in its singular form.
 *
 * @return true if the section name matches either its singular or plural form ending with a 's'.
 */
//--------------------------------------------------------------------------------------------------
bool IsNameSingularPlural
(
    std::string sectionName,
    std::string singularName
)
//--------------------------------------------------------------------------------------------------
{
    std::string pluralName = singularName + 's';

    if (sectionName == singularName || sectionName == pluralName)
    {
        return true;
    }

    return false;
}


} // namespace parser
