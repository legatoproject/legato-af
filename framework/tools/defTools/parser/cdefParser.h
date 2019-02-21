//--------------------------------------------------------------------------------------------------
/**
 * @file cdefParser.h  Parser for .cdef files.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_CDEF_PARSER_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_CDEF_PARSER_H_INCLUDE_GUARD


namespace cdef
{


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
);



} // namespace cdef

#endif // LEGATO_DEFTOOLS_CDEF_PARSER_H_INCLUDE_GUARD
