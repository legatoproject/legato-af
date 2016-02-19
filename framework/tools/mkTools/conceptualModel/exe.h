//--------------------------------------------------------------------------------------------------
/**
 * @file exe.h
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MODEL_EXE_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MODEL_EXE_H_INCLUDE_GUARD

struct App_t;

//--------------------------------------------------------------------------------------------------
/**
 * Represents a single executable.
 */
//--------------------------------------------------------------------------------------------------
struct Exe_t
{
    Exe_t(const std::string& exePath, App_t* appPtr, const std::string& workingDir);

    std::string path;   ///< Path to the executable file. If relative, relative to working dir.

    std::string name;   ///< Name of the executable.

    App_t* appPtr;      ///< Pointer to the app that this exe is part of. NULL if created by mkexe.

    const parseTree::Executable_t* exeDefPtr;   ///< Pointer to exe definition in the parse tree.
                                                ///< NULL if created by mkexe.

    /// List of instantiated components.  Sorted such that component instances appear after any
    /// other component instances that they depend on.
    std::list<ComponentInstance_t*> componentInstances;

    std::list<ObjectFile_t*> cObjectFiles;  ///< .o files to build into exe from C sources.
    std::list<ObjectFile_t*> cxxObjectFiles;///< .o files to build into exe from C++ sources.

    ObjectFile_t mainObjectFile;     ///< _main.c.o file.
};


#endif // LEGATO_MKTOOLS_MODEL_EXE_H_INCLUDE_GUARD
