//--------------------------------------------------------------------------------------------------
/**
 * Utility functions used by the mk tools.
 *
 * Copyright (C) 2013-2014, Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include <sys/stat.h>
#include <stdlib.h>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <list>
#include "LegatoObjectModel.h"
#include "Utilities.h"
#include "../Parser/Parser.h"
#include <string.h>

extern "C" {
    #include <unistd.h>
}


namespace mk
{


/// Map of API file paths (after env var substitution) to protocol hashes.
static std::map<std::string, std::string> ApiHashCache;


//--------------------------------------------------------------------------------------------------
/**
 * Fetch the value of a given environment variable.
 *
 * @return  The value ("" if not found).
 */
//--------------------------------------------------------------------------------------------------
std::string GetEnvValue
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
 * Fetch the value of a given environment variable.
 *
 * @return  The value.
 *
 * @throw   Exception if environment variable not found.
 */
//--------------------------------------------------------------------------------------------------
std::string GetRequiredEnvValue
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
 * Get the command-line path to use to invoke the (cross) compiler for a given target.
 *
 * @return  The path to the compiler.
 *
 * @throw   legato::Exception if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetCompilerPath
(
    const std::string& target  ///< Name of the target platform (e.g., "localhost" or "ar7").
)
//--------------------------------------------------------------------------------------------------
{
    if (target == "localhost")
    {
        return "gcc";
    }

    std::string varName;
    std::string envValue;

    varName = target;
    std::transform(varName.begin(), varName.end(), varName.begin(), ::toupper);

    varName += "_TOOLCHAIN_DIR";
    envValue = GetRequiredEnvValue(varName);

    if(!envValue.empty())
    {
        return legato::CombinePath(envValue, "arm-poky-linux-gnueabi-gcc");
    }

    throw std::runtime_error("Attempting to build for target '" + target + ", but '"
                             + varName + "' is not set.");
}



//----------------------------------------------------------------------------------------------
/**
 * Get the sysroot path to use when linking for a given target.
 *
 * @return  The path to the sysroot base directory.
 *
 * @throw   legato::Exception if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetSysRootPath
(
    const std::string& target  ///< Name of the target platform (e.g., "localhost" or "ar7").
)
//--------------------------------------------------------------------------------------------------
{
    // Convert the target name to all-caps.
    std::string allCapsTarget = target;
    std::transform(allCapsTarget.begin(), allCapsTarget.end(), allCapsTarget.begin(), ::toupper);

    std::string envValue;

    // See if there's a XXXX_SYSROOT_DIR variable defined.
    std::string varName = allCapsTarget + "_SYSROOT_DIR";
    envValue = GetEnvValue(varName);
    if (!envValue.empty())
    {
        return envValue;
    }

    // Since there's no specific variable defined for the sysroot, try to figure it out from
    // the compiler.
    std::string compilerPath = GetCompilerPath(target);
    std::string commandLine = compilerPath + " --print-sysroot";

    FILE* output = popen(commandLine.c_str(), "r");

    if (output == NULL)
    {
        throw legato::Exception("Could not exec '" + commandLine + "' to get sysroot path.");
    }

    char buffer[1024] = { 0 };
    static const size_t bufferSize = sizeof(buffer);

    if (fgets(buffer, bufferSize, output) != buffer)
    {
        std::cerr << "Warning: Failed to receive sysroot path from compiler '" << compilerPath
                  << "' (errno: " << strerror(errno) << ").  Assuming '/'." << std::endl;
        buffer[0] = '/';
        buffer[1] = '\0';
    }
    else
    {
        // Remove any trailing newline character.
        size_t len = strlen(buffer);
        if (buffer[len - 1] == '\n')
        {
            buffer[len - 1] = '\0';
        }
    }

    // Close the connection and collect the exit code from ifgen.
    int result;
    do
    {
        result = pclose(output);

    } while ((result == -1) && (errno == EINTR));

    if (result == -1)
    {
        std::stringstream msg;
        msg << "Failed to receive the sysroot path from the compiler '" << compilerPath
            << "'. pclose() errno = " << strerror(errno);
        throw legato::Exception(msg.str());
    }
    else if (!WIFEXITED(result))
    {
        std::stringstream msg;
        msg << "Failed to receive the sysroot path from the compiler '" << compilerPath
            << "'. Compiler was interrupted by something.";
        throw legato::Exception(msg.str());
    }
    else if (WEXITSTATUS(result) != EXIT_SUCCESS)
    {
        std::stringstream msg;
        msg << "Failed to receive the sysroot path from the compiler '" << compilerPath
            << "'. Compiler '' exited with code " << WEXITSTATUS(result);
        throw legato::Exception(msg.str());
    }

    return buffer;
}


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
)
//--------------------------------------------------------------------------------------------------
{
    if (setenv("LEGATO_TARGET", target.c_str(), true /* overwrite existing */) != 0)
    {
        throw std::runtime_error("Failed to set LEGATO_TARGET environment variable to '"
                                 + target + "'.");
    }

    std::string path = GetRequiredEnvValue("LEGATO_ROOT");

    if (path.empty())
    {
        throw std::runtime_error("LEGATO_ROOT environment variable is empty.");
    }

    path = legato::CombinePath(path, "build/" + target);

    if (setenv("LEGATO_BUILD", path.c_str(), true /* overwrite existing */) != 0)
    {
        throw std::runtime_error("Failed to set LEGATO_BUILD environment variable to '"
                                 + path + "'.");
    }
}




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
)
//--------------------------------------------------------------------------------------------------
{
    int ret = system(commandLine.c_str());

    if (ret != EXIT_SUCCESS)
    {
        std::stringstream buffer;

        buffer << "Command execution failure, exit code: " << ret <<".";

        throw legato::Exception(buffer.str());
    }
}




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
)
//--------------------------------------------------------------------------------------------------
{
    ExecuteCommandLine(commandLine.str());
}



