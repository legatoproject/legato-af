//--------------------------------------------------------------------------------------------------
/**
 * @file exe.h
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DEFTOOLS_MODEL_EXE_H_INCLUDE_GUARD
#define LEGATO_DEFTOOLS_MODEL_EXE_H_INCLUDE_GUARD

struct App_t;

//--------------------------------------------------------------------------------------------------
/**
 * Represents a single executable.
 */
//--------------------------------------------------------------------------------------------------
struct Exe_t : public HasTargetInfo_t
{
    Exe_t(const std::string& exePath, App_t* appPtr, const std::string& workingDir);

    std::string path;   ///< Path to the executable file. If relative, relative to working dir.

    std::string name;   ///< Name of the executable.

    App_t* appPtr;      ///< Pointer to the app that this exe is part of. NULL if created by mkexe.

    std::string workingDir;   ///< mk tool working directory path.

    const parseTree::Executable_t* exeDefPtr;   ///< Pointer to exe definition in the parse tree.
                                                ///< NULL if created by mkexe.

    /// List of instantiated components.  Sorted such that component instances appear after any
    /// other component instances that they depend on.
    std::list<ComponentInstance_t*> componentInstances;

    std::list<ObjectFile_t*> cObjectFiles;  ///< .o files to build into exe from C sources.
    std::list<ObjectFile_t*> cxxObjectFiles;///< .o files to build into exe from C++ sources.

    bool hasCCode;
    bool hasCppCode;
    bool hasCOrCppCode;
    bool hasJavaCode;
    bool hasPythonCode;
    bool hasIncompatibleLanguageCode;

    void AddComponentInstance(ComponentInstance_t* componentInstancePtr);

    void AddCObjectFile(ObjectFile_t* object)
    {
        cObjectFiles.push_back(object);
        hasCCode = true;
        hasCOrCppCode = true;
    }

    void AddCppObjectFile(ObjectFile_t* object)
    {
        cxxObjectFiles.push_back(object);
        hasCppCode = true;
        hasCOrCppCode = true;
    }

    ObjectFile_t MainObjectFile() const;

    static std::string NameFromPath(std::string path)
    {
        return path::GetIdentifierSafeName(path::GetLastNode(path));
    }
};


#endif // LEGATO_DEFTOOLS_MODEL_EXE_H_INCLUDE_GUARD
