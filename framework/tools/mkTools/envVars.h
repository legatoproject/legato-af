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
    const std::string& target  ///< Name of the target platform (e.g., "localhost" or "ar7").
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
 * Look for environment variables (specified as "$VAR_NAME" or "${VAR_NAME}") in a given string
 * and replace with environment variable contents.
 *
 * @return The converted string.
 **/
//--------------------------------------------------------------------------------------------------
std::string DoSubstitution
(
    const std::string& path,
    std::set<std::string>* usedVarsPtr = NULL ///< If not null, returns a list of all
                                              ///  variable names used in this substitution.
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



} // namespace envVars

#endif // LEGATO_ENVVARS_H_INCLUDE_GUARD
