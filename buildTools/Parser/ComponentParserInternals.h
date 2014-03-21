//--------------------------------------------------------------------------------------------------
/**
 * Definitions needed by the parser's internals.  Not to be shared outside the parser.
 *
 * @warning THIS FILE IS INCLUDED FROM C CODE.
 *
 * Copyright (C) 2013 Sierra Wireless Inc., all rights reserved.
 */
//--------------------------------------------------------------------------------------------------

#ifndef COMPONENT_PARSER_H_INCLUDE_GUARD
#define COMPONENT_PARSER_H_INCLUDE_GUARD


// Maximum number of errors that will be reported before stopping the parsing.
// Note: this is an arbitrary number.
#define CYY_MAX_ERROR_COUNT 5


//--------------------------------------------------------------------------------------------------
/**
 * Component parsing function, implemented by the Bison-generated parser.
 *
 * @note    cyy_parse() may return a non-zero value before parsing has finished, because an error
 *          was encountered.  To aide in troubleshooting, cyy_parse() can be called again to look
 *          for further errors.  However, there's no point in doing this if the end-of-file has
 *          been reached.  cyy_EndOfFile can be used to check for end of file.
 *
 * @return 0 when parsing successfully completes.  Non-zero when an error is encountered.
 */
//--------------------------------------------------------------------------------------------------
int cyy_parse(void);


//--------------------------------------------------------------------------------------------------
/**
 * End of file flag.  This is set when cyy_parse() returns due to an end of file condition.
 */
//--------------------------------------------------------------------------------------------------
extern int cyy_EndOfFile;


//--------------------------------------------------------------------------------------------------
/**
 * Count of the number of errors that have been reported during scanning.
 */
//--------------------------------------------------------------------------------------------------
extern size_t cyy_ErrorCount;


//--------------------------------------------------------------------------------------------------
/**
 * Name of the file that is currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
extern const char* cyy_FileName;


//--------------------------------------------------------------------------------------------------
/**
 * Non-zero if verbose operation is requested.
 */
//--------------------------------------------------------------------------------------------------
extern int cyy_IsVerbose;


//--------------------------------------------------------------------------------------------------
/**
 * Restarts the component parsing with a new file stream to parse.
 */
//--------------------------------------------------------------------------------------------------
void cyy_restart(FILE *new_file);


//--------------------------------------------------------------------------------------------------
/**
 * The standard error reporting function that will be called by both the
 * lexer and the parser when they detect errors or when the YYERROR macro is invoked.
 */
//--------------------------------------------------------------------------------------------------
void cyy_error(const char* errorString);


//--------------------------------------------------------------------------------------------------
/**
 * Add a source code file to a Component.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddSourceFile
(
    const char* filePath    ///< The file path.  Storage will never be freed.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a file mapping to a Component.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddFileMapping
(
    const char* permissions,///< String representing the permissions.
    const char* sourcePath, ///< The file path in the target file system, outside sandbox.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add an included file to a Component.  This is a file that should be bundled into any app that
 * this component is a part of.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddIncludedFile
(
    const char* permissions,///< String representing the permissions.
    const char* sourcePath, ///< The file path in the build host file system.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add an imported interface to a Component.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddImportedInterface
(
    const char* instanceName,
    const char* apiFile
);


//--------------------------------------------------------------------------------------------------
/**
 * Add an exported interface to a Component.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddExportedInterface
(
    const char* instanceName,
    const char* apiFile
);


#endif // COMPONENT_PARSER_H_INCLUDE_GUARD
