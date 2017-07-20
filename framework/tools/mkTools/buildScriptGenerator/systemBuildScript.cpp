//--------------------------------------------------------------------------------------------------
/**
 * @file systemBuildScript.cpp
 *
 * Implementation of the build script generator for systems.
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
#include "moduleBuildScript.h"
#include "componentBuildScript.h"
#include "systemBuildScript.h"

namespace ninja
{



//--------------------------------------------------------------------------------------------------
/**
 * Generate comment header for a system build script.
 */
//--------------------------------------------------------------------------------------------------
void SystemBuildScriptGenerator_t::GenerateCommentHeader
(
    model::System_t* systemPtr
)
//--------------------------------------------------------------------------------------------------
{
    script  << "# Build script for system '" << systemPtr->name << "'\n"
            "\n"
            "# == Auto-generated file.  Do not edit. ==\n"
            "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate system-specific build rules.
 */
//--------------------------------------------------------------------------------------------------
void SystemBuildScriptGenerator_t::GenerateSystemBuildRules
(
    model::System_t* systemPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Generate build rule for creating a system update pack.
    // This must be run if any of the apps have changed (which will show up in a change to their
    // info.properties) or if the users.cfg has changed.
    // $out is the system update file to generate.
    // $in is a list of all the app update packs to include in the system.
    script <<
    "rule PackSystem\n"
    "  description = Packaging system\n"
    "  command = $\n"

    // Copy the framework bin and lib directories into the system's staging area.
    "            mkdir -p $stagingDir/bin && $\n"
    "            mkdir -p $stagingDir/lib && $\n"
    "            find $$LEGATO_ROOT/build/$target/framework/bin/* -type d -prune -o"
                                  " -print | xargs cp -P -t $stagingDir/bin && $\n"
    "            find $$LEGATO_ROOT/build/$target/framework/lib/* -type d -prune -o"
                       " \\( -type f -o -type l \\) -print | xargs cp -P -t $stagingDir/lib && $\n"

    // Create modules directory and copy kernel modules into it
    "            mkdir -p $stagingDir/modules && $\n"
    "            if [ -d $builddir/modules ] ; then $\n"
    "                find $builddir/modules/*/*.ko -print"
                          "| xargs cp -P -t $stagingDir/modules ; $\n"
    "            fi && $\n"

    // Create an apps directory for the symlinks to the apps.
    "            mkdir -p $stagingDir/apps && $\n";

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
        if (appPtr->isPreloaded && !appPtr->preloadedMd5.empty())
        {
            script <<
            "            md5=" << appPtr->preloadedMd5 << " && $\n";
        }
        else
        {
            script <<
            "            md5=`grep '^app.md5=' " << appInfoFile << " | sed 's/^app.md5=//'` && $\n";
        }

        // Add a symlink to /legato/apps/$HASH from staging/system/apps/appName.
        script <<
        "            ln -sf /legato/apps/$$md5 " << symLink << " && $\n";
    }

    // Create symlinks to all the shell commands specified in the .sdef file's "commands:" section.
    for (const auto& mapEntry : systemPtr->commands)
    {
        const auto commandPtr = mapEntry.second;

        std::string symLink = "../apps/"
                            + commandPtr->appPtr->name
                            + "/read-only"
                            + commandPtr->exePath;
        script <<
        "            ln -sf " << symLink << " $stagingDir/bin/" << commandPtr->name << " && $\n";
    }

    script <<
    // Delete the old info.properties file, if there is one.
    "            rm -f $stagingDir/info.properties && $\n"

    // Compute the MD5 checksum of the staging area.
    // Don't follow symlinks (-P), and include the directory structure and the contents of symlinks
    // as part of the MD5 hash.
    "            md5=$$( ( cd $stagingDir && $\n"
    "                      find -P | sort && $\n"
    "                      find -P -type f | sort | xargs cat && $\n"
    "                      find -P -type l | sort | xargs -r -n 1 readlink $\n"
    "                    ) | md5sum) && $\n"
    "            md5=$${md5%% *} && $\n"

    // Get the Legato framework version and append the MD5 sum to it to get the system version.
    "           frameworkVersion=$$( cat $$LEGATO_ROOT/version ) && $\n"
    "           version=$$( printf '%s_%s' \"$$frameworkVersion\" \"$$md5\" ) && $\n"

    // Generate the system's info.properties file.
    "            ( echo \"system.name=" << systemPtr->name << "\" && $\n"
    "              echo \"system.md5=$$md5\" $\n"
    "            ) > $stagingDir/info.properties && $\n"

    // Generate the system's version file
    "            printf '%s\\n' \"$$version\" > $stagingDir/version && $\n"

    // Pack the system's staging area into a compressed tarball in the working directory.
    "            tar cjf $builddir/" << systemPtr->name << ".$target -C $stagingDir . && $\n"

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
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script the build statements for packing up everything into a system udpate pack.
 **/
//--------------------------------------------------------------------------------------------------
void SystemBuildScriptGenerator_t::GenerateSystemPackBuildStatement
(
    model::System_t* systemPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Generate build statement for zipping up the staging area into a system bundle.
    // This depends on the system's info.properties file, which is the last thing to be added to
    // the system's staging area.
    auto outputFile = path::MakeAbsolute(path::Combine(buildParams.outputDir,
                                                       systemPtr->name + ".$target.update"));
    script << "build " << outputFile << ": PackSystem";

    // Input to this is the app update packs to be included in the system update pack.
    for (auto& mapEntry : systemPtr->apps)
    {
        auto appPtr = mapEntry.second;

        if (appPtr->isPreloaded == false)
        {
            auto& appName = appPtr->name;

            script << " $builddir/app/" << appName << "/" << appName << ".$target.update";
        }
    }

    // This also must be run if the users.cfg has changed.
    script << " | $builddir/staging/config/users.cfg";

    // It must also be run again if any preloaded apps have changed.
    for (auto& mapEntry : systemPtr->apps)
    {
        auto appPtr = mapEntry.second;

        if (appPtr->isPreloaded == true)
        {
            auto& appName = appPtr->name;

            script << " $builddir/app/" << appName << "/" << appName << ".$target.update";
        }
    }

    // Also re-package system if any module binaries changed.
    for (auto& mapEntry : systemPtr->modules)
    {
        auto modulePtr = mapEntry.second;

        script << " $builddir/" << modulePtr->koFilePtr->path;
    }

    // This must also be done again if any of the Legato framework daemons or on-target
    // tools has changed.  We can detect that by checking the "md5" file in the framework's
    // build directory.
    script << " " << path::Combine(envVars::Get("LEGATO_ROOT"), "build/$target/framework/md5")
           << "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for the build script itself.
 **/
//--------------------------------------------------------------------------------------------------
void SystemBuildScriptGenerator_t::GenerateNinjaScriptBuildStatement
(
    model::System_t* systemPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Create a set of dependencies.
    std::set<std::string> dependencies;

    // Add the .sdef file to the dependencies.
    dependencies.insert(systemPtr->defFilePtr->path);

    // For each module in the system
    for (auto& mapEntry: systemPtr->modules)
    {
        // Add the .mdef file to dependencies.
        dependencies.insert(mapEntry.second->defFilePtr->path);
    }

    // For each app in the system,
    for (auto& mapEntry : systemPtr->apps)
    {
        // Add the .adef file to the dependencies.
        dependencies.insert(mapEntry.second->defFilePtr->path);
    }

    // For each component in the system,
    for (auto& mapEntry : model::Component_t::GetComponentMap())
    {
        auto componentPtr = mapEntry.second;

        // Add the .cdef file to the dependencies.
        dependencies.insert(componentPtr->defFilePtr->path);

        // Add all the .api files to the dependencies.
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
    // It also depends on changes to the mk tools.
    dependencies.insert(path::Combine(envVars::Get("LEGATO_ROOT"), "build/tools/mk"));

    baseGeneratorPtr->GenerateNinjaScriptBuildStatement(dependencies);
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate build rules required for a system
 */
//--------------------------------------------------------------------------------------------------
void SystemBuildScriptGenerator_t::GenerateBuildRules
(
    model::System_t* systemPtr
)
{
    appGeneratorPtr->GenerateBuildRules();
    GenerateSystemBuildRules(systemPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate build script for system
 */
//--------------------------------------------------------------------------------------------------
void SystemBuildScriptGenerator_t::Generate
(
    model::System_t* systemPtr
)
{
    // Start the script with a comment
    GenerateCommentHeader(systemPtr);

    // Add file-level variable definitions.
    std::string includes;
    includes = " -I " + buildParams.workingDir;
    for (const auto& dir : buildParams.interfaceDirs)
    {
        includes += " -I" + dir;
    }
    script << "builddir = " << path::MakeAbsolute(buildParams.workingDir) << "\n\n";
    script << "stagingDir = " << path::Combine(buildParams.workingDir, "staging") << "\n\n";
    script << "cFlags = " << buildParams.cFlags << includes << "\n\n";
    script << "cxxFlags = " << buildParams.cxxFlags << includes << "\n\n";
    script << "ldFlags = " << buildParams.ldFlags << "\n\n";
    script << "target = " << buildParams.target << "\n\n";
    GenerateBuildRules(systemPtr);

    // If we are not just generating code,
    if (!buildParams.codeGenOnly)
    {
        // For each module in .sdef file
        for (auto& mapEntry : systemPtr->modules)
        {
            auto modulePtr = mapEntry.second;

            moduleGeneratorPtr->GenerateBuildStatements(modulePtr);
        }

        // For each app built by the mk tools for this system,
        for (auto& mapEntry : systemPtr->apps)
        {
            auto appPtr = mapEntry.second;

            // Generate build statements for the app's executables.
            appGeneratorPtr->GenerateExeBuildStatements(appPtr);

            // Generate build statements for bundling files into the app's staging area.
            auto appWorkingDir = "$builddir/" + appPtr->workingDir;
            appGeneratorPtr->GenerateAppBundleBuildStatement(appPtr, appWorkingDir);
        }

        // For each component in the system.
        for (auto& mapEntry : model::Component_t::GetComponentMap())
        {
            componentGeneratorPtr->GenerateBuildStatements(mapEntry.second);
            componentGeneratorPtr->GenerateIpcBuildStatements(mapEntry.second);
        }

        // Generate build statement for packing everything into a system update pack.
        GenerateSystemPackBuildStatement(systemPtr);
    }

    // Add a build statement for the build.ninja file itself.
    GenerateNinjaScriptBuildStatement(systemPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate build script for the system.
 **/
//--------------------------------------------------------------------------------------------------
void Generate
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Minimize(buildParams.workingDir + "/build.ninja");

    SystemBuildScriptGenerator_t systemGenerator(filePath, buildParams);

    systemGenerator.Generate(systemPtr);
}


} // namespace ninja
