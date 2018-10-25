//--------------------------------------------------------------------------------------------------
/**
 * @file rtosAppBuildScript.cpp
 *
 * Implementation of the build script generator for applications for RTOSes
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptCommon.h"
#include "rtosAppBuildScript.h"


namespace ninja
{

//--------------------------------------------------------------------------------------------------
/**
 * Generate app build rules.
 **/
//--------------------------------------------------------------------------------------------------
void RtosAppBuildScriptGenerator_t::GenerateAppBuildRules
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    script <<
        // Add a bundled file into the app's staging area.
        "rule BundleFile\n"
        "  description = Bundling file\n"
        "  command = install -m $modeFlags $in $out\n"
        "\n"
        "rule StageApp\n"
        "  description = Staging app\n"
        "  command = touch $out\n"
        "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script the build statements for packing up everything into an application
 * bundle.
 **/
//--------------------------------------------------------------------------------------------------
void RtosAppBuildScriptGenerator_t::GenerateAppBundleBuildStatement
(
    model::App_t* appPtr,
    const std::string& outputDir    ///< Path to the directory into which the built app will be put.
)
//--------------------------------------------------------------------------------------------------
{
    // Give this a FS target info
    appPtr->SetTargetInfo(new target::FileSystemInfo_t());

    // On RTOS just bundle files into the staging directory.
    GenerateStagingBundleBuildStatements(appPtr);

    // Require all files to be staged to stage this app
    script <<
        "\n"
        "build $builddir/" + appPtr->workingDir + "/.staging.tag: StageApp";
    // This depends on all the bundled files and executables in the app.
    for (auto filePath : appPtr->GetTargetInfo<target::FileSystemInfo_t>()->allBundledFiles)
    {
        script << " " << filePath.destPath;
    }
    script << "\n\n";
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for an application.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateRtos
(
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Minimize(buildParams.workingDir + "/build.ninja");

    RtosAppBuildScriptGenerator_t appGenerator(filePath, buildParams);

    appGenerator.Generate(appPtr);
}


} // namespace ninja
