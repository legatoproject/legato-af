//--------------------------------------------------------------------------------------------------
/**
 * @file moduleBuildScript.cpp
 *
 * Implementation of the build script generator for pre-built kernel modules.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
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
 * If it's a pre-built module, just copy it. Otherwise generate a module Makefile and build it.
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
    script << "build " << "$builddir/" << modulePtr->koFilePtr->path << ": ";

    if (modulePtr->path.empty() || !file::FileExists(modulePtr->path))
    {
        // No pre-built module: generate and invoke a Makefile
        GenerateMakefile(modulePtr, buildParams);
        script << "MakeKernelModule " << "$builddir/"
               << path::GetContainingDir(modulePtr->koFilePtr->path) << "\n";
    }
    else
    {
        // Pre-built module: add build statement for copying the .ko file
        script << "CopyF " << modulePtr->path << "\n"
               << "  modeFlags = u+rw-x,g+r-wx,o+r-wx\n";
    }
    script << "\n";
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
 * Generate a Makefile for a kernel module.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateMakefile
(
    const model::Module_t* modulePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string buildPath = path::MakeAbsolute(buildParams.workingDir
                                + "/modules/" + modulePtr->name);
    const std::string& compilerPath = buildParams.cCompilerPath;

    std::ofstream makefile;
    OpenFile(makefile, buildPath + "/Makefile", buildParams.beVerbose);

    // Specify kernel module name and list all object files to link
    makefile << "obj-m += " << modulePtr->name << ".o\n";

    // Don't list object files in case of a single source file with module name
    if (modulePtr->cObjectFiles.size() > 1 ||
        modulePtr->cObjectFiles.front()->path != modulePtr->name + ".o")
    {
        for (auto obj : modulePtr->cObjectFiles)
        {
            makefile << modulePtr->name << "-objs += " << obj->path << "\n";
        }
    }
    makefile << "\n";

    // Specify directory where the sources are located
    makefile << "src = " << modulePtr->dir << "\n\n";

    // Add compiler and linker options
    for (auto obj : modulePtr->cFlags)
    {
        makefile << "ccflags-y += " << obj << "\n";
    }
    for (auto obj : modulePtr->ldFlags)
    {
        makefile << "ldflags-y += " << obj << "\n";
    }
    makefile << "\n";

    makefile << "KBUILD := " << modulePtr->kernelDir << "\n";

    if (buildParams.target != "localhost")
    {
        // Specify the CROSS_COMPILE and ARCH environment variables
        // Note: compiler path may contain dashes in directory names
        std::string compiler = path::GetLastNode(compilerPath);
        std::string cross = path::GetContainingDir(compilerPath) + "/"
                                + compiler.substr(0, compiler.rfind('-') + 1);
        std::string arch = compiler.substr(0, compiler.find('-'));

        makefile << "export CROSS_COMPILE := " << cross << "\n";
        makefile << "export ARCH := " << arch << "\n";
    }
    makefile << "\n";

    // Specify build rules
    makefile << "all:\n";
    makefile << "\tmake -C $(KBUILD) M=" + buildPath + " modules\n";
    makefile << "\n";
    makefile << "clean:\n";
    makefile << "\t make -C $(KBUILD) M=" + buildPath + " clean\n";

    CloseFile(makefile);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for a pre-built kernel module.
 **/
//--------------------------------------------------------------------------------------------------
void Generate
(
    model::Module_t* modulePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Minimize(buildParams.workingDir + "/build.ninja");

    std::ofstream script;
    OpenFile(script, filePath, buildParams.beVerbose);

    // Start the script with a comment, the file-level variable definitions, and
    // a set of generic rules.
    GenerateCommentHeader(script, modulePtr);
    script << "builddir = " << path::MakeAbsolute(buildParams.workingDir) << "\n\n";
    script << "target = " << buildParams.target << "\n\n";
    GenerateIfgenFlagsDef(script, buildParams.interfaceDirs);
    GenerateBuildRules(script, buildParams);

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
