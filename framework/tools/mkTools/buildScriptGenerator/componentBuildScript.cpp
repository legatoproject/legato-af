//--------------------------------------------------------------------------------------------------
/**
 * @file componentBuildScript.cpp
 *
 * Component build script generation functions.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptCommon.h"
#include "componentBuildScript.h"

namespace ninja
{



//--------------------------------------------------------------------------------------------------
/**
 * Generate comment header for component build script.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateCommentHeader
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    script  << "# Build script for component '" << componentPtr->name << "'\n"
            << "\n"
            << "# == Auto-generated file.  Do not edit. ==\n"
            << "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the contents that are common to both cFlags and cxxFlags variable
 * definitions for a given Component.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateCommonCAndCxxFlags
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Include the component's generated sources directory (where interfaces.h is put).
    script << " -I$builddir/" << componentPtr->workingDir << "/src";

    // For each server-side interface, include the appropriate (async or sync) server
    // code generation directory.
    for (auto ifPtr : componentPtr->serverApis)
    {
        script << " -I$builddir/" << path::GetContainingDir(ifPtr->interfaceFile);
    }

    // For each client-side interface, include the client code generation directory.
    for (auto ifPtr : componentPtr->clientApis)
    {
        script << " -I$builddir/" << path::GetContainingDir(ifPtr->interfaceFile);
    }

    // For each "types-only" required API, include the client code generation directory.
    for (auto ifPtr : componentPtr->typesOnlyApis)
    {
        script << " -I$builddir/" << path::GetContainingDir(ifPtr->interfaceFile);
    }

    // For each server-side USETYPES statement, include the server code generation directory.
    // NOTE: It's very important that this comes after the serverApis, because the server
    //       may serve the async version of an API that another API uses types from, and
    //       we need to get the correct version.  Include guards will prevent redefinitions.
    for (auto apiFilePtr : componentPtr->serverUsetypesApis)
    {
        script << " -I$builddir/" << apiFilePtr->codeGenDir << "/server";
    }

    // for each client-side USETYPES statement, include the client code generation directory.
    for (auto apiFilePtr : componentPtr->clientUsetypesApis)
    {
        script << " -I$builddir/" << apiFilePtr->codeGenDir << "/client";
    }

    // Define the component name, log session variable, and log filter variable.
    script << " -DLEGATO_COMPONENT=" << componentPtr->name;
    script << " -DLE_LOG_SESSION=" << componentPtr->name << "_LogSession ";
    script << " -DLE_LOG_LEVEL_FILTER_PTR=" << componentPtr->name << "_LogLevelFilterPtr ";

    // Define the COMPONENT_INIT.
    script << " \"-DCOMPONENT_INIT=LE_CI_LINKAGE LE_SHARED void " << componentPtr->initFuncName << "()\"";
}


//--------------------------------------------------------------------------------------------------
/**
 * Add to a given string the list of sub-component library (.so) files that a given component's
 * library depends on.  If any of these files changes, the component's library must be re-linked.
 *
 * @note This is recursive if the component depends on any other components.
 **/
