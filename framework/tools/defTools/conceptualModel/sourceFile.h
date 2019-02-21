//--------------------------------------------------------------------------------------------------
/**
 * @file sourceFile.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODEL_SOURCE_FILE_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODEL_SOURCE_FILE_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Represents a single compiled-language source code file.
 */
//--------------------------------------------------------------------------------------------------
struct SourceFile_t
{
    std::string path;       ///< File system path to the source file.
                            ///< Absolute or relative to root of the working directory tree.

    ProgramLang_t language; ///< Programming language the source code is written in.

    std::string objectFile; ///< If compiled language, absolute file system path to the object file.

    /// Constructor
    SourceFile_t(const std::string& filePath): path(filePath) {};
};


#endif // LEGATO_DEFTOOLS_MODEL_SOURCE_FILE_H_INCLUDE_GUARD
