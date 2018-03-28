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
void ComponentBuildScriptGenerator_t::GenerateCommentHeader
(
    model::Component_t* componentPtr
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
void ComponentBuildScriptGenerator_t::GenerateCommonCAndCxxFlags
(
    model::Component_t* componentPtr
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
    script << " \"-DCOMPONENT_INIT=LE_CI_LINKAGE LE_SHARED void "
           << componentPtr->getTargetInfo<target::LinuxComponentInfo_t>()->initFuncName << "()\"";
}


//--------------------------------------------------------------------------------------------------
/**
 * Stream out (to a given ninja script) the compiler command line arguments required
 * to Set the DT_RUNPATH variable inside the executable's ELF headers to include the expected
 * on-target runtime locations of the libraries needed.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateRunPathLdFlags
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // DT_RUNPATH is set using linker parameters --enable-new-dtags and -rpath.
    // $ORIGIN is a way of referring to the location of the executable (or shared library) file
    // when it is loaded by the dynamic linker/loader at runtime.
    script << " -Wl,--enable-new-dtags,-rpath=\"\\$$ORIGIN/../lib";

    // When building for execution on the build host, add the localhost bin/lib directory.
    if (buildParams.target == "localhost")
    {
        script << ":$$LEGATO_BUILD/framework/lib";
    }

    script << "\"";
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script the list of implicit dependencies for a given component's library.
 * If any of these files change, the component library must be re-linked.
 *
 * @note This is recursive if the component depends on any other components.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GetImplicitDependencies
(
    model::Component_t* componentPtr
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
        if (subComponentPtr->getTargetInfo<target::LinuxComponentInfo_t>()->lib != "")
        {
            script << " " << subComponentPtr->getTargetInfo<target::LinuxComponentInfo_t>()->lib;
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
        GetImplicitDependencies(subComponentPtr);
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
void ComponentBuildScriptGenerator_t::GetExternalDependencies
(
    model::Component_t* componentPtr
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
void ComponentBuildScriptGenerator_t::GetDependentLibLdFlags
(
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    // List of already handled components so we don't add flags for the same component twice
    std::set<model::Component_t*> addedComponents;
    std::string ldFlags;

    GetDependentLibLdFlags(componentPtr, addedComponents, ldFlags);

    script << ldFlags;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the ldFlags variable contents needed to tell the linker to link
 * with libraries that a given Component depends on.
 *
 * @note This is recursive if the component depends on any other components.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GetDependentLibLdFlags
(
    model::Component_t* componentPtr,
    std::set<model::Component_t*>& addedComponents,
    std::string& ldFlags
)
//--------------------------------------------------------------------------------------------------
{
    for (auto subComponentPtr : componentPtr->subComponents)
    {
        // If code is already generated for this component, skip it.
        if (addedComponents.find(subComponentPtr) != addedComponents.end())
        {
            continue;
        }

        // Link with whatever this component depends on.
        GetDependentLibLdFlags(subComponentPtr, addedComponents, ldFlags);

        // If the component has itself been built into a library, link with that.
        if (subComponentPtr->getTargetInfo<target::LinuxComponentInfo_t>()->lib != "")
        {
            ldFlags = std::string(" \"-L")
                + path::GetContainingDir(
                    subComponentPtr->getTargetInfo<target::LinuxComponentInfo_t>()->lib
                )
                + "\""
                + " -l"
                + path::GetLibShortName(
                    subComponentPtr->getTargetInfo<target::LinuxComponentInfo_t>()->lib
                )
                + ldFlags;
        }

        // If the component has an external build, add the external build's working directory.
        if (subComponentPtr->HasExternalBuild())
        {
            ldFlags = " \"-L" + path::Combine(buildParams.workingDir,
                                              subComponentPtr->workingDir) + "\"" + ldFlags;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the ldFlags variable definition for a given Component.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateLdFlagsDef
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
 * Populate a string with a space-separated list of absolute paths to all .h files that need
 * to be generated by ifgen before the component's C/C++ source files can be built.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GetCInterfaceHeaders
(
    std::list<std::string>& result,   ///< String to populate.
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{

    for (auto ifPtr : componentPtr->typesOnlyApis)
    {
        model::InterfaceCFiles_t cFiles;
        ifPtr->GetInterfaceFiles(cFiles);

        result.push_back("$builddir/" + cFiles.interfaceFile);
    }

    for (auto ifPtr : componentPtr->serverApis)
    {
        model::InterfaceCFiles_t cFiles;
        ifPtr->GetInterfaceFiles(cFiles);

        result.push_back("$builddir/" + cFiles.interfaceFile);
        result.push_back("$builddir/" + cFiles.internalHFile);
    }

    for (auto ifPtr : componentPtr->clientApis)
    {
        model::InterfaceCFiles_t cFiles;
        ifPtr->GetInterfaceFiles(cFiles);

        result.push_back("$builddir/" + cFiles.interfaceFile);
        result.push_back("$builddir/" + cFiles.internalHFile);
    }

    for (auto apiFilePtr : componentPtr->clientUsetypesApis)
    {
        result.push_back("$builddir/" +
                         apiFilePtr->GetClientInterfaceFile(apiFilePtr->defaultPrefix));
    }

    for (auto apiFilePtr : componentPtr->serverUsetypesApis)
    {
        result.push_back("$builddir/" +
                         apiFilePtr->GetServerInterfaceFile(apiFilePtr->defaultPrefix));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Populate a string with a space-separated list of absolute paths to all .java files that need
 * to be generated by ifgen before the component's Java source files can be built.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GetJavaInterfaceFiles
(
    std::list<std::string>& result,   ///< String to populate.
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{

    for (auto ifPtr : componentPtr->typesOnlyApis)
    {
        model::InterfaceJavaFiles_t javaFiles;
        ifPtr->GetInterfaceFiles(javaFiles);

        result.push_back("$builddir/" + javaFiles.interfaceSourceFile);
    }

    for (auto ifPtr : componentPtr->serverApis)
    {
        model::InterfaceJavaFiles_t javaFiles;
        ifPtr->GetInterfaceFiles(javaFiles);

        result.push_back("$builddir/" + javaFiles.interfaceSourceFile);
        result.push_back("$builddir/" + javaFiles.implementationSourceFile);
    }

    for (auto ifPtr : componentPtr->clientApis)
    {
        model::InterfaceJavaFiles_t javaFiles;
        ifPtr->GetInterfaceFiles(javaFiles);

        result.push_back("$builddir/" + javaFiles.interfaceSourceFile);
        result.push_back("$builddir/" + javaFiles.implementationSourceFile);
    }

    for (auto apiFilePtr : componentPtr->clientUsetypesApis)
    {
        result.push_back("$builddir/" +
                         apiFilePtr->GetJavaInterfaceFile(apiFilePtr->defaultPrefix));
    }

    for (auto apiFilePtr : componentPtr->serverUsetypesApis)
    {
        result.push_back("$builddir/" +
                         apiFilePtr->GetJavaInterfaceFile(apiFilePtr->defaultPrefix));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script a build statement for building a given component's library.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateComponentLinkStatement
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
    script << "build " << componentPtr->getTargetInfo<target::LinuxComponentInfo_t>()->lib
           << ": " << rule;

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
    GetImplicitDependencies(componentPtr);
    script << "\n";

    // Define the ldFlags variable.
    GenerateLdFlagsDef(componentPtr);

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
void ComponentBuildScriptGenerator_t::GenerateCSourceBuildStatement
(
    model::Component_t* componentPtr,
    const model::ObjectFile_t* objFilePtr,  ///< The object file to build.
    const std::list<std::string>& apiHeaders ///< IPC API .h files needed by component.
)
//--------------------------------------------------------------------------------------------------
{
    // Create the build statement.
    script << "build $builddir/" << objFilePtr->path << ":"
              " CompileC " << objFilePtr->sourceFilePath;

    if (HasExternalDependencies(componentPtr))
    {
        script << " | ";
        GetExternalDependencies(componentPtr);
    }

    // Add order-only dependencies for all the generated .h files that will be needed by the
    // component.  This ensures that the .c files won't be compiled until all the .h files are
    // available.
    if (!apiHeaders.empty())
    {
        script << " || ";
        std::copy(apiHeaders.begin(), apiHeaders.end(),
                  std::ostream_iterator<std::string>(script, " "));
    }

    script << "\n";

    // Define the cFlags variable.
    script << "  cFlags = $cFlags";
    GenerateCommonCAndCxxFlags(componentPtr);
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
void ComponentBuildScriptGenerator_t::GenerateCxxSourceBuildStatement
(
    model::Component_t* componentPtr,
    const model::ObjectFile_t* objFilePtr,  ///< The object file to build.
    const std::list<std::string>& apiHeaders ///< IPC API .h files needed by component.
)
//--------------------------------------------------------------------------------------------------
{
    // Create the build statement.
    script << "build $builddir/" << objFilePtr->path << ":"
              " CompileCxx " << objFilePtr->sourceFilePath;

    if (HasExternalDependencies(componentPtr))
    {
        script << " | ";
        GetExternalDependencies(componentPtr);
    }

    // Add order-only dependencies for all the generated .h files that will be needed by the
    // component.  This ensures that the .c files won't be compiled until all the .h files are
    // available.
    if (!apiHeaders.empty())
    {
        script << " || ";
        std::copy(apiHeaders.begin(), apiHeaders.end(),
                  std::ostream_iterator<std::string>(script, " "));
    }
    script << "\n";

    // Define the cxxFlags variable.
    script << "  cxxFlags = $cxxFlags";
    GenerateCommonCAndCxxFlags(componentPtr);
    for (auto& arg : componentPtr->cxxFlags)
    {
        script << " " << arg;
    }
    script << "\n\n";
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate the build commands necessary to compile java code and create a Jar file to contain the
 * generated .class files.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateJavaBuildCommand
(
    const std::string& outputJar,
    const std::string& classDestPath,
    const std::list<std::string>& sources,
    const std::list<std::string>& jarClassPath
)
//--------------------------------------------------------------------------------------------------
{
    // Generate the rule to compile the Java code into .class files, and to package them up into a
    // .jar file.
    script << "build " << path::Combine(classDestPath, "build.stamp") << " $\n"
              "  : CompileJava";

    for (auto& source : sources)
    {
        script << " " << source;
    }

    script << " $\n  |";

    for (auto& dep : jarClassPath)
    {
        script << " " << dep;
    }

    script << "\n"
              "  classPath = ";

    for (auto iter = jarClassPath.begin(); iter != jarClassPath.end(); ++iter)
    {
        if (iter != jarClassPath.begin())
        {
            script << ":";
        }

        script << *iter;
    }

    script << "\n\n";

    script << "build " << outputJar << " $\n"
              "  : MakeJar " << path::Combine(classDestPath, "build.stamp") << "\n"
              "  classPath = ";

    for (auto iter = jarClassPath.begin(); iter != jarClassPath.end(); ++iter)
    {
        if (iter != jarClassPath.begin())
        {
            script << ":";
        }

        script << *iter;
    }

    script << "\n\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build statements for a component library that is shareable between multiple
 * executables.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateBuildStatements
(
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Create a set of header files that need to be generated for all IPC API interfaces.
    std::list<std::string> interfaceHeaders;

    if (componentPtr->HasCOrCppCode())
    {
        // Add the build statement for the component library.
        GenerateComponentLinkStatement(componentPtr);

        GetCInterfaceHeaders(interfaceHeaders, componentPtr);
    }
    else if (componentPtr->HasJavaCode())
    {
        GetJavaInterfaceFiles(interfaceHeaders, componentPtr);
    }

    // Add build statements for all the component's object files.
    for (auto objFilePtr : componentPtr->cObjectFiles)
    {
        GenerateCSourceBuildStatement(componentPtr, objFilePtr, interfaceHeaders);
    }
    for (auto objFilePtr : componentPtr->cxxObjectFiles)
    {
        GenerateCxxSourceBuildStatement(componentPtr, objFilePtr, interfaceHeaders);
    }

    if (componentPtr->HasCOrCppCode())
    {
        // Add a build statement for the generated component-specific code.
        script << "build $builddir/" << componentPtr->workingDir + "/obj/_componentMain.c.o" << ":"
                  " CompileC $builddir/" << componentPtr->workingDir + "/src/_componentMain.c"
               << "\n";

        script << "  cFlags = $cFlags";
        GenerateCommonCAndCxxFlags(componentPtr);
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

        sourceList.insert(sourceList.end(), interfaceHeaders.begin(), interfaceHeaders.end());

        auto legatoJarPath = path::Combine(envVars::Get("LEGATO_ROOT"),
                                           "build/$target/framework/lib/legato.jar");
        auto classDestPath = "$builddir/" + componentPtr->workingDir + "/obj";

        // Append to the class path based on the component's bundled .jar files.
        std::list<std::string> classPath = { legatoJarPath };
        componentPtr->GetBundledFilesOfType(model::BundleAccess_t::Source, ".jar", classPath);

        GenerateJavaBuildCommand(componentPtr->getTargetInfo<target::LinuxComponentInfo_t>()->lib,
                                 classDestPath,
                                 sourceList,
                                 classPath);
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
                   << " : BuildExternal | ";
            if (0 != lineno)
            {
                script << componentPtr->name << "ExternalBuild_line" << (lineno - 1);
            }
            else
            {
                // First line of an external build depends on the required components.
                GetImplicitDependencies(componentPtr);
            }

            script << std::endl;
            script << "  workingdir = " << componentPtr->workingDir << std::endl
                   << "  externalCommand = " << EscapeString(*commandPtr) << std::endl;
        }

        // Overall build depends on last line
        script << "build " << componentPtr->name << "ExternalBuild";

        // Assume every bundled file could be a build output of the external build step,
        // if this has bundled files
        for (auto fileSystemObjPtr : componentPtr->bundledFiles)
        {
            script << " " << fileSystemObjPtr->srcPath;
        }

        script << " : phony "
               << componentPtr->name << "ExternalBuild_line"
               << (lineno - 1);
        script << "\n\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write out to a given script file a space-separated list of paths to all the .api files needed
 * by a given .api file (specified through USETYPES statements in the .api files).
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GetIncludedApis
(
    const model::ApiFile_t* apiFilePtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto includedApiPtr : apiFilePtr->includes)
    {
        script << " " << includedApiPtr->path;

        // Recurse.
        GetIncludedApis(includedApiPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given script a build statement for building the header file for a given types-only
 * included API interface.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateTypesOnlyBuildStatement
(
    const model::ApiTypesOnlyInterface_t* ifPtr
)
//--------------------------------------------------------------------------------------------------
{
    model::InterfaceCFiles_t cFiles;
    ifPtr->GetInterfaceFiles(cFiles);

    if (generatedIPC.find(cFiles.interfaceFile) == generatedIPC.end())
    {
        generatedIPC.insert(cFiles.interfaceFile);

        script << "build $builddir/" << cFiles.interfaceFile << ":"
                  " GenInterfaceCode " << ifPtr->apiFilePtr->path << " |";
        GetIncludedApis(ifPtr->apiFilePtr);
        script << "\n"
                  "  ifgenFlags = --gen-interface"
                  " --name-prefix " << ifPtr->internalName <<
                  " $ifgenFlags\n"
                  "  outputDir = $builddir/" << path::GetContainingDir(cFiles.interfaceFile) << "\n"
                  "\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given script a build statement for building the java file for a given types-only
 * included API interface.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateJavaTypesOnlyBuildStatement
(
    const model::ApiTypesOnlyInterface_t* ifPtr
)
//--------------------------------------------------------------------------------------------------
{
    model::InterfaceJavaFiles_t javaFiles;
    ifPtr->GetInterfaceFiles(javaFiles);

    if (generatedIPC.find(javaFiles.interfaceSourceFile) == generatedIPC.end())
    {
        generatedIPC.insert(javaFiles.interfaceSourceFile);

        script << "build " <<
                  path::Combine(buildParams.workingDir, javaFiles.interfaceSourceFile) << ":"
                  " GenInterfaceCode " << ifPtr->apiFilePtr->path << " |";
        GetIncludedApis(ifPtr->apiFilePtr);
        script << "\n"
                  "  ifgenFlags = --gen-interface --lang Java"
                  " --name-prefix " << ifPtr->internalName << " $ifgenFlags\n"
                  "  outputDir = $builddir/" <<
                  path::Combine(ifPtr->componentPtr->workingDir, "src") << "\n"
                  "\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given script a build statement for building the interface header file for a given
 * .api file that has been referred to by a USETYPES statement in another .api file used by
 * a client-side interface.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateClientUsetypesBuildStatement
(
    const model::ApiFile_t* apiFilePtr
)
//--------------------------------------------------------------------------------------------------
{
    std::string headerFile = apiFilePtr->GetClientInterfaceFile(apiFilePtr->defaultPrefix);

    if (generatedIPC.find(headerFile) == generatedIPC.end())
    {
        generatedIPC.insert(headerFile);

        script << "build $builddir/" << headerFile <<
                  ": GenInterfaceCode " << apiFilePtr->path << " |";
        GetIncludedApis(apiFilePtr);
        script << "\n"
                  "  outputDir = $builddir/" << path::GetContainingDir(headerFile) << "\n"
                  "  ifgenFlags = --gen-interface $ifgenFlags\n"
                  "\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given script a build statement for building the interface header file for a given
 * .api file that has been referred to by a USETYPES statement in another .api file used by
 * a server-side interface.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateServerUsetypesBuildStatement
(
    const model::ApiFile_t* apiFilePtr
)
//--------------------------------------------------------------------------------------------------
{
    std::string headerFile = apiFilePtr->GetServerInterfaceFile(apiFilePtr->defaultPrefix);

    if (generatedIPC.find(headerFile) == generatedIPC.end())
    {
        generatedIPC.insert(headerFile);

        script << "build $builddir/" << headerFile <<
                  ": GenInterfaceCode " << apiFilePtr->path << " |";
        GetIncludedApis(apiFilePtr);
        script << "\n"
                  "  outputDir = $builddir/" << path::GetContainingDir(headerFile) << "\n"
                  "  ifgenFlags = --gen-server-interface $ifgenFlags\n"
                  "\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given script a build statement for building the java interface file for a given
 * .api file that has been referred to by a USETYPES statement in another .api file used by
 * a client/server-side interface.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateJavaUsetypesBuildStatement
(
    const model::ApiFile_t* apiFilePtr
)
//--------------------------------------------------------------------------------------------------
{
    std::string interfaceFile = apiFilePtr->GetJavaInterfaceFile(apiFilePtr->defaultPrefix);

    if (generatedIPC.find(interfaceFile) == generatedIPC.end())
    {
        generatedIPC.insert(interfaceFile);
        script << "build " << path::Combine(buildParams.workingDir, interfaceFile) <<
                  ": GenInterfaceCode " << apiFilePtr->path << " |";
        GetIncludedApis(apiFilePtr);
        script << "\n"
                  "  outputDir = $builddir/" <<
                  path::Combine(apiFilePtr->codeGenDir, "src") << "\n"
                  "  ifgenFlags = --gen-interface --lang Java $ifgenFlags\n"
                  "\n";
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Print to a given script a build statement for building the object file for a given client-side
 * API interface.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateCBuildStatement
(
    const model::ApiClientInterface_t* ifPtr
)
//--------------------------------------------------------------------------------------------------
{
    model::InterfaceCFiles_t cFiles;
    ifPtr->GetInterfaceFiles(cFiles);

    if (   (!buildParams.codeGenOnly)
        && (generatedIPC.find(cFiles.objectFile) == generatedIPC.end())  )
    {
        generatedIPC.insert(cFiles.objectFile);

        // .o file
        script << "build $builddir/" << cFiles.objectFile << ":"
                  " CompileC $builddir/" << cFiles.sourceFile;

        // Add dependencies on the generated .h files for this interface so we make sure
        // those get built first.
        script << " | $builddir/" << cFiles.internalHFile << " $builddir/" << cFiles.interfaceFile;

        // Build a set containing all the .h files that will be included by the .h file generated
        // for this .api file.
        std::set<std::string> apiHeaders;
        ifPtr->apiFilePtr->GetClientUsetypesApiHeaders(apiHeaders);

        // If there are some, add them as order-only dependencies.
        for (auto const &hFilePath : apiHeaders)
        {
            script << " $builddir/" << hFilePath;
        }

        // Define a cFlags variable that tells the compiler where to look for the interface
        // headers needed due to USETYPES statements.
        script << "\n"
                  "  cFlags = $cFlags";
        std::set<std::string> includeDirs;
        for (auto const &hFilePath : apiHeaders)
        {
            auto dirPath = path::GetContainingDir(hFilePath);
            if (includeDirs.find(dirPath) == includeDirs.end())
            {
                includeDirs.insert(dirPath);
                script << " -I$builddir/" << dirPath;
            }
        }
        script << "\n\n";
    }

    // .c file and .h files
    std::string generatedFiles;
    std::string ifgenFlags;
    if (generatedIPC.find(cFiles.sourceFile) == generatedIPC.end())
    {
        generatedIPC.insert(cFiles.sourceFile);
        generatedFiles += " $builddir/" + cFiles.sourceFile;
        ifgenFlags += " --gen-client";
    }
    if (generatedIPC.find(cFiles.interfaceFile) == generatedIPC.end())
    {
        generatedIPC.insert(cFiles.interfaceFile);
        generatedFiles += " $builddir/" + cFiles.interfaceFile;
        ifgenFlags += " --gen-interface";
    }
    if (generatedIPC.find(cFiles.internalHFile) == generatedIPC.end())
    {
        generatedIPC.insert(cFiles.internalHFile);
        generatedFiles += " $builddir/" + cFiles.internalHFile;
        ifgenFlags += " --gen-local";
    }
    if (!generatedFiles.empty())
    {
        ifgenFlags += " --name-prefix " + ifPtr->internalName;
        script << "build" << generatedFiles <<
                  ": GenInterfaceCode " << ifPtr->apiFilePtr->path << " |";
        GetIncludedApis(ifPtr->apiFilePtr);
        script << "\n"
                  "  ifgenFlags =" << ifgenFlags << " $ifgenFlags\n"
                  "  outputDir = $builddir/" << path::GetContainingDir(cFiles.sourceFile) << "\n\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the Java ifgen build statement for the client/server side of an API.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateJavaBuildStatement
(
    const model::InterfaceJavaFiles_t& javaFiles,
    const model::Component_t* componentPtr,
    const model::ApiFile_t* apiFilePtr,
    const std::string& internalName,
    bool isClient
)
//--------------------------------------------------------------------------------------------------
{
    std::string apiFlag = isClient ? "--gen-client" : "--gen-server";
    std::string generatedFiles;
    std::string requiredFlags;

    std::string interfaceSourcePath =
        path::Combine(buildParams.workingDir, javaFiles.interfaceSourceFile);
    if (generatedIPC.find(interfaceSourcePath) == generatedIPC.end())
    {
        generatedIPC.insert(interfaceSourcePath);
        generatedFiles += interfaceSourcePath + " ";
        requiredFlags += " --gen-interface";
    }

    std::string implementationSourcePath =
        path::Combine(buildParams.workingDir, javaFiles.implementationSourceFile);

    if (generatedIPC.find(implementationSourcePath) == generatedIPC.end())
    {
        generatedIPC.insert(implementationSourcePath);
        generatedFiles += implementationSourcePath + " ";
        requiredFlags += " " + apiFlag;
    }

    script << "build " << generatedFiles << ": $\n"
              "      GenInterfaceCode " << apiFilePtr->path << " | ";

    GetIncludedApis(apiFilePtr);

    script << "\n"
              "  ifgenFlags = --lang Java" << requiredFlags << " --name-prefix "
           << internalName << " $ifgenFlags\n"
              "  outputDir = " << path::Combine(buildParams.workingDir,
                                                path::Combine(componentPtr->workingDir, "src"))
           << "\n\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the Java ifgen build statement for the client side of an API.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateJavaBuildStatement
(
    const model::ApiClientInterface_t* ifPtr
)
//--------------------------------------------------------------------------------------------------
{
    model::InterfaceJavaFiles_t javaFiles;
    ifPtr->GetInterfaceFiles(javaFiles);

    GenerateJavaBuildStatement(javaFiles,
                               ifPtr->componentPtr,
                               ifPtr->apiFilePtr,
                               ifPtr->internalName,
                               true /* is a client */);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the Python ifgen build statement for the client/server side of an API.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GeneratePythonBuildStatement
