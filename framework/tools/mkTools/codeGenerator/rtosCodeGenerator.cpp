//--------------------------------------------------------------------------------------------------
/**
 * @file codeGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


namespace code
{


//--------------------------------------------------------------------------------------------------
/**
 * Generate prototype for a component Init function.
 */
//--------------------------------------------------------------------------------------------------
void GenerateComponentInitPrototype
(
    std::ostream &output,
    model::Component_t* componentPtr
)
{
    int argCount = 0;
    output << "void _" << componentPtr->name << "_Init(";
    for (auto serverApiPtr : componentPtr->serverApis)
    {
        if (argCount++ != 0)
        {
            output << ", ";
        }
        output << "le_msg_LocalService_t* " << serverApiPtr->internalName << "Ptr";
    }
    for (auto clientApiPtr : componentPtr->clientApis)
    {
        if (argCount++ != 0)
        {
            output << ", ";
        }
        output << "le_msg_LocalService_t* " << clientApiPtr->internalName << "Ptr";
    }
    output << ")";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate prototype for a per-component service Init function.
 */
//--------------------------------------------------------------------------------------------------
void GenerateEarlyInitPrototype
(
    std::ostream &output,
    model::Component_t* componentPtr
)
{
    output << "void _"<< componentPtr->name << "_InitEarly(";
    int argCount = 0;
    for (auto ifPtr : componentPtr->serverApis)
    {
        if (argCount++ != 0)
        {
            output << ", ";
        }
        output << "le_msg_LocalService_t* " << ifPtr->internalName << "Ptr";
    }
    output << ")";
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate _componentMain.c for a given component.
 *
 * This resolves the undefined service name symbols in all the interfaces' .o files
 * and creates a component-specific interface initialization function.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateRtosComponentMainFile
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    if (componentPtr->HasJavaCode())
    {
        componentPtr->defFilePtr->ThrowException("Java is not supported on RTOS targets");
    }

    if (!componentPtr->HasCOrCppCode())
    {
        // Nothing to do
        return;
    }

    auto& compName = componentPtr->name;

    // This generator is for RTOS & generates necessary code for RTOS task initialization.
    // Add the component-specific info now (if not already present)
    componentPtr->SetTargetInfo(new target::RtosComponentInfo_t(componentPtr, buildParams));

    std::string componentInitFuncName = componentPtr->initFuncName;

    // Compute the path to the output file.
    std::string outputDir = path::Minimize(buildParams.workingDir
                                        + '/'
                                        + componentPtr->workingDir
                                        + "/src");
    std::string filePath = outputDir + "/_componentMain.c";

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Generating component-specific initialization code for"
                                        " component '%s' in '%s'."),
                                compName, filePath)
                  << std::endl;
    }

    // Open the .c file for writing.
    file::MakeDir(outputDir);
    std::ofstream fileStream(filePath, std::ofstream::trunc);
    if (!fileStream.is_open())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for writing."), filePath)
        );
    }

    // Generate file header and #include directives,
    // define the default component's log session variables,
    // and perform initial RTOS task setup such as setting log control variables.
    fileStream << "/*\n"
                  " * AUTO-GENERATED _componentMain.c for the " << compName <<
                  " component.\n"
                  " */\n"
                  "\n"
                  "#include \"legato.h\"\n"
                  "#include \"../liblegato/eventLoop.h\"\n"
                  "\n";

    // For each of the component's client-side interfaces,
    for (auto interfacePtr : componentPtr->clientApis)
    {
        // Declare the client-side interface initialization function.
        fileStream << "void " << interfacePtr->internalName
                   << "_SetBinding(le_msg_LocalService_t* servicePtr);\n";
        fileStream << "void " << interfacePtr->internalName << "_ConnectService(void);\n";
    }

    // For each of the component's server-side interfaces,
    for (auto interfacePtr : componentPtr->serverApis)
    {
        // Declare the server-side interface initialization function.
        fileStream << "void " << interfacePtr->internalName
                   << "_InitService(le_msg_LocalService_t* servicePtr);\n";
        fileStream << "void " << interfacePtr->internalName
                   << "_SetServiceRef(le_msg_LocalService_t* servicePtr);\n";
        fileStream << "void " << interfacePtr->internalName << "_AdvertiseService(void);\n";
    }

    fileStream << "\n"
                  "// Component instance initialization function (COMPONENT_INIT).\n"
                  "void " << componentInitFuncName << "(void);\n"
                  "// One-time component initalization function (COMPONENT_INIT_ONCE).\n"
                  "__attribute__((weak))\n"
                  "void " << componentInitFuncName << "_ONCE(void)\n"
                  "{\n"
                  "}\n"
                  "\n"
                  "// Has one-time init been performed for this component yet?\n"
                  "static bool ComponentOnceInit = false;\n"
                  "\n"
                  "// Server-side service initialization function.\n";
    fileStream << "LE_SHARED ";
    GenerateEarlyInitPrototype(fileStream, componentPtr);
    fileStream << "\n"
                  "{\n";
    for (auto ifPtr : componentPtr->serverApis)
    {
        // For all services initialize
        fileStream << "    " << ifPtr->internalName << "_InitService("
                   << ifPtr->internalName << "Ptr);\n";

    }


    fileStream << "\n"
                  "    // Perform one-time initialization\n"
                  "    if (!ComponentOnceInit)\n"
                  "    {\n"
                  "        " << componentInitFuncName << "_ONCE();\n"
                  "        ComponentOnceInit = true;\n"
                  "    }\n"
                  "}\n"
                  "\n";

    fileStream << "LE_SHARED ";
    GenerateComponentInitPrototype(fileStream, componentPtr);
    fileStream << "\n"
                  "{\n"
                  "    LE_DEBUG(\"Initializing " << compName << " component library.\");\n"
                  "\n";

    /**
     * Queue the component init function to be safe
     */
    fileStream << "    // Queue the COMPONENT_INIT function to be called by the event loop\n"
                  "    // Do it here, because in RTOS, as soon as AdvertiseService is invoked\n"
                  "    // clients can start queueing messages. That can lead to a race\n"
                  "    // condition where a client's IPC message is processed before COMPONENT_INIT\n"
                  "    // had a chance to run\n"
                  "    event_QueueComponentInit(" << componentInitFuncName << ");\n\n";


    // Queue the initialization function to the event loop.
    // Call each of the component's server-side interfaces' initialization functions,
    // except those that are marked [manual-start].
    if (!componentPtr->serverApis.empty())
    {
        fileStream << "    // Advertise server-side IPC interfaces.\n";

        for (auto ifPtr : componentPtr->serverApis)
        {
            // For all services, set up service reference
            fileStream << "    " << ifPtr->internalName << "_SetServiceRef("
                       << ifPtr->internalName << "Ptr);\n";

            // If not marked for manual start,
            if (!(ifPtr->manualStart))
            {
                // Call the interface initialization function.
                fileStream << "    " << ifPtr->internalName << "_AdvertiseService();\n";
            }
            else
            {
                fileStream << "    // '" << ifPtr->internalName << "' is [manual-start].\n";
            }
        }

        fileStream << "\n";
    }

    // Call each of the component's client-side interfaces' initialization functions,
    // except those that are marked [manual-start].
    if (!componentPtr->clientApis.empty())
    {
        fileStream << "    // Connect client-side IPC interfaces.\n";

        for (auto ifPtr : componentPtr->clientApis)
        {
            // For all services, set binding
            fileStream << "    " << ifPtr->internalName << "_SetBinding("
                       << ifPtr->internalName << "Ptr);\n";
            // If not marked for manual start,
            if (!(ifPtr->manualStart))
            {
                // Call the interface initialization function.
                fileStream << "    " << ifPtr->internalName << "_ConnectService();\n";
            }
            else
            {
                fileStream << "    // '" << ifPtr->internalName << "' is [manual-start].\n";
            }
        }

        fileStream << "\n";
    }

    fileStream << "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate an _main.c file for a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateRtosExeMain
