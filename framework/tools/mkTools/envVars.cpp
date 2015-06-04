//--------------------------------------------------------------------------------------------------
/**
 * @file envVars.cpp
 *
 * Environment variable helper functions used by various modules.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include <string.h>

/// The standard C environment variable list.
extern char**environ;



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
)
{
    const char* value = getenv(name.c_str());

    if (value == nullptr)
    {
        return "";
    }

    return value;
}


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
)
{
    const char* value = getenv(name.c_str());

    if (value == nullptr)
    {
        throw std::runtime_error("The required environment value, " + name + ", has not been set.");
    }

    return value;
}


//----------------------------------------------------------------------------------------------
/**
 * Adds target-specific environment variables (e.g., LEGATO_TARGET) to the process's environment.
 *
 * The environment will get inherited by any child processes, including the shell that is used
 * to run the compiler and linker.  Also, this allows these environment variables to be used in
 * paths in .sdef, .adef, and .cdef files.
 */
//----------------------------------------------------------------------------------------------
void SetTargetSpecific
(
    const std::string& target  ///< Name of the target platform (e.g., "localhost" or "ar7").
)
//--------------------------------------------------------------------------------------------------
{
    if (setenv("LEGATO_TARGET", target.c_str(), true /* overwrite existing */) != 0)
    {
        throw mk::Exception_t("Failed to set LEGATO_TARGET environment variable to '"
                                   + target + "'.");
    }

    std::string path = GetRequired("LEGATO_ROOT");

    if (path.empty())
    {
        throw mk::Exception_t("LEGATO_ROOT environment variable is empty.");
    }

    path = path::Combine(path, "build/" + target);

    if (setenv("LEGATO_BUILD", path.c_str(), true /* overwrite existing */) != 0)
    {
        throw mk::Exception_t("Failed to set LEGATO_BUILD environment variable to '"
                                   + path + "'.");
    }
}


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
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    std::string result;
    std::string envVarName;

    enum
    {
        NORMAL,
        AFTER_DOLLAR,
        UNBRACKETED_VAR_NAME,
        BRACKETED_VAR_NAME,
    }
    state = NORMAL;

    for (auto i = path.begin(); i != path.end() ; i++)
    {
        switch (state)
        {
            case NORMAL:

                if (*i == '$')
                {
                    envVarName = "";
                    state = AFTER_DOLLAR;
                }
                else
                {
                    result += *i;
                }
                break;

            case AFTER_DOLLAR:

                // Opening curly can be used to start a bracketed environment variable name,
                // which must be terminated by a closing curly.
                if (*i == '{')
                {
                    state = BRACKETED_VAR_NAME;
                    break;
                }
                else
                {
                    state = UNBRACKETED_VAR_NAME;
                }
                // ** FALL THROUGH **

            case UNBRACKETED_VAR_NAME:

                // The first character in the env var name can be an alpha character or underscore.
                // The remaining can be alphanumeric or underscore.
                if (   (isalpha(*i))
                    || (!envVarName.empty() && isdigit(*i))
                    || (*i == '_') )
                {
                    envVarName += *i;
                }
                else
                {
                    // Look up the environment variable, and if found, add its value to the result.
                    const char* envVarPtr = getenv(envVarName.c_str());
                    if (envVarPtr != NULL)
                    {
                        result += envVarPtr;
                    }

                    // Copy into the result string the current character from the source string
                    // (i.e., the one right after the environment variable).
                    result += *i;

                    state = NORMAL;
                }
                break;

            case BRACKETED_VAR_NAME:

                // The first character in the env var name can be an alpha character or underscore.
                // The remaining can be alphanumeric or underscore.
                if (   (isalpha(*i))
                    || (!envVarName.empty() && isdigit(*i))
                    || (*i == '_') )
                {
                    envVarName += *i;
                }
                else if (*i == '}') // Make sure properly terminated with closing curly.
                {
                    // Look up the environment variable, and if found, add its value to the result.
                    const char* envVarPtr = getenv(envVarName.c_str());
                    if (envVarPtr != NULL)
                    {
                        result += envVarPtr;
                    }
                    state = NORMAL;
                }
                else
                {
                    throw mk::Exception_t("Invalid character inside bracketed environment"
                                               "variable name.");
                }
                break;
        }
    }

    switch (state)
    {
        case NORMAL:
            break;

        case AFTER_DOLLAR:
            throw mk::Exception_t("Environment variable name missing after '$'.");

        case UNBRACKETED_VAR_NAME:
            // The end of the string terminates the environment variable name.
            // Look up the environment variable, and if found, add its value to the result.
            {
                const char* envVarPtr = getenv(envVarName.c_str());
                if (envVarPtr != NULL)
                {
                    result += envVarPtr;
                }
            }
            break;

        case BRACKETED_VAR_NAME:
            throw mk::Exception_t("Closing brace missing from environment variable.");
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets the file system path to the file in which environment variabls are saved.
 **/
//--------------------------------------------------------------------------------------------------
static std::string GetSaveFilePath
(
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    return path::Combine(buildParams.workingDir, "mktool_environment");
}


//--------------------------------------------------------------------------------------------------
/**
 * Saves the environment variables (in a file in the build's working directory)
 * for later use by MatchesSaved().
 */
//--------------------------------------------------------------------------------------------------
void Save
(
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto filePath = GetSaveFilePath(buildParams);

    // Make sure the containing directory exists.
    file::MakeDir(buildParams.workingDir);

    // Open the file
    std::ofstream saveFile(filePath);
    if (!saveFile.is_open())
    {
        throw mk::Exception_t("Failed to open file '" + filePath + "' for writing.");
    }

    // Write each environment variable as a line in the file.
    for (int i = 0; environ[i] != NULL; i++)
    {
        saveFile << environ[i] << '\n';

        if (saveFile.fail())
        {
            throw mk::Exception_t("Error writing to file '" + filePath + "'.");
        }
    }

    // Close the file.
    saveFile.close();
    if (saveFile.fail())
    {
        throw mk::Exception_t("Error closing file '" + filePath + "'.");
    }
}


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
)
//--------------------------------------------------------------------------------------------------
{
    auto filePath = GetSaveFilePath(buildParams);

    if (!file::FileExists(filePath))
    {
        if (buildParams.beVerbose)
        {
            std::cout << "Environment variables from previous run not found." << std::endl;
        }
        return false;
    }

    // Open the file
    std::ifstream saveFile(filePath);
    if (!saveFile.is_open())
    {
        throw mk::Exception_t("Failed to open file '" + filePath + "' for reading.");
    }

    int i;
    char lineBuff[8 * 1024];

    // For each environment variable in the process's current set,
    for (i = 0; environ[i] != NULL; i++)
    {
        // Read a line from the file (discarding '\n') and check for EOF or error.
        saveFile.getline(lineBuff, sizeof(lineBuff));
        if (saveFile.eof())
        {
            goto different;
        }
        else if (!saveFile.good())
        {
            throw mk::Exception_t("Error reading from file '" + filePath + "'.");
        }

        // Compare the line from the file with the environment variable.
        if (strcmp(environ[i], lineBuff) != 0)
        {
            goto different;
        }
    }

    // Read one more line to make sure we get an end-of-file, otherwise there are less args
    // this time than last time.
    saveFile.getline(lineBuff, sizeof(lineBuff));
    if (saveFile.eof())
    {
        return true;
    }

different:

    if (buildParams.beVerbose)
    {
        std::cout << "Environment variables are different this time." << std::endl;
    }
    return false;
}



} // namespace envVars
