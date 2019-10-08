//--------------------------------------------------------------------------------------------------
/**
 * @file envVars.h
 *
 * Environment variable helper functions used by various modules.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_ENVVARS_H_INCLUDE_GUARD
#define LEGATO_ENVVARS_H_INCLUDE_GUARD

namespace envVars
{


//--------------------------------------------------------------------------------------------------
/**
 * Fetch the value of a given optional environment variable.
 *
 * @return  The value ("" if not found).
 */
//--------------------------------------------------------------------------------------------------
std::string Get
(
    const std::string& name  ///< The name of the environment variable.
);

//--------------------------------------------------------------------------------------------------
/**
 * Fetch the value of a given optional environment variable and translate it to a boolean value as
 * though it were a KConfig setting.
 *
 * @return  True if the value is "1" or "y", false if it is any other value or not set.
 */
//--------------------------------------------------------------------------------------------------
bool GetConfigBool
(
    const std::string &name  ///< The name of the environment variable.
);

//--------------------------------------------------------------------------------------------------
/**
 * Fetch the value of a given mandatory environment variable.
 *
 * @return  The value.
 *
 * @throw   Exception if environment variable not found.
 */
//--------------------------------------------------------------------------------------------------
std::string GetRequired
(
    const std::string& name  ///< The name of the environment variable.
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the value of a given environment variable.  If the variable already exists, replaces its
 * value.
 */
//--------------------------------------------------------------------------------------------------
void Set
(
    const std::string& name,  ///< The name of the environment variable.
    const std::string& value  ///< The value of the environment variable.
);


//--------------------------------------------------------------------------------------------------
/**
 * Unset the value of a given environment variable.
 */
//--------------------------------------------------------------------------------------------------
void Unset
(
    const std::string& name  ///< The name of the environment variable.
);


//----------------------------------------------------------------------------------------------
/**
 * Adds target-specific environment variables (e.g., LEGATO_TARGET) to the process's environment.
 *
 * The environment will get inherited by any child processes, including the shell that ninja uses
 * to run the compiler and linker.  Also, this allows these environment variables to be used in
 * paths in .sdef, .adef, and .cdef files.
 */
//----------------------------------------------------------------------------------------------
void SetTargetSpecific
(
    const mk::BuildParams_t& buildParams
);


//----------------------------------------------------------------------------------------------
/**
 * Checks if a given environment variable name is one of the reserved environment variable names
 * (e.g., LEGATO_TARGET).
 *
 * @return true if reserved, false if not.
 */
//----------------------------------------------------------------------------------------------
bool IsReserved
(
    const std::string& name  ///< Name of the variable.
);


//--------------------------------------------------------------------------------------------------
/**
 * Saves the environment variables (in a file in the build's working directory)
 * for later use by MatchesSaved().
 */
//--------------------------------------------------------------------------------------------------
void Save
(
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Compares the current environment variables with those stored in the build's working directory.
 *
 * @return true if the environment variabls are effectively the same or
 *         false if there's a significant difference.
 */
//--------------------------------------------------------------------------------------------------
bool MatchesSaved
(
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Execute the given callback function.  Once for every current environment variable.
 */
//--------------------------------------------------------------------------------------------------
void Iterate
(
    /// Function that is called with the name and value of every environment variable.
    const std::function<void(const std::string&, const std::string&)>& callback
);

//--------------------------------------------------------------------------------------------------
/**
 * Load environment variables from a file into the current process' environment.  The environment
 * variables are formatted as VAR=value\n in the file.  Blank lines and comments (lines beginning
 * with #) are ignored.
 */
//--------------------------------------------------------------------------------------------------
void Load
(
    const std::string       &envFilePath, ///< Path to the file listing the environment variables.
    const mk::BuildParams_t &buildParams  ///< Current build parameters.
);

} // namespace envVars

#endif // LEGATO_ENVVARS_H_INCLUDE_GUARD
