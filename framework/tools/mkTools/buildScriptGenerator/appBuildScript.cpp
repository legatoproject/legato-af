//--------------------------------------------------------------------------------------------------
/**
 * @file appBuildScript.cpp
 *
 * Implementation of the build script generator for applications.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptCommon.h"
#include "exeBuildScript.h"
#include "componentBuildScript.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

namespace ninja
{



//--------------------------------------------------------------------------------------------------
/**
 * Generate comment header for an app build script.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateCommentHeader
(
    std::ofstream& script,  ///< Build script to write the variable definition to.
    const model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    script  << "# Build script for application '" << appPtr->name << "'\n"
            "\n"
            "# == Auto-generated file.  Do not edit. ==\n"
            "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for all the IPC client and server
 * header files, source code files, and object files needed by a given app.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateIpcBuildStatements
(
    std::ofstream& script,
    const model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // It's possible that multiple components in the same application will share the same interface.
    // To prevent the generation of multiple build statements (which would cause ninja to fail),
    // we use a set containing the output file paths to keep track of what build statements we've
    // already generated.

    std::set<std::string> generatedSet;

    for (auto componentPtr : appPtr->components)
    {
        GenerateIpcBuildStatements(script, componentPtr, buildParams, generatedSet);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statement for bundling a single file into
 * the staging area.
 *
 * Adds the absolute destination file path to the bundledFiles set.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateFileBundleBuildStatement
(
    std::ofstream& script,
    std::set<std::string>& bundledFiles, ///< Set to fill with bundled file paths.
    const std::string& srcPath,   ///< Absolute path of source directory on localhost.
    const std::string& destPath,  ///< Absolute path of dest (staging) directory on localhost.
    std::function<void(const std::string&)> exceptionFunc    ///< Function to use to throw exceptions.
)
//--------------------------------------------------------------------------------------------------
{
    std::string containingDir = path::GetContainingDir(destPath);

    if (bundledFiles.find(destPath) == bundledFiles.end())
    {
        script << "build " << destPath << " : BundleFile " << srcPath
               << " | " << containingDir << "\n\n";

        bundledFiles.insert(destPath);

        // If the destination directory doesn't already have a build statement for it,
        // create one to make the directory.
        if (bundledFiles.find(containingDir) == bundledFiles.end())
        {
            script << "build " << containingDir << " : MakeDir\n\n";

            bundledFiles.insert(containingDir);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for bundling files from a directory into
 * the staging area.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateDirBundleBuildStatements
(
    std::ofstream& script,
    std::set<std::string>& bundledFiles, ///< Set to fill with bundled file paths.
    const std::string& srcPath,   ///< Absolute path of source directory on localhost.
    const std::string& destPath,  ///< Absolute path of dest (staging) directory on localhost.
    std::function<void(const std::string&)> exceptionFunc    ///< Function to use to throw exceptions.
)
//--------------------------------------------------------------------------------------------------
{
    // Attempt to open the source as a directory stream.
    DIR* dir = opendir(srcPath.c_str());
    if (dir == NULL)
    {
        // If failed for some reason other than this just not being a directory,
        if (errno != ENOTDIR)
        {
            int err = errno;
            exceptionFunc("Can't access file or directory '" + srcPath + "'"
                          " (" + strerror(err) + ")");
        }
        // If the source is not a directory,
        else
        {
            exceptionFunc("Not a directory: '" + srcPath + "'.");
        }
    }

    // Loop over list of directory contents.
    for (;;)
    {
        // Read an entry from the directory.
        struct dirent entry;
        struct dirent* entryPtr;

        int result = readdir_r(dir, &entry, &entryPtr);

        if (result != 0)
        {
            exceptionFunc("Internal error: readdir_r() failed.  Errno = "
                          + std::string(strerror(errno)));
        }
        else if (entryPtr == NULL)
        {
            // Hit end of the directory.  Nothing more to do.
            break;
        }
        // Skip "." and ".."
        else if ((strcmp(entryPtr->d_name, ".") != 0) && (strcmp(entryPtr->d_name, "..") != 0))
        {
            auto entrySrcPath = path::Combine(srcPath, entryPtr->d_name);
            auto entryDestPath = path::Combine(destPath, entryPtr->d_name);

            // If this is a directory, then recursively descend into it.
            if (file::DirectoryExists(entrySrcPath))
            {
                GenerateDirBundleBuildStatements(script,
                                                 bundledFiles,
                                                 entrySrcPath,
                                                 entryDestPath,
                                                 exceptionFunc);
            }
            // If this is a file, create a build statement for it.
            else if (file::FileExists(entrySrcPath))
            {
                GenerateFileBundleBuildStatement(script,
                                                 bundledFiles,
                                                 entrySrcPath,
                                                 entryDestPath,
                                                 exceptionFunc);
            }
            // If this is anything else, we don't support it.
            else
            {
                exceptionFunc("File system object is not a directory or a file: '"
                              + entrySrcPath + "'.");
            }
        }
    }

    closedir(dir);
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statement for bundling a single file into
 * the staging area.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateFileBundleBuildStatement
(
    std::ofstream& script,
    std::set<std::string>& bundledFiles, ///< Set to fill with bundled file paths.
    const model::FileSystemObject_t* fileSystemObjPtr,  ///< File bundling info.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Create a lambda function that uses the file system object's parse tree object to throw
    // an exception that contains the definition file path, line number, etc.
    auto throwException = [&fileSystemObjPtr](const std::string& msg)
        {
            fileSystemObjPtr->parseTreePtr->ThrowException(msg);
        };

    path::Path_t destPath = buildParams.workingDir;
    destPath += "staging";
    destPath += fileSystemObjPtr->destPath;

    GenerateFileBundleBuildStatement(script,
                                     bundledFiles,
                                     fileSystemObjPtr->srcPath,
                                     destPath.str,
                                     throwException);
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for bundling files from a directory into
 * the staging area.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateDirBundleBuildStatements
(
    std::ofstream& script,
    std::set<std::string>& bundledFiles, ///< Set to fill with bundled file paths.
    const model::FileSystemObject_t* fileSystemObjPtr,  ///< Directory bundling info.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Create a lambda function that uses the file system object's parse tree object to throw
    // an exception that contains the definition file path, line number, etc.
    auto throwException = [&fileSystemObjPtr](const std::string& msg)
        {
            fileSystemObjPtr->parseTreePtr->ThrowException(msg);
        };

    path::Path_t destPath = buildParams.workingDir;
    destPath += "staging";
    destPath += fileSystemObjPtr->destPath;

    GenerateDirBundleBuildStatements(script,
                                     bundledFiles,
                                     fileSystemObjPtr->srcPath,
                                     destPath.str,
                                     throwException);
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for bundling files into the staging area.
 *
 * @note Uses a set to track the bundled objects (destination paths) that have been included so far.
 *       This allows us to avoid bundling two files into the same location in the staging area.
 *       The set can also be used later by the calling function to add these staged files to the
 *       bundle's dependency list.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateStagingBundleBuildStatements
(
    std::ofstream& script,
    std::set<std::string>& bundledFiles, ///< Set to fill with bundled file paths.
    const model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Start with the application's list of bundled items first, so they override any items
    // bundled by components.
    // NOTE: Source paths for bundled items are always absolute.
    for (auto fileSystemObjPtr : appPtr->bundledFiles)
    {
        GenerateFileBundleBuildStatement(script, bundledFiles, fileSystemObjPtr, buildParams);
    }
    for (auto fileSystemObjPtr : appPtr->bundledDirs)
    {
        GenerateDirBundleBuildStatements(script, bundledFiles, fileSystemObjPtr, buildParams);
    }

    // Now do the same for each component in the app.
    for (auto componentPtr : appPtr->components)
    {
        for (auto fileSystemObjPtr : componentPtr->bundledFiles)
        {
            GenerateFileBundleBuildStatement(script, bundledFiles, fileSystemObjPtr, buildParams);
        }
        for (auto fileSystemObjPtr : componentPtr->bundledDirs)
        {
            GenerateDirBundleBuildStatements(script, bundledFiles, fileSystemObjPtr, buildParams);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script the build statements for packing up everything into an application
 * bundle.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateAppBundleBuildStatement
(
    std::ofstream& script,
    const model::App_t* appPtr,
    const mk::BuildParams_t& buildParams,
    const std::string& outputDir    ///< Path to the directory into which the built app will be put.
)
//--------------------------------------------------------------------------------------------------
{
    std::set<std::string> bundledFiles;

    // Generate build statements for bundling files into the staging area.
    GenerateStagingBundleBuildStatements(script, bundledFiles, appPtr, buildParams);

    // Generate build statement for zipping up the staging area into an application bundle.
    auto outputFile = path::MakeAbsolute(path::Combine(outputDir,
                                                       appPtr->name + "." + buildParams.target));
    script << "build " << outputFile << ": PackApp |";

    // This depends on all the bundled files, executables, and component libraries in the app.
    for (auto filePath : bundledFiles)
    {
        script << " " << filePath;
    }
    for (auto componentPtr : appPtr->components)
    {
        if (!(componentPtr->lib.empty()))
        {
            script << " " << componentPtr->lib;
        }
    }
    for (auto exePtr : appPtr->executables)
    {
        script << " $builddir/" << exePtr->path;
    }

    // Tell the build rule where the staging directory is.
    script << "\n"
              "  stagingDir = " << path::Combine(buildParams.workingDir, "staging") << "\n\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for the build script itself.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateNinjaScriptBuildStatement
(
    std::ofstream& script,
    const model::App_t* appPtr,
    const std::string& filePath     ///< Path to the build.ninja file.
)
//--------------------------------------------------------------------------------------------------
{
    // Generate a build statement for the build.ninja.
    script << "build " << filePath << ": RegenNinjaScript | " << appPtr->defFilePtr->path;

    // In addition to the .adef file, the build.ninja depends on the .cdef files of all components
    // and all the .api files they use.
    // Create a set of dependencies.
    std::set<std::string> dependencies;
    for (auto componentPtr : appPtr->components)
    {
        dependencies.insert(componentPtr->defFilePtr->path);

        for (auto ifPtr : componentPtr->typesOnlyApis)
        {
            dependencies.insert(ifPtr->apiFilePtr->path);
        }

        for (auto ifPtr : componentPtr->serverApis)
        {
            dependencies.insert(ifPtr->apiFilePtr->path);
        }

        for (auto ifPtr : componentPtr->clientApis)
        {
            dependencies.insert(ifPtr->apiFilePtr->path);
        }

        for (auto apiFilePtr : componentPtr->clientUsetypesApis)
        {
            dependencies.insert(apiFilePtr->path);
        }

        for (auto apiFilePtr : componentPtr->serverUsetypesApis)
        {
            dependencies.insert(apiFilePtr->path);
        }
    }

    // Write the dependencies to the script.
    for (auto dep : dependencies)
    {
        script << " " << dep;
    }
    script << "\n\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for an application.
 **/
