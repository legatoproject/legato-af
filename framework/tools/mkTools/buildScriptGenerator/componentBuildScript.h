//--------------------------------------------------------------------------------------------------
/**
 * @file componentBuildScript.h
 *
 * Functions shared with other build script generator files.
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_NINJA_COMPONENT_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_NINJA_COMPONENT_BUILD_SCRIPT_H_INCLUDE_GUARD


namespace ninja
{

//--------------------------------------------------------------------------------------------------
/**
 * Generate a build statements for a component library that is shareable between multiple
 * executables.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateBuildStatements
(
    std::ofstream& script,  ///< Script to write the build statements to.
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
);


} // namespace ninja

#endif // LEGATO_NINJA_COMPONENT_BUILD_SCRIPT_H_INCLUDE_GUARD
