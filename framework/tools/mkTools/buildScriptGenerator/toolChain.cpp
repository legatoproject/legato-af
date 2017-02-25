//--------------------------------------------------------------------------------------------------
/**
 * @file toolChain.cpp
 *
 * Implementation of tool chain related functions needed by the build script generator.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include <algorithm>
#include <string.h>

#include "mkTools.h"


namespace ninja
{


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

    useClang = envVars::Get("USE_CLANG");

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
 * @throw Exception_t if the tool chain path cannot be determined.
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
    envValue = envVars::GetRequired(varName);

    if(envValue.empty())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Attempting to build for target '%s', but '%s' is not set."),
                       target, varName)
        );
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
    envValue = envVars::Get(varName);

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
    return path::Combine(GetCrossBuildToolChainDir(target),
                         GetCrossBuildToolChainPrefix(target) + toolName);
}


//----------------------------------------------------------------------------------------------
/**
 * Get the command-line path to use to invoke the (cross) C compiler for a given target.
 *
 * @return  The path to the compiler.
 *
 * @throw   mk::Exception_t if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetCCompilerPath
(
    const std::string& target   ///< Name of the target platform (e.g., "localhost" or "ar7").
)
//--------------------------------------------------------------------------------------------------
{
    std::string gnuCompiler;

    gnuCompiler = "gcc";

    if (target == "localhost")
    {
        if (ShouldUseClang())
        {
            return "clang";
        }

        return gnuCompiler;
    }

    return GetCrossBuildToolPath(target, gnuCompiler);
}


//----------------------------------------------------------------------------------------------
/**
 * Get the command-line path to use to invoke the (cross) C++ compiler for a given target.
 *
 * @return  The path to the compiler.
 *
 * @throw   mk::Exception_t if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetCxxCompilerPath
(
    const std::string& target   ///< Name of the target platform (e.g., "localhost" or "ar7").
)
//--------------------------------------------------------------------------------------------------
{
    std::string gnuCompiler;

    gnuCompiler = "g++";

    if (target == "localhost")
    {
        if (ShouldUseClang())
        {
            return "clang++";
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
 * @throw   mk::Exception_t if target not recognized.
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
 * @throw   mk::Exception_t if target not recognized.
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
 * @throw   mk::Exception_t if target not recognized.
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
        return "";
    }

    std::string commandLine = compilerPath + " --print-sysroot";

    FILE* output = popen(commandLine.c_str(), "r");

    if (output == NULL)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Could not exec '%s' to get sysroot path."), commandLine)
        );
    }

    char buffer[1024] = { 0 };
    static const size_t bufferSize = sizeof(buffer);

    if (fgets(buffer, bufferSize, output) != buffer)
    {
        std::cerr <<
            mk::format(LE_I18N("** WARNING: Failed to receive sysroot path from compiler '%s' "
                               "(errno: %s)."), compilerPath, strerror(errno))
                  << std::endl;
        buffer[0] = '\0';
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

    // Yocto >= 1.8 returns '/not/exist' as a sysroot path
    if (buffer == std::string("/not/exist"))
    {
        std::cerr << mk::format(LE_I18N("** WARNING: Invalid sysroot returned from compiler '%s' "
                                        "(returned '%s')."), compilerPath, buffer)
                  << std::endl;
        buffer[0] = '\0';
    }

    // Close the connection and collect the exit code from the compiler.
    int result = pclose(output);

    if (result == -1)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to receive the sysroot path from the compiler '%s'. "
                               "pclose() errno = %s"), compilerPath, strerror(errno))
        );
    }
    else if (!WIFEXITED(result))
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to receive the sysroot path from the compiler '%s'. "
                               "Compiler was interrupted by something."),
                       compilerPath)
        );
    }
    else if (WEXITSTATUS(result) != EXIT_SUCCESS)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to receive the sysroot path from the compiler '%s'. "
                               "Compiler exited with code %d"),
                       compilerPath, WEXITSTATUS(result))
        );
    }

    return buffer;
}



} // namespace ninja
