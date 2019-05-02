//--------------------------------------------------------------------------------------------------
/**
 * @file moduleBuildScript.cpp
 *
 * Implementation of the build script generator for pre-built kernel modules.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "buildScriptCommon.h"
#include "moduleBuildScript.h"

namespace ninja
{



//--------------------------------------------------------------------------------------------------
/**
 * Generate comment header for a build script.
 */
//--------------------------------------------------------------------------------------------------
void ModuleBuildScriptGenerator_t::GenerateCommentHeader
(
    model::Module_t* modulePtr
)
//--------------------------------------------------------------------------------------------------
{
    script  << "# Build script for module '" << modulePtr->name << "'\n"
               "\n"
               "# == Auto-generated file.  Do not edit. ==\n"
               "\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the path to the ko file in module's build directory.
 */
//--------------------------------------------------------------------------------------------------
static std::string FindKoPathOfSubKernelModule
(
    model::Module_t* modulePtr,
    std::string moduleName
)
{
    std::string modulePath;
    auto itMod = modulePtr->subKernelModules.find(moduleName);
    if (itMod != modulePtr->subKernelModules.end())
    {
        // Get the ko build file path
        for (auto const& itKoFiles : modulePtr->koFiles)
        {
            auto koName = path::RemoveSuffix(path::GetLastNode(itKoFiles.second->path), ".ko");
            if (koName.compare(moduleName) == 0)
            {
               modulePath = itKoFiles.second->path;
            }
        }
    }

    return modulePath;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to a given build script the build statements related to a given module.
 * If it's a pre-built module, just copy it. Otherwise generate a module Makefile and build it.
 **/
//--------------------------------------------------------------------------------------------------
void ModuleBuildScriptGenerator_t::GenerateBuildStatements
(
    model::Module_t* modulePtr
)
//--------------------------------------------------------------------------------------------------
{
    if (modulePtr->HasExternalBuild())
    {
        // Create external build commands for each line
        std::list<std::string>::const_iterator commandPtr;
        int lineno;
        for (commandPtr = modulePtr->externalBuildCommands.begin(), lineno = 0;
             commandPtr != modulePtr->externalBuildCommands.end();
             ++commandPtr, ++lineno)
        {
            script << "build " << modulePtr->name << "ExternalBuild_line"
                   << lineno
                   << " : BuildExternal | ";
            if (lineno != 0)
            {
                // Create dependencies to the previous command line
                script << modulePtr->name << "ExternalBuild_line" << (lineno - 1);
            }
            script << std::endl;
            script   << "  externalCommand = " << EscapeString(*commandPtr) << std::endl;
        }

        // Overall build depends on last line
        script << "build " << modulePtr->name << "ExternalBuild";

        // Assume every pre-built file listed in the preBuilt section could be a build output of
        // the external build step
        for (auto const& itKo : modulePtr->koFiles)
        {
            script << " " << itKo.first;
        }

        // Use phony rule to create alias
        script << " : phony "
               << modulePtr->name << "ExternalBuild_line"
               << (lineno - 1);
        script << "\n\n";
    }

    if (modulePtr->moduleBuildType == model::Module_t::Sources)
    {
        for (auto const& it : modulePtr->koFiles)
        {
            script << "build " << "$builddir/" << it.second->path << ": ";

            // No pre-built module: generate and invoke a Makefile
            GenerateMakefile(modulePtr);
            script << "MakeKernelModule " << "$builddir/"
                   << path::GetContainingDir(it.second->path);

            // Specify dependencies with "||'.
            // Include order-only build dependencies to make sure the dependency module is
            // built first than the module that depends on it. Expressed with sytax
            // "|| dep1 dep2" on the end of a build line.
            // This also allows the ninja build to detect any circular dependencies found in the
            // kernel modules.
            if (!modulePtr->requiredModules.empty())
            {
                script << " || ";
            }
            else
            {
                if (!modulePtr->requiredSubModules.empty())
                {
                    script << " || ";
                }
            }

            for (auto const& reqMod : modulePtr->requiredModules)
            {
                model::Module_t* reqModPtr = model::Module_t::GetModule(reqMod.first);
                if (reqModPtr == NULL)
                {
                    throw mk::Exception_t(
                              mk::format(
                                  LE_I18N("Internal Error: Module object not found for '%s'."),
                                  reqMod.first));
                }

                for (auto const& reqMod : reqModPtr->koFiles)
                {
                    script << "$builddir/" << reqMod.second->path << " ";
                }
            }

            // For each sub kernel module, iterate through its list of required modules and print
            // the path to its .ko file in the build directory
            std::string koName = path::RemoveSuffix(path::GetLastNode(it.second->path), ".ko");

            for (auto const& reqMod : modulePtr->requiredSubModules)
            {
                if (koName.compare(reqMod.first) == 0)
                {
                    for (auto const& itSubModMap : reqMod.second)
                    {
                        script << " $builddir/"
                               << FindKoPathOfSubKernelModule(modulePtr, itSubModMap.first) << " ";
                    }
                }
            }

            script << "\n";
        }
    }
    else if (modulePtr->moduleBuildType == model::Module_t::Prebuilt)
    {
        for (auto const& it: modulePtr->koFiles)
        {
            script << "build " << "$builddir/" << it.second->path << ": ";

            // Pre-built module: add build statement for bundling the .ko file
            script << "BundleFile " << it.first << "\n"
                   << "  modeFlags = u+rw-x,g+r-wx,o+r-wx\n";
        }
    }
    else
    {
        throw mk::Exception_t(
                  mk::format(LE_I18N("error: %s must have either 'sources' or 'preBuilt' section."),
                              modulePtr->defFilePtr->path));
    }
    script << "\n";
    // Generate build statament
    GenerateModuleBundleBuildStatement(modulePtr, buildParams.outputDir);
    script << "\n";

    if ((!modulePtr->installScript.empty())
         && (!(modulePtr->removeScript.empty())))
    {
        std::string stageInstallPath;
        stageInstallPath +="staging/modules/files/";
        stageInstallPath += modulePtr->name;
        stageInstallPath += "/scripts/";
        stageInstallPath += path::GetLastNode(modulePtr->installScript);

        script << "build " << "$builddir/" << stageInstallPath << ": ";

        // Build statement for bundling the module install script
        script << "BundleFile " << modulePtr->installScript << "\n"
               << "  modeFlags = u+rwx,g+rx-w,o+rx-w\n";

        std::string stageRemovePath;
        stageRemovePath +="staging/modules/files/";
        stageRemovePath += modulePtr->name;
        stageRemovePath += "/scripts/";
        stageRemovePath += path::GetLastNode(modulePtr->removeScript);

        script << "build " << "$builddir/" << stageRemovePath << ": ";

        // Build statement for bundling the module remove file
        script << "BundleFile " << modulePtr->removeScript << "\n"
               << "  modeFlags = u+rwx,g+rx-w,o+rx-w\n";
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statements for the build script itself.
 **/
//--------------------------------------------------------------------------------------------------
void ModuleBuildScriptGenerator_t::GenerateNinjaScriptBuildStatement
(
    model::Module_t* modulePtr
)
//--------------------------------------------------------------------------------------------------
{
    // The build.ninja depends on module .mdef and .ko files
    // Create a set of dependencies.
    std::set<std::string> dependencies;

    for (auto const& it : modulePtr->koFiles)
    {
        dependencies.insert(it.first);
    }

    dependencies.insert(modulePtr->defFilePtr->path);
    // It also depends on changes to the mk tools.
    dependencies.insert(path::Combine(envVars::Get("LEGATO_ROOT"), "build/tools/bin/mk"));

    baseGeneratorPtr->GenerateNinjaScriptBuildStatement(dependencies);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a Makefile for a kernel module.
 **/
//--------------------------------------------------------------------------------------------------
void ModuleBuildScriptGenerator_t::GenerateMakefile
(
    model::Module_t* modulePtr
)
//--------------------------------------------------------------------------------------------------
{
    std::string buildPath = path::MakeAbsolute(buildParams.workingDir
                                + "/modules/" + modulePtr->name);
    const std::string& compilerPath = buildParams.cCompilerPath;

    std::ofstream makefile;
    OpenFile(makefile, buildPath + "/Makefile", buildParams.beVerbose);

    // Specify kernel module name and list all object files to link
    makefile << "obj-m += ";

    if (modulePtr->cObjectFiles.size() > 0)
    {
        makefile << modulePtr->name <<  ".o";
    }

    // Specify sub kernel modules
    for (auto const& obj : modulePtr->subKernelModules)
    {
        makefile << " " << obj.first << ".o" <<" ";
    }
    makefile << "\n";

    // Don't list object files in case of a single source file with module name
    if (!modulePtr->cObjectFiles.empty()
        && (modulePtr->cObjectFiles.size() > 1 ||
            modulePtr->cObjectFiles.front()->path != modulePtr->name + ".o"))
    {
        for (auto const& obj : modulePtr->cObjectFiles)
        {
            makefile << modulePtr->name << "-objs += " << obj->path << "\n";
        }
    }

    // List the sub kernel module object files
    for (auto const& obj : modulePtr->subKernelModules)
    {
        // If sub kernel module has only one source file that matches the name of the sub
        // module name, then do not print the object file in Makefile.
        auto skipMod = false;

        if (obj.second.size() == 1)
        {
            std::string cObjectName = path::RemoveSuffix(
                                            path::GetLastNode(obj.second.front()->path), ".o");
            if (obj.first.compare(cObjectName) == 0)
            {
                skipMod = true;
            }
        }

        if (!skipMod)
        {
            // Print in the format <sub module name>-objs += source1.o source2.o
            makefile << obj.first << "-objs += ";

            for (auto const& it : obj.second)
            {
                makefile << it->path << " ";
            }
            makefile << "\n";
        }
    }

    makefile << "\n";

    // Specify directory where the sources are located
    makefile << "src = " << modulePtr->dir << "\n\n";

    // Add compiler and linker options
    for (auto const &obj : modulePtr->cFlags)
    {
        makefile << "ccflags-y += " << obj << "\n";
    }
    for (auto const &obj : modulePtr->ldFlags)
    {
        makefile << "ldflags-y += " << obj << "\n";
    }
    makefile << "\n";

    makefile << "KBUILD := " << modulePtr->kernelDir << "\n";

    // Iterate through all the required modules and concatenate the modules's Module.symvers file
    // path for later passing it to KBUILD_EXTRA_SYMBOLS variable during make.
    std::string buildPathModuleSymvers;
    for (auto const &reqMod :  modulePtr->requiredModules)
    {
        model::Module_t* reqModPtr = model::Module_t::GetModule(reqMod.first);
        if (reqModPtr == NULL)
        {
            throw mk::Exception_t(
                      mk::format(LE_I18N("Internal Error: Module object not found for '%s'."),
                                 reqMod.first));
        }

        if (reqModPtr->moduleBuildType == model::Module_t::Sources)
        {
            std::string reqModuleSymvers = path::MakeAbsolute(buildParams.workingDir
                                           + "/modules/" + reqMod.first + "/Module.symvers");
            buildPathModuleSymvers = buildPathModuleSymvers + reqModuleSymvers + " ";
        }
    }

    if (buildParams.target != "localhost")
    {
        // Specify the CROSS_COMPILE and ARCH environment variables
        // Note: compiler path may contain dashes in directory names
        std::string compiler = path::GetLastNode(compilerPath);
        std::string cross = path::GetContainingDir(compilerPath) + "/"
                                + compiler.substr(0, compiler.rfind('-') + 1);
        std::string arch = compiler.substr(0, compiler.find('-'));
        if ((arch == "i586") || (arch == "i686"))
        {
            arch = "x86";
        }

        makefile << "export CROSS_COMPILE := " << cross << "\n";
        makefile << "export ARCH := " << arch << "\n";
    }
    makefile << "\n";

    // Specify build rules
    makefile << "all:\n";

    makefile << "\tmake -C $(KBUILD) M=" + buildPath;

    // Pass KBUILD_EXTRA_SYMBOLS to resolve module build dependencies.
    if (!buildPathModuleSymvers.empty())
    {
        makefile << " 'KBUILD_EXTRA_SYMBOLS=" + buildPathModuleSymvers + "'";
    }

    makefile << " modules\n";

    makefile << "\n";
    makefile << "clean:\n";
    makefile << "\t make -C $(KBUILD) M=" + buildPath + " clean\n";

    CloseFile(makefile);
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given build script the build statement for bundling a single file into
 * the staging area.
 **/
//--------------------------------------------------------------------------------------------------
void ModuleBuildScriptGenerator_t::GenerateFileBundleBuildStatement
(
    model::FileSystemObjectSet_t& bundledFiles, ///< Set to fill with bundled file paths.
    model::Module_t* modulePtr, ///< Module to bundle the file into.
    const model::FileSystemObject_t* fileSystemObjPtr  ///< File bundling info.
)
//--------------------------------------------------------------------------------------------------
{
    // The file will be added to the module's staging area.
    path::Path_t destPath = "$builddir/staging/modules/files/";
    destPath += modulePtr->name;
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
void ModuleBuildScriptGenerator_t::GenerateDirBundleBuildStatements
(
    model::FileSystemObjectSet_t& bundledFiles, ///< Set to fill with bundled file paths.
    model::Module_t* modulePtr, ///< Module to bundle the directory into.
    const model::FileSystemObject_t* fileSystemObjPtr  ///< Directory bundling info.
)
//--------------------------------------------------------------------------------------------------
{
    // The files will be added to the modules's staging area.
    path::Path_t destPath = "$builddir/staging/modules/files/";
    destPath += modulePtr->name;
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
 * Write to a given build script the build statements for bundling a given module's files into the
 * module's staging area.
 *
 **/
//--------------------------------------------------------------------------------------------------
void ModuleBuildScriptGenerator_t::GenerateStagingBundleBuildStatements
(
    model::Module_t* modulePtr
)
//--------------------------------------------------------------------------------------------------
{
    auto& allBundledFiles = modulePtr->GetTargetInfo<target::FileSystemInfo_t>()->allBundledFiles;

    for (auto fileSystemObjPtr : modulePtr->bundledFiles)
    {
        GenerateFileBundleBuildStatement(allBundledFiles,
                                         modulePtr,
                                         fileSystemObjPtr.get());
    }

    for (auto fileSystemObjPtr : modulePtr->bundledDirs)
    {
        GenerateDirBundleBuildStatements(allBundledFiles,
                                         modulePtr,
                                         fileSystemObjPtr.get());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Write to a given script the build statements for packing up everything into a module bundle.
 *
 **/
//--------------------------------------------------------------------------------------------------
void ModuleBuildScriptGenerator_t::GenerateModuleBundleBuildStatement
(
    model::Module_t* modulePtr,
    const std::string& outputDir  ///< Path to the directory into which built module will be added.
)
//--------------------------------------------------------------------------------------------------
{
    // Give this a FS target info
    modulePtr->SetTargetInfo(new target::FileSystemInfo_t());

    // Generate build statements for bundling files into the staging area.
    GenerateStagingBundleBuildStatements(modulePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for a kernel module.
 */
//--------------------------------------------------------------------------------------------------
void ModuleBuildScriptGenerator_t::Generate
(
    model::Module_t* modulePtr
)
{
    // Start the script with a comment, the file-level variable definitions, and
    // a set of generic rules.
    GenerateCommentHeader(modulePtr);
    script << "builddir = " << path::MakeAbsolute(buildParams.workingDir) << "\n\n";
    script << "target = " << buildParams.target << "\n\n";
    script << "ifgenFlags = ";
    baseGeneratorPtr->GenerateIfgenFlags();
    script << "\n\n";

    baseGeneratorPtr->GenerateBuildRules();

    if (!buildParams.codeGenOnly)
    {
        // Add build statements for the module.
        GenerateBuildStatements(modulePtr);
    }

    // Add a build statement for the build.ninja file itself.
    GenerateNinjaScriptBuildStatement(modulePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Generate a build script for a pre-built kernel module.
 **/
//--------------------------------------------------------------------------------------------------
void Generate
(
    model::Module_t* modulePtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Minimize(buildParams.workingDir + "/build.ninja");

    ModuleBuildScriptGenerator_t scriptGenerator(filePath, buildParams);
    scriptGenerator.Generate(modulePtr);
}


} // namespace ninja
