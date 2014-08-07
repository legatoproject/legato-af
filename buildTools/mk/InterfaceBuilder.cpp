//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the routines for building auto-generated IPC Interfaces.
 *
 * Copyright (C) 2014 Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include "InterfaceBuilder.h"
#include "Utilities.h"


//--------------------------------------------------------------------------------------------------
/**
 * Generate IPC API client header files for a given protocol in a given directory.
 */
//--------------------------------------------------------------------------------------------------
void InterfaceBuilder_t::GenerateApiClientHeaders
(
    const legato::ImportedInterface& interface
)
//--------------------------------------------------------------------------------------------------
{
    std::stringstream commandLine;

    // Use the ifgen tool to generate the API code.
    commandLine << "ifgen --gen-local --gen-interface";

    // Set the C identifier prefix.
    commandLine << " --name-prefix " << interface.InternalName() << "_";

    // Set the generated file name prefix the same as the C identifier prefix.
    commandLine << " --file-prefix " << interface.InternalName() << "_";

    // Specify the output directory.
    commandLine << " --output-dir " << m_Params.ObjOutputDir();

    // Specify all the interface search directories as places to look for interface files.
    for (auto dir : m_Params.InterfaceDirs())
    {
        commandLine << " --import-dir " << dir;
    }

    // Specify the path to the protocol file.
    commandLine << " " << interface.Api().FilePath();

    if (m_Params.IsVerbose())
    {
        std::cout << std::endl << "$ " << commandLine.str() << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(commandLine.str());
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate IPC API server header files for a given protocol in a given directory.
 */
//--------------------------------------------------------------------------------------------------
void InterfaceBuilder_t::GenerateApiServerHeaders
(
    const legato::ExportedInterface& interface  ///< Interface for which code is to be generated.
)
//--------------------------------------------------------------------------------------------------
{
    std::string commandLine;

    // Use the ifgen tool to generate the API code.
    commandLine = "ifgen --gen-local --gen-server-interface";

    // Tell ifgen if the server needs to handle requests asynchronously.
    if (interface.IsAsync())
    {
        commandLine += " --async-server";
    }

    // Set the C identifier prefix.
    commandLine += " --name-prefix " + interface.InternalName() + "_";

    // Set the generated file name prefix the same as the C identifier prefix.
    commandLine += " --file-prefix " + interface.InternalName() + "_";

    // Specify the output directory.
    commandLine += " --output-dir " + m_Params.ObjOutputDir();

    // Specify the path to the protocol file.
    commandLine += " " + interface.Api().FilePath();

    // Specify all the interface search directories as places to look for interface files.
    for (auto dir : m_Params.InterfaceDirs())
    {
        commandLine += " --import-dir " + dir;
    }

    if (m_Params.IsVerbose())
    {
        std::cout << std::endl << "$ " << commandLine << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(commandLine);

    // For each API that this API imports types from, also generate that API's server header.
    for (auto apiPtr : interface.Api().Dependencies())
    {
        commandLine = "ifgen --gen-server-interface";

        // Tell ifgen if the server needs to handle requests asynchronously.
        if (interface.IsAsync())
        {
            commandLine += " --async-server";
        }

        // Specify the output directory.
        commandLine += " --output-dir " + m_Params.ObjOutputDir();

        // Specify the path to the protocol file.
        commandLine += " " + apiPtr->FilePath();

        // Specify all the interface search directories as places to look for interface files.
        for (auto dir : m_Params.InterfaceDirs())
        {
            commandLine += " --import-dir " + dir;
        }

        if (m_Params.IsVerbose())
        {
            std::cout << std::endl << "$ " << commandLine << std::endl << std::endl;
        }

        mk::ExecuteCommandLine(commandLine);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate IPC API client code for a given interface in a given directory.
 *
 * @return Path to the generated .c file.
 */
//--------------------------------------------------------------------------------------------------
std::string InterfaceBuilder_t::GenerateApiClientCode
(
    legato::ImportedInterface& interface    ///< Interface object for which code is to be generated.
)
//--------------------------------------------------------------------------------------------------
{
    std::stringstream commandLine;

    // Use the ifgen tool to generate the API code.
    commandLine << "ifgen --gen-client --gen-interface --gen-local";

    // Set the C identifier prefix.
    commandLine << " --name-prefix " << interface.InternalName() << "_";

    // Specify the output directory.
    commandLine << " --output-dir " << m_Params.ObjOutputDir();

    // Specify the path to the protocol file.
    commandLine << " " << interface.Api().FilePath();

    // Specify all the interface search directories as places to look for interface files.
    for (auto dir : m_Params.InterfaceDirs())
    {
        commandLine << " --import-dir " << dir;
    }

    if (m_Params.IsVerbose())
    {
        std::cout << std::endl << "$ " << commandLine.str() << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(commandLine.str());

    return m_Params.ObjOutputDir() + "/" + interface.Api().Name() + "_client.c";
}




//--------------------------------------------------------------------------------------------------
/**
 * Generate IPC API server code for a given interface in a given directory.
 *
 * @return Path to the generated .c file.
 */
//--------------------------------------------------------------------------------------------------
std::string InterfaceBuilder_t::GenerateApiServerCode
(
    legato::ExportedInterface& interface    ///< Interface object for which code is to be generated.
)
//--------------------------------------------------------------------------------------------------
{
    std::string commandLine;

    // Use the ifgen tool to generate the API code.
    commandLine = "ifgen --gen-server --gen-server-interface --gen-local";

    // Tell ifgen if the server needs to handle requests asynchronously.
    if (interface.IsAsync())
    {
        commandLine += " --async-server";
    }

    // Set the C identifier prefix.
    commandLine += " --name-prefix " + interface.InternalName() + "_";

    // Specify the output directory.
    commandLine += " --output-dir " + m_Params.ObjOutputDir();

    // Specify the path to the protocol file.
    commandLine += " " + interface.Api().FilePath();

    // Specify all the interface search directories as places to look for interface files.
    for (auto dir : m_Params.InterfaceDirs())
    {
        commandLine += " --import-dir " + dir;
    }

    if (m_Params.IsVerbose())
    {
        std::cout << std::endl << "$ " << commandLine << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(commandLine);

    // For each API that this API imports types from, also generate that API's server header.
    for (auto apiPtr : interface.Api().Dependencies())
    {
        commandLine = "ifgen --gen-server-interface";

        // Tell ifgen if the server needs to handle requests asynchronously.
        if (interface.IsAsync())
        {
            commandLine += " --async-server";
        }

        // Specify the output directory.
        commandLine += " --output-dir " + m_Params.ObjOutputDir();

        // Specify the path to the protocol file.
        commandLine += " " + apiPtr->FilePath();

        // Specify all the interface search directories as places to look for interface files.
        for (auto dir : m_Params.InterfaceDirs())
        {
            commandLine += " --import-dir " + dir;
        }

        if (m_Params.IsVerbose())
        {
            std::cout << std::endl << "$ " << commandLine << std::endl << std::endl;
        }

        mk::ExecuteCommandLine(commandLine);
    }

    return m_Params.ObjOutputDir() + "/" + interface.Api().Name() + "_server.c";
}



//--------------------------------------------------------------------------------------------------
/**
 * Compile/link an interface library (.so file) for a given interface from its source file.
 **/
//--------------------------------------------------------------------------------------------------
void InterfaceBuilder_t::BuildInterfaceLibrary
(
    legato::Interface& interface,
    const std::string& sourceFilePath
)
//--------------------------------------------------------------------------------------------------
{
    // Build the interface library from the generated code using the appropriate C/C++ compiler.
    std::stringstream commandLine;
    commandLine << mk::GetCompilerPath(m_Params.Target());

    const legato::Library& lib = interface.Lib();

    if (m_Params.IsVerbose())
    {
        std::cout << "Building interface library '" << lib.BuildOutputPath() << "'." << std::endl;
    }
    commandLine << " -o " << lib.BuildOutputPath()
                << " -shared"
                << " -fPIC"
                << " -Wall"
                << " -Werror"
                << " -I$LEGATO_ROOT/framework/c/inc";

    // Add the CFLAGS to the command-line.
    commandLine << " " << m_Params.CCompilerFlags();

    // Add the generated C source code file to the command-line.
    commandLine << " \"" << sourceFilePath << "\"" ;

    // Add the standard runtime libs.
    commandLine << " -L$LEGATO_BUILD/bin/lib -llegato -lpthread -lrt -lm";

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

    interface.Lib().MarkUpToDate();
}



//--------------------------------------------------------------------------------------------------
/**
 * Generates the code and/or library required for an exported interface.
 */
//--------------------------------------------------------------------------------------------------
void InterfaceBuilder_t::Build
(
    legato::ExportedInterface& interface
)
//--------------------------------------------------------------------------------------------------
{
    // NOTE: For now, ifgen can't share code between multiple server-side instances of the same API
    //       in the same process, so we have to generate a unique .so for each.

    // If this interface has already been built, we shouldn't do it again.
    if (interface.Lib().IsUpToDate())
    {
        if (m_Params.IsVerbose())
        {
            std::cout << "Interface '" << interface.InternalName()
                      << "' already up to date." << std::endl;
        }

        return;
    }

    if (m_Params.IsVerbose())
    {
        // If the interface is for something generated outside of an app, then use the
        // service instance name instead of the app-wide unique name.
        std::string interfaceName = interface.AppUniqueName();
        if (interfaceName.empty())
        {
            interfaceName = interface.InternalName();
        }

        std::cout << "Generating";
        if (interface.IsAsync())
        {
            std::cout << " asynchronous";
        }
        std::cout << " server-side IPC code for exported service '"
                  << interfaceName
                  << "' using protocol '" << interface.Api().FilePath()
                  << "' with internal name '" << interface.InternalName()
                  << "'" << std::endl;

        std::cout << "    into directory '" << m_Params.ObjOutputDir()
                  << "'." << std::endl;
    }

    // Make sure the directory exists.
    legato::MakeDir(m_Params.ObjOutputDir());

    // Set the library build output directory path (the directory where the library will be put).
    // TODO: Add a version number to the library.
    interface.Lib().BuildOutputDir(m_Params.LibOutputDir());

    GenerateApiServerHeaders(interface);

    std::string sourceFilePath = GenerateApiServerCode(interface);

    BuildInterfaceLibrary(interface, sourceFilePath);
}



//--------------------------------------------------------------------------------------------------
/**
 * Generates the code and/or library required for an imported interface.
 */
//--------------------------------------------------------------------------------------------------
void InterfaceBuilder_t::Build
(
    legato::ImportedInterface& interface
)
//--------------------------------------------------------------------------------------------------
{
    // NOTE: For now, ifgen can't share code between multiple client-side instances of the same API
    //       in the same process, so we have to generate a unique .so for each.

    // If this interface has already been built, we shouldn't do it again.
    if (interface.Lib().IsUpToDate())
    {
        if (m_Params.IsVerbose())
        {
            std::cout << "Interface '" << interface.InternalName()
                      << "' already up to date." << std::endl;
        }

        return;
    }

    if (m_Params.IsVerbose())
    {
        // If the interface is for something generated outside of an app, then use the
        // service instance name instead of the app-wide unique name.
        std::string interfaceName = interface.AppUniqueName();
        if (interfaceName.empty())
        {
            interfaceName = interface.InternalName();
        }

        std::cout << "Generating client-side IPC code for interface '"
                  << interfaceName
                  << "' using protocol '" << interface.Api().FilePath()
                  << "' with internal name '" << interface.InternalName()
                  << "'." << std::endl;

        std::cout << "    into directory '" << m_Params.ObjOutputDir()
                  << "'." << std::endl;
    }

    // Make sure the directory exists.
    legato::MakeDir(m_Params.ObjOutputDir());

    GenerateApiClientHeaders(interface);

    // If only the typedefs are being used, then don't build anything but the API headers for
    // this interface.
    if (!interface.TypesOnly())
    {
        // Set the library build output directory path (the directory where the library will be put).
        // TODO: Add a version number to the library.
        interface.Lib().BuildOutputDir(m_Params.LibOutputDir());

        std::string sourceFilePath = GenerateApiClientCode(interface);

        BuildInterfaceLibrary(interface, sourceFilePath);
    }
    else
    {
        interface.Lib().MarkUpToDate();
    }
}

