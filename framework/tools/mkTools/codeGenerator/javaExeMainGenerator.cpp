//--------------------------------------------------------------------------------------------------
/**
 * @file javaExeMainGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


namespace code
{


//--------------------------------------------------------------------------------------------------
/**
 * Generate the io.legato.generated.app.<AppName>.GeneratedMain class for a given Java executable.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateJavaExeMain
(
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Compute the path to the file to be generated.
    auto sourceFile = exePtr->MainObjectFile().sourceFilePath;

    // Open the file as an output stream.
    file::MakeDir(path::GetContainingDir(sourceFile));
    std::ofstream outputFile(sourceFile);
    if (outputFile.is_open() == false)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Could not open '%s' for writing."), sourceFile)
        );
    }

    auto& exeName = exePtr->name;
    auto& appName = exePtr->appPtr->name;

    outputFile << "\n"
                  "// Startup code for the executable '" << exeName << "'.\n"
                  "// This is a generated file, do not edit.\n"
                  "\n"
                  "package io.legato.generated.exe." << exeName << ";\n"
                  "\n"
                  "import java.util.logging.Level;\n"
                  "import java.util.logging.Logger;\n"
                  "import java.util.logging.LogRecord;\n"
                  "import java.util.logging.SimpleFormatter;\n"
                  "\n"
                  "import io.legato.Runtime;\n"
                  "import io.legato.LogHandler;\n"
                  "\n"
                  "public class Main\n"
                  "{\n"
                  "    public static void main(String[] args)\n"
                  "    {\n"
                  "        io.legato.Runtime.connectToLogControl();\n"
                  "        Logger logger = Logger.getLogger(\"" << appName << "\");\n"
                  "\n"
                  "        try\n"
                  "        {\n"
                  "            SimpleFormatter formatter = new SimpleFormatter();\n"
                  "            LogHandler handler = new LogHandler(\"" << appName << "\");\n"
                  "            handler.setFormatter(formatter);\n"
                  "\n"
                  "            logger.setUseParentHandlers(false);\n"
                  "            logger.addHandler(handler);\n"
                  "            logger.setLevel(Level.ALL);\n"
                  "\n";

    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        if (componentInstancePtr->componentPtr->HasJavaCode())
        {
            auto componentPtr = componentInstancePtr->componentPtr;
            auto& name = componentPtr->name;

            for (auto ifInstancePtr : componentInstancePtr->serverApis)
            {
                auto& internalName = ifInstancePtr->ifPtr->internalName;

                outputFile << "            io.legato.generated.component." << name << ".Factory."
                           << internalName << "ServiceInstanceName = \""
                           << ifInstancePtr->name
                           << "\";\n";
            }

            // For each of the component instance's client-side interfaces,
            for (auto ifInstancePtr : componentInstancePtr->clientApis)
            {
                auto& internalName = ifInstancePtr->ifPtr->internalName;

                outputFile << "            io.legato.generated.component." << name << ".Factory."
                           << internalName << "ServiceInstanceName = \""
                           << ifInstancePtr->name
                           << "\";\n";
            }

            outputFile << "            io.legato.generated.component."
                       << name << ".Factory.createComponent(logger);\n"
                          "\n";
        }
    }

    outputFile << "            io.legato.Runtime.runEventLoop();\n"
                  "        }\n"
                  "        catch (Exception exception)\n"
                  "        {\n"
                  "            logger.log(Level.SEVERE, \"A fatal error occurred during startup: "
                  "\" + exception.getMessage());\n"
                  "        }\n"
                  "    }\n"
                  "}\n";
}


}
