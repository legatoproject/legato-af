//--------------------------------------------------------------------------------------------------
/**
 * @file sdefParser.cpp  Implementation of the .sdef file parser.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"


namespace parser
{

namespace sdef
{

namespace internal
{

//--------------------------------------------------------------------------------------------------
/**
 * Sets environment variables based on the contents of a buildVars section.
 *
 * @note This must be done in the parse stage so that the variable values are available in
 *    processing directives.
 */
//--------------------------------------------------------------------------------------------------
static void SetBuildVar
(
    Lexer_t& lexer,
    const parseTree::TokenList_t* buildVarPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Make sure they're not trying to redefine one of the reserved environment variables
    // like LEGATO_ROOT
    const auto& nameTokenPtr = buildVarPtr->firstTokenPtr;
    const auto& name = nameTokenPtr->text;
    bool needReset = false;

    if (envVars::IsReserved(name))
    {
        buildVarPtr->firstTokenPtr->ThrowException(
            mk::format(LE_I18N("%s is a reserved environment variable name."), name)
        );
    }

    // Unquote and do environment variable substitution
    auto valueTokenPtr = buildVarPtr->Contents()[0];
    std::string value = path::Unquote(DoSubstitution(valueTokenPtr));

    // Do not allow redefinition of a variable which has already been used by the lexer to
    // a different value.  This would result in different definitions being used in different
    // locations.
    if (value != envVars::Get(name))
    {
        auto varUsedTokenPtr = lexer.FindVarUse(name);

        if (varUsedTokenPtr)
        {
            if ((varUsedTokenPtr->line > nameTokenPtr->line) ||
                ((varUsedTokenPtr->line == nameTokenPtr->line) &&
                 (varUsedTokenPtr->column > nameTokenPtr->column)))
            {
                needReset = true;
            }
            else
            {
                buildVarPtr->firstTokenPtr->ThrowException(
                    mk::format(LE_I18N("Cannot set value of %s; it has already been used in a "
                                       "processing directive.\n"
                                       "%s: note: First used here."),
                               name, varUsedTokenPtr->GetLocation())
                );
            }
        }
    }

    // Update the process environment
    envVars::Set(name, value);

    // And re-parse lookahead, if needed
    if (needReset)
    {
        lexer.ResetTo(valueTokenPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Parses the contents of a "preloaded:" section in an app's override list.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseAppPreloadedSection
(
    Lexer_t& lexer,
    parseTree::Token_t* sectionNameTokenPtr     ///< The token containing the section name.
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionPtr = new parseTree::SimpleSection_t(sectionNameTokenPtr);

    // Expect a ':' next.
    (void)lexer.Pull(parseTree::Token_t::COLON);

    // Expect the content token next.
    if (lexer.IsMatch(parseTree::Token_t::BOOLEAN))
    {
        // Boolean is accepted for backward compatibility, but must be deprecated
        // in favor of more explicit keywords.
        sectionPtr->AddContent(lexer.Pull(parseTree::Token_t::BOOLEAN));
    }
    else if (lexer.IsMatch(parseTree::Token_t::MD5_HASH))
    {
        sectionPtr->AddContent(lexer.Pull(parseTree::Token_t::MD5_HASH));
    }
    else if (lexer.IsMatch(parseTree::Token_t::NAME))
    {
        auto nameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

        const std::string& actionName = nameTokenPtr->text;
        if ((actionName == "buildVersion") || (actionName == "anyVersion"))
        {
            sectionPtr->AddContent(nameTokenPtr);
        }
    }
    else
    {
        lexer.ThrowException(LE_I18N("'preloaded' section must contain 'buildVersion', "
                                     "'anyVersion', or an MD5 hash."));
    }

    return sectionPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses an entry in an app's override list.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseAppOverride
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    // App overrides are all inside sections.

    // Pull the section name out of the file.
    auto sectionNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    const std::string& sectionName = sectionNameTokenPtr->text;

    // Branch based on the section name.
    if (   (sectionName == "cpuShare")
           || (sectionName == "maxCoreDumpFileBytes")
           || (sectionName == "maxFileBytes")
           || (sectionName == "maxFileDescriptors")
           || (sectionName == "maxFileSystemBytes")
           || (sectionName == "maxLockedMemoryBytes")
           || (sectionName == "maxMemoryBytes")
           || (sectionName == "maxMQueueBytes")
           || (sectionName == "maxQueuedSignals")
           || (sectionName == "maxStackBytes")
           || (sectionName == "watchdogTimeout")
           || (sectionName == "maxWatchdogTimeout")
           || (sectionName == "maxThreads")
           || (sectionName == "maxSecureStorageBytes") )
    {
        return ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::INTEGER);
    }
    else if (sectionName == "faultAction")
    {
        return ParseFaultAction(lexer, sectionNameTokenPtr);
    }
    else if (sectionName == "groups")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::GROUP_NAME);
    }
    else if (sectionName == "maxPriority")
    {
        return ParsePriority(lexer, sectionNameTokenPtr);
    }
    else if (sectionName == "pools")
    {
        return ParseSimpleNamedItemListSection(lexer,
                                               sectionNameTokenPtr,
                                               parseTree::Content_t::POOL,
                                               parseTree::Token_t::NAME);
    }
    else if (sectionName == "sandboxed")
    {
        return ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::BOOLEAN);
    }
    else if (sectionName == "start")
    {
        return ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::NAME);
    }
    else if (sectionName == "preloaded")
    {
        return ParseAppPreloadedSection(lexer, sectionNameTokenPtr);
    }
    else if (sectionName == "watchdogAction")
    {
        return ParseWatchdogAction(lexer, sectionNameTokenPtr);
    }
    else
    {
        lexer.ThrowException(
            mk::format(LE_I18N("Unrecognized app override section name '%s'."), sectionName)
        );
        return NULL;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Parses an entry in the "apps:" section in a .sdef file.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItemList_t* ParseApp
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    // Each entry in the apps: subsection can be either just an file path, or it can be
    // a file path followed by overrides inside curlies.
    // Pull the app name out of the file and create a new object for it.
    auto itemPtr = new parseTree::App_t(lexer.Pull(parseTree::Token_t::FILE_PATH));

    // If there's a curly next,
    if (lexer.IsMatch(parseTree::Token_t::OPEN_CURLY))
    {
        // Pull the curly out of the token stream.
        (void)lexer.Pull(parseTree::Token_t::OPEN_CURLY);

        // Until we find a closing '}', keep parsing overrides.
        while (!lexer.IsMatch(parseTree::Token_t::CLOSE_CURLY))
        {
            if (lexer.IsMatch(parseTree::Token_t::END_OF_FILE))
            {
                lexer.ThrowException(
                    mk::format(LE_I18N("Unexpected end-of-file before end of application override "
                                       "list for app '%s'.\n"
                                       "%s: note: Appliction override list starts here."),
                               itemPtr->firstTokenPtr->text,
                               itemPtr->firstTokenPtr->GetLocation())
                );
            }

            itemPtr->AddContent(ParseAppOverride(lexer));
        }

        // Pull out the '}' and make that the last token in the app.
        itemPtr->lastTokenPtr = lexer.Pull(parseTree::Token_t::CLOSE_CURLY);
    }

    return itemPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a binding.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::Binding_t* ParseBinding
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Binding_t* bindingPtr;

    // Client side of the binding must be one of the following forms:
    //      "clientApp.externalInterface
    //      "clientApp.exe.component.internalInterface
    //      "clientApp.*.internalInterface
    //      "<clientUser>.externalInterface

    // The first part is always an IPC Agent token, followed by a '.'.
    bindingPtr = new parseTree::Binding_t(lexer.Pull(parseTree::Token_t::IPC_AGENT));
    (void)lexer.Pull(parseTree::Token_t::DOT);

    // If a '*' comes next, then it's a wildcard binding.
    if (lexer.IsMatch(parseTree::Token_t::STAR))
    {
        // Check that the "IPC agent" is an app.
        if (bindingPtr->firstTokenPtr->text[0] == '<')
        {
            lexer.ThrowException(LE_I18N("Wildcard bindings not permitted for non-app users."));
        }

        // Expect a "*.interfaceName" to follow.
        bindingPtr->AddContent(lexer.Pull(parseTree::Token_t::STAR));
        (void)lexer.Pull(parseTree::Token_t::DOT);
        bindingPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));
    }
    // If a '*' does not come next, expect a name.
    else
    {
        bindingPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));

        // If the next thing is a '.', then this must be an internal interface binding override.
        // Otherwise, we are done the client-side part.
        if (lexer.IsMatch(parseTree::Token_t::DOT))
        {
            // Check that the "IPC agent" is an app.
            if (bindingPtr->firstTokenPtr->text[0] == '<')
            {
                lexer.ThrowException(
                    mk::format(LE_I18N("Too many parts to client-side interface specification for"
                                       " non-app user '%s'."
                                       " Can only override internal interface bindings for apps."),
                               bindingPtr->firstTokenPtr->text)
                );
            }

            // For the internal interface binding override, we have already pulled the token
            // for the exe name.  Now we expect ".component.internalInterface".
            (void)lexer.Pull(parseTree::Token_t::DOT);
            bindingPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));
            (void)lexer.Pull(parseTree::Token_t::DOT);
            bindingPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));
        }
    }

    // ->
    (void)lexer.Pull(parseTree::Token_t::ARROW);

    // Server side of the binding must be one of the following forms:
    //      serverApp.externalInterface"
    //      <serverUser>.externalInterface"
    bindingPtr->AddContent(lexer.Pull(parseTree::Token_t::IPC_AGENT));
    (void)lexer.Pull(parseTree::Token_t::DOT);
    // If a '*' comes next, then it's an (illegal) attempt to do a server-side wildcard binding.
    if (lexer.IsMatch(parseTree::Token_t::STAR))
    {
        lexer.ThrowException(LE_I18N("Wildcard bindings not permitted for"
                                     " server-side interfaces."));
    }
    bindingPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));

    // Expect closing curly to end section or whitespace to separate bindings.
    // If there is another '.' here, the user probably is trying to bind to an internal
    // interface on the server side.
    if (lexer.IsMatch(parseTree::Token_t::DOT))
    {
        lexer.ThrowException(LE_I18N("Too many parts to server-side interface specification."
                                     " Can only bind to external interfaces in .sdef files."));
    }

    return bindingPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse an environment variable definition in the "buildVars:" section.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::TokenList_t* ParseBuildVar
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    // An buildVars entry is a simple named item containing a FILE_PATH token.
    parseTree::Token_t* nameToken = lexer.Pull(parseTree::Token_t::NAME);

    parseTree::TokenList_t* buildVar = ParseSimpleNamedItem(lexer,
                                                            nameToken,
                                                            parseTree::Content_t::ENV_VAR,
                                                            parseTree::Token_t::FILE_PATH);

    // Immediately set the build variable in the environment.  Further parsing steps depend
    // on it being set
    SetBuildVar(lexer, buildVar);

    return buildVar;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a command.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::Command_t* ParseCommand
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Command_t* commandPtr;

    // The first part is always the command name.  Paths are not allowed.
    commandPtr = new parseTree::Command_t(lexer.Pull(parseTree::Token_t::NAME));

    // '='
    (void)lexer.Pull(parseTree::Token_t::EQUALS);

    // App name
    commandPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));

    // ':'
    lexer.Pull(parseTree::Token_t::COLON);

    // Path to executable within app.
    commandPtr->AddContent(lexer.Pull(parseTree::Token_t::FILE_PATH));

    return commandPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse an item from inside an "extern:" section.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseExternItem
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    // Must be of the form "alias = application.interface"

    // Pull the alias
    auto aliasNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);
    auto itemPtr = parseTree::CreateTokenList(parseTree::CompoundItem_t::EXTERN_API_INTERFACE,
                                              aliasNameTokenPtr);
    itemPtr->AddContent(aliasNameTokenPtr);

    if (lexer.IsMatch(parseTree::Token_t::EQUALS) || lexer.IsMatch(parseTree::Token_t::WHITESPACE))
    {
        // The first token is an alias.  Pull out the '=' and any whitespace and get the exe name.
        // Pull the equals token
        (void)lexer.Pull(parseTree::Token_t::EQUALS);
        itemPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));
    }

    // Rest is ".interface".
    lexer.Pull(parseTree::Token_t::DOT);
    itemPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));

    return itemPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse an item from inside an "links:" section.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseLinksItem
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    // Must be "linkName = ( componentName arg1 etc )"

    // Pull the "linkName"
    auto linkNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);
    auto itemPtr = parseTree::CreateTokenList(parseTree::CompoundItem_t::TOKEN_LIST_SECTION,
                                              linkNameTokenPtr);
    itemPtr->AddContent(linkNameTokenPtr);

    // Pull the tokens: = (
    lexer.Pull(parseTree::Token_t::EQUALS);
    lexer.Pull(parseTree::Token_t::OPEN_PARENTHESIS);

    // Pull the componentName token and add it to the token list content
    auto componentNameTokenPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);
    itemPtr->AddContent(componentNameTokenPtr);

    // Until we find a closing ')', keep parsing overrides.
    while (!lexer.IsMatch(parseTree::Token_t::CLOSE_PARENTHESIS))
    {
        auto argPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);
        itemPtr->AddContent(argPtr);
    }

    lexer.Pull(parseTree::Token_t::CLOSE_PARENTHESIS);

    return itemPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a section in a .sdef file.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseSection
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    // Create a new Section object for the parse tree.  Pull the section name out of the file.
    auto sectionNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    const std::string& sectionName = sectionNameTokenPtr->text;

    // Branch based on the section name.
    if (sectionName == "apps")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseApp);
    }
    else if (sectionName == "bindings")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseBinding);
    }
    else if (sectionName == "buildVars")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseBuildVar);
    }
    else if (sectionName == "cflags")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (sectionName == "commands")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseCommand);
    }
    else if (sectionName == "cxxflags")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (sectionName == "interfaceSearch")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (sectionName == "appSearch")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (sectionName == "componentSearch")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (sectionName == "moduleSearch")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (IsNameSingularPlural(sectionName, "kernelModule"))
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseRequiredModule);
    }
    else if (sectionName == "ldflags")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (sectionName == "externalWatchdogKick")
    {
        return ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::INTEGER);
    }
    else if (sectionName == "extern")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, internal::ParseExternItem);
    }
    else if (sectionName == "links")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, internal::ParseLinksItem);
    }
    else
    {
        lexer.ThrowException(
            mk::format(LE_I18N("Unrecognized section name '%s'."), sectionName)
        );
        return NULL;
    }
}


} // namespace internal


//--------------------------------------------------------------------------------------------------
/**
 * Parses a .sdef file in version 1 format.
 *
 * @return Pointer to a fully populated SdefFile_t object.
 *
 * @throw mk::Exception_t if an error is encountered.
 */
//--------------------------------------------------------------------------------------------------
parseTree::SdefFile_t* Parse
(
    const std::string& filePath,    ///< Path to .adef file to be parsed.
    bool beVerbose                  ///< true if progress messages should be printed.
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::SdefFile_t* filePtr = new parseTree::SdefFile_t(filePath);

    ParseFile(filePtr, beVerbose, internal::ParseSection);

    return filePtr;
}

} // namespace adef

} // namespace parser
