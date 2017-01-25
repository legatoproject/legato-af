//--------------------------------------------------------------------------------------------------
/**
 * @file toolChain.h
 *
 * Tool chain related functions needed by the build script generator.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_TOOL_CHAIN_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_TOOL_CHAIN_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Determine if the compiler we are using is clang.
 *
 * @return  Is the specified compiler clang ?
 */
//--------------------------------------------------------------------------------------------------
bool IsCompilerClang
(
    const std::string& compilerPath ///< Path to the compiler
);


//----------------------------------------------------------------------------------------------
/**
 * Get the command-line path to use to invoke the (cross) C compiler for a given target.
 *
 * @return  The path to the compiler.
 *
 * @throw   mk::Exception_t if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetCCompilerPath
(
    const std::string& target   ///< Name of the target platform (e.g., "localhost" or "ar7").
);


//----------------------------------------------------------------------------------------------
/**
 * Get the command-line path to use to invoke the (cross) C++ compiler for a given target.
 *
 * @return  The path to the compiler.
 *
 * @throw   mk::Exception_t if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetCxxCompilerPath
(
    const std::string& target   ///< Name of the target platform (e.g., "localhost" or "ar7").
);


//----------------------------------------------------------------------------------------------
/**
 * Get the command-line path to use to invoke the (cross) linker for a given target.
 *
 * @return  The linker's file system path.
 *
 * @throw   mk::Exception_t if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetLinkerPath
(
    const std::string& target   ///< Name of the target platform (e.g., "localhost" or "ar7").
);


//----------------------------------------------------------------------------------------------
/**
 * Get the command-line path to use to invoke the static library archiver for a given target.
 *
 * @return  The archiver's file system path.
 *
 * @throw   mk::Exception_t if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetArchiverPath
(
    const std::string& target   ///< Name of the target platform (e.g., "localhost" or "ar7").
);


//----------------------------------------------------------------------------------------------
/**
 * Get the sysroot path to use when linking for a given compiler.
 *
 * @return  The path to the sysroot base directory.
 *
 * @throw   mk::Exception_t if target not recognized.
 */
//----------------------------------------------------------------------------------------------
std::string GetSysRootPath
(
    const std::string& compilerPath ///< Path to the compiler
);


#endif // LEGATO_MKTOOLS_TOOL_CHAIN_H_INCLUDE_GUARD
