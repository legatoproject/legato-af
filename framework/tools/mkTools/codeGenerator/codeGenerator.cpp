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


void GenerateCLangInterfacesHeader
(
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate an interfaces.h file for a given component.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateInterfacesHeader
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    if (componentPtr->HasCOrCppCode())
    {
        GenerateCLangInterfacesHeader(componentPtr, buildParams);
    }
}

} // namespace code
