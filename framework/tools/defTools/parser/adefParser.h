//--------------------------------------------------------------------------------------------------
/**
 * @file adefParser.h  Parser for .adef files.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_ADEF_PARSER_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_ADEF_PARSER_H_INCLUDE_GUARD


namespace adef
{


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
);



} // namespace adef

#endif // LEGATO_DEFTOOLS_ADEF_PARSER_H_INCLUDE_GUARD
