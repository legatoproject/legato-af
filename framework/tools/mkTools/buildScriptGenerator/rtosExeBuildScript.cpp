//--------------------------------------------------------------------------------------------------
/**
 * @file rtosExeBuildScript.cpp
 *
 * Implementation of the build script generator for executables built using mkexe.
 *
 * On RTOS these will translate to a collection of RTOS tasks.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "rtosExeBuildScript.h"

namespace ninja
{

//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script a build statement for a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void RtosExeBuildScriptGenerator_t::GenerateBuildStatement
(
    model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    // On RTOS executables compile to .o and are linked at a later stage.
    exePtr->path += ".o";

    auto exePath = exePtr->path;

    if (!path::IsAbsolute(exePath))
    {
        exePath = "$builddir/" + exePath;
    }
    script << "build " << exePath << ": PartialLink" <<
              " $builddir/" << exePtr->MainObjectFile().path;

    // Link in all the .o files for C/C++ sources.
    for (auto objFilePtr : exePtr->cObjectFiles)
    {
        script << " $builddir/" << objFilePtr->path;
    }
    for (auto objFilePtr : exePtr->cxxObjectFiles)
    {
        script << " $builddir/" << objFilePtr->path;
    }

    script << "\n"
              "  entry=" << exePtr->GetTargetInfo<target::RtosExeInfo_t>()->entryPoint << "\n"
              "  ldFlags="
              "-Wl,--undefined=" << exePtr->GetTargetInfo<target::RtosExeInfo_t>()->initFunc <<
              " $ldFlags\n"
              "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for an executable and associated component and IPC interface
 * libraries.
 *
 * @note This is only used by mkexe.
 */
//--------------------------------------------------------------------------------------------------
void GenerateRtos
(
    model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Minimize(buildParams.workingDir + "/build.ninja");

    RtosExeBuildScriptGenerator_t scriptGenerator(filePath, buildParams);

    scriptGenerator.Generate(exePtr);
}

}
