//--------------------------------------------------------------------------------------------------
/**
 * @file pythonExeMainGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace code
{


//--------------------------------------------------------------------------------------------------
/**
 * Generates a main .py for a given executable.
 */
//--------------------------------------------------------------------------------------------------
void GeneratePythonExeMain
(
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto mainObjectFile = exePtr->MainObjectFile();
    // Compute the path to the file to be generated.
    auto launcherFile = mainObjectFile.sourceFilePath;
    auto binDir = path::GetContainingDir(exePtr->path);

    file::MakeDir(path::GetContainingDir(launcherFile));

    // Open the file as an output stream.
    std::ofstream outputFile(launcherFile);

    outputFile << "#!/usr/bin/env python\n";
    outputFile << "import sys\n"
        "import os"  << "\n"
        "root = sys.path[0]\n"
        "sys.path.insert(1, os.path.join(root,'../lib'))\n";
    outputFile  << "sys.path.insert(1, '/legato/systems/current/lib/python2.7/site-packages')"
        << "\n";
    outputFile << "import liblegato\n";

    // Convert argv list to a char**, making sure the string pointers don't die
    outputFile  << "argv_keepalive = [liblegato.ffi.new('char[]', arg) for arg in sys.argv]"
        << "\n";
    outputFile << "argv = liblegato.ffi.new('char *[]', argv_keepalive)\n";

    outputFile << "liblegato.le_arg_SetArgs(len(sys.argv), argv)\n";

    for (auto componentInstPtr : exePtr->componentInstances)
    {
        auto componentPtr = componentInstPtr->componentPtr;

        if (componentPtr->HasPythonCode())
        {
            for (auto ifInstancePtr : componentInstPtr->clientApis)
            {
                auto ifPtr = ifInstancePtr->ifPtr;
                auto apiName = ifPtr->internalName;

                outputFile << "import " << apiName << "\n";

                outputFile  << apiName << ".set_ServiceInstanceName('"
                    << ifInstancePtr->name << "')\n";

                // If not marked for manual start,
                if (!(ifPtr->manualStart) && !(ifPtr->optional))
                {
                    // Have the component connect to services before running packages.
                    outputFile << apiName << ".ConnectService()\n";
                }
                else
                {
                    outputFile << "    // '" << ifPtr->internalName << "' is [manual-start].\n";
                }
            }
            // Copy packages to bin and make the main exe import (run) each package
            // This path insertion removes the need to have __init__.py in every component
            // directory.
            // Every subsequent component gets top name resolution priority.
            outputFile << "sys.path.insert(1, os.path.join(root, '" << componentPtr->name
                << "'))\n";
            for (auto pyPkgPtr : componentPtr->pythonPackages)
            {
                auto importName = pyPkgPtr->packageName;
                if (path::HasSuffix(importName, ".py"))
                    importName = path::RemoveSuffix(importName, ".py");
                outputFile << "import " << importName << "\n";
            }
        }
    }
    outputFile << "liblegato.le_event_RunLoop()";
    outputFile << "\n\n";
    outputFile.close();
}



} // namespace code
