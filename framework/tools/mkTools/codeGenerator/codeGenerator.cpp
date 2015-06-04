//--------------------------------------------------------------------------------------------------
/**
 * @file codeGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace code
{


//--------------------------------------------------------------------------------------------------
/**
 * Generate an interfaces.h file for a given component.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateInterfacesHeader
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string outputDir = path::Minimize(buildParams.workingDir
                                        + '/'
                                        + componentPtr->workingDir
                                        + "/src");
    std::string filePath = outputDir + "/interfaces.h";

    if (buildParams.beVerbose)
    {
        std::cout << "Generating interfaces.h for component '" << componentPtr->name << "'"
                     "in '" << filePath << "'." << std::endl;
    }

    // Make sure the working file output directory exists.
    file::MakeDir(outputDir);

    // Open the interfaces.h file for writing.
    std::ofstream fileStream(filePath, std::ofstream::trunc);
    if (!fileStream.is_open())
    {
        throw mk::Exception_t("Failed to open file '" + filePath + "' for writing.");
    }

    std::string includeGuardName = "__" + componentPtr->name
                                        + "_COMPONENT_INTERFACE_H_INCLUDE_GUARD";

    fileStream << "/*\n"
                  " * AUTO-GENERATED interface.h for the " << componentPtr->name << " component.\n"
                  "\n"
                  " * Don't bother hand-editing this file.\n"
                  " */\n"
                  "\n"
                  "#ifndef " << includeGuardName << "\n"
                  "#define " << includeGuardName << "\n"
                  "\n"
                  "#ifdef __cplusplus\n"
                  "extern \"C\" {\n"
                  "#endif\n"
                  "\n";

    // #include the client-side .h for each of the .api files from which only data types are used.
    for (auto interfacePtr : componentPtr->typesOnlyApis)
    {
        fileStream << "#include \"" << interfacePtr->internalName << "_interface.h" << "\"\n";
    }

    // For each of the component's client-side interfaces, #include the client-side .h file.
    for (auto interfacePtr : componentPtr->clientApis)
    {
        fileStream << "#include \"" << interfacePtr->internalName << "_interface.h" << "\"\n";
    }

    // For each of the component's server-side interfaces, #include the server-side .h file.
    for (auto interfacePtr : componentPtr->serverApis)
    {
        fileStream << "#include \"" << interfacePtr->internalName << "_server.h" << "\"\n";
    }

    // Put the finishing touches on interfaces.h.
    fileStream << "\n"
                  "#ifdef __cplusplus\n"
                  "}\n"
                  "#endif\n"
                  "\n"
                  "#endif // " << includeGuardName << "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate _componentMain.c for a given component.
 *
 * This resolves the undefined service name symbols in all the interfaces' .o files
 * and creates a component-specific interface initialization function.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateComponentMainFile
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams,
    bool isStandAlone   ///< true = fully resolve all interface name variables.
)
//--------------------------------------------------------------------------------------------------
{
    std::string outputDir = path::Minimize(buildParams.workingDir
                                        + '/'
                                        + componentPtr->workingDir
                                        + "/src");
    std::string filePath = outputDir + "/_componentMain.c";

    if (buildParams.beVerbose)
    {
        std::cout << "Generating component-specific IPC code for component '" <<
                     componentPtr->name << "' in '" << filePath << "'." << std::endl;
    }

    // Make sure the working file output directory exists.
    file::MakeDir(outputDir);

    // Open the serviceNames.c file for writing.
    std::ofstream fileStream(filePath, std::ofstream::trunc);
    if (!fileStream.is_open())
    {
        throw mk::Exception_t("Failed to open file '" + filePath + "' for writing.");
    }

    fileStream << "/*\n"
                  " * AUTO-GENERATED _componentMain.c for the " << componentPtr->name <<
                  " component.\n"
                  "\n"
                  " * Don't bother hand-editing this file.\n"
                  " */\n"
                  "\n"
                  "#ifdef __cplusplus\n"
                  "extern \"C\" {\n"
                  "#endif\n"
                  "\n";

    // For each of the component's client-side interfaces, define service name variables
    // and declare the initialization function.
    for (auto interfacePtr : componentPtr->clientApis)
    {
        std::string ifLevelVar = interfacePtr->internalName + "_ServiceInstanceNamePtr";

        // If the component is being built for use by an executable built by mkexe, mkapp, or mksys,
        // then create an extern variable declaration that will be satisfied by the generated
        // _main.c for the executable, thereby allowing exe-specific interface instance naming.
        if (!isStandAlone)
        {
            std::string exeLevelVar = "_" + componentPtr->name + "_" + interfacePtr->internalName
                                    + "_ServiceInstanceName";

            fileStream << "extern const char* " << exeLevelVar << ";\n";

            fileStream << "const char** " << ifLevelVar << " = &" << exeLevelVar << ";\n\n";
        }
        // If the component is being built for stand-alone use, then fully resolve the interface
        // instance names, using the internal name as the name to send to the Service Directory.
        else
        {
            std::string constName = interfacePtr->internalName + "_InterfaceName";
            fileStream << "static const char* " << constName << " = \""
                                                        << interfacePtr->internalName << "\";\n";
            fileStream << "const char** " << ifLevelVar << " = &" << constName << ";\n\n";
        }

        // If not marked for manual start,
        if (!(interfacePtr->manualStart))
        {
            // Declare the client-side interface initialization function.
            fileStream << "void " << interfacePtr->internalName << "_ConnectService(void);\n\n";
        }
    }

    // For each of the component's server-side interfaces, define service name variables
    // and declare the initialization function.
    for (auto interfacePtr : componentPtr->serverApis)
    {
        std::string ifLevelVar = interfacePtr->internalName + "_ServiceInstanceNamePtr";

        // If the component is being built for use by an executable built by mkexe, mkapp, or mksys,
        // then create an extern variable declaration that will be satisfied by the generated
        // _main.c for the executable, thereby allowing exe-specific interface instance naming.
        if (!isStandAlone)
        {
            std::string exeLevelVar = "_" + componentPtr->name + "_" + interfacePtr->internalName
                                    + "_ServiceInstanceName";

            fileStream << "extern const char* " << exeLevelVar << ";\n";

            fileStream << "const char** " << ifLevelVar << " = &" << exeLevelVar << ";\n\n";
        }
        // If the component is being built for stand-alone use, then fully resolve the interface
        // instance names, using the internal name as the name to send to the Service Directory.
        else
        {
            std::string constName = interfacePtr->internalName + "_InterfaceName";
            fileStream << "static const char " << constName << "[] = \""
                                                        << interfacePtr->internalName << "\";\n";
            fileStream << "const char** " << ifLevelVar << " = &" << constName << ";\n\n";
        }

        // If not marked for manual start,
        if (!(interfacePtr->manualStart))
        {
            // Declare the server-side interface initialization function.
            fileStream << "void " << interfacePtr->internalName << "_AdvertiseService(void);\n\n";
        }
    }

    // Define the component's server-side IPC initialization function.
    fileStream << "// IPC API server initialization function.\n"
                  "// To be called by the executable at startup.\n"
                  "void _" << componentPtr->name << "_InitServerIpcInterfaces(void)\n"
                  "{\n";
    // Call each of the component's server-side interfaces' initialization functions,
    // except those that are marked [manual-start].
    for (auto ifPtr : componentPtr->serverApis)
    {
        // If not marked for manual start,
        if (!(ifPtr->manualStart))
        {
            // Call the interface initialization function.
            fileStream << "    " << ifPtr->internalName << "_AdvertiseService();\n";
        }
    }
    fileStream << "}\n\n";

    // Define the component's client-side IPC initialization function.
    fileStream << "// IPC API client initialization function.\n"
                  "// To be called by the executable at startup.\n"
                  "void _" << componentPtr->name << "_InitClientIpcInterfaces(void)\n"
                  "{\n";
    // Call each of the component's client-side interfaces' initialization functions,
    // except those that are marked [manual-start].
    for (auto ifPtr : componentPtr->clientApis)
    {
        // If not marked for manual start,
        if (!(ifPtr->manualStart))
        {
            // Call the interface initialization function.
            fileStream << "    " << ifPtr->internalName << "_ConnectService();\n";
        }
    }
    fileStream << "}\n\n";

    // Put the finishing touches on the file.
    fileStream << "\n"
                  "#ifdef __cplusplus\n"
                  "}\n"
                  "#endif\n";
}



