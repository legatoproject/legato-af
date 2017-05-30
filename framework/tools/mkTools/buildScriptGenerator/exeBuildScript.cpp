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
static void GenerateCommentHeader
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const model::Exe_t* exePtr
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
 * Get the rule to be used to link an executable.
 *
 * @return String containing the name of the rule.
 **/
//--------------------------------------------------------------------------------------------------
static std::string GetLinkRule
(
    const model::Exe_t* exePtr
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
 * Print to a given build script the ldFlags variable contents needed to tell the linker to link
 * with libraries that a given executable depends on.
 **/
//--------------------------------------------------------------------------------------------------
static void GetDependentLibLdFlags
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    // Traverse the component instance list in reverse order.
    auto& list = exePtr->componentInstances;
    for (auto i = list.rbegin(); i != list.rend(); i++)
    {
        auto componentPtr = (*i)->componentPtr;
        auto& lib = componentPtr->lib;

        // If the component has itself been built into a library, link with that.
        if (lib != "")
        {
            script << " \"-L" << path::GetContainingDir(lib) << "\"";

            script << " -l" << path::GetLibShortName(lib);
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
static void GenerateBuildStatement
(
    std::ofstream& script,
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
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
            script << " " << componentPtr->lib;

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
    GenerateRunPathLdFlags(script, buildParams.target);

    // Link with all the static libraries that the components need.
    for (const auto& lib : staticLibs)
    {
        script << " " << lib;
    }

    // Add the library output directory to the list of places to search for libraries to link with.
    script << " -L" << buildParams.libOutputDir;

    // Include a list of -l directives for all the libraries the executable needs.
    GetDependentLibLdFlags(script, exePtr);

    // Link again with all the static libraries that the components need, in case there are
    // dynamic libraries that need symbols from them, or in case there are interdependencies
    // between them.
    for (const auto& lib : staticLibs)
    {
        script << " " << lib;
    }

    // Include another list of -l directives for all the libraries the executable needs.
    GetDependentLibLdFlags(script, exePtr);

    // Link with the standard runtime libs.
    script << " \"-L$$LEGATO_BUILD/framework/lib\" -llegato -lpthread -lrt -ldl -lm";

    // Add ldFlags from earlier definition.
    script << " $ldFlags\n"
              "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for all the IPC client and server
 * header files, source code files, and object files needed by a given executable.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateIpcBuildStatements
(
    std::ofstream& script,
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
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
        GenerateIpcBuildStatements(script, instancePtr->componentPtr, buildParams, generatedSet);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the contents that are common to both cFlags and cxxFlags variable
 * definitions for a given executable's .o file build statements.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateCandCxxFlags
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Define the component name, log session variable, and log filter variable.
    std::string componentName = exePtr->name + "_exe";
    script << " -DLE_COMPONENT_NAME=" << componentName;
    script << " -DLE_LOG_SESSION=" << componentName << "_LogSession ";
    script << " -DLE_LOG_LEVEL_FILTER_PTR=" << componentName << "_LogLevelFilterPtr ";

    // Define the COMPONENT_INIT.
    script << " \"-DCOMPONENT_INIT=LE_CI_LINKAGE LE_SHARED"
              " void _" << componentName << "_COMPONENT_INIT()\"";
}



//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the build statements related to a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateBuildStatements
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
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
            GenerateCandCxxFlags(script, exePtr, buildParams);
            script << "\n\n";
        }
        for (auto objFilePtr : exePtr->cxxObjectFiles)
        {
            script << "build $builddir/" << objFilePtr->path << ":"
                      " CompileCxx " << objFilePtr->sourceFilePath << "\n"
                      "  cxxFlags = $cxxFlags ";
            GenerateCandCxxFlags(script, exePtr, buildParams);
            script << "\n\n";
        }

        // Add a build statement for the executable's _main.c.o file.
        auto mainObjectFile = exePtr->MainObjectFile();

        std::string defaultComponentName = exePtr->name + "_exe";
        script << "build $builddir/" << mainObjectFile.path << ":"
                  " CompileC " << mainObjectFile.sourceFilePath << "\n"
                  // Define the component name, log session variable, and log filter variable.
                  "  cFlags = $cFlags"
                  " -DLE_COMPONENT_NAME=" << defaultComponentName <<
                  " -DLE_LOG_SESSION=" << defaultComponentName << "_LogSession"
                  " -DLE_LOG_LEVEL_FILTER_PTR=" << defaultComponentName << "_LogLevelFilterPtr "
                  "\n\n";

        // Add a build statement for the executable file.
        GenerateBuildStatement(script, exePtr, buildParams);
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
                classPath.push_back(componentPtr->lib);
                dependencies.push_back(componentPtr->lib);
            }
        }

        GenerateJavaBuildCommand(script,
                                 path::Combine("$builddir/", exePtr->path),
                                 classDestPath,
                                 { mainObjectFile.sourceFilePath },
                                 classPath,
                                 dependencies);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for the build script itself.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateNinjaScriptBuildStatement
(
    std::ofstream& script,
    const model::Exe_t* exePtr,
    const std::string& filePath     ///< Path to the build.ninja file.
)
//--------------------------------------------------------------------------------------------------
{
    // Generate a build statement for the build.ninja.
    script << "build " << filePath << ": RegenNinjaScript |";

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
 * Generate a build script for an executable and associated component and IPC interface
 * libraries.
 *
 * @note This is only used by mkexe.
 **/
//--------------------------------------------------------------------------------------------------
void Generate
(
    const model::Exe_t* exePtr,
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
    GenerateCommentHeader(script, exePtr);
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
    GenerateIfgenFlagsDef(script, buildParams.interfaceDirs);
    GenerateBuildRules(script, buildParams.target, argc, argv);

    if (!buildParams.codeGenOnly)
    {
        // Add build statements for the executable and .o files included in it.
        GenerateBuildStatements(script, exePtr, buildParams);

        // Add build statements for all the components included in this executable.
        for (auto componentInstancePtr : exePtr->componentInstances)
        {
            GenerateBuildStatements(script, componentInstancePtr->componentPtr, buildParams);
        }
    }

    // Add build statements for all the IPC interfaces' generated files.
    GenerateIpcBuildStatements(script, exePtr, buildParams);

    // Add a build statement for the build.ninja file itself.
    GenerateNinjaScriptBuildStatement(script, exePtr, filePath);

    CloseFile(script);
}


} // namespace ninja
