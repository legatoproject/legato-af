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
#include "buildScriptCommon.h"
#include "exeBuildScript.h"
#include "componentBuildScript.h"

namespace ninja
{



//--------------------------------------------------------------------------------------------------
/**
 * Generate comment header for a build script.
 */
//--------------------------------------------------------------------------------------------------
void ExeBuildScriptGenerator_t::GenerateCommentHeader
(
    model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    script  << "# Build script for executable '" << exePtr->name << "'\n"
               "\n"
               "# == Auto-generated file.  Do not edit. ==\n"
               "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for all the IPC client and server
 * header files, source code files, and object files needed by a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void ExeBuildScriptGenerator_t::GenerateIpcBuildStatements
(
    model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    // It's possible that multiple components in the same executable will share the same interface.
    // To prevent the generation of multiple build statements (which would cause ninja to fail),
    // we use a set containing the output file paths to keep track of what build statements we've
    // already generated.

    std::set<std::string> generatedSet;

    for (auto instancePtr : exePtr->componentInstances)
    {
        componentGeneratorPtr->GenerateIpcBuildStatements(instancePtr->componentPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the contents that are common to both cFlags and cxxFlags variable
 * definitions for a given executable's .o file build statements.
 **/
//--------------------------------------------------------------------------------------------------
void ExeBuildScriptGenerator_t::GenerateCandCxxFlags
(
    model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    // Define the component name
    std::string componentName = exePtr->name + "_exe";
    script << " -DLE_COMPONENT_NAME=" << componentName;

    // Define the COMPONENT_INIT.
    script << " \"-DCOMPONENT_INIT=LE_CI_LINKAGE LE_SHARED"
              " void _" << componentName << "_COMPONENT_INIT()\""
           << " \"-DCOMPONENT_INIT_ONCE=LE_CI_LINKAGE LE_SHARED void "
           << componentName << "_COMPONENT_INIT_ONCE()\"";

}



//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the build statements related to a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void ExeBuildScriptGenerator_t::GenerateBuildStatements
(
    model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    if (exePtr->hasCOrCppCode)
    {
        // Add build statements for all the .o files to be built from C/C++ sources.
        for (auto objFilePtr : exePtr->cObjectFiles)
        {
            script << "build $builddir/" << objFilePtr->path << ":"
                      " CompileC " << objFilePtr->sourceFilePath << "\n"
                      "  cFlags = $cFlags ";
            GenerateCandCxxFlags(exePtr);
            script << "\n\n";
        }
        for (auto objFilePtr : exePtr->cxxObjectFiles)
        {
            script << "build $builddir/" << objFilePtr->path << ":"
                      " CompileCxx " << objFilePtr->sourceFilePath << "\n"
                      "  cxxFlags = $cxxFlags ";
            GenerateCandCxxFlags(exePtr);
            script << "\n\n";
        }

        // Add a build statement for the executable's _main.c.o file.
        auto mainObjectFile = exePtr->MainObjectFile();

        std::string defaultComponentName = exePtr->name + "_exe";
        script << "build $builddir/" << mainObjectFile.path << ":"
                  " CompileC " << mainObjectFile.sourceFilePath << "\n"
                  // Define the component name, log session variable, and log filter variable.
                  "  cFlags = $cFlags ";
        GenerateCandCxxFlags(exePtr);
        script << "\n\n";

        // Add a build statement for the executable file.
        GenerateBuildStatement(exePtr);
    }
    else if (exePtr->hasJavaCode)
    {
        auto legatoJarPath = path::Combine(envVars::Get("LEGATO_ROOT"),
                                           "build/$target/framework/lib/legato.jar");

        auto mainObjectFile = exePtr->MainObjectFile();
        std::string classDestPath;

        std::list<std::string> classPath = { legatoJarPath };
        std::list<std::string> dependencies = { legatoJarPath };

        if (exePtr->appPtr != NULL)
        {
            classDestPath = "$builddir/app/" + exePtr->appPtr->name;
        }
        else
        {
            classDestPath = "$builddir/";
        }

        classDestPath += "/obj/" + exePtr->name;

        for (auto componentInstPtr : exePtr->componentInstances)
        {
            auto componentPtr = componentInstPtr->componentPtr;

            if (componentPtr->HasJavaCode())
            {
                classPath.push_back(componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()
                                                ->lib);
                componentPtr->GetBundledFilesOfType(model::BundleAccess_t::Source,
                                                    ".jar",
                                                    classPath);
            }
        }

        componentGeneratorPtr->GenerateJavaBuildCommand(path::Combine("$builddir/", exePtr->path),
                                                        classDestPath,
                                                        { mainObjectFile.sourceFilePath },
                                                        classPath);
    }
    else if (exePtr->hasPythonCode)
    {
        auto mainObjectFile = exePtr->MainObjectFile();
        // Compute the path to the file to be generated.
        auto launcherFile = mainObjectFile.sourceFilePath;

        script << "build $builddir/" << exePtr->path << " : BundleFile " << launcherFile << "\n"
               << "  modeFlags = u+rwx,g+rwx,o+xr-w";
        script << "\n\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for the build script itself.
 **/
//--------------------------------------------------------------------------------------------------
void ExeBuildScriptGenerator_t::GenerateNinjaScriptBuildStatement
(
    model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    // The build.ninja depends on the .cdef files of all component instances, and all
    // the .api files they use.
    // Create a set of dependencies.
    std::set<std::string> dependencies;
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        auto componentPtr = componentInstancePtr->componentPtr;

        dependencies.insert(componentPtr->defFilePtr->path);

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
 * Generate all build rules required to build an executable.
 */
//--------------------------------------------------------------------------------------------------
void ExeBuildScriptGenerator_t::GenerateBuildRules
(
    void
)
{
    componentGeneratorPtr->GenerateBuildRules();
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for an executable and associated component and IPC interface
 * libraries.
 *
 * @note This is only used by mkexe.
 **/
//--------------------------------------------------------------------------------------------------
void ExeBuildScriptGenerator_t::Generate
(
    model::Exe_t* exePtr
)
{
    // Start the script with a comment, the file-level variable definitions, and
    // a set of generic rules.
    GenerateCommentHeader(exePtr);
    std::string includes;
    includes = " -I " + buildParams.workingDir;
    for (const auto& dir : buildParams.interfaceDirs)
    {
        includes += " -I" + dir;
    }
    script << "builddir = " << path::MakeAbsolute(buildParams.workingDir) << "\n\n";
    script << "cFlags = " << buildParams.cFlags << includes << "\n\n";
    script << "cxxFlags = " << buildParams.cxxFlags << includes << "\n\n";
    script << "ldFlags = " << buildParams.ldFlags << "\n\n";
    script << "target = " << buildParams.target << "\n\n";
    GenerateBuildRules();

    if (!buildParams.codeGenOnly)
    {
        // Add build statements for the executable and .o files included in it.
        GenerateBuildStatements(exePtr);

        // Add build statements for all the components included in this executable.
        for (auto componentInstancePtr : exePtr->componentInstances)
        {
            componentGeneratorPtr->GenerateBuildStatementsRecursive(
                componentInstancePtr->componentPtr);
        }
    }

    // Add build statements for all the IPC interfaces' generated files.
    GenerateIpcBuildStatements(exePtr);

    // Add a build statement for the build.ninja file itself.
    GenerateNinjaScriptBuildStatement(exePtr);
}

} // namespace ninja