//--------------------------------------------------------------------------------------------------
/**
 * Generate component initializer declarations and functions calls for a given component
 * and all its sub-functions.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateComponentInitFunctionCall
(
    model::ComponentInstance_t* instancePtr,
    std::stringstream& compInitDeclBuffer,  ///< Buffer to put init function declarations in.
    std::stringstream& compInitBuffer       ///< Buffer to put init function calls in.
)
//--------------------------------------------------------------------------------------------------
{
    // If the component has a COMPONENT_INIT that needs to be run,
    auto componentPtr = instancePtr->componentPtr;
    if ((!componentPtr->cSources.empty()) || (!componentPtr->cxxSources.empty()))
    {
        std::string initFuncName = "_" + componentPtr->name + "_COMPONENT_INIT";

        // Generate forward declaration of the component initializer.
        compInitDeclBuffer << "void " << initFuncName << "(void);" << std::endl;

        // Generate call to queue the initialization function to the event loop.
        compInitBuffer << "    event_QueueComponentInit(" << initFuncName << ");"
                       << std::endl;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate COMPONENT_INIT function declarations and functions calls for all components in an
 * executable.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateComponentInitFunctionCalls
(
    const model::Exe_t* exePtr,
    std::stringstream& compInitDeclBuffer,  ///< Buffer to put init function declarations in.
    std::stringstream& compInitBuffer       ///< Buffer to put init function calls in.
)
//--------------------------------------------------------------------------------------------------
{
    // NOTE: The executable's list of component instances is already sorted such that components
    //       appear later than any other components that they use (require).
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        GenerateComponentInitFunctionCall(componentInstancePtr,
                                          compInitDeclBuffer,
                                          compInitBuffer);
    }

    // If there are C/C++ source files other than the _main.c file, then the executable's
    // "default component" needs to have its COMPONENT_INIT run too.
    if ((!exePtr->cSources.empty()) || (!exePtr->cxxSources.empty()))
    {
        std::string initFuncName = "_" + exePtr->name + "_exe_COMPONENT_INIT";

        // Generate forward declaration of the component initializer.
        compInitDeclBuffer << "void " << initFuncName << "(void);" << std::endl;

        // Queue up the component initializer to be called when the Event Loop starts.
        compInitBuffer << "    event_QueueComponentInit(" << initFuncName << ");"
                       << std::endl;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generates a main .c for a given executable.
 */
