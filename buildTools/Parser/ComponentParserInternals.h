//--------------------------------------------------------------------------------------------------
/**
 * Definitions needed by the parser's internals.  Not to be shared outside the parser.
 *
 * @warning THIS FILE IS INCLUDED FROM C CODE.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
    const char* filePath    ///< The file path.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a C compiler command-line argument to a Component.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddCFlag
(
    const char* arg ///< The command-line argument.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a C++ compiler command-line argument to a Component.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddCxxFlag
(
    const char* arg ///< The command-line argument.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a linker command-line argument to a Component.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddLdFlag
(
    const char* arg ///< The command-line argument.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a required file to a Component.  This is a file that is expected to exist outside the
 * application's sandbox in the target file system and that the component needs access to.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddRequiredFile
(
    const char* sourcePath, ///< The file path in the target file system, outside sandbox.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a required directory to a Component.  This is a directory that is expected to exist outside
 * the application's sandbox in the target file system and that the component needs access to.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddRequiredDir
(
    const char* sourcePath, ///< The directory path in the target file system, outside sandbox.
    const char* destPath    ///< The directory path in the target file system, inside sandbox.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a required library to a component.  This is a library that is expected to exist outside
 * the application's sandbox in the target file system and that the component needs access to.
 *
 * Furthermore, this library will be linked with any executable that this component is a part of.
 * At link time, the library search path with be searched for the library in the build host file
 * system.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddRequiredLib
(
    const char* libName     ///< The name of the library.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a required component to a component.  This is another component that is used by the
 * component that is currently being parsed.
 *
 * This will add that component to the component's list of subcomponents. Any executable
 * that includes a component also includes all of that component's subcomponents and their
 * subcomponents, etc.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddRequiredComponent
(
    const char* path        ///< The path to the component.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add to a Component a file from the build host file system that should be bundled into any
 * app that this component is a part of.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddBundledFile
(
    const char* permissions,///< String representing the permissions.
    const char* sourcePath, ///< The file path in the build host file system.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add to a Component a directory from the build host file system that should be bundled into
 * any app that this component is a part of.
 */
//--------------------------------------------------------------------------------------------------
void cyy_AddBundledDir
(
    const char* permissions,///< String representing permissions to be applied to files in the dir.
    const char* sourcePath, ///< The file path in the build host file system.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a required (client-side) IPC API interface to a Component.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddRequiredApi
(
    const char* instanceName,   ///< Interface instance name or
                                ///  NULL if should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a types-only required (client-side) IPC API interface to a Component.
 *
 * This only imports the type definitions from the .api file without generating the client-side IPC
 * library or automatically calling the client-side IPC initialization function.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddTypesOnlyRequiredApi
(
    const char* instanceName,   ///< Prefix to apply to the definitions, or
                                ///  NULL if prefix should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a manual-start required (client-side) IPC API interface to a Component.
 *
 * The client-side IPC code will be generated, but the initialization code will not be run
 * automatically by the executable's main function.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddManualStartRequiredApi
(
    const char* instanceName,   ///< Prefix to apply to the definitions, or
                                ///  NULL if prefix should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a provided (server-side) IPC API interface to a Component.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddProvidedApi
(
    const char* instanceName,   ///< Interface instance name or
                                ///  NULL if should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add an asynchronous provided (server-side) IPC API interface to a Component.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddAsyncProvidedApi
(
    const char* instanceName,   ///< Interface instance name or
                                ///  NULL if should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a manual-start provided (server-side) IPC API interface to a Component.
 *
 * The server-side IPC code will be generated, but the initialization code will not be run
 * automatically by the executable's main function.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddManualStartProvidedApi
(
    const char* instanceName,   ///< Interface instance name or
                                ///  NULL if should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Add a manual-start, asynchronous provided (server-side) IPC API interface to a Component.
 *
 * The server-side IPC code will be generated, but the initialization code will not be run
 * automatically by the executable's main function.
 **/
//--------------------------------------------------------------------------------------------------
void cyy_AddManualStartAsyncProvidedApi
(
    const char* instanceName,   ///< Interface instance name or
                                ///  NULL if should be derived from .api file name.

    const char* apiFile         ///< Path to the .api file.
);


#endif // COMPONENT_PARSER_H_INCLUDE_GUARD
