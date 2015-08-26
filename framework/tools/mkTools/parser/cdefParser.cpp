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
    else if (subsectionName == "device")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseRequiredDevice);
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
 * Make sure that the supplied type name is one of our supported types.
 */
//--------------------------------------------------------------------------------------------------
static void ValidateAssetDataTypeName
(
    Lexer_t& lexer,
    const std::string& typeName  ///< Name of the type in question.
)
//--------------------------------------------------------------------------------------------------
{
    if (   (typeName != "bool")
        && (typeName != "int")
        && (typeName != "float")
        && (typeName != "string"))
    {
        lexer.ThrowException("Unknown type name, '" + typeName + ",' on asset field.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Extract the default value from the token stream.  While doing this, make sure that the type of
 * token makes sense for the data type of the field.
 *
 * @return The extracted token.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::Token_t* MatchDefaultValue
(
    Lexer_t& lexer,
    const std::string& typeName  ///< The data type of the asset field.
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Token_t* defaultValueTokenPtr = NULL;

    if (typeName == "bool")
    {
        defaultValueTokenPtr = lexer.Pull(parseTree::Token_t::BOOLEAN);
    }
    else if (typeName == "int")
    {
        defaultValueTokenPtr = lexer.Pull(parseTree::Token_t::SIGNED_INTEGER);
    }
    else if (typeName == "float")
    {
        defaultValueTokenPtr = lexer.Pull(parseTree::Token_t::FLOAT);
    }
    else if (typeName == "string")
    {
        defaultValueTokenPtr = lexer.Pull(parseTree::Token_t::STRING);
    }
    else
    {
        lexer.ThrowException("Unknown type name, '" + typeName + ",' on asset field.");
    }

    return defaultValueTokenPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse an asset setting or variable.  Which one that is being parsed is passed as the AssetType_t
 * template parameter.
 *
 * @return Pointer to the subsection.
 */
//--------------------------------------------------------------------------------------------------
template <typename AssetType_t>
static parseTree::CompoundItem_t* ParseAssetField
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Token_t* dataTypeNamePtr = lexer.Pull(parseTree::Token_t::NAME);
    ValidateAssetDataTypeName(lexer, dataTypeNamePtr->text);
    SkipWhitespaceAndComments(lexer);

    parseTree::Token_t* fieldNamePtr = lexer.Pull(parseTree::Token_t::FILE_PATH);
    SkipWhitespaceAndComments(lexer);

    auto assetPtr = new AssetType_t(dataTypeNamePtr);
    assetPtr->AddContent(fieldNamePtr);

    if (lexer.IsMatch(parseTree::Token_t::EQUALS))
    {
        (void)lexer.Pull(parseTree::Token_t::EQUALS);
        SkipWhitespaceAndComments(lexer);

        parseTree::Token_t* defaultValuePtr = MatchDefaultValue(lexer, dataTypeNamePtr->text);
        assetPtr->AddContent(defaultValuePtr);
    }

    return assetPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse an asset commands subsection.
 *
 * @return Pointer to the subsection.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseAssetCommandsSubsection
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::Token_t* fieldNamePtr = lexer.Pull(parseTree::Token_t::FILE_PATH);
    SkipWhitespaceAndComments(lexer);

    return new parseTree::AssetCommand_t(fieldNamePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse an asset and its subsections.
 *
 * @return Pointer to the subsection.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseAssetFieldTypeSubsection
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    auto sectionNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    const std::string& subsectionName = sectionNameTokenPtr->text;

    if (subsectionName == "settings")
    {
        return ParseComplexSection(lexer,
                                   sectionNameTokenPtr,
                                   ParseAssetField<parseTree::AssetSetting_t>);
    }
    else if (subsectionName == "variables")
    {
        return ParseComplexSection(lexer,
                                   sectionNameTokenPtr,
                                   ParseAssetField<parseTree::AssetVariable_t>);
    }
    else if (subsectionName == "commands")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseAssetCommandsSubsection);
    }
    else
    {
        lexer.ThrowException("Unexpected subsection name '" + subsectionName
                             + "' in 'assets' section.");
        return NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse an assets section.
 *
 * @return Pointer to the subsection.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseAssetsSubsection
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    auto assetNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    return ParseNamedComplexSection(lexer,
                                    new parseTree::Asset_t(assetNameTokenPtr),
                                    ParseAssetFieldTypeSubsection);
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
    else if (sectionName == "assets")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseAssetsSubsection);
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
