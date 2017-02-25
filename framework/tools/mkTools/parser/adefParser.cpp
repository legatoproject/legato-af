//--------------------------------------------------------------------------------------------------
/**
 * @file adefParser.cpp  Implementation of the .adef file parser.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


namespace parser
{

namespace adef
{

namespace internal
{

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

    // In a .adef, the binding must be one of the following forms:
    //   External bindings:
    //      "*.clientInterface -> app.exportedInterface"
    //      "clientExe.clientComponent.clientInterface -> app.exportedInterface"
    //      "*.clientInterface -> <user>.exportedInterface"
    //      "clientExe.clientComponent.clientInterface -> <user>.exportedInterface"
    //   Internal bindings:
    //      "clientExe.clientComponent.clientInterface -> serverExe.serverComponent.serverInterface"

    // Match the client side first.
    if (lexer.IsMatch(parseTree::Token_t::STAR))
    {
        auto starPtr = lexer.Pull(parseTree::Token_t::STAR);
        bindingPtr = new parseTree::Binding_t(starPtr);
    }
    else
    {
        auto exeNamePtr = lexer.Pull(parseTree::Token_t::NAME);
        bindingPtr = new parseTree::Binding_t(exeNamePtr);
        (void)lexer.Pull(parseTree::Token_t::DOT);
        bindingPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));
    }
    (void)lexer.Pull(parseTree::Token_t::DOT);
    bindingPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));

    // ->
    (void)lexer.Pull(parseTree::Token_t::ARROW);

    // Match the server side.
    auto firstServerTokenPtr = lexer.Pull(parseTree::Token_t::IPC_AGENT);
    bindingPtr->AddContent(firstServerTokenPtr);
    (void)lexer.Pull(parseTree::Token_t::DOT);
    bindingPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));
    if (lexer.IsMatch(parseTree::Token_t::DOT))
    {
        // The first part of the server-side specification is actually an exe name.
        lexer.ConvertToName(firstServerTokenPtr);

        (void)lexer.Pull(parseTree::Token_t::DOT);
        bindingPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));
    }

    return bindingPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse an executable spec.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::TokenList_t* ParseExecutable
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    // An executable spec is a named item containing a list of FILE_PATH tokens.
    return ParseTokenListNamedItem(lexer,
                                   lexer.Pull(parseTree::Token_t::NAME),
                                   parseTree::Content_t::EXECUTABLE,
                                   parseTree::Token_t::FILE_PATH    );

}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a "run = (...)" entry in a "processes:" section.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::RunProcess_t* ParseRunEntry
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::RunProcess_t* entryPtr;

    // Must be either "procName = ( exe arg1 etc )" or just "( exe arg1 etc )".
    if (lexer.IsMatch(parseTree::Token_t::NAME))
    {
        auto procNamePtr = lexer.Pull(parseTree::Token_t::NAME);

        entryPtr = new parseTree::RunProcess_t(procNamePtr);
        entryPtr->AddContent(procNamePtr);

        (void)lexer.Pull(parseTree::Token_t::EQUALS);

        (void)lexer.Pull(parseTree::Token_t::OPEN_PARENTHESIS);
    }
    else
    {
        entryPtr = new parseTree::RunProcess_t(lexer.Pull(parseTree::Token_t::OPEN_PARENTHESIS));
    }

    entryPtr->AddContent(lexer.Pull(parseTree::Token_t::FILE_PATH));

    while (lexer.IsMatch(parseTree::Token_t::FILE_PATH))
    {
        entryPtr->AddContent(lexer.Pull(parseTree::Token_t::FILE_PATH));
    }

    entryPtr->lastTokenPtr = lexer.Pull(parseTree::Token_t::CLOSE_PARENTHESIS);

    return entryPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse an environment variable.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::TokenList_t* ParseEnvVarsEntry
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    // An envVars entry a simple named item containing a FILE_PATH token.
    return ParseSimpleNamedItem(lexer,
                                lexer.Pull(parseTree::Token_t::NAME),
                                parseTree::Content_t::ENV_VAR,
                                parseTree::Token_t::FILE_PATH);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a subsection within a "processes:" section.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseProcessesSubsection
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    auto subsectionNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    const std::string& subsectionName = subsectionNameTokenPtr->text;

    if (subsectionName == "run")
    {
        return ParseComplexSection(lexer, subsectionNameTokenPtr, ParseRunEntry);
    }
    else if (subsectionName == "envVars")
    {
        return ParseComplexSection(lexer, subsectionNameTokenPtr, ParseEnvVarsEntry);
    }
    else if (subsectionName == "faultAction")
    {
        return ParseFaultAction(lexer, subsectionNameTokenPtr);
    }
    else if (subsectionName == "priority")
    {
        return ParsePriority(lexer, subsectionNameTokenPtr);
    }
    else if (subsectionName == "maxCoreDumpFileBytes")
    {
        return ParseSimpleSection(lexer, subsectionNameTokenPtr, parseTree::Token_t::INTEGER);
    }
    else if (subsectionName == "maxFileBytes")
    {
        return ParseSimpleSection(lexer, subsectionNameTokenPtr, parseTree::Token_t::INTEGER);
    }
    else if (subsectionName == "maxFileDescriptors")
    {
        return ParseSimpleSection(lexer, subsectionNameTokenPtr, parseTree::Token_t::INTEGER);
    }
    else if (subsectionName == "maxLockedMemoryBytes")
    {
        return ParseSimpleSection(lexer, subsectionNameTokenPtr, parseTree::Token_t::INTEGER);
    }
    else if (subsectionName == "watchdogAction")
    {
        return ParseWatchdogAction(lexer, subsectionNameTokenPtr);
    }
    else if (subsectionName == "watchdogTimeout")
    {
        return ParseWatchdogTimeout(lexer, subsectionNameTokenPtr);
    }
    else
    {
        lexer.ThrowException(
            mk::format(LE_I18N("Unexpected subsection name '%s' in 'processes' section."),
                       subsectionName)
        );
        return NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse an API interface item from inside an "extern:" section.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::TokenList_t* ParseExternApiInterface
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    // Must be of the form "alias = exe.component.interface"
    // or "exe.component.interface" (without the alias).

    auto firstTokenPtr = lexer.Pull(parseTree::Token_t::NAME);
    auto ifPtr = parseTree::CreateTokenList(parseTree::CompoundItem_t::EXTERN_API_INTERFACE,
                                            firstTokenPtr);
    ifPtr->AddContent(firstTokenPtr);

    if (lexer.IsMatch(parseTree::Token_t::EQUALS) || lexer.IsMatch(parseTree::Token_t::WHITESPACE))
    {
        // The first token is an alias.  Pull out the '=' and any whitespace and get the exe name.
        (void)lexer.Pull(parseTree::Token_t::EQUALS);
        ifPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));
    }

    // Rest is ".component.interface".
    (void)lexer.Pull(parseTree::Token_t::DOT);
    ifPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));
    (void)lexer.Pull(parseTree::Token_t::DOT);
    ifPtr->AddContent(lexer.Pull(parseTree::Token_t::NAME));

    return ifPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a subsection inside a "provides:" section.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseProvidesSubsection
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    auto subsectionNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    const std::string& subsectionName = subsectionNameTokenPtr->text;

    if (subsectionName == "api")
    {
        return ParseComplexSection(lexer, subsectionNameTokenPtr, ParseExternApiInterface);
    }
    else
    {
        lexer.ThrowException(
            mk::format(LE_I18N("Unexpected subsection name '%s' in 'provides' section."),
                       subsectionName)
        );
        return NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse an configuration tree item from inside a "required:" section's "configTree:" subsection.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::RequiredConfigTree_t* ParseRequiredConfigTree
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::RequiredConfigTree_t* itemPtr = NULL;
    parseTree::Token_t* treeNamePtr = NULL;

    // Accept an optional set of read and/or write permissions.
    if (lexer.IsMatch(parseTree::Token_t::FILE_PERMISSIONS))
    {
        auto permissionsPtr = lexer.Pull(parseTree::Token_t::FILE_PERMISSIONS);

        if (   (permissionsPtr->text != "[r]")
            && (permissionsPtr->text != "[w]")
            && (permissionsPtr->text != "[rw]")
            && (permissionsPtr->text != "[wr]") )
        {
            permissionsPtr->ThrowException(LE_I18N("Invalid access permissions "
                                                   "for configuration tree."));
        }

        itemPtr = new parseTree::RequiredConfigTree_t(permissionsPtr);
        itemPtr->AddContent(permissionsPtr);
    }

    // If just a "DOT" is found, provide read access
    // to the current application, if a name is found provide
    // read access to the application mentioned.
    if (lexer.IsMatch(parseTree::Token_t::DOT))
    {
        // Create a new item if first token, else add the "DOT"
        // to file permission
        treeNamePtr = lexer.Pull(parseTree::Token_t::DOT);
    }
    else if (lexer.IsMatch(parseTree::Token_t::NAME))
    {
        // Create a new item if first one, else add the file name
        // to file permission
        treeNamePtr = lexer.Pull(parseTree::Token_t::NAME);
    }
    else
    {
        lexer.ThrowException(LE_I18N("Unexpected token in configTree Subsection. "
                                     "File permissions (e.g., '[rw]') or "
                                     "config tree name or '.' expected."));
    }

    if (itemPtr == NULL)
    {
        itemPtr = new parseTree::RequiredConfigTree_t(treeNamePtr);
    }

    itemPtr->AddContent(treeNamePtr);

    return itemPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a subsection inside a "requires:" section.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseRequiresSubsection
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    auto subsectionNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    const std::string& subsectionName = subsectionNameTokenPtr->text;

    if (subsectionName == "api")
    {
        subsectionNameTokenPtr->PrintWarning(LE_I18N("'api' subsection in 'requires' section is "
                                                     "deprecated in .adef files.  "
                                                     "Use the extern section instead."));

        return ParseComplexSection(lexer, subsectionNameTokenPtr, ParseExternApiInterface);
    }
    else if (subsectionName == "configTree")
    {
        return ParseComplexSection(lexer, subsectionNameTokenPtr, ParseRequiredConfigTree);
    }
    else if (subsectionName == "dir")
    {
        return ParseComplexSection(lexer, subsectionNameTokenPtr, ParseRequiredDir);
    }
    else if (subsectionName == "file")
    {
        return ParseComplexSection(lexer, subsectionNameTokenPtr, ParseRequiredFile);
    }
    else if (subsectionName == "device")
    {
        return ParseComplexSection(lexer, subsectionNameTokenPtr, ParseRequiredDevice);
    }
    else
    {
        lexer.ThrowException(
            mk::format(LE_I18N("Unexpected subsection name '%s' in 'requires' section."),
                       subsectionName)
        );
        return NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a section in a .adef file.
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
    // Pull the section name out of the file.
    auto sectionNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    const std::string& sectionName = sectionNameTokenPtr->text;

    // Branch based on the section name.
    if (   (sectionName == "cpuShare")
        || (sectionName == "maxFileSystemBytes")
        || (sectionName == "maxMemoryBytes")
        || (sectionName == "maxMQueueBytes")
        || (sectionName == "maxQueuedSignals")
        || (sectionName == "maxThreads")
        || (sectionName == "maxSecureStorageBytes") )
    {
        return ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::INTEGER);
    }
    else if (sectionName == "bindings")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, internal::ParseBinding);
    }
    else if (sectionName == "bundles")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseBundlesSubsection);
    }
    else if (sectionName == "components")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (sectionName == "executables")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, internal::ParseExecutable);
    }
    else if (sectionName == "extern")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, internal::ParseExternApiInterface);
    }
    else if (sectionName == "groups")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::GROUP_NAME);
    }
    else if (sectionName == "processes")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, internal::ParseProcessesSubsection);
    }
    else if (sectionName == "provides")
    {
        sectionNameTokenPtr->PrintWarning(LE_I18N("'provides' section is deprecated in .adef files."
                                                  " Use the extern section instead."));

        return ParseComplexSection(lexer, sectionNameTokenPtr, internal::ParseProvidesSubsection);
    }
    else if (sectionName == "requires")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, internal::ParseRequiresSubsection);
    }
    else if (sectionName == "sandboxed")
    {
        return ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::NAME);
    }
    else if (sectionName == "start")
    {
        return ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::NAME);
    }
    else if (sectionName == "version")
    {
        return ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_NAME);
    }
    else if (sectionName == "watchdogAction")
    {
        return ParseWatchdogAction(lexer, sectionNameTokenPtr);
    }
    else if (sectionName == "watchdogTimeout")
    {
        return ParseWatchdogTimeout(lexer, sectionNameTokenPtr);
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
 * Parses a .adef file in version 1 format.
 *
 * @return Pointer to a fully populated AdefFile_t object.
 *
 * @throw mk::Exception_t if an error is encountered.
 */
//--------------------------------------------------------------------------------------------------
parseTree::AdefFile_t* Parse
(
    const std::string& filePath,    ///< Path to .adef file to be parsed.
    bool beVerbose                  ///< true if progress messages should be printed.
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::AdefFile_t* filePtr = new parseTree::AdefFile_t(filePath);

    ParseFile(filePtr, beVerbose, internal::ParseSection);

    return filePtr;
}



} // namespace adef

} // namespace parser