//--------------------------------------------------------------------------------------------------
void Generate
(
    const model::App_t* appPtr,
    const mk::BuildParams_t& buildParams,
    const std::string& outputDir,   ///< Path to the directory into which the built app will be put.
    int argc,           ///< Count of the number of command line parameters.
    const char** argv   ///< Pointer to array of pointers to command line argument strings.
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Minimize(  buildParams.workingDir
                                       + '/'
                                       + appPtr->workingDir
                                       + "/build.ninja");

    std::ofstream script;
    OpenFile(script, filePath, buildParams.beVerbose);

    // Start the script with a comment, the file-level variable definitions, and
    // a set of generic rules.
    GenerateCommentHeader(script, appPtr);
    std::string includes;
    for (const auto& dir : buildParams.interfaceDirs)
    {
        includes += " -I" + dir;
    }
    script << "builddir =" << buildParams.workingDir << "\n\n";
    script << "cFlags =" << buildParams.cFlags << includes << "\n\n";
    script << "cxxFlags =" << buildParams.cxxFlags << includes << "\n\n";
    script << "ldFlags =" << buildParams.ldFlags << "\n\n";
    GenerateIfgenFlagsDef(script, buildParams.interfaceDirs);
    GenerateBuildRules(script, buildParams.target, argc, argv);
    script << "rule BundleFile\n"
              "  command = cp -T $in $out\n\n";
    script << "rule MakeDir\n"
              "  command = mkdir -p $out\n\n";
    script << "rule PackApp\n"
              "  command = mkinfo $stagingDir " << appPtr->name << " " << appPtr->version <<
              " && tar cjf $out -C $stagingDir .\n\n";

    if (!buildParams.codeGenOnly)
    {
        // For each executable built by the mk tools for this application,
        for (auto exePtr : appPtr->executables)
        {
            GenerateBuildStatements(script, exePtr, buildParams);
        }

        // For each component included in executables in this application.
        for (auto componentPtr : appPtr->components)
        {
            GenerateBuildStatements(script, componentPtr, buildParams);
        }

        // Generate build statement for packing everything into an application bundle.
        GenerateAppBundleBuildStatement(script, appPtr, buildParams, outputDir);
    }

    // Add build statements for all the IPC interfaces' generated files.
    GenerateIpcBuildStatements(script, appPtr, buildParams);

    // Add a build statement for the build.ninja file itself.
    GenerateNinjaScriptBuildStatement(script, appPtr, filePath);

    CloseFile(script);
}


} // namespace ninja
