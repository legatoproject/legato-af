//--------------------------------------------------------------------------------------------------
/**
 * @file javaComponentMainGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


namespace code
{


//--------------------------------------------------------------------------------------------------
/**
 * Generate io.legato.generated.component.<componentName>.Factory class for a given component.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateJavaComponentMainFile
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
                                           + "/src/io/legato/generated/component/" + compName);
    std::string filePath = outputDir + "/Factory.java";

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Generating component-specific IPC code for component "
                                  "'%s' in '%s'."), compName, filePath)
                  << std::endl;
    }

    // Open the .java file for writing.
    file::MakeDir(outputDir);
    std::ofstream outputFile(filePath, std::ofstream::trunc);
    if (!outputFile.is_open())
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Failed to open file '%s' for writing."), filePath)
        );
    }

    std::string apiImports;
    std::string serverVars;
    std::string serverInits;
    std::string clientInits;

    std::string instanceNames;

    for (auto& serverApi : componentPtr->serverApis)
    {
        std::string className = serverApi->internalName + "Server";
        std::string varName = "instance" + serverApi->internalName;

        apiImports += "import io.legato.api.implementation." + serverApi->internalName +
                      "Server;\n";

        serverVars += "    public static " + className + " " + varName + ";\n";

        instanceNames += "    public static String " + serverApi->internalName + "ServiceInstanceName;\n";

        serverInits += "        " + varName + " = new " + className + "(component);\n";

        if (serverApi->manualStart == false)
        {
            serverInits += "        " + varName + ".open(" +
                                                serverApi->internalName + "ServiceInstanceName);\n";
        }
    }

    for (auto& clientApi : componentPtr->clientApis)
    {
        std::string className = clientApi->internalName + "Client";
        std::string varName = "instance" + clientApi->internalName;

        apiImports += "import io.legato.api.implementation." + className + ";\n" +
                "import io.legato.api." + clientApi->internalName + ";\n";
        clientInits += "        " + className + " " + varName + " = new " + className + "();\n";

        instanceNames += "    public static String " + clientApi->internalName + "ServiceInstanceName;\n";

        if (clientApi->manualStart == false)
        {
            clientInits += "        " + varName + ".open(" +
                                                clientApi->internalName + "ServiceInstanceName);\n";
        }

        clientInits += "        component.registerService(" + clientApi->internalName + ".class, " +
                       "instance" + clientApi->internalName + ");\n";
    }

    if (instanceNames.empty() == false)
    {
        instanceNames = "    // Our binding instance names.\n" +
                     instanceNames + "\n";
    }

    if (componentPtr->serverApis.size() > 0)
    {
        serverVars = "    // Our server instances.\n" +
                     serverVars + "\n";

        serverInits = "        // Init server interfaces.\n"
                      "        logger.log(Level.INFO, \"Initializing server APIs.\");\n" +
                      serverInits + "\n";
    }

    if (componentPtr->clientApis.size() > 0)
    {
        clientInits = "        // Init client interfaces.\n"
                      "        logger.log(Level.INFO, \"Initializing client APIs.\");\n" +
                      clientInits + "\n";
    }

    outputFile << "\n"
                  "// Startup code for the component, '" << compName << "'.\n"
                  "// This is a generated file, do not edit.\n"
                  "\n"
                  "package io.legato.generated.component." << compName << ";\n"
                  "\n"
                  "import java.util.logging.Logger;\n"
                  "import io.legato.Level;\n"
                  "\n"
                  "import io.legato.Runtime;\n"
                  "import io.legato.Component;\n"
               << apiImports
               << "\n"
                  "import " << componentPtr->javaPackages.front()->packageName
                            << "." << componentPtr->name << ";\n"
                  "\n"
                  "public class Factory\n"
                  "{\n"
               << instanceNames
               << serverVars
               << "    public static Component createComponent(Logger logger) throws Exception\n"
                  "    {\n"
                  "        // Construct component.\n"
                  "        " << compName << " component = new " << compName << "();\n"
                  "        component.setLogger(logger);\n"
                  "\n"
               << serverInits
               << clientInits
               << "        // Schedule the component init to be called.\n"
                  "        logger.log(Level.INFO, \"Scheduling init for component "
                                                                            << compName << ".\");\n"
                  "        Runtime.scheduleComponentInit(component);\n"
                  "\n"
                  "        return component;\n"
                  "    }\n"
                  "}\n";
}


}
