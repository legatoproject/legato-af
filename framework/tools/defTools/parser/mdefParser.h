//--------------------------------------------------------------------------------------------------
/**
 * @file mdefParser.h  Parser for .mdef files.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MDEF_PARSER_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MDEF_PARSER_H_INCLUDE_GUARD


namespace mdef
{


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
);



} // namespace mdef

#endif // LEGATO_DEFTOOLS_MDEF_PARSER_H_INCLUDE_GUARD
