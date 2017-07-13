//--------------------------------------------------------------------------------------------------
/**
 * @file appBuildScript.cpp
 *
 * Implementation of the build script generator for applications.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
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
        "  command = legato-install -m $modeFlags $in $out\n"
        "\n"

        // Generate a rule for creating an info.properties file.
        "rule MakeAppInfoProperties\n"
        "  description = Creating info.properties\n"
        // Delete the old info.properties file, if there is one.
        "  command = rm -f $out && $\n"
        // Compute the MD5 checksum of the staging area.
        // Don't follow symlinks (-P), and include the directory structure and the contents of
        // symlinks as part of the MD5 hash.
        "            md5=$$( ( cd $workingDir/staging && $\n"
        "                      find -P | sort && $\n"
        "                      find -P -type f | sort | xargs cat && $\n"
        "                      find -P -type l | sort | xargs -r -n 1 readlink $\n"
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
        "\n"

        "rule BinPackApp\n"
        "  description = Packaging app for distribution.\n"
        "  command = cp -r $stagingDir/* $workingDir/ && $\n"
        "            rm $workingDir/info.properties $workingDir/root.cfg && $\n"
        "            tar cjf $out -C $workingDir/ .\n"
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
    const model::FileSystemObject_t& fileObject,  ///< File object to generate
    model::FileSystemObjectSet_t& bundledFiles    ///< Set to fill with bundled file paths.
)
//--------------------------------------------------------------------------------------------------
{
    std::string containingDir = path::GetContainingDir(fileObject.destPath);
    auto bundledFileIter = bundledFiles.find(fileObject);

    if (bundledFileIter == bundledFiles.end())
    {
        script << "build " << fileObject.destPath << " : BundleFile " << fileObject.srcPath << "\n"
               << "  modeFlags = " << PermissionsToModeFlags(fileObject.permissions) << "\n";

        bundledFiles.insert(fileObject);
    }
    else
    {
        if (fileObject.srcPath != bundledFileIter->srcPath)
        {
            fileObject.parseTreePtr->ThrowException(
                mk::format(LE_I18N("error: Cannot bundle file '%s' with destination '%s' since it"
                                   " conflicts with existing bundled file '%s'."),
                           fileObject.srcPath, fileObject.destPath, bundledFileIter->srcPath)
            );
        }
        else if (fileObject.permissions != bundledFileIter->permissions)
        {
            fileObject.parseTreePtr->ThrowException(
                mk::format(LE_I18N("error: Cannot bundle file '%s'.  It is already bundled with"
                                   " different permissions."),
                           fileObject.srcPath)
            );
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
    const model::FileSystemObject_t& fileObject, ///< File system object to bundle
    model::FileSystemObjectSet_t& bundledFiles   ///< Set to fill with bundled file paths.
)
//--------------------------------------------------------------------------------------------------
{
    // Attempt to open the source as a directory stream.
    DIR* dir = opendir(fileObject.srcPath.c_str());
    if (dir == NULL)
    {
        // If failed for some reason other than this just not being a directory,
        if (errno != ENOTDIR)
        {
            int err = errno;
            fileObject.parseTreePtr->ThrowException(
                mk::format(LE_I18N("Can't access file or directory '%s' (%s)"),
                           fileObject.srcPath,
                           strerror(err))
            );
        }
        // If the source is not a directory,
        else
        {
            fileObject.parseTreePtr->ThrowException(
                mk::format(LE_I18N("Not a directory: '%s'."), fileObject.srcPath)
            );
        }
    }

    // Loop over list of directory contents.
    for (;;)
    {
        // Setting errno so as to be able to detect errors from end of directory
        // (as recommended in the documentation).
        errno = 0;

        // Read an entry from the directory.
        struct dirent* entryPtr = readdir(dir);

        if (entryPtr == NULL)
        {
            if (errno != 0)
            {
                throw mk::Exception_t(
                    mk::format(LE_I18N("Internal error: readdir() failed.  Errno = %s"),
                               strerror(errno))
                );
            }
            else
            {
                // Hit end of the directory.  Nothing more to do.
                break;
            }
        }
        // Skip "." and ".."
        else if ((strcmp(entryPtr->d_name, ".") != 0) && (strcmp(entryPtr->d_name, "..") != 0))
        {
            auto entrySrcPath = path::Combine(fileObject.srcPath, entryPtr->d_name);
            auto entryDestPath = path::Combine(fileObject.destPath, entryPtr->d_name);

            // If this is a directory, then recursively descend into it.
            if (file::DirectoryExists(entrySrcPath))
            {
                GenerateDirBundleBuildStatements(script,
                                                 model::FileSystemObject_t(entrySrcPath,
                                                                           entryDestPath,
                                                                           fileObject.permissions,
                                                                           &fileObject),
                                                 bundledFiles);
            }
            // If this is a file, create a build statement for it.
            else if (file::FileExists(entrySrcPath))
            {
                GenerateFileBundleBuildStatement(script,
                                                 model::FileSystemObject_t(entrySrcPath,
                                                                           entryDestPath,
                                                                           fileObject.permissions,
                                                                           &fileObject),
                                                 bundledFiles);
            }
            // If this is anything else, we don't support it.
            else
            {
                fileObject.parseTreePtr->ThrowException(
                    mk::format(LE_I18N("File system object is not a directory or a file: '%s'."),
                               entrySrcPath)
                );
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
    model::FileSystemObjectSet_t& bundledFiles, ///< Set to fill with bundled file paths.
    const model::App_t* appPtr, ///< App to bundle the file into.
    const model::FileSystemObject_t* fileSystemObjPtr,  ///< File bundling info.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
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
                                     model::FileSystemObject_t(fileSystemObjPtr->srcPath,
                                                               destPath.str,
                                                               fileSystemObjPtr->permissions,
                                                               fileSystemObjPtr),
                                     bundledFiles);
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
    model::FileSystemObjectSet_t& bundledFiles, ///< Set to fill with bundled file paths.
    const model::App_t* appPtr, ///< App to bundle the directory into.
    const model::FileSystemObject_t* fileSystemObjPtr,  ///< Directory bundling info.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
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
                                     model::FileSystemObject_t(fileSystemObjPtr->srcPath,
                                                               destPath.str,
                                                               fileSystemObjPtr->permissions,
                                                               fileSystemObjPtr),
                                     bundledFiles);
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
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto& allBundledFiles = appPtr->getTargetInfo<target::FileSystemAppInfo_t>()->allBundledFiles;

    // Start with the application's list of bundled items first, so they override any items
    // bundled by components.
    // NOTE: Source paths for bundled items are always absolute.
    for (auto fileSystemObjPtr : appPtr->bundledFiles)
    {
        GenerateFileBundleBuildStatement(script,
                                         allBundledFiles,
                                         appPtr,
                                         fileSystemObjPtr,
                                         buildParams);
    }
    for (auto fileSystemObjPtr : appPtr->bundledDirs)
    {
        GenerateDirBundleBuildStatements(script,
                                         allBundledFiles,
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
                                             allBundledFiles,
                                             appPtr,
                                             fileSystemObjPtr,
                                             buildParams);
        }
        for (auto fileSystemObjPtr : componentPtr->bundledDirs)
        {
            GenerateDirBundleBuildStatements(script,
                                             allBundledFiles,
                                             appPtr,
                                             fileSystemObjPtr,
                                             buildParams);
        }

        // Generate a statement for bundling a component library into an application, if it has
        // a component library (which will only be the case if the component has sources).
        if ((componentPtr->HasCOrCppCode()) || (componentPtr->HasJavaCode()))
        {
            auto destPath = "$builddir/" + appPtr->workingDir
                          + "/staging/read-only/lib/" +
                path::GetLastNode(componentPtr->getTargetInfo<target::LinuxComponentInfo_t>()->lib);
            auto lib = componentPtr->getTargetInfo<target::LinuxComponentInfo_t>()->lib;

            // Copy the component library into the app's lib directory.
            // Cannot use hard link as this will cause builds to fail occasionally (LE-7383)
            script << "build " << destPath << " : CopyFile "
                   << lib << "\n\n";

            // Add the component library to the set of bundled files.
            allBundledFiles.insert(model::FileSystemObject_t(
                                       lib,
                                       destPath,
                                       model::Permissions_t(true,
                                                            false,
                                                            componentPtr->HasCOrCppCode())));
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
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams,
    const std::string& outputDir    ///< Path to the directory into which the built app will be put.
)
//--------------------------------------------------------------------------------------------------
{
    // Give this a FS target info
    appPtr->setTargetInfo(new target::FileSystemAppInfo_t());

    // Generate build statements for bundling files into the staging area.
    GenerateStagingBundleBuildStatements(script,
                                         appPtr,
                                         buildParams);

    // Compute the staging directory path.
    auto stagingDir = "$builddir/" + path::Combine(appPtr->workingDir, "staging");

    // Compute the info.properties file path.
    auto infoPropertiesPath = stagingDir + "/info.properties";

    // Generate build statement for generating the info.properties file.
    script << "build " << infoPropertiesPath << " : MakeAppInfoProperties |";

    // This depends on all the bundled files and executables in the app.
    for (auto filePath : appPtr->getTargetInfo<target::FileSystemAppInfo_t>()->allBundledFiles)
    {
        script << " " << filePath.destPath;
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

    // Tell the build rule what the app's name and version are and where its working directory
    // is.
    script << "  name = " << appPtr->name << "\n"
        "  version = " << appPtr->version << "\n"
        "  workingDir = $builddir/" + appPtr->workingDir << "\n"
        "\n";

    // Are we building a binary app package as well?
    if (buildParams.binPack)
    {
        const std::string appPackDir = "$builddir/" + appPtr->name;
        const std::string interfacesDir = appPackDir + "/interfaces";

        // We need to copy all the included .api files into the pack directory, so generate rules to
        // do this.
        for (auto apiFile : model::ApiFile_t::GetApiFileMap())
        {
            script << "build " << interfacesDir << "/" << path::GetLastNode(apiFile.second->path)
                   << ": CopyFile " << apiFile.second->path << "\n"
                      "\n";
        }

        // Now, copy all of the app files into the pack directory, and get it packed up as our final
        // output.
        auto outputFile = path::Combine(outputDir, appPtr->name) + ".$target.app";
        script << "build " << outputFile << ": BinPackApp " << infoPropertiesPath;

        if (!model::ApiFile_t::GetApiFileMap().empty())
        {
            script << " ||";

            for (auto apiFile : model::ApiFile_t::GetApiFileMap())
            {
                script << " " << interfacesDir << "/" << path::GetLastNode(apiFile.second->path);
            }
        }

        script << "\n"
                  "  stagingDir = $builddir/" << appPtr->workingDir << "/staging" << "\n"
                  "  workingDir = " << appPackDir << "\n"
                  "\n";


    }
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
    model::App_t* appPtr,
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
    includes = " -I " + buildParams.workingDir;
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
    GenerateBuildRules(script, buildParams, argc, argv);
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
