//--------------------------------------------------------------------------------------------------
/**
 * Utility functions used by the mk tools.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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

//--------------------------------------------------------------------------------------------------
/**
 * Determine if we should build with clang.
 *
 * @return  Should we use clang ?
 */
//--------------------------------------------------------------------------------------------------
static bool ShouldUseClang
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    std::string useClang;

    useClang = GetEnvValue("USE_CLANG");

    if (useClang == "1")
    {
        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Determine if the compiler we are using is clang.
 *
 * @return  Is the specified compiler clang ?
 */
//--------------------------------------------------------------------------------------------------
bool IsCompilerClang
(
    const std::string& compilerPath ///< Path to the compiler
)
//--------------------------------------------------------------------------------------------------
{
    return (compilerPath == "clang");
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the file system path of the directory containing the cross-build tool chain for a given
 * target.
 *
 * @return The directory path.
 *
 * @throw Exception if the tool chain path cannot be determined.
 **/
//--------------------------------------------------------------------------------------------------
static std::string GetCrossBuildToolChainDir
(
    const std::string& target  ///< Name of the target platform (e.g., "localhost" or "ar7").
)
//--------------------------------------------------------------------------------------------------
{
    std::string varName;
    std::string envValue;

    varName = target;
    std::transform(varName.begin(), varName.end(), varName.begin(), ::toupper);

    varName += "_TOOLCHAIN_DIR";
    envValue = GetRequiredEnvValue(varName);

    if(envValue.empty())
    {
        throw std::runtime_error("Attempting to build for target '" + target + ", but '"
                                 + varName + "' is not set.");
    }

    return envValue;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the prefix of the cross-build tool chain.
 *
 * @return The tool chain prefix.
 **/
//--------------------------------------------------------------------------------------------------
static std::string GetCrossBuildToolChainPrefix
(
    const std::string& target  ///< Name of the target platform (e.g., "localhost" or "ar7").
)
//--------------------------------------------------------------------------------------------------
{
    std::string varName;
    std::string envValue;

    varName = target;
    std::transform(varName.begin(), varName.end(), varName.begin(), ::toupper);

    varName += "_TOOLCHAIN_PREFIX";
    envValue = GetEnvValue(varName);

    if(envValue.empty())
    {
        return "arm-poky-linux-gnueabi-";
    }

    return envValue;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the path for a tool from the cross-build tool chain.
 *
 * @return The tool chain prefix.
 **/
//--------------------------------------------------------------------------------------------------
static std::string GetCrossBuildToolPath
(
    const std::string& target,  ///< Name of the target platform (e.g., "localhost" or "ar7").
    const std::string& toolName
)
//--------------------------------------------------------------------------------------------------
{
    return legato::CombinePath(GetCrossBuildToolChainDir(target),
                               GetCrossBuildToolChainPrefix(target) + toolName);
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
    const std::string& target,  ///< Name of the target platform (e.g., "localhost" or "ar7").
    legato::ProgrammingLanguage_t language ///< The source code language.
)
//--------------------------------------------------------------------------------------------------
{
    std::string gnuCompiler;

    switch (language)
    {
        case legato::LANG_C:

            gnuCompiler = "gcc";
            break;

        case legato::LANG_CXX:

            gnuCompiler = "g++";
            break;
    }

    if (target == "localhost")
    {
        if (ShouldUseClang())
        {
            switch (language)
            {
                case legato::LANG_C:

                    return "clang";

                case legato::LANG_CXX:

                    return "clang++";
            }
        }

        return gnuCompiler;
    }

    return GetCrossBuildToolPath(target, gnuCompiler);
}



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
)
//--------------------------------------------------------------------------------------------------
{
    if (target == "localhost")
    {
        if(ShouldUseClang())
        {
            return "clang";
        }

        return "ld";
    }

    return GetCrossBuildToolPath(target, "ld");
}


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
)
//--------------------------------------------------------------------------------------------------
{
    if (target == "localhost")
    {
        return "ar";
    }

    return GetCrossBuildToolPath(target, "ar");
}


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
)
//--------------------------------------------------------------------------------------------------
{
    // If compiler is clang, skip sysroot determination
    if(IsCompilerClang(compilerPath))
    {
        return "/";
    }

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
)
//--------------------------------------------------------------------------------------------------
{
    if (legato::IsSharedLibrary(libraryPath))
    {
        std::string dir = legato::GetContainingDir(libraryPath);

        if (dir != "")
        {
            outputStream << " \"-L" << dir << "\"";
        }

        outputStream << " -l" << legato::LibraryShortName(libraryPath);
    }
    else
    {
        outputStream << " \"-l:" << legato::GetLastPathNode(libraryPath) << "\"";
    }
}



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
)
//--------------------------------------------------------------------------------------------------
{
    // Link with required libraries.
    for (auto lib : component.RequiredLibs())
    {
        outputStream << " -l" << lib;
    }

    // Link with bundled libraries.
    for (auto lib : component.BundledLibs())
    {
        mk::GetLinkDirectiveForLibrary(outputStream, lib);
    }

    // For each sub-component,
    for (const auto& mapEntry : component.SubComponents())
    {
        auto componentPtr = mapEntry.second;

        // If the component has itself been built into a library, link with that.
        if (componentPtr->Lib().Exists())
        {
            outputStream << " -l" << componentPtr->Lib().ShortName();
        }

        // Link with whatever this component depends on, bundles, or requires.
        GetComponentLibLinkDirectives(outputStream, *componentPtr);
    }
}


}  // namespace mk
