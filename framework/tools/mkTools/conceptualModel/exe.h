//--------------------------------------------------------------------------------------------------
/**
 * @file exe.h
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_MKTOOLS_MODEL_EXE_H_INCLUDE_GUARD
#define LEGATO_MKTOOLS_MODEL_EXE_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * Represents a single executable.
 */
//--------------------------------------------------------------------------------------------------
struct Exe_t
{
    Exe_t(const std::string& exePath);

    std::string path;   ///< Path to the executable file. If relative, relative to working dir.

    std::string name;   ///< Name of the executable.

    const parseTree::Executable_t* exeDefPtr;   ///< Pointer to exe definition in the parse tree.
                                                ///< NULL if created by mkexe.

    /// List of instantiated components.  Sorted such that component instances appear after any
    /// other component instances that they depend on.
    std::list<ComponentInstance_t*> componentInstances;

    std::list<std::string> cSources;  ///< C source code files built directly into this exe.
    std::list<std::string> cxxSources;  ///< C++ source code files built directly into this exe.

    std::string mainCSourceFile;    ///< _main.c file's path, relative to working directory root.
    std::string mainObjectFile;     ///< _main.c.o file's path, relative to working directory root.
};


#endif // LEGATO_MKTOOLS_MODEL_EXE_H_INCLUDE_GUARD
