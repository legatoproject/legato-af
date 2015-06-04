//--------------------------------------------------------------------------------------------------
/**
 * @file buildScriptCommon.h
 *
 * Declarations of common functions shared by the build script generators.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_BUILD_SCRIPT_COMMON_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_BUILD_SCRIPT_COMMON_H_INCLUDE_GUARD

namespace ninja
{



//--------------------------------------------------------------------------------------------------
/**
 * Create a build script file and open it.
 **/
//--------------------------------------------------------------------------------------------------
void OpenFile
(
    std::ofstream& script,
    const std::string& filePath,
    bool beVerbose
);


//--------------------------------------------------------------------------------------------------
/**
 * Close a build script file and check for errors.
 **/
//--------------------------------------------------------------------------------------------------
void CloseFile
(
    std::ofstream& script
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the absolute path to a given C or C++ source code file belonging to a given component.
 *
 * @return the absolute path.
 **/
//--------------------------------------------------------------------------------------------------
std::string GetAbsoluteSourcePath
(
    const std::string& sourceFile,  ///< Path (possibly relative) to the source code file.
    const model::Component_t* componentPtr  ///< The component that the source code belongs to.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the object code file for a given C or C++ source code file.
 **/
//--------------------------------------------------------------------------------------------------
std::string GetObjectFile
(
    const std::string& sourceFile   ///< Path to the source code file.
);


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the ifgenFlags variable definition.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateIfgenFlagsDef
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const std::list<std::string>& interfaceDirs ///< List of directories to search for .api files.
);


//--------------------------------------------------------------------------------------------------
/**
 * Generate generic build rules.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateBuildRules
(
    std::ofstream& script,      ///< Ninja script to write rules to.
    const std::string& target,  ///< Build target (e.g., "localhost");
    int argc,                   ///< Count of the number of command line parameters.
    const char** argv           ///< Pointer to array of pointers to command line argument strings.
);


//--------------------------------------------------------------------------------------------------
/**
 * Stream out (to a given ninja script) the compiler command line arguments required
 * to Set the DT_RUNPATH variable inside the executable's ELF headers to include the expected
 * on-target runtime locations of the libraries needed.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateRunPathLdFlags
(
    std::ofstream& script,
    const std::string& target
);


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for all the IPC client and server
 * header files, source code files, and object files needed by a given component.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateIpcBuildStatements
(
    std::ofstream& script,
    const model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams,
    std::set<std::string>& generatedSet ///< Paths to files that already have build statements.
);


} // namespace ninja

#endif // LEGATO_MKTOOLS_BUILD_SCRIPT_COMMON_H_INCLUDE_GUARD
