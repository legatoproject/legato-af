//--------------------------------------------------------------------------------------------------
/**
 * @file cdefParser.cpp  Implementation of the .cdef file parser.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


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
    SkipWhitespaceAndComments(lexer);

    // If there's an '=' following it, then attempt to convert it into an alias (NAME)
    // and pull out the '=' and the actual API file path.
    if (lexer.IsMatch(parseTree::Token_t::EQUALS))
    {
        lexer.ConvertToName(apiFilePathPtr);
        aliasPtr = apiFilePathPtr;
        (void)lexer.Pull(parseTree::Token_t::EQUALS);
        SkipWhitespaceAndComments(lexer);
        apiFilePathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);
        SkipWhitespaceAndComments(lexer);
    }

    // Create a new token list item.
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
        SkipWhitespaceAndComments(lexer);
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
static parseTree::CompoundItemList_t* ParseProvidesSubsection
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
    else
    {
        lexer.ThrowException("Unexpected subsection name '" + subsectionName
                             + "' in 'provides' section.");
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
    SkipWhitespaceAndComments(lexer);

    // If there's an '=' following it, then attempt to convert it into an alias (NAME)
    // and pull out the '=' and the actual API file path.
    if (lexer.IsMatch(parseTree::Token_t::EQUALS))
    {
        lexer.ConvertToName(apiFilePathPtr);
        aliasPtr = apiFilePathPtr;
        (void)lexer.Pull(parseTree::Token_t::EQUALS);
        SkipWhitespaceAndComments(lexer);
        apiFilePathPtr = lexer.Pull(parseTree::Token_t::FILE_PATH);
        SkipWhitespaceAndComments(lexer);
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
        SkipWhitespaceAndComments(lexer);
    }

    return apiPtr;
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
    else if (   (subsectionName == "lib")
             || (subsectionName == "component") )
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else
    {
        lexer.ThrowException("Unexpected subsection name '" + subsectionName
                             + "' in 'requires' section.");
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
    else if (sectionName == "sources")
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
        lexer.ThrowException("Unrecognized section name '" + sectionName + "'.");
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
