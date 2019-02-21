//--------------------------------------------------------------------------------------------------
/**
 * @file apiParser.h  Parser for .api files.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_API_PARSER_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_API_PARSER_H_INCLUDE_GUARD


namespace api
{


//--------------------------------------------------------------------------------------------------
/**
 * Gets a list of other .api files that a given .api file depends on.
 *
 * @throw mk::Exception_t if an error is encountered.
 */
//--------------------------------------------------------------------------------------------------
void GetDependencies
(
    const std::string& filePath,    ///< Path to .api file to be parsed.
    std::function<void (std::string&&)> handlerFunc ///< Function to call with dependencies.
);



} // namespace api

#endif // LEGATO_DEFTOOLS_API_PARSER_H_INCLUDE_GUARD
