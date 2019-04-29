//--------------------------------------------------------------------------------------------------
/**
 * @file linuxSystemBuildScript.cpp
 *
 * Implementation of the build script generator for Linux-based systems.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "linuxSystemBuildScript.h"

//--------------------------------------------------------------------------------------------------
/**
 * Constant to use as a symlink target (in place of MD5-based directory name), when the app is
 * preloaded and its version may not match the latest version.
 */
//--------------------------------------------------------------------------------------------------
#define PRELOADED_ANY_VERSION "PRELOADED_ANY_VERSION"

namespace ninja
{

//--------------------------------------------------------------------------------------------------
/**
 * Generate system-specific build rules.
 */
//--------------------------------------------------------------------------------------------------
void LinuxSystemBuildScriptGenerator_t::GenerateSystemBuildRules
(
    model::System_t* systemPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Generate build rule for creating system info.properties.
    // This must be run if any of the apps have changed (which will show up in a change to their
    // info.properties) or if the users.cfg has changed.
    script <<
    "rule MakeSystemInfoProperties\n"
    "  description = Creating system info.properties\n"
    "  command = $\n"

    // Copy the framework bin and lib directories into the system's staging area.
    "            mkdir -p $stagingDir/bin && $\n"
    "            mkdir -p $stagingDir/lib && $\n"
    "            find $$LEGATO_ROOT/build/$target/framework/bin/* -type d -prune -o"
                                  " -print | xargs cp -P -t $stagingDir/bin && $\n"
    "            find $$LEGATO_ROOT/build/$target/framework/lib/* -type d -prune -o"
                       " \\( -type f -o -type l \\) -print | xargs cp -P -t $stagingDir/lib && $\n"

    // Copy liblegato Python files into the system's staging area.
    // cd in a subshell is used because it allows us to use the --parents option to recreate the
    // relative directory structure inside $stagingDir. Without cd, --parents would reproduce the
    // full path relative to LEGATO_ROOT, while we want it relative to lib/
    "            (cd $$LEGATO_ROOT/build/$target/framework/lib/ ; "
    "             find . -path './*/site-packages/*'"
    "             -exec cp -P --parents -t $stagingDir/lib/ {} \\; ; ) && $\n"

    // Create modules directory and copy kernel modules into it
    "            mkdir -p $stagingDir/modules && $\n"
    "            if [ -d $builddir/modules ] ; then $\n"
    "                find $builddir/modules/*/*.ko -print"
                          "| xargs cp -P -t $stagingDir/modules ; $\n"
    "            fi && $\n"

    // Create an apps directory for the symlinks to the apps.
    "            mkdir -p $stagingDir/apps && $\n";

    // Copy the link libComponents into the staging directory
    for (auto &linkEntry : systemPtr->links)
    {
        const auto componentPtr = linkEntry.second->componentPtr;
        script << "            "
               << "legato-install -m 775 "
               << componentPtr->GetTargetInfo<target::LinuxComponentInfo_t>()->lib
               << " $stagingDir/lib/libComponent_"
               << componentPtr->name + ".so" << " ; $\n";
    }

    // Create symlinks inside the system's "apps" directory that point to the apps actual
    // install location on target (under /legato/apps/).
    for (auto& mapEntry : systemPtr->apps)
    {
        auto appPtr = mapEntry.second;
        std::string appInfoFile = "$builddir/app/" + appPtr->name + "/staging/info.properties";
        std::string symLink = "$stagingDir/apps/" + appPtr->name;

        // If the app is "preloaded" on the target and its MD5 is specified in the .sdef file,
        // then "hard code" the specified MD5 into the build script.
        // Otherwise, extract the app's MD5 hash from the info.properties file.
        if (appPtr->preloadedMode == model::App_t::SPECIFIC_MD5 && !appPtr->preloadedMd5.empty())
        {
            script <<
            "            md5=" << appPtr->preloadedMd5 << " && $\n";
        }
        else if (appPtr->preloadedMode == model::App_t::ANY_VERSION)
        {
            script <<
            "            md5=" << PRELOADED_ANY_VERSION << " && $\n";
        }
        else // NONE, BUILD_VERSION
        {
            script <<
            "            md5=`grep '^app.md5=' " << appInfoFile << " | sed 's/^app.md5=//'` && $\n";
        }

        // Add a symlink to /legato/apps/$HASH from staging/system/apps/appName.
        script <<
        "            ln -sf /legato/apps/$$md5 " << symLink << " && $\n";
    }

    // Create a wrapper that is going to execute shell commands specified in the .sdef file's "commands:"
    // section through 'app runProc'.
    for (const auto& mapEntry : systemPtr->commands)
    {
        const auto commandPtr = mapEntry.second;

        std::string exePath = commandPtr->exePath;
        if (exePath[0] == '/')
        {
            exePath.erase(0, 1);
        }

        script <<
        "            ( echo '#!/bin/sh' && $\n"
        "              echo 'exec /legato/systems/current/bin/app runProc "
                                            << commandPtr->appPtr->name
                                            << " --exe=" << exePath
                                            << " -- \"$$@\"' ) > $stagingDir/bin/" << commandPtr->name << " && $\n";
        script <<
        "            chmod +x $stagingDir/bin/" << commandPtr->name << " && $\n";
    }

    script <<
    // Delete the old info.properties file, if there is one.
    "            rm -f $out && $\n"

    // Compute the MD5 checksum of the staging area.
    // Don't follow symlinks (-P), and include the directory structure and the contents of symlinks
    // as part of the MD5 hash.
    "            md5=$$( ( cd $stagingDir && $\n"
    "                      find -P -print0 |LC_ALL=C sort -z && $\n"
    "                      find -P -type f -print0 |LC_ALL=C sort -z |xargs -0 md5sum && $\n"
    "                      find -P -type l -print0 |LC_ALL=C sort -z |xargs -0 -r -n 1 readlink $\n"
    "                    ) |tee /proc/self/fd/2 | md5sum) && $\n"
    "            md5=$${md5%% *} && $\n"

    // Get the Legato framework version and append the MD5 sum to it to get the system version.
    "           frameworkVersion=$$( cat $$LEGATO_ROOT/version ) && $\n"
    "           version=$$( printf '%s_%s' \"$$frameworkVersion\" \"$$md5\" ) && $\n"

    // Generate the system's info.properties file.
    "            ( echo \"system.name=" << systemPtr->name << "\" && $\n"
    "              echo \"system.md5=$$md5\" $\n"
    "            ) > $out && $\n"
    // Generate the system's version file
    "            printf '%s\\n' \"$$version\" > $stagingDir/version\n"
    "\n";

    // New generate the unsigned system package
    // $out is the system update file to generate.
    // $in is a list of all the app update packs to include in the system.
    script <<
    "rule PackSystem\n"
    "  description = Packaging system\n"
    "  command = $\n"
    // Change all file time stamp to generate reproducible build. Can't use gnu tar --mtime option
    // as it is not available in other tar (e.g bsdtar)
    "            mtime=`stat -c %Y " << systemPtr->defFilePtr->path <<"` && $\n"
    "            find $stagingDir -exec touch  --no-dereference --date=@$$mtime {} \\; && $\n"
    // Pack the system's staging area into a compressed tarball.
    "           (cd $stagingDir && find . -print0 | LC_ALL=C sort -z"
                                 " |tar --no-recursion --null -T -"
                                 " -cjf - ) > $builddir/"<< systemPtr->name <<".$target && $\n"

    // Get the size of the tarball.
    "            tarballSize=`stat -c '%s' $builddir/" << systemPtr->name << ".$target` && $\n"

    // Get the app's MD5 hash from its info.properties file.
    "            md5=`grep '^system.md5=' $stagingDir/info.properties | "
                                                                    "sed 's/^system.md5=//'` && $\n"
    // Generate a JSON header and concatenate the tarball and all the app update packs to it
    // to create the system update pack.
    "            ( printf '{\\n' && $\n"
    "              printf '\"command\":\"updateSystem\",\\n' && $\n"
    "              printf '\"md5\":\"%s\",\\n' \"$$md5\" && $\n"
    "              printf '\"size\":%s\\n' \"$$tarballSize\" && $\n"
    "              printf '}' && $\n"
    "              cat $builddir/" << systemPtr->name << ".$target && $\n"
    "              cat $in $\n"
    "            ) > $out\n"
    "\n";

    if (buildParams.signPkg)
    {

        // Generate the signed system update package
        script <<
        "rule PackSignedSystem\n"
        "  description = Signing and packaging system\n"
        "  command = rm -rf $stagingDir.signed ; mkdir $stagingDir.signed && "
                    "cp -r $stagingDir/* $stagingDir.signed/ && $\n"
        "            cp " << buildParams.pubCert << " $stagingDir.signed/ima_pub.cert  && $\n"
        // Remove the contents inside apps directory as we need to recreate the symlinks again
        "            rm -rf $stagingDir.signed/apps/* && $\n";

        // Create symlinks inside the system's "apps" directory that point to the apps actual
        // install location on target (under /legato/apps/).
        for (auto& mapEntry : systemPtr->apps)
        {
            auto appPtr = mapEntry.second;
            std::string appInfoFile = "$builddir/app/" + appPtr->name +
                                       "/staging.signed/info.properties";
            std::string symLink = "$stagingDir.signed/apps/" + appPtr->name;

            // If the app is "preloaded" on the target and its MD5 is specified in the .sdef file,
            // then "hard code" the specified MD5 into the build script.
            // Otherwise, extract the app's MD5 hash from the info.properties file.
            if (appPtr->preloadedMode == model::App_t::SPECIFIC_MD5 && !appPtr->preloadedMd5.empty())
            {
                script <<
                "            md5=" << appPtr->preloadedMd5 << " && $\n";
            }
            else if (appPtr->preloadedMode == model::App_t::ANY_VERSION)
            {
                script <<
                "            md5=" << PRELOADED_ANY_VERSION << " && $\n";
            }
            else // NONE, BUILD_VERSION
            {
                script <<
                "            md5=`grep '^app.md5=' " << appInfoFile
                             << " | sed 's/^app.md5=//'` && $\n";
            }

            // Add a symlink to /legato/apps/$HASH from staging/system/apps/appName.
            script <<
            "            ln -sf /legato/apps/$$md5 " << symLink << " && $\n";
        }

        script <<
        // Recompute the MD5 checksum of the staging area.
        // Don't follow symlinks (-P), and include the directory structure and the contents
        // of symlinks as part of the MD5 hash.
        "            md5signed=$$( ( cd $stagingDir.signed && $\n"
        "                      find -P -print0 |LC_ALL=C sort -z && $\n"
        "                      find -P -type f -print0 |LC_ALL=C sort -z |xargs -0 md5sum && $\n"
        "                      find -P -type l -print0 |LC_ALL=C sort -z "
                               "|xargs -0 -r -n 1 readlink $\n"
        "                    ) | md5sum) && $\n"
        "            md5signed=$${md5signed%% *} && $\n"
        // Get the systems's MD5 hash from its info.properties file and replace with signed one
        "            md5=`grep '^system.md5=' $stagingDir.signed/info.properties | "
                                                          "sed 's/^system.md5=//'` && $\n"
        "            sed -i \"s/$$md5/$$md5signed/g\" $stagingDir.signed/info.properties && $\n"
        // Change all file time stamp to generate reproducible build. Can't use gnu tar --mtime
        // option as it is not available in other tar (e.g bsdtar)
        "            mtime=`stat -c %Y " << systemPtr->defFilePtr->path <<"` && $\n"
        "            find $stagingDir.signed -exec touch  --no-dereference "
                    "--date=@$$mtime {} \\; && $\n"
        // No need to recompute the md5 hash again as it is used for app/system version and
        // enable signing shouldn't change the app/system version.
        // Require signing image. Sign the staging area and create tarball
        "            "<< baseGeneratorPtr->GetPathEnvVarDecl() << " && $\n"
        "            fakeroot ima-sign.sh --sign -y legato -d $stagingDir.signed -t $builddir/"
        << systemPtr->name << ".signed.$target" << " -p "  << buildParams.privKey << " && $\n" <<
        // Get the size of the tarball.
        "            tarballSize=`stat -c '%s' $builddir/" << systemPtr->name
        << ".signed.$target` && $\n"

        // Get the app's MD5 hash from its info.properties file.
        "            md5=`grep '^system.md5=' $stagingDir.signed/info.properties | "
                                                          "sed 's/^system.md5=//'` && $\n"
        // Generate a JSON header and concatenate the tarball and all the app update packs to it
        // to create the system update pack.
        "            ( printf '{\\n' && $\n"
        "              printf '\"command\":\"updateSystem\",\\n' && $\n"
        "              printf '\"md5\":\"%s\",\\n' \"$$md5signed\" && $\n"
        "              printf '\"size\":%s\\n' \"$$tarballSize\" && $\n"
        "              printf '}' && $\n"
        "              cat $builddir/" << systemPtr->name << ".signed.$target && $\n"
        "              cat $in $\n"
        "            ) > $out\n"
        "\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate config tree references for exported RPC services
 */
//--------------------------------------------------------------------------------------------------
template<class T>
void LinuxSystemBuildScriptGenerator_t::GenerateRpcCfgReferences
(
    const std::map<std::string, T> &externInterfaces,
    std::set<std::string> &rpcCfgRefs
)
{
    for (auto &apiRef : externInterfaces)
    {
        std::string apiRefFile = apiRef.second->ifPtr->GetRpcReferenceFile();

        // If a build rule has already been generated for this file; skip it.
        if (rpcCfgRefs.find(apiRefFile) != rpcCfgRefs.end())
        {
            continue;
        }

        script << "build  $builddir/" << apiRefFile << ": GenInterfaceCode "
               << apiRef.second->ifPtr->apiFilePtr->path << " |";
        baseGeneratorPtr->GenerateIncludedApis(apiRef.second->ifPtr->apiFilePtr);
        script <<
            "\n"
            "  outputDir = $builddir/" << path::GetContainingDir(apiRefFile) << "\n"
            "  ifgenFlags = --lang Cfg --service-name " << apiRef.second->ifPtr->internalName <<
            " --gen-rpc-reference\n"
            "\n";

        rpcCfgRefs.insert(apiRefFile);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script the build statements for packing up everything into a system udpate pack.
 **/
//--------------------------------------------------------------------------------------------------
void LinuxSystemBuildScriptGenerator_t::GenerateSystemPackBuildStatement
(
    model::System_t* systemPtr
)
//--------------------------------------------------------------------------------------------------
{
    // On Linux, first construct framework.cfg from framework.cfg.in and the references for
    // the individual RPC references

    std::set<std::string> rpcCfgRefs;

    GenerateRpcCfgReferences(systemPtr->externServerInterfaces, rpcCfgRefs);
    GenerateRpcCfgReferences(systemPtr->externClientInterfaces, rpcCfgRefs);

    script <<
        "build $stagingDir/config/framework.cfg :"
        " ProcessConfig $builddir/config/framework.cfg.in |";

    for (auto rpcCfgRef : rpcCfgRefs)
    {
        script << " $builddir/" << rpcCfgRef;
    }

    script << "\n\n";

    // Generate build statement for zipping up the staging area into a system bundle.
    // Build the system staging area by adding framework binaries, app symlink and generate
    // system info.properties file which is the last thing to be added to the system staging dir.

    // Compute the info.properties file path.
    std::string infoPropertiesPath = "$stagingDir/info.properties";

    // Generate build statement for generating the info.properties file.
    script << "build " << infoPropertiesPath << " : MakeSystemInfoProperties |";

    // Input to this is the app update packs to be included in the system update pack.
    std::string sysAppsUpdates = " ";

    for (auto& mapEntry : systemPtr->apps)
    {
        auto appPtr = mapEntry.second;

        if (appPtr->preloadedMode == model::App_t::NONE)
        {
            auto& appName = appPtr->name;

            sysAppsUpdates += " $builddir/app/" + appName + "/" + appName + ".$target.update";
        }
    }

    script << sysAppsUpdates;

    // This also must be run if any system config tree has changed.
    script <<
        " $builddir/staging/config/users.cfg"
        " $builddir/staging/config/apps.cfg"
        " $builddir/staging/config/modules.cfg"
        " $builddir/staging/config/framework.cfg";

    // It must also be run again if any preloaded apps have changed.
    for (auto& mapEntry : systemPtr->apps)
    {
        auto appPtr = mapEntry.second;

        if (appPtr->preloadedMode == model::App_t::BUILD_VERSION)
        {
            auto& appName = appPtr->name;

            script << " $builddir/app/" << appName << "/" << appName << ".$target.update";
        }
    }

    // Also recompute info.properties if any module binaries changed.
    for (auto& mapEntry : systemPtr->modules)
    {
        auto modulePtr = mapEntry.second.modPtr;

        for (auto const& it: modulePtr->koFiles)
        {
            script << " $builddir/" << it.second->path;
        }
        for (auto filePath : modulePtr->GetTargetInfo<target::FileSystemInfo_t>()->allBundledFiles)
        {
            script << " " << filePath.destPath;
        }
    }

    // This must also be done again if any of the Legato framework daemons or on-target
    // tools has changed.  We can detect that by checking the "md5" file in the framework's
    // build directory.
    script << " " << path::Combine(envVars::Get("LEGATO_ROOT"), "build/$target/framework/md5")
           << "\n"
           << "\n";

    // Now package the system
    auto outputFile = path::MakeAbsolute(path::Combine(buildParams.outputDir,
                                                       systemPtr->name + ".$target.update"));
    script << "build " << outputFile << ": PackSystem " << sysAppsUpdates << " | "
           << infoPropertiesPath << "\n"
           "\n";

    if (buildParams.signPkg)
    {
        // Now create the signed system package.
        auto outputFileSigned = path::MakeAbsolute(path::Combine(buildParams.outputDir,
                                                                 systemPtr->name +
                                                                 ".$target.signed.update"));
        script << "build " << outputFileSigned << ": PackSignedSystem";

        // Input to this is the app update packs to be included in the system update pack.
        for (auto& mapEntry : systemPtr->apps)
        {
            auto appPtr = mapEntry.second;

            if (appPtr->preloadedMode == model::App_t::NONE)
            {
                auto& appName = appPtr->name;

                script << " $builddir/app/" << appName << "/" << appName
                       << ".$target.signed.update";
            }
        }

        // This must be run if the public certificate has been changed. No need to include private
        // key in dependency as public and private key come in pair. Must run if system staging
        // directory info.properties has been changed.
        script << " | "  << buildParams.pubCert << " " << infoPropertiesPath << "\n"
               "\n";

        // No need to check kernel module, cfg file, preloaded app  or md5 change as it is already
        // taken care by while generating info.properties.

    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate build script for the system.
 **/
//--------------------------------------------------------------------------------------------------
void GenerateLinux
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Minimize(buildParams.workingDir + "/build.ninja");

    LinuxSystemBuildScriptGenerator_t systemGenerator(filePath, buildParams);

    systemGenerator.Generate(systemPtr);
}

} // namespace ninja
