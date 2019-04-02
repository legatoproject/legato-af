//--------------------------------------------------------------------------------------------------
/**
 * @file systemBuildScript.cpp
 *
 * Implementation of the build script generator for systems.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptCommon.h"
#include "exeBuildScript.h"
#include "appBuildScript.h"
#include "moduleBuildScript.h"
#include "componentBuildScript.h"
#include "systemBuildScript.h"

//--------------------------------------------------------------------------------------------------
/**
 * Constant to use as a symlink target (in place of MD5-based directory name), when the app is
 * preloaded and its version may not match the latest version.
 */
//--------------------------------------------------------------------------------------------------
#define PRELOADED_ANY_VERSION "PRELOADED_ANY_VERSION"

namespace ninja
{


//--------------------------------------------------------------------------------------------------
/**
 * Generate comment header for a system build script.
 */
//--------------------------------------------------------------------------------------------------
void SystemBuildScriptGenerator_t::GenerateCommentHeader
(
    model::System_t* systemPtr
)
//--------------------------------------------------------------------------------------------------
{
    script  << "# Build script for system '" << systemPtr->name << "'\n"
            "\n"
            "# == Auto-generated file.  Do not edit. ==\n"
            "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for the build script itself.
 **/
//--------------------------------------------------------------------------------------------------
void SystemBuildScriptGenerator_t::GenerateNinjaScriptBuildStatement
(
    model::System_t* systemPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Create a set of dependencies.
    std::set<std::string> dependencies;

    // Add the .sdef file to the dependencies.
    dependencies.insert(systemPtr->defFilePtr->path);

    // For each module in the system
    for (auto& mapEntry: systemPtr->modules)
    {
        // Add the .mdef file to dependencies.
        dependencies.insert(mapEntry.second.modPtr->defFilePtr->path);
    }

    // For each app in the system,
    for (auto& mapEntry : systemPtr->apps)
    {
        // Add the .adef file to the dependencies.
        dependencies.insert(mapEntry.second->defFilePtr->path);
    }

    // For each component in the system,
    for (auto& mapEntry : model::Component_t::GetComponentMap())
    {
        auto componentPtr = mapEntry.second;

        // Add the .cdef file to the dependencies.
        dependencies.insert(componentPtr->defFilePtr->path);

        // Add all the .api files to the dependencies.
        for (auto ifPtr : componentPtr->typesOnlyApis)
        {
            dependencies.insert(ifPtr->apiFilePtr->path);
        }
        for (auto ifPtr : componentPtr->serverApis)
        {
            dependencies.insert(ifPtr->apiFilePtr->path);
        }
        for (auto ifPtr : componentPtr->clientApis)
        {
            dependencies.insert(ifPtr->apiFilePtr->path);
        }
        for (auto apiFilePtr : componentPtr->clientUsetypesApis)
        {
            dependencies.insert(apiFilePtr->path);
        }
        for (auto apiFilePtr : componentPtr->serverUsetypesApis)
        {
            dependencies.insert(apiFilePtr->path);
        }
    }
    // It also depends on changes to the mk tools.
    dependencies.insert(path::Combine(envVars::Get("LEGATO_ROOT"), "build/tools/bin/mk"));

    baseGeneratorPtr->GenerateNinjaScriptBuildStatement(dependencies);
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate build rules required for a system
 */
//--------------------------------------------------------------------------------------------------
void SystemBuildScriptGenerator_t::GenerateBuildRules
(
    model::System_t* systemPtr
)
{
    appGeneratorPtr->GenerateBuildRules();
    GenerateSystemBuildRules(systemPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate build script for system
 */
//--------------------------------------------------------------------------------------------------
void SystemBuildScriptGenerator_t::Generate
(
    model::System_t* systemPtr
)
{
    // Start the script with a comment
    GenerateCommentHeader(systemPtr);

    // Add file-level variable definitions.
    std::string includes;
    includes = " -I " + buildParams.workingDir;
    for (const auto& dir : buildParams.interfaceDirs)
    {
        includes += " -I" + dir;
    }

    auto buildDir = path::MakeAbsolute(buildParams.workingDir);

    script << "builddir = " << buildDir << "\n\n";
    script << "stagingDir = " << path::Combine(buildDir, "staging") << "\n\n";
    script << "cFlags = " << buildParams.cFlags << includes << "\n\n";
    script << "cxxFlags = " << buildParams.cxxFlags << includes << "\n\n";
    script << "ldFlags = " << buildParams.ldFlags << "\n\n";
    script << "target = " << buildParams.target << "\n\n";
    GenerateBuildRules(systemPtr);

    // If we are not just generating code,
    if (!buildParams.codeGenOnly)
    {
        // For each module in .sdef file
        for (auto& mapEntry : systemPtr->modules)
        {
            auto modulePtr = mapEntry.second.modPtr;

            moduleGeneratorPtr->GenerateBuildStatements(modulePtr);
        }

        // For each app built by the mk tools for this system,
        for (auto& mapEntry : systemPtr->apps)
        {
            auto appPtr = mapEntry.second;

            // Generate build statements for the app's executables.
            appGeneratorPtr->GenerateExeBuildStatements(appPtr);

            // Generate build statements for bundling files into the app's staging area.
            auto appWorkingDir = "$builddir/" + appPtr->workingDir;
            appGeneratorPtr->GenerateAppBundleBuildStatement(appPtr, appWorkingDir);
        }

        // For each component in the system.
        for (auto& mapEntry : model::Component_t::GetComponentMap())
        {
            componentGeneratorPtr->GenerateBuildStatements(mapEntry.second);
            componentGeneratorPtr->GenerateIpcBuildStatements(mapEntry.second);
        }

        // Generate build statement for packing everything into a system update pack.
        GenerateSystemPackBuildStatement(systemPtr);
    }

    // Add a build statement for the build.ninja file itself.
    GenerateNinjaScriptBuildStatement(systemPtr);
}

} // namespace ninja
