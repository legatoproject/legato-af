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
 * Get symbols which are needed by IPC, but not provided by any service.
 * A typical case is optional bindings.
 */
//--------------------------------------------------------------------------------------------------
static void GetNeededSymbols
(
    model::System_t* systemPtr,
    std::set<std::string> &neededSymbols
)
{
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
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate a list of files and sections which are needed for a specific runGroup.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateSectionListForGroup
(
    std::ostream &outputFile,
    std::map<model::Component_t *, uint8_t> &componentRunGroupMap,
    uint8_t runGroup,
    const char *sectionNames
)
{
    for (const auto &componentRunGroupItem : componentRunGroupMap)
    {
        auto currentComponent = componentRunGroupItem.first;
        auto currentRunGroup = componentRunGroupItem.second;

        // Skip items which are not part of this run group
        if (currentRunGroup != runGroup)
        {
            continue;
        }

        if (currentComponent->HasCOrCppCode())
        {
            outputFile <<
                "        " <<
                currentComponent->GetTargetInfo<target::RtosComponentInfo_t>()->staticlib <<
                "(" << sectionNames << ")\n";
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * If run groups are defined, put each run group in its own section so they can be used
 * in an overlay.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateGccSystemOverlay
(
    std::ostream &outputFile,
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
{
    std::map<model::Component_t *, uint8_t> componentRunGroupMap;

    // First generate a map components to their run groups.
    // Any component which is a member of applications in a single run group is a member of that
    // run group.
    // Any component which is a member of applications in several different run groups is promoted
    // to the common group.
    for (const auto &appItem : systemPtr->apps)
    {
        auto appPtr = appItem.second;

        for (const auto &exeItem : appPtr->executables)
        {
            auto exePtr = exeItem.second;

            for (const auto componentInst : exePtr->componentInstances)
            {
                auto componentPtr = componentInst->componentPtr;

                auto componentRunGroupIter = componentRunGroupMap.find(componentPtr);

                if (componentRunGroupIter != componentRunGroupMap.end())
                {
                    // If this already has a different run group, move it into the common
                    // section
                    auto runGroup = componentRunGroupIter->second;

                    if (runGroup && runGroup != appPtr->runGroup)
                    {
                        componentRunGroupIter->second = 0;
                    }
                }
                else
                {
                    componentRunGroupMap.insert(std::make_pair(componentPtr, appPtr->runGroup));
                }
            }
        }
    }

    std::set<uint8_t> runGroups;

    // Now get all the run groups.  Ignore 0, as it is not included in any overlay
    for (auto &componentRunGroupItem : componentRunGroupMap)
    {
        uint8_t runGroup = componentRunGroupItem.second;

        if (runGroup)
        {
            runGroups.insert(runGroup);
        }
    }

    outputFile <<
        "SECTIONS\n"
        "{\n";

    // Generate memory pools in their own section so they can be included in the hibernation
    // area
    for (auto runGroup : runGroups)
    {
        outputFile <<
            "    .bss.group" << (int)runGroup << "._mem_Pools : {\n";

        GenerateSectionListForGroup(outputFile,
                                    componentRunGroupMap, runGroup, ".bss._mem_*Data");

        outputFile << "    }\n";
    }

    outputFile << "\n";

    // All other pools go in a generic ".bss._mem_Pools" section
    outputFile <<
        "    .bss._mem_Pools :\n"
        "    {\n"
        "        *(.bss._mem_*Data);\n"
        "    }\n\n";

    // Then generate sections for all other .bss memory.  Other RAM areas are currently
    // not considered for overlays
    for (auto runGroup : runGroups)
    {
        outputFile <<
            "    .bss.group" << (int)runGroup << " : {\n";

        GenerateSectionListForGroup(outputFile,
                                    componentRunGroupMap, runGroup, ".bss .bss.*");

        outputFile << "    }\n";
    }

    outputFile << "}\n";
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate linker script for GCC-compatible compilers
 */
//--------------------------------------------------------------------------------------------------
static void GenerateGccLinkerScript
(
    std::ostream &outputFile,
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
{
    std::set<std::string> neededSymbols;

    GetNeededSymbols(systemPtr, neededSymbols);

    outputFile <<
        "/*\n"
        " * Auto-generated file.  Do not edit.\n"
        " */\n";

    // Now go through all needed but undefined symbols, creating an entry for them
    for (auto interfaceSymbol : neededSymbols)
    {
        outputFile << "PROVIDE(" << interfaceSymbol << " = 0);\n";
    }

    outputFile << "\n";

    GenerateGccSystemOverlay(outputFile, systemPtr, buildParams);
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate linker script for ARM RVCT-compatible compilers
 */
//--------------------------------------------------------------------------------------------------
void GenerateArmLinkerScript
(
    std::ostream &outputFile,
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
{
    outputFile <<
        "#\n"
        "# Auto-generated file.  Do not edit.\n"
        "#\n";

    outputFile <<
        "HIDE *\n"
        "SHOW le_microSupervisor_Main\n";
}


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

    if (buildParams.compilerType == mk::BuildParams_t::COMPILER_GCC)
    {
        GenerateGccLinkerScript(outputFile, systemPtr, buildParams);
    }
    else if (buildParams.compilerType == mk::BuildParams_t::COMPILER_ARM_RVCT)
    {
        GenerateArmLinkerScript(outputFile, systemPtr, buildParams);
    }
}

} // end namespace code
