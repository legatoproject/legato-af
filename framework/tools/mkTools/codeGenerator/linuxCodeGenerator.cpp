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


void GenerateCLangComponentMainFile
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
);


void GenerateCLangExeMain
(
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
);



void GenerateJavaComponentMainFile
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
);


void GenerateJavaExeMain
(
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
);

void GeneratePythonExeMain
(
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Sum any pools in this reference to the parent file.
 *
 * This is needed on RTOS as API pools are shared across all references to the API.
 */
//--------------------------------------------------------------------------------------------------
static void AddPoolsToApiFile
(
    model::ApiRef_t *apiRefPtr
)
{
    for (const auto &poolEntry : apiRefPtr->poolSizeEntries)
    {
        auto insertResult = apiRefPtr->apiFilePtr->poolSizeEntries.insert(poolEntry);

        // If insertion failed because an entry already exists, add the size of the pool
        // in this
        if (!insertResult.second)
        {
            // The joys of STL maps.
            // Use the largest pool size value for the new pool size
            if (poolEntry.second > insertResult.first->second)
            {
                insertResult.first->second = poolEntry.second;
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Find how large an API pool needs to be
 *
 * On Linux API pools sizes are shared across all components built in a system, even if
 * the individual pools are not shared.  Go through each application and component in the system
 * to calculate the correct pool size.
 */
//--------------------------------------------------------------------------------------------------
void CalculateLinuxApiPoolSize
(
    const mk::BuildParams_t& buildParams
)
{
    const auto &componentMap = model::Component_t::GetComponentMap();
    for (auto componentEntry : componentMap)
    {
        auto componentPtr = componentEntry.second;

        for (auto serverApiPtr : componentPtr->serverApis)
        {
            AddPoolsToApiFile(serverApiPtr);
        }

        for (auto clientApiPtr : componentPtr->clientApis)
        {
            AddPoolsToApiFile(clientApiPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate _componentMain.c for a given component.
 *
 * This resolves the undefined service name symbols in all the interfaces' .o files
 * and creates a component-specific interface initialization function.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateLinuxComponentMainFile
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // This generator is for Linux & generates necessary code to create a Linux shared library.
    // Add the component-specific info now (if not already present)
    componentPtr->SetTargetInfo(new target::LinuxComponentInfo_t(componentPtr, buildParams));

    if (componentPtr->HasCOrCppCode())
    {
        GenerateCLangComponentMainFile(componentPtr, buildParams);
    }
    else if (componentPtr->HasJavaCode())
    {
        GenerateJavaComponentMainFile(componentPtr, buildParams);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate an _main.c file for a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateLinuxExeMain
(
    model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    if (exePtr->hasCOrCppCode)
    {
        GenerateCLangExeMain(exePtr, buildParams);
    }
    else if (exePtr->hasJavaCode)
    {
        GenerateJavaExeMain(exePtr, buildParams);
    }
    else if (exePtr->hasPythonCode)
    {
        GeneratePythonExeMain(exePtr, buildParams);
    }
}


} // namespace code