//--------------------------------------------------------------------------------------------------
/**
 * Generates the identifier that is to be used for the component initialization function for a
 * given component.
 *
 * @return The name of the function.
 */
//--------------------------------------------------------------------------------------------------
std::string GetComponentInitName
(
    const legato::Component& component
)
//--------------------------------------------------------------------------------------------------
{
    return "_" + component.CName() + "_COMPONENT_INIT";
}



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
)
//--------------------------------------------------------------------------------------------------
{
    using namespace legato;

    static const char filePath[] = "$LEGATO_ROOT/interfaces/le_cfg.api";

    static const Api_t* apiPtr = parser::GetApiObject(DoEnvVarSubstitution(filePath), buildParams);

    return apiPtr->Hash();
}


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
)
//--------------------------------------------------------------------------------------------------
{
    using namespace legato;

    static const char filePath[] = "$LEGATO_ROOT/interfaces/le_wdog.api";

    static const Api_t* apiPtr = parser::GetApiObject(DoEnvVarSubstitution(filePath), buildParams);

    return apiPtr->Hash();
}


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
)
//--------------------------------------------------------------------------------------------------
{
    bool isDirectory = legato::DirectoryExists(sourcePath);

    // Generate the destination path in the build host's file system.
    std::string destPath = stagingDirPath + sandboxPath;

    // First we have to make sure that the containing directory exists in the staging area
    // before trying to copy anything into it.
    auto pos = destPath.rfind('/');
    legato::MakeDir(destPath.substr(0, pos));

    // Construct the copy shell command to use.
    std::string copyCommand = "cp";

    if (isDirectory)
    {
        copyCommand += " -r";
    }

    copyCommand += " \"";
    copyCommand += sourcePath;
    copyCommand += "\" \"";
    copyCommand += destPath;
    copyCommand += "\"";

    if (isVerbose)
    {
        std::cout << std::endl << "$ " << copyCommand << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(copyCommand);
}




}
