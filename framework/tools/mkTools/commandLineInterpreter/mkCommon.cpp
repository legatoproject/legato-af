//--------------------------------------------------------------------------------------------------
/**
 * @file mkCommon.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "mkCommon.h"
#include <string.h>
#include <unistd.h>


namespace
{


//----------------------------------------------------------------------------------------------
/**
 * Get the path to the toolchain
 *
 * @return  The path to the toolchain, or an empty string if not specified.
 */
//----------------------------------------------------------------------------------------------
std::string GetTargetEnvInfo
(
    const std::string& target,         ///< The target device type (e.g., wp85)
    const std::string& info            ///< The information to look for (TOOLCHAIN_DIR, ...)
)
//--------------------------------------------------------------------------------------------------
{
    // If the all-caps, target-specific tool path env var (e.g., "WP85_CC") is set, then use that.
    auto allCapsPrefix = target + "_";
    std::transform(allCapsPrefix.begin(), allCapsPrefix.end(), allCapsPrefix.begin(), ::toupper);
    auto value = envVars::Get(allCapsPrefix + info);
    if (!value.empty())
    {
        return value;
    }

    // Else, if the target-specific tool path env var (e.g., "wp85_CC") is set, then use that.
    value = envVars::Get(target + "_" + info);
    if (!value.empty())
    {
        return value;
    }

    // Else, if the tool path env var (e.g., "CC") is set, use that.
    value = envVars::Get(info);
    if (!value.empty())
    {
        return value;
    }

    return "";
}


//----------------------------------------------------------------------------------------------
/**
 * Get the path to a specific build tool.
 *
 * @return  The path to the tool, or an empty string if not specified.
 */
//----------------------------------------------------------------------------------------------
std::string GetToolPath
(
    const std::string& target,          ///< The target device type (e.g., wp85)
    const std::string& toolEnvVarName,  ///< Base name of tool path env var (e.g., "CC")
    bool provideDefault = true          ///< If the tool can't be found, should it provide a default?
)
//--------------------------------------------------------------------------------------------------
{
    auto path = GetTargetEnvInfo(target, toolEnvVarName);
    if (!path.empty())
    {
        return path;
    }

    // Else look for XXXX_TOOLCHAIN_DIR and/or XXXX_TOOLCHAIN_PREFIX environment variables, and
    // if they're set, use those to generate the tool path, assuming the toolchain is the
    // GNU Compiler Collection.
    auto toolChainDir = GetTargetEnvInfo(target, "TOOLCHAIN_DIR");
    auto toolChainPrefix = GetTargetEnvInfo(target, "TOOLCHAIN_PREFIX");

    // toolChainPrefix can be blank, but still valid
    if (toolEnvVarName == "CC")
    {
        path = path::Combine(toolChainDir, toolChainPrefix+"gcc");
    }
    else if (toolEnvVarName == "CXX")
    {
        path = path::Combine(toolChainDir, toolChainPrefix+"g++");
    }
    else if (provideDefault)
    {
        // By default, convert the tool env var name to all lowercase and use that as the
        // tool executable name.
        auto toolName = toolEnvVarName;
        std::transform(toolName.begin(), toolName.end(), toolName.begin(), ::tolower);
        path = path::Combine(toolChainDir, toolChainPrefix+toolName);
    }
    else
    {
        path = "";
    }

    return path;
}


//----------------------------------------------------------------------------------------------
/**
 * Get the sysroot path to use when linking for a given compiler.
 *
 * @return  The path to the sysroot base directory, or an empty string if not specified.
 *
 * @throws  mk::Exception_t on error.
 */
