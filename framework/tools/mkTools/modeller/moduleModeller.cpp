//--------------------------------------------------------------------------------------------------
/**
 * @file moduleModeller.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
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
    const mk::BuildParams_t& buildParams
)
{
    auto tokenListPtr = static_cast<parseTree::TokenList_t*>(sectionPtr);

    if (modulePtr->moduleBuildType != model::Module_t::Prebuilt)
    {
        // Allow either Sources or Prebuilt section, not both
        modulePtr->moduleBuildType = model::Module_t::Sources;
    }
    else
    {
        throw mk::Exception_t(
                  LE_I18N("error: Use either 'sources' or 'preBuilt' section."));
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
                modulePtr->cObjectFiles.push_back(objFilePtr);
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

    modulePtr->SetBuildEnvironment(modulePtr->moduleBuildType, modulePtr->defFilePtr->path);
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds the prebuilt files from a given "preBuilt:" section to a given Module_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddPrebuilt
(
    model::Module_t* modulePtr,
    std::string prebuiltFile
)
{
    if (modulePtr->moduleBuildType != model::Module_t::Sources)
    {
        //Allow either Sources or PreBuilt section, not both
        modulePtr->moduleBuildType = model::Module_t::Prebuilt;
    }
    else
    {
        throw mk::Exception_t(
                  LE_I18N("error: Use either 'sources' or 'preBuilt' section."));
    }

    auto searchFile = modulePtr->koFiles.find(prebuiltFile);
    if (searchFile != modulePtr->koFiles.end())
    {
        throw mk::Exception_t(
                  mk::format(LE_I18N("error: Duplicate preBuilt file %s."), prebuiltFile));
    }

    modulePtr->SetBuildEnvironment(modulePtr->moduleBuildType, prebuiltFile);
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
            auto simpleSectionPtr = ToSimpleSectionPtr(sectionPtr);
            auto modulePath =
                path::Unquote(DoSubstitution(simpleSectionPtr->Text(), simpleSectionPtr));
            if (!path::HasSuffix(modulePath, ".ko"))
            {
                // Throw exception: not a kernel module
                sectionPtr->ThrowException(
                    mk::format(LE_I18N("File '%s' is not a kernel module (*.ko)."), modulePath)
                );
            }
            if (!file::FileExists(modulePath))
            {
                // Throw exception: file doesn't exist
                sectionPtr->ThrowException(
                    mk::format(LE_I18N("Module file '%s' does not exist."),  modulePath)
                );
            }
            AddPrebuilt(modulePtr, modulePath);
        }
        else if ("sources" == sectionName)
        {
            AddSources(modulePtr, sectionPtr, buildParams);
        }
        else if ("cflags" == sectionName)
        {
            AddCFlags(modulePtr, sectionPtr);
        }
        else if ("ldflags" == sectionName)
        {
            AddLdFlags(modulePtr, sectionPtr);
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
