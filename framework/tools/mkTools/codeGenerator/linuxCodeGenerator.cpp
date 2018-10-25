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