//----------------------------------------------------------------------------------------------
std::string GetSysRootPath
(
    const std::string& target,          ///< The target device type (e.g., wp85)
    const std::string& cCompilerPath    ///< Path to the C compiler
)
//--------------------------------------------------------------------------------------------------
{
    // If LEGATO_SYSROOT is set, use that.
    auto sysRoot = envVars::Get("LEGATO_SYSROOT");
    if (! sysRoot.empty())
    {
        return sysRoot;
    }

    // Else, if the target-specific XXXX_SYSROOT is set, then use that.
    sysRoot = GetTargetEnvInfo(target, "SYSROOT");
    if (! sysRoot.empty())
    {
        return sysRoot;
    }

    // Else, if the compiler is gcc, ask gcc what sysroot it uses by default.
    if (path::HasSuffix(cCompilerPath, "gcc"))
    {
        std::string commandLine = cCompilerPath + " --print-sysroot";

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
                mk::format(LE_I18N("** WARNING: Failed to receive sysroot path from compiler "
                                   "'%s' (errno: %s)."), commandLine, strerror(errno))
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
            throw mk::Exception_t(
                mk::format(LE_I18N("** WARNING: Invalid sysroot returned from compiler"
                                   " '%s' (returned '%s')."), commandLine, buffer));
        }

        // Close the connection and collect the exit code from the compiler.
        int result = pclose(output);

        if ( (result == -1) && (errno != EINTR) )
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Failed to receive the sysroot path from the compiler '%s'. "
                                   "pclose() errno = %s"), commandLine, strerror(errno))
            );
        }
        else if (!WIFEXITED(result))
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Failed to receive the sysroot path from the compiler '%s'. "
                                   "Compiler was interrupted by something."),
                           commandLine)
            );
        }
        else if (WEXITSTATUS(result) != EXIT_SUCCESS)
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("Failed to receive the sysroot path from the compiler '%s'. "
                                   "Compiler exited with code %d"),
                           commandLine, WEXITSTATUS(result))
            );
        }

        return buffer;
    }

    return sysRoot;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a list of cross tool paths. Currently assuming Yocto toolchain only. If different
 * toolchains are available in the future, the different ways to get the tool paths can be
 * case-switched according to the provided target.
 *
 * @return  A list of cross tool paths. Or an empty list if the toolchain directory can't be found.
 */
//--------------------------------------------------------------------------------------------------
std::list<std::string> GetCrossToolPaths
(
    const std::string& target           ///< The target device type (e.g., wp85)
)
//--------------------------------------------------------------------------------------------------
{
    std::list<std::string> crossToolPaths;
    auto toolChainDir = GetTargetEnvInfo(target, "TOOLCHAIN_DIR");

    if (toolChainDir.empty())
    {
        // return an empty list
        return crossToolPaths;
    }
    else
    {
        crossToolPaths.push_back(toolChainDir);
    }

    // Assuming Yocto toolchain; deducing toochain root for the host tools from the toolchain dir.
    std::string toolChainHostRoot = path::GetContainingDir(
                                    path::GetContainingDir(
                                    path::GetContainingDir(toolChainDir)));

    const char* relPaths[] = {
                              "/usr/bin",
                              "/usr/sbin",
                              "/bin",
                              "/sbin"
                             };

    for (size_t i = 0; i < sizeof(relPaths)/sizeof(relPaths[0]); i++)
    {
        auto toolChainHostDir = toolChainHostRoot + relPaths[i];
        if (file::DirectoryExists(toolChainHostDir))
        {
            crossToolPaths.push_back(toolChainHostDir);
        }
    }

    return crossToolPaths;
}


} // anonymous namespace


