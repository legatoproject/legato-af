//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the routines for building Components.
 *
 * Copyright (C) 201 Sierra Wireless Inc., all rights reserved.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include "ComponentBuilder.h"
#include "Utilities.h"


//--------------------------------------------------------------------------------------------------
/**
 * Generates the interface code for imported and exported interfaces.
 */
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::GenerateInterfaceCode
(
    legato::Component& component
)
//--------------------------------------------------------------------------------------------------
{
    // Don't do anything if the component doesn't import or export any interfaces.
    if (component.ImportedInterfaces().empty() && component.ExportedInterfaces().empty())
    {
        return;
    }

    // Generate the path to the directory in which the generated files will go.
    std::string dirPath = m_Params.ObjOutputDir()
                        + "/components/"
                        + component.Name();

    if (m_Params.IsVerbose())
    {
        std::cout << "Generating interface code for '" << component.Name()
                  << "' in directory '" << dirPath << "'." << std::endl;
    }

    // Make sure the directory exists.
    legato::MakeDir(dirPath);

    // Open the interface.h file for writing.
    std::string interfacesHeaderFilePath = legato::CombinePath(dirPath, "interfaces.h");
    std::ofstream interfaceHeaderFile(interfacesHeaderFilePath, std::ofstream::trunc);
    if (!interfaceHeaderFile.is_open())
    {
        throw legato::Exception("Failed to open file '" + interfacesHeaderFilePath + "'.");
    }

    // For each interface imported by the component, generate the client-side IPC code.
    const auto& importMap = component.ImportedInterfaces();
    for (auto i = importMap.cbegin(); i != importMap.cend(); i++)
    {
        const legato::ImportedInterface& interface = std::get<1>(*i);

        if (m_Params.IsVerbose())
        {
            std::cout << "  Generating code for imported interface '" << interface.InstanceName()
                      << "'." << std::endl;
        }

        mk::GenerateApiClientCode(interface.InstanceName(),
                                  interface.ApiFilePath(),
                                  dirPath,
                                  m_Params.IsVerbose());

        // Add the generated code to the component.
        std::string cFilePath(legato::AbsolutePath(legato::CombinePath(dirPath,
                                                                       interface.InstanceName()
                                                                         + "_client.c")));
        component.AddSourceFile(cFilePath);

        // Add the interface code's header to the component's interface.h file.
        interfaceHeaderFile << "#include \"" << interface.InstanceName() << "_interface.h" << "\""
                            << std::endl;
    }

    // For each interface exported by the component, generate the server-side IPC code.
    const auto& exportMap = component.ExportedInterfaces();
    for (auto i = exportMap.cbegin(); i != exportMap.cend(); i++)
    {
        const legato::Interface& interface = std::get<1>(*i);

        if (m_Params.IsVerbose())
        {
            std::cout << "  Generating code for exported interface '" << interface.InstanceName()
                      << "'." << std::endl;
        }

        mk::GenerateApiServerCode(interface.InstanceName(),
                                  interface.ApiFilePath(),
                                  dirPath,
                                  m_Params.IsVerbose());

        // Add the generated code to the component.
        std::string cFilePath(legato::AbsolutePath(legato::CombinePath(dirPath,
                                                                       interface.InstanceName()
                                                                         + "_server.c")));
        component.AddSourceFile(cFilePath);

        // Add the interface code's header to the component's interface.h file.
        interfaceHeaderFile << "#include \"" << interface.InstanceName() << "_server.h" << "\""
                            << std::endl;
    }

    // Add the directory to the include search path so the compiler can find the "interface.h" file.
    component.AddIncludeDir(dirPath);
}




//--------------------------------------------------------------------------------------------------
/**
 * Builds a component's shared library (.so) file.
 */
//--------------------------------------------------------------------------------------------------
void ComponentBuilder_t::Build
(
    const legato::Component& component  ///< The component whose library is to be built.
)
//--------------------------------------------------------------------------------------------------
{
    // Essentially, when we build a component, we use gcc to build a library (.so) from a bunch
    // of C source code files.  The library goes into the component's library output directory.

    // TODO: Check if files need recompiling.  Maybe change to use intermediate .o files
    //       to break up compiling so it runs faster for large components that have had
    //       only small changes to them.

    std::stringstream commandLine;
    commandLine << mk::GetCompilerPath(m_Params.Target());

    // Specify the output file path.
    // TODO: Add a version number to the library.
    std::string libPath = m_Params.LibOutputDir() + "/lib" + component.Name() + ".so";
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
    commandLine << " -DLE_LOG_SESSION=" << component.Name() << "_LogSession ";
    commandLine << " -DLE_LOG_LEVEL_FILTER_PTR=" << component.Name() << "_LogLevelFilterPtr ";

    // Define the COMPONENT_INIT.
    commandLine << " \"-DCOMPONENT_INIT=void " << mk::GetComponentInitName(component) << "()\"";

    // Add the CFLAGS to the command-line.
    commandLine << " " << m_Params.CCompilerFlags();

    // Add the list of C source code files.
    if (component.CSourcesList().empty())
    {
        throw legato::Exception("Component '" + component.Name() + "' has no source files.");
    }
    for (const auto& sourceFile : component.CSourcesList())
    {
        commandLine << " \"" ;

        if ((component.Path() != "") && (!legato::IsAbsolutePath(sourceFile)))
        {
            commandLine << component.Path() << "/" << sourceFile;
        }
        else
        {
            commandLine << sourceFile;
        }

        commandLine << "\"" ;
    }

    // Run the command.
    if (m_Params.IsVerbose())
    {
        std::cout << commandLine.str() << std::endl;
    }
    mk::ExecuteCommandLine(commandLine);
}