//--------------------------------------------------------------------------------------------------
void GenerateExeMain
(
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto& exeName = exePtr->name;

    auto sourceFile = path::Combine(buildParams.workingDir, exePtr->mainCSourceFile);

    if (buildParams.beVerbose)
    {
        std::cout << "Generating startup code for executable '" << exeName << "'"
                     " (" << exePtr->path << ") "
                     "in '" << sourceFile << "'." << std::endl;
    }

    // The code is broken down into multiple sections.  This is to make writing the code
    // generator a little easier.  We just have to stream the various parts into the
    // required sections, so we only need to loop over our data structures once.
    std::stringstream compInitDeclBuffer;
    std::stringstream sessionBuffer;
    std::stringstream filterBuffer;
    std::stringstream serverNameBuffer;
    std::stringstream clientNameBuffer;
    std::stringstream serverInitDeclBuffer;
    std::stringstream clientInitDeclBuffer;
    std::stringstream logInitBuffer;
    std::stringstream serverInitBuffer;
    std::stringstream clientInitBuffer;
    std::stringstream compInitBuffer;

    // Commented sections to make the generated code more readable.
    compInitDeclBuffer<< "// Declare all component initializers." << std::endl;
    sessionBuffer     << "// Declare component log session variables." << std::endl;
    filterBuffer      << "// Declare log filter level pointer variables." << std::endl;
    serverNameBuffer << "// Declare server-side IPC API interface names." << std::endl;
    clientNameBuffer << "// Declare client-side IPC API interface names." << std::endl;
    serverInitDeclBuffer << "// Declare server-side IPC API interface initializers." << std::endl;
    clientInitDeclBuffer << "// Declare client-side IPC API interface initializers." << std::endl;
    logInitBuffer     << "    // Initialize all log sessions." << std::endl;
    serverInitBuffer  << "    // Initialize all server-side IPC API interfaces." << std::endl;
    clientInitBuffer  << "    // Initialize all client-side IPC API interfaces." << std::endl;
    compInitBuffer    << "    // Schedule component initializers for execution by the event loop."
                      << std::endl;

    // Iterate over the list of Component Instances,
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        auto componentPtr = componentInstancePtr->componentPtr;

        const auto& compName = componentPtr->name;

        // Define the component's Log Session Reference variable.
        sessionBuffer  << "le_log_SessionRef_t " << compName << "_LogSession;" << std::endl;

        // Define the component's Log Filter Level variable.
        filterBuffer   << "le_log_Level_t* " << compName << "_LogLevelFilterPtr;" << std::endl;

        // Register the component with the Log Control Daemon.
        logInitBuffer  << "    " << compName << "_LogSession = log_RegComponent(\""
                       << compName << "\", &" << compName << "_LogLevelFilterPtr);"
                       << std::endl;

        // For each of the component instance's server-side interfaces,
        for (auto ifInstancePtr : componentInstancePtr->serverApis)
        {
            auto& internalName = ifInstancePtr->ifPtr->internalName;

            // Define the interface name variable.
            serverNameBuffer << "const char* _" << compName << "_" << internalName
                             << "_ServiceInstanceName = \"" << ifInstancePtr->name << "\";"
                             << std::endl;
        }

        // For each of the component instance's client-side interfaces,
        for (auto ifInstancePtr : componentInstancePtr->clientApis)
        {
            auto& internalName = ifInstancePtr->ifPtr->internalName;

            // Define the interface name variable.
            clientNameBuffer << "const char* _" << compName << "_" << internalName
                             << "_ServiceInstanceName = \"" << ifInstancePtr->name << "\";"
                             << std::endl;

        }

        // Declare the component's IPC initialization functions and call them.
        if (!(componentInstancePtr->serverApis.empty()))
        {
            serverInitDeclBuffer << "void _" << compName << "_InitServerIpcInterfaces(void);\n";
            serverInitBuffer << "    _" << compName << "_InitServerIpcInterfaces();\n";
        }
        if (!(componentInstancePtr->clientApis.empty()))
        {
            clientInitDeclBuffer << "void _" << compName << "_InitClientIpcInterfaces(void);\n";
            clientInitBuffer << "    _" << compName << "_InitClientIpcInterfaces();\n";
        }
    }

    // For the executable's "default" component.
    std::string compName = exeName + "_exe";

    // Define the component's Log Session Reference variable.
    sessionBuffer  << "le_log_SessionRef_t " << compName << "_LogSession;" << std::endl;

    // Define the component's Log Filter Level variable.
    filterBuffer   << "le_log_Level_t* " << compName << "_LogLevelFilterPtr;" << std::endl;

    // Register the component with the Log Control Daemon.
    logInitBuffer  << "    " << compName << "_LogSession = log_RegComponent(\""
                   << compName << "\", &" << compName << "_LogLevelFilterPtr);"
                   << std::endl;

    // Queue up the COMPONENT_INITs to be called when the Event Loop starts.
    GenerateComponentInitFunctionCalls(exePtr, compInitDeclBuffer, compInitBuffer);

    // Now that we have all of our subsections filled out, dump all of the generated code
    // and the static template code to the target output file.

    // Open the file as an output stream.
    file::MakeDir(path::GetContainingDir(sourceFile));
    std::ofstream outputFile(sourceFile);
    if (outputFile.is_open() == false)
    {
        throw mk::Exception_t("Could not open, '" + sourceFile + ",' for writing.");
    }

    outputFile  << std::endl
                << "// Startup code for the executable '" << exeName << "'." << std::endl
                << "// This is a generated file, do not edit." << std::endl
                << std::endl
                << "#include \"legato.h\"" << std::endl
                << "#include \"../src/eventLoop.h\"" << std::endl
                << "#include \"../src/log.h\"" << std::endl
                << "#include \"../src/args.h\"" << std::endl

                << std::endl
                << std::endl

                << serverNameBuffer.str() << std::endl
                << serverInitDeclBuffer.str() << std::endl

                << clientNameBuffer.str() << std::endl
                << clientInitDeclBuffer.str() << std::endl

                << compInitDeclBuffer.str() << std::endl

                << sessionBuffer.str() << std::endl

                << filterBuffer.str() << std::endl
                << std::endl
                << std::endl

                << "int main(int argc, char* argv[])" << std::endl
                << "{" << std::endl
                << "    // Gather the program arguments for later processing." << std::endl
                << "    arg_SetArgs((size_t)argc, (char**)argv);" << std::endl
                << std::endl

                << logInitBuffer.str() << std::endl
                << "    // Connect to the log control daemon." << std::endl
                << "    // Note that there are some rare cases where we don't want the" << std::endl
                << "    // process to try to connect to the Log Control Daemon (e.g.," << std::endl
                << "    // the Supervisor and the Service Directory shouldn't)." << std::endl
                << "    // The NO_LOG_CONTROL macro can be used to control that." << std::endl
                << "    #ifndef NO_LOG_CONTROL" << std::endl
                << "        log_ConnectToControlDaemon();" << std::endl
                << "    #else" << std::endl
                << "        LE_DEBUG(\"Not connecting to the Log Control Daemon.\");" << std::endl
                << "    #endif" << std::endl
                << std::endl
                << "    LE_DEBUG(\"== Log sessions registered. ==\");" << std::endl
                << std::endl

                // Queue component initialization functions for execution by the event loop.
                << compInitBuffer.str() << std::endl

                // Start server-side IPC interfaces.  Any events that are caused by
                // this will get handled after the component initializers because those
                // are already on the event queue.
                << serverInitBuffer.str() << std::endl

                // Start client-side IPC interfaces.  If there are any clients in
                // this thread that are bound to services provided by servers in this thread,
                // then at least we won't have the initialization deadlock of clients blocked
                // waiting for services that are yet to be advertised by the same thread.
                // However, until we support component-specific event loops and side-processing
                // of other components' events while blocked, we will still have deadlocks
                // if bound-together clients and servers are running in the same thread.
                << clientInitBuffer.str() << std::endl

                << "    LE_DEBUG(\"== Starting Event Processing Loop ==\");" << std::endl
                << "    le_event_RunLoop();" << std::endl
                << "    LE_FATAL(\"== SHOULDN'T GET HERE! ==\");" << std::endl
                << "}" << std::endl;

    outputFile.close();
}



} // namespace code
