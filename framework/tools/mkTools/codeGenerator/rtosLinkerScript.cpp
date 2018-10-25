/**
 * @file rtosLinkerScript.cpp
 *
 * Create a linker script for default bindings for RTOS so if an optional API
 * isn't bound, the binding appears as "NULL"
 */

#include "mkTools.h"

namespace code
{

//--------------------------------------------------------------------------------------------------
/**
 * Generate linker script for RTOS system.
 *
 * This linker script will create NULL definitions for all services which are not provided by
 * any executable.
 */
//--------------------------------------------------------------------------------------------------
void GenerateLinkerScript
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
{
    auto linkerScriptFile = path::Combine(buildParams.workingDir, "src/legato.ld");

    // Open the file as an output stream.
    file::MakeDir(path::GetContainingDir(linkerScriptFile));
    std::ofstream outputFile(linkerScriptFile);
    if (outputFile.is_open() == false)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Could not open '%s' for writing."), linkerScriptFile)
        );
    }

    std::set<std::string> neededSymbols;

    outputFile <<
        "/*\n"
        " * Auto-generated file.  Do not edit.\n"
        " */\n";

    // Add all required services to needed symbol list
    for (auto appItem : systemPtr->apps)
    {
        auto appPtr = appItem.second;
        for (auto exeItem : appPtr->executables)
        {
            auto exePtr = exeItem.second;
            for (auto componentInstancePtr : exePtr->componentInstances)
            {
                for (auto clientApiInstancePtr : componentInstancePtr->clientApis)
                {
                    auto bindingPtr = clientApiInstancePtr->bindingPtr;
                    // Optional interfaces may not be bound, so check if binding exists first.
                    if (bindingPtr)
                    {
                        auto interfaceSymbol =
                            ConvertInterfaceNameToSymbol(bindingPtr->serverIfName);

                        neededSymbols.insert(interfaceSymbol);
                    }
                }
            }
        }
    }

    // Then remove all services which are provided by some app
    for (auto appItem : systemPtr->apps)
    {
        auto appPtr = appItem.second;
        for (auto exeItem : appPtr->executables)
        {
            auto exePtr = exeItem.second;
            for (auto componentInstancePtr : exePtr->componentInstances)
            {
                for (auto serverApiInstancePtr : componentInstancePtr->serverApis)
                {
                    auto interfaceSymbol = ConvertInterfaceNameToSymbol(serverApiInstancePtr->name);

                    auto symbolIter = neededSymbols.find(interfaceSymbol);
                    if (symbolIter != neededSymbols.end())
                    {
                        neededSymbols.erase(symbolIter);
                    }
                }
            }
        }
    }

    // Now go through all needed but undefined symbols, creating an entry for them
    for (auto interfaceSymbol : neededSymbols)
    {
        outputFile << "PROVIDE(" << interfaceSymbol << " = 0);\n";
    }
}

} // end namespace code
