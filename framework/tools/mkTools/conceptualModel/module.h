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
struct Module_t : public HasTargetInfo_t
{
    Module_t(parseTree::MdefFile_t* filePtr);  ///< Constructor

    std::string name;  ///< Module name

    parseTree::MdefFile_t* defFilePtr;  ///< Module's .mdef file

    std::string dir;        ///< Absolute path to the directory containing the .mdef file.

    std::string workingDir;   ///< Module target directory

    std::string kernelDir;    ///< Kernel build directory

    std::list<std::string> cFlags;    ///< List of options for C compiler

    std::list<std::string> ldFlags;   ///< List of options for linker

    std::list<ObjectFile_t*> cObjectFiles;  ///< List of .o files to build from C source files.

    enum ModuleBuildType_t : int {Invalid = 0, Sources, Prebuilt};

    ModuleBuildType_t moduleBuildType; ///< Enum to differentiate type of kernel module:
                                       ///< Sources or Prebuilt

    std::map<std::string, ObjectFile_t*> koFiles; ///< Map of kernel object (.ko) file and pointer
                                                  ///< to .ko file in target directory
    std::map<std::string, parseTree::Token_t*> koFilesToken; ///< Map of ko files and its token

    const parseTree::RequiredModule_t* parseTreePtr; ///< Ptr to this module's section in the .sdef
                                             ///< file parse tree.

    std::map<std::string, std::string> params; ///< Module insmod parameters

    ///< Map of required modules.
    ///< Key is module name and value is the pair of token pointer and it's bool 'optional' value.
    std::map<std::string, std::pair<parseTree::Token_t*, bool>> requiredModules;

    enum {AUTO, MANUAL} loadTrigger;  ///< Module is loaded either auto at startup or manually

    FileObjectPtrSet_t bundledFiles; ///< List of files to be bundled in the module.
    FileObjectPtrSet_t bundledDirs;  ///< List of directories to be bundled in the module.

    std::string installScript;   ///< Install script file path
    std::string removeScript;    ///< Remove script file path

    void SetBuildEnvironment(ModuleBuildType_t type, std::string path);  ///< Set build targets and
                                                                         ///< environment

    void AddParam(std::string name, std::string value);  ///< Add module param

    static Module_t* GetModule(const std::string& path); ///< Get module object for a module name

protected:

    static std::map<std::string, Module_t*> ModuleMap;  ///< Map of module name to module objects

};


#endif // LEGATO_MKTOOLS_MODEL_MODULE_H_INCLUDE_GUARD
