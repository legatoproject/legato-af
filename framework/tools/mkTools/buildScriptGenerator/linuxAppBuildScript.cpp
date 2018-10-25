//--------------------------------------------------------------------------------------------------
/**
 * @file linuxAppBuildScript.cpp
 *
 * Implementation of the build script generator for applications for Linux
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptCommon.h"
#include "linuxAppBuildScript.h"


namespace ninja
{

//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for bundling a given app's files into the
 * app's staging area.
 *
 * This adds Linux shared libraries and executables to the standard set of bundled files.
 **/
//--------------------------------------------------------------------------------------------------
void LinuxAppBuildScriptGenerator_t::GenerateStagingBundleBuildStatements
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto& allBundledFiles = appPtr->GetTargetInfo<target::FileSystemInfo_t>()->allBundledFiles;

    // First bundle all the standard bundled files for the app
    AppBuildScriptGenerator_t::GenerateStagingBundleBuildStatements(appPtr);

    // Now generate statements for bundling the component libraries into the app.
    for (auto componentPtr : appPtr->components)
    {
        // Generate a statement for bundling a component library into an application, if it has
        // a component library (which will only be the case if the component has sources).
        if ((componentPtr->HasCOrCppCode()) || (componentPtr->HasJavaCode()))
        {
            auto destPath = "$builddir/" + appPtr->workingDir
                          + "/staging/read-only/lib/" +
                path::GetLastNode(componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()->lib);
            auto lib = componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()->lib;

            // Copy the component library into the app's lib directory.
            // Cannot use hard link as this will cause builds to fail occasionally (LE-7383)
            script << "build " << destPath << " : BundleFile " << lib << "\n"
                   << "  modeFlags = " << baseGeneratorPtr->PermissionsToModeFlags(
                                                                 model::Permissions_t(true,
                                                                                      false,
                                                                                      true))
                   << "\n\n";

            // Add the component library to the set of bundled files.
            allBundledFiles.insert(model::FileSystemObject_t(
                                       lib,
                                       destPath,
                                       model::Permissions_t(true,
                                                            false,
                                                            componentPtr->HasCOrCppCode())));
        }
    }

    // Finally bundle all executables into the app
    for (auto& exeMapPtr : appPtr->executables)
    {
        auto exePtr = exeMapPtr.second;
        auto destPath = "$builddir/" + appPtr->workingDir
            + "/staging/read-only/bin/" + exePtr->name;
        auto exePath = "$builddir/" + exePtr->path;
        if (exePtr->hasJavaCode)
        {
            destPath += ".jar";
        }

        // Copy the component library into the app's lib directory.
        // Cannot use hard link as this will cause builds to fail occasionally (LE-7383)
        script << "build " << destPath << " : BundleFile " << exePath << "\n"
               << "  modeFlags = " << baseGeneratorPtr->PermissionsToModeFlags(
                                                            model::Permissions_t(true,
                                                                                 false,
                                                                                 true))
               << "\n\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for an application.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateLinux
(
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Minimize(buildParams.workingDir + "/build.ninja");

    LinuxAppBuildScriptGenerator_t appGenerator(filePath, buildParams);

    appGenerator.Generate(appPtr);
}


} // namespace ninja
