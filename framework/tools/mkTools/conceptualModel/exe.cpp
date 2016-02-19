//--------------------------------------------------------------------------------------------------
/**
 * @file exe.cpp
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

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
    const std::string& workingDir   ///< mk tool working directory path.
)
//--------------------------------------------------------------------------------------------------
:   path(exePath),
    name(path::GetIdentifierSafeName(path::GetLastNode(path))),
    appPtr(appPointer),
    exeDefPtr(NULL),
    mainObjectFile("obj/" + name + "/_main.c.o", LANG_C, "src/" + name + "/_main.c")
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

        // The main C source code file and its object file will be generated in a subdirectory
        // of the app's working dir too.
        mainObjectFile.path = path::Combine(appPtr->workingDir, mainObjectFile.path);
        mainObjectFile.sourceFilePath = path::Combine(appPtr->workingDir,
                                                      mainObjectFile.sourceFilePath);
    }

    // Compute the absolute path of the main C source file that will be generated for this exe.
    mainObjectFile.sourceFilePath = path::Combine(workingDir, mainObjectFile.sourceFilePath);
}


} // namespace model
