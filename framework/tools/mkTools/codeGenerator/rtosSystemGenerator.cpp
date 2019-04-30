//--------------------------------------------------------------------------------------------------
/**
 * @file rtosSystemGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace code
{


//--------------------------------------------------------------------------------------------------
/**
 * Writing a priority to script writes the priority's LE_PRIORITY_... constant.
 */
//--------------------------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& out, const model::Priority_t& priority)
{
    int numericalPriority = model::Priority_t::MEDIUM;

    if (priority.IsSet())
    {
        numericalPriority = priority.GetNumericalValue();
    }

    switch (numericalPriority)
    {
        case model::Priority_t::IDLE:
            out << "LE_THREAD_PRIORITY_IDLE";
            break;
        case model::Priority_t::LOW:
            out << "LE_THREAD_PRIORITY_LOW";
            break;
        case model::Priority_t::MEDIUM:
            out << "LE_THREAD_PRIORITY_MEDIUM";
            break;
        case model::Priority_t::HIGH:
            out << "LE_THREAD_PRIORITY_HIGH";
            break;
        default:
            // This can generate an invalid priority if Priority_t::numericalValue is out of range.
            // But that should never happen.
            out << "LE_THREAD_PRIORITY_RT_" << numericalPriority;
            break;
    }

    return out;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add the number of times a component is called in a process to the component's global
 * usage count.
 */
//--------------------------------------------------------------------------------------------------
static void CountExeComponentUsage
(
    model::Exe_t* exePtr
)
{
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        // Skip components with no C or C++ code; usage count is for component-specific data,
        // which non-C/C++ components don't have.
        if (!componentInstancePtr->componentPtr->HasCOrCppCode())
        {
            continue;
        }

        // Set the instance number of this instance, and increment the total
        // number of times this component is used.
        componentInstancePtr->SetTargetInfo<target::RtosComponentInstanceInfo_t>(
            new target::RtosComponentInstanceInfo_t(
                componentInstancePtr->componentPtr->
                GetTargetInfo<target::RtosComponentInfo_t>()->globalUsage++)
        );
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Count how many times each component is used in all processes.
 *
 * On RTOS we need to know how many copies of each component's per-instance data are needed.
 * So count how many instances of each component there are across all processes on the system.
 */
//--------------------------------------------------------------------------------------------------
void CountSystemComponentUsage
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
{
    // Go through each process in the system, adding how many times each component is referenced.
    for (auto& appItem : systemPtr->apps)
    {
        auto appPtr = appItem.second;

        for (auto processEnvPtr : appPtr->processEnvs)
        {
            for (auto processPtr : processEnvPtr->processes)
            {
                auto exeIter = appPtr->executables.find(model::Exe_t::NameFromPath(processPtr->exePath));
                if (exeIter != appPtr->executables.end())
                {
                    auto exePtr = exeIter->second;

                    CountExeComponentUsage(exePtr);
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a tasks.c for tasks in a given system.
 */
//--------------------------------------------------------------------------------------------------
void GenerateRtosSystemTasks
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
{
    auto sourceFile = path::Combine(buildParams.workingDir, "src/tasks.c");

    // Open the file as an output stream.
    file::MakeDir(path::GetContainingDir(sourceFile));
    std::ofstream outputFile(sourceFile);
    if (outputFile.is_open() == false)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Could not open '%s' for writing."), sourceFile)
        );
    }

    // Generate the file header comment and #include directives.
    outputFile << "\n"
                  "// Tasks for system '" << systemPtr->name << "'.\n"
                  "// This is a generated file, do not edit.\n"
                  "\n"
                  "#include \"legato.h\"\n"
                  "#include \"microSupervisor.h\"\n"
                  "\n"
                  "\n";

    // Generate forward declarations for all entry points.
    for (auto& appItem : systemPtr->apps)
    {
        auto appPtr = appItem.second;

        for (auto& exeItem : appPtr->executables)
        {
            auto exePtr = exeItem.second;

            outputFile <<
                "extern void* " << exePtr->GetTargetInfo<target::RtosExeInfo_t>()->entryPoint <<
                    "(void* args);\n";
        }
    }
    outputFile << "\n";

    // Generate task lists
    for (auto& appItem : systemPtr->apps)
    {
        auto appPtr = appItem.second;

        std::string appTasksName = appPtr->name + "Tasks";

        outputFile <<
            "////////////////////////////////////////////////////////////////\n"
            "// Tasks for app '" << appPtr->name << "'\n";
        for (auto processEnvPtr : appPtr->processEnvs)
        {
            for (auto processPtr : processEnvPtr->processes)
            {
                std::string taskName = appPtr->name + "_" + processPtr->GetName();
                std::string argListName =
                    "_" + taskName + "_Args";
                outputFile <<
                    "#if LE_CONFIG_STATIC_THREAD_STACKS\n"
                    "// Stack for process " << processPtr->GetName() << "\n"
                    "LE_THREAD_DEFINE_STATIC_STACK(" << taskName << ", ";
                if (processEnvPtr->maxStackBytes.IsSet())
                {
                    outputFile << processEnvPtr->maxStackBytes.Get();
                }
                else
                {
                    // Zero forces a minimum stack size
                    outputFile << "0";
                }
                outputFile  << ");\n"
                            << "#endif /* end LE_CONFIG_STATIC_THREAD_STACKS */\n\n";

                outputFile <<
                    "// Arguments for process "
                           << processPtr->GetName()
                           << "\n"
                    "static const char* " << argListName << "[] =\n"
                    "{\n";
                for (auto arg : processPtr->commandLineArgs)
                {
                    outputFile <<
                        "    \"" << arg << "\",\n";
                }

                outputFile <<
                    "    NULL\n"
                    "};\n";
            }
        }
        outputFile <<
            "// Task list for all processes in app\n"
            "static Task_t " << appTasksName << "[] =\n"
            "{\n";
        for (auto processEnvPtr : appPtr->processEnvs)
        {
            for (auto processPtr : processEnvPtr->processes)
            {
                auto taskName = appPtr->name + std::string("_") + processPtr->GetName();
                auto exePtr = appPtr->executables[model::Exe_t::NameFromPath(processPtr->exePath)];

                outputFile <<
                    "    {\n"
                    "        .nameStr = \""
                           << processPtr->GetName()
                           << "\",\n"
                    "        .priority = " << processEnvPtr->GetStartPriority() << ",\n"
                    "#if LE_CONFIG_STATIC_THREAD_STACKS\n"
                    "        .stackSize = sizeof(_thread_stack_" << taskName << "),\n"
                    "        .stackPtr = _thread_stack_" << taskName << ",\n"
                    "#else /* !LE_CONFIG_STATIC_THREAD_STACKS */\n"
                    "        .stackSize = " << (processEnvPtr->maxStackBytes.IsSet() ?
                        processEnvPtr->maxStackBytes.Get() : 0) << ",\n"
                    "        .stackPtr = NULL,\n"
                    "#endif /* end !LE_CONFIG_STATIC_THREAD_STACKS */\n"
                    "        .entryPoint = "
                           << exePtr->GetTargetInfo<target::RtosExeInfo_t>()->entryPoint
                           << ",\n"
                    "        .defaultArgc = "
                           << processPtr->commandLineArgs.size()
                           << ",\n"
                    "        .defaultArgv = " << "_" << taskName << "_Args\n"
                    "    },\n";
            }
        }

        outputFile <<
            "};\n"
            "\n"
            "// ThreadInfo list for app '" << appPtr->name << "'\n"
            "static TaskInfo_t " << appPtr->name
                   << "TaskInfo[" << appPtr->executables.size() << "];\n";
    }

    // Generate app list
    outputFile <<
        "// App list for system '" << systemPtr->name << "'\n"
        "static const App_t SystemApps[] =\n"
        "{\n";
    for (auto& appItem : systemPtr->apps)
    {
        auto appPtr = appItem.second;

        outputFile <<
            "    {\n"
            "        .appNameStr = \"" << appPtr->name << "\",\n"
            "        .manualStart = " << ((appPtr->startTrigger == model::App_t::MANUAL)?
                                          "true":"false") << ",\n"
            "        .taskCount = " << appPtr->executables.size() << ",\n"
            "        .taskList = " << appPtr->name << "Tasks,\n"
            "        .threadList = " << appPtr->name << "TaskInfo,\n"
            "    },\n";
    }

    // Output final NULL.
    // microSupervisor uses this to know how when it hits the end of the app list.
    outputFile <<
        "    {\n"
        "        .appNameStr = NULL\n"
        "    }\n"
        "};\n"
        "\n"
        "const App_t *_le_supervisor_GetSystemApps\n"
        "(\n"
        "    void\n"
        ")\n"
        "{\n"
        "    return SystemApps;\n"
        "}\n"
        "\n";

    outputFile << "// CLI command list, if any\n";
    for (auto cmdItem : systemPtr->commands)
    {
        const auto  commandPtr = cmdItem.second;
        std::string component = commandPtr->exePath.substr(1);
        std::string description = "Legato '" + commandPtr->name + "' command";

        outputFile  << "LE_RTOS_CLI_DEFINECMD(" << commandPtr->appPtr->name << ", " << component
                    << ", \"" << commandPtr->name  << "\",\n    \"" << description << "\");\n";
    }
    outputFile  << "\n";

    for (auto appItem : systemPtr->apps)
    {
        auto appPtr = appItem.second;
        for (auto exeItem : appPtr->executables)
        {
            auto exePtr = exeItem.second;
            outputFile <<
                "void " << exePtr->GetTargetInfo<target::RtosExeInfo_t>()->initFunc << "(void);\n";
        }
    }

    outputFile <<
        "\n"
        "/**\n"
        " * Initialize all services in system.\n"
        " */\n"
        "void _le_supervisor_InitAllServices\n"
        "(\n"
        "    void\n"
        ")\n"
        "{\n";
    for (auto appItem : systemPtr->apps)
    {
        auto appPtr = appItem.second;
        for (auto exeItem : appPtr->executables)
        {
            auto exePtr = exeItem.second;
            outputFile
                << "    " << exePtr->GetTargetInfo<target::RtosExeInfo_t>()->initFunc << "();\n";
        }
    }
    outputFile << "\n    // Any CLI command registration will follow\n";

    // Create CLI commands for all the the shell commands specified in the .sdef file's "commands:"
    // section.
    for (auto cmdItem : systemPtr->commands)
    {
        const auto  commandPtr = cmdItem.second;
        std::string component = commandPtr->exePath.substr(1);

        outputFile  << "    le_rtos_cli_RegisterCommand(&LE_RTOS_CLI_CMD("
                    << commandPtr->appPtr->name << ", " << component << "));\n";
    }

    outputFile <<
        "}\n";
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate a rpcServices.c for rpc services and links in a given system.
 */
//--------------------------------------------------------------------------------------------------
void GenerateRtosRpcServices
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
{
    auto sourceFile = path::Combine(buildParams.workingDir, "src/rpcServices.c");

    // Open the file as an output stream.
    file::MakeDir(path::GetContainingDir(sourceFile));
    std::ofstream outputFile(sourceFile);
    if (outputFile.is_open() == false)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Could not open '%s' for writing."), sourceFile)
        );
    }

    std::set<std::string> includedHeaders;

    // Generate the file header comment and #include directives.
    outputFile << "\n"
                  "// RPC data for system '" << systemPtr->name << "'.\n"
                  "// This is a generated file, do not edit.\n"
                  "\n"
                  "#include \"legato.h\"\n"
                  "#include \"le_rpcProxy.h\"\n"
                  "\n";
    for (auto &serverApiEntry : systemPtr->externServerInterfaces)
    {
        std::string headerName = serverApiEntry.second->ifPtr->apiFilePtr->defaultPrefix + "_common.h";
        if (includedHeaders.find(headerName) == includedHeaders.end())
        {
            outputFile << "#include \"" << headerName << "\"\n";
            includedHeaders.insert(headerName);
        }
    }
    for (auto &clientApiEntry : systemPtr->externClientInterfaces)
    {
        std::string headerName = clientApiEntry.second->ifPtr->apiFilePtr->defaultPrefix + "_common.h";
        if (includedHeaders.find(headerName) == includedHeaders.end())
        {
            outputFile << "#include \"" << headerName << "\"\n";
            includedHeaders.insert(headerName);
        }
    }
    outputFile << "\n";

    // Declaration of all services provided by RPC.
    // Note: if the system is a client of the service, the RPC proxy will be a server
    // for the system, and visa versa.
    for (auto &clientApiEntry : systemPtr->externClientInterfaces)
    {
        outputFile << "le_msg_LocalService_t "
                   << ConvertInterfaceNameToSymbol(clientApiEntry.second->name)
                   << ";\n";
    }
    // Forward-declaration of all services required by RPC
    for (auto &serverApiEntry : systemPtr->externServerInterfaces)
    {
        outputFile << "extern le_msg_LocalService_t "
                   << ConvertInterfaceNameToSymbol(serverApiEntry.second->name)
                   << ";\n";
    }

    outputFile << "\n"
        "//--------------------------------------------------------------------------------------\n"
        "/**\n"
        " * Argument lists for communication links.\n"
        " */\n"
        "//--------------------------------------------------------------------------------------\n"
        ;
    for (auto &linkEntry : systemPtr->links)
    {
        outputFile <<
            "static const char *" << linkEntry.first << "ArgV[] =\n"
            "{\n";
        for (auto &arg : linkEntry.second->args)
        {
            outputFile << "    \"" << arg << "\",\n";
        }
        outputFile <<
            "    NULL\n"
            "};\n";
    }


    outputFile << "\n"
        "//--------------------------------------------------------------------------------------\n"
        "/**\n"
        " * All communication links available to this system.\n"
        " */\n"
        "//--------------------------------------------------------------------------------------\n"
        "const rpcProxy_SystemLinkElement_t rpcProxy_SystemLinkArray[] =\n"
        "{\n";
    for (auto &linkEntry : systemPtr->links)
    {
        outputFile <<
            "    {\n"
            "        .systemName = \"" << linkEntry.first << "\",\n"
            "        .argc = " << linkEntry.second->args.size() << ",\n"
            "        .argv = " << linkEntry.first << "ArgV\n"
            "    },\n";
    }
    outputFile <<
        "    { .systemName = NULL }\n"
        "};\n"
        "\n"
        "//--------------------------------------------------------------------------------------\n"
        "/**\n"
        " * Each local service required by this system, including message pools\n"
        " */\n"
        "//--------------------------------------------------------------------------------------\n";
    for (auto &externClientEntry : systemPtr->externClientInterfaces)
    {
        std::string defaultCapsPrefix;
        std::transform(externClientEntry.second->ifPtr->apiFilePtr->defaultPrefix.begin(),
                       externClientEntry.second->ifPtr->apiFilePtr->defaultPrefix.end(),
                       std::back_inserter(defaultCapsPrefix),
                       ::toupper);

        outputFile <<
            "//----------------------------------------------------------------------------------\n"
            "/**\n"
            " * Prototype for " << externClientEntry.first << " RPC services.\n"
            " */\n"
            "//----------------------------------------------------------------------------------\n"
            "static le_msg_ServiceRef_t rpcProxy_Init" << externClientEntry.first << "Service\n"
            "(\n"
            "    void\n"
            ");\n"
            "\n"
            "/**\n"
            " * Local service reference for " << externClientEntry.first << "\n"
            " */\n"
            "static const rpcProxy_ExternLocalServer_t"
            " rpcProxy_" << externClientEntry.first << "Server = \n"
            "{\n"
            "    .common = {\n"
            "        .serviceName = \"" << externClientEntry.first << "\",\n"
            "        .protocolIdStr = IFGEN_" << defaultCapsPrefix << "_PROTOCOL_ID,\n"
            "        .messageSize = IFGEN_" << defaultCapsPrefix << "_MSG_SIZE\n"
            "    },\n"
            "    .initLocalServicePtr = &rpcProxy_Init" << externClientEntry.first << "Service\n"
            "};\n"
            "\n"
            "LE_MEM_DEFINE_STATIC_POOL(" << externClientEntry.first << "Messages, 1,"
            " IFGEN_" << defaultCapsPrefix << "_LOCAL_MSG_SIZE);\n"
            "\n";
    }
    outputFile <<
        "//--------------------------------------------------------------------------------------\n"
        "/**\n"
        " * All services which should be exposed over RPC by this system.\n"
        " */\n"
        "//--------------------------------------------------------------------------------------\n"
        "const rpcProxy_ExternServer_t *rpcProxy_ServerReferenceArray[] =\n"
        "{\n";
    for (auto &externClientEntry : systemPtr->externClientInterfaces)
    {
        outputFile <<
            "    &rpcProxy_" << externClientEntry.first << "Server.common,\n";
    }

    outputFile <<
        "    NULL\n"
        "};\n"
        "\n"
        "//--------------------------------------------------------------------------------------\n"
        "/**\n"
        " * Each local service required by this system.\n"
        " */\n"
        "//--------------------------------------------------------------------------------------\n";
    for (auto &externServerEntry : systemPtr->externServerInterfaces)
    {
        std::string defaultCapsPrefix;
        std::transform(externServerEntry.second->ifPtr->apiFilePtr->defaultPrefix.begin(),
                       externServerEntry.second->ifPtr->apiFilePtr->defaultPrefix.end(),
                       std::back_inserter(defaultCapsPrefix),
                       ::toupper);

        outputFile <<
            "static const rpcProxy_ExternLocalClient_t"
            " rpcProxy_" << externServerEntry.first << "Client =\n"
            "{\n"
            "     .common = {\n"
            "         .serviceName = \"" << externServerEntry.first << "\",\n"
            "         .protocolIdStr = IFGEN_" << defaultCapsPrefix << "_PROTOCOL_ID,\n"
            "        .messageSize = IFGEN_" << defaultCapsPrefix << "_MSG_SIZE\n"
            "     },\n"
            "     .localServicePtr = &"
                   << ConvertInterfaceNameToSymbol(externServerEntry.second->name) << "\n"
            "};\n"
            "\n";
    }
    outputFile <<
        "//--------------------------------------------------------------------------------------\n"
        "/**\n"
        " * All clients which are required over RPC by this system.\n"
        " */\n"
        "//--------------------------------------------------------------------------------------\n"
        "const rpcProxy_ExternClient_t *rpcProxy_ClientReferenceArray[] =\n"
        "{\n";
    for (auto &externServerEntry : systemPtr->externServerInterfaces)
    {
        outputFile <<
            "    &rpcProxy_" << externServerEntry.first << "Client.common,\n";
    }
    outputFile <<
        "    NULL\n"
        "};\n"
        "\n";
    for (auto &externClientEntry : systemPtr->externClientInterfaces)
    {
        outputFile <<
            "//----------------------------------------------------------------------------------\n"
            "/**\n"
            " * Initialize service for " << externClientEntry.first << " RPC services.\n"
            " */\n"
            "//----------------------------------------------------------------------------------\n"
            "static le_msg_ServiceRef_t rpcProxy_Init" << externClientEntry.first << "Service\n"
            "(\n"
            "    void\n"
            ")\n"
            "{\n";

        std::string defaultCapsPrefix;
        std::transform(externClientEntry.second->ifPtr->apiFilePtr->defaultPrefix.begin(),
                       externClientEntry.second->ifPtr->apiFilePtr->defaultPrefix.end(),
                       std::back_inserter(defaultCapsPrefix),
                       ::toupper);

        outputFile <<
            "    le_mem_PoolRef_t serverMsgPoolRef = \n"
            "        le_mem_InitStaticPool(" << externClientEntry.first << "Messages, 1, IFGEN_"
                   << defaultCapsPrefix << "_LOCAL_MSG_SIZE);\n"
            "\n"
            "    return le_msg_InitLocalService(&"
                   << ConvertInterfaceNameToSymbol(externClientEntry.second->name)
                   << ", \"" << externClientEntry.first << "\""
                   << ", serverMsgPoolRef);\n"
        "}\n";
    }
}


}
