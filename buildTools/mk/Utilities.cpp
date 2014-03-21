//--------------------------------------------------------------------------------------------------
/**
 * Utility functions used by the mk tools.
 *
 * Copyright (C) 201 Sierra Wireless Inc., all rights reserved.
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

extern "C" {
    #include <unistd.h>
}


namespace mk
{


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
 * @return  The target.
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

    throw std::runtime_error("Attempting to build for target '" + target + ", but '" + varName + "' is not set.");
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



#if 0
//--------------------------------------------------------------------------------------------------
/**
 *  .
 */
//--------------------------------------------------------------------------------------------------
std::string IpcLibName
(
    const std::string& componentName,  ///
    const std::string& niceName,       ///
    legato::InterfaceRef::Type type    ///
)
{
    return componentName + "." + niceName + ".ipc." + IpcTypeStr(niceName, type);
}




//--------------------------------------------------------------------------------------------------
/**
 *  .
 */
//--------------------------------------------------------------------------------------------------
std::string IpcInitFuncName
(
    const std::string& componentName,  ///
    const std::string& niceName,       ///
    legato::InterfaceRef::Type type    ///
)
{
    std::string name;

    switch (type)
    {
        case legato::InterfaceRef::IMPORT:
            name = "Client";
            break;

        case legato::InterfaceRef::EXPORT:
            name = "Server";
            break;

        default:
            throw std::runtime_error("Unknown interface type for, '" + niceName + ".'");
    }

    return /*componentName + "_" +*/ niceName + "_Start" + name;
}



#endif



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
 */
//--------------------------------------------------------------------------------------------------
std::string GetComponentInitName
(
    const legato::Component& component
)
//--------------------------------------------------------------------------------------------------
{
    return "_" + component.CName() + "_Init_Function";
}




//--------------------------------------------------------------------------------------------------
/**
 * Generate IPC API code for a given protocol in a given directory.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateApiCode
(
    const std::string& instanceName,    ///< Name of the interface instance.
    const std::string& protocolFile,    ///< Path to the .api file.
    const std::string& outputDir,       ///< Directory into which the library will go.
    const std::string& clientOrServer,  ///< "client" or "server"
    bool isVerbose                      ///< If true, write troubleshooting info to stdout.
)
//--------------------------------------------------------------------------------------------------
{
    std::stringstream commandLine;

    // Use the ifgen tool to generate the API code.
    commandLine << "ifgen --gen-local --gen-interface";

    if(clientOrServer == "server")
    {
        commandLine << " --gen-server --gen-server-interface";
    }
    else if(clientOrServer == "client")
    {
        commandLine << " --gen-client";
    }

    // Set the C identifier prefix to the instance name, followed by an underscore.
    commandLine << " --name-prefix " << instanceName << "_";

    // Set the generated file name prefix to the instance name, followed by an underscore.
    commandLine << " --file-prefix " << instanceName << "_";

    // Specify the output directory.
    commandLine << " --output-dir " << outputDir;

    // Specify the path to the protocol file.
    commandLine << " " << protocolFile;

    if (isVerbose)
    {
        std::cout << commandLine.str() << std::endl;
    }

    // Execute the command.
    ExecuteCommandLine(commandLine.str());
}



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
)
//--------------------------------------------------------------------------------------------------
{
    GenerateApiCode(instanceName, protocolFile, outputDir, "client", isVerbose);
}




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
)
//--------------------------------------------------------------------------------------------------
{
    GenerateApiCode(instanceName, protocolFile, outputDir, "server", isVerbose);
}



}
