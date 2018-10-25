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
void AppBuildScriptGenerator_t::GenerateCommentHeader
(
    model::App_t* appPtr
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
void AppBuildScriptGenerator_t::GenerateAppBuildRules
(
    void
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
        "                      find -P -print0 |LC_ALL=C sort -z && $\n"
        "                      find -P -type f -print0 |LC_ALL=C sort -z |xargs -0 md5sum && $\n"
        "                      find -P -type l -print0 |LC_ALL=C sort -z"
                             " |xargs -0 -r -n 1 readlink $\n"
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

        "  command = $\n"
        // Change all file time stamp to generate reproducible build. Can't use gnu tar --mtime
        // option as it is not available in other tar (e.g bsdtar)
        "            mtime=`stat -c %Y $adefPath` && $\n"
        "            find $workingDir/staging -exec touch --no-dereference "
                    "--date=@$$mtime {} \\; && $\n"
        "            (cd $workingDir/staging && find . -print0 | LC_ALL=C sort -z"
                     " |tar --no-recursion --null -T - -cjf - ) > $workingDir/$name.$target && $\n"
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
        // Change all file time stamp to generate reproducible build. Can't use gnu tar --mtime
        // option as it is not available in other tar (e.g bsdtar)
        "            mtime=`stat -c %Y $adefPath` && $\n"
        "            find $workingDir -exec touch  --no-dereference --date=@$$mtime {} \\; && $\n"
        "            (cd $workingDir/ && find . -print0 |LC_ALL=C sort -z"
        "  |tar --no-recursion --null -T - -cjf - ) > $out\n"
        "\n";

    if (buildParams.signPkg)
    {
        script <<
            // Create an update pack file for an app.
            "rule PackSignedApp\n"
            "  description = Signing and packaging app\n"
            // Require signing image. Create a separate staging area and compute its md5 hash
            "  command = rm -rf $workingDir/staging.signed ; mkdir $workingDir/staging.signed && "
                        "cp -r $workingDir/staging/* $workingDir/staging.signed/ && $\n"
            // No need to recompute the md5 hash again as it is used for app version and enable
            // signing shouldn't change the app version.
            "            cp " << buildParams.pubCert <<
                        " $workingDir/staging.signed/ima_pub.cert  && $\n"
            // Recompute the MD5 checksum of the staging area.
            // Don't follow symlinks (-P), and include the directory structure and the contents
            // of symlinks as part of the MD5 hash.
            "            md5signed=$$( ( cd $workingDir/staging.signed && $\n"
            "                      find -P -print0 |LC_ALL=C sort -z && $\n"
            "                      find -P -type f -print0 |LC_ALL=C sort -z |xargs -0 md5sum && $\n"
            "                      find -P -type l -print0 |LC_ALL=C sort -z"
                                 " |xargs -0 -r -n 1 readlink $\n"
            "                    ) | md5sum) && $\n"
            "            md5signed=$${md5signed%% *} && $\n"
            // Get the app's MD5 hash from its info.properties file and replace with signed one.
            "            md5=`grep '^app.md5=' $workingDir/staging.signed/info.properties"
                        " | sed 's/^app.md5=//'` && $\n"
            "            sed -i \"s/$$md5/$$md5signed/g\" "
                                                "$workingDir/staging.signed/info.properties && $\n"
            // Change all file time stamp to generate reproducible build. Can't use gnu tar --mtime
            // option as it is not available in other tar (e.g bsdtar)
            "            mtime=`stat -c %Y $adefPath` && $\n"
            "            find $workingDir/staging.signed -exec touch --no-dereference "
                        "--date=@$$mtime {} \\; && $\n"
            "            "<< baseGeneratorPtr->GetPathEnvVarDecl() << " && $\n"
            "            fakeroot ima-sign.sh --sign -y legato -d $workingDir/staging.signed "
                        "-t $workingDir/$name.$target.signed -p " << buildParams.privKey <<" && $\n"
            // Get the size of the tarball.
            "            tarballSize=`stat -c '%s' $workingDir/$name.$target.signed` && $\n"
            // Get the app's MD5 hash from its info.properties file.
            "            md5=`grep '^app.md5=' $workingDir/staging.signed/info.properties"
                        " | sed 's/^app.md5=//'` && $\n"
            // Generate a JSON header and concatenate the tarball to it to create the update pack.
            "            ( printf '{\\n' && $\n"
            "              printf '\"command\":\"updateApp\",\\n' && $\n"
            "              printf '\"name\":\"$name\",\\n' && $\n"
            "              printf '\"version\":\"$version\",\\n' && $\n"
            "              printf '\"md5\":\"%s\",\\n' \"$$md5signed\" && $\n"
            "              printf '\"size\":%s\\n' \"$$tarballSize\" && $\n"
            "              printf '}' && $\n"
            "              cat $workingDir/$name.$target.signed $\n"
            "            ) > $out\n"
            "\n"

            "rule BinPackSignedApp\n"
            "  description = Signing and packaging app for distribution.\n"
            "  command = rm -rf $workingDir/staging.signed.bin ; "
                        "mkdir $workingDir/staging.signed.bin "
                        "&& cp -r $stagingDir/* $workingDir/staging.signed.bin/ && $\n"
            "            cp " <<  buildParams.pubCert  << " " <<
                        "$workingDir/staging.signed.bin/ima_pub.cert  && $\n" <<
            // Change all file time stamp to generate reproducible build. Can't use gnu tar --mtime
            // option as it is not available in other tar (e.g bsdtar)
            "            mtime=`stat -c %Y $adefPath` && $\n"
            "            find $workingDir/staging.signed.bin -exec touch --no-dereference "
                        "--date=@$$mtime {} \\; && $\n"
            // Require signing image. Sign the staging area and create tarball
            "            "<< baseGeneratorPtr->GetPathEnvVarDecl() << " && $\n"
            "            fakeroot ima-sign.sh --sign -y legato -d $workingDir/staging.signed.bin/ "
                        "-t $out -p " << buildParams.privKey << "\n"
            "\n";
    }

}


//--------------------------------------------------------------------------------------------------
/**
 * Generates build statements for all the executables in a given app.
 */
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateExeBuildStatements
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto mapItem : appPtr->executables)
    {
        exeGeneratorPtr->GenerateBuildStatements(mapItem.second);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statement for bundling a single file into
 * the staging area.
 **/
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateFileBundleBuildStatement
(
    model::FileSystemObjectSet_t& bundledFiles, ///< Set to fill with bundled file paths.
    model::App_t* appPtr, ///< App to bundle the file into.
    const model::FileSystemObject_t* fileSystemObjPtr  ///< File bundling info.
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

    baseGeneratorPtr->GenerateFileBundleBuildStatement(model::FileSystemObject_t(
                                                               fileSystemObjPtr->srcPath,
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
void AppBuildScriptGenerator_t::GenerateDirBundleBuildStatements
(
    model::FileSystemObjectSet_t& bundledFiles, ///< Set to fill with bundled file paths.
    model::App_t* appPtr, ///< App to bundle the directory into.
    const model::FileSystemObject_t* fileSystemObjPtr  ///< Directory bundling info.
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

    baseGeneratorPtr->GenerateDirBundleBuildStatements(model::FileSystemObject_t(
                                                               fileSystemObjPtr->srcPath,
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
void AppBuildScriptGenerator_t::GenerateStagingBundleBuildStatements
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto& allBundledFiles = appPtr->GetTargetInfo<target::FileSystemInfo_t>()->allBundledFiles;

    // Start with the application's list of bundled items first, so they override any items
    // bundled by components.
    // NOTE: Source paths for bundled items are always absolute.
    for (auto fileSystemObjPtr : appPtr->bundledFiles)
    {
        GenerateFileBundleBuildStatement(allBundledFiles,
                                         appPtr,
                                         fileSystemObjPtr.get());
    }
    for (auto fileSystemObjPtr : appPtr->bundledDirs)
    {
        GenerateDirBundleBuildStatements(allBundledFiles,
                                         appPtr,
                                         fileSystemObjPtr.get());
    }
    for (auto fileSystemObjPtr : appPtr->bundledBinaries)
    {
        GenerateFileBundleBuildStatement(allBundledFiles,
                                         appPtr,
                                         fileSystemObjPtr.get());
    }

    // Now do the same for each component in the app.
    for (auto componentPtr : appPtr->components)
    {
        for (auto fileSystemObjPtr : componentPtr->bundledFiles)
        {
            GenerateFileBundleBuildStatement(allBundledFiles,
                                             appPtr,
                                             fileSystemObjPtr.get());
        }
        for (auto fileSystemObjPtr : componentPtr->bundledDirs)
        {
            GenerateDirBundleBuildStatements(allBundledFiles,
                                             appPtr,
                                             fileSystemObjPtr.get());
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script the build statements for packing up everything into an application
 * bundle.
 **/
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateAppBundleBuildStatement
(
    model::App_t* appPtr,
    const std::string& outputDir    ///< Path to the directory into which the built app will be put.
)
//--------------------------------------------------------------------------------------------------
{
    // Give this a FS target info
    appPtr->SetTargetInfo(new target::FileSystemInfo_t());

    // Generate build statements for bundling files into the staging area.
    GenerateStagingBundleBuildStatements(appPtr);

    // Compute the staging directory path.
    auto stagingDir = "$builddir/" + path::Combine(appPtr->workingDir, "staging");

    // Compute the info.properties file path.
    auto infoPropertiesPath = stagingDir + "/info.properties";

    // Generate build statement for generating the info.properties file.
    script << "build " << infoPropertiesPath << " : MakeAppInfoProperties |";

    // This depends on all the bundled files and executables in the app.
    for (auto filePath : appPtr->GetTargetInfo<target::FileSystemInfo_t>()->allBundledFiles)
    {
        script << " " << filePath.destPath;
    }
    for (auto mapItem : appPtr->executables)
    {
        auto exeName = mapItem.second->name;
        if (mapItem.second->hasJavaCode)
        {
            exeName += ".jar";
        }
        script << " $builddir/" << appPtr->workingDir
               << "/staging/read-only/bin/" << exeName;
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
        "  adefPath = " << appPtr->defFilePtr->path << "\n"
        "  version = " << appPtr->version << "\n"
        "  workingDir = $builddir/" + appPtr->workingDir << "\n"
        "\n";

    if (buildParams.signPkg)
    {
      // No need to check the environment variable for keys again as it is already checked
      // after parsing mkapp input parameters.

      // Now create the signed app package. This should be build after unsigned app package
      // is built.
      auto outputFileSigned = path::Combine(outputDir, appPtr->name) + ".$target.signed.update";
      script << "build " << outputFileSigned << ": PackSignedApp " << infoPropertiesPath << " | "
      // No need to include private key in dependency as public and private key come in pair.
             << " "<< buildParams.pubCert << "\n";
      // Tell the build rule what the app's name and version are and where its working directory
      // is.
      script << "  name = " << appPtr->name << "\n"
          "  adefPath = " << appPtr->defFilePtr->path << "\n"
          "  version = " << appPtr->version << "\n"
          "  workingDir = $builddir/" + appPtr->workingDir << "\n"
          "\n";

    }
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
                  "  adefPath = " << appPtr->defFilePtr->path << "\n"
                  "  stagingDir = $builddir/" << appPtr->workingDir << "/staging" << "\n"
                  "  workingDir = " << appPackDir << "\n"
                  "\n";

        if (buildParams.signPkg)
        {
            // Now create the signed app package. This should be build after unsigned app package
            // is built.
            auto outputFileSigned = path::Combine(outputDir, appPtr->name) + ".$target.signed.app";
            script << "build " << outputFileSigned << ": BinPackSignedApp " << infoPropertiesPath
                   << " | " << " " << buildParams.pubCert << " " << outputFile << "\n"
                    "  adefPath = " << appPtr->defFilePtr->path << "\n"
                    "  stagingDir = " << appPackDir << "\n"
                    "  workingDir = " << "$builddir/" << "\n"
                    "\n";
        }

    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for the build script itself.
 **/
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateNinjaScriptBuildStatement
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
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

    baseGeneratorPtr->GenerateNinjaScriptBuildStatement(dependencies);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate all build rules required for building an application.
 */
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::GenerateBuildRules
(
    void
)
{
    exeGeneratorPtr->GenerateBuildRules();
    GenerateAppBuildRules();
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for an application.
 */
//--------------------------------------------------------------------------------------------------
void AppBuildScriptGenerator_t::Generate
(
    model::App_t* appPtr
)
{
    // Start the script with a comment, the file-level variable definitions, and
    // a set of generic rules.
    GenerateCommentHeader(appPtr);
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
    GenerateBuildRules();

    // For each component included in executables in this application, generate IPC code.
    for (auto componentPtr : appPtr->components)
    {
        componentGeneratorPtr->GenerateIpcBuildStatements(componentPtr);
    }

    // If we are not just generating code,
    if (!buildParams.codeGenOnly)
    {
        // For each component included in executables in this application.
        for (auto componentPtr : appPtr->components)
        {
            componentGeneratorPtr->GenerateBuildStatementsRecursive(componentPtr);
        }

        // For each executable built by the mk tools for this application,
        GenerateExeBuildStatements(appPtr);

        // Generate build statement for packing everything into an application bundle.
        GenerateAppBundleBuildStatement(appPtr, buildParams.outputDir);
    }

    // Add a build statement for the build.ninja file itself.
    GenerateNinjaScriptBuildStatement(appPtr);
}


} // namespace ninja
