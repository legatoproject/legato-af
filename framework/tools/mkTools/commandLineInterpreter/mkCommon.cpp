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

namespace cli
{


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
        if (buildParams.beVerbose)
        {
            std::cout << "Executing ninja build system..." << std::endl;
            std::cout << "$ ninja -v -d explain -f " << ninjaFilePath << std::endl;

            (void)execlp("ninja",
                         "ninja",
                         "-v",
                         "-d", "explain",
                         "-f", ninjaFilePath.c_str(),
                         (char*)NULL);

            // REMINDER: If you change the list of arguments passed to ninja, don't forget to
            //           update the std::cout message above that to match your changes.
        }
        else
        {
            (void)execlp("ninja",
                         "ninja",
                         "-f", ninjaFilePath.c_str(),
                         (char*)NULL);
        }

        int errCode = errno;

        throw mk::Exception_t("Failed to execute ninja ("
                                   + std::string(strerror(errCode)) + ").");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate code for a given component.
 */
//--------------------------------------------------------------------------------------------------
void GenerateCode
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
    code::GenerateComponentMainFile(componentPtr, buildParams, false);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate code for all the components in a given set.
 */
//--------------------------------------------------------------------------------------------------
void GenerateCode
(
    const std::set<model::Component_t*>& components,  ///< Set of components to generate code for.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    for (auto componentPtr : components)
    {
        GenerateCode(componentPtr, buildParams);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate code for all the components in a given map.
 */
//--------------------------------------------------------------------------------------------------
void GenerateCode
(
    const std::map<std::string, model::Component_t*>& components,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    for (auto& mapEntry : components)
    {
        GenerateCode(mapEntry.second, buildParams);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate code specific to an individual app (excluding code for the components).
 */
//--------------------------------------------------------------------------------------------------
void GenerateCode
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
        code::GenerateExeMain(exePtr.second, buildParams);
    }

    // Generate the manifest.app file for Air Vantage.
    airVantage::GenerateManifest(appPtr, buildParams);
}



} // namespace cli
