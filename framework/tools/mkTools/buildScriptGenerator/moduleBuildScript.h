//--------------------------------------------------------------------------------------------------
/**
 * @file moduleBuildScript.h
 *
 * Functions provided by the Modules Build Script Generator to other build script generators.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_NINJA_MODULE_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_NINJA_MODULE_BUILD_SCRIPT_H_INCLUDE_GUARD

namespace ninja
{


//--------------------------------------------------------------------------------------------------
/**
 * Create Makefile for a given module.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateMakefile
(
    const model::Module_t* modulePtr,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the build statements related to a given module.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateBuildStatements
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const model::Module_t* modulePtr,
    const mk::BuildParams_t& buildParams
);


} // namespace ninja

#endif // LEGATO_MKTOOLS_NINJA_MODULE_BUILD_SCRIPT_H_INCLUDE_GUARD
