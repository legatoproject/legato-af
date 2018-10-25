//--------------------------------------------------------------------------------------------------
/**
 * @file componentBuildScript.cpp
 *
 * Component build script generation functions.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptCommon.h"
#include "linuxComponentBuildScript.h"


namespace ninja
{


//--------------------------------------------------------------------------------------------------
/**
 * Linux components depend on any libraries built from other components.
 *
 * @note This is recursive if the component depends on any other components.
 **/
//--------------------------------------------------------------------------------------------------
void LinuxComponentBuildScriptGenerator_t::GetImplicitDependencies
(
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (componentPtr->HasCOrCppCode())
    {
        // Build a path to the Legato library dir and the Legato lib so that we can add it as a
        // dependency for code components.
        auto baseLibPath = path::Combine(envVars::Get("LEGATO_ROOT"),
                                         "build/" + buildParams.target + "/framework/lib/");

        // Add liblegato
        auto liblegatoPath = path::Combine(baseLibPath, "liblegato.so");

        // Changes to liblegato should trigger re-linking of the component library.
        componentPtr->implicitDependencies.insert(liblegatoPath);
    }

    // For each sub-component,
    for (auto subComponentPtr : componentPtr->subComponents)
    {
        // If the sub-component has itself been built into a library, the component depends
        // on that sub-component library.
        if (subComponentPtr.componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()->lib != "")
        {
            script << " "
                   << subComponentPtr.componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()
                                                  ->lib;
        }
    }

    ComponentBuildScriptGenerator_t::GetImplicitDependencies(componentPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the contents that are common to both cFlags and cxxFlags variable
 * definitions for a given Component.
 **/
//--------------------------------------------------------------------------------------------------
void LinuxComponentBuildScriptGenerator_t::GenerateCommonCAndCxxFlags
(
    model::Component_t* componentPtr
)
{
    ComponentBuildScriptGenerator_t::GenerateCommonCAndCxxFlags(componentPtr);

    // Generate log session variable, and log filter variable.
    script << " -DLE_LOG_SESSION=" << componentPtr->name << "_LogSession ";
    script << " -DLE_LOG_LEVEL_FILTER_PTR=" << componentPtr->name << "_LogLevelFilterPtr ";
}



//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the ldFlags variable contents needed to tell the linker to link
 * with libraries that a given Component depends on.
 *
 * @note This is recursive if the component depends on any other components.
 **/
//--------------------------------------------------------------------------------------------------
void LinuxComponentBuildScriptGenerator_t::GetDependentLibLdFlags
(
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto subComponentPtr : componentPtr->subComponents)
    {
        // If the component has itself been built into a library, link with that.
        if (subComponentPtr.componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()->lib != "")
        {
            script << " \"-L"
                   << path::GetContainingDir(
                       subComponentPtr.componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()
                                                   ->lib
                      )
                   << "\"";

            script << " -l"
                   << path::GetLibShortName(
                       subComponentPtr.componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()
                                                   ->lib
                   );
        }

        // If the component has an external build, add the external build's working directory.
        if (subComponentPtr.componentPtr->HasExternalBuild())
        {
            script << " \"-L$builddir" << subComponentPtr.componentPtr->dir << "\"";
        }

        // Link with whatever this component depends on.
        GetDependentLibLdFlags(subComponentPtr.componentPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the ldFlags variable definition for a given Component.
 **/
//--------------------------------------------------------------------------------------------------
void LinuxComponentBuildScriptGenerator_t::GenerateLdFlagsDef
(
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    script << "  ldFlags = ";
    script << buildParams.ldFlags;

    // Add the ldflags from the Component.cdef file.
    for (auto& arg : componentPtr->ldFlags)
    {
        script << " " << arg;
    }

    // Add the library output directory to the list of places to search for libraries to link with.
    if (!buildParams.libOutputDir.empty())
    {
        script << " -L" << buildParams.libOutputDir;
    }

    // Set the DT_RUNPATH variable inside the executable's ELF headers to include the expected
    // on-target runtime locations of the libraries needed.
    GenerateRunPathLdFlags();

    // Includes a list of -l directives for all the libraries the component needs.
    GetDependentLibLdFlags(componentPtr);

    // Link with the standard runtime libs.
    script << " \"-L$$LEGATO_BUILD/framework/lib\" -llegato -lpthread -lrt -lm\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script a build statement for building a given component's library.
 **/
//--------------------------------------------------------------------------------------------------
void LinuxComponentBuildScriptGenerator_t::GenerateComponentLinkStatement
(
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    std::string rule;

    // Determine which rules should be used for building the component.
    if (!componentPtr->cxxObjectFiles.empty())
    {
        rule = "LinkCxxLib";
    }
    else if (!componentPtr->cObjectFiles.empty())
    {
        rule = "LinkCLib";
    }
    else
    {
        // No source files.  No library to build.
        return;
    }
    // Create the build statement.
    script << "build " << componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()->lib
           << ": " << rule;

    // Add source dependencies
    GetObjectFiles(componentPtr);
    std::set<std::string> commonObjects;
    GetCommonApiFiles(componentPtr, commonObjects);
    for (auto const &commonObject : commonObjects)
    {
        script << " $builddir/" << commonObject;
    }

    // Add implicit dependencies.
    script << " |";
    GetImplicitDependencies(componentPtr);
    GetExternalDependencies(componentPtr);
    script << "\n";

    // Define the ldFlags variable.
    GenerateLdFlagsDef(componentPtr);

    script << "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for building a single component.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateLinux
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Combine(buildParams.workingDir, "build.ninja");

    LinuxComponentBuildScriptGenerator_t componentGenerator(filePath, buildParams);

    componentGenerator.Generate(componentPtr);
}


} // namespace ninja