(
    const model::InterfacePythonFiles_t& pythonFiles,
    const model::Component_t* componentPtr,
    const model::ApiFile_t* apiFilePtr,
    const std::string& internalName,
    const std::string& workDir,
    bool isClient
)
//--------------------------------------------------------------------------------------------------
{
    std::string apiFlag = "--gen-all";
    std::string outputDir = path::Combine("$builddir", apiFilePtr->codeGenDir);
    script << "build " << path::Combine(outputDir, pythonFiles.cdefSourceFile) << " $\n"
              "      " << path::Combine(outputDir, pythonFiles.wrapperSourceFile) << " : $\n"
              "      GenInterfaceCode " << apiFilePtr->path << " | ";

    GetIncludedApis(apiFilePtr);
    script << "\n"
              "  ifgenFlags = --lang Python " << apiFlag << " --name-prefix "
           << internalName << " $ifgenFlags\n"
              "  outputDir = " << outputDir << "\n\n";

    // Generate only the cffi cdef.h file of the included APIs
    apiFlag = "--gen-cdef";
    std::string apiList = "";
    for (auto includedApiPtr : apiFilePtr->includes)
    {
        // Extract the basename of the included APIs
        std::string pyCdefSourceFile = path::GetLastNode(includedApiPtr->path);
        std::string baseName = path::RemoveSuffix(pyCdefSourceFile, ".api");
        // Create the cffi cdef.h filename
        std::string pyCdefSourceFilePath = path::Combine(outputDir, pyCdefSourceFile + "_cdef.h");
        apiList += " " + pyCdefSourceFilePath;

        script << "build " << pyCdefSourceFilePath << " : $\n"
                  "      GenInterfaceCode " << includedApiPtr->path << " | ";

        GetIncludedApis(includedApiPtr);
        // cffi cdef.h files generated in folder includedApi
        script << "\n"
                  "  ifgenFlags = --lang Python " << apiFlag << " --name-prefix "
               << baseName << " $ifgenFlags\n"
                  "  outputDir = " << outputDir + "/includedApi" << "\n\n";
    }
    // generate the ffi C code. Add implicit dependencies on the included APIs
    script << "build " << path::Combine(outputDir, pythonFiles.cExtensionSourceFile) <<  ": $\n"
              "      GenPyApiCExtension " << path::Combine(outputDir, pythonFiles.cdefSourceFile)
           << " | " << apiList << "\n"
              "      workDir = " << outputDir << "\n";
    script << "\n\n";


    std::string interfaceIncludes;

    for (auto apiFilePtr : componentPtr->clientUsetypesApis)
    {
        interfaceIncludes += " -I$builddir/" +
            path::GetContainingDir(apiFilePtr->GetClientInterfaceFile(apiFilePtr->defaultPrefix));
    }

    script << "build " << path::Combine(outputDir, pythonFiles.cExtensionObjectFile) << " : $\n"
              "      CompileC "
           << path::Combine(outputDir, pythonFiles.cExtensionSourceFile) << "\n"
              "      cFlags = -I=/usr/include/python2.7/ -DNO_LOG_SESSION"
           << interfaceIncludes << " -D_FTS_H -DPY_BUILD $cFlags";

    script << "\n\n";

    std::string legatoRoot = envVars::Get("LEGATO_BUILD");

    script << "build " << path::Combine(outputDir, pythonFiles.cExtensionBinaryFile) << " : $\n"
              "      LinkCLib " << path::Combine(outputDir, pythonFiles.cExtensionObjectFile)
           << "\n"
              "      ldFlags = -L" << legatoRoot
           << "/framework/lib -llegato -lpthread -lrt -lm -lpython2.7 $ldFlags\n";
    script << "\n\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the Python ifgen build statement for the client side of an API.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GeneratePythonBuildStatement
(
    const model::ApiClientInterface_t* ifPtr
)
//--------------------------------------------------------------------------------------------------
{
    model::InterfacePythonFiles_t pythonFiles;
    ifPtr->GetInterfaceFiles(pythonFiles);

    GeneratePythonBuildStatement(
                               pythonFiles,
                               ifPtr->componentPtr,
                               ifPtr->apiFilePtr,
                               ifPtr->internalName,
                               buildParams.workingDir,
                               true // isClient
                               );
}

//
//--------------------------------------------------------------------------------------------------
/**
 * Print to a given script a build statement for building the object file for a given server-side
 * API interface.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateCBuildStatement
(
    const model::ApiServerInterface_t* ifPtr
)
//--------------------------------------------------------------------------------------------------
{
    model::InterfaceCFiles_t cFiles;
    ifPtr->GetInterfaceFiles(cFiles);

    if (   (!buildParams.codeGenOnly)
        && (generatedIPC.find(cFiles.objectFile) == generatedIPC.end())  )
    {
        generatedIPC.insert(cFiles.objectFile);

        // .o file
        script << "build $builddir/" << cFiles.objectFile << ":"
                  " CompileC $builddir/" << cFiles.sourceFile;

        // Add order-only dependencies on the generated .h files for this interface so we make sure
        // those get built first.
        script << " | $builddir/" << cFiles.internalHFile << " $builddir/" << cFiles.interfaceFile;

        // Build a set containing all the .h files that will be included (via USETYPES statements)
        // by the .h file generated for this .api file.
        std::set<std::string> apiHeaders;
        ifPtr->apiFilePtr->GetServerUsetypesApiHeaders(apiHeaders);

        // If there are some, add them as order-only dependencies.
        for (auto const &hFilePath : apiHeaders)
        {
            script << " $builddir/" << hFilePath;
        }

        // Define a cFlags variable that tells the compiler where to look for the interface
        // headers needed due to USETYPES statements.
        script << "\n"
                  "  cFlags = $cFlags";
        std::set<std::string> includeDirs;
        for (auto const &hFilePath : apiHeaders)
        {
            auto dirPath = path::GetContainingDir(hFilePath);
            if (includeDirs.find(dirPath) == includeDirs.end())
            {
                includeDirs.insert(dirPath);
                script << " -I$builddir/" << dirPath;
            }
        }
        script << "\n\n";
    }

    // .c file and .h files
    std::string generatedFiles;
    std::string ifgenFlags;
    if (generatedIPC.find(cFiles.sourceFile) == generatedIPC.end())
    {
        generatedIPC.insert(cFiles.sourceFile);
        generatedFiles += " $builddir/" + cFiles.sourceFile;
        ifgenFlags += " --gen-server";
    }
    if (generatedIPC.find(cFiles.interfaceFile) == generatedIPC.end())
    {
        generatedIPC.insert(cFiles.interfaceFile);
        generatedFiles += " $builddir/" + cFiles.interfaceFile;
        ifgenFlags += " --gen-server-interface";
    }
    if (generatedIPC.find(cFiles.internalHFile) == generatedIPC.end())
    {
        generatedIPC.insert(cFiles.internalHFile);
        generatedFiles += " $builddir/" + cFiles.internalHFile;
        ifgenFlags += " --gen-local";
    }
    if (!generatedFiles.empty())
    {
        if (ifPtr->async)
        {
            ifgenFlags += " --async-server";
        }
        ifgenFlags += " --name-prefix " + ifPtr->internalName;
        script << "build" << generatedFiles << ":"
                  " GenInterfaceCode " << ifPtr->apiFilePtr->path << " |";
        GetIncludedApis(ifPtr->apiFilePtr);
        script << "\n"
                  "  ifgenFlags =" << ifgenFlags << " $ifgenFlags\n"
                  "  outputDir = $builddir/" << path::GetContainingDir(cFiles.sourceFile) << "\n"
                  "\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the Java ifgen build statement for the server side of an API.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateJavaBuildStatement
(
    const model::ApiServerInterface_t* ifPtr
)
//--------------------------------------------------------------------------------------------------
{
    model::InterfaceJavaFiles_t javaFiles;
    ifPtr->GetInterfaceFiles(javaFiles);

    GenerateJavaBuildStatement(javaFiles,
                               ifPtr->componentPtr,
                               ifPtr->apiFilePtr,
                               ifPtr->internalName,
                               false /* not a client */);
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for all the IPC client and server
 * header files, source code files, and object files needed by a given component and all its
 * subcomponents.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateIpcBuildStatements
