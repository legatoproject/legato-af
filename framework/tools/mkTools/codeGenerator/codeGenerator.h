//--------------------------------------------------------------------------------------------------
/**
 * @file codeGenerator.h
 *
 * Copyright (C) Sierra Wireless Inc.,
 **/
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_CODE_GENERATOR_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_CODE_GENERATOR_H_INCLUDE_GUARD

namespace code
{


//--------------------------------------------------------------------------------------------------
/**
 * Generate an interfaces.h file for a given component.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateInterfacesHeader
(
    model::Component_t* componentPtr,
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
void GenerateComponentMainFile
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate an _main.c file for a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateExeMain
(
    model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
);



} // namespace code


#endif // LEGATO_MKTOOLS_CODE_GENERATOR_H_INCLUDE_GUARD
