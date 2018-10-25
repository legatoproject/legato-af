//--------------------------------------------------------------------------------------------------
/**
 * @file componentBuildScript.cpp
 *
 * Component build script generation functions.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptCommon.h"
#include "rtosComponentBuildScript.h"


namespace ninja
{

//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the contents that are common to both cFlags and cxxFlags variable
 * definitions for a given Component.
 **/
//--------------------------------------------------------------------------------------------------
void RtosComponentBuildScriptGenerator_t::GenerateCommonCAndCxxFlags
(
    model::Component_t* componentPtr
)
{
    ComponentBuildScriptGenerator_t::GenerateCommonCAndCxxFlags(componentPtr);

    int componentCount = componentPtr->GetTargetInfo<target::RtosComponentInfo_t>()->globalUsage;
    int componentKey = componentPtr->GetTargetInfo<target::RtosComponentInfo_t>()->componentKey;

    script << " -DLE_CDATA_COMPONENT_COUNT=" << componentCount;
    // If component count is greater than 1, assign it a key.
    if (componentCount > 1)
    {
        script << " -DLE_CDATA_KEY=" << componentKey;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Link a component for RTOS.
 *
 * This performs a partial link into an object file.
 */
//--------------------------------------------------------------------------------------------------
void RtosComponentBuildScriptGenerator_t::GenerateComponentLinkStatement
(
    model::Component_t* componentPtr
)
{
    if (componentPtr->cxxObjectFiles.empty() &&
        componentPtr->cObjectFiles.empty())
    {
        // Nothing to build
        return;
    }

    // Create the build statement
    script << "build " << componentPtr->GetTargetInfo<target::RtosComponentInfo_t>()->staticlib
           << ": PartialLink";

    // Add source dependencies
    GetObjectFiles(componentPtr);

    // Add implicit dependencies.
    script << " |";
    GetImplicitDependencies(componentPtr);
    GetExternalDependencies(componentPtr);
    script << "\n";
    script << "  entry=" << componentPtr->initFuncName << "\n"
           << "  ldFlags=";

    script << "-Wl,--undefined=_" << componentPtr->name << "_InitServices $ldflags\n";
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for building a single component.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateRtos
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Combine(buildParams.workingDir, "build.ninja");

    RtosComponentBuildScriptGenerator_t componentGenerator(filePath, buildParams);

    componentGenerator.Generate(componentPtr);
}

} // namespace ninja
