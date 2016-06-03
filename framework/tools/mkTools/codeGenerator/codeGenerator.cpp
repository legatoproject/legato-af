//--------------------------------------------------------------------------------------------------
/**
 * @file codeGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"


namespace code
{


void GenerateCLangInterfacesHeader
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
);


void GenerateCLangComponentMainFile
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams,
    bool isStandAlone   ///< true = fully resolve all interface name variables.
);


void GenerateCLangExeMain
(
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
);



void GenerateJavaComponentMainFile
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams,
    bool isStandAlone   ///< true = fully resolve all interface name variables.
);


void GenerateJavaExeMain
(
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
);




//--------------------------------------------------------------------------------------------------
/**
 * Generate an interfaces.h file for a given component.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateInterfacesHeader
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    if (componentPtr->HasCOrCppCode())
    {
        GenerateCLangInterfacesHeader(componentPtr, buildParams);
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
void GenerateComponentMainFile
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams,
    bool isStandAlone   ///< true = fully resolve all interface name variables.
)
//--------------------------------------------------------------------------------------------------
{
    if (componentPtr->HasCOrCppCode())
    {
        GenerateCLangComponentMainFile(componentPtr, buildParams, isStandAlone);
    }
    else if (componentPtr->HasJavaCode())
    {
        GenerateJavaComponentMainFile(componentPtr, buildParams, isStandAlone);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate an _main.c file for a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateExeMain
(
    const model::Exe_t* exePtr,
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
}


} // namespace code
