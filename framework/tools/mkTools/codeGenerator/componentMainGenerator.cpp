//--------------------------------------------------------------------------------------------------
/**
 * @file componentMainGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

//--------------------------------------------------------------------------------------------------
/**
 * Define the service name variables for an IPC interface.
 */
//--------------------------------------------------------------------------------------------------
static void DefineServiceNameVars
(
    std::ofstream& fileStream,      ///< File stream to write to.
    const model::ApiRef_t* interfacePtr,  ///< Ptr to client or server interface.
    bool isStandAlone   ///< true = fully resolve all interface name variables.
)
//--------------------------------------------------------------------------------------------------
{
    std::string ifLevelVar = interfacePtr->internalName + "_ServiceInstanceNamePtr";

    // If the component is being built for use by an executable built by mkexe, mkapp, or mksys,
    // then create an extern variable declaration that will be satisfied by the generated
    // _main.c for the executable, thereby allowing exe-specific interface instance naming.
    if (!isStandAlone)
    {
        std::string exeLevelVar = "_" + interfacePtr->componentPtr->name
                                + "_" + interfacePtr->internalName
                                + "_ServiceInstanceName";

        fileStream << "extern const char* " << exeLevelVar << ";\n";

        fileStream << "const char** " << ifLevelVar << " = &" << exeLevelVar << ";\n";
    }
    // If the component is being built for stand-alone use, then fully resolve the interface
    // instance names, using the internal name as the name to send to the Service Directory.
    else
    {
        std::string constName = interfacePtr->internalName + "_InterfaceName";
        fileStream << "static const char* " << constName << " = \""
                                                    << interfacePtr->internalName << "\";\n";
        fileStream << "const char** " << ifLevelVar << " = &" << constName << ";\n";
    }
}


namespace code
{


//--------------------------------------------------------------------------------------------------
/**
 * Generate _componentMain.c for a given component.
 *
 * This resolves the undefined service name symbols in all the interfaces' .o files
 * and creates a component-specific interface initialization function.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateCLangComponentMainFile
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto& compName = componentPtr->name;

    // Compute the path to the output file.
    std::string outputDir = path::Minimize(buildParams.workingDir
                                        + '/'
                                        + componentPtr->workingDir
                                        + "/src");
    std::string filePath = outputDir + "/_componentMain.c";

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Generating component-specific IPC code for"
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

    // Generate file header and #include directives.
    fileStream << "/*\n"
                  " * AUTO-GENERATED _componentMain.c for the " << compName <<
                  " component.\n"
                  "\n"
                  " * Don't bother hand-editing this file.\n"
                  " */\n"
                  "\n"
                  "#include \"legato.h\"\n"
                  "#include \"../liblegato/eventLoop.h\"\n"
                  "#include \"../liblegato/linux/logPlatform.h\"\n"
                  "#include \"../liblegato/log.h\"\n"
                  "\n"
                  "#ifdef __cplusplus\n"
                  "extern \"C\" {\n"
                  "#endif\n"
                  "\n";

    // For each of the component's client-side interfaces,
    for (auto interfacePtr : componentPtr->clientApis)
    {
        DefineServiceNameVars(fileStream, interfacePtr, componentPtr->isStandAloneComp);

        // Declare the client-side interface initialization function.
        fileStream << "void " << interfacePtr->internalName << "_ConnectService(void);\n";
    }

    // For each of the component's server-side interfaces,
    for (auto interfacePtr : componentPtr->serverApis)
    {
        DefineServiceNameVars(fileStream, interfacePtr, componentPtr->isStandAloneComp);

        // Declare the server-side interface initialization function.
        fileStream << "void " << interfacePtr->internalName << "_AdvertiseService(void);\n";
    }

    // Declare the component's log session variables.
    fileStream << "// Component log session variables.\n"
                  "le_log_SessionRef_t " << compName << "_LogSession;\n"
                  "le_log_Level_t* " << compName << "_LogLevelFilterPtr;\n"
                  "\n"

    // Generate forward declaration of the COMPONENT_INIT function.
                  "// Declare component's COMPONENT_INIT_ONCE function,\n"
                  "// and provide default empty implementation.\n"
                  "__attribute__((weak))\n"
                  "void " << componentPtr->initFuncName << "_ONCE(void)\n"
                  "{\n"
                  "}\n"
                  "// Component initialization function (COMPONENT_INIT).\n"
                  "void " << componentPtr->initFuncName << "(void);\n"
                  "\n"

    // Define the library initialization function to be run by the dynamic linker/loader.
                  "// Library initialization function.\n"
                  "// Will be called by the dynamic linker loader when the library is loaded.\n"
                  "__attribute__((constructor)) void _" << compName << "_Init(void)\n"
                  "{\n"
                  "    LE_DEBUG(\"Initializing " << compName << " component library.\");\n"
                  "\n";

    // Call each of the component's server-side interfaces' initialization functions,
    // except those that are marked [manual-start].
    if (!componentPtr->serverApis.empty())
    {
        fileStream << "    // Advertise server-side IPC interfaces.\n";

        for (auto ifPtr : componentPtr->serverApis)
        {
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

    // Register with the Log Daemon.
    fileStream << "    // Register the component with the Log Daemon.\n"
                  "    " << compName << "_LogSession = log_RegComponent(\"" <<
                  compName << "\", &" << compName << "_LogLevelFilterPtr);\n"

    // Queue the initialization function to the event loop.
                  "\n"
                  "// Queue the default component's COMPONENT_INIT_ONCE to Event Loop.\n"
                  "    event_QueueComponentInit(" << componentPtr->initFuncName << "_ONCE);\n"
                  "\n"
                  "    //Queue the COMPONENT_INIT function to be called by the event loop\n"
                  "    event_QueueComponentInit(" << componentPtr->initFuncName << ");\n"

    // Put the finishing touches on the file.
                  "}\n"
                  "\n"
                  "\n"
                  "#ifdef __cplusplus\n"
                  "}\n"
                  "#endif\n";
}


} // namespace code
