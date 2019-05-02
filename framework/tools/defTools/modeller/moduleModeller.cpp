//--------------------------------------------------------------------------------------------------
/**
 * @file moduleModeller.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"
#include "modellerCommon.h"


namespace modeller
{

//--------------------------------------------------------------------------------------------------
/**
 * Print a summary of a kernel module object.
 **/
//--------------------------------------------------------------------------------------------------
void PrintSummary
(
    model::Module_t* modulePtr
)
//--------------------------------------------------------------------------------------------------
{
    std::cout << std::endl << "== '" << modulePtr->name << "' kernel module summary ==" << std::endl
              << std::endl;

    if (modulePtr->moduleBuildType == model::Module_t::Prebuilt)
    {
        std::cout << "  Pre-built module at:" << std::endl;

        for (auto const& it : modulePtr->koFiles)
        {
             std::cout << "    '" << it.first << "'" << std::endl;
        }
    }

    if (modulePtr->moduleBuildType == model::Module_t::Sources)
    {
        std::cout << "  Built from source files:" << std::endl;
        for (auto obj : modulePtr->cObjectFiles)
        {
            std::cout << "    `" << obj->sourceFilePath << "'" << std::endl;
        }

        std::cout << "  For kernel in directory:" << std::endl;
        std::cout << "    '" << modulePtr->kernelDir << "'" << std::endl;

        if (!modulePtr->cFlags.empty())
        {
            std::cout << "  With additional CFLAGS:" << std::endl;
            for (auto const &cflag : modulePtr->cFlags)
            {
                std::cout << "    " << cflag << std::endl;
            }
        }

        if (!modulePtr->ldFlags.empty())
        {
            std::cout << "  With additional LDFLAGS:" << std::endl;
            for (auto const &ldflag : modulePtr->ldFlags)
            {
                std::cout << "    " << ldflag << std::endl;
            }
        }
    }

    if (modulePtr->HasExternalBuild())
    {
        for (const auto& itCmd : modulePtr->externalBuildCommands)
        {
            std::cout << " " << itCmd << std::endl;
        }
    }

    // load trigger.
    if (modulePtr->loadTrigger == model::Module_t::AUTO)
    {
        std::cout << LE_I18N("  Will be loaded automatically when the Legato framework starts.")
                  << std::endl;
    }
    else
    {
        std::cout << LE_I18N("  Will only load when requested to start.") << std::endl;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the source files from a given "sources:" section to a given Module_t object.
*/
//--------------------------------------------------------------------------------------------------
static void AddSources
(
    model::Module_t* modulePtr,
    parseTree::CompoundItem_t* sectionPtr, ///< The parse tree object for the "sources:" section.
    const mk::BuildParams_t& buildParams,
    bool isSubModule
)
{
    auto tokenListPtr = static_cast<parseTree::TokenList_t*>(sectionPtr);

    if ((modulePtr->moduleBuildType != model::Module_t::Prebuilt) &&
        (!modulePtr->HasExternalBuild()))
    {
        // Allow either sources or preBuilt/externalBuild section.
        modulePtr->moduleBuildType = model::Module_t::Sources;
    }
    else
    {
        throw mk::Exception_t(
                  LE_I18N("error: Use either 'sources' or 'preBuilt/externalBuild' section."));
    }

    for (auto contentPtr: tokenListPtr->Contents())
    {
        auto filePath = path::Unquote(DoSubstitution(contentPtr));
        // Find the file (returns an absolute path or "" if not found).
        auto fullFilePath = file::FindFile(filePath, { modulePtr->dir });

        if (fullFilePath.empty())
        {
            // Not found in .mdef directory, try all source directories
            fullFilePath = file::FindFile(filePath, buildParams.sourceDirs);
        }

        // File should have been found. If not, generate exception.
        if (!fullFilePath.empty())
        {
            // Assume drivers use only C sources for now.
            if (path::IsCSource(filePath))
            {
                auto objFilePath = path::RemoveSuffix(filePath, ".c") + ".o";
                auto objFilePtr = new model::ObjectFile_t(objFilePath, fullFilePath);
                if (!isSubModule)
                {
                    // Not a sub kernel module
                    modulePtr->cObjectFiles.push_back(objFilePtr);
                }
                else
                {
                    // A sub kernel module, hence add to the list of object file pointer for sub
                    // kernel module.
                    modulePtr->subCObjectFiles.push_back(objFilePtr);
                }
            }
            else
            {
                contentPtr->ThrowException("Unrecognized file name extension on source code file '"
                                           + filePath + "'.");
            }
        }
        else
        {
            std::cerr << "Looked in the following places:" << std::endl;
            for (auto& dir : buildParams.sourceDirs)
            {
                std::cerr << "  '" << dir << "'" << std::endl;
            }
            sectionPtr->ThrowException("File '" + contentPtr->text + "' does not exist.");
        }
    }

    if (!isSubModule)
    {
        // Setup build environment for non sub kernel modules
        modulePtr->SetBuildEnvironment(modulePtr->moduleBuildType, modulePtr->defFilePtr->path);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  Adds prebuilt ko files from a given "preBuilt:" section to a given Module_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddPrebuilt
(
    model::Module_t* modulePtr,
    parseTree::CompoundItem_t* sectionPtr ///< The parse tree object for the "preBuilt:" section.
)
{
    auto tokenListPtr = static_cast<parseTree::TokenList_t*>(sectionPtr);

    if (modulePtr->moduleBuildType != model::Module_t::Sources)
    {
        // Allow either sources or preBuilt section.
        modulePtr->moduleBuildType = model::Module_t::Prebuilt;
    }
    else
    {
        throw mk::Exception_t(
                  LE_I18N("error: Use either 'sources' or 'preBuilt' section."));
    }

    for (auto contentPtr: tokenListPtr->Contents())
    {
        auto modulePath = path::Unquote(DoSubstitution(contentPtr));
        if (!path::HasSuffix(modulePath, ".ko"))
        {
            // Throw exception: not a kernel module
            sectionPtr->ThrowException(
                mk::format(LE_I18N("File '%s' is not a kernel module (*.ko)."), modulePath)
            );
        }

        // If the file does not exist then the module might be generated from an external build
        // process. Throw exception if externelBuild section does not exist.
        if (!file::FileExists(modulePath) && !modulePtr->HasExternalBuild())
        {
            // Throw exception: file doesn't exist
            sectionPtr->ThrowException(
                mk::format(LE_I18N("Module file '%s' does not exist."),  modulePath)
            );
        }

        auto searchFile = modulePtr->koFiles.find(modulePath);
        if (searchFile != modulePtr->koFiles.end())
        {
            // Throw exception: duplicate file not allowed
            throw mk::Exception_t(
                      mk::format(LE_I18N("error: Duplicate preBuilt file %s."), modulePath));
        }
        modulePtr->SetBuildEnvironment(modulePtr->moduleBuildType, modulePath);

        modulePtr->koFilesToken.insert(
            std::make_pair(path::GetLastNode(modulePath), sectionPtr->firstTokenPtr));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the contents of a "cflags:" section to the list of cFlags for a given Module_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddCFlags
(
    model::Module_t* modulePtr,
    parseTree::CompoundItem_t* sectionPtr  ///< Parse tree object for the section.
)
//--------------------------------------------------------------------------------------------------
{
    auto tokenListPtr = static_cast<parseTree::TokenList_t*>(sectionPtr);

    // The section contains a list of FILE_PATH tokens.
    for (auto contentPtr: tokenListPtr->Contents())
    {
        modulePtr->cFlags.push_back(DoSubstitution(contentPtr));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the contents of a "ldflags:" section to the list of ldFlags for a given Module_t.
 */
//--------------------------------------------------------------------------------------------------
static void AddLdFlags
(
    model::Module_t* modulePtr,
    parseTree::CompoundItem_t* sectionPtr  ///< Parse tree object for the section.
)
//--------------------------------------------------------------------------------------------------
{
    auto tokenListPtr = static_cast<parseTree::TokenList_t*>(sectionPtr);

    // The section contains a list of FILE_PATH tokens.
    for (auto contentPtr: tokenListPtr->Contents())
    {
        modulePtr->ldFlags.push_back(DoSubstitution(contentPtr));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Model a "requires:" section.
 */
//--------------------------------------------------------------------------------------------------
static void AddRequiredItems
(
    model::Module_t* modulePtr,
    bool isSubModule,
    const parseTree::Content_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::list<const parseTree::CompoundItem_t*> reqKernelModulesSections;

    for (auto subsectionPtr : ToCompoundItemListPtr(sectionPtr)->Contents())
    {
        auto& subsectionName = subsectionPtr->firstTokenPtr->text;

        if (parser::IsNameSingularPlural(subsectionName, "kernelModule"))
        {
           reqKernelModulesSections.push_back(subsectionPtr);
        }
        else
        {
            subsectionPtr->ThrowException(
                mk::format(LE_I18N("Internal error: Unrecognized sub-section '%s'."),
                           subsectionName)
            );
        }
    }

    // Add the required kernel modules.
    if (isSubModule)
    {
        AddRequiredKernelModules(modulePtr->requiredModulesOfSubMod, modulePtr,
                                 reqKernelModulesSections, buildParams);
    }
    else
    {
        AddRequiredKernelModules(modulePtr->requiredModules, modulePtr, reqKernelModulesSections,
                                 buildParams);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the items from a given "bundles:" section to a given Module_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddBundledItems
(
    model::Module_t* modulePtr,
    const parseTree::CompoundItem_t* sectionPtr   ///< Ptr to "bundles:" section in the parse tree.
)
//--------------------------------------------------------------------------------------------------
{
    // Bundles section is comprised of subsections (either "file:" or "dir:") which all have
    // the same basic structure (ComplexSection_t).
    // "file:" sections contain BundledFile_t objects (with type BUNDLED_FILE).
    // "dir:" sections contain BundledDir_t objects (with type BUNDLED_DIR).
    for (auto memberPtr : parseTree::ToComplexSectionPtr(sectionPtr)->Contents())
    {
        auto subsectionPtr = parseTree::ToCompoundItemListPtr(memberPtr);

        if (subsectionPtr->Name() == "file")
        {
            for (auto itemPtr : subsectionPtr->Contents())
            {
                auto bundledFileTokenListPtr = parseTree::ToTokenListPtr(itemPtr);

                auto bundledFilePtr = GetBundledItem(bundledFileTokenListPtr);

                // If the source path is not absolute, then it is relative to the directory
                // containing the .adef file.
                if (!path::IsAbsolute(bundledFilePtr->srcPath))
                {
                    bundledFilePtr->srcPath = path::Combine(modulePtr->dir, bundledFilePtr->srcPath);
                }

                // Make sure that the source path exists and is a file.
                if (file::FileExists(bundledFilePtr->srcPath))
                {
                    modulePtr->bundledFiles.insert(
                        std::shared_ptr<model::FileSystemObject_t>(bundledFilePtr));
                }
                else if (file::AnythingExists(bundledFilePtr->srcPath))
                {
                    bundledFileTokenListPtr->ThrowException(
                        mk::format(LE_I18N("Not a regular file: '%s'."), bundledFilePtr->srcPath)
                    );
                }
                else
                {
                    bundledFileTokenListPtr->ThrowException(
                        mk::format(LE_I18N("File not found: '%s'."),  bundledFilePtr->srcPath)
                    );
                }
            }
        }
        else if (subsectionPtr->Name() == "dir")
        {
            for (auto itemPtr : subsectionPtr->Contents())
            {
                auto bundledDirTokenListPtr = parseTree::ToTokenListPtr(itemPtr);

                auto bundledDirPtr = GetBundledItem(bundledDirTokenListPtr);

                // If the source path is not absolute, then it is relative to the directory
                // containing the .adef file.
                if (!path::IsAbsolute(bundledDirPtr->srcPath))
                {
                    bundledDirPtr->srcPath = path::Combine(modulePtr->dir, bundledDirPtr->srcPath);
                }

                // Make sure that the source path exists and is a directory.
                if (file::DirectoryExists(bundledDirPtr->srcPath))
                {
                    modulePtr->bundledDirs.insert(
                        std::shared_ptr<model::FileSystemObject_t>(bundledDirPtr));
                }
                else if (file::AnythingExists(bundledDirPtr->srcPath))
                {
                    bundledDirTokenListPtr->ThrowException(
                        mk::format(LE_I18N("Not a directory: '%s'."), bundledDirPtr->srcPath)
                    );
                }
                else
                {
                    bundledDirTokenListPtr->ThrowException(
                        mk::format(LE_I18N("Directory not found: '%s'."), bundledDirPtr->srcPath)
                    );
                }
            }
        }
        else
        {
            subsectionPtr->ThrowException(
                mk::format(LE_I18N("Internal error: Unexpected content item: %s."),
                           subsectionPtr->TypeName())
            );
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Model a "scripts:" section.
 */
//--------------------------------------------------------------------------------------------------
static void AddScripts
(
    model::Module_t* modulePtr,
    const parseTree::Content_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto subsectionPtr : ToCompoundItemListPtr(sectionPtr)->Contents())
    {
        auto& subsectionName = subsectionPtr->firstTokenPtr->text;

        auto simpleSectionPtr = ToSimpleSectionPtr(subsectionPtr);
        auto scriptPath =
            path::Unquote(DoSubstitution(simpleSectionPtr->Text(), simpleSectionPtr));

        if (!file::FileExists(scriptPath))
        {
            // Throw exception: file doesn't exist
            subsectionPtr->ThrowException(
                mk::format(LE_I18N("Script file '%s' does not exist."),  scriptPath)
            );
        }

        if ("install" == subsectionName)
        {
            if (!modulePtr->installScript.empty())
            {
                 subsectionPtr->ThrowException(
                    mk::format(LE_I18N("Internal error: Multiple install scripts not allowed.\n"
                                       "Install script '%s' found."),
                               modulePtr->installScript)
                );

            }

           modulePtr->installScript = scriptPath;
        }
        else if ("remove" == subsectionName)
        {
            if (!modulePtr->removeScript.empty())
            {
                 subsectionPtr->ThrowException(
                    mk::format(LE_I18N("Internal error: Multiple remove scripts not allowed.\n"
                                       "Remove script '%s' found."),
                               modulePtr->removeScript)
                );
            }

            modulePtr->removeScript = scriptPath;
        }
        else
        {
            subsectionPtr->ThrowException(
                mk::format(LE_I18N("Internal error: Unrecognized sub-section '%s'."),
                           subsectionName)
            );
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Model sub kernel module section 'kernelModule:'
 */
//--------------------------------------------------------------------------------------------------
static void AddSubKernelModule
(
    model::Module_t* modulePtr,
    const parseTree::Content_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    for (auto subsectionPtr : ToCompoundItemListPtr(sectionPtr)->Contents())
    {
        auto& subsectionName = subsectionPtr->firstTokenPtr->text;

        if (subsectionName == "name")
        {
            auto simpleSectionPtr = ToSimpleSectionPtr(subsectionPtr);
            modulePtr->subModuleName = path::Unquote(DoSubstitution(simpleSectionPtr->Text(),
                                                                    simpleSectionPtr));
        }
        else if (subsectionName == "sources")
        {
            AddSources(modulePtr, subsectionPtr, buildParams, true);
        }
        else if (subsectionName == "requires")
        {
            AddRequiredItems(modulePtr, true, subsectionPtr, buildParams);
        }
        else
        {
            subsectionPtr->ThrowException(
                mk::format(LE_I18N("Internal error: Unrecognized sub-section '%s'."),
                           subsectionName)
            );
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add the commands from a given "externalBuild:" section to a given Module_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddExternalBuild
(
    model::Module_t* modulePtr,
    parseTree::CompoundItem_t* sectionPtr ///< The parse tree object for the "externalBuild:" section.
)
{
    auto tokenListPtr = static_cast<parseTree::TokenList_t*>(sectionPtr);

    for (auto contentPtr: tokenListPtr->Contents())
    {
        modulePtr->externalBuildCommands.push_back(path::Unquote(DoSubstitution(contentPtr)));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a module whose .mdef file can be found at a given path.
 *
 * @return Pointer to the module object.
 */
//--------------------------------------------------------------------------------------------------
model::Module_t* GetModule
(
    const std::string& mdefPath,    ///< Path to the module's .mdef file.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto mdefFilePtr = parser::mdef::Parse(mdefPath, buildParams.beVerbose);
    auto modulePtr = new model::Module_t(mdefFilePtr);

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Modelling module defined in: '%s'"), mdefPath)
                  << std::endl;
    }

    for (auto sectionPtr : mdefFilePtr->sections)
    {
        auto& sectionName = sectionPtr->firstTokenPtr->text;
        if ("params" == sectionName)
        {
            for (auto& paramsPtr : ToCompoundItemListPtr(sectionPtr)->Contents())
            {
                auto paramName = paramsPtr->firstTokenPtr->text;
                // We should call this RemoveQuotes(), but it still works.
                auto paramVal = RemoveAngleBrackets(paramsPtr->lastTokenPtr->text);
                modulePtr->AddParam(paramName, paramVal);

            }
        }
        else if ("preBuilt" == sectionName)
        {
            AddPrebuilt(modulePtr, sectionPtr);
        }
        else if ("sources" == sectionName)
        {
            AddSources(modulePtr, sectionPtr, buildParams, false);
        }
        else if ("cflags" == sectionName)
        {
            AddCFlags(modulePtr, sectionPtr);
        }
        else if ("ldflags" == sectionName)
        {
            AddLdFlags(modulePtr, sectionPtr);
        }
        else if ("requires" == sectionName)
        {
            AddRequiredItems(modulePtr, false, sectionPtr, buildParams);
        }
        else if (sectionName == "load")
        {
            SetLoad(modulePtr, ToSimpleSectionPtr(sectionPtr));
        }
        else if (sectionName == "bundles")
        {
            AddBundledItems(modulePtr, sectionPtr);
        }
        else if (sectionName == "scripts")
        {
            AddScripts(modulePtr, sectionPtr);
        }
        else if (sectionName == "kernelModule")
        {
            // First clear variables related to sub modules
            modulePtr->subModuleName.clear();
            modulePtr->subCObjectFiles.clear();
            modulePtr->requiredModulesOfSubMod.clear();

            AddSubKernelModule(modulePtr, sectionPtr, buildParams);

            // If the sub module name is not provided, then generate the name by appending number
            // at the end of the module name. E.g. if the module name is 'myMod', then the sub
            // module names are myMod1, myMod2 and so on.
            if (modulePtr->subModuleName.empty())
            {
                static int subModuleCount = 1;
                modulePtr->name =
                       path::RemoveSuffix(path::GetLastNode(modulePtr->defFilePtr->path), ".mdef");
                modulePtr->subModuleName += modulePtr->name + std::to_string(subModuleCount);
                subModuleCount++;
            }

            // Add to the map of sub kernel module.
            modulePtr->subKernelModules[modulePtr->subModuleName] = modulePtr->subCObjectFiles;
            modulePtr->requiredSubModules[modulePtr->subModuleName] =
                                                            modulePtr->requiredModulesOfSubMod;
            // Setup the build environment as both module name and sub module name is now available.
            modulePtr->SetBuildEnvironmentSubModule(modulePtr->defFilePtr->path);
        }
        else if (sectionName == "externalBuild")
        {
            modulePtr->name = path::RemoveSuffix(path::GetLastNode(modulePtr->defFilePtr->path),
                                                 ".mdef");
            AddExternalBuild(modulePtr, sectionPtr);
        }
        else
        {
            sectionPtr->ThrowException(
                mk::format(LE_I18N("Internal error: Unrecognized section '%s'."), sectionName)
            );
        }
    }

    // We should have either provided a 'sources:' or 'preBuilt:' section
    if (modulePtr->moduleBuildType == model::Module_t::Invalid)
    {
        delete modulePtr;

        // Throw generic exception at file level
        throw mk::Exception_t(
            mk::format(LE_I18N("%s: error: Use either 'sources' or 'preBuilt' section."), mdefPath)
        );
    }

    // Setup path to kernel sources from KERNELROOT or SYSROOT variables
    auto kernel = envVars::Get("LEGATO_KERNELROOT");
    modulePtr->kernelDir = path::Unquote(parseTree::DoSubstitution(kernel));
    if (modulePtr->kernelDir.empty())
    {
        modulePtr->kernelDir = path::Combine(envVars::Get("LEGATO_SYSROOT"), "usr/src/kernel");
    }
    if (!file::FileExists(modulePtr->kernelDir + "/.config"))
    {
        auto kernelDir = modulePtr->kernelDir;
        delete modulePtr;
        throw mk::Exception_t(
            mk::format(LE_I18N("%s: error: '%s' is not a valid kernel source directory."),
            mdefPath, kernelDir)
        );
    }

    return modulePtr;
}

} // namespace modeller
