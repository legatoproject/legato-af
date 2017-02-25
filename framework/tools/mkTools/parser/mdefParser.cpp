//--------------------------------------------------------------------------------------------------
/**
 * @file mdefParser.cpp  Implementation of the .mdef file parser.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


namespace parser
{

namespace mdef
{

namespace internal
{

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
        return ParseSimpleSection(lexer, sectionNameTokenPtr, parseTree::Token_t::FILE_PATH);
    }
    else if (sectionName == "params")
    {
        return ParseSimpleNamedItemListSection(lexer,
                                               sectionNameTokenPtr,
                                               parseTree::Content_t::MODULE_PARAM,
                                               parseTree::Token_t::STRING);
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
