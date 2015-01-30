//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the routines for building Components.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include "ComponentBuilder.h"
#include "InterfaceBuilder.h"
#include "Utilities.h"


//--------------------------------------------------------------------------------------------------
/**
 * Generates the custom interfaces.h header file for a given component.
 */
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::GenerateInterfacesHeader
(
    legato::Component& component,
    const std::string& outputDir    ///< Overrides the ObjOutputDir in the build params.
)
//--------------------------------------------------------------------------------------------------
{
    // Don't do anything if the component doesn't import or export any interfaces.
    if (component.RequiredApis().empty() && component.ProvidedApis().empty())
    {
        return;
    }

    if (m_Params.IsVerbose())
    {
        std::cout << "Generating interfaces.h for component '" << component.Name()
                  << "' in directory '" << outputDir << "'." << std::endl;
    }

    // Make sure the working file output directory exists.
    legato::MakeDir(outputDir);

    // Open the interfaces.h file for writing.
    std::string interfacesHeaderFilePath = legato::CombinePath(outputDir, "interfaces.h");
    std::ofstream interfaceHeaderFile(interfacesHeaderFilePath, std::ofstream::trunc);
    if (!interfaceHeaderFile.is_open())
    {
        throw legato::Exception("Failed to open file '" + interfacesHeaderFilePath + "'.");
    }

    std::string includeGuardName = "__" + component.Name() + "_COMPONENT_INTERFACE_H_INCLUDE_GUARD";

    interfaceHeaderFile << "/*" << std::endl
                        << " * AUTO-GENERATED interface.h for the " << component.Name()
                                                                    << " component." << std::endl
                        << std::endl
                        << " * Don't bother hand-editing this file." << std::endl
                        << " */" << std::endl
                        << std::endl
                        << "#ifndef " << includeGuardName << std::endl
                        << "#define " << includeGuardName << std::endl
                        << std::endl
                        << "#ifdef __cplusplus" << std::endl
                        << "extern \"C\" {" << std::endl
                        << "#endif" << std::endl
                        << std::endl;

    // For each of the component's client-side interfaces, #include the client-side IPC headers.
    ///@ todo Support other programming languages besides C/C++.
    const auto& importMap = component.RequiredApis();
    for (auto i = importMap.cbegin(); i != importMap.cend(); i++)
    {
        const legato::ClientInterface& interface = std::get<1>(*i);

        // Add the interface code's header to the component's interface.h file.
        interfaceHeaderFile << "#include \"" << interface.InternalName() << "_interface.h" << "\""
                            << std::endl;
    }

    // For each service provided by the component, #include the server-side IPC header files.
    ///@ todo Support other programming languages besides C/C++.
    const auto& exportMap = component.ProvidedApis();
    for (auto i = exportMap.cbegin(); i != exportMap.cend(); i++)
    {
        const legato::ServerInterface& interface = std::get<1>(*i);

        interfaceHeaderFile << "#include \"" << interface.InternalName() << "_server.h" << "\""
                            << std::endl;
    }

    // Put the finishing touches on interfaces.h.
    interfaceHeaderFile << std::endl
                        << "#ifdef __cplusplus" << std::endl
                        << "}" << std::endl
                        << "#endif" << std::endl
                        << std::endl
                        << "#endif // " << includeGuardName << std::endl;


    // Add the directory to the include search path so the compiler can find the "interface.h" file.
    component.AddIncludeDir(outputDir);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate interface header files required in order to build the component.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::GenerateInterfaceHeaders
(
    legato::Component& component,
    const std::string& outputDir   ///< Overrides the ObjOutputDir in the build params.
)
//--------------------------------------------------------------------------------------------------
{
    legato::BuildParams_t interfaceBuildParams(m_Params);
    interfaceBuildParams.ObjOutputDir(outputDir);
    InterfaceBuilder_t interfaceBuilder(interfaceBuildParams);

    // Generate interface headers for client-side interfaces.
    for (auto& mapEntry : component.RequiredApis())
    {
        interfaceBuilder.GenerateApiHeaders(mapEntry.second, outputDir);
    }

    // Generate interface headers for server-side interfaces.
    for (auto& mapEntry : component.ProvidedApis())
    {
        interfaceBuilder.GenerateApiHeaders(mapEntry.second, outputDir);
    }

    // Generate custom "interfaces.h" for this component.
    GenerateInterfacesHeader(component, outputDir);
}


//--------------------------------------------------------------------------------------------------
/**
 * Build the .o files for a given component from its C/C++ sources.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::BuildObjectFiles
(
    legato::Component& component,
    const std::string& outputDir   ///< Overrides the ObjOutputDir in the build params.
)
//--------------------------------------------------------------------------------------------------
{
    // Make sure the working file output directory exists.
    legato::MakeDir(outputDir);

    // If the component doesn't have any C/C++ source files, then we don't need to do anything.
    if (component.CSources().empty() && component.CxxSources().empty())
    {
        if (m_Params.IsVerbose())
        {
            std::cout << "Component '" << component.Name() << "' has no C/C++ source files."
                      << std::endl;
        }
        return;
    }

    // Generate the component's own interfaces.h header file.
    /// @todo Support other languages.
    GenerateInterfacesHeader(component, outputDir);

    // Clear out the component's list of object files.  We'll reconstruct it as we compile sources.
    component.ObjectFiles().clear();

    // Compile each C source file,
    if (m_Params.IsVerbose())
    {
        if (component.HasCSources())
        {
            std::cout << "Compiling C sources for component '" << component.Name() << "'."
                      << std::endl;
        }
    }
    for (const auto& sourceFile : component.CSources())
    {
        CompileCSourceFile(component, sourceFile, outputDir);
    }

    // Compile each C++ source file,
    if (m_Params.IsVerbose())
    {
        if (component.HasCxxSources())
        {
            std::cout << "Compiling C++ sources for component '" << component.Name() << "'."
                      << std::endl;
        }
    }
    for (const auto& sourceFile : component.CxxSources())
    {
        CompileCxxSourceFile(component, sourceFile, outputDir);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Copies a component's (and all its sub-components') bundled files to the staging area.
 */
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::Bundle
(
    legato::Component& component
)
//--------------------------------------------------------------------------------------------------
{
    // Copy lower-level components' (sub-components') files first so they can be overridden by
    // higher-level components' bundled files.
    try
    {
        for (auto& mapEntry : component.SubComponents())
        {
            Bundle(*(mapEntry.second));
        }
    }
    catch (legato::DependencyException e)
    {
        throw legato::DependencyException(e.what() + std::string(" required by ")
                                          + component.Name());
    }

    // Print progress message.
    if (m_Params.IsVerbose())
    {
        std::cout << "Bundling files for component '" << component.Name() << "'." << std::endl;
    }

    // Copy all bundled files into the staging area.
    auto& bundledFiles = component.BundledFiles();
    for (auto fileMapping : bundledFiles)
    {
        mk::CopyToStaging(  fileMapping.m_SourcePath,
                            m_Params.StagingDir(),
                            fileMapping.m_DestPath,
                            m_Params.IsVerbose()    );
    }

    // Copy all bundled directories into the staging area.
    auto& bundledDirs = component.BundledDirs();
    for (auto fileMapping : bundledDirs)
    {
        mk::CopyToStaging(  fileMapping.m_SourcePath,
                            m_Params.StagingDir(),
                            fileMapping.m_DestPath,
                            m_Params.IsVerbose()    );
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Builds a component library.
 */
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::Build
(
    legato::Component& component,
    const std::string& outputDir   ///< Overrides the ObjOutputDir in the build params.
)
//--------------------------------------------------------------------------------------------------
{
    // If the component was already built, then we don't need to do anything.
    if (component.IsBuilt())
    {
        if (m_Params.IsVerbose())
        {
            std::cout << "Component '" << component.Name() << "' was already built."
                      << std::endl;
        }
    }
    else
    {
        // Print progress message.
        if (m_Params.IsVerbose())
        {
            std::cout << "Building component '" << component.Name() << "'."
                      << std::endl;
        }

        // Generate interface header files.
        GenerateInterfaceHeaders(component, outputDir);

        // Build the .o files from the component's sources.
        BuildObjectFiles(component, outputDir);

        // If the component doesn't have any object files, then we don't need to do anything else.
        if (component.ObjectFiles().empty())
        {
            if (m_Params.IsVerbose())
            {
                std::cout << "Component '" << component.Name() << "' has no object files to link."
                          << std::endl;
            }
        }
        else
        {
            // Create the library from the compiled object files.
            LinkLib(component);
        }

        component.MarkBuilt();
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Compile one of a component's C source code files into an object (.o) file.
 *
 * @note The object code file will have the same name as the .c file, with ".o" appended.
 *       (foo.c compiles to foo.c.o.)
 */
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::CompileCSourceFile
(
    legato::Component& component,
    const std::string& sourceFile,  ///< File to compile.
    const std::string& outputDir    ///< Directory where the object file will be put.
)
//--------------------------------------------------------------------------------------------------
{
    // TODO: Check file timestamps to see if compiling is actually needed.

    // Start the command line with the path to the appropriate compiler executable.
    std::stringstream commandLine;
    std::string compilerPath = mk::GetCompilerPath(m_Params.Target(), legato::LANG_C);
    commandLine << compilerPath;

    // Add the language-specific flags from the Component.cdef file to the command-line.
    for (auto& arg : component.CFlags())
    {
        commandLine << " " << arg;
    }

    // Add the user-specfied, language-specific flags to the command-line.
    commandLine << " " << m_Params.CCompilerFlags();

    // Enable all warnings.
    commandLine << " -Wall";

    // Compile to position-independent code so it can be linked into a shared library.
    commandLine << " -fPIC";

    // Specify the source code file to be compiled.
    commandLine << " -c \"" ;
    if ((component.Path() != "") && (!legato::IsAbsolutePath(sourceFile)))
    {
        commandLine << legato::CombinePath(component.Path(), sourceFile);
    }
    else
    {
        commandLine << sourceFile;
    }
    commandLine << "\"";

    // Specify the output (.o) file path.
    std::string objectFile = legato::CombinePath(outputDir, legato::GetLastPathNode(sourceFile))
                           + ".o";
    commandLine << " -o \"" << objectFile << "\"";

    // Add the object file to the Component's list.
    component.ObjectFiles().push_back(objectFile);

    // Treat all warnings as errors, unless using the clang compiler.
    if (!mk::IsCompilerClang(compilerPath))
    {
        commandLine << " -Werror";
    }

    // Add the include paths specified on the command-line.
    for (auto i : m_Params.InterfaceDirs())
    {
        commandLine << " \"-I" << i << "\"";
    }

    // Add the include paths specific to the component.
    for (auto i : component.IncludePath())
    {
        commandLine << " \"-I" << i << "\"";
    }

    // Define the component name, log session variable, and log filter variable.
    commandLine << " -DLEGATO_COMPONENT=" << component.CName();
    commandLine << " -DLE_LOG_SESSION=" << component.CName() << "_LogSession ";
    commandLine << " -DLE_LOG_LEVEL_FILTER_PTR=" << component.CName() << "_LogLevelFilterPtr ";

    // Define the COMPONENT_INIT.
    commandLine << " \"-DCOMPONENT_INIT=LE_CI_LINKAGE void " << component.InitFuncName() << "()\"";

    if (m_Params.IsVerbose())
    {
        std::cout << std::endl << "$ " << commandLine.str() << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(commandLine);
}


//--------------------------------------------------------------------------------------------------
/**
 * Compile one of a component's C++ source code files into an object (.o) file.
 *
 * @note The object code file will have the same name as the source code file, with ".o" appended.
 *       (foo.cpp compiles to foo.cpp.o.)
 */
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::CompileCxxSourceFile
(
    legato::Component& component,
    const std::string& sourceFile,  ///< File to compile.
    const std::string& outputDir    ///< Directory where the object file will be put.
)
//--------------------------------------------------------------------------------------------------
{
    // TODO: Check file timestamps to see if compiling is actually needed.

    // Start the command line with the path to the appropriate compiler executable.
    std::stringstream commandLine;
    std::string compilerPath = mk::GetCompilerPath(m_Params.Target(), legato::LANG_CXX);
    commandLine << compilerPath;

    // Add the language-specific flags from the Component.cdef file to the command-line.
    for (auto& arg : component.CxxFlags())
    {
        commandLine << " " << arg;
    }

    // Add the user-specfied, language-specific flags to the command-line.
    commandLine << " " << m_Params.CxxCompilerFlags();

    // Enable all warnings.
    commandLine << " -Wall";

    // Compile to position-independent code so it can be linked into a shared library.
    commandLine << " -fPIC";

    // Specify the source code file to be compiled.
    commandLine << " -c \"" ;
    if ((component.Path() != "") && (!legato::IsAbsolutePath(sourceFile)))
    {
        commandLine << legato::CombinePath(component.Path(), sourceFile);
    }
    else
    {
        commandLine << sourceFile;
    }
    commandLine << "\"";

    // Specify the output (.o) file path.
    std::string objectFile = legato::CombinePath(outputDir, legato::GetLastPathNode(sourceFile))
                           + ".o";
    commandLine << " -o \"" << objectFile << "\"";

    // Add the object file to the Component's list.
    component.ObjectFiles().push_back(objectFile);

    // Treat all warnings as errors, unless using the clang compiler.
    if (!mk::IsCompilerClang(compilerPath))
    {
        commandLine << " -Werror";
    }

    // Search include paths specified on the command-line.
    for (auto i : m_Params.InterfaceDirs())
    {
        commandLine << " -I" << i;
    }

    // Search the include paths specific to the component.
    for (auto i : component.IncludePath())
    {
        commandLine << " -I" << i;
    }

    // Define the component name, log session variable, and log filter variable.
    commandLine << " -DLEGATO_COMPONENT=" << component.CName();
    commandLine << " -DLE_LOG_SESSION=" << component.CName() << "_LogSession ";
    commandLine << " -DLE_LOG_LEVEL_FILTER_PTR=" << component.CName() << "_LogLevelFilterPtr ";

    // Define the COMPONENT_INIT.
    commandLine << " \"-DCOMPONENT_INIT=LE_CI_LINKAGE void " << component.InitFuncName() << "()\"";

    if (m_Params.IsVerbose())
    {
        std::cout << std::endl << "$ " << commandLine.str() << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(commandLine);
}



//--------------------------------------------------------------------------------------------------
/**
 * Links a component's .o files together into a component library according to the settings
 * in the Components object's Library object.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::LinkLib
(
    legato::Component& component
)
//--------------------------------------------------------------------------------------------------
{
    if (m_Params.IsVerbose())
    {
        std::cout << "Linking component '" << component.Name() << "'." << std::endl;
    }

    // If the component library doesn't have its library output directory set yet, set it now to
    // the library output directory in the build parameters.
    auto& library = component.Lib();
    if (library.BuildOutputDir().empty())
    {
        library.BuildOutputDir(m_Params.LibOutputDir());
    }

    if (library.IsStatic())
    {
        LinkStaticLib(component);
    }
    else
    {
        LinkSharedLib(component);
    }

    library.MarkExisting();
    library.MarkUpToDate();
}


//--------------------------------------------------------------------------------------------------
/**
 * Links a component's object files together to form a shared component instance library (.so) file.
 *
 * @note    The component library is not linked with any interface library, so the dependency on
 *          the interfaces is not recorded in the library's ELF header.  Therefore it is up to
 *          the component instance library to link the component library before the interface
 *          libraries.
 *
 * @todo    Link with generic interface client/server shared libraries when they become available.
 */
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::LinkSharedLib
(
    legato::Component& component
)
//--------------------------------------------------------------------------------------------------
{
    // TODO: Check file timestamps to see if linking is actually needed.

    // Determine which programming language toolchain to use.
    legato::ProgrammingLanguage_t language = legato::LANG_C;
    if (component.HasCxxSources())
    {
        language = legato::LANG_CXX;
    }

    // Start constructing the command-line.
    std::stringstream commandLine;
    commandLine << mk::GetCompilerPath(m_Params.Target(), language)
                << " -shared"
                << " -o \"" << component.Lib().BuildOutputPath() << "\"";

    // Include all of the object (.o) files built from the component's source files.
    for (const auto& objectFile : component.ObjectFiles())
    {
        commandLine << " \"" << objectFile << "\"";
    }

    // Add the linker flags from the Component.cdef file.
    for (auto& arg : component.LdFlags())
    {
        commandLine << " " << arg;
    }

    // On the localhost, set the DT_RUNPATH variable inside the library to include the
    // expected locations of the sub-libraries needed.
    if (m_Params.Target() == "localhost")
    {
        commandLine << " -Wl,--enable-new-dtags"
                    << ",-rpath=\"\\$ORIGIN:"
                    << m_Params.LibOutputDir()
                    << ":$LEGATO_ROOT/build/localhost/bin/lib\"";
    }
    // On embedded targets, set the DT_RUNPATH variable inside the library to include the
    // expected location of libraries bundled in this application (this is needed for
    // unsandboxed applications).
    else
    {
        commandLine << " -Wl,--enable-new-dtags,-rpath=\"\\$ORIGIN\"";
    }

    // Add the library output path to the list of directories to be searched for library files.
    commandLine << " \"-L" << m_Params.LibOutputDir() << "\"";

    // Include all of the library (.a and .so) files specified in the Component.cdefs of this
    // component and sub-components.
    mk::GetComponentLibLinkDirectives(commandLine, component);

    // Link with external library files.
    for (const auto& library : component.RequiredLibs())
    {
        commandLine << " -l" << library;
    }

    // Link with the standard runtime libs.
    commandLine << " \"-L$LEGATO_BUILD/bin/lib\" -llegato -lpthread -lrt -lm";

    if (m_Params.IsVerbose())
    {
        std::cout << std::endl << "$ " << commandLine.str() << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(commandLine);
}


//--------------------------------------------------------------------------------------------------
/**
 * Links a component instance's object files together to form a static component library (.a) file.
 */
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::LinkStaticLib
(
    legato::Component& component
)
//--------------------------------------------------------------------------------------------------
{
    // TODO: Check file timestamps to see if linking is actually needed.

    // Remove the old version of the library to be safe.
    legato::CleanFile(component.Lib().BuildOutputPath());

    // Construct the link command line, beginning with the appropriate archiver, based on the
    // target platform that we are building for.
    std::stringstream commandLine;
    commandLine << mk::GetArchiverPath(m_Params.Target());

    // Add the archiver command flags and the path to the library file to construct.
    commandLine << " rsc \"" << component.Lib().BuildOutputPath() << "\"";

    // Include all of the object (.o) files built from the component's source files.
    for (const auto& objectFile : component.ObjectFiles())
    {
        commandLine << " \"" << objectFile << "\"";
    }

    if (m_Params.IsVerbose())
    {
        std::cout << std::endl << "$ " << commandLine.str() << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(commandLine);
}
