//--------------------------------------------------------------------------------------------------
/**
 * @file module.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MODEL_MODULE_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MODEL_MODULE_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single module.
 */
//--------------------------------------------------------------------------------------------------
struct Module_t
{
    Module_t(parseTree::MdefFile_t* filePtr);  ///< Constructor

    std::string name;  ///< Module name

    parseTree::MdefFile_t* defFilePtr;  ///< Module's .mdef file

    std::string dir;        ///< Absolute path to the directory containing the .mdef file.

    std::string path;  ///< Path to pre-built module binary file

    std::string workingDir;   ///< Module target directory

    std::string kernelDir;    ///< Kernel build directory

    std::list<std::string> cFlags;    ///< List of options for C compiler

    std::list<std::string> ldFlags;   ///< List of options for linker

    std::list<ObjectFile_t*> cObjectFiles;  ///< List of .o files to build from C source files.

    ObjectFile_t *koFilePtr;   ///< Pointer to .ko file in target directory

    const parseTree::Module_t* parseTreePtr; ///< Ptr to this module's section in the .sdef file parse tree.

    std::map<std::string, std::string> params; ///< Module insmod parameters

    void SetBuildEnvironment();   ///< Set build targets and environment

    void AddParam(std::string name, std::string value);  ///< Add module param
};


#endif // LEGATO_MKTOOLS_MODEL_MODULE_H_INCLUDE_GUARD
