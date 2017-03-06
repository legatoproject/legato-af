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
    const model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Include the component's generated sources directory (where interfaces.h is put).
    script << " -I$builddir/" << componentPtr->workingDir << "/src";

    // For each server-side interface, include the appropriate (async or sync) server
    // code generation directory.
    for (auto ifPtr : componentPtr->serverApis)
    {
        model::InterfaceCFiles_t cFiles;
        ifPtr->GetInterfaceFiles(cFiles);
        script << " -I$builddir/" << path::GetContainingDir(cFiles.interfaceFile);
    }

    // For each client-side interface, include the client code generation directory.
    for (auto ifPtr : componentPtr->clientApis)
    {
        model::InterfaceCFiles_t cFiles;
        ifPtr->GetInterfaceFiles(cFiles);
        script << " -I$builddir/" << path::GetContainingDir(cFiles.interfaceFile);
    }

    // For each "types-only" required API, include the client code generation directory.
    for (auto ifPtr : componentPtr->typesOnlyApis)
    {
        model::InterfaceCFiles_t cFiles;
        ifPtr->GetInterfaceFiles(cFiles);
        script << " -I$builddir/" << path::GetContainingDir(cFiles.interfaceFile);
    }

    // Subcomponents with external builds do not interface via interfaces, so add these components
    // directly
    for (auto subComponentPtr : componentPtr->subComponents)
    {
        if (subComponentPtr->HasExternalBuild())
        {
            script << " -I" << subComponentPtr->dir
                   << " -I$builddir/" << subComponentPtr->workingDir;
        }
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
    script << " -DLE_COMPONENT_NAME=" << componentPtr->name;
    script << " -DLE_LOG_SESSION=" << componentPtr->name << "_LogSession ";
    script << " -DLE_LOG_LEVEL_FILTER_PTR=" << componentPtr->name << "_LogLevelFilterPtr ";

    // Define the COMPONENT_INIT.
    script << " \"-DCOMPONENT_INIT=LE_CI_LINKAGE LE_SHARED void " << componentPtr->initFuncName << "()\"";
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script the list of implicit dependencies for a given component's library.
 * If any of these files change, the component library must be re-linked.
 *
 * @note This is recursive if the component depends on any other components.
 **/
//--------------------------------------------------------------------------------------------------
static void GetImplicitDependencies
(
    std::ofstream& script,  ///< Build script to write to.
    const model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    // For each
    for (const auto& dependency : componentPtr->implicitDependencies)
    {
        script << " " << dependency;
    }

    // For each sub-component,
    for (auto subComponentPtr : componentPtr->subComponents)
    {
        // If the sub-component has itself been built into a library, the component depends
        // on that sub-component library.
        if (subComponentPtr->lib != "")
        {
            script << " " << subComponentPtr->lib;
        }

        // If the sub-component has an external build step, this component depends on that
        // build step being run
        if (subComponentPtr->HasExternalBuild())
        {
            script << " " << subComponentPtr->name + "ExternalBuild";
        }

        // Component also depends on whatever the sub-component depends on.
        // NOTE: Might be able to optimize this out for sub-components that build to a library,
        //       because the sub-component library will depend on those other things, so depending
        //       on the sub-component library is sufficient to imply an indirect dependency on
        //       those other things.
        GetImplicitDependencies(script, subComponentPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Does this component depend on components with external build steps.
 **/
//--------------------------------------------------------------------------------------------------
static bool HasExternalDependencies
(
    const model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    // For each sub-component,
    for (auto subComponentPtr : componentPtr->subComponents)
    {
        // If the sub-component has an external build step, this component depends on that
        // build step being run
        if (subComponentPtr->HasExternalBuild())
        {
            return true;
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script the list of dependencies with external compile steps.  These must be
 * added as compile (rather than link) dependencies since an external build step could generate
 * configuration .h files.
 **/
//--------------------------------------------------------------------------------------------------
static void GetExternalDependencies
(
    std::ofstream& script,  ///< Build script to write to.
    const model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    // For each sub-component,
    for (auto subComponentPtr : componentPtr->subComponents)
    {
        // If the sub-component has an external build step, this component depends on that
        // build step being run
        if (subComponentPtr->HasExternalBuild())
        {
            script << " " << subComponentPtr->name + "ExternalBuild";
        }
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

        // If the component has an external build, add the external build's working directory.
        if (subComponentPtr->HasExternalBuild())
        {
            script << " \"-L$builddir" << subComponentPtr->dir << "\"";
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
    GenerateRunPathLdFlags(script, buildParams.target);

    // Includes a list of -l directives for all the libraries the component needs.
    GetDependentLibLdFlags(script, componentPtr);

    // Link with the standard runtime libs.
    script << " \"-L$$LEGATO_BUILD/framework/lib\" -llegato -lpthread -lrt -lm\n";
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
    const model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{

    for (auto ifPtr : componentPtr->typesOnlyApis)
    {
        model::InterfaceCFiles_t cFiles;
        ifPtr->GetInterfaceFiles(cFiles);

        result += " $builddir/" + cFiles.interfaceFile;
    }

    for (auto ifPtr : componentPtr->serverApis)
    {
        model::InterfaceCFiles_t cFiles;
        ifPtr->GetInterfaceFiles(cFiles);

        result += " $builddir/" + cFiles.interfaceFile;
        result += " $builddir/" + cFiles.internalHFile;
    }

    for (auto ifPtr : componentPtr->clientApis)
    {
        model::InterfaceCFiles_t cFiles;
        ifPtr->GetInterfaceFiles(cFiles);

        result += " $builddir/" + cFiles.interfaceFile;
        result += " $builddir/" + cFiles.internalHFile;
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
 * Print to a given build script a build statement for building a given component's library.
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
    script << "build " << componentPtr->lib << ": " << rule;

    // Includes object files compiled from the component's C/C++ source files.
    for (auto objFilePtr : componentPtr->cObjectFiles)
    {
        script << " $builddir/" << objFilePtr->path;
    }
    for (auto objFilePtr : componentPtr->cxxObjectFiles)
    {
        script << " $builddir/" << objFilePtr->path;
    }

    // Also includes all the object files for the auto-generated IPC API client and server
    // code for the component's required and provided APIs.
    for (auto apiPtr : componentPtr->clientApis)
    {
        model::InterfaceCFiles_t cFiles;
        apiPtr->GetInterfaceFiles(cFiles);

        script << " $builddir/" << cFiles.objectFile;
    }
    for (auto apiPtr : componentPtr->serverApis)
    {
        model::InterfaceCFiles_t cFiles;
        apiPtr->GetInterfaceFiles(cFiles);

        script << " $builddir/" << cFiles.objectFile;
    }

    // And the object file for the component-specific generated code in _componentMain.c.
    script << " $builddir/" << componentPtr->workingDir << "/obj/_componentMain.c.o";

    // Add implicit dependencies.
    script << " |";
    GetImplicitDependencies(script, componentPtr);
    GetExternalDependencies(script, componentPtr);
    script << "\n";

    // Define the ldFlags variable.
    GenerateLdFlagsDef(script, componentPtr, buildParams);

    script << "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given script a build statement for building a given C source code file's object file.
 *
 * The source file path can be absolute, relative to the component's source directory, or
 * begin with "$builddir/" to make it relative to the root of the working directory tree.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateCSourceBuildStatement
(
    std::ofstream& script,  ///< Build script to write to.
    const model::Component_t* componentPtr,
    const model::ObjectFile_t* objFilePtr,  ///< The object file to build.
    const std::string& apiHeaders ///< String containing IPC API .h files needed by component.
)
//--------------------------------------------------------------------------------------------------
{
    // Create the build statement.
    script << "build $builddir/" << objFilePtr->path << ":"
              " CompileC " << objFilePtr->sourceFilePath;

    if (HasExternalDependencies(componentPtr))
    {
        script << " | ";
        GetExternalDependencies(script, componentPtr);
    }

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
    GenerateCommonCAndCxxFlags(script, componentPtr);
    for (auto& arg : componentPtr->cFlags)
    {
        script << " " << arg;
    }
    script << "\n\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script a statement for building a given C++ source code file's object file.
 *
 * The source file path can be absolute, relative to the component's source directory, or
 * begin with "$builddir/" to make it relative to the root of the working directory tree.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateCxxSourceBuildStatement
(
    std::ofstream& script,  ///< Build script to write to.
    const model::Component_t* componentPtr,
    const model::ObjectFile_t* objFilePtr,  ///< The object file to build.
    const std::string& apiHeaders ///< String containing IPC API .h files needed by component.
)
//--------------------------------------------------------------------------------------------------
{
    // Create the build statement.
    script << "build $builddir/" << objFilePtr->path << ":"
              " CompileCxx " << objFilePtr->sourceFilePath;

    if (HasExternalDependencies(componentPtr))
    {
        script << " | ";
        GetExternalDependencies(script, componentPtr);
    }

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
    GenerateCommonCAndCxxFlags(script, componentPtr);
    for (auto& arg : componentPtr->cxxFlags)
    {
        script << " " << arg;
    }
    script << "\n\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * .
 **/
//--------------------------------------------------------------------------------------------------
template<typename ApiType>
static void ColateSources
(
    const std::string& workDir,
    const std::list<ApiType*>& interfaces,
    std::list<std::string>& sourceList
)
//--------------------------------------------------------------------------------------------------
{
    for (auto apiPtr : interfaces)
    {
        model::InterfaceJavaFiles_t javaFiles;
        apiPtr->GetInterfaceFiles(javaFiles);

        if (javaFiles.interfaceSourceFile.empty() == false)
        {
            sourceList.push_back(path::Combine(workDir, javaFiles.interfaceSourceFile));
        }

        if (javaFiles.implementationSourceFile.empty() == false)
        {
            sourceList.push_back(path::Combine(workDir, javaFiles.implementationSourceFile));
        }
    }
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
    if (componentPtr->HasCOrCppCode())
    {
        // Add the build statement for the component library.
        GenerateComponentLibBuildStatement(script, componentPtr, buildParams);
    }

    // Create a set of header files that need to be generated for all IPC API interfaces.
    std::string interfaceHeaders;
    GetInterfaceHeaders(interfaceHeaders, componentPtr);

    // Add build statements for all the component's object files.
    for (auto objFilePtr : componentPtr->cObjectFiles)
    {
        GenerateCSourceBuildStatement(script, componentPtr, objFilePtr, interfaceHeaders);
    }
    for (auto objFilePtr : componentPtr->cxxObjectFiles)
    {
        GenerateCxxSourceBuildStatement(script, componentPtr, objFilePtr, interfaceHeaders);
    }

    if (componentPtr->HasCOrCppCode())
    {
        // Add a build statement for the generated component-specific code.
        script << "build $builddir/" << componentPtr->workingDir + "/obj/_componentMain.c.o" << ":"
                  " CompileC $builddir/" << componentPtr->workingDir + "/src/_componentMain.c"
               << "\n";

        script << "  cFlags = $cFlags";
        GenerateCommonCAndCxxFlags(script, componentPtr);
        script << "\n\n";
    }
    else if (componentPtr->HasJavaCode())
    {
        std::list<std::string> sourceList =
            {
                "$builddir/" + componentPtr->workingDir + "/src/io/legato/generated/component/" +
                componentPtr->name + "/Factory.java"
            };

        for (auto package : componentPtr->javaPackages)
        {
            for (auto sourceFile : package->sourceFiles)
            {
                sourceList.push_back(path::Combine(componentPtr->dir, sourceFile));
            }
        }

        ColateSources<model::ApiTypesOnlyInterface_t>(buildParams.workingDir,
                                                      componentPtr->typesOnlyApis,
                                                      sourceList);

        ColateSources<model::ApiServerInterface_t>(buildParams.workingDir,
                                                   componentPtr->serverApis,
                                                   sourceList);

        ColateSources<model::ApiClientInterface_t>(buildParams.workingDir,
                                                   componentPtr->clientApis,
                                                   sourceList);

        auto legatoJarPath = path::Combine(envVars::Get("LEGATO_ROOT"),
                                           "build/$target/framework/lib/legato.jar");
        auto classDestPath = "$builddir/" + componentPtr->workingDir + "/obj";

        GenerateJavaBuildCommand(script,
                                 componentPtr->lib,
                                 classDestPath,
                                 sourceList,
                                 { legatoJarPath },
                                 { legatoJarPath });
    }
    else if (componentPtr->HasExternalBuild())
    {
        // Create external build commands for each line
        std::list<std::string>::const_iterator commandPtr;
        int lineno;
        for (commandPtr = componentPtr->externalBuildCommands.begin(),
                 lineno = 0;
             commandPtr != componentPtr->externalBuildCommands.end();
             ++commandPtr, ++lineno)
        {
            script << "build " << componentPtr->name << "ExternalBuild_line"
                   << lineno
                   << " : BuildExternal";
            if (0 != lineno)
            {
                script << " | " << componentPtr->name << "ExternalBuild_line" << (lineno - 1);
            }
            script << std::endl;
            script << "  workingdir = " << componentPtr->workingDir << std::endl
                   << "  externalCommand = " << EscapeString(*commandPtr) << std::endl;
        }

        // Overall build depends on last line
        script << "build " << componentPtr->name << "ExternalBuild : phony "
               << componentPtr->name << "ExternalBuild_line"
               << (lineno - 1);
        script << "\n\n";
    }
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
    includes = " -I " + buildParams.workingDir;
    for (const auto& dir : buildParams.interfaceDirs)
    {
        includes += " -I" + dir;
    }
    script << "builddir =" << buildParams.workingDir << "\n\n";
    script << "cFlags =" << buildParams.cFlags << includes << "\n\n";
    script << "cxxFlags =" << buildParams.cxxFlags << includes << "\n\n";
    script << "ldFlags =" << buildParams.ldFlags << "\n\n";
    script << "target = " << buildParams.target << "\n\n";
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
