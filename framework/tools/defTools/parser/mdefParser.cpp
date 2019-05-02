//--------------------------------------------------------------------------------------------------
/**
 * @file mdefParser.cpp  Implementation of the .mdef file parser.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"


namespace parser
{

namespace mdef
{

namespace internal
{

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

    if (IsNameSingularPlural(subsectionName, "kernelModule"))
    {
        return ParseComplexSection(lexer, subsectionNameTokenPtr, ParseRequiredModule);
    }
    else
    {
        lexer.ThrowException(
            mk::format(LE_I18N("Unexpected subsection name '%s' in 'requires' section."),
                       subsectionName));
        return NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse "scripts:" section and parse "install:" and "remove:" sub-sections.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseScriptsSubsection
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    auto subsectionNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    const std::string& subsectionName = subsectionNameTokenPtr->text;

    if (subsectionName == "install")
    {
        return ParseSimpleSection(lexer, subsectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (subsectionName == "remove")
    {
        return ParseSimpleSection(lexer, subsectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else
    {
        lexer.ThrowException(
            mk::format(LE_I18N("Unexpected subsection name '%s' in 'scripts' section."),
                       subsectionName));
        return NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse "kernelModule:" section and parse "name:", "sources:" and "requires:" sub-sections.
 *
 * @return Pointer to the item.
 */
//--------------------------------------------------------------------------------------------------
static parseTree::CompoundItem_t* ParseKernelModuleSubsection
(
    Lexer_t& lexer
)
//--------------------------------------------------------------------------------------------------
{
    auto subsectionNameTokenPtr = lexer.Pull(parseTree::Token_t::NAME);

    const std::string& subsectionName = subsectionNameTokenPtr->text;

    if (subsectionName == "name")
    {
        return ParseSimpleSection(lexer, subsectionNameTokenPtr, parseTree::Token_t::NAME);
    }
    else if (subsectionName == "sources")
    {
        return ParseTokenListSection(lexer, subsectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (subsectionName == "requires")
    {
        return ParseComplexSection(lexer, subsectionNameTokenPtr, internal::ParseRequiresSubsection);
    }
    else
    {
        lexer.ThrowException(
            mk::format(LE_I18N("Unexpected subsection name '%s' in 'kernelModule' section."),
                       subsectionName));
        return NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a section in a .mdef file.
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
    if (sectionName == "preBuilt")
    {
        return ParseSimpleOrTokenListSection(lexer, sectionNameTokenPtr,
                                             parseTree::Token_t::FILE_PATH);
    }
    else if (sectionName == "params")
    {
        return ParseSimpleNamedItemListSection(lexer,
                                               sectionNameTokenPtr,
                                               parseTree::Content_t::MODULE_PARAM,
                                               parseTree::Token_t::STRING);
    }
    else if (sectionName == "sources")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (sectionName == "cflags" || sectionName == "ldflags")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::ARG);
    }
    else if (sectionName == "requires")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, internal::ParseRequiresSubsection);
    }
    else if (sectionName == "load")
    {
        return ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::NAME);
    }
    else if (sectionName == "bundles")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseBundlesSubsection);
    }
    else if (sectionName == "scripts")
    {
        return ParseComplexSection(lexer, sectionNameTokenPtr, ParseScriptsSubsection);
    }
    else if (sectionName == "kernelModule")
    {
       return ParseComplexSection(lexer, sectionNameTokenPtr, ParseKernelModuleSubsection);
    }
    else if (sectionName == "externalBuild")
    {
        return ParseTokenListSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else
    {
        lexer.ThrowException(
            mk::format(LE_I18N("Unrecognized keyword '%s'."), sectionName)
        );
        return NULL;
    }
}


} // namespace internal


//--------------------------------------------------------------------------------------------------
/**
 * Parses a .mdef file.
 *
 * @return Pointer to a fully populated MdefFile_t object.
 *
 * @throw mk::Exception_t if an error is encountered.
 */
//--------------------------------------------------------------------------------------------------
parseTree::MdefFile_t* Parse
(
    const std::string& filePath,    ///< Path to .mdef file to be parsed.
    bool beVerbose                  ///< true if progress messages should be printed.
)
//--------------------------------------------------------------------------------------------------
{
    parseTree::MdefFile_t* filePtr = new parseTree::MdefFile_t(filePath);

    ParseFile(filePtr, beVerbose, internal::ParseSection);

    return filePtr;
}



} // namespace adef

} // namespace parser
