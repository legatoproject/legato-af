//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the routines for building Components.
 *
 * Copyright (C) 2013-2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include "ComponentBuilder.h"
#include "InterfaceBuilder.h"
#include "Utilities.h"


//--------------------------------------------------------------------------------------------------
/**
 * Generates the interfaces.h header file for this component.
 */
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::GenerateInterfacesHeader
(
    legato::Component& component
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
                  << "' in directory '" << m_Params.ObjOutputDir() << "'." << std::endl;
    }

    // Make sure the working file output directory exists.
    legato::MakeDir(m_Params.ObjOutputDir());

    // Open the interfaces.h file for writing.
    std::string interfacesHeaderFilePath = legato::CombinePath(m_Params.ObjOutputDir(),
                                                               "interfaces.h");
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

    // For each interface imported by the component, #include the client-side IPC header files.
    ///@ todo Support other programming languages besides C/C++.
    const auto& importMap = component.RequiredApis();
    for (auto i = importMap.cbegin(); i != importMap.cend(); i++)
    {
        const legato::ImportedInterface& interface = std::get<1>(*i);

        // Add the interface code's header to the component's interface.h file.
        interfaceHeaderFile << "#include \"" << interface.InternalName() << "_interface.h" << "\""
                            << std::endl;
    }

    // For each service provided by the component, #include the server-side IPC header files.
    ///@ todo Support other programming languages besides C/C++.
    const auto& exportMap = component.ProvidedApis();
    for (auto i = exportMap.cbegin(); i != exportMap.cend(); i++)
    {
        const legato::ExportedInterface& interface = std::get<1>(*i);

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
    component.AddIncludeDir(m_Params.ObjOutputDir());
}


