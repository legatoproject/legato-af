//--------------------------------------------------------------------------------------------------
/**
 * @file cdefParser.cpp  Implementation of the .cdef file parser.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"


namespace parser
{

namespace cdef
{

namespace internal
{


//--------------------------------------------------------------------------------------------------
/**
 * Parse an API item from inside a "provided:" section's "api:" subsection.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::ProvidedApi_t* ParseProvidedApi
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Token_t* aliasPtr = NULL;

    // Assume there's only a file path.
    parseTree::Token_t* apiFilePathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);

    // If there's an '=' following it, then attempt to convert it into an alias (NAME)
    // and pull out the '=' and the actual API file path.
    if (lexer.IsMatch(parseTree::Token_t::EQUALS))
    {
        lexer.ConvertToName(apiFilePathPtr);
        aliasPtr = apiFilePathPtr;
        (void)lexer.Pull(parseTree::Token_t::EQUALS);
        apiFilePathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);
    }

    // Create a new Provided API item.
    parseTree::Token_t* firstPtr = (aliasPtr != NULL ? aliasPtr : apiFilePathPtr);
    auto apiPtr = new parseTree::ProvidedApi_t(firstPtr);

    // Add its contents.
    if (aliasPtr != NULL)
    {
        apiPtr->AddContent(aliasPtr);
    }
    apiPtr->AddContent(apiFilePathPtr);

    // Accept optional server-side IPC options.
    while (lexer.IsMatch(parseTree::Token_t::SERVER_IPC_OPTION))
    {
        apiPtr->AddContent(lexer.Pull(parseTree::Token_t::SERVER_IPC_OPTION));
    }

    return apiPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a subsection inside a "provides:" section.
 *
 * @return Pointer to the subsection.
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
        return ParseComplexSection(lexer, subsectionNameTokenPtr, ParseProvidedApi);
    }
    else if (subsectionName == "headerDir")
    {
        return ParseTokenListSection(lexer, subsectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (subsectionName == "lib")
    {
        return ParseTokenListSection(lexer, subsectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
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
 * Parse an API item from inside a "required:" section's "api:" subsection.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::RequiredApi_t* ParseRequiredApi
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Token_t* aliasPtr = NULL;

    // Assume there's only a file path.
    parseTree::Token_t* apiFilePathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);

    // If there's an '=' following it, then attempt to convert it into an alias (NAME)
    // and pull out the '=' and the actual API file path.
    if (lexer.IsMatch(parseTree::Token_t::EQUALS))
    {
        lexer.ConvertToName(apiFilePathPtr);
        aliasPtr = apiFilePathPtr;
        (void)lexer.Pull(parseTree::Token_t::EQUALS);
        apiFilePathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);
    }

    // Create parse tree node for this.
    parseTree::Token_t* firstPtr = (aliasPtr != NULL ? aliasPtr : apiFilePathPtr);
    auto apiPtr = new parseTree::RequiredApi_t(firstPtr);

    // Add its contents.
    if (aliasPtr != NULL)
    {
        apiPtr->AddContent(aliasPtr);
    }
    apiPtr->AddContent(apiFilePathPtr);

    // Accept optional client-side IPC options.
    while (lexer.IsMatch(parseTree::Token_t::CLIENT_IPC_OPTION))
    {
        apiPtr->AddContent(lexer.Pull(parseTree::Token_t::CLIENT_IPC_OPTION));
    }

    return apiPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a component item from "component:" subsection inside a "required:" section.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::RequiredComponent_t* ParseRequiredComponent
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Token_t* componentFilePathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);

    auto componentPtr = new parseTree::RequiredComponent_t(componentFilePathPtr);

    componentPtr->AddContent(componentFilePathPtr);

    // Accept provide-header option.
    while (lexer.IsMatch(parseTree::Token_t::PROVIDE_HEADER_OPTION))
    {
        componentPtr->AddContent(lexer.Pull(parseTree::Token_t::PROVIDE_HEADER_OPTION));
    }

    return componentPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse a subsection inside a "requires:" section.
 *
 * @return Pointer to the subsection.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseRequiresSubsection
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    const std::string& subsectionName = sectionNameTokenPtr->text;

    if (subsectionName == "api")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseRequiredApi);
    }
    else if (subsectionName == "file")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseRequiredFile);
    }
    else if (subsectionName == "dir")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseRequiredDir);
    }
    else if (subsectionName == "device")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseRequiredDevice);
    }
    else if (subsectionName == "lib")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (subsectionName == "component")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseRequiredComponent);
    }
    else if (IsNameSingularPlural(subsectionName, "kernelModule"))
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseRequiredModule);
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
 * Parses a section in a .cdef file.
 *
 * @return Pointer to the section.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseSection
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    const std::string& sectionName = sectionNameTokenPtr->text;

    // Branch based on the section name.
    if (   (sectionName == "cflags")
        || (sectionName == "cxxflags")
        || (sectionName == "ldflags") )
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::ARG);
    }
    else if (sectionName == "externalBuild")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (sectionName == "sources")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (sectionName == "javaPackage")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::DOTTED_NAME);
    }
    else if (sectionName == "pythonPackage")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (sectionName == "bundles")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseBundlesSubsection);
    }
    else if (sectionName == "provides")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseProvidesSubsection);
    }
    else if (sectionName == "requires")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseRequiresSubsection);
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
 * Parses a .cdef file in version 1 format.
 *
 * @return Pointer to a fully populated CdefFile_t object.
 *
 * @throw mk::Exception_t if an error is encountered.
 */
//--------------------------------------------------------------------------------------------------
parseTree::CdefFile_t* Parse
(
    const std::string& filePath,    ///< Path to .cdef file to be parsed.
    bool beVerbose                  ///< true if progress messages should be printed.
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::CdefFile_t* filePtr = new parseTree::CdefFile_t(filePath);

    ParseFile(filePtr, beVerbose, internal::ParseSection);

    return filePtr;
}



} // namespace cdef

} // namespace parser