(
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    bool isJava = componentPtr->HasJavaCode();
    bool isPython = componentPtr->HasPythonCode();

    for (auto typesOnlyApi : componentPtr->typesOnlyApis)
    {
        if (isJava)
        {
            GenerateJavaTypesOnlyBuildStatement(typesOnlyApi);
        }
        else
        {
            GenerateTypesOnlyBuildStatement(typesOnlyApi);
        }
    }

    for (auto apiFilePtr : componentPtr->clientUsetypesApis)
    {
        if (isJava)
        {
            GenerateJavaUsetypesBuildStatement(apiFilePtr);
        }
        else
        {
            GenerateClientUsetypesBuildStatement(apiFilePtr);
        }
    }

    for (auto apiFilePtr : componentPtr->serverUsetypesApis)
    {
        if (isJava)
        {
            GenerateJavaUsetypesBuildStatement(apiFilePtr);
        }
        else
        {
            GenerateServerUsetypesBuildStatement(apiFilePtr);
        }
    }

    for (auto clientApi : componentPtr->clientApis)
    {
        if (isJava)
        {
            GenerateJavaBuildStatement(clientApi);
        }
        else if (isPython)
        {
            GeneratePythonBuildStatement(clientApi);

        }
        else
        {
            GenerateCBuildStatement(clientApi);
        }
    }

    for (auto serverApi : componentPtr->serverApis)
    {
        if (isJava)
        {
            GenerateJavaBuildStatement(serverApi);
        }
        else if (isPython)
        {
            throw mk::Exception_t(LE_I18N("Python server IPC not implemented."));
        }
        else
        {
            GenerateCBuildStatement(serverApi);
        }
    }

    // Recurse to all sub-components
    for (auto subComponentPtr : componentPtr->subComponents)
    {
        GenerateIpcBuildStatements(subComponentPtr);
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for all the sub-components of a given
 * component and all their sub-components.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateBuildStatementsRecursive
(
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    // It's possible that multiple components will share the same sub-component.
    // To prevent the generation of multiple build statements (which would cause ninja to fail),
    // we use a set containing the component names to keep track of what build statements we've
    // already generated.
    if (generatedComponents.find(componentPtr->name) == generatedComponents.end())
    {
        generatedComponents.insert(componentPtr->name);

        GenerateBuildStatements(componentPtr);

        for (auto subComponentPtr : componentPtr->subComponents)
        {
            GenerateBuildStatementsRecursive(subComponentPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Assemble a list of all files this ninja build script depends on.
 */
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::AddNinjaDependencies
(
    model::Component_t* componentPtr,
    std::set<std::string>& dependencies
)
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
        AddNinjaDependencies(subComponentPtr, dependencies);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for the build script itself.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateNinjaScriptBuildStatement
(
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    // The build.ninja depends on the .cdef file, all sub-component .cdef files, and all
    // .api files used.
    // Create a set to be filled with all the dependencies.
    std::set<std::string> dependencies;

    AddNinjaDependencies(componentPtr, dependencies);

    baseGeneratorPtr->GenerateNinjaScriptBuildStatement(dependencies);
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate build rules needed to build components.
 */
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::GenerateBuildRules
(
    void
)
{
    baseGeneratorPtr->GenerateIfgenFlagsDef();
    baseGeneratorPtr->GenerateBuildRules();
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for building a single component.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuildScriptGenerator_t::Generate
(
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Start the script with a comment, the file-level variable definitions, and
    // a set of generic rules.
    GenerateCommentHeader(componentPtr);
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
    GenerateBuildRules();

    if (!buildParams.codeGenOnly)
    {
        // Add a build statement for the component library and its source files.
        GenerateBuildStatementsRecursive(componentPtr);
    }

    // Add build statements for all the IPC interfaces' generated files.
    GenerateIpcBuildStatements(componentPtr);

    // Add a build statement for the build.ninja file itself.
    GenerateNinjaScriptBuildStatement(componentPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for building a single component.
 **/
//--------------------------------------------------------------------------------------------------
void Generate
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Combine(buildParams.workingDir, "build.ninja");

    ComponentBuildScriptGenerator_t componentGenerator(filePath, buildParams);

    componentGenerator.Generate(componentPtr);
}


} // namespace ninja