(
    model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    if (exePtr->hasJavaCode)
    {
        exePtr->exeDefPtr->ThrowException("Java is not supported on RTOS targets");
    }

    if (exePtr->hasPythonCode)
    {
        exePtr->exeDefPtr->ThrowException("Python is not supported on RTOS targets");
    }

    if (!exePtr->hasCOrCppCode)
    {
        // Nothing to do
        return;
    }

    auto& exeName = exePtr->name;

    // Set target info for executable
    exePtr->SetTargetInfo<target::RtosExeInfo_t>(new target::RtosExeInfo_t);

    // Compute the name of the init function.
    std::string exeFullName;
    if (exePtr->appPtr)
    {
        exeFullName = "_" + exePtr->appPtr->name + "_" + exePtr->name;
    }
    else
    {
        exeFullName = "_" + exePtr->name;
    }
    std::string initFuncName = "_" + exePtr->name + "_COMPONENT_INIT";
    std::string mainFuncName = exeFullName + "_Main";
    std::string serviceInitFuncName =
        exeFullName + "InitEarly";

    exePtr->GetTargetInfo<target::RtosExeInfo_t>()->entryPoint = mainFuncName;
    exePtr->GetTargetInfo<target::RtosExeInfo_t>()->initFunc = serviceInitFuncName;

    auto sourceFile = exePtr->MainObjectFile().sourceFilePath;

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Generating startup code for task '%s' (%s) "
                                        "in '%s'."),
                                exeName, exePtr->path, sourceFile)
                  << std::endl;
    }

    // Open the file as an output stream.
    file::MakeDir(path::GetContainingDir(sourceFile));
    std::ofstream outputFile(sourceFile);
    if (outputFile.is_open() == false)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Could not open '%s' for writing."), sourceFile)
        );
    }

    // Generate common prefix for executable main source.
    outputFile << "// Startup code for the executable '" << exeName << "'.\n"
                  "// This is a generated file, do not edit.\n"
                  "\n"
                  "#include \"legato.h\"\n"
                  "#include \"../liblegato/eventLoop.h\"\n"
                  "#include \"../liblegato/thread.h\"\n"
                  "#include \"../liblegato/cdata.h\"\n"
                  "#include \"../daemons/rtos/microSupervisor/microSupervisor.h\"\n"
                  "\n";

    // Declaration of all services required by this executable
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        for (auto clientApiInstancePtr : componentInstancePtr->clientApis)
        {
            if (clientApiInstancePtr->systemExtern)
            {
                // If service is exported, declare extern for exported service.
                outputFile << "extern le_msg_LocalService_t "
                           << ConvertInterfaceNameToSymbol(clientApiInstancePtr->
                                                           name)
                           << ";\n";
            }
            else if (clientApiInstancePtr->bindingPtr)
            {
                // If service is bound, declare extern for bound service.
                outputFile << "extern le_msg_LocalService_t "
                           << ConvertInterfaceNameToSymbol(clientApiInstancePtr->
                                                           bindingPtr->
                                                           serverIfName)
                           << ";\n";
            }
        }
    }
    // Declaration of all services provided by this executable
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        for (auto serverApiInstancePtr : componentInstancePtr->serverApis)
        {
            outputFile << "LE_SHARED le_msg_LocalService_t "
                       << ConvertInterfaceNameToSymbol(serverApiInstancePtr->name)
                       << ";\n";
        }
    }
    outputFile << "\n";


    // Forward declaration for all component init functions.
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        auto componentPtr = componentInstancePtr->componentPtr;
        if (componentPtr->HasCOrCppCode())
        {
            GenerateComponentInitPrototype(outputFile, componentPtr);
            outputFile << ";\n";
        }
    }
    outputFile << "\n";

    // Generate forward declaration of the default component's COMPONENT_INIT function.
    // If there are C/C++ source files other than the _main.c file,
    if ((!exePtr->cObjectFiles.empty()) || (!exePtr->cxxObjectFiles.empty()))
    {
        outputFile << "// Declare default component's COMPONENT_INIT function.\n"
                      "void " << initFuncName << "(void);\n"
                      "\n"
                      "\n";
    }

    outputFile << "static const cdata_MapEntry_t componentDataMap[] =\n"
                  "{\n";

    // Declare component instance ID of all shared components
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        auto componentPtr = componentInstancePtr->componentPtr;
        // Component instance IDs are only required for Legato C/C++ components
        if (componentPtr->HasCOrCppCode())
        {
            auto componentTargetInfoPtr =
                componentPtr->GetTargetInfo<target::RtosComponentInfo_t>();

            if (componentTargetInfoPtr->globalUsage > 0)
            {
                outputFile << "    { "
                           << componentTargetInfoPtr->componentKey << ", "
                           << componentInstancePtr->
                                  GetTargetInfo<target::RtosComponentInstanceInfo_t>()->
                                  instanceNum
                           << " },\n";
            }
        }
    }
    outputFile << "    { -1, -1 }\n"
                  "};\n"
                  "\n";

    // Prototypes for all components that need to be initialized
    for (auto componentInstPtr : exePtr->componentInstances)
    {
        if (componentInstPtr->componentPtr->HasCOrCppCode())
        {
            GenerateEarlyInitPrototype(outputFile, componentInstPtr->componentPtr);
            outputFile << ";\n";
        }
    }

    // Define function to initalize services
    outputFile <<
        "\n"
        "LE_SHARED void " << serviceInitFuncName <<
        "(\n"
        "    void\n"
        ")\n"
        "{\n";
    for (auto componentInstPtr : exePtr->componentInstances)
    {
        // No C/C++ code means no services on RTOS.
        if (!componentInstPtr->componentPtr->HasCOrCppCode())
        {
            continue;
        }

        int argCount = 0;
        outputFile
            << "    _" << componentInstPtr->componentPtr->name << "_InitEarly(";
        for (auto apiPtr : componentInstPtr->serverApis)
        {
            if (argCount++ != 0)
            {
                outputFile << ", ";
            }
            outputFile << "&" << ConvertInterfaceNameToSymbol(apiPtr->name);
        }
        outputFile << ");\n";
    }

    outputFile <<
        "}\n"
        "\n";

    // Define main task function.
    outputFile << "LE_SHARED void* " << mainFuncName << "(void* args)\n"
                  "{\n"
                  "    TaskInfo_t* taskInfo = args;\n"
                  "\n"

    // Set component instance map
                  "    thread_SetCDataInstancePtr(componentDataMap);\n"
    // Set arguments
                  "    LE_DEBUG(\"Starting " << mainFuncName << ".  taskInfo=%p with %d arguments\",\n"
                  "             taskInfo, taskInfo->argc);\n"
                  "    le_arg_SetArgs(taskInfo->argc, taskInfo->argv);\n"
                  "\n";

    // Set bindings and initialize included components for C/C++ components
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        int argCount = 0;
        auto componentPtr = componentInstancePtr->componentPtr;

        if (!componentPtr->HasCOrCppCode())
        {
            continue;
        }

        outputFile << "    _" << componentPtr->name  << "_Init(";
        for (auto serverApiInstancePtr : componentInstancePtr->serverApis)
        {
            if (argCount++ != 0)
            {
                outputFile << ", ";
            }
            outputFile << "&"
                       << ConvertInterfaceNameToSymbol(serverApiInstancePtr->
                                                       name);
        }
        for (auto clientApiInstancePtr : componentInstancePtr->clientApis)
        {
            if (argCount++ != 0)
            {
                outputFile << ", ";
            }

            if (clientApiInstancePtr->systemExtern)
            {
                // Binding is exported externally -- bind to rpcProxy
                outputFile << "&"
                           << ConvertInterfaceNameToSymbol(clientApiInstancePtr->name);
            }
            else if (clientApiInstancePtr->bindingPtr)
            {
                // A binding exists for this client API -- pass to component init
                outputFile << "&"
                           << ConvertInterfaceNameToSymbol(clientApiInstancePtr->
                                                           bindingPtr->
                                                           serverIfName);
            }
            else
            {
                // No binding exists for this API -- pass NULL as binding.
                outputFile << "NULL";
            }
        }
        outputFile << ");\n";
    }

    // If there is C/C++ source in this task
    if ((!exePtr->cObjectFiles.empty()) || (!exePtr->cxxObjectFiles.empty()))
    {
        outputFile << "// Queue the default component's COMPONENT_INIT to Event Loop.\n"
                      "    event_QueueComponentInit(" << initFuncName << ");\n";
    }

    // Start the event loop
    outputFile << "    LE_DEBUG(\"== Starting Event Processing Loop ==\");\n"
                  "    le_event_RunLoop();\n"
                  "    LE_FATAL(\"== SHOULDN'T GET HERE! ==\");\n"
                  "    return NULL;\n"
                  "}\n";

    outputFile.close();
}


} // namespace code
