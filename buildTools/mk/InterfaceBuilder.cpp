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
 * Generate a string containing --import-dir arguments for ifgen.
 *
 * @return The string.
 **/
//--------------------------------------------------------------------------------------------------
std::string InterfaceBuilder_t::GenerateImportDirArgs
(
    const legato::Api_t& api
)
const
//--------------------------------------------------------------------------------------------------
{
    std::string commandLine;

    // Specify all the interface search directories as places to look for .api files.
    for (auto dir : m_Params.InterfaceDirs())
    {
        commandLine += " --import-dir \"" + dir + "\"";
    }

    // Specify the directory the .api file is in as a place to look for other .api files.
    commandLine += " --import-dir \"" + legato::GetContainingDir(api.FilePath()) + "\"";

    return commandLine;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate IPC API client header files for a given protocol in a given directory.
 */
//--------------------------------------------------------------------------------------------------
void InterfaceBuilder_t::GenerateApiHeaders
(
    const legato::Api_t& api,
    const std::string& outputDir        ///< Directory where generated code should go.
)
const
//--------------------------------------------------------------------------------------------------
{
    std::stringstream commandLine;

    // Use the ifgen tool to generate the API code.
    commandLine << "ifgen --gen-local --gen-interface";

    // Specify the output directory.
    commandLine << " --output-dir \"" << outputDir << "\"";

    // Specify where ifgen should look for any additional .api files it might need.
    commandLine << GenerateImportDirArgs(api);

    // Specify the path to the protocol file.
    commandLine << " \"" << api.FilePath() << "\"";

    if (m_Params.IsVerbose())
    {
        std::cout << std::endl << "$ " << commandLine.str() << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(commandLine.str());

    // Now do the same for any other APIs that this API depends on.
    for (auto apiPtr : api.Dependencies())
    {
        GenerateApiHeaders(*apiPtr, outputDir);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate IPC API client header files for a given protocol in a given directory.
 */
//--------------------------------------------------------------------------------------------------
void InterfaceBuilder_t::GenerateApiHeaders
(
    const legato::ClientInterface& interface,
    const std::string& outputDir        ///< Directory where generated code should go.
)
const
//--------------------------------------------------------------------------------------------------
{
    std::stringstream commandLine;

    // Use the ifgen tool to generate the API code.
    commandLine << "ifgen --gen-local --gen-interface";

    // Set the C identifier prefix.
    commandLine << " --name-prefix " << interface.InternalName() << "_";

    // Set the generated file name prefix the same as the C identifier prefix.
    commandLine << " --file-prefix " << interface.InternalName() << "_";

    // Set the name to be used when talking to the Service Directory.
    commandLine << " --service-name " << interface.ExternalName();

    // Specify the output directory.
    commandLine << " --output-dir \"" << outputDir << "\"";

    // Specify where ifgen should look for any additional .api files it might need.
    commandLine << GenerateImportDirArgs(interface.Api());

    // Specify the path to the protocol file.
    commandLine << " \"" << interface.Api().FilePath() << "\"";

    if (m_Params.IsVerbose())
    {
        std::cout << std::endl << "$ " << commandLine.str() << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(commandLine.str());

    // Now do the same for any other APIs that this API depends on.
    for (auto apiPtr : interface.Api().Dependencies())
    {
        GenerateApiHeaders(*apiPtr, outputDir);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate IPC API server header files for a given protocol in a given directory.
 */
//--------------------------------------------------------------------------------------------------
void InterfaceBuilder_t::GenerateApiHeaders
(
    const legato::ServerInterface& interface, ///< Interface for which code is to be generated.
    const std::string& outputDir        ///< Directory where generated code should go.
)
const
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

    // Set the name to be used when talking to the Service Directory.
    commandLine += " --service-name " + interface.ExternalName();

    // Specify the output directory.
    commandLine += " --output-dir \"" + outputDir + "\"";

    // Specify the path to the .api file.
    commandLine += " \"" + interface.Api().FilePath() + "\"";

    // Specify where ifgen should look for any additional .api files it might need.
    commandLine += GenerateImportDirArgs(interface.Api());

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
        commandLine += " --output-dir \"" + outputDir + "\"";

        // Specify the path to the protocol file.
        commandLine += " \"" + apiPtr->FilePath() + "\"";

        // Specify where ifgen should look for any additional .api files it might need.
        commandLine += GenerateImportDirArgs(interface.Api());

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
std::string InterfaceBuilder_t::GenerateApiCode
(
    legato::ClientInterface& interface, ///< Interface object for which code is to be generated.
    const std::string& outputDir        ///< Directory where generated code should go.
)
const
//--------------------------------------------------------------------------------------------------
{
    std::stringstream commandLine;

    // Use the ifgen tool to generate the API code.
    commandLine << "ifgen --gen-client --gen-interface --gen-local";

    // Set the C identifier prefix.
    commandLine << " --name-prefix " << interface.InternalName() << "_";

    // Set the name to be used when talking to the Service Directory.
    commandLine << " --service-name " << interface.ExternalName();

    // Specify the output directory.
    commandLine << " --output-dir \"" << outputDir << "\"";

    // Specify the path to the .api file.
    commandLine << " \"" << interface.Api().FilePath() << "\"";

    // Specify where ifgen should look for any additional .api files it might need.
    commandLine << GenerateImportDirArgs(interface.Api());

    if (m_Params.IsVerbose())
    {
        std::cout << std::endl << "$ " << commandLine.str() << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(commandLine.str());

    return legato::CombinePath(outputDir, interface.Api().Name() + "_client.c");
}




//--------------------------------------------------------------------------------------------------
/**
 * Generate IPC API server code for a given interface in a given directory.
 *
 * @return Path to the generated .c file.
 */
//--------------------------------------------------------------------------------------------------
std::string InterfaceBuilder_t::GenerateApiCode
(
    legato::ServerInterface& interface, ///< Interface object for which code is to be generated.
    const std::string& outputDir        ///< Directory where generated code should go.
)
const
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

    // Set the name to be used when talking to the service directory.
    commandLine += " --service-name " + interface.ExternalName();

    // Specify the output directory.
    commandLine += " --output-dir \"" + outputDir + "\"";

    // Specify the path to the protocol file.
    commandLine += " \"" + interface.Api().FilePath() + "\"";

    // Specify where ifgen should look for any additional .api files it might need.
    commandLine += GenerateImportDirArgs(interface.Api());

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
        commandLine += " --output-dir \"" + outputDir + "\"";

        // Specify the path to the protocol file.
        commandLine += " \"" + apiPtr->FilePath() + "\"";

        // Specify all the interface search directories as places to look for interface files.
        for (auto dir : m_Params.InterfaceDirs())
        {
            commandLine += " --import-dir \"" + dir + "\"";
        }

        if (m_Params.IsVerbose())
        {
            std::cout << std::endl << "$ " << commandLine << std::endl << std::endl;
        }

        mk::ExecuteCommandLine(commandLine);
    }

    return legato::CombinePath(outputDir, interface.Api().Name() + "_server.c");
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
    std::string compilerPath = mk::GetCompilerPath(m_Params.Target(), legato::LANG_C);
    commandLine << compilerPath;

    legato::Library& lib = interface.Lib();

    if (m_Params.IsVerbose())
    {
        std::cout << "Building interface library '" << lib.BuildOutputPath() << "'." << std::endl;
    }
    commandLine << " -o " << lib.BuildOutputPath()
                << " -shared"
                << " -fPIC"
                << " -Wall";

    if(!mk::IsCompilerClang(compilerPath))
    {
        commandLine << " -Werror";
    }

    commandLine << " \"-I$LEGATO_ROOT/framework/c/inc\"";

    // Add the CFLAGS to the command-line.
    commandLine << " " << m_Params.CCompilerFlags();

    // Add the generated C source code file to the command-line.
    commandLine << " \"" << sourceFilePath << "\"" ;

    // Add the standard runtime libs.
    commandLine << " \"-L$LEGATO_BUILD/bin/lib\" -llegato -lpthread -lrt -lm";

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

    lib.MarkUpToDate();
    lib.MarkExisting();
}



//--------------------------------------------------------------------------------------------------
/**
 * Generates the code and/or library required for a provided (server-side) interface.
 */
//--------------------------------------------------------------------------------------------------
void InterfaceBuilder_t::Build
(
    legato::ServerInterface& interface,
    const std::string& objOutputDir     ///< Directory where intermediate build artifacts go.
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
        std::cout << "Generating";
        if (interface.IsAsync())
        {
            std::cout << " asynchronous";
        }
        std::cout << " server-side IPC code for provided service '"
                  << interface.ExternalName()
                  << "' using protocol '" << interface.Api().FilePath()
                  << "' with internal name '" << interface.InternalName()
                  << "'" << std::endl;

        std::cout << "    into directory '" << objOutputDir
                  << "'." << std::endl;
    }

    // Make sure the directory exists.
    legato::MakeDir(objOutputDir);

    // Set the library build output directory path (the directory where the library will be put).
    interface.Lib().BuildOutputDir(m_Params.LibOutputDir());

    GenerateApiHeaders(interface, objOutputDir);

    std::string sourceFilePath = GenerateApiCode(interface, objOutputDir);

    BuildInterfaceLibrary(interface, sourceFilePath);
}



//--------------------------------------------------------------------------------------------------
/**
 * Generates the code and/or library required for a required (client-side) interface.
 */
//--------------------------------------------------------------------------------------------------
void InterfaceBuilder_t::Build
(
    legato::ClientInterface& interface,
    const std::string& objOutputDir     ///< Directory where intermediate build artifacts go.
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
        std::cout << "Generating client-side IPC code for interface '"
                  << interface.ExternalName()
                  << "' using protocol '" << interface.Api().FilePath()
                  << "' with internal name '" << interface.InternalName()
                  << "' and external name '" << interface.ExternalName()
                  << "'." << std::endl;

        std::cout << "    into directory '" << objOutputDir
                  << "'." << std::endl;
    }

    // Make sure the directory exists.
    legato::MakeDir(objOutputDir);

    GenerateApiHeaders(interface, objOutputDir);

    // If only the typedefs are being used, then don't build anything but the API headers for
    // this interface.
    if (!interface.TypesOnly())
    {
        // Set the library build output directory path (the directory where the library will be put).
        interface.Lib().BuildOutputDir(m_Params.LibOutputDir());

        std::string sourceFilePath = GenerateApiCode(interface, objOutputDir);

        BuildInterfaceLibrary(interface, sourceFilePath);
    }
    else
    {
        interface.Lib().MarkUpToDate();
    }
}

