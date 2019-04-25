//--------------------------------------------------------------------------------------------------
/**
 * @file exeMainGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace code
{


//--------------------------------------------------------------------------------------------------
/**
 * Generates a main .c for a given executable.
 */
//--------------------------------------------------------------------------------------------------
void GenerateCLangExeMain
(
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto& exeName = exePtr->name;

    // Compute the name of the executable's "default" component...
    std::string defaultCompName = exeName + "_exe";

    // Compute the name of the default component's COMPONENT_INIT function.
    std::string initFuncName = "_" + defaultCompName + "_COMPONENT_INIT";

    // Compute the path to the file to be generated.
    auto sourceFile = exePtr->MainObjectFile().sourceFilePath;

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Generating startup code for executable '%s' (%s) "
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

    // Generate the file header comment and #include directives.
    outputFile << "\n"
                  "// Startup code for the executable '" << exeName << "'.\n"
                  "// This is a generated file, do not edit.\n"
                  "\n"
                  "#include \"legato.h\"\n"
                  "#include \"../liblegato/eventLoop.h\"\n"
                  "#include \"../liblegato/linux/logPlatform.h\"\n"
                  "#include \"../liblegato/log.h\"\n"
                  "#include <dlfcn.h>\n"
                  "\n"
                  "\n";

    outputFile << "// Define IPC API interface names.\n";

    // Iterate over the list of Component Instances,
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        auto componentPtr = componentInstancePtr->componentPtr;

        const auto& compName = componentPtr->name;

        // For each of the component instance's server-side interfaces,
        for (auto ifInstancePtr : componentInstancePtr->serverApis)
        {
            auto& internalName = ifInstancePtr->ifPtr->internalName;

            // Define the interface name variable.
            outputFile << "LE_SHARED const char* _" << compName << "_" << internalName
                       << "_ServiceInstanceName = \"" << ifInstancePtr->name << "\";\n";
        }

        // For each of the component instance's client-side interfaces,
        for (auto ifInstancePtr : componentInstancePtr->clientApis)
        {
            auto& internalName = ifInstancePtr->ifPtr->internalName;

            // Define the interface name variable.
            outputFile << "LE_SHARED const char* _" << compName << "_" << internalName
                       << "_ServiceInstanceName = \"" << ifInstancePtr->name << "\";\n";

        }
    }

    // Define the default component's log session variables.
    outputFile << "\n"
                  "// Define default component's log session variables.\n"
                  "LE_SHARED le_log_SessionRef_t " << defaultCompName << "_LogSession;\n"
                  "LE_SHARED le_log_Level_t* " << defaultCompName << "_LogLevelFilterPtr;\n"
                  "\n";

    // Generate forward declaration of the default component's COMPONENT_INIT function.
    // If there are C/C++ source files other than the _main.c file,
    if ((!exePtr->cObjectFiles.empty()) || (!exePtr->cxxObjectFiles.empty()))
    {
        outputFile <<
            "// Declare default component's COMPONENT_INIT_ONCE function,\n"
            "// and provide default empty implementation.\n"
            "__attribute__((weak))\n"
            "void " << initFuncName << "_ONCE(void)\n"
            "{\n"
            "}\n"
            "\n"
            "\n"
            "// Declare default component's COMPONENT_INIT function.\n"
            "void " << initFuncName << "(void);\n"
            "\n"
            "\n";
    }

    // Define function that loads a shared library using dlopen().
    outputFile << "// Loads a library using dlopen().\n"
                  "__attribute__((unused)) static void LoadLib\n"
                  "(\n"
                  "    const char* libName\n"
                  ")\n"
                  "{\n"
                  "    dlopen(libName, RTLD_LAZY | RTLD_GLOBAL);\n"
                  "    const char* errorMsg = dlerror();\n"
                  "    LE_FATAL_IF(errorMsg != NULL,\n"
                  "                \"Failed to load library '%s' (%s)\","
                  "                libName,\n"
                  "                errorMsg);\n"
                  "}\n"
                  "\n"
                  "\n"

    // Define main().
                  "int main(int argc, const char* argv[])\n"
                  "{\n"
                  "    // Pass the args to the Command Line Arguments API.\n"
                  "    le_arg_SetArgs((size_t)argc, argv);\n"

    // Make stdout line buffered.
                  "    // Make stdout line buffered so printf shows up in logs without flushing.\n"
                  "    setlinebuf(stdout);\n"
                  "\n"

    // Register the component with the Log Control Daemon.
                  "    " << defaultCompName << "_LogSession = log_RegComponent(\"" <<
                  defaultCompName << "\", &" << defaultCompName << "_LogLevelFilterPtr);\n"
                  "\n"
                  "    // Connect to the log control daemon.\n"
                  "    // Note that there are some rare cases where we don't want the\n"
                  "    // process to try to connect to the Log Control Daemon (e.g.,\n"
                  "    // the Supervisor and the Service Directory shouldn't).\n"
                  "    // The NO_LOG_CONTROL macro can be used to control that.\n"
                  "    #ifndef NO_LOG_CONTROL\n"
                  "        log_ConnectToControlDaemon();\n"
                  "    #else\n"
                  "        LE_DEBUG(\"Not connecting to the Log Control Daemon.\");\n"
                  "    #endif\n"
                  "\n";

    // Iterate over the list of Component Instances, loading their dynamic libraries.
    outputFile << "    // Load dynamic libraries.\n";
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        auto componentPtr = componentInstancePtr->componentPtr;

        // If the component contains shared libraries, load them.
        for (const auto fileSystemObjectPtr : componentPtr->bundledFiles)
        {
            std::string filePath = path::GetLastNode(fileSystemObjectPtr->destPath);

            if (path::IsSharedLibrary(filePath))
            {
                outputFile << "    LoadLib(\"" << filePath << "\");\n";
            }
        }

        std::string componentLib = componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()->lib;

        if (!componentLib.empty())
        {
            outputFile << "    LoadLib(\"" << path::GetLastNode(componentLib) << "\");\n";
        }
    }

    outputFile << "\n";

    // If there are C/C++ source files other than the _main.c file,
    if ((!exePtr->cObjectFiles.empty()) || (!exePtr->cxxObjectFiles.empty()))
    {
        outputFile <<
            "// Queue the default component's COMPONENT_INIT_ONCE to Event Loop.\n"
            "    event_QueueComponentInit(" << initFuncName << "_ONCE);\n"
            "\n"
            "// Queue the default component's COMPONENT_INIT to Event Loop.\n"
            "    event_QueueComponentInit(" << initFuncName << ");\n";
    }

    outputFile << "    // Set the Signal Fault handler\n"
                  "    le_sig_InstallShowStackHandler();\n"
                  "\n";

    outputFile << "    // Set the Signal Term handler\n"
                  "    le_sig_InstallDefaultTermHandler();\n"
                  "\n";

    // Start the event loop and finish up the file.
    outputFile << "    LE_DEBUG(\"== Starting Event Processing Loop ==\");\n"
                  "    le_event_RunLoop();\n"
                  "    LE_FATAL(\"== SHOULDN'T GET HERE! ==\");\n"
                  "}\n";

    outputFile.close();
}



} // namespace code