//--------------------------------------------------------------------------------------------------
static void GetSubComponentLibs
(
    std::stringstream& result,  ///< Stream to write the variable definition to.
    const model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    // For each sub-component,
    for (auto subComponentPtr : componentPtr->subComponents)
    {
        // If the sub-component has itself been built into a library, the component depends
        // on that sub-component library.
        if (subComponentPtr->lib != "")
        {
            result << " " << subComponentPtr->lib;
        }

        // Component also depends on whatever the sub-component depends on.
        // NOTE: Might be able to optimize this out for sub-components that build to a library,
        //       because the sub-component library will depend on those other things, so depending
        //       on the sub-component library is sufficient to imply an indirect dependency on
        //       those other things.
        GetSubComponentLibs(result, subComponentPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the ldFlags variable contents needed to tell the linker to link
 * with libraries that a given Component depends on.
 *
 * @note This is recursive if the component depends on any other components.
 **/
//--------------------------------------------------------------------------------------------------
void GetDependentLibLdFlags
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto subComponentPtr : componentPtr->subComponents)
    {
        // If the component has itself been built into a library, link with that.
        if (subComponentPtr->lib != "")
        {
            script << " \"-L" << path::GetContainingDir(subComponentPtr->lib) << "\"";

            script << " -l" << path::GetLibShortName(subComponentPtr->lib);
        }

        // Link with whatever this component depends on.
        GetDependentLibLdFlags(script, subComponentPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the ldFlags variable definition for a given Component.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateLdFlagsDef
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    script << "  ldFlags = ";

    // Add the ldflags from the Component.cdef file.
    for (auto& arg : componentPtr->ldFlags)
    {
        script << " " << arg;
    }

    // Add the library output directory to the list of places to search for libraries to link with.
    script << " -L" << buildParams.libOutputDir;

    // Set the DT_RUNPATH variable inside the executable's ELF headers to include the expected
    // on-target runtime locations of the libraries needed.
    GenerateRunPathLdFlags(script, buildParams.target);

    // Includes a list of -l directives for all the libraries the component needs.
    GetDependentLibLdFlags(script, componentPtr);

    // Link with the standard runtime libs.
    script << " \"-L$$LEGATO_BUILD/bin/lib\" -llegato -lpthread -lrt -lm\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Populate a string with a space-separated list of absolute paths to all .h files that need
 * to be generated by ifgen before the component's C/C++ source files can be built.
 **/
//--------------------------------------------------------------------------------------------------
static void GetInterfaceHeaders
(
    std::string& result,   ///< String to populate.
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    for (auto ifPtr : componentPtr->typesOnlyApis)
    {
        result += " $builddir/" + ifPtr->interfaceFile;
    }

    for (auto ifPtr : componentPtr->serverApis)
    {
        result += " $builddir/" + ifPtr->interfaceFile;
        result += " $builddir/" + ifPtr->internalHFile;
    }

    for (auto ifPtr : componentPtr->clientApis)
    {
        result += " $builddir/" + ifPtr->interfaceFile;
        result += " $builddir/" + ifPtr->internalHFile;
    }

    for (auto apiFilePtr : componentPtr->clientUsetypesApis)
    {
        result += " $builddir/" + apiFilePtr->GetClientInterfaceFile(apiFilePtr->defaultPrefix);
    }

    for (auto apiFilePtr : componentPtr->serverUsetypesApis)
    {
        result += " $builddir/" + apiFilePtr->GetServerInterfaceFile(apiFilePtr->defaultPrefix);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script a rule for building a given component's library.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateComponentLibBuildStatement
(
    std::ofstream& script,  ///< Build script to write to.
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string rule;

    // Determine which rules should be used for building the component.
    if (!componentPtr->cxxSources.empty())
    {
        rule = "LinkCxxLib";
    }
    else if (!componentPtr->cSources.empty())
    {
        rule = "LinkCLib";
    }
    else
    {
        // No source files.  No library to build.
        return;
    }

    // Create the build statement.
    script << "build " << componentPtr->lib << ": " << rule;

    // Includes object files compiled from the component's C/C++ source files.
    for (auto sourceFile : componentPtr->cSources)
    {
        script << " " << GetObjectFile(sourceFile);
    }
    for (auto sourceFile : componentPtr->cxxSources)
    {
        script << " " << GetObjectFile(sourceFile);
    }

    // Also includes all the object files for the auto-generated IPC API client and server
    // code for the component's required and provided APIs.
    for (auto apiPtr : componentPtr->clientApis)
    {
        script << " $builddir/" << apiPtr->objectFile;
    }
    for (auto apiPtr : componentPtr->serverApis)
    {
        script << " $builddir/" << apiPtr->objectFile;
    }

    // And the object file for the component-specific generated code in _componentMain.c.
    script << " $builddir/" << componentPtr->workingDir << "/obj/_componentMain.c.o";

    // Add implicit dependencies.
    std::stringstream implicitDependencies;
    GetSubComponentLibs(implicitDependencies, componentPtr);
    if (!implicitDependencies.str().empty())
    {
        script << " |" << implicitDependencies.str();
    }
    script << "\n";

    // Define the ldFlags variable.
    GenerateLdFlagsDef(script, componentPtr, buildParams);

    script << "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script a rule for building a given C source code file's object file.
 *
 * The source file path can be absolute, relative to the component's source directory, or
 * begin with "$builddir/" to make it relative to the root of the working directory tree.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateCSourceBuildStatement
(
    std::ofstream& script,  ///< Build script to write to.
    const model::Component_t* componentPtr,
    const std::string& sourceFile,  ///< Path to the source file.
    const std::string& apiHeaders,///< String containing IPC API .h files needed by component.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Create the build statement.
    script << "build " << GetObjectFile(sourceFile) << ":"
              " CompileC " << GetAbsoluteSourcePath(sourceFile, componentPtr);

    // Add order-only dependencies for all the generated .h files that will be needed by the
    // component.  This ensures that the .c files won't be compiled until all the .h files are
    // available.
    if (!apiHeaders.empty())
    {
        script << " || $builddir/" << apiHeaders;
    }
    script << "\n";

    // Define the cFlags variable.
    script << "  cFlags = $cFlags";
    GenerateCommonCAndCxxFlags(script, componentPtr, buildParams);
    for (auto& arg : componentPtr->cFlags)
    {
        script << " " << arg;
    }
    script << "\n\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script a rule for building a given C++ source code file's object file.
 *
 * The source file path can be absolute, relative to the component's source directory, or
 * begin with "$builddir/" to make it relative to the root of the working directory tree.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateCxxSourceBuildStatement
(
    std::ofstream& script,  ///< Build script to write to.
    const model::Component_t* componentPtr,
    const std::string& sourceFile,  ///< Path to the source file.
    const std::string& apiHeaders,///< String containing IPC API .h files needed by component.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Create the build statement.
    script << "build " << GetObjectFile(sourceFile) << ":"
              " CompileCxx " << GetAbsoluteSourcePath(sourceFile, componentPtr);

    // Add order-only dependencies for all the generated .h files that will be needed by the
    // component.  This ensures that the .c files won't be compiled until all the .h files are
    // available.
    if (!apiHeaders.empty())
    {
        script << " || $builddir/" << apiHeaders;
    }
    script << "\n";

    // Define the cxxFlags variable.
    script << "  cxxFlags = $cxxFlags";
    GenerateCommonCAndCxxFlags(script, componentPtr, buildParams);
    for (auto& arg : componentPtr->cxxFlags)
    {
        script << " " << arg;
    }
    script << "\n\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build statements for a component library that is shareable between multiple
 * executables.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateBuildStatements
(
    std::ofstream& script,  ///< Script to write the build statements to.
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Add the build statement for the component library.
    GenerateComponentLibBuildStatement(script, componentPtr, buildParams);

    // Create a set of header files that need to be generated for all IPC API interfaces.
    std::string interfaceHeaders;
    GetInterfaceHeaders(interfaceHeaders, componentPtr, buildParams);

    // Add build statements for all the component's object files.
    for (auto cSourceFile : componentPtr->cSources)
    {
        GenerateCSourceBuildStatement(script,
                                      componentPtr,
                                      cSourceFile,
                                      interfaceHeaders,
                                      buildParams);
    }
    for (auto cxxSourceFile : componentPtr->cxxSources)
    {
        GenerateCxxSourceBuildStatement(script,
                                        componentPtr,
                                        cxxSourceFile,
                                        interfaceHeaders,
                                        buildParams);
    }

    // Add a build statement for the generated component-specific code.
    script << "build $builddir/" << componentPtr->workingDir + "/obj/_componentMain.c.o" << ":"
              " CompileC $builddir/" << componentPtr->workingDir + "/src/_componentMain.c" << "\n";
    script << "  cFlags = $cFlags";
    GenerateCommonCAndCxxFlags(script, componentPtr, buildParams);
    script << "\n\n";
}



//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for all the IPC client and server
 * header files, source code files, and object files needed by a given component and all its
 * subcomponents.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateIpcBuildStatements
(
    std::ofstream& script,
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // It's possible that multiple components will share the same interface.
    // To prevent the generation of multiple build statements (which would cause ninja to fail),
    // we use a set containing the output file paths to keep track of what build statements we've
    // already generated.

    std::set<std::string> generatedSet;

    // Use a lambda to recursively descend through the tree of sub-components.

    std::function<void(const model::Component_t* componentPtr)> generate;
    generate = [&script, &generatedSet, &buildParams, &generate]
        (
            const model::Component_t* componentPtr
        )
        {
            GenerateIpcBuildStatements(script, componentPtr, buildParams, generatedSet);

            for (auto subComponentPtr : componentPtr->subComponents)
            {
                generate(subComponentPtr);
            }
        };

    generate(componentPtr);
}



//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for all the sub-components of a given
 * component and all their sub-components.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateSubComponentBuildStatements
(
    std::ofstream& script,
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // It's possible that multiple components will share the same sub-component.
    // To prevent the generation of multiple build statements (which would cause ninja to fail),
    // we use a set containing the component names to keep track of what build statements we've
    // already generated.

    std::set<std::string> generatedSet;

    // Use a lambda to recursively descend through the tree of sub-components.

    std::function<void(const model::Component_t* componentPtr)> generate =
        [&script, &generatedSet, &buildParams, &generate]
        (
            const model::Component_t* componentPtr
        )
        {
            if (generatedSet.find(componentPtr->name) == generatedSet.end())
            {
                generatedSet.insert(componentPtr->name);

                GenerateBuildStatements(script, componentPtr, buildParams);

                for (auto subComponentPtr : componentPtr->subComponents)
                {
                    generate(subComponentPtr);
                }
            }
        };

    for (auto subComponentPtr : componentPtr->subComponents)
    {
        generate(subComponentPtr);
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
    const model::Component_t* componentPtr,
    const std::string& filePath     ///< Path to the build.ninja file.
)
//--------------------------------------------------------------------------------------------------
{
    script << "build " << filePath << ": RegenNinjaScript |";

    // The build.ninja depends on the .cdef file, all sub-component .cdef files, and all
    // .api files used.
    // Create a set to be filled with all the dependencies.
    std::set<std::string> dependencies;

    // Define a recursive lambda function that populates the set.
    std::function<void(const model::Component_t*)> lambda;
    lambda = [&lambda, &dependencies](const model::Component_t* componentPtr)
    {
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

        // Recurse into sub-components.
        for (auto subComponentPtr : componentPtr->subComponents)
        {
            lambda(subComponentPtr);
        }
    };

    // Call the lambda function.
    lambda(componentPtr);

    // Write the dependencies to the script.
    for (auto dep : dependencies)
    {
        script << " " << dep;
    }
    script << "\n\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for building a single component.
 **/
//--------------------------------------------------------------------------------------------------
void Generate
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams,
    int argc,           ///< Count of the number of command line parameters.
    const char** argv   ///< Pointer to an array of pointers to command line argument strings.
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Combine(buildParams.workingDir, "build.ninja");

    std::ofstream script;
    OpenFile(script, filePath, buildParams.beVerbose);

    // Start the script with a comment, the file-level variable definitions, and
    // a set of generic rules.
    GenerateCommentHeader(script, componentPtr);
    std::string includes;
    for (const auto& dir : buildParams.interfaceDirs)
    {
        includes += " -I" + dir;
    }
    script << "builddir =" << buildParams.workingDir << "\n\n";
    script << "cFlags =" << buildParams.cFlags << includes << "\n\n";
    script << "cxxFlags =" << buildParams.cxxFlags << includes << "\n\n";
    script << "ldFlags =" << buildParams.ldFlags << "\n\n";
    GenerateIfgenFlagsDef(script, buildParams.interfaceDirs);
    GenerateBuildRules(script, buildParams.target, argc, argv);

    if (!buildParams.codeGenOnly)
    {
        // Add a build statement for the component library and its source files.
        GenerateBuildStatements(script, componentPtr, buildParams);

        // Add build statements for all the component's sub-components.
        GenerateSubComponentBuildStatements(script, componentPtr, buildParams);
    }

    // Add build statements for all the IPC interfaces' generated files.
    GenerateIpcBuildStatements(script, componentPtr, buildParams);

    // Add a build statement for the build.ninja file itself.
    GenerateNinjaScriptBuildStatement(script, componentPtr, filePath);

    CloseFile(script);
}


} // namespace ninja
