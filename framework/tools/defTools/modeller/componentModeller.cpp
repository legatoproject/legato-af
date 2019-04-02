//--------------------------------------------------------------------------------------------------
/**
 * @file componentModeller.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"
#include "modellerCommon.h"
#include "componentModeller.h"

namespace modeller
{


//--------------------------------------------------------------------------------------------------
/**
 * Find a source code file or a header file for a component.
 *
 * @return the absolute path to the file, or an empty string if environment variable substitution
 *         resulted in an empty string.
 *
 * @throw Exception_t if a non-empty path was provided, but a file doesn't exist at that path.
 */
//--------------------------------------------------------------------------------------------------
static std::string FindFile
(
    model::Component_t* componentPtr,
    const parseTree::Token_t* tokenPtr, ///< Parse tree token containing the file path.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto filePath = path::Unquote(DoSubstitution(tokenPtr));

    // If the environment variable substitution resulted in an empty string, then just
    // skip this file.
    if (filePath.empty())
    {
        return filePath;
    }

    // Check the component's directory first.
    auto fullFilePath = file::FindFile(filePath, { componentPtr->dir });
    if (fullFilePath.empty())
    {
        fullFilePath = file::FindFile(filePath, buildParams.sourceDirs);

        if (fullFilePath.empty())
        {
            tokenPtr->ThrowException(
                mk::format(LE_I18N("Couldn't find source file '%s'"), filePath)
            );
        }
    }

    return path::MakeAbsolute(fullFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the BUILDDIR environment variable which points to the directory where the files are
 * generated from the code generator for a given component.
 */
//--------------------------------------------------------------------------------------------------
static void SetComponentBuildDirEnvVar
(
    model::Component_t* componentPtr,
    const mk::BuildParams_t& buildParams
)
{
    std::string legatoRoot = envVars::Get("LEGATO_ROOT");
    std::string componentBuildDir;

    // BUILDDIR is a combination of buildParams.workingDir, component and pathMd5.
    // /home/user/workspace/legato/build/wp76xx/system/component/5b3b157328326f66388f6a3296e4dcba/
    componentBuildDir = path::Minimize(buildParams.workingDir + "/component/" +
                                       componentPtr->defFilePtr->pathMd5);

    envVars::Set("BUILDDIR", componentBuildDir);
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the commands from a given "externalBuild:" section to a given Component_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddExternalBuild
(
    model::Component_t* componentPtr,
    parseTree::CompoundItem_t* sectionPtr ///< The parse tree object for the "externalBuild:"
                                          ///  section.
)
{
    auto tokenListPtr = static_cast<parseTree::TokenList_t*>(sectionPtr);

    for (auto contentPtr: tokenListPtr->Contents())
    {
        componentPtr->externalBuildCommands.push_back(path::Unquote(DoSubstitution(contentPtr)));
    }

    if (componentPtr->HasIncompatibleLanguageCode())
    {
        componentPtr->ThrowIncompatibleLanguageException(sectionPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the source files from a given "sources:" section to a given Component_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddSources
(
    model::Component_t* componentPtr,
    parseTree::CompoundItem_t* sectionPtr, ///< The parse tree object for the "sources:" section.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto tokenListPtr = static_cast<parseTree::TokenList_t*>(sectionPtr);

    for (auto contentPtr: tokenListPtr->Contents())
    {
        // Find the file (returns an absolute path or "" if not found).
        auto filePath = FindFile(componentPtr, contentPtr, buildParams);

        // If the environment variable substitution resulted in an empty string, then just
        // skip this file.
        if (!filePath.empty())
        {
            auto objFilePath = path::Combine(componentPtr->workingDir, "obj/")
                             + md5(path::MakeCanonical(filePath)) + ".o";

            if (path::IsCSource(filePath))
            {
                auto objFilePtr = new model::ObjectFile_t(objFilePath, filePath);

                componentPtr->cObjectFiles.push_back(objFilePtr);
            }
            else if (path::IsCxxSource(filePath))
            {
                auto objFilePtr = new model::ObjectFile_t(objFilePath, filePath);

                componentPtr->cxxObjectFiles.push_back(objFilePtr);
            }
            else
            {
                contentPtr->ThrowException(
                    mk::format(LE_I18N("Unrecognized file name extension on source code file"
                                       " '%s'."), filePath)
                );
            }
        }
    }

    if (componentPtr->HasIncompatibleLanguageCode())
    {
        componentPtr->ThrowIncompatibleLanguageException(sectionPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the source files from a given "javaPackage:" section to a given Component_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddJavaPackage
(
    model::Component_t* componentPtr,
    parseTree::CompoundItem_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto tokenListPtr = static_cast<parseTree::TokenList_t*>(sectionPtr);

    if (envVars::Get("LE_CONFIG_CONFIGURED") == "y" &&
        envVars::Get("LE_CONFIG_ENABLE_JAVA") != "y")
    {
        // Warn the user if adding a Java package without the appropriate config environment
        // variable set.  This is not an error because the user may be invoking the mk tools
        // directly and may not have set this configuration in the environment.
        sectionPtr->PrintWarning(
            LE_I18N("Java package added, but LE_CONFIG_ENABLE_JAVA is not set.  Are the KConfig "
                "values correctly configured?")
        );
    }

    for (auto contentPtr: tokenListPtr->Contents())
    {
        std::string packagePath = contentPtr->text;
        auto packagePtr = new model::JavaPackage_t(contentPtr->text, componentPtr->dir);

        componentPtr->javaPackages.push_back(packagePtr);
    }

    if (componentPtr->HasIncompatibleLanguageCode())
    {
        componentPtr->ThrowIncompatibleLanguageException(sectionPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the source files from a given "pythonPackage:" section to a given Component_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddPythonPackage
(
    model::Component_t* componentPtr,
    parseTree::CompoundItem_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto tokenListPtr = static_cast<parseTree::TokenList_t*>(sectionPtr);

    if (envVars::Get("LE_CONFIG_CONFIGURED") == "y" &&
        envVars::Get("LE_CONFIG_ENABLE_PYTHON") != "y")
    {
        // Warn the user if adding a Python package without the appropriate config environment
        // variable set.  This is not an error because the user may be invoking the mk tools
        // directly and may not have set this configuration in the environment.
        sectionPtr->PrintWarning(
            LE_I18N("Python package added, but LE_CONFIG_ENABLE_PYTHON is not set.  Are the KConfig "
                "values correctly configured?")
        );
    }

    for (auto contentPtr: tokenListPtr->Contents())
    {
        std::string packagePath = contentPtr->text;
        auto packagePtr = new model::PythonPackage_t(contentPtr->text, componentPtr->dir);

        componentPtr->pythonPackages.push_back(packagePtr);
    }

    if (componentPtr->HasIncompatibleLanguageCode())
    {
        componentPtr->ThrowIncompatibleLanguageException(sectionPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the python files corresponding to all client APIs.
 */
//--------------------------------------------------------------------------------------------------
static void AddPythonClientFiles
(
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Bundle all the api C extensions and wrappers
    for (auto ifPtr : componentPtr->clientApis)
    {
        model::InterfacePythonFiles_t pythonFiles;
        ifPtr->GetInterfaceFiles(pythonFiles);

        model::Permissions_t soPerms(1, 0, 1);

        auto soDestPath = path::Combine("lib/", pythonFiles.cExtensionBinaryFile);
        auto soSrcPath = "$builddir/" + path::Combine(ifPtr->apiFilePtr->codeGenDir,
                pythonFiles.cExtensionBinaryFile);
        auto soFile = new model::FileSystemObject_t(soSrcPath, soDestPath, soPerms);

        componentPtr->bundledFiles.insert(
                std::shared_ptr<model::FileSystemObject_t>(soFile));
        model::Permissions_t wrapperPerms(soPerms);

        auto wrapperDestPath =  path::Combine("lib/", pythonFiles.wrapperSourceFile);
        auto wrapperSrcPath = "$builddir/"
            + path::Combine(ifPtr->apiFilePtr->codeGenDir,
                    pythonFiles.wrapperSourceFile);
        auto wrapperFile = new model::FileSystemObject_t(wrapperSrcPath,
                wrapperDestPath,
                wrapperPerms);

        componentPtr->bundledFiles.insert(
                std::shared_ptr<model::FileSystemObject_t>(wrapperFile));
    }
    for (auto pyPkgPtr : componentPtr->pythonPackages)
    {
        model::Permissions_t pyPerms(1, 0, 1);

        auto pyDestPath =  "bin/"
            + path::Combine(componentPtr->name, pyPkgPtr->packageName);
        auto pySrcPath = pyPkgPtr->packagePath;
        auto pyFile = new model::FileSystemObject_t(pySrcPath, pyDestPath, pyPerms);

        if (path::IsPythonSource(pyPkgPtr->packagePath))
        {
            // It's just a single-file module.
            componentPtr->bundledFiles.insert(
                    std::shared_ptr<model::FileSystemObject_t>(pyFile));
        }
        else
        {
            componentPtr->bundledDirs.insert(
                    std::shared_ptr<model::FileSystemObject_t>(pyFile));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Adds the contents of a "cflags:" section to the list of cFlags for a given Component_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddCFlags
(
    model::Component_t* componentPtr,
    parseTree::CompoundItem_t* sectionPtr  ///< Parse tree object for the section.
)
//--------------------------------------------------------------------------------------------------
{
    auto tokenListPtr = static_cast<parseTree::TokenList_t*>(sectionPtr);

    // The section contains a list of FILE_PATH tokens.
    for (auto contentPtr: tokenListPtr->Contents())
    {
        componentPtr->cFlags.push_back(DoSubstitution(contentPtr));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the contents of a "cxxflags:" section to the list of cxxFlags for a given Component_t.
 */
//--------------------------------------------------------------------------------------------------
static void AddCxxFlags
(
    model::Component_t* componentPtr,
    parseTree::CompoundItem_t* sectionPtr  ///< Parse tree object for the section.
)
//--------------------------------------------------------------------------------------------------
{
    auto tokenListPtr = static_cast<parseTree::TokenList_t*>(sectionPtr);

    // The section contains a list of FILE_PATH tokens.
    for (auto contentPtr: tokenListPtr->Contents())
    {
        componentPtr->cxxFlags.push_back(DoSubstitution(contentPtr));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the contents of a "ldflags:" section to the list of ldFlags for a given Component_t.
 */
//--------------------------------------------------------------------------------------------------
static void AddLdFlags
(
    model::Component_t* componentPtr,
    parseTree::CompoundItem_t* sectionPtr  ///< Parse tree object for the section.
)
//--------------------------------------------------------------------------------------------------
{
    auto tokenListPtr = static_cast<parseTree::TokenList_t*>(sectionPtr);

    // The section contains a list of FILE_PATH tokens.
    for (auto contentPtr: tokenListPtr->Contents())
    {
        componentPtr->ldFlags.push_back(DoSubstitution(contentPtr));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the items from a given "bundles:" section to a given Component_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddBundledItems
(
    model::Component_t* componentPtr,
    const parseTree::CompoundItem_t* sectionPtr,   ///< Ptr to "bundles:" section in the parse tree.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Bundles section is comprised of subsections (either "file:" or "dir:") which all have
    // the same basic structure (ComplexSection_t).
    // "file:" sections contain BundledFile_t objects (with type BUNDLED_FILE).
    // "dir:" sections contain BundledDir_t objects (with type BUNDLED_DIR).
    for (auto memberPtr : static_cast<const parseTree::ComplexSection_t*>(sectionPtr)->Contents())
    {
        auto subsectionPtr = parseTree::ToCompoundItemListPtr(memberPtr);

        if (subsectionPtr->Name() == "file")
        {
            for (auto itemPtr : subsectionPtr->Contents())
            {
                auto bundledFileTokenListPtr = parseTree::ToTokenListPtr(itemPtr);

                auto bundledFilePtr = GetBundledItem(bundledFileTokenListPtr);

                // If the source path is not absolute, then it is relative to the directory
                // containing the .cdef file.
                if (!path::IsAbsolute(bundledFilePtr->srcPath))
                {
                    bundledFilePtr->srcPath = path::Combine(componentPtr->dir,
                                                            bundledFilePtr->srcPath);
                }

                // Always add files -- will be checked when bundled as it could be generated by an
                // externalBuild command.
                componentPtr->bundledFiles.insert(
                    std::shared_ptr<model::FileSystemObject_t>(bundledFilePtr));
            }
        }
        else if (subsectionPtr->Name() == "dir")
        {
            for (auto itemPtr : subsectionPtr->Contents())
            {
                auto bundledDirTokenListPtr = parseTree::ToTokenListPtr(itemPtr);

                auto bundledDirPtr = GetBundledItem(bundledDirTokenListPtr);

                // If the source path is not absolute, then it is relative to the directory
                // containing the .cdef file.
                if (!path::IsAbsolute(bundledDirPtr->srcPath))
                {
                    bundledDirPtr->srcPath = path::Combine(componentPtr->dir,
                                                           bundledDirPtr->srcPath);
                }

                // Always add the directories.
                // Directory existence is checked later when building the script.
                componentPtr->bundledDirs.insert(
                    std::shared_ptr<model::FileSystemObject_t>(bundledDirPtr));
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
 * If a given .api has any USETYPES statements in it, add those to a given set of
 * USETYPES included .api files.
 */
//--------------------------------------------------------------------------------------------------
static void GetUsetypesApis
(
    std::set<const model::ApiFile_t*>& set,    ///< Set to add the USETYPES-included .api files to.
    model::ApiFile_t* apiFilePtr
)
//--------------------------------------------------------------------------------------------------
{
    apiFilePtr->GetUsetypesApis(set);
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a server-side IPC API instance to a component for a given provided api in the parse tree.
 */
//--------------------------------------------------------------------------------------------------
static void GetProvidedApi
(
    model::Component_t* componentPtr,
    const parseTree::TokenList_t* itemPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string internalName;
    std::string apiFilePath;

    // If the first content item is a NAME, then it is the internal alias and the API file path
    // will follow.
    const auto& contentList = itemPtr->Contents();
    if (contentList[0]->type == parseTree::Token_t::NAME)
    {
        internalName = contentList[0]->text;
        apiFilePath = file::FindFile(DoSubstitution(contentList[1]), buildParams.interfaceDirs);
        if (apiFilePath == "")
        {
            contentList[1]->ThrowException(
                mk::format(LE_I18N("Couldn't find file '%s'."), contentList[1]->text)
            );
        }
    }
    // If the first content item is not a NAME, then it must be the file path.
    else
    {
        apiFilePath = file::FindFile(DoSubstitution(contentList[0]), buildParams.interfaceDirs);
        if (apiFilePath == "")
        {
            contentList[0]->ThrowException(
                mk::format(LE_I18N("Couldn't find file '%s'."), contentList[0]->text)
            );
        }
    }

    // Check for options.
    bool async = false;
    bool manualStart = false;
    bool direct = false;
    for (auto contentPtr : contentList)
    {
        if (contentPtr->type == parseTree::Token_t::SERVER_IPC_OPTION)
        {
            if (contentPtr->text == "[async]")
            {
                async = true;
            }
            else if (contentPtr->text == "[manual-start]")
            {
                manualStart = true;
            }
            else if (contentPtr->text == "[direct]")
            {
                direct = true;
            }
        }
    }

    if (direct && async)
    {
        itemPtr->ThrowException(LE_I18N("Can't use [direct] with [async]"
                                  " for the same interface."));
    }

    // Get a pointer to the .api file object.
    auto apiFilePtr = GetApiFilePtr(apiFilePath, buildParams.interfaceDirs, contentList[0]);

    // If no internal alias was specified, then use the .api file's default prefix.
    if (internalName.empty())
    {
        internalName = apiFilePtr->defaultPrefix;
    }

    // Create the new interface object and add it to the appropriate list.
    auto ifPtr = new model::ApiServerInterface_t(itemPtr,
                                                 apiFilePtr,
                                                 componentPtr,
                                                 internalName,
                                                 async);
    ifPtr->manualStart = manualStart;
    ifPtr->direct = direct;

    componentPtr->serverApis.push_back(ifPtr);

    // If the .api has any USETYPES statements in it, add those to the component's list
    // of server-side USETYPES included .api files.
    GetUsetypesApis(componentPtr->serverUsetypesApis, apiFilePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Processes an item from the "lib:" subsection of the "provided:" and "requires:" section.
 */
//--------------------------------------------------------------------------------------------------
static void AddLib
(
    model::Component_t* providedToComponentPtr,
    model::Component_t* providedFromComponentPtr,
    const mk::BuildParams_t& buildParams,
    std::string lib
)
//--------------------------------------------------------------------------------------------------
{
    // Skip if environment variable substitution resulted in an empty string.
    if (!lib.empty())
    {
        // If the library specifier ends in a .a extension, it is a static library.
        if (path::HasSuffix(lib, ".a"))
        {
            // Assume relative paths are build outputs, and should be searched in generated
            // library search path
            if (lib.find('/') == std::string::npos)
            {
                lib = "-l" + path::GetLibShortName(lib);
            }
            providedToComponentPtr->staticLibs.insert(lib);
        }
        // If it's not a .a file,
        else
        {
            // If the library specifier contains a ".so" extension,
            if (lib.find(".so") != std::string::npos)
            {
                // Try to find it relative to the component directory or the library output
                // directory.
                std::list<std::string> searchDirs =
                        { providedFromComponentPtr->dir, buildParams.libOutputDir };

                auto libPath = file::FindFile(lib, searchDirs);

                // If found,
                if (!libPath.empty())
                {
                    // Add the library to the list of the component's implicit dependencies.
                    providedToComponentPtr->implicitDependencies.insert(libPath);

                    // Add a -L ldflag for the directory that the library is in.
                    providedToComponentPtr->ldFlags.push_back("-L" +
                                                              path::GetContainingDir(libPath));
                }

                // Compute the short name for this library.
                lib = path::GetLibShortName(lib);
            }

            // Add a -l option to the component's LDFLAGS.
            providedToComponentPtr->ldFlags.push_back("-l" + lib);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get header search directory paths from a "headerDir:" section and add them to searchPathList.
 */
//--------------------------------------------------------------------------------------------------
static void ReadSearchDirs
(
    std::list<std::string>& searchPathList,   ///< And add the new search paths to this list.
    const parseTree::TokenListSection_t* sectionPtr  ///< From the search paths from this section.
)
//--------------------------------------------------------------------------------------------------
{
   for (const auto tokenPtr : sectionPtr->Contents())
   {
        auto dirPath = path::Unquote(DoSubstitution(tokenPtr));

        // If the environment variable substitution resulted in an empty string, just ignore this.
        if (!dirPath.empty())
        {
            searchPathList.push_back(dirPath);
        }

    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the items from a given "provides:" section to a given Component_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddProvidedItems
(
    model::Component_t* componentPtr,
    const parseTree::CompoundItem_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // "provides:" section is comprised of subsections.
    for (auto memberPtr : static_cast<const parseTree::ComplexSection_t*>(sectionPtr)->Contents())
    {
        auto subsectionName = memberPtr->firstTokenPtr->text;

        if (subsectionName == "api")
        {
            auto subsectionPtr = parseTree::ToCompoundItemListPtr(memberPtr);
            for (auto itemPtr : subsectionPtr->Contents())
            {
                auto apiPtr = parseTree::ToTokenListPtr(itemPtr);

                GetProvidedApi(componentPtr, apiPtr, buildParams);
            }
        }
        else if (subsectionName == "headerDir")
        {
           ReadSearchDirs(componentPtr->headerDirs, parseTree::ToTokenListSectionPtr(memberPtr));
        }
        else if (subsectionName == "lib")
        {
            auto subsectionPtr = parseTree::ToTokenListPtr(memberPtr);
            for (auto itemPtr : subsectionPtr->Contents())
            {
                componentPtr->providedLibs.insert(DoSubstitution(itemPtr));
            }
        }
        else
        {
            memberPtr->ThrowException(
                mk::format(LE_I18N("Internal error: Unexpected provided item: %s."), subsectionName)
            );
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a client-side IPC API instance to a component for a given required API in the parse tree.
 */
//--------------------------------------------------------------------------------------------------
static void GetRequiredApi
(
    model::Component_t* componentPtr,
    const parseTree::TokenList_t* itemPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    const auto& contentList = itemPtr->Contents();

    // If the first content item is a NAME, then it is the internal alias and the API file path
    // will follow.
    std::string internalName;
    std::string apiFilePath;
    if (contentList[0]->type == parseTree::Token_t::NAME)
    {
        internalName = contentList[0]->text;
        apiFilePath = file::FindFile(DoSubstitution(contentList[1]), buildParams.interfaceDirs);
        if (apiFilePath == "")
        {
            contentList[1]->ThrowException(
                mk::format(LE_I18N("Couldn't find file '%s'."), contentList[1]->text)
            );
        }
    }
    // If the first content item is not a NAME, then it must be the file path.
    else
    {
        auto substString = DoSubstitution(contentList[0]);
        apiFilePath = file::FindFile(substString, buildParams.interfaceDirs);
        if (apiFilePath == "")
        {
            contentList[0]->ThrowException(mk::format(LE_I18N("Couldn't find file '%s.'"),
                                                      substString));
        }
    }

    // Check for options.
    bool typesOnly = false;
    bool manualStart = false;
    bool optional = false;
    for (auto contentPtr : contentList)
    {
        if (contentPtr->type == parseTree::Token_t::CLIENT_IPC_OPTION)
        {
            if (contentPtr->text == "[types-only]")
            {
                typesOnly = true;
            }
            else if (contentPtr->text == "[manual-start]")
            {
                manualStart = true;
            }
            else if (contentPtr->text == "[optional]")
            {
                manualStart = true; // [optional] implies [manual-start].
                optional = true;
            }
        }
    }
    if (typesOnly && manualStart)
    {
        itemPtr->ThrowException(LE_I18N("Can't use [types-only] with [manual-start] or [optional]"
                                  " for the same interface."));
    }

    // Get a pointer to the .api file object.
    auto apiFilePtr = GetApiFilePtr(apiFilePath, buildParams.interfaceDirs, contentList[0]);

    // If no internal alias was specified, then use the .api file's default prefix.
    if (internalName.empty())
    {
        internalName = apiFilePtr->defaultPrefix;
    }

    // If we are only importing data types from this .api file.
    if (typesOnly)
    {
        auto ifPtr = new model::ApiTypesOnlyInterface_t(itemPtr,
                                                        apiFilePtr, componentPtr, internalName);

        componentPtr->typesOnlyApis.push_back(ifPtr);
    }
    else
    {
        auto ifPtr = new model::ApiClientInterface_t(itemPtr,
                                                     apiFilePtr, componentPtr, internalName);

        ifPtr->manualStart = manualStart;
        ifPtr->optional = optional;

        componentPtr->clientApis.push_back(ifPtr);
    }

    // If the .api has any USETYPES statements in it, add those to the component's list
    // of client-side USETYPES included .api files.
    GetUsetypesApis(componentPtr->clientUsetypesApis, apiFilePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Processes an item from the "component:" subsection of the "requires:" section.
 */
//--------------------------------------------------------------------------------------------------
static void GetRequiredComponent
(
    model::Component_t* componentPtr,
    const parseTree::TokenList_t* itemPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    const auto& contentList = itemPtr->Contents();

    // Check for provide-header option, the only option available.
    bool isProvideHeader = false;
    model::Component_t* subcomponentPtr = NULL;
    for (auto contentPtr : contentList)
    {
        if (contentPtr->type == parseTree::Token_t::PROVIDE_HEADER_OPTION)
        {
            if (contentPtr->text == "[provide-header]")
            {
                isProvideHeader = true;
            }
        }
        else
        {
            subcomponentPtr = GetComponent(contentPtr, buildParams, { componentPtr->dir });
        }
    }

    if (subcomponentPtr != NULL)
    {
        model::Component_t::ComponentProvideHeader_t subComp;
        subComp.componentPtr = subcomponentPtr;
        subComp.isProvideHeader = isProvideHeader;

        componentPtr->subComponents.push_back(subComp);

        if (!subcomponentPtr->providedLibs.empty())
        {
            // If the subcomponent provides libraries then process them.
            for (auto &lib : subcomponentPtr->providedLibs)
            {
                AddLib(componentPtr, subcomponentPtr, buildParams, lib);
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the items from a given "requires:" section to a given Component_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddRequiredItems
(
    model::Component_t* componentPtr,
    const parseTree::CompoundItem_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::list<const parseTree::CompoundItem_t*> reqKernelModulesSections;

    // "requires:" section is comprised of subsections,
    for (auto memberPtr : static_cast<const parseTree::ComplexSection_t*>(sectionPtr)->Contents())
    {
        const std::string& subsectionName = memberPtr->firstTokenPtr->text;

        if (subsectionName == "api")
        {
            auto subsectionPtr = parseTree::ToCompoundItemListPtr(memberPtr);
            for (auto itemPtr : subsectionPtr->Contents())
            {
                auto apiPtr = parseTree::ToTokenListPtr(itemPtr);

                GetRequiredApi(componentPtr, apiPtr, buildParams);
            }
        }
        else if (subsectionName == "file")
        {
            auto subsectionPtr = parseTree::ToCompoundItemListPtr(memberPtr);
            for (auto itemPtr : subsectionPtr->Contents())
            {
                auto fileSpecPtr = parseTree::ToTokenListPtr(itemPtr);

                componentPtr->requiredFiles.insert(
                    std::shared_ptr<model::FileSystemObject_t>(GetRequiredFile(fileSpecPtr)));
            }
        }
        else if (subsectionName == "dir")
        {
            auto subsectionPtr = parseTree::ToCompoundItemListPtr(memberPtr);
            for (auto itemPtr : subsectionPtr->Contents())
            {
                auto dirSpecPtr = parseTree::ToTokenListPtr(itemPtr);

                componentPtr->requiredDirs.insert(
                    std::shared_ptr<model::FileSystemObject_t>(GetRequiredDir(dirSpecPtr)));
            }
        }
        else if (subsectionName == "device")
        {
            auto subsectionPtr = parseTree::ToCompoundItemListPtr(memberPtr);

            for (auto itemPtr : subsectionPtr->Contents())
            {
                auto deviceSpecPtr = parseTree::ToTokenListPtr(itemPtr);

                componentPtr->requiredDevices.insert(
                    std::shared_ptr<model::FileSystemObject_t>(GetRequiredDevice(deviceSpecPtr)));
            }
        }
        else if (subsectionName == "component")
        {
            auto subsectionPtr = parseTree::ToCompoundItemListPtr(memberPtr);
            for (auto itemPtr : subsectionPtr->Contents())
            {
                auto subitemPtr = parseTree::ToTokenListPtr(itemPtr);

                GetRequiredComponent(componentPtr, subitemPtr, buildParams);

            }
        }
        else if (subsectionName == "lib")
        {
            auto subsectionPtr = parseTree::ToTokenListPtr(memberPtr);
            for (auto itemPtr : subsectionPtr->Contents())
            {
                AddLib(componentPtr, componentPtr, buildParams, DoSubstitution(itemPtr));
            }
        }
        else if (parser::IsNameSingularPlural(subsectionName, "kernelModule"))
        {
            reqKernelModulesSections.push_back(memberPtr);
        }
        else
        {
            memberPtr->ThrowException(
                mk::format(LE_I18N("Internal error: Unexpected required item: %s."), subsectionName)
            );
        }
    }

    AddRequiredKernelModules(componentPtr->requiredModules, nullptr, reqKernelModulesSections,
                             buildParams);

}


//--------------------------------------------------------------------------------------------------
/**
 * Print a summary of a component model.
 **/
//--------------------------------------------------------------------------------------------------
static void PrintSummary
(
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    std::cout << mk::format(LE_I18N("== '%s' component summary =="), componentPtr->name)
              << std::endl;

    if (!componentPtr->cObjectFiles.empty())
    {
        std::cout << LE_I18N("  C sources:") << std::endl;

        for (auto objFilePtr : componentPtr->cObjectFiles)
        {
            std::cout << mk::format(LE_I18N("    '%s'"), objFilePtr->sourceFilePath)
                      << std::endl;
        }
    }

    if (!componentPtr->cxxObjectFiles.empty())
    {
        std::cout << LE_I18N("  C++ sources:") << std::endl;

        for (auto objFilePtr : componentPtr->cxxObjectFiles)
        {
            std::cout << mk::format(LE_I18N("    '%s'"), objFilePtr->sourceFilePath)
                      << std::endl;
        }
    }

    if (!componentPtr->subComponents.empty())
    {
        std::cout << LE_I18N("  Depends on components:") << std::endl;

        for (auto subComponentPtr : componentPtr->subComponents)
        {
            std::cout << mk::format(LE_I18N("    '%s'"), subComponentPtr.componentPtr->name)
                      << std::endl;

            if (subComponentPtr.isProvideHeader)
            {
                std::cout << LE_I18N("      provide headers from this component.")
                          << std::endl;
            }
        }
    }

    if (!componentPtr->headerDirs.empty())
    {
        std::cout << LE_I18N("  Provides header directory:") << std::endl;

        for (const auto &headerDirsPtr : componentPtr->headerDirs)
        {
            std::cout << mk::format(LE_I18N("    '%s'"), headerDirsPtr) << std::endl;
        }
    }


    if (!componentPtr->providedLibs.empty())
    {
        std::cout << LE_I18N("  Provides libraries:") << std::endl;

        for (const auto &providedLibPtr : componentPtr->providedLibs)
        {
            std::cout << mk::format(LE_I18N("    '%s'"), providedLibPtr) << std::endl;
        }
    }


    if (!componentPtr->bundledFiles.empty())
    {
        std::cout << LE_I18N("  Includes files from the build host:") << std::endl;

        for (auto itemPtr : componentPtr->bundledFiles)
        {
            std::cout << mk::format(LE_I18N("    '%s':"), itemPtr->srcPath) << std::endl;
            std::cout << mk::format(LE_I18N("      appearing inside app as: '%s'"),
                                    itemPtr->destPath)
                      << std::endl;
            std::cout << LE_I18N("      permissions:");
            PrintPermissions(itemPtr->permissions);
            std::cout << std::endl;
        }
    }

    if (!componentPtr->bundledDirs.empty())
    {
        std::cout << LE_I18N("  Includes directories from the build host:") << std::endl;

        for (auto itemPtr : componentPtr->bundledDirs)
        {
            std::cout << mk::format(LE_I18N("    '%s':"), itemPtr->srcPath) << std::endl;
            std::cout << mk::format(LE_I18N("      appearing inside app as: '%s'"),
                                    itemPtr->destPath)
                      << std::endl;
            std::cout << LE_I18N("      permissions:");
            PrintPermissions(itemPtr->permissions);
            std::cout << std::endl;
        }
    }

    if (!componentPtr->requiredFiles.empty())
    {
        std::cout << LE_I18N("  Imports files from the target host:") << std::endl;

        for (auto itemPtr : componentPtr->requiredFiles)
        {
            std::cout << mk::format(LE_I18N("    '%s':"), itemPtr->srcPath) << std::endl;
            std::cout << mk::format(LE_I18N("      appearing inside app as: '%s'"),
                                    itemPtr->destPath)
                      << std::endl;
        }
    }

    if (!componentPtr->requiredDirs.empty())
    {
        std::cout << LE_I18N("  Imports directories from the target host:") << std::endl;

        for (auto itemPtr : componentPtr->requiredDirs)
        {
            std::cout << mk::format(LE_I18N("    '%s':"), itemPtr->srcPath) << std::endl;
            std::cout << mk::format(LE_I18N("      appearing inside app as: '%s'"),
                                    itemPtr->destPath)
                      << std::endl;
        }
    }

    if (!componentPtr->typesOnlyApis.empty())
    {
        std::cout << LE_I18N("  Type definitions imported from:") << std::endl;

        for (auto itemPtr : componentPtr->typesOnlyApis)
        {
            std::cout << mk::format(LE_I18N("    '%s'"), itemPtr->apiFilePtr->path) << std::endl;
            std::cout << mk::format(LE_I18N("      With identifier prefix: '%s':"),
                                    itemPtr->internalName)
                      << std::endl;
        }
    }

    if (!componentPtr->clientApis.empty())
    {
        std::cout << LE_I18N("  IPC API client-side interfaces:") << std::endl;

        for (auto itemPtr : componentPtr->clientApis)
        {
            std::cout << mk::format(LE_I18N("    '%s':"), itemPtr->internalName) << std::endl;
            std::cout << mk::format(LE_I18N("      API defined in: '%s'"),
                                    itemPtr->apiFilePtr->path)
                      << std::endl;
            if (itemPtr->manualStart)
            {
                std::cout << LE_I18N("      Automatic service connection at start-up suppressed.")
                          << std::endl;
            }
            if (itemPtr->optional)
            {
                std::cout << LE_I18N("      Binding this to a service is optional.")
                          << std::endl;
            }
        }
    }

    if (!componentPtr->serverApis.empty())
    {
        std::cout << LE_I18N("  IPC API server-side interfaces:") << std::endl;

        for (auto itemPtr : componentPtr->serverApis)
        {
            std::cout << mk::format(LE_I18N("    '%s':"), itemPtr->internalName) << std::endl;
            std::cout << mk::format(LE_I18N("      API defined in: '%s'"), itemPtr->internalName)
                      << std::endl;
            if (itemPtr->async)
            {
                std::cout << LE_I18N("      Asynchronous server-side processing mode selected.")
                          << std::endl;
            }
            if (itemPtr->manualStart)
            {
                std::cout << LE_I18N("      Automatic service advertisement at start-up"
                                     " suppressed.")
                          << std::endl;
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove API from given set if the API is already present in client or server required APIs.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SanitizeApiSet
(
    model::Component_t* componentPtr,
    std::set<const model::ApiFile_t*>& apiSet
)
//--------------------------------------------------------------------------------------------------
{
    auto iter = apiSet.begin();
    auto current = iter;
    while(iter != apiSet.end())
    {
        // Save the current iterator, then increment it.
        current = iter++;
        bool forceRemove = false;
        for (auto comp : componentPtr->clientApis)
        {
            if (comp->internalName == (*current)->defaultPrefix)
            {
                forceRemove = true;
                break;
            }
        }
        if (!forceRemove)
        {
            for (auto comp : componentPtr->serverApis)
            {
                if (comp->internalName == (*current)->defaultPrefix && !comp->async)
                {
                    forceRemove = true;
                    break;
                }
            }
        }
        if (forceRemove)
        {
            apiSet.erase(current);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a single component residing in a given directory.
 *
 * @return Pointer to the component object.
 */
//--------------------------------------------------------------------------------------------------
model::Component_t* GetComponent
(
    const std::string& componentDir,    ///< Directory the component is found in.
    const mk::BuildParams_t& buildParams
)
{
    return GetComponent(componentDir, buildParams, buildParams.isStandAloneComp);
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a single component residing in a given directory.
 *
 * @return Pointer to the component object.
 */
//--------------------------------------------------------------------------------------------------
model::Component_t* GetComponent
(
    const std::string& componentDir,    ///< Directory the component is found in.
    const mk::BuildParams_t& buildParams,
    bool isStandAloneComp
)
//--------------------------------------------------------------------------------------------------
{
    // If component has already been modelled, return a pointer to the previously created object.
    auto componentPtr = model::Component_t::GetComponent(componentDir);
    if (componentPtr != NULL)
    {
        // Verify Component Stand-alone state
        if (componentPtr->isStandAloneComp != isStandAloneComp)
        {
            componentPtr->defFilePtr->ThrowException(
                mk::format(LE_I18N("Internal error: Mismatching stand-alone component state"),
                           componentPtr->isStandAloneComp)
            );
        }
        return componentPtr;
    }

    // Parse the .cdef file.
    auto cdefFilePath = path::Combine(componentDir, "Component.cdef");
    auto cdefFilePtr = parser::cdef::Parse(cdefFilePath, buildParams.beVerbose);

    // Create a new object for this component.
    // By default, it will be built in a sub-directory called "component/<compName>" under the
    // build's root working directory.
    componentPtr = model::Component_t::CreateComponent(cdefFilePtr);

    file::MakeDir(path::Combine(buildParams.workingDir, componentPtr->workingDir));

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Modelling component: '%s'\n"
                                  "  found at: '%s'"),
                                  componentPtr->name, componentPtr->dir)
                  << std::endl;
    }

    if (isStandAloneComp)
    {
        // Flag this component as a stand-alone component
        componentPtr->isStandAloneComp = true;
    }

    // Set BUILDDIR environment variable for this component
    SetComponentBuildDirEnvVar(componentPtr, buildParams);

    // Iterate over the .cdef file's list of sections.
    for (auto sectionPtr : cdefFilePtr->sections)
    {
        const std::string& sectionName = sectionPtr->firstTokenPtr->text;

        if (sectionName == "externalBuild")
        {
            AddExternalBuild(componentPtr, sectionPtr);
        }
        else if (sectionName == "sources")
        {
            AddSources(componentPtr, sectionPtr, buildParams);
        }
        else if (sectionName == "javaPackage")
        {
            AddJavaPackage(componentPtr, sectionPtr, buildParams);
        }
        else if (sectionName == "pythonPackage")
        {
            AddPythonPackage(componentPtr, sectionPtr, buildParams);
        }
        else if (sectionName == "cflags")
        {
            AddCFlags(componentPtr, sectionPtr);
        }
        else if (sectionName == "cxxflags")
        {
            AddCxxFlags(componentPtr, sectionPtr);
        }
        else if (sectionName == "ldflags")
        {
            AddLdFlags(componentPtr, sectionPtr);
        }
        else if (sectionName == "bundles")
        {
            AddBundledItems(componentPtr, sectionPtr, buildParams);
        }
        else if (sectionName == "provides")
        {
            AddProvidedItems(componentPtr, sectionPtr, buildParams);
        }
        else if (sectionName == "requires")
        {
            AddRequiredItems(componentPtr, sectionPtr, buildParams);
        }
        else
        {
            sectionPtr->ThrowException(
                mk::format(LE_I18N("Internal error: Unrecognized section '%s'."), sectionName)
            );
        }
    }

    // In case of Java code generation, remove client/server use types API
    // which are already required to avoid duplicate classes
    if (componentPtr->HasJavaCode()) {
        SanitizeApiSet(componentPtr, componentPtr->clientUsetypesApis);
        SanitizeApiSet(componentPtr, componentPtr->serverUsetypesApis);
    }

    // Build a path to the Legato library dir and the Legato lib so that we can add it as a
    // dependency for code components.
    auto baseLibPath = path::Combine(envVars::Get("LEGATO_ROOT"),
                                                 "build/" + buildParams.target + "/framework/lib/");

    // If there are C sources or C++ sources, a library will be built for this component.
    if (componentPtr->HasCOrCppCode())
    {
        // It will have an init function that will need to be executed (unless it is built
        // "stand-alone").
        componentPtr->initFuncName = "_" + componentPtr->name + "_COMPONENT_INIT";
    }
    else if (componentPtr->HasJavaCode())
    {
        auto liblegatoPath = path::Combine(baseLibPath, "liblegato.so");

        // Changes to liblegato should trigger re-linking of the component library.
        auto legatoJarPath = path::Combine(baseLibPath, "legato.jar");
        auto liblegatoJniPath = path::Combine(baseLibPath, "liblegatoJni.so");

        componentPtr->implicitDependencies.insert(liblegatoPath);
        componentPtr->implicitDependencies.insert(legatoJarPath);
        componentPtr->implicitDependencies.insert(liblegatoJniPath);
    }

    if (componentPtr->HasPythonCode())
    {
        // Add the python wrapper of each APIs
        AddPythonClientFiles(componentPtr);
    }

    if (buildParams.beVerbose)
    {
        PrintSummary(componentPtr);
    }

    // Unset BUILDDIR environment variable for this component
    envVars::Unset("BUILDDIR");

    return componentPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a single component residing in a directory specified by a given
 * FILE_PATH token.
 *
 * @return Pointer to the component object, or NULL if the token specifies an empty environment
 *         variable.
 *
 * @throw mkTools::Exception_t on error.
 */
//--------------------------------------------------------------------------------------------------
model::Component_t* GetComponent
(
    const parseTree::Token_t* tokenPtr,
    const mk::BuildParams_t& buildParams,
    const std::list<std::string>& preSearchDirs ///< Dirs to search before buildParams source dirs
)
{
    return GetComponent(tokenPtr, buildParams, preSearchDirs, buildParams.isStandAloneComp);
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a single component residing in a directory specified by a given
 * FILE_PATH token.
 *
 * @return Pointer to the component object, or NULL if the token specifies an empty environment
 *         variable.
 *
 * @throw mkTools::Exception_t on error.
 */
//--------------------------------------------------------------------------------------------------
model::Component_t* GetComponent
(
    const parseTree::Token_t* tokenPtr,
    const mk::BuildParams_t& buildParams,
    const std::list<std::string>& preSearchDirs, ///< Dirs to search before buildParams source dirs
    bool isStandAloneComp
)
//--------------------------------------------------------------------------------------------------
{
    // Resolve the path to the component.
    std::string componentPath = path::Unquote(DoSubstitution(tokenPtr));

    // Skip if environment variable substitution resulted in an empty string.
    if (componentPath.empty())
    {
        return NULL;
    }

    auto resolvedPath = file::FindComponent(componentPath, preSearchDirs);
    if (resolvedPath.empty())
    {
        resolvedPath = file::FindComponent(componentPath, buildParams.componentDirs);
    }
    if (resolvedPath.empty())
    {
        tokenPtr->ThrowException(
            mk::format(LE_I18N("Couldn't find component '%s'."), componentPath)
        );
    }

    // Get the component object.
    return GetComponent(path::MakeAbsolute(resolvedPath), buildParams, isStandAloneComp);
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds an instance of a given component to a given executable.
 **/
//--------------------------------------------------------------------------------------------------
void AddComponentInstance
(
    model::Exe_t* exePtr,
    model::Component_t* componentPtr
)
//--------------------------------------------------------------------------------------------------
{
    // If there is already an instance of this component in this executable, ignore this.
    // Someday we may support multiple instances of the same component, but not today.
    for (auto instancePtr : exePtr->componentInstances)
    {
        if (instancePtr->componentPtr == componentPtr)
        {
            return;
        }
    }

    // Recursively add instances of any sub-components to the executable first, so the exe's
    // resulting component instance list will be sorted in the order in which the component
    // initialization functions must be called.
    for (auto subComponentPtr : componentPtr->subComponents)
    {
        AddComponentInstance(exePtr, subComponentPtr.componentPtr);
    }

    // Add an instance of the component to the executable.
    auto componentInstancePtr = new model::ComponentInstance_t(exePtr, componentPtr);
    exePtr->AddComponentInstance(componentInstancePtr);

    // For each of the component's client-side interfaces, create an interface instance.
    for (auto ifPtr : componentPtr->clientApis)
    {
        auto ifInstancePtr = new model::ApiClientInterfaceInstance_t(componentInstancePtr, ifPtr);
        componentInstancePtr->clientApis.push_back(ifInstancePtr);
    }

    // For each of the component's server-side interfaces, create an interface instance.
    for (auto ifPtr : componentPtr->serverApis)
    {
        auto ifInstancePtr = new model::ApiServerInterfaceInstance_t(componentInstancePtr, ifPtr);
        componentInstancePtr->serverApis.push_back(ifInstancePtr);

        ifInstancePtr->name = exePtr->name + '.' + componentPtr->name + '.' + ifPtr->internalName;
    }
}


} // namespace modeller
