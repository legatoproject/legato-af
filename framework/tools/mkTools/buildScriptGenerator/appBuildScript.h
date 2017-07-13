//--------------------------------------------------------------------------------------------------
/**
 * @file appBuildScript.h
 *
 * Functions provided by the Application Build Script Generator to other build script generators.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_NINJA_APP_BUILD_SCRIPT_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_NINJA_APP_BUILD_SCRIPT_H_INCLUDE_GUARD

namespace ninja
{


//--------------------------------------------------------------------------------------------------
/**
 * Generate app build rules.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateAppBuildRules
(
    std::ofstream& script   ///< Ninja script to write rules to.
);


//--------------------------------------------------------------------------------------------------
/**
 * Generates build statements for all the executables in a given app.
 */
//--------------------------------------------------------------------------------------------------
void GenerateExeBuildStatements
(
    std::ofstream& script,      ///< Ninja script to write rules to.
    const model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for bundling a given app's files into the
 * app's staging area.
 *
 * @note Uses a set to track the bundled objects (destination paths) that have been included so far.
 *       This allows us to avoid bundling two files into the same location in the staging area.
 *       The set can also be used later by the calling function to add these staged files to the
 *       bundle's dependency list.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateStagingBundleBuildStatements
(
    std::ofstream& script,
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
);


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script the build statements for packing up everything into an application
 * bundle.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateAppBundleBuildStatement
(
    std::ofstream& script,
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams,
    const std::string& outputDir    ///< Path to the directory into which the built app will be put.
);



} // namespace ninja

#endif // LEGATO_MKTOOLS_NINJA_APP_BUILD_SCRIPT_H_INCLUDE_GUARD
