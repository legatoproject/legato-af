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
                auto exePtr = appPtr->executables[model::Exe_t::NameFromPath(processPtr->exePath)];

                CountExeComponentUsage(exePtr);
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
                std::string argListName =
                    "_" + appPtr->name +
                    "_" + processPtr->GetName() +
                    "_Args";
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
                auto exePtr = appPtr->executables[model::Exe_t::NameFromPath(processPtr->exePath)];

                outputFile <<
                    "    {\n"
                    "        .nameStr = \""
                           << processPtr->GetName()
                           << "\",\n"
                    "        .priority = " << processEnvPtr->GetStartPriority() << ",\n";
                if (processEnvPtr->maxStackBytes.IsSet())
                {
                    outputFile <<
                        "        .stackSize = " << processEnvPtr->maxStackBytes.Get() << ",\n";
                }
                outputFile <<
                    "        .entryPoint = "
                           << exePtr->GetTargetInfo<target::RtosExeInfo_t>()->entryPoint
                           << ",\n"
                    "        .defaultArgc = "
                           << processPtr->commandLineArgs.size()
                           << ",\n"
                    "        .defaultArgv = " << "_" << appPtr->name
                                              << "_" << processPtr->GetName()
                                              << "_Args\n"
                    "    },\n";
            }
        }

        outputFile <<
            "};\n"
            "\n"
            "// ThreadInfo list for app '" << appPtr->name << "'\n"
            "static TaskInfo_t* " << appPtr->name
                   << "TaskInfo[" << appPtr->executables.size() << "];\n";
    }

    // Generate app list
    outputFile <<
        "// App list for system '" << systemPtr->name << "'\n"
        "/* global */ const App_t _le_supervisor_SystemApps[] =\n"
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

}