namespace cli
{


//--------------------------------------------------------------------------------------------------
/**
 * Figure out what compiler, linker, etc. to use based on the target device type and store that
 * info in the @c buildParams object.
 *
 * If a given tool is not specified through the means documented in
 * @ref buildToolsmk_ToolChainConfig, then the corresponding entry in @c buildParams will be
 * left empty.
 */
//--------------------------------------------------------------------------------------------------
void FindToolChain
(
    mk::BuildParams_t& buildParams  ///< [in,out] target member must be set before call
)
//--------------------------------------------------------------------------------------------------
{
    buildParams.cPreProcessorPath = GetToolPath(buildParams.target, "CPP");
    buildParams.cCompilerPath = GetToolPath(buildParams.target, "CC");
    buildParams.cxxCompilerPath = GetToolPath(buildParams.target, "CXX");
    buildParams.cppPath = GetToolPath(buildParams.target, "CPP");
    buildParams.toolChainDir = GetTargetEnvInfo(buildParams.target, "TOOLCHAIN_DIR");
    buildParams.toolChainPrefix = GetTargetEnvInfo(buildParams.target, "TOOLCHAIN_PREFIX");
    buildParams.sysrootDir = GetSysRootPath(buildParams.target, buildParams.cCompilerPath);
    buildParams.linkerPath = GetToolPath(buildParams.target, "LD");
    buildParams.archiverPath = GetToolPath(buildParams.target, "AR");
    buildParams.assemblerPath = GetToolPath(buildParams.target, "AS");
    buildParams.stripPath = GetToolPath(buildParams.target, "STRIP");
    buildParams.objcopyPath = GetToolPath(buildParams.target, "OBJCOPY");
    buildParams.readelfPath = GetToolPath(buildParams.target, "READELF");
    buildParams.compilerCachePath = GetToolPath(buildParams.target, "CCACHE", false);
    buildParams.crossToolPaths = GetCrossToolPaths(buildParams.target);

    if (buildParams.beVerbose)
    {
        std::cout << "C pre-processor = " << buildParams.cPreProcessorPath << std::endl;
        std::cout << "C compiler = " << buildParams.cCompilerPath << std::endl;
        std::cout << "C++ compiler = " << buildParams.cxxCompilerPath << std::endl;
        std::cout << "Preprocessor = " << buildParams.cppPath << std::endl;
        std::cout << "Compiler directory = " << buildParams.toolChainDir << std::endl;
        std::cout << "Compiler prefix = " << buildParams.toolChainPrefix << std::endl;
        std::cout << "Compiler sysroot = " << buildParams.sysrootDir << std::endl;
        std::cout << "Linker = " << buildParams.linkerPath << std::endl;
        std::cout << "Static lib archiver = " << buildParams.archiverPath << std::endl;
        std::cout << "Assembler = " << buildParams.assemblerPath << std::endl;
        std::cout << "Debug symbol stripper = " << buildParams.stripPath << std::endl;
        std::cout << "Object file copier/translator = " << buildParams.objcopyPath << std::endl;
        std::cout << "ELF file info extractor = " << buildParams.readelfPath << std::endl;
        std::cout << "Compiler cache = " << buildParams.compilerCachePath << std::endl;

        std::cout << "Cross tool paths = ";
        for (const auto &crossToolPath : buildParams.crossToolPaths)
        {
            std::cout << crossToolPath << ":";
        }
        std::cout << std::endl;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks the info in @c buildParams object for IMA signing. If nothing specified in @c buildParams
 * object then check environment variables for IMA signing and update @c buildParams object
 * accordingly.
 */
//--------------------------------------------------------------------------------------------------
void CheckForIMASigning
(
    mk::BuildParams_t& buildParams
)
{
    // No ima sign flag is provided in command line, so check environment variable
    // LE_CONFIG_ENABLE_IMA
    if (!buildParams.signPkg)
    {
        std::string imaEnable = envVars::Get("LE_CONFIG_ENABLE_IMA");
        buildParams.signPkg = (imaEnable.compare("1") == 0);
    }

    if (buildParams.signPkg)
    {
        // Get key values from environment variable if no path is specified
        if (buildParams.privKey.empty() && buildParams.pubCert.empty())
        {
            buildParams.privKey = envVars::Get("IMA_PRIVATE_KEY");
            if (buildParams.privKey.empty())
            {
                buildParams.privKey = envVars::GetRequired("LE_CONFIG_IMA_PRIVATE_KEY");
            }

            buildParams.pubCert = envVars::Get("IMA_PUBLIC_CERT");
            if (buildParams.pubCert.empty())
            {
                buildParams.pubCert = envVars::GetRequired("LE_CONFIG_IMA_PUBLIC_CERT");
            }
        }

        // Now checks whether private key exists or not. No need to check empty value, if privKey
        // string is empty FileExists() function should return false.
        if (!file::FileExists(buildParams.privKey))
        {
            throw mk::Exception_t(mk::format(LE_I18N("Bad private key location '%s'. Provide path "
                                             "via environment variable LE_CONFIG_IMA_PRIVATE_KEY "
                                             "or -K flag"),
                                             buildParams.privKey));
        }

        // Now checks whether public certificate exists or not
        if (!file::FileExists(buildParams.pubCert))
        {
            throw mk::Exception_t(mk::format(LE_I18N("Bad public certificate location '%s'. Provide"
                                             " path via environment variable "
                                             "LE_CONFIG_IMA_PUBLIC_CERT or -P flag"),
                                             buildParams.pubCert));
        }
    }
    else
    {
        if ((false == buildParams.privKey.empty()) || (false == buildParams.pubCert.empty()))
        {
            throw mk::Exception_t(LE_I18N("Wrong option. Sign (-S) option or environment variable "
                                  "LE_CONFIG_ENABLE_IMA must be set to sign the package."));
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Run the Ninja build tool.  Executes the build.ninja script in the root of the working directory
 * tree, if it exists.
 *
 * If the build.ninja file exists, this function will never return.  If the build.ninja file doesn't
 * exist, then this function WILL (quietly) return.
 *
 * @throw mk::Exception if the build.ninja file exists but ninja can't be run.
 */
//--------------------------------------------------------------------------------------------------
void RunNinja
(
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto ninjaFilePath = path::Combine(buildParams.workingDir, "build.ninja");

    if (file::FileExists(ninjaFilePath))
    {
        std::string ninjaArgvJobCount;
        const char* ninjaArgv[9] = {
            "ninja",
            "-f", ninjaFilePath.c_str(),
            (char*)NULL,
        };
        int ninjaArgvPos = 3;

        if (buildParams.jobCount > 0)
        {
            ninjaArgvJobCount = std::to_string(buildParams.jobCount);
            ninjaArgv[ninjaArgvPos++] = "-j";
            ninjaArgv[ninjaArgvPos++] = ninjaArgvJobCount.c_str();
            ninjaArgv[ninjaArgvPos] = NULL;
        }

        if (buildParams.beVerbose)
        {
            ninjaArgv[ninjaArgvPos++] = "-v";
            ninjaArgv[ninjaArgvPos++] = "-d";
            ninjaArgv[ninjaArgvPos++] = "explain";
            ninjaArgv[ninjaArgvPos] = NULL;

            std::cout << LE_I18N("Executing ninja build system...") << std::endl;
            std::cout << "$";
            for (int i = 0; ninjaArgv[i] != NULL; i++)
            {
                std::cout << " " << ninjaArgv[i];
            }
            std::cout << std::endl;
        }

        // According to the execvp prototype, it takes a char* const array.
        //   int execvp(const char *file, char *const argv[]);
        // Using a const_cast here since in practice, the array is not being
        // modified even though the protoype says it could.
        //
        // From http://pubs.opengroup.org/onlinepubs/9699919799/functions/fexecve.html :
        //   The statement about argv[] and envp[] being constants is included to make
        //   explicit to future writers of language bindings that these objects are
        //   completely constant. Due to a limitation of the ISO C standard, it is not
        //   possible to state that idea in standard C. Specifying two levels of const-
        //   qualification for the argv[] and  envp[] parameters for the exec functions
        //   may seem to be the natural choice, given that these functions do not modify
        //   either the array of pointers or the characters to which the function points,
        //   but this would disallow existing correct code.
        (void)execvp("ninja", const_cast<char **>(ninjaArgv));

        int errCode = errno;

        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to execute ninja (%s)."), strerror(errCode))
        );
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate Linux code for a given component.
 */
//--------------------------------------------------------------------------------------------------
void GenerateLinuxCode
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Create a working directory to build the component in.
    file::MakeDir(path::Combine(buildParams.workingDir, componentPtr->workingDir));

    // Generate a custom "interfaces.h" file for this component.
    code::GenerateInterfacesHeader(componentPtr, buildParams);

    // Generate a custom "_componentMain.c" file for this component.
    code::GenerateLinuxComponentMainFile(componentPtr, buildParams);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate RTOS code for a given component.
 */
//--------------------------------------------------------------------------------------------------
void GenerateRtosCode
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Create a working directory to build the component in.
    file::MakeDir(path::Combine(buildParams.workingDir, componentPtr->workingDir));

    // Generate a custom "interfaces.h" file for this component.
    code::GenerateInterfacesHeader(componentPtr, buildParams);

    // Generate a custom "_componentMain.c" file for this component.
    code::GenerateRtosComponentMainFile(componentPtr, buildParams);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate code for all the components in a given map.
 */
//--------------------------------------------------------------------------------------------------
void GenerateLinuxCode
(
    const std::map<std::string, model::Component_t*>& components,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    for (auto& mapEntry : components)
    {
        GenerateLinuxCode(mapEntry.second, buildParams);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate code for all the components in a given map.
 */
//--------------------------------------------------------------------------------------------------
void GenerateRtosCode
(
    const std::map<std::string, model::Component_t*>& components,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    for (auto& mapEntry : components)
    {
        GenerateRtosCode(mapEntry.second, buildParams);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate code specific to an individual app (excluding code for the components).
 */
//--------------------------------------------------------------------------------------------------
void GenerateLinuxCode
(
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Create the working directory, if it doesn't already exist.
    file::MakeDir(path::Combine(buildParams.workingDir, appPtr->workingDir));

    // Generate the configuration data file.
    config::Generate(appPtr, buildParams);

    // For each executable in the application,
    for (auto exePtr : appPtr->executables)
    {
        // Generate _main.c.
        code::GenerateLinuxExeMain(exePtr.second, buildParams);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate code specific to an individual app (excluding code for the components).
 */
//--------------------------------------------------------------------------------------------------
void GenerateRtosCode
(
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Create the working directory, if it doesn't already exist.
    file::MakeDir(path::Combine(buildParams.workingDir, appPtr->workingDir));

    // No configuration data file on RTOS -- everything which would be generated here is embedded
    // into the task.

    // For each executable in the application,
    for (auto exePtr : appPtr->executables)
    {
        // Generate _main.c.
        code::GenerateRtosExeMain(exePtr.second, buildParams);
    }
}


} // namespace cli