//--------------------------------------------------------------------------------------------------
/**
 * Build IPC API libraries required by this component.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::BuildInterfaces
(
    legato::Component& component  ///< The component whose library is to be built.
)
//--------------------------------------------------------------------------------------------------
{
    // Create an Interface Builder object.
    InterfaceBuilder_t interfaceBuilder(m_Params);

    if (m_Params.IsVerbose())
    {
        std::cout << "Building interfaces for component '" << component.Name() << "'." << std::endl;
    }

    // Build the IPC API libs and add them to the list of libraries that need
    // to be bundled in the application.
    for (auto& mapEntry : component.ProvidedApis())
    {
        auto& interface = mapEntry.second;
        interfaceBuilder.Build(interface);

        // Add the library to the list of files that need to be mapped into the sandbox.
        component.AddRequiredFile({ legato::PERMISSION_READABLE,
                                 "lib/lib" + interface.Lib().ShortName() + ".so",
                                 "/lib/" });

    }
    for (auto& mapEntry : component.RequiredApis())
    {
        auto& interface = mapEntry.second;

        interfaceBuilder.Build(interface);

        if (!interface.TypesOnly())
        {
            // Add the library to the list of files that need to be mapped into the sandbox.
            component.AddRequiredFile({ legato::PERMISSION_READABLE,
                                     "lib/lib" + interface.Lib().ShortName() + ".so",
                                     "/lib/" });
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Build the component library (.so file) for a given component.
 **/
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::BuildComponentLib
(
    legato::Component& component  ///< The component whose library is to be built.
)
//--------------------------------------------------------------------------------------------------
{
    // First, generate the component's own interfaces.h header file.
    /// @todo Support other languages.
    GenerateInterfacesHeader(component);

    // Essentially, when we build a component, we use gcc to build a library (.so) from a bunch
    // of C source code files.  The library goes into the component's library output directory.

    // If the component doesn't have any C/C++ source files, then we don't need to do anything.
    if (component.CSourcesList().empty())
    {
        if (m_Params.IsVerbose())
        {
            std::cout << "Component '" << component.Name() << "' has no C/C++ source files."
                      << std::endl;
        }
        return;
    }

    // TODO: Check if files need recompiling.  Maybe change to use intermediate .o files
    //       to break up compiling so it runs faster for large components that have had
    //       only small changes to them.

    std::stringstream commandLine;
    commandLine << mk::GetCompilerPath(m_Params.Target());

    // Specify the output file path.
    // TODO: Add a version number to the library.
    std::string libFileName = "lib" + component.Name() + ".so";
    std::string libPath = legato::CombinePath(m_Params.LibOutputDir(), libFileName);
    if (m_Params.IsVerbose())
    {
        std::cout << "Building component library '" << libPath << "'." << std::endl;
    }
    commandLine << " -o " << libPath
                << " -shared"
                << " -fPIC"
                << " -Wall"
                << " -Werror";

    // Add the include paths specified on the command-line.
    for (auto i : m_Params.InterfaceDirs())
    {
        commandLine << " -I" << i;
    }

    // Add the include paths specific to the component.
    for (auto i : component.IncludePath())
    {
        commandLine << " -I" << i;
    }

    // Define the component name, log session variable, and log filter variable.
    commandLine << " -DLEGATO_COMPONENT=" << component.CName();
    commandLine << " -DLE_LOG_SESSION=" << component.CName() << "_LogSession ";
    commandLine << " -DLE_LOG_LEVEL_FILTER_PTR=" << component.CName() << "_LogLevelFilterPtr ";

    // Define the COMPONENT_INIT.
    commandLine << " \"-DCOMPONENT_INIT=LE_CI_LINKAGE void " << mk::GetComponentInitName(component)
                << "()\"";

    // Add the CFLAGS to the command-line.
    commandLine << " " << m_Params.CCompilerFlags();

    // Add the list of C source code files to the command-line.
    for (const auto& sourceFile : component.CSourcesList())
    {
        commandLine << " \"" ;

        if ((component.Path() != "") && (!legato::IsAbsolutePath(sourceFile)))
        {
            commandLine << legato::CombinePath(component.Path(), sourceFile);
        }
        else
        {
            commandLine << sourceFile;
        }

        commandLine << "\"" ;
    }

    // Add the library output path to the list of directories to be searched for library files.
    commandLine << " -L" << m_Params.LibOutputDir();

    // Add the target's sysroot lib directory to the list of directories to search for libraries.
    commandLine << " -L" << mk::GetSysRootPath(m_Params.Target());

    // Add sub-components' libraries to the command-line.
    for (const auto& mapEntry : component.SubComponents())
    {
        commandLine << " -l" << mapEntry.second->CName();
    }

    // Add the list of client and server IPC API interface library files to the command-line.
    for (const auto& mapEntry : component.RequiredApis())
    {
        auto& interface = mapEntry.second;

        // If only the typedefs are being used, then skip this interface.
        if (!interface.TypesOnly())
        {
            commandLine << " -l" << interface.Lib().ShortName();
        }
    }
    for (const auto& mapEntry : component.ProvidedApis())
    {
        commandLine << " -l" << mapEntry.second.Lib().ShortName();
    }

    // Add the list of external library files to the command-line.
    for (const auto& library : component.LibraryList())
    {
        commandLine << " -l" << library;
    }

    // Add the standard runtime libs.
    commandLine << " -L$LEGATO_BUILD/bin/lib -llegato -lpthread -lrt -lm";

    if (component.HasCppSources())
    {
        commandLine << " -lstdc++";
    }

    // On the localhost, set the DT_RUNPATH variable inside the library to include the
    // expected locations of the sub-libraries needed.
    if (m_Params.Target() == "localhost")
    {
        commandLine << " -Wl,--enable-new-dtags"
                    << ",-rpath=\"\\$ORIGIN:" << m_Params.LibOutputDir()
                                              << ":$LEGATO_ROOT/build/localhost/bin/lib\"";
    }
    // On embedded targets, set the DT_RUNPATH variable inside the library to include the
    // expected location of libraries bundled in this application (this is needed for unsandboxed
    // applications).
    else
    {
        commandLine << " -Wl,--enable-new-dtags,-rpath=\"\\$ORIGIN\"";
    }

    if (m_Params.IsVerbose())
    {
        std::cout << std::endl << "$ " << commandLine.str() << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(commandLine);

    // Add the component library to the list of files that need to be mapped into the sandbox.
    // NOTE: Technically speaking, this file is bundled as a part of the app, but because it is
    // being built right into the app's staging area, it will get included in the application
    // image anyway, so we just need to specify the mapping as an "external" file so it gets
    // bind mounted into the sandbox when the app starts up.
    // Source path is relative to app install dir.
    component.AddRequiredFile({ legato::PERMISSION_READABLE,
                              "lib/lib" + component.Lib().ShortName() + ".so",
                              "/lib/" });
}


//--------------------------------------------------------------------------------------------------
/**
 * Builds a component, including copying bundled files to the staging area, etc.
 */
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::Build
(
    legato::Component& component  ///< The component to be built.
)
//--------------------------------------------------------------------------------------------------
{
    // If the component is already up-to-date, then we don't need to do anything.
    if (component.Lib().IsUpToDate())
    {
        if (m_Params.IsVerbose())
        {
            std::cout << "Component '" << component.Name() << "' is up-to-date." << std::endl;
        }
        return;
    }

    // Do dependency loop detection.
    if (component.BeingProcessed())
    {
        throw legato::DependencyException("Dependency loop detected in component: "
                                          + component.Name());
    }
    component.BeingProcessed(true);

    // Build the IPC API libraries needed by this component.
    BuildInterfaces(component);

    // Build sub-components needed by this component, before building this component.
    // Note, we use a recursive, depth-first tree walk over the components dependency tree so
    // that the build happens in the correct order (lower-level stuff gets built before the
    // higher-level stuff that needs it).
    try
    {
        for (auto& mapEntry : component.SubComponents())
        {
            Build(*(mapEntry.second));
        }
    }
    catch (legato::DependencyException e)
    {
        throw legato::DependencyException(e.what() + std::string(" required by ")
                                          + component.Name());
    }

    // Copy all bundled files into the staging area.
    auto& bundledFilesList = component.BundledFiles();
    for (auto fileMapping : bundledFilesList)
    {
        mk::CopyToStaging(  fileMapping.m_SourcePath,
                            m_Params.StagingDir(),
                            fileMapping.m_DestPath,
                            m_Params.IsVerbose()    );
    }

    // Copy all bundled directories into the staging area.
    auto& bundledDirsList = component.BundledDirs();
    for (auto fileMapping : bundledDirsList)
    {
        mk::CopyToStaging(  fileMapping.m_SourcePath,
                            m_Params.StagingDir(),
                            fileMapping.m_DestPath,
                            m_Params.IsVerbose()    );
    }

    // Build this component's library.
    BuildComponentLib(component);

    component.BeingProcessed(false);
}
