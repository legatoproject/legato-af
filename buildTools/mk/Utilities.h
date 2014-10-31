//--------------------------------------------------------------------------------------------------
/**
 * Utility functions used by the mk tools.
 *
 * Copyright (C) 2013-2014, Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef UTILITIES_INCLUDE_GUARD_H
#define UTILITIES_INCLUDE_GUARD_H

namespace mk
{

//--------------------------------------------------------------------------------------------------
/**
 * Determine the compiler we are using is clang.
 *
 * @return  Are we using clang ?
 */
//--------------------------------------------------------------------------------------------------
bool IsCompilerClang
(
    const std::string& compilerPath ///< Path to the compiler
);


//----------------------------------------------------------------------------------------------
/**
 * Get the command-line path to use to invoke the (cross) compiler for a given target and
 * source code language.
 *
 * @return  The compiler's file system path.
 *
 * @throw   legato::Exception if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetCompilerPath
(
    const std::string& target,  ///< Name of the target platform (e.g., "localhost" or "ar7").
    legato::ProgrammingLanguage_t language ///< The source code language.
);


//----------------------------------------------------------------------------------------------
/**
 * Get the command-line path to use to invoke the (cross) linker for a given target.
 *
 * @return  The linker's file system path.
 *
 * @throw   legato::Exception if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetLinkerPath
(
    const std::string& target   ///< Name of the target platform (e.g., "localhost" or "ar7").
);


//----------------------------------------------------------------------------------------------
/**
 * Get the command-line path to use to invoke the static library archiver for a given target.
 *
 * @return  The archiver's file system path.
 *
 * @throw   legato::Exception if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetArchiverPath
(
    const std::string& target   ///< Name of the target platform (e.g., "localhost" or "ar7").
);


//----------------------------------------------------------------------------------------------
/**
 * Get the sysroot path to use when linking for a given compiler.
 *
 * @return  The path to the sysroot base directory.
 *
 * @throw   legato::Exception if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetSysRootPath
(
    const std::string& compilerPath ///< Path to the compiler
);


//----------------------------------------------------------------------------------------------
/**
 * Adds target-specific environment variables (e.g., LEGATO_TARGET) to the process's environment.
 *
 * The environment will get inherited by any child processes, including the shell that is used
 * to run the compiler and linker.  So, this allows these environment variables to be used in
 * paths in .adef and Component.cdef files.
 */
//----------------------------------------------------------------------------------------------
void SetTargetSpecificEnvVars
(
    const std::string& target  ///< Name of the target platform (e.g., "localhost" or "ar7").
);


//--------------------------------------------------------------------------------------------------
/**
 * Execute a shell command-line string.
 *
 * @throw   legato::Exception on failure.
 */
//--------------------------------------------------------------------------------------------------
void ExecuteCommandLine
(
    const std::string& commandLine
);


//--------------------------------------------------------------------------------------------------
/**
 * Execute a shell command-line string.
 *
 * @throw   legato::Exception on failure.
 */
//--------------------------------------------------------------------------------------------------
void ExecuteCommandLine
(
    const std::stringstream& commandLine
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the API protocol hash string for the framework's Config API.
 *
 * @return the hash string.
 **/
//--------------------------------------------------------------------------------------------------
const std::string& ConfigApiHash
(
    const legato::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the API protocol hash string for the framework's Watchdog API.
 *
 * @return the hash string.
 **/
//--------------------------------------------------------------------------------------------------
const std::string& WatchdogApiHash
(
    const legato::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Copy a file or directory from the build host's file system to the application's staging
 * directory.
 **/
//--------------------------------------------------------------------------------------------------
void CopyToStaging
(
    const std::string& sourcePath,      ///< Build host file system path to source file.
    const std::string& stagingDirPath,  ///< Build host file system path to staging directory.
    const std::string& sandboxPath,     ///< Must be an absolute path in the app's runtime sandbox.
    bool isVerbose                      ///< true if progress should be printed to stdout.
);


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given output stream the appropriate compiler/linker command-line directives to be
 * used to link with a given library file.
 **/
//--------------------------------------------------------------------------------------------------
void GetLinkDirectiveForLibrary
(
    std::ostream& outputStream,
    const std::string& libraryPath
);


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given output stream a list of library link directives for libraries required or
 * bundled by a given Component and all its sub-components.
 **/
//--------------------------------------------------------------------------------------------------
void GetComponentLibLinkDirectives
(
    std::ostream& outputStream,
    const legato::Component& component
);



}

#endif // UTILITIES_INCLUDE_GUARD_H

