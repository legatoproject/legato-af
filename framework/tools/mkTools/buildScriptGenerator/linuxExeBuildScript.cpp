//--------------------------------------------------------------------------------------------------
/**
 * @file exeBuildScript.cpp
 *
 * Implementation of the build script generator for executables built using mkexe.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "linuxExeBuildScript.h"

namespace ninja
{

//--------------------------------------------------------------------------------------------------
/**
 * Get the rule to be used to link an executable.
 *
 * @return String containing the name of the rule.
 **/
//--------------------------------------------------------------------------------------------------
std::string LinuxExeBuildScriptGenerator_t::GetLinkRule
(
    model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    if (exePtr->cxxObjectFiles.empty())
    {
        return "LinkCExe";
    }
    else
    {
        return "LinkCxxExe";
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the contents that are common to both cFlags and cxxFlags variable
 * definitions for a given executable's .o file build statements.
 **/
//--------------------------------------------------------------------------------------------------
void LinuxExeBuildScriptGenerator_t::GenerateCandCxxFlags
(
    model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    ExeBuildScriptGenerator_t::GenerateCandCxxFlags(exePtr);

    std::string componentName = exePtr->name + "_exe";

    // Define log session variable, and log filter variable.
    script << " -DLE_LOG_SESSION=" << componentName << "_LogSession ";
    script << " -DLE_LOG_LEVEL_FILTER_PTR=" << componentName << "_LogLevelFilterPtr ";
}

//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the ldFlags variable contents needed to tell the linker to link
 * with libraries that a given executable depends on.
 **/
//--------------------------------------------------------------------------------------------------
void LinuxExeBuildScriptGenerator_t::GetDependentLibLdFlags
(
    model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    // Traverse the component instance list in reverse order.
    auto& list = exePtr->componentInstances;
    for (auto i = list.rbegin(); i != list.rend(); i++)
    {
        auto componentPtr = (*i)->componentPtr;
        auto& lib = componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()->lib;

        // If the component has itself been built into a library, link with that.
        if (lib != "")
        {
            script << " \"-L" << path::GetContainingDir(lib) << "\"";

            script << " -l" << path::GetLibShortName(lib);
        }

        // If the component has an external build, add the external build's working directory.
        if (componentPtr->HasExternalBuild())
        {
            script << " \"-L" << path::Combine(buildParams.workingDir,
                                               componentPtr->workingDir) << "\"";
        }

        // If the component has ldFlags defined in its .cdef file, add those too.
        for (auto& arg : componentPtr->ldFlags)
        {
            script << " " << arg;
        }
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script a build statement for a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void LinuxExeBuildScriptGenerator_t::GenerateBuildStatement
(
    model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    auto exePath = exePtr->path;

    if (!path::IsAbsolute(exePath))
    {
        exePath = "$builddir/" + exePath;
    }
    script << "build " << exePath << ": " << GetLinkRule(exePtr) <<
              " $builddir/" << exePtr->MainObjectFile().path;

    // Link in all the .o files for C/C++ sources.
    for (auto objFilePtr : exePtr->cObjectFiles)
    {
        script << " $builddir/" << objFilePtr->path;
    }
    for (auto objFilePtr : exePtr->cxxObjectFiles)
    {
        script << " $builddir/" << objFilePtr->path;
    }

    // Declare the exe's (implicit) dependencies, including all the components' shared libraries
    // and liblegato.  Collect a set of static libraries needed by the components, while we are
    // looking at them.
    std::set<std::string> staticLibs;
    script << " |";
    if (!exePtr->componentInstances.empty())
    {
        for (auto componentInstancePtr : exePtr->componentInstances)
        {
            auto componentPtr = componentInstancePtr->componentPtr;
            script << " " << componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()->lib;

            for (const auto& dependency : componentPtr->implicitDependencies)
            {
                script << " " << dependency;
            }

            for (const auto& lib : componentPtr->staticLibs)
            {
                staticLibs.insert(lib);
            }
        }
    }
    script << " " << path::Combine(envVars::Get("LEGATO_ROOT"),
                                   "build/$target/framework/lib/liblegato.so");
    script << "\n";

    // Define an exe-specific ldFlags variable that adds all the components' and interfaces'
    // shared libraries to the linker command line.
    script << "  ldFlags =";

    // Make the executable able to export symbols to dynamic shared libraries that get loaded.
    // This is needed so the executable can define executable-specific interface name variables
    // for component libraries to use.
    script << " -rdynamic";

    // Set the DT_RUNPATH variable inside the executable's ELF headers to include the expected
    // on-target runtime locations of the libraries needed.
    componentGeneratorPtr->GenerateRunPathLdFlags();

    // Link with all the static libraries that the components need.
    for (const auto& lib : staticLibs)
    {
        script << " " << lib;
    }

    // Add the library output directory to the list of places to search for libraries to link with.
    script << " -L" << buildParams.libOutputDir;

    // Include a list of -l directives for all the libraries the executable needs.
    GetDependentLibLdFlags(exePtr);

    // Link again with all the static libraries that the components need, in case there are
    // dynamic libraries that need symbols from them, or in case there are interdependencies
    // between them.
    for (const auto& lib : staticLibs)
    {
        script << " " << lib;
    }

    // Include another list of -l directives for all the libraries the executable needs.
    GetDependentLibLdFlags(exePtr);

    // Link with the standard runtime libs.
    script << " \"-L$$LEGATO_BUILD/framework/lib\" -llegato -lpthread -lrt -ldl -lm";

    // Add ldFlags from earlier definition.
    script << " $ldFlags\n"
              "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for an executable and associated component and IPC interface
 * libraries.
 *
 * @note This is only used by mkexe.
 */
//--------------------------------------------------------------------------------------------------
void GenerateLinux
(
    model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Minimize(buildParams.workingDir + "/build.ninja");

    LinuxExeBuildScriptGenerator_t scriptGenerator(filePath, buildParams);

    scriptGenerator.Generate(exePtr);
}

}
