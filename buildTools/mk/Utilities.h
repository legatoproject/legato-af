//--------------------------------------------------------------------------------------------------
/**
 * Utility functions used by the mk tools.
 *
 * Copyright (C) 201 Sierra Wireless Inc., all rights reserved.
 */
//--------------------------------------------------------------------------------------------------

#ifndef UTILITIES_INCLUDE_GUARD_H
#define UTILITIES_INCLUDE_GUARD_H

namespace mk
{


//----------------------------------------------------------------------------------------------
/**
 * Get the command-line path to use to invoke the (cross) compiler for a given target.
 *
 * @return  The target.
 *
 * @throw   legato::Exception if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetCompilerPath
(
    const std::string& target  ///< Name of the target platform (e.g., "localhost" or "ar7").
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


//----------------------------------------------------------------------------------------------
/**
 * Generates the identifier that is to be used for the component initialization function for a
 * given component.
 */
//--------------------------------------------------------------------------------------------------
std::string GetComponentInitName
(
    const legato::Component& component
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate IPC API client code for a given protocol in a given directory.
 */
//--------------------------------------------------------------------------------------------------
void GenerateApiClientCode
(
    const std::string& instanceName,    ///< Name of the interface instance.
    const std::string& protocolFile,    ///< Path to the .api file.
    const std::string& outputDir,       ///< Directory into which the library will go.
    bool isVerbose                      ///< If true, write troubleshooting info to stdout.
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate IPC API server code for a given protocol in a given directory.
 */
//--------------------------------------------------------------------------------------------------
void GenerateApiServerCode
(
    const std::string& instanceName,    ///< Name of the interface instance.
    const std::string& protocolFile,    ///< Path to the .api file.
    const std::string& outputDir,       ///< Directory into which the library will go.
    bool isVerbose                      ///< If true, write troubleshooting info to stdout.
);


}

#endif // UTILITIES_INCLUDE_GUARD_H

