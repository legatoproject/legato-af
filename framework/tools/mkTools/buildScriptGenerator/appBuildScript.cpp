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
#include "appBuildScript.h"
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
 * Generate app build rules.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateAppBuildRules
(
    std::ofstream& script   ///< Ninja script to write rules to.
)
//--------------------------------------------------------------------------------------------------
{
    script <<
        // Add a bundled file into the app's staging area.
        "rule BundleFile\n"
        "  description = Bundling file\n"
        "  command = cp -T $in $out && $\n"
        "            chmod $modeFlags $out\n"
        "\n"

        // Generate a rule for creating an info.properties file.
        "rule MakeAppInfoProperties\n"
        "  description = Creating info.properties\n"
        // Delete the old info.properties file, if there is one.
        "  command = rm -f $out && $\n"
        // Compute the MD5 checksum of the staging area.
        // Don't follow symlinks (-P), and include the directory structure and the contents of
        // symlinks as part of the MD5 hash.
        "            md5=$$( ( find -P $workingDir/staging | sort && $\n"
        "                      find -P $workingDir/staging -type f | sort | xargs cat && $\n"
        "                      find -P $workingDir/staging -type l | sort |"
                                                                       " xargs -r -n 1 readlink $\n"
        "                    ) | md5sum) && $\n"
        "            md5=$${md5%% *} && $\n"
        // Generate the app's info.properties file.
        "            ( echo \"app.name=$name\" && $\n"
        "              echo \"app.md5=$$md5\" && $\n"
        "              echo \"app.version=$version\" && $\n"
        "              echo \"legato.version=`cat $$LEGATO_ROOT/version`\" $\n"
        "            ) > $out\n"
        "\n"

        // Create an update pack file for an app.
        "rule PackApp\n"
        "  description = Packaging app\n"
        // Pack the staging area into a tarball.
        "  command = tar cjf $workingDir/$name.$target -C $workingDir/staging . && $\n"
        // Get the size of the tarball.
        "            tarballSize=`stat -c '%s' $workingDir/$name.$target` && $\n"
        // Get the app's MD5 hash from its info.properties file.
        "            md5=`grep '^app.md5=' $in | sed 's/^app.md5=//'` && $\n"
        // Generate a JSON header and concatenate the tarball to it to create the update pack.
        "            ( printf '{\\n' && $\n"
        "              printf '\"command\":\"updateApp\",\\n' && $\n"
        "              printf '\"name\":\"$name\",\\n' && $\n"
        "              printf '\"version\":\"$version\",\\n' && $\n"
        "              printf '\"md5\":\"%s\",\\n' \"$$md5\" && $\n"
        "              printf '\"size\":%s\\n' \"$$tarballSize\" && $\n"
        "              printf '}' && $\n"
        "              cat $workingDir/$name.$target $\n"
        "            ) > $out\n"
        "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generates build statements for all the executables in a given app.
 */
//--------------------------------------------------------------------------------------------------
void GenerateExeBuildStatements
(
    std::ofstream& script,      ///< Ninja script to write rules to.
    const model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    for (auto mapItem : appPtr->executables)
    {
        GenerateBuildStatements(script, mapItem.second, buildParams);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a permission string for chmod based on the permissions we want to set on the target
 * file.
 **/
//--------------------------------------------------------------------------------------------------
static std::string PermissionsToModeFlags
(
    model::Permissions_t permissions  ///< The permission flags to set on the given file(s).
)
//--------------------------------------------------------------------------------------------------
{
    std::string flags;

    if (permissions.IsExecutable())
    {
        flags = "u+rwx,g+rwx,o+x";
    }
    else
    {
        flags = "u+rw,g+rw,o";
    }

    if (permissions.IsReadable())
    {
        flags += "+r";
    }
    else
    {
        flags += "-r";
    }

    if (permissions.IsWriteable())
    {
        flags += "+w";
    }
    else
    {
        flags += "-w";
    }

    return flags;
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
    model::Permissions_t permissions,  ///< Permissions to set on the dest file.
    std::set<std::string>& bundledFiles, ///< Set to fill with bundled file paths.
    const std::string& srcPath,   ///< Absolute path of source directory on localhost.
    const std::string& destPath,  ///< Absolute path of dest (staging) directory on localhost.
    std::function<void(const std::string&)> exceptionFunc ///< Function to use to throw exceptions.
)
//--------------------------------------------------------------------------------------------------
{
    std::string containingDir = path::GetContainingDir(destPath);

    if (bundledFiles.find(destPath) == bundledFiles.end())
    {
        script << "build " << destPath << " : BundleFile " << srcPath
               << " | " << containingDir << "\n"
               << "  modeFlags = " << PermissionsToModeFlags(permissions) << "\n";

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
    const model::Permissions_t& permissions,
    std::set<std::string>& bundledFiles, ///< Set to fill with bundled file paths.
    const std::string& srcPath,   ///< Absolute path of source directory on localhost.
    const std::string& destPath,  ///< Absolute path of dest (staging) directory on localhost.
    std::function<void(const std::string&)> exceptionFunc ///< Function to use to throw exceptions.
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
                                                 permissions,
                                                 bundledFiles,
                                                 entrySrcPath,
                                                 entryDestPath,
                                                 exceptionFunc);
            }
            // If this is a file, create a build statement for it.
            else if (file::FileExists(entrySrcPath))
            {
                GenerateFileBundleBuildStatement(script,
                                                 permissions,
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
    const model::App_t* appPtr, ///< App to bundle the file into.
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

    // The file will be put in the app's staging area.
    path::Path_t destPath = "$builddir";
    destPath += appPtr->workingDir;
    destPath += "staging";

    // Put in different place depending on whether it should be read-only or writeable on target.
    if (fileSystemObjPtr->permissions.IsWriteable())
    {
        destPath += "writeable";
    }
    else
    {
        destPath += "read-only";
    }

    destPath += fileSystemObjPtr->destPath;

    GenerateFileBundleBuildStatement(script,
                                     fileSystemObjPtr->permissions,
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
    const model::App_t* appPtr, ///< App to bundle the directory into.
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

    // The files will be put in the app's staging area.
    path::Path_t destPath = "$builddir";
    destPath += appPtr->workingDir;
    destPath += "staging";

    // Put in different place depending on whether the directory contents should be read-only
    // or writeable on target.
    if (fileSystemObjPtr->permissions.IsWriteable())
    {
        destPath += "writeable";
    }
    else
    {
        destPath += "read-only";
    }

    destPath += fileSystemObjPtr->destPath;

    GenerateDirBundleBuildStatements(script,
                                     fileSystemObjPtr->permissions,
                                     bundledFiles,
                                     fileSystemObjPtr->srcPath,
                                     destPath.str,
                                     throwException);
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for bundling a given app's files into the
 * app's staging area.
 *
 * @note Uses a set to track the bundled objects (destination paths) that have been included so far.
 *       This allows us to avoid bundling two files into the same location in the staging area.
 *       The set can also be used later by the calling function to add these staged files to the
 *       bundle's dependency list.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateStagingBundleBuildStatements
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
        GenerateFileBundleBuildStatement(script,
                                         bundledFiles,
                                         appPtr,
                                         fileSystemObjPtr,
                                         buildParams);
    }
    for (auto fileSystemObjPtr : appPtr->bundledDirs)
    {
        GenerateDirBundleBuildStatements(script,
                                         bundledFiles,
                                         appPtr,
                                         fileSystemObjPtr,
                                         buildParams);
    }

    // Now do the same for each component in the app, and also generate statements for bundling
    // the component libraries into the app.
    for (auto componentPtr : appPtr->components)
    {
        for (auto fileSystemObjPtr : componentPtr->bundledFiles)
        {
            GenerateFileBundleBuildStatement(script,
                                             bundledFiles,
                                             appPtr,
                                             fileSystemObjPtr,
                                             buildParams);
        }
        for (auto fileSystemObjPtr : componentPtr->bundledDirs)
        {
            GenerateDirBundleBuildStatements(script,
                                             bundledFiles,
                                             appPtr,
                                             fileSystemObjPtr,
                                             buildParams);
        }

        // Generate a statement for bundling a component library into an application, if it has
        // a component library (which will only be the case if the component has sources).
        if ((componentPtr->HasCOrCppCode()) || (componentPtr->HasJavaCode()))
        {
            auto destPath = "$builddir/" + appPtr->workingDir
                          + "/staging/read-only/lib/libComponent_" + componentPtr->name;

            if (componentPtr->HasJavaCode())
            {
                destPath += ".jar";
            }
            else
            {
                destPath += ".so";
            }

            // Hard link the component library into the app's lib directory.
            script << "build " << destPath << " : HardLink " << componentPtr->lib << "\n\n";

            // Add the component library to the set of bundled files.
            bundledFiles.insert(destPath);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script the build statements for packing up everything into an application
 * bundle.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateAppBundleBuildStatement
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

    // Compute the staging directory path.
    auto stagingDir = "$builddir/" + path::Combine(appPtr->workingDir, "staging");

    // Compute the info.properties file path.
    auto infoPropertiesPath = stagingDir + "/info.properties";

    // Generate build statement for generating the info.properties file.
    script << "build " << infoPropertiesPath << " : MakeAppInfoProperties |";

    // This depends on all the bundled files and executables in the app.
    for (auto filePath : bundledFiles)
    {
        script << " " << filePath;
    }
    for (auto mapItem : appPtr->executables)
    {
        script << " $builddir/" << mapItem.second->path;
    }

    // It also depends on the generated config file.
    script << " $builddir/" << appPtr->ConfigFilePath();

    // End of dependency list.
    script << "\n";

    // Tell the build rule what the app's name and version are and where its working directory is.
    script << "  name = " << appPtr->name << "\n"
              "  version = " << appPtr->version << "\n"
              "  workingDir = $builddir/" + appPtr->workingDir << "\n"
              "\n";

    // Generate build statement for zipping up the staging area into an update pack file.
    // This depends on the info.properties file, which is the last thing to be added to the
    // app's staging area.
    auto outputFile = path::Combine(outputDir, appPtr->name) + ".$target.update";
    script << "build " << outputFile << ": PackApp " << infoPropertiesPath << "\n";

    // Tell the build rule what the app's name and version are and where its working directory is.
    script << "  name = " << appPtr->name << "\n"
              "  version = " << appPtr->version << "\n"
              "  workingDir = $builddir/" + appPtr->workingDir << "\n"
              "\n";
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
    std::string filePath = path::Minimize(buildParams.workingDir + "/build.ninja");

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
    script << "target = " << buildParams.target << "\n\n";
    GenerateIfgenFlagsDef(script, buildParams.interfaceDirs);
    GenerateBuildRules(script, buildParams.target, argc, argv);
    GenerateAppBuildRules(script);

    // If we are not just generating code,
    if (!buildParams.codeGenOnly)
    {
        // For each component included in executables in this application.
        for (auto componentPtr : appPtr->components)
        {
            GenerateBuildStatements(script, componentPtr, buildParams);
        }

        // For each executable built by the mk tools for this application,
        GenerateExeBuildStatements(script, appPtr, buildParams);

        // Generate build statement for packing everything into an application bundle.
        GenerateAppBundleBuildStatement(script, appPtr, buildParams, outputDir);
    }

    // Add build statements for all the IPC interfaces' generated files.
    GenerateIpcBuildStatements(script, buildParams);

    // Add a build statement for the build.ninja file itself.
    GenerateNinjaScriptBuildStatement(script, appPtr, filePath);

    CloseFile(script);
}


} // namespace ninja
