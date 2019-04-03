//--------------------------------------------------------------------------------------------------
/**
 * @page defTools_parsers   Parsers
 *
 * The defTools parsers are composed of a Lexer and multiple parsing functions.
 *
 * There's a set of parsing functions for each type of definition file (.cdef, .adef, .mdef and .sdef),
 * and there's one function in each set for each definition file format version supported.
 * There is also a very limited functionality parser for extracting dependencies from .api files.
 *
 * - @ref lexer.h
 * - @ref cdefParser.h
 * - @ref adefParser.h
 * - @ref mdefParser.h
 * - @ref sdefParser.h
 * - @ref apiParser.h
 *
 * Also, there's a set of parsing functions declared in @ref parser.h that are shared by multiple
 * parsers.
 *
 * The Lexer analyzes the input stream and generates lexical tokens from it.  All parsing functions
 * use the same Lexer class to do the lexical analysis.  The Lexer provides a ThrowException()
 * function, which is used internally and by the parsing functions to throw exceptions containing
 * error reports with the current file path, line number and column number in them.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * @file parser.h   Includes all the parser header files.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_PARSER_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_PARSER_H_INCLUDE_GUARD

namespace parser
{


#include "lexer.h"
#include "cdefParser.h"
#include "adefParser.h"
#include "mdefParser.h"
#include "sdefParser.h"
#include "apiParser.h"


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
);


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
);


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
);


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
);


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
);


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
);


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
    parseTree::CompoundItemList_t* sectionPtr,  ///< The token containing the section name.
    std::function<parseTree::CompoundItem_t* (Lexer_t& lexer)> contentParserFunc
);


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
);


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
);


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
);

//--------------------------------------------------------------------------------------------------
/**
 * Parse a subsection inside a "kernelModules:" section.
 *
 * @return Pointer to the subsection.
 */
//--------------------------------------------------------------------------------------------------
parseTree::RequiredModule_t* ParseRequiredModule
(
    Lexer_t& lexer
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


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
);


} // namespace parser

#endif // LEGATO_DEFTOOLS_PARSER_H_INCLUDE_GUARD
