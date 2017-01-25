//--------------------------------------------------------------------------------------------------
/**
 * @file exeBuildScript.h
 *
 * Functions provided by the Executables Build Script Generator to other build script generators.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_NINJA_EXE_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_NINJA_EXE_BUILD_SCRIPT_H_INCLUDE_GUARD

namespace ninja
{


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the build statements related to a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateBuildStatements
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const model::Exe_t* exePtr,
    const mk::BuildParams_t& buildParams
);


} // namespace ninja

#endif // LEGATO_MKTOOLS_NINJA_EXE_BUILD_SCRIPT_H_INCLUDE_GUARD
