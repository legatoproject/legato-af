//--------------------------------------------------------------------------------------------------
/**
 * @file appModeller.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "defTools.h"
#include "modellerCommon.h"
#include "componentModeller.h"
#include "envVars.h"


namespace modeller
{

//--------------------------------------------------------------------------------------------------
/**
 * Print a warning message to stderr for a given app.
 **/
//--------------------------------------------------------------------------------------------------
static void PrintWarning
(
    const model::App_t* appPtr,
    const std::string& warning
)
//--------------------------------------------------------------------------------------------------
{
    std::cerr << mk::format(LE_I18N("** WARNING: application %s: %s"), appPtr->name, warning)
              << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Print a note to stderr for a given app.
 **/
//--------------------------------------------------------------------------------------------------
static void PrintNote
(
    const model::App_t* appPtr,
    const std::string& note
)
//--------------------------------------------------------------------------------------------------
{
    std::cerr << mk::format(LE_I18N("** NOTE: application %s: %s"), appPtr->name, note)
              << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check if a process name already exists in an app
 */
//--------------------------------------------------------------------------------------------------
static bool DoesProcessExist
(
    model::App_t* appPtr,
    const std::string &processName
)
{
    for (auto processEnvPtr : appPtr->processEnvs)
    {
        for (auto processPtr : processEnvPtr->processes)
        {
            if (processPtr->GetName() == processName)
            {
                return true;
            }
        }
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the items from a given "bundles:" section to a given App_t object.
 */
//--------------------------------------------------------------------------------------------------
static void AddBundledItems
(
    model::App_t* appPtr,
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
                    bundledFilePtr->srcPath = path::Combine(appPtr->dir, bundledFilePtr->srcPath);
                }

                // Make sure that the source path exists and is a file.
                if (file::FileExists(bundledFilePtr->srcPath))
                {
                    appPtr->bundledFiles.insert(
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
                    bundledDirPtr->srcPath = path::Combine(appPtr->dir, bundledDirPtr->srcPath);
                }

                // Make sure that the source path exists and is a directory.
                if (file::DirectoryExists(bundledDirPtr->srcPath))
                {
                    appPtr->bundledDirs.insert(
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
        else if (subsectionPtr->Name() == "binary")
        {
            for (auto itemPtr : subsectionPtr->Contents())
            {
                auto bundledBinaryTokenListPtr = parseTree::ToTokenListPtr(itemPtr);
                auto bundledBinaryPtr = GetBundledItem(bundledBinaryTokenListPtr);

                // Binary paths are never absolute
                bundledBinaryPtr->srcPath = path::Combine(appPtr->dir, bundledBinaryPtr->srcPath);

                appPtr->bundledBinaries.insert(
                    std::shared_ptr<model::FileSystemObject_t>(bundledBinaryPtr));
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
 * Adds to the app the components listed in a given "components" section in the parse tree.
 */
//--------------------------------------------------------------------------------------------------
static void AddComponents
(
    model::App_t* appPtr,
    const parseTree::TokenListSection_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Iterate over the list of contents of the section in the parse tree and add each item
    // as a component.
    for (auto tokenPtr : sectionPtr->Contents())
    {
        // Get the component object.
        auto componentPtr = GetComponent(tokenPtr, buildParams, { appPtr->dir });

        // Skip if environment variable substitution resulted in an empty string.
        if (componentPtr != NULL)
        {
            if (buildParams.beVerbose)
            {
                std::cout << "Application '" << appPtr->name << "' contains component '"
                          << componentPtr->name
                          << "' (" << componentPtr->dir << ")." << std::endl;
            }

            // Add the component to the app's list of components.
            appPtr->components.insert(componentPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a list of all provided in-place APIs in a component, and add to a map.
 */
//--------------------------------------------------------------------------------------------------
static void FindDirectServers
(
    const model::ComponentInstance_t *componentPtr,
    std::map<std::string, model::ApiServerInterfaceInstance_t*> &directServers
)
{
    for (const auto serverApiPtr : componentPtr->serverApis)
    {
        if (serverApiPtr->ifPtr->direct)
        {
            auto newServer =
                directServers.insert(std::make_pair(serverApiPtr->ifPtr->apiFilePtr->defaultPrefix,
                                                     serverApiPtr));
            if (!newServer.second)
            {
                serverApiPtr->ifPtr->itemPtr->ThrowException(
                    mk::format(LE_I18N("Direct API '%s' conflicts with previous definition at"
                                       " %s"),
                               serverApiPtr->ifPtr->internalName,
                               newServer.first->second->ifPtr->itemPtr->Contents()[0]->GetLocation()
                    )
                );
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  Search for clients of a direct server that are out of order.
 *
 *  @return True if there is an out of order dependency.
 */
//--------------------------------------------------------------------------------------------------
static bool ApiDependencyPrecedesServer
(
    model::Exe_t                 *exePtr,           ///< [IN]   Executable instance to examine.
    const std::string            &apiName,          ///< [IN]   API prefix to search for
                                                    ///         dependencies of.
    model::ComponentInstance_t   *serverInstPtr,    ///< [IN]   Server component.
    model::ComponentInstance_t  *&foundInstancePtr  ///< [OUT]  Out-of-order client component, if
                                                    ///         found.
)
{
    auto apiMatch = [&apiName](model::ApiClientInterfaceInstance_t *clientPtr)
    {
        return clientPtr->ifPtr->apiFilePtr->defaultPrefix == apiName;
    };

    for (auto it = exePtr->componentInstances.rbegin();
        it != exePtr->componentInstances.rend() && *it != serverInstPtr;
        ++it)
    {
        auto found = std::find_if((*it)->clientApis.begin(), (*it)->clientApis.end(), apiMatch);
        if (found != (*it)->clientApis.end())
        {
            foundInstancePtr = *it;
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a local binding of client interface to server interface
 */
//--------------------------------------------------------------------------------------------------
static void BindLocalInterface
(
    model::Exe_t* exePtr,
    model::ApiClientInterfaceInstance_t* clientIfacePtr,
    model::ApiServerInterfaceInstance_t* serverIfacePtr
)
{
    // Done before bindings are set, so should have no bindings here.
    if (clientIfacePtr->bindingPtr)
    {
        throw mk::Exception_t(LE_I18N("Internal Error: early binding definition"));
    }

    model::Binding_t* bindingPtr = new model::Binding_t(NULL);

    bindingPtr->clientType = model::Binding_t::LOCAL;
    if (exePtr->appPtr)
    {
        bindingPtr->clientAgentName = exePtr->appPtr->name;
        bindingPtr->serverAgentName = exePtr->appPtr->name;
    }

    bindingPtr->clientIfName = clientIfacePtr->ifPtr->internalName;
    bindingPtr->serverIfName = serverIfacePtr->ifPtr->internalName;

    bindingPtr->parseTreePtr = serverIfacePtr->ifPtr->itemPtr;

    clientIfacePtr->bindingPtr = bindingPtr;

    // And mark client as dependent on server
    clientIfacePtr->
        componentInstancePtr->
        requiredComponentInstances.insert(serverIfacePtr->componentInstancePtr);

    // Check for dependency ordering issues.
    model::ComponentInstance_t *foundInstancePtr;
    if (ApiDependencyPrecedesServer(exePtr, serverIfacePtr->ifPtr->apiFilePtr->defaultPrefix,
        serverIfacePtr->componentInstancePtr, foundInstancePtr))
    {
        exePtr->exeDefPtr->ThrowException(
            mk::format(
                LE_I18N("Client component '%s' of API '%s' found after direct server '%s'.  "
                    "Please reorder the required components."),
                foundInstancePtr->componentPtr->name,
                serverIfacePtr->ifPtr->apiFilePtr->defaultPrefix,
                serverIfacePtr->componentInstancePtr->componentPtr->name
            ));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 *  For direct API connections, automatically set up the bindings and component dependencies.
 */
//--------------------------------------------------------------------------------------------------
static void BindLocalInterfaces
(
    model::Exe_t* exePtr,
    model::ComponentInstance_t* componentInstPtr,
    std::map<std::string, model::ApiServerInterfaceInstance_t*> directServers
)
{
    for (auto clientIfacePtr : componentInstPtr->clientApis)
    {
        auto directServer = directServers.find(clientIfacePtr->ifPtr->apiFilePtr->defaultPrefix);
        if (directServer != directServers.end())
        {
            printf("%s: Creating local binding in '%s' from '%s' to '%s'\n",
                   exePtr->appPtr->defFilePtr->path.c_str(),
                   exePtr->name.c_str(),
                   clientIfacePtr->name.c_str(),
                   directServer->second->name.c_str());
            BindLocalInterface(exePtr, clientIfacePtr, directServer->second);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Any direct servers in an executable will automatically bind to corresponding required APIs
 * within the same executable.
 */
//--------------------------------------------------------------------------------------------------
static void AddLocalBindings
(
    model::Exe_t* exePtr
)
{
    std::map<std::string, model::ApiServerInterfaceInstance_t*> directServers;

    for (const auto componentInstPtr : exePtr->componentInstances)
    {
        FindDirectServers(componentInstPtr, directServers);
    }

    for (auto componentInstPtr : exePtr->componentInstances)
    {
        BindLocalInterfaces(exePtr, componentInstPtr, directServers);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds an Exe_t object to an application's list of executables, and makes sure all components
 * used by that executable are in the application's list of components.
 */
//--------------------------------------------------------------------------------------------------
void AddExecutable
(
    model::App_t* appPtr,
    model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    // Add the executable to the app.
    auto insertion = appPtr->executables.insert({ exePtr->name, exePtr });

    if (insertion.second == false)
    {
        exePtr->exeDefPtr->ThrowException(
            mk::format(LE_I18N("Duplicate executable found: %s."), exePtr->name)
        );
    }

    // Add all the components used in the executable to the app's list of components.
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        auto componentPtr = componentInstancePtr->componentPtr;
        appPtr->components.insert(componentPtr);
    }

    // If none of the components in the executable has any source code files, then the executable
    // would just sit there doing nothing, so throw an exception.
    if (   (exePtr->hasCOrCppCode == false)
        && (exePtr->hasJavaCode == false)
        && (exePtr->hasPythonCode == false))
    {
        exePtr->exeDefPtr->ThrowException(LE_I18N("Executable doesn't contain any components"
                                                  " that have source code files."));
    }

    // Add all automatic local bindings within the executable
    AddLocalBindings(exePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates Exe_t objects for all executables in the executables section.
 */
//--------------------------------------------------------------------------------------------------
static void AddExecutables
(
    model::App_t* appPtr,
    const parseTree::CompoundItem_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto addExe = [appPtr, &buildParams](const parseTree::Executable_t* itemPtr)
        {
            // The exe name is the first token in the named item
            auto& exeName = itemPtr->firstTokenPtr->text;

            if (buildParams.beVerbose)
            {
                std::cout << mk::format(LE_I18N("Application '%s' contains executable '%s'."),
                                        appPtr->name, exeName)
                          << std::endl;
            }

            // Compute the path to the executable, relative to the app's working directory
            // and create an object for this exe.
            auto exePtr = new model::Exe_t("obj/" + exeName + "/" + exeName,
                                           appPtr,
                                           buildParams.workingDir);
            exePtr->exeDefPtr = itemPtr;

            // Iterate over the list of contents of the executable specification in the parse
            // tree and add each item as a component.
            for (auto tokenPtr : itemPtr->Contents())
            {
                // Get the component object.
                auto componentPtr = GetComponent(tokenPtr, buildParams, { appPtr->dir });

                // Skip if environment variable substitution resulted in an empty string.
                if (componentPtr != NULL)
                {
                    if (buildParams.beVerbose)
                    {
                        std::cout << mk::format(LE_I18N("Executable '%s' in application '%s'"
                                                        " contains component '%s' (%s)."),
                                                exeName, appPtr->name,
                                                componentPtr->name, componentPtr->dir)
                                  << std::endl;
                    }

                    // Add an instance of the component to the executable.
                    AddComponentInstance(exePtr, componentPtr);
                }

            }

            if (exePtr->hasJavaCode)
            {
                exePtr->path += ".jar";
            }

            // Add the executable to the application.
            AddExecutable(appPtr, exePtr);
        };

    auto executablesSectionPtr = ToCompoundItemListPtr(sectionPtr);

    for (auto itemPtr : executablesSectionPtr->Contents())
    {
        addExe(static_cast<const parseTree::Executable_t*>(ToTokenListPtr(itemPtr)));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Walk the parse tree for an "extern:" section looking for extern API interfaces. Adds a
 * pointer to each found items to the list provided.
 */
//--------------------------------------------------------------------------------------------------
static void AddExternApiInterfaces
(
    std::list<const parseTree::ExternApiInterface_t*>& interfaces, ///< [OUT] List of extern items.
    const parseTree::ComplexSection_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Each item in the section is either an ExternApiInterface_t or a ComplexSection_t.
    // We are looking for the ExternApiInterface_t items.
    for (auto itemPtr : sectionPtr->Contents())
    {
        if (itemPtr->type == parseTree::Content_t::EXTERN_API_INTERFACE)
        {
            // Add to the list of extern API interfaces to be processed later.
            interfaces.push_back(dynamic_cast<parseTree::ExternApiInterface_t*>(itemPtr));
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the API file and interface name for a pre-built interface found in an entry in a "requires:"
 * or "provides:" subsection of an "extern:" section.
 *
 * @return Pointer to the API File object.
 */
//--------------------------------------------------------------------------------------------------
static model::ApiFile_t* GetPreBuiltInterface
(
    std::string& interfaceName, ///< [OUT] String to be filled with the interface name.
    const parseTree::TokenList_t* itemPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    const auto& contentList = itemPtr->Contents();

    // If the first content item is a DOTTED NAME then it's the interface name, and the API file
    // path will follow.
    std::string apiFilePath;
    if (contentList[0]->type == parseTree::Token_t::DOTTED_NAME)
    {
        interfaceName = contentList[0]->text;
        apiFilePath = file::FindFile(DoSubstitution(contentList[1]),
                                     buildParams.interfaceDirs);
        if (apiFilePath == "")
        {
            contentList[1]->ThrowException("Couldn't find file '" + contentList[1]->text + "'.");
        }
    }
    // If the first content item is not a NAME, then it must be the API file path.
    else
    {
        apiFilePath = file::FindFile(DoSubstitution(contentList[0]),
                                     buildParams.interfaceDirs);
        if (apiFilePath == "")
        {
            contentList[0]->ThrowException(
                                 mk::format(LE_I18N("Couldn't find file, '%s'."),
                                                    DoSubstitution(contentList[0])));
        }
    }

    // Get a pointer to the .api file object.
    auto apiFilePtr = GetApiFilePtr(apiFilePath, buildParams.interfaceDirs, contentList[0]);

    // If no interface name was specified, then use the .api file's default prefix.
    if (interfaceName.empty())
    {
        interfaceName = apiFilePtr->defaultPrefix;
    }

    return apiFilePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Walk the parse tree for an "extern:" section looking for "requires:" and "provides:" sections.
 * When found, adds their pre-built IPC API interfaces to the App object.
 */
//--------------------------------------------------------------------------------------------------
static void ModelPreBuiltInterfaces
(
    model::App_t* appPtr,
    const parseTree::ComplexSection_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Each item in the section is either an ExternApiInterface_t, or a ComplexSection_t.
    // We are looking for the ComplexSection_t objects.
    for (auto itemPtr : sectionPtr->Contents())
    {
        if (itemPtr->type == parseTree::Content_t::COMPLEX_SECTION)
        {
            const auto subsectionPtr = dynamic_cast<parseTree::ComplexSection_t*>(itemPtr);
            const auto& subsectionName = subsectionPtr->firstTokenPtr->text;

            if (subsectionName == "requires")
            {
                // Each item in the section is a RequiredApi_t.
                for (auto itemPtr : subsectionPtr->Contents())
                {
                    const auto contentPtr = dynamic_cast<parseTree::RequiredApi_t*>(itemPtr);
                    std::string interfaceName;
                    auto apiFilePtr = GetPreBuiltInterface(interfaceName, contentPtr, buildParams);

                    // Create ApiClientInterface_t and ApiClientInterfaceInstance_t objects.
                    auto ifPtr = new model::ApiClientInterface_t(contentPtr,
                                                                 apiFilePtr,
                                                                 NULL, // component is unknown
                                                                 interfaceName);
                    auto ifInstancePtr = new model::ApiClientInterfaceInstance_t(NULL, ifPtr);

                    // Add the interface instance object to the app's list of pre-built client-side
                    // interfaces.
                    appPtr->preBuiltClientInterfaces[interfaceName] = ifInstancePtr;
                }
            }
            else if (subsectionName == "provides")
            {
                // Each item in the section is a ProvidedApi_t.
                for (auto itemPtr : subsectionPtr->Contents())
                {
                    const auto contentPtr = dynamic_cast<parseTree::ProvidedApi_t*>(itemPtr);

                    std::string interfaceName;
                    auto apiFilePtr = GetPreBuiltInterface(interfaceName, contentPtr, buildParams);

                    // Create ApiServerInterface_t and ApiServerInterfaceInstance_t objects.
                    auto ifPtr = new model::ApiServerInterface_t(contentPtr,
                                                                 apiFilePtr,
                                                                 NULL,  // component is unknown
                                                                 interfaceName,
                                                                 false); // Don't care if async
                    auto ifInstancePtr = new model::ApiServerInterfaceInstance_t(NULL, ifPtr);

                    // Add the interface instance object to the app's list of pre-built server-side
                    // interfaces.
                    appPtr->preBuiltServerInterfaces[interfaceName] = ifInstancePtr;
                }
            }
            else
            {
                itemPtr->ThrowException("Internal error: unexpected subsection '"
                                        + subsectionName
                                        + "' in extern section.");
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add access permissions for a configuration tree to an application.
 */
//--------------------------------------------------------------------------------------------------
static void AddConfigTree
(
    model::App_t* appPtr,
    const parseTree::RequiredConfigTree_t* specPtr ///< The access specification in the parse tree.
)
//--------------------------------------------------------------------------------------------------
{
    auto& contents = specPtr->Contents();

    model::Permissions_t permissions;
    const parseTree::Token_t* treeNameTokenPtr;
    std::string fileName;

    // Check for optional FILE_PERMISSIONS token.
    if (contents[0]->type == parseTree::Token_t::FILE_PERMISSIONS)
    {
        GetPermissions(permissions, contents[0]);
        treeNameTokenPtr = contents[1];
    }
    else // No FILE_PERMISSIONS
    {
        permissions.SetReadable();  // read-only by default.
        treeNameTokenPtr = contents[0];
    }

    // Replace the "DOT" with current application name.
    if (treeNameTokenPtr->type == parseTree::Token_t::DOT)
    {
        fileName = appPtr->name;
    }
    else
    {
        fileName = treeNameTokenPtr->text;
    }

    // Check for duplicates.
    if (appPtr->configTrees.find(fileName) != appPtr->configTrees.end())
    {
        treeNameTokenPtr->ThrowException(
            mk::format(LE_I18N("Configuration tree '%s' appears in application more than once."),
                       fileName)
        );
    }

    // Add config tree access permissions to the app.
    appPtr->configTrees[fileName] = permissions;
}


//--------------------------------------------------------------------------------------------------
/**
 * Model a "requires:" section.
 */
//--------------------------------------------------------------------------------------------------
static void AddRequiredItems
(
    model::App_t* appPtr,
    const parseTree::Content_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::list<const parseTree::CompoundItem_t*> reqKernelModulesSections;

    for (auto subsectionPtr : ToCompoundItemListPtr(sectionPtr)->Contents())
    {
        auto& subsectionName = subsectionPtr->firstTokenPtr->text;

        if (subsectionName == "file")
        {
            for (auto itemPtr : parseTree::ToCompoundItemListPtr(subsectionPtr)->Contents())
            {
                auto fileSpecPtr = parseTree::ToTokenListPtr(itemPtr);

                appPtr->requiredFiles.insert(
                    std::shared_ptr<model::FileSystemObject_t>(GetRequiredFile(fileSpecPtr)));
            }
        }
        else if (subsectionName == "dir")
        {
            for (auto itemPtr : parseTree::ToCompoundItemListPtr(subsectionPtr)->Contents())
            {
                auto dirSpecPtr = parseTree::ToTokenListPtr(itemPtr);

                appPtr->requiredDirs.insert(
                    std::shared_ptr<model::FileSystemObject_t>(GetRequiredDir(dirSpecPtr)));
            }
        }
        else if (subsectionName == "device")
        {
            for (auto itemPtr : parseTree::ToCompoundItemListPtr(subsectionPtr)->Contents())
            {
                auto deviceSpecPtr = parseTree::ToTokenListPtr(itemPtr);

                appPtr->requiredDevices.insert(
                    std::shared_ptr<model::FileSystemObject_t>(GetRequiredDevice(deviceSpecPtr)));
            }
        }
        else if (subsectionName == "configTree")
        {
            for (auto itemPtr : parseTree::ToCompoundItemListPtr(subsectionPtr)->Contents())
            {
                auto configTreeSpecPtr = parseTree::ToTokenListPtr(itemPtr);

                AddConfigTree(appPtr,
                            static_cast<const parseTree::RequiredConfigTree_t*>(configTreeSpecPtr));
            }
        }
        else if (parser::IsNameSingularPlural(subsectionName, "kernelModule"))
        {
            reqKernelModulesSections.push_back(subsectionPtr);
        }
        else
        {
            subsectionPtr->ThrowException(
                mk::format(LE_I18N("Internal error: Unrecognized section '%s'."),
                           subsectionName)
            );
        }
    }

    AddRequiredKernelModules(appPtr->requiredModules, nullptr, reqKernelModulesSections,
                             buildParams);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add processes to a process environment, based on the contents of a given run section in
 * the parse tree.
 */
//--------------------------------------------------------------------------------------------------
static void AddProcesses
(
    model::App_t* appPtr,
    model::ProcessEnv_t* procEnvPtr,
    const parseTree::CompoundItemList_t* sectionPtr ///< run section.
)
//--------------------------------------------------------------------------------------------------
{
    // Each item in this section is a process specification in the form of a TokenList_t.
    for (auto itemPtr : sectionPtr->Contents())
    {
        auto processSpecPtr = dynamic_cast<const parseTree::RunProcess_t*>(itemPtr);
        if (processSpecPtr == NULL)
        {
            itemPtr->ThrowException(
                mk::format(LE_I18N("Internal error: '%s'' is not a RunProcess_t."),
                           itemPtr->TypeName())
            );
        }

        // If the first token is an open parenthesis, then no process name was specified and
        // the first content token is the executable path, which also is used as the process name.
        // Otherwise, the first content token is the process name, followed by the exe path.
        auto tokens = processSpecPtr->Contents();
        auto i = tokens.begin();

        // In case the tokens are empty, go on to the next process specification.
        if (i != tokens.end())
        {
            auto procName = (*i)->text;

            if (DoesProcessExist(appPtr, procName))
            {
                (*i)->ThrowException(
                    mk::format(LE_I18N("Process name '%s' already used."
                                       "  Process names must be unique"),
                               procName)
                );
            }

            auto procPtr = new model::Process_t(processSpecPtr);
            procEnvPtr->processes.push_back(procPtr);

            procPtr->SetName(procName);
            if (processSpecPtr->firstTokenPtr->type != parseTree::Token_t::OPEN_PARENTHESIS)
            {
                i++;
            }

            if (i != tokens.end())
            {
                procPtr->exePath = path::Unquote((*i)->text);
            }

            for (i++ ; i != tokens.end() ; i++)
            {
                procPtr->commandLineArgs.push_back(path::Unquote((*i)->text));
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add process environments and processes to an application, based on the contents of a given
 * processes section in the parse tree.
 */
//--------------------------------------------------------------------------------------------------
static void AddProcessesSection
(
    model::App_t* appPtr,
    const parseTree::CompoundItemList_t* sectionPtr ///< processes section.
)
//--------------------------------------------------------------------------------------------------
{
    auto procEnvPtr = new model::ProcessEnv_t();
    appPtr->processEnvs.push_back(procEnvPtr);

    // The processes section contains a list of subsections.
    for (auto subsectionPtr : sectionPtr->Contents())
    {
        auto& subsectionName = subsectionPtr->firstTokenPtr->text;

        if (subsectionName == "run")
        {
            AddProcesses(appPtr, procEnvPtr, ToCompoundItemListPtr(subsectionPtr));
        }
        else if (subsectionName == "envVars")
        {
            // Each item in this section is a token list with one content item (the value).
            for (auto itemPtr : ToCompoundItemListPtr(subsectionPtr)->Contents())
            {
                auto envVarPtr = ToTokenListPtr(itemPtr);
                auto& name = envVarPtr->firstTokenPtr->text;
                auto& value = envVarPtr->Contents()[0];

                procEnvPtr->envVars[name] = path::Unquote(DoSubstitution(value));
            }
        }
        else if (subsectionName == "faultAction")
        {
            procEnvPtr->faultAction = ToSimpleSectionPtr(subsectionPtr)->Text();
        }
        else if (subsectionName == "priority")
        {
            procEnvPtr->SetStartPriority(ToSimpleSectionPtr(subsectionPtr)->Text());
        }
        else if (subsectionName == "maxCoreDumpFileBytes")
        {
            procEnvPtr->maxCoreDumpFileBytes = GetNonNegativeInt(ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "maxFileBytes")
        {
            procEnvPtr->maxFileBytes = GetNonNegativeInt(ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "maxFileDescriptors")
        {
            procEnvPtr->maxFileDescriptors = GetPositiveInt(ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "maxLockedMemoryBytes")
        {
            procEnvPtr->maxLockedMemoryBytes = GetNonNegativeInt(ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "maxStackBytes")
        {
            procEnvPtr->maxStackBytes = GetPositiveInt(ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "watchdogAction")
        {
            procEnvPtr->watchdogAction = ToSimpleSectionPtr(subsectionPtr)->Text();
        }
        else if (subsectionName == "watchdogTimeout")
        {
            auto timeoutSectionPtr = ToSimpleSectionPtr(subsectionPtr);
            auto tokenPtr = timeoutSectionPtr->Contents()[0];
            if (tokenPtr->type == parseTree::Token_t::NAME)
            {
                procEnvPtr->watchdogTimeout = tokenPtr->text; // Never timeout (watchdog disabled).
            }
            else
            {
                procEnvPtr->watchdogTimeout = GetInt(timeoutSectionPtr);
            }
        }
        else if (subsectionName == "maxWatchdogTimeout")
        {
            auto maxTimeoutSectionPtr = ToSimpleSectionPtr(subsectionPtr);
            procEnvPtr->maxWatchdogTimeout = GetInt(maxTimeoutSectionPtr);
        }
        else
        {
            subsectionPtr->ThrowException(
                mk::format(LE_I18N("Internal error: Unrecognized section '%s'."), subsectionName)
            );
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add process environments and processes to an application, based on the contents of a list of
 * processes sections in the parse tree.
 */
//--------------------------------------------------------------------------------------------------
static void AddProcessesSections
(
    model::App_t* appPtr,
    std::list<const parseTree::CompoundItem_t*> processesSections ///< List of processes sections.
)
//--------------------------------------------------------------------------------------------------
{
    for (auto sectionPtr : processesSections)
    {
        AddProcessesSection(appPtr, ToCompoundItemListPtr(sectionPtr));
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Mark an interface instance as externally visible for binding at the system level.
 */
//--------------------------------------------------------------------------------------------------
static void MarkInterfaceExternal
(
    model::App_t* appPtr,
    model::ApiInterfaceInstance_t* ifInstancePtr, ///< Ptr to the interface instance object.
    const parseTree::Token_t* nameTokenPtr    ///< Ptr to token containing name to use outside app.
)
//--------------------------------------------------------------------------------------------------
{
    // If the interface is already marked external, this is a duplicate.
    if (ifInstancePtr->externMarkPtr != NULL)
    {
        nameTokenPtr->ThrowException(
            mk::format(LE_I18N("Same interface marked 'extern' more than once.\n"
                               "%s: note: Previously done here."),
                       ifInstancePtr->externMarkPtr->GetLocation())
        );
    }

    // Mark it external and assign it the external name.
    ifInstancePtr->externMarkPtr = nameTokenPtr;
    ifInstancePtr->name = nameTokenPtr->text;
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark a single API interface instance as externally visible to other apps.
 */
//--------------------------------------------------------------------------------------------------
static void MakeInterfaceExternal
(
    model::App_t* appPtr,
    const parseTree::Token_t* nameTokenPtr,
    const parseTree::Token_t* exeTokenPtr,
    const parseTree::Token_t* componentTokenPtr,
    const parseTree::Token_t* interfaceTokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    const auto& exeName = exeTokenPtr->text;
    const auto& componentName = componentTokenPtr->text;
    const auto& interfaceName = interfaceTokenPtr->text;

    // Check that there are no other external interfaces using the same name already.
    const auto& name = nameTokenPtr->text;
    if (   (appPtr->externServerInterfaces.find(name) != appPtr->externServerInterfaces.end())
        || (appPtr->externClientInterfaces.find(name) != appPtr->externClientInterfaces.end()) )
    {
        nameTokenPtr->ThrowException(
            mk::format(LE_I18N("Duplicate external interface name: '%s'."), name)
        );
    }

    // Find the component instance.
    auto componentInstancePtr = appPtr->FindComponentInstance(exeTokenPtr, componentTokenPtr);

    // Find the interface (look in both the client and server interface lists.
    auto serverIfPtr = componentInstancePtr->FindServerInterface(interfaceTokenPtr->text);
    auto clientIfPtr = componentInstancePtr->FindClientInterface(interfaceTokenPtr->text);

    if ((clientIfPtr == NULL) && (serverIfPtr == NULL))
    {
        nameTokenPtr->ThrowException(
            mk::format(LE_I18N("Interface '%s' not found in component '%s' in executable '%s'."),
                       interfaceName, componentName, exeName)
        );
    }

    // Mark the interface "external", and add it to the appropriate list of external interfaces.
    if (clientIfPtr != NULL)
    {
        MarkInterfaceExternal(appPtr, clientIfPtr, nameTokenPtr);

        appPtr->externClientInterfaces[name] = clientIfPtr;
    }
    else
    {
        MarkInterfaceExternal(appPtr, serverIfPtr, nameTokenPtr);

        appPtr->externServerInterfaces[name] = serverIfPtr;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark API interface instances as externally visible to other apps.
 */
//--------------------------------------------------------------------------------------------------
static void MakeInterfacesExternal
(
    model::App_t* appPtr,
    const std::list<const parseTree::ExternApiInterface_t*>& interfaces
)
//--------------------------------------------------------------------------------------------------
{
    for (auto ifPtr : interfaces)
    {
        // Each interface spec is a token list.
        auto tokens = dynamic_cast<const parseTree::TokenList_t*>(ifPtr)->Contents();

        // If there are 4 content tokens, the first token is the external name
        // to be used to identify the interface, and the remaining three tokens are the
        // exe, component, and interface names of the interface instance.
        if (tokens.size() == 4)
        {
            MakeInterfaceExternal(appPtr, tokens[0], tokens[1], tokens[2], tokens[3]);
        }
        // Otherwise, there are 3 content tokens and the interface is exported using the
        // internal name of the interface on the component.
        else
        {
            MakeInterfaceExternal(appPtr, tokens[2], tokens[0], tokens[1], tokens[2]);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Exract the server side details from a bindings section in the parse tree.
 */
//--------------------------------------------------------------------------------------------------
static void GetBindingServerSide
(
    model::Binding_t* bindingPtr,   ///< Binding object to populate with the server-side details.
    const std::vector<parseTree::Token_t*>& tokens, ///< List of tokens in the binding spec.
    size_t startIndex,  ///< Index into the token list of the first token in the server i/f spec.
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    // startIndex   startIndex + 1  startIndex + 2
    // NAME         NAME            NAME            = internal binding
    // IPC_AGENT    NAME                            = external binding
    // STAR         NAME                            = internal binding to pre-built binary server

    // External binding?
    if (tokens[startIndex]->type == parseTree::Token_t::IPC_AGENT)
    {
        auto& serverAgentName = tokens[startIndex]->text;
        bindingPtr->serverIfName = tokens[startIndex + 1]->text;

        if (serverAgentName[0] == '<')  // non-app user?
        {
            bindingPtr->serverType = model::Binding_t::EXTERNAL_USER;
            bindingPtr->serverAgentName = RemoveAngleBrackets(serverAgentName);
        }
        else // app
        {
            bindingPtr->serverType = model::Binding_t::EXTERNAL_APP;
            bindingPtr->serverAgentName = serverAgentName;
        }
    }
    // Internal binding to pre-built binary? (*.interface)
    else if (tokens[startIndex]->type == parseTree::Token_t::STAR)
    {
        bindingPtr->serverType = model::Binding_t::INTERNAL;
        bindingPtr->serverAgentName = appPtr->name;
        bindingPtr->serverIfName = tokens[startIndex + 1]->text;
    }
    // Internal binding to exe built by mk tools (exe.component.interface).
    else
    {
        // Find the interface that matches this specification.
        auto serverIfPtr = appPtr->FindServerInterface(tokens[startIndex],
                                                       tokens[startIndex + 1],
                                                       tokens[startIndex + 2]);
        // Populate the binding object.
        bindingPtr->serverType = model::Binding_t::INTERNAL;
        bindingPtr->serverAgentName = appPtr->name;
        bindingPtr->serverIfName = serverIfPtr->name;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Add all the IPC bindings from a .adef's bindings section to a given app object.
 */
//--------------------------------------------------------------------------------------------------
static void AddBindings
(
    model::App_t* appPtr,
    const parseTree::CompoundItem_t* bindingsSectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    // The bindings section is a list of compound items.
    auto sectionPtr = ToCompoundItemListPtr(bindingsSectionPtr);

    for (auto itemPtr : sectionPtr->Contents())
    {
        // Each binding specification inside the bindings section is a token list.
        auto bindingSpecPtr = dynamic_cast<const parseTree::Binding_t*>(itemPtr);
        auto tokens = bindingSpecPtr->Contents();

        // Create a new Binding object for the model.
        auto bindingPtr = new model::Binding_t(bindingSpecPtr);

        // Bindings in .adef files are always for that app's client-side internal interfaces.
        bindingPtr->clientType = model::Binding_t::INTERNAL;
        bindingPtr->clientAgentName = appPtr->name;

        // Is this a binding of pre-built client interfaces with a given name?
        if (tokens[0]->type == parseTree::Token_t::STAR)
        {
            // 0    1    2         3    4
            // STAR NAME IPC_AGENT NAME      = external binding to user or app
            // STAR NAME NAME      NAME NAME = internal binding to exe
            bindingPtr->clientIfName = tokens[1]->text;
            GetBindingServerSide(bindingPtr, tokens, 2, appPtr);

            // Look up the interface object.
            auto i = appPtr->preBuiltClientInterfaces.find(bindingPtr->clientIfName);

            if (i == appPtr->preBuiltClientInterfaces.end())
            {
                if (appPtr->isPreBuilt)
                {
                    auto str = mk::format(LE_I18N("INTERNAL ERROR: No such client-side"
                                                  " pre-built interface '%s'."),
                                          bindingPtr->clientIfName);
                    tokens[1]->ThrowException(str);
                }
                else
                {
                    PrintWarning(appPtr,
                                 mk::format(LE_I18N("Binding for unreferenced client-side interface"
                                                    " '%s'.  Bindings for unreferenced interfaces"
                                                    " are deprecated."),
                                            bindingPtr->clientIfName));
                    PrintNote(appPtr, LE_I18N("If this is used by a legacy app, it should be"
                                              " included in the extern: requires: section"));


                    // Add the interface instance object to the app's list of pre-built client-side
                    // interfaces.
                    auto ifPtr = new model::ApiClientInterface_t(bindingSpecPtr,
                                                                 NULL,
                                                                 NULL, // component is unknown
                                                                 bindingPtr->clientIfName);
                    auto ifInstancePtr = new model::ApiClientInterfaceInstance_t(NULL, ifPtr);

                    i = appPtr->preBuiltClientInterfaces.insert(
                        make_pair(bindingPtr->clientIfName, ifInstancePtr)).first;
                }
            }

            auto interfacePtr = i->second;

            // Check for multiple bindings of the same client-side pre-built interface.
            if (interfacePtr->bindingPtr != NULL)
            {
                std::stringstream msg;
                msg << "Duplicate binding of pre-built client-side interface '"
                    << bindingPtr->clientIfName << "'. Previous binding is at line "
                    << interfacePtr->bindingPtr->parseTreePtr->firstTokenPtr->line << ".";
                tokens[1]->ThrowException(msg.str());
            }

            // Store the binding.
            interfacePtr->bindingPtr = bindingPtr;
        }
        else // Normal client interface binding.
        {
            // 0    1    2    3         4    5
            // NAME NAME NAME IPC_AGENT NAME      = external binding to user or app
            // NAME NAME NAME NAME      NAME NAME = internal binding to exe
            auto clientIfPtr = appPtr->FindClientInterface(tokens[0], tokens[1], tokens[2]);
            bindingPtr->clientIfName = clientIfPtr->name;
            GetBindingServerSide(bindingPtr, tokens, 3, appPtr);

            // Check for multiple bindings of the same client-side interface.
            if (clientIfPtr->bindingPtr != NULL)
            {
                tokens[0]->ThrowException(
                    mk::format(LE_I18N("Client interface bound more than once.\n"
                                       "%s: note: First binding here"),
                               clientIfPtr->bindingPtr->parseTreePtr->Contents()[0]->GetLocation()
                    )
                );
            }

            // Record the binding in the client-side interface object.
            clientIfPtr->bindingPtr = bindingPtr;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add all the IPC bindings from a list of bindings sections to a given app object.
 */
//--------------------------------------------------------------------------------------------------
static void AddBindings
(
    model::App_t* appPtr,
    std::list<const parseTree::CompoundItem_t*> bindingsSections
)
//--------------------------------------------------------------------------------------------------
{
    for (auto bindingsSectionPtr : bindingsSections)
    {
        AddBindings(appPtr, bindingsSectionPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print to standard out a description of a given IPC binding.
 **/
//--------------------------------------------------------------------------------------------------
static void PrintBindingSummary
(
    const std::string& indent,
    const std::string& clientIfName,
    const model::Binding_t* bindingPtr
)
//--------------------------------------------------------------------------------------------------
{
    std::cout << indent;
    switch (bindingPtr->serverType)
    {
        case model::Binding_t::INTERNAL:
        case model::Binding_t::LOCAL:
            std::cout << mk::format(LE_I18N("'%s' -> bound to service '%s'"
                                            " on another exe inside the same app."),
                                    clientIfName, bindingPtr->serverIfName);
            break;
        case model::Binding_t::EXTERNAL_APP:
            std::cout << mk::format(LE_I18N("'%s' -> bound to service '%s' served by app '%s'."),
                                    clientIfName, bindingPtr->serverIfName,
                                    bindingPtr->serverAgentName);
            break;
        case model::Binding_t::EXTERNAL_USER:
            std::cout << mk::format(LE_I18N("'%s' -> bound to service '%s' served by user <%s>."),
                                    clientIfName, bindingPtr->serverIfName,
                                    bindingPtr->serverAgentName);
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Print a summary of an application object.
 **/
//--------------------------------------------------------------------------------------------------
void PrintSummary
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    std::cout << std::endl
              << mk::format(LE_I18N("== '%s' application summary =="), appPtr->name) << std::endl
              << std::endl;

    if (!appPtr->components.empty())
    {
        std::cout << LE_I18N("  Uses components:") << std::endl;

        for (auto componentPtr : appPtr->components)
        {
            std::cout << mk::format(LE_I18N("    '%s'"), componentPtr->name) << std::endl;
        }
    }

    if (!appPtr->executables.empty())
    {
        std::cout << LE_I18N("  Builds executables:") << std::endl;

        for (auto mapItem : appPtr->executables)
        {
            auto exePtr = mapItem.second;

            std::cout << mk::format(LE_I18N("    '%s'"), exePtr->name) << std::endl;

            if (!exePtr->componentInstances.empty())
            {
                std::cout << LE_I18N("      Instantiates components:") << std::endl;

                for (const auto& componentInstancePtr : exePtr->componentInstances)
                {
                    std::cout << mk::format(LE_I18N("        '%s'"),
                                            componentInstancePtr->componentPtr->name)
                              << std::endl;
                }
            }
        }
    }

    if (!appPtr->bundledFiles.empty())
    {
        std::cout << LE_I18N("  Includes files from the build host:") << std::endl;

        for (auto itemPtr : appPtr->bundledFiles)
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

    if (!appPtr->bundledDirs.empty())
    {
        std::cout << LE_I18N("  Includes directories from the build host:") << std::endl;

        for (auto itemPtr : appPtr->bundledDirs)
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

    if (!appPtr->isSandboxed)
    {
        std::cout << LE_I18N("  WARNING: This application is UNSANDBOXED.") << std::endl;
    }
    else
    {
        std::cout << LE_I18N("  Runs inside a sandbox.") << std::endl;
        if (!appPtr->requiredFiles.empty())
        {
            std::cout << LE_I18N("  Imports the following files from the target host:")
                      << std::endl;

            for (auto itemPtr : appPtr->requiredFiles)
            {
                std::cout << mk::format(LE_I18N("    '%s':"), itemPtr->srcPath) << std::endl;
                std::cout << mk::format(LE_I18N("      appearing inside app as: '%s'"),
                                        itemPtr->destPath)
                          << std::endl;
            }
        }

        if (!appPtr->requiredDirs.empty())
        {
            std::cout << LE_I18N("  Imports the following directories from the target host:")
                      << std::endl;

            for (auto itemPtr : appPtr->requiredDirs)
            {
                std::cout << mk::format(LE_I18N("    '%s':"), itemPtr->srcPath) << std::endl;
                std::cout << mk::format(LE_I18N("      appearing inside app as: '%s'"),
                                        itemPtr->destPath)
                          << std::endl;
            }
        }

        std::cout << LE_I18N("  Has the following limits:") << std::endl;
        std::cout << mk::format(LE_I18N("    maxSecureStorageBytes: %d"),
                                appPtr->maxSecureStorageBytes.Get())
                  << std::endl;
        std::cout << mk::format(LE_I18N("    maxThreads: %d"), appPtr->maxThreads.Get())
                  << std::endl;
        std::cout << mk::format(LE_I18N("    maxMQueueBytes: %d"), appPtr->maxMQueueBytes.Get())
                  << std::endl;
        std::cout << mk::format(LE_I18N("    maxQueuedSignals: %d"), appPtr->maxQueuedSignals.Get())
                  << std::endl;
        std::cout << mk::format(LE_I18N("    maxMemoryBytes: %d"), appPtr->maxMemoryBytes.Get())
                  << std::endl;
        std::cout << mk::format(LE_I18N("    cpuShare: %d"), appPtr->cpuShare.Get())
                  << std::endl;
        std::cout << mk::format(LE_I18N("    maxFileSystemBytes: %d"),
                                appPtr->maxFileSystemBytes.Get())
                  << std::endl;

        // Config Tree access.
        std::cout << LE_I18N("  Has access to the following configuration trees:") << std::endl;
        std::cout << LE_I18N("    Its own tree: read + write") << std::endl;
        for (const auto& mapEntry : appPtr->configTrees)
        {
            std::cout << mk::format(LE_I18N("    %s: "), mapEntry.first);

            if (mapEntry.second.IsWriteable())
            {
                std::cout << LE_I18N("read + write") << std::endl;
            }
            else
            {
                std::cout << LE_I18N("read only") << std::endl;
            }
        }
    }

    // Start trigger.
    if (appPtr->startTrigger == model::App_t::AUTO)
    {
        std::cout << LE_I18N("  Will be started automatically when the Legato framework starts.")
                  << std::endl;
    }
    else
    {
        std::cout << LE_I18N("  Will only start when requested to start.") << std::endl;
    }

    // Process list
    bool containsAtLeastOneProcess = false;
    for (auto procEnvPtr : appPtr->processEnvs)
    {
        if (!procEnvPtr->processes.empty())
        {
            containsAtLeastOneProcess = true;

            for (auto procPtr : procEnvPtr->processes)
            {
                std::cout << mk::format(LE_I18N("  When started, will run process: '%s'"),
                                        procPtr->GetName())
                          << std::endl;

                // Exe path.
                std::cout << mk::format(LE_I18N("    Executing file: '%s'"), procPtr->exePath)
                          << std::endl;

                // Command-line args.
                if (procPtr->commandLineArgs.empty())
                {
                    std::cout << LE_I18N("    Without any command line arguments.") << std::endl;
                }
                else
                {
                    std::cout << LE_I18N("    With the following command line arguments:")
                              << std::endl;
                    for (const auto& arg : procPtr->commandLineArgs)
                    {
                        std::cout << mk::format(LE_I18N("      '%s'"), arg)
                                  << std::endl;
                    }
                }

                // Priority.
                if (procEnvPtr->GetStartPriority().IsSet())
                {
                    std::cout << mk::format(LE_I18N("    At priority: %s"),
                                            procEnvPtr->GetStartPriority().Get())
                              << std::endl;
                }

                // Environment variables.
                std::cout << LE_I18N("    With the following environment variables:") << std::endl;
                for (const auto& pair : procEnvPtr->envVars)
                {
                    std::cout << mk::format(LE_I18N("      %s=%s"), pair.first, pair.second)
                              << std::endl;
                }

                // Fault action.
                if (procEnvPtr->faultAction.IsSet())
                {
                    std::cout << mk::format(LE_I18N("    Fault recovery action: %s"),
                                            procEnvPtr->faultAction.Get()) << std::endl;
                }
                else
                {
                    std::cout << LE_I18N("    Fault recovery action: ignore (default)")
                              << std::endl;
                }

                // Watchdog.
                if (procEnvPtr->watchdogTimeout.IsSet())
                {
                    std::cout << mk::format(LE_I18N("    Watchdog timeout: %d"),
                                            procEnvPtr->watchdogTimeout.Get())
                              << std::endl;
                }
                else if (appPtr->watchdogTimeout.IsSet())
                {
                    std::cout << mk::format(LE_I18N("    Watchdog timeout: %d"),
                                            appPtr->watchdogTimeout.Get())
                              << std::endl;
                }

                if (procEnvPtr->maxWatchdogTimeout.IsSet())
                {
                    std::cout << mk::format(LE_I18N("    Maximum watchdog timeout: %d"),
                                            procEnvPtr->maxWatchdogTimeout.Get())
                              << std::endl;
                }
                else if (appPtr->maxWatchdogTimeout.IsSet())
                {
                    std::cout << mk::format(LE_I18N("    Maximum watchdog timeout: %d"),
                                            appPtr->maxWatchdogTimeout.Get())
                              << std::endl;
                }

                if (procEnvPtr->watchdogAction.IsSet())
                {
                    std::cout << mk::format(LE_I18N("    Watchdog action: %s"),
                                            procEnvPtr->watchdogAction.Get())
                              << std::endl;
                }
                else if (appPtr->watchdogAction.IsSet())
                {
                    std::cout << mk::format(LE_I18N("    Watchdog action: %s"),
                                            appPtr->watchdogAction.Get())
                              << std::endl;
                }
                if (   (!procEnvPtr->watchdogTimeout.IsSet())
                    && (!procEnvPtr->maxWatchdogTimeout.IsSet())
                    && (!procEnvPtr->watchdogAction.IsSet())
                    && (!appPtr->watchdogTimeout.IsSet())
                    && (!appPtr->maxWatchdogTimeout.IsSet())
                    && (!appPtr->watchdogAction.IsSet())  )
                {
                    std::cout << LE_I18N("    Watchdog timeout: disabled") << std::endl;
                }

                // Limits.
                if (appPtr->isSandboxed)
                {
                    std::cout << LE_I18N("    With the following limits:") << std::endl;
                    std::cout << mk::format(LE_I18N("      Max. core dump file size: %d bytes"),
                                            procEnvPtr->maxCoreDumpFileBytes.Get())
                              << std::endl;
                    std::cout << mk::format(LE_I18N("      Max. file size: %d bytes"),
                                            procEnvPtr->maxFileBytes.Get())
                              << std::endl;
                    std::cout << mk::format(LE_I18N("      Max. locked memory size: %d bytes"),
                                            procEnvPtr->maxLockedMemoryBytes.Get())
                              << std::endl;
                    std::cout << mk::format(LE_I18N("      Max. number of file descriptors: %d"),
                                            procEnvPtr->maxFileDescriptors.Get())
                              << std::endl;
                    if (procEnvPtr->maxStackBytes.IsSet())
                    {
                        std::cout << mk::format(LE_I18N("      Stack size: %d bytes"),
                                                procEnvPtr->maxStackBytes.Get())
                                  << std::endl;
                    }
                    else
                    {
                        std::cout << LE_I18N("      Stack size: OS default")
                                  << std::endl;
                    }
                }
            }
        }
    }
    if ((!containsAtLeastOneProcess) && appPtr->isSandboxed)
    {
        std::cout << LE_I18N("  When \"started\", will create a sandbox without running anything"
                             " in it.")
                  << std::endl;
    }

    // Groups
    if (appPtr->isSandboxed && !appPtr->groups.empty())
    {
        std::cout << LE_I18N("  Will be a member of the following access control groups:")
                  << std::endl;
        for (auto& group : appPtr->groups)
        {
            std::cout << "    " << group << std::endl;
        }
    }

    // IPC interfaces and bindings.
    std::list<const model::ApiClientInterfaceInstance_t*> requiredClientIfs;
    std::list<const model::ApiClientInterfaceInstance_t*> boundClientIfs;
    std::list<const model::ApiServerInterfaceInstance_t*> serverIfs;
    for (auto mapItem : appPtr->executables)
    {
        auto exePtr = mapItem.second;

        std::cout << mk::format(LE_I18N("  Executable '%s':"),
                                exePtr->name)
                  << std::endl;

        for (auto componentInstancePtr : exePtr->componentInstances)
        {
            for (auto ifInstancePtr : componentInstancePtr->clientApis)
            {
                if (ifInstancePtr->bindingPtr == NULL)
                {
                    requiredClientIfs.push_back(ifInstancePtr);
                }
                else
                {
                    boundClientIfs.push_back(ifInstancePtr);
                }
            }
            for (auto ifInstancePtr : componentInstancePtr->serverApis)
            {
                serverIfs.push_back(ifInstancePtr);
            }
        }
        if (!(serverIfs.empty()))
        {
            std::cout << LE_I18N("    Serves the following IPC API interfaces:") << std::endl;
        }
        for (auto ifPtr : serverIfs)
        {
            std::cout << mk::format(LE_I18N("      '%s'"), ifPtr->name) << std::endl
                      << mk::format(LE_I18N("        API defined in: '%s'"),
                                    ifPtr->ifPtr->apiFilePtr->path)
                      << std::endl;
        }
        if (!(requiredClientIfs.empty()) || !(boundClientIfs.empty()))
        {
            std::cout << LE_I18N("    Has the following client-side IPC API interfaces:")
                      << std::endl;

            for (auto ifPtr : boundClientIfs)
            {
                PrintBindingSummary("      ", ifPtr->name, ifPtr->bindingPtr);

                std::cout << std::endl
                          << mk::format(LE_I18N("        API defined in: '%s'"),
                                        ifPtr->ifPtr->apiFilePtr->path)
                          << std::endl;
            }

            for (auto ifPtr : requiredClientIfs)
            {
                std::cout << mk::format(LE_I18N("      '%s' -> UNBOUND."), ifPtr->name) << std::endl
                          << mk::format(LE_I18N("        API defined in: '%s'"),
                                        ifPtr->ifPtr->apiFilePtr->path)
                          << std::endl;
            }
        }
    }
    if (!appPtr->preBuiltServerInterfaces.empty())
    {
        std::cout << "  Has the following server-side interfaces on pre-built executables:"
                  << std::endl;

        for (const auto& mapEntry: appPtr->preBuiltServerInterfaces)
        {
            auto ifPtr = mapEntry.second;
            std::cout << "    '" << ifPtr->name << "'" << std::endl
                      << "      API defined in: '" << ifPtr->ifPtr->apiFilePtr->path << "'"
                      << std::endl;
        }
    }
    if (!appPtr->preBuiltClientInterfaces.empty())
    {
        std::cout << LE_I18N("  Has the following client-side interfaces on pre-built executables:")
                  << std::endl;

        for (const auto& mapEntry: appPtr->preBuiltClientInterfaces)
        {
            auto ifPtr = mapEntry.second;

            if (ifPtr->bindingPtr != NULL)
            {
                PrintBindingSummary("    ", ifPtr->name, ifPtr->bindingPtr);
            }
            else
            {
                std::cout << "      '" << ifPtr->name << "' -> UNBOUND.";
            }
            std::cout << std::endl
                      << "        API defined in: '" << ifPtr->ifPtr->apiFilePtr->path << "'"
                      << std::endl;
        }
    }

    std::cout << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks all of an application's limits and prints warnings or errors to stderr if there are
 * conflicts between them.
 *
 * @throw legato::Exception if there is a fatal error.
 **/
//--------------------------------------------------------------------------------------------------
void CheckForLimitsConflicts
(
    const model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    size_t maxMemoryBytes = appPtr->maxMemoryBytes.Get();
    size_t maxFileSystemBytes = appPtr->maxFileSystemBytes.Get();

    for (auto procEnvPtr : appPtr->processEnvs)
    {
        size_t maxLockedMemoryBytes = procEnvPtr->maxLockedMemoryBytes.Get();

        if (maxLockedMemoryBytes > maxMemoryBytes)
        {
            PrintWarning(appPtr,
                         mk::format(LE_I18N("maxLockedMemoryBytes (%d) will be limited by the "
                                            "maxMemoryBytes limit (%d)."),
                                    maxLockedMemoryBytes, maxMemoryBytes));
        }

        if (procEnvPtr->maxStackBytes.IsSet())
        {
            size_t maxStackBytes = procEnvPtr->maxStackBytes.Get();

            if (maxStackBytes > maxMemoryBytes)
            {
                PrintWarning(appPtr,
                             mk::format(LE_I18N("maxStackBytes (%d) is larger than the "
                                                "maxMemoryBytes limit(d)."),
                                        maxStackBytes, maxMemoryBytes));
            }
        }

        size_t maxFileBytes = procEnvPtr->maxFileBytes.Get();
        size_t maxCoreDumpFileBytes = procEnvPtr->maxCoreDumpFileBytes.Get();

        if (maxCoreDumpFileBytes > maxFileBytes)
        {
            PrintWarning(appPtr,
                         mk::format(LE_I18N("maxCoreDumpFileBytes (%d) will be limited by "
                                            "the maxFileBytes limit (%d)."),
                                    maxCoreDumpFileBytes, maxFileBytes));
        }

        if (maxCoreDumpFileBytes > maxFileSystemBytes)
        {
            PrintWarning(appPtr,
                         mk::format(LE_I18N("maxCoreDumpFileBytes (%d) will be limited by "
                                            "the maxFileSystemBytes limit (%d) if the core file "
                                            "is inside the sandbox temporary file system."),
                                    maxCoreDumpFileBytes, maxFileSystemBytes));
        }

        if (maxFileBytes > maxFileSystemBytes)
        {
            PrintWarning(appPtr,
                         mk::format(LE_I18N("maxFileBytes (%d) will be limited by "
                                            "the maxFileSystemBytes limit (%d) if the file is "
                                            "inside the sandbox temporary file system."),
                                    maxFileBytes, maxFileSystemBytes));
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Ensure that all processes have a PATH environment variable.
 **/
//--------------------------------------------------------------------------------------------------
static void EnsurePathIsSet
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    // The default path depends on whether the application is sandboxed or not.
    std::string defaultPath = "/usr/local/bin:/usr/bin:/bin";
    if (appPtr->isSandboxed == false)
    {
        defaultPath = "/legato/systems/current/apps/" + appPtr->name + "/read-only/bin:" + defaultPath;
    }

    // Check all process environments and add the default PATH to any that don't have a PATH
    // environment variable set already.
    for (auto procEnvPtr : appPtr->processEnvs)
    {
        bool pathSpecified = false;

        for (const auto& pair : procEnvPtr->envVars)
        {
            if (pair.first == "PATH")
            {
                pathSpecified = true;
                break;
            }
        }

        if (!pathSpecified)
        {
            procEnvPtr->envVars["PATH"] = defaultPath;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the list of all the required kernel modules of all the components listed.
 */
//--------------------------------------------------------------------------------------------------
void GetRequiredKModules
(
    model::App_t* appPtr,
    model::Component_t* compPtr
)
//--------------------------------------------------------------------------------------------------
{
        for (auto subComponent : compPtr->subComponents)
        {
            if (compPtr->subComponents.size() != 0)
            {
                GetRequiredKModules(appPtr, subComponent.componentPtr);
            }
            appPtr->requiredModules.insert(compPtr->requiredModules.begin(),
                                           compPtr->requiredModules.end());
        }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the list of all the required kernel modules of all the components listed on the executables
 * section of the adef.
 */
//--------------------------------------------------------------------------------------------------
void GetKModuleFromExecs
(
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (const auto& iter : appPtr->executables)
    {
        model::Exe_t *exec = iter.second;
        for (auto const& it : exec->componentInstances)
        {
            model::Component_t *compPtr = it->componentPtr;
            for (auto subComponent : compPtr->subComponents)
            {
                GetRequiredKModules(appPtr, subComponent.componentPtr);

                appPtr->requiredModules.insert(compPtr->requiredModules.begin(),
                                           compPtr->requiredModules.end());
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a single application whose .adef file can be found at a given path.
 *
 * @return Pointer to the application object.
 */
//--------------------------------------------------------------------------------------------------
model::App_t* GetApp
(
    const std::string& adefPath,    ///< Path to the application's .adef file.
    const mk::BuildParams_t& buildParams,
    bool isPreBuilt
)
//--------------------------------------------------------------------------------------------------
{
    // Parse the .adef file.
    const auto adefFilePtr = parser::adef::Parse(adefPath, buildParams.beVerbose);

    // Create a new App_t object for this app.
    auto appPtr = new model::App_t(adefFilePtr);

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Modelling application: '%s'\n"
                                        "  defined in '%s'"), appPtr->name, adefFilePtr->path)
                  << std::endl;
    }

    // Mark if app is pre-built or not.  Affects some diagnostic messages.
    appPtr->isPreBuilt = isPreBuilt;

    // Lists of things that need to be modelled near the end.
    std::list<const parseTree::CompoundItem_t*> processesSections;
    std::list<const parseTree::CompoundItem_t*> bindingsSections;
    std::list<const parseTree::ExternApiInterface_t*> externApiInterfaces;

    // Iterate over the .adef file's list of sections, processing content items.
    for (auto sectionPtr : adefFilePtr->sections)
    {
        auto& sectionName = sectionPtr->firstTokenPtr->text;

        if (sectionName == "bindings")
        {
            // Remember for later, when we know all interfaces have been instantiated in all
            // executables.
            bindingsSections.push_back(sectionPtr);
        }
        else if (sectionName == "bundles")
        {
            AddBundledItems(appPtr, sectionPtr);
        }
        else if (sectionName == "components")
        {
            AddComponents(appPtr, ToTokenListSectionPtr(sectionPtr), buildParams);
        }
        else if (sectionName == "cpuShare")
        {
            appPtr->cpuShare = GetPositiveInt(ToSimpleSectionPtr(sectionPtr));
        }
        else if (sectionName == "executables")
        {
            AddExecutables(appPtr, sectionPtr, buildParams);
        }
        else if (sectionName == "extern")
        {
            auto complexSectionPtr = dynamic_cast<parseTree::ComplexSection_t*>(sectionPtr);
            AddExternApiInterfaces(externApiInterfaces, complexSectionPtr);
            ModelPreBuiltInterfaces(appPtr, complexSectionPtr, buildParams);
        }
        else if (sectionName == "groups")
        {
            AddGroups(appPtr, ToTokenListSectionPtr(sectionPtr));
        }
        else if (sectionName == "maxFileSystemBytes")
        {
            appPtr->maxFileSystemBytes = GetNonNegativeInt(ToSimpleSectionPtr(sectionPtr));
        }
        else if (sectionName == "maxMemoryBytes")
        {
            appPtr->maxMemoryBytes = GetPositiveInt(ToSimpleSectionPtr(sectionPtr));
        }
        else if (sectionName == "maxMQueueBytes")
        {
            appPtr->maxMQueueBytes = GetNonNegativeInt(ToSimpleSectionPtr(sectionPtr));
        }
        else if (sectionName == "maxQueuedSignals")
        {
            appPtr->maxQueuedSignals = GetNonNegativeInt(ToSimpleSectionPtr(sectionPtr));
        }
        else if (sectionName == "maxThreads")
        {
            appPtr->maxThreads = GetPositiveInt(ToSimpleSectionPtr(sectionPtr));
        }
        else if (sectionName == "maxSecureStorageBytes")
        {
            appPtr->maxSecureStorageBytes = GetNonNegativeInt(ToSimpleSectionPtr(sectionPtr));
        }
        else if (sectionName == "processes")
        {
            processesSections.push_back(sectionPtr);
        }
        else if (sectionName == "requires")
        {
            AddRequiredItems(appPtr, sectionPtr, buildParams);
        }
        else if (sectionName == "sandboxed")
        {
            appPtr->isSandboxed = (ToSimpleSectionPtr(sectionPtr)->Text() != "false");
        }
        else if (sectionName == "start")
        {
            SetStart(appPtr, ToSimpleSectionPtr(sectionPtr));
        }
        else if (sectionName == "version")
        {
            // Get the label
            appPtr->version = ToSimpleSectionPtr(sectionPtr)->Text();
            // Check whether it could be an environment variable
            if (appPtr->version[0] == '$')
            {
                // If confirmed, process the label
                appPtr->version = DoSubstitution(appPtr->version, sectionPtr);
            }
        }
        else if (sectionName == "watchdogAction")
        {
            SetWatchdogAction(appPtr, ToSimpleSectionPtr(sectionPtr));
        }
        else if (sectionName == "watchdogTimeout")
        {
            SetWatchdogTimeout(appPtr, ToSimpleSectionPtr(sectionPtr));
        }
        else if (sectionName == "maxWatchdogTimeout")
        {
            SetMaxWatchdogTimeout(appPtr, ToSimpleSectionPtr(sectionPtr));
        }
        else
        {
            sectionPtr->ThrowException(
                mk::format(LE_I18N("Internal error: Unrecognized section '%s'."), sectionName)
            );
        }
    }

    // Model all process environments and processes.
    AddProcessesSections(appPtr, processesSections);

    // Process IPC API externs on executables built by the mk tools.
    // This must be done after all components and executables have been modelled.
    MakeInterfacesExternal(appPtr, externApiInterfaces);

    // Process bindings.  This must be done after all the components and executables have been
    // modelled and all the external API interfaces have been processed.
    AddBindings(appPtr, bindingsSections);

    // Ensure that all processes have a PATH environment variable.
    EnsurePathIsSet(appPtr);

    for(auto it : appPtr->components)
    {
        GetRequiredKModules(appPtr, it);
    }

    GetKModuleFromExecs(appPtr);

    return appPtr;
}


} // namespace modeller
