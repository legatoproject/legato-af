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

    std::string path;  ///< Path to pre-built module binary file

    std::string workingDir;   ///< Module target directory

    ObjectFile_t *objFilePtr;   ///< Ptr to .ko file in target directory

    const parseTree::Module_t* parseTreePtr; ///< Ptr to this module's section in the .sdef file parse tree.

    std::map<std::string, std::string> params; ///< Module insmod parameters

    void SetPath(std::string modulePath);  ///< Set path to module binary

    void AddParam(std::string name, std::string value);  ///< Add module param
};


#endif // LEGATO_MKTOOLS_MODEL_MODULE_H_INCLUDE_GUARD
