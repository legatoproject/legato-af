//--------------------------------------------------------------------------------------------------
/**
 * @file moduleBuildScript.cpp
 *
 * Implementation of the build script generator for pre-built kernel modules.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptCommon.h"
#include "moduleBuildScript.h"

namespace ninja
{



//--------------------------------------------------------------------------------------------------
/**
 * Generate comment header for a build script.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateCommentHeader
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const model::Module_t* modulePtr
)
//--------------------------------------------------------------------------------------------------
{
    script  << "# Build script for module '" << modulePtr->name << "'\n"
               "\n"
               "# == Auto-generated file.  Do not edit. ==\n"
               "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the build statements related to a given module.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateBuildStatements
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const model::Module_t* modulePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Add build statement for copying the .ko file
    script << "build " << "$builddir/" << modulePtr->objFilePtr->path << ": "
           << "CopyF " << modulePtr->path << "\n"
           << "  modeFlags = u+rw-x,g+r-wx,o+r-wx\n"
           "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for the build script itself.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateNinjaScriptBuildStatement
(
    std::ofstream& script,
    const model::Module_t* modulePtr,
    const std::string& filePath     ///< Path to the build.ninja file.
)
//--------------------------------------------------------------------------------------------------
{
    // Generate a build statement for the build.ninja.
    script << "build " << filePath << ": RegenNinjaScript |";

    // The build.ninja depends on module .mdef and .ko files
    // Create a set of dependencies.
    std::set<std::string> dependencies;
    dependencies.insert(modulePtr->path);
    dependencies.insert(modulePtr->defFilePtr->path);
    // It also depends on changes to the mk tools.
    dependencies.insert(path::Combine(envVars::Get("LEGATO_ROOT"), "build/tools/mk"));

    // Write the dependencies to the script.
    for (auto dep : dependencies)
    {
        script << " " << dep;
    }
    script << "\n\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for a pre-built kernel module.
 *
 * @note TBD: This will be used by mkmod.
 **/
//--------------------------------------------------------------------------------------------------
void Generate
(
    const model::Module_t* modulePtr,
    const mk::BuildParams_t& buildParams,
    int argc,           ///< Count of the number of command line parameters.
    const char** argv   ///< Pointer to array of pointers to command line argument strings.
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Minimize(buildParams.workingDir + "/build.ninja");

    std::ofstream script;
    OpenFile(script, filePath, buildParams.beVerbose);

    // Start the script with a comment, the file-level variable definitions, and
    // a set of generic rules.
    GenerateCommentHeader(script, modulePtr);
    script << "builddir = " << buildParams.workingDir << "\n\n";
    script << "target = " << buildParams.target << "\n\n";
    GenerateIfgenFlagsDef(script, buildParams.interfaceDirs);
    GenerateBuildRules(script, buildParams.target, argc, argv);

    if (!buildParams.codeGenOnly)
    {
        // Add build statements for the module.
        GenerateBuildStatements(script, modulePtr, buildParams);
    }

    // Add a build statement for the build.ninja file itself.
    GenerateNinjaScriptBuildStatement(script, modulePtr, filePath);

    CloseFile(script);
}


} // namespace ninja
