//--------------------------------------------------------------------------------------------------
/**
 * @file codeGenerator.h
 *
 * Copyright (C) Sierra Wireless Inc.
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
 * Generate _componentMain.c for a given component on Linux.
 *
 * This resolves the undefined service name symbols in all the interfaces' .o files
 * and creates a component-specific interface initialization function.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateLinuxComponentMainFile
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
void GenerateRtosComponentMainFile
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
);

//--------------------------------------------------------------------------------------------------
/**
 * Generate an _main.c file for a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateLinuxExeMain
(
    model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
);

//--------------------------------------------------------------------------------------------------
/**
 * Generate an _main.c file for a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateRtosExeMain
(
    model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
);

//--------------------------------------------------------------------------------------------------
/**
 * Count how many times a component is used on the system.
 *
 * This is an analysis step which is required for some RTOS code generation tasks.
 */
//--------------------------------------------------------------------------------------------------
void CountSystemComponentUsage
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
);

//--------------------------------------------------------------------------------------------------
/**
 * Generate a task list for a given system.
 */
//--------------------------------------------------------------------------------------------------
void GenerateRtosSystemTasks
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate a rpcServices.c for rpc services and links in a given system.
 */
//--------------------------------------------------------------------------------------------------
void GenerateRtosRpcServices
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate linker script for RTOS system.
 *
 * This linker script will create weak definitions for all APIs, so optional APIs will
 * appear as unbound if no binding exists for them.
 */
//--------------------------------------------------------------------------------------------------
void GenerateLinkerScript
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Convert service-directory interface name to symbol for use in local bindings.
 */
//--------------------------------------------------------------------------------------------------
std::string ConvertInterfaceNameToSymbol
(
    const std::string &interfaceName
);


} // namespace code


#endif // LEGATO_MKTOOLS_CODE_GENERATOR_H_INCLUDE_GUARD
