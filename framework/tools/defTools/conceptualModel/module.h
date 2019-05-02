//--------------------------------------------------------------------------------------------------
/**
 * @file module.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODEL_MODULE_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODEL_MODULE_H_INCLUDE_GUARD


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
    std::list<ObjectFile_t*> subCObjectFiles; ///< List of .o files to build from C source files
                                              ///< only for sub kernel modules

    std::list<std::string> externalBuildCommands; ///< List of external build commands.

    enum ModuleBuildType_t : int {Invalid = 0, Sources, Prebuilt};

    ModuleBuildType_t moduleBuildType; ///< Enum to differentiate type of kernel module:
                                       ///< Sources or Prebuilt

    std::multimap<std::string, ObjectFile_t*> koFiles; ///< Map of kernel object (.ko) file
                                                       ///< (includes sub .ko files) and pointer to
                                                       ///< .ko file in target directory
    std::map<std::string, parseTree::Token_t*> koFilesToken; ///< Map of ko files and its token.
                                                             ///< Applicable for prebuilt only.

    const parseTree::RequiredModule_t* parseTreePtr; ///< Ptr to this module's section in the .sdef
                                                     ///< file parse tree.

    std::map<std::string, std::string> params; ///< Module insmod parameters


    std::string subModuleName; ///< Name of a sub kernel module

    std::map<std::string, std::list<ObjectFile_t*>> subKernelModules;
    ///< Map of sub kernel modules and the list of pointer to object files

    typedef struct {
        Module_t* modPtr;             ///< Pointer to module object
        parseTree::Token_t* tokenPtr; ///< Pointer to module token
        bool isOptional;              ///< Module's optional value
    } ModuleInfoOptional_t;           ///< Structure type with module's info and optional value.

    std::map<std::string, ModuleInfoOptional_t> requiredModules;
    ///< Map of required modules.
    ///< Key is module name and value is the struct of token pointer and its bool 'optional' value.

    std::map<std::string, std::map<std::string, ModuleInfoOptional_t>> requiredSubModules;
    ///< Name of the subModName and the map of corresponding requiredSubModules

    std::map<std::string, ModuleInfoOptional_t> requiredModulesOfSubMod;
    ///< Map of the required modules of sub modules.
    ///< Key is the name of the required module and Value is the struct of its token pointer and
    ///< bool 'optional' value.

    enum {AUTO, MANUAL} loadTrigger;  ///< Module is loaded either auto at startup or manually

    FileObjectPtrSet_t bundledFiles; ///< List of files to be bundled in the module.
    FileObjectPtrSet_t bundledDirs;  ///< List of directories to be bundled in the module.

    std::string installScript;   ///< Install script file path
    std::string removeScript;    ///< Remove script file path

    void SetBuildEnvironment(ModuleBuildType_t type, std::string path);
    ///< Set build targets and environment

    void SetBuildEnvironmentSubModule(std::string path);
    ///< Set build targets and environment for sub module

    void AddParam(std::string name, std::string value);  ///< Add module param

    static Module_t* GetModule(const std::string& path); ///< Get module object for a module name

    // Is the module built using an external build process
    bool HasExternalBuild() const
    {
        return (!externalBuildCommands.empty());
    }

protected:

    static std::map<std::string, Module_t*> ModuleMap;  ///< Map of module name to module objects

};


#endif // LEGATO_DEFTOOLS_MODEL_MODULE_H_INCLUDE_GUARD
