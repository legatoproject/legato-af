//--------------------------------------------------------------------------------------------------
/**
 * @file exe.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#include "defTools.h"

namespace model
{


//--------------------------------------------------------------------------------------------------
/**
 * Constructor.
 **/
//--------------------------------------------------------------------------------------------------
Exe_t::Exe_t
(
    const std::string& exePath,
    App_t* appPointer,
    const std::string& mkWorkingDir   ///< mk tool working directory path.
)
//--------------------------------------------------------------------------------------------------
:   path(exePath),
    name(Exe_t::NameFromPath(path)),
    appPtr(appPointer),
    workingDir(mkWorkingDir),
    exeDefPtr(NULL),

    hasCCode(false),
    hasCppCode(false),
    hasCOrCppCode(false),
    hasJavaCode(false),
    hasPythonCode(false),
    hasIncompatibleLanguageCode(false)
//--------------------------------------------------------------------------------------------------
{
    // If being built as part of an app,
    if (appPtr != NULL)
    {
        // If the executable file's path is not absolute, then it is relative to the app's working
        // directory, so prefix the exe's path with the app's working dir path.
        if (!path::IsAbsolute(path))
        {
            path = path::Combine(appPtr->workingDir, path);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a new component instance to the executable.
 **/
//--------------------------------------------------------------------------------------------------
void Exe_t::AddComponentInstance
(
    ComponentInstance_t* componentInstancePtr  ///< The instance to add.
)
//--------------------------------------------------------------------------------------------------
{
    componentInstances.push_back(componentInstancePtr);

    hasCCode |= componentInstancePtr->componentPtr->HasCCode();
    hasCppCode |= componentInstancePtr->componentPtr->HasCppCode();

    hasCOrCppCode |= hasCCode || hasCppCode;

    hasJavaCode |= componentInstancePtr->componentPtr->HasJavaCode();

    hasPythonCode |= componentInstancePtr->componentPtr->HasPythonCode();

    hasIncompatibleLanguageCode |= (hasCOrCppCode + hasJavaCode + hasPythonCode) > 1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Compute the paths to the main object file and main source file based on the language detected.
 **/
//--------------------------------------------------------------------------------------------------
ObjectFile_t Exe_t::MainObjectFile
(
)
const
//--------------------------------------------------------------------------------------------------
{
    std::string objectName;
    std::string sourceName;

    if (hasCOrCppCode)
    {
        objectName = "obj/" + name + "/_main.c.o";
        sourceName = "src/" + name + "/_main.c";
    }
    else if (hasJavaCode)
    {
        objectName = "src/" + name + "/io/legato/generated/exe/" + name + "/Main.class";
        sourceName = "src/" + name + "/io/legato/generated/exe/" + name + "/Main.java";
    }
    else if (hasPythonCode)
    {
        objectName = "src/" + name + "_main.py";
        sourceName = "src/" + name + "_main.py";
    }
    else
    {
        throw mk::Exception_t(LE_I18N("Unexpected language for main executable."));
    }

    ObjectFile_t mainObjectFile(objectName, sourceName);

    // If being built as part of an app,
    if (appPtr != NULL)
    {
        // The main C source code file and its object file will be generated in a subdirectory
        // of the app's working dir too.
        mainObjectFile.path = path::Combine(appPtr->workingDir, mainObjectFile.path);
        mainObjectFile.sourceFilePath = path::Combine(appPtr->workingDir,
                                                      mainObjectFile.sourceFilePath);
    }

    // Compute the absolute path of the main C source file that will be generated for this exe.
    mainObjectFile.sourceFilePath = path::Combine(workingDir, mainObjectFile.sourceFilePath);

    return mainObjectFile;
}


} // namespace model
