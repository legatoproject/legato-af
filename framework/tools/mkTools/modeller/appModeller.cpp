//--------------------------------------------------------------------------------------------------
/**
 * @file appModeller.cpp
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"
#include "modellerCommon.h"


namespace modeller
{

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
                    appPtr->bundledFiles.push_back(bundledFilePtr);
                }
                else if (file::AnythingExists(bundledFilePtr->srcPath))
                {
                    bundledFileTokenListPtr->ThrowException("Not a regular file: '"
                                                            + bundledFilePtr->srcPath + "'");
                }
                else
                {
                    bundledFileTokenListPtr->ThrowException("File not found: '"
                                                            + bundledFilePtr->srcPath + "'");
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
                    appPtr->bundledDirs.push_back(bundledDirPtr);
                }
                else if (file::AnythingExists(bundledDirPtr->srcPath))
                {
                    bundledDirTokenListPtr->ThrowException("Not a directory: '"
                                                           + bundledDirPtr->srcPath + "'");
                }
                else
                {
                    bundledDirTokenListPtr->ThrowException("Directory not found: '"
                                                           + bundledDirPtr->srcPath + "'");
                }
            }
        }
        else
        {
            subsectionPtr->ThrowException("Internal error: Unexpected content item: "
                                          + subsectionPtr->TypeName()   );
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Adds an Exe_t object to an application's list of executables, and makes sure all components
 * used by that executable are in the application's list of components.
 */
//--------------------------------------------------------------------------------------------------
static void AddExecutable
(
    model::App_t* appPtr,
    model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    // Add the executable to the app.
    appPtr->executables.push_back(exePtr);

    // Add all the components used in the executable to the app's list of components.
    bool hasSources = false;
    for (auto componentInstancePtr : exePtr->componentInstances)
    {
        auto componentPtr = componentInstancePtr->componentPtr;

        appPtr->components.insert(componentPtr);

        // Remember if this component has sources.
        if (   (componentPtr->cSources.empty() == false)
            || (componentPtr->cxxSources.empty() == false)  )
        {
            hasSources = true;
        }
    }

    // If none of the components in the executable has any source code files, then the executable
    // would just sit there doing nothing, so throw an exception.
    if (!hasSources)
    {
        exePtr->exeDefPtr->ThrowException("Executable doesn't contain any components that have"
                                          " source code files.");
    }
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
                std::cout << "Application '" << appPtr->name << "' contains executable '"
                          << exeName << "'." << std::endl;
            }

            // Compute the path to the executable, relative to the app's working directory
            // and create an object for this exe.
            auto exePtr = new model::Exe_t("staging/bin/" + exeName);
            exePtr->exeDefPtr = itemPtr;

            // Iterate over the list of contents of the executable specification in the parse
            // tree and add each item as a component.
            for (auto token : itemPtr->Contents())
            {
                // Resolve the path to the component.
                std::string componentPath = envVars::DoSubstitution(token->text);

                // Skip if environment variable substitution resulted in an empty string.
                if (!componentPath.empty())
                {
                    componentPath = path::Unquote(componentPath);

                    auto resolvedPath = file::FindComponent(componentPath, buildParams.sourceDirs);
                    if (resolvedPath.empty())
                    {
                        token->ThrowException("Couldn't find component '" + componentPath + "'.");
                    }

                    // Get the component object.
                    auto componentPtr = GetComponent(path::MakeAbsolute(resolvedPath), buildParams);

                    if (buildParams.beVerbose)
                    {
                        std::cout << "Executable '" << exeName << "' in application '"
                                  << appPtr->name << "' contains component '" << componentPtr->name
                                  << "' (" << componentPtr->dir << ")." << std::endl;
                    }

                    // Add an instance of the component to the executable.
                    AddComponentInstance(exePtr, componentPtr);
                }
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
 * Iterate over a "provides:" section process subsections.  Pointers to "api:" subsections will be
 * added to a provided list for later processing.
 */
//--------------------------------------------------------------------------------------------------
static void AddProvidedItems
(
    model::App_t* appPtr,
    std::list<const parseTree::CompoundItem_t*>& apiSubsections, ///< [OUT] List of api subsections.
    const parseTree::Content_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto subsectionPtr : ToCompoundItemListPtr(sectionPtr)->Contents())
    {
        auto& subsectionName = subsectionPtr->firstTokenPtr->text;

        if (subsectionName == "api")
        {
            apiSubsections.push_back(subsectionPtr);
        }
        else
        {
            subsectionPtr->ThrowException("INTERNAL ERROR: Unrecognized section '"
                                          + subsectionName + "'.");
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

    // Check for duplicates.
    if (appPtr->configTrees.find(treeNameTokenPtr->text) != appPtr->configTrees.end())
    {
        treeNameTokenPtr->ThrowException("Configuration tree '" + treeNameTokenPtr->text
                                         + "' appears in application more than once.");
    }

    // Add config tree access permissions to the app.
    appPtr->configTrees[treeNameTokenPtr->text] = permissions;
}


//--------------------------------------------------------------------------------------------------
/**
 * Iterate over a "requires:" section process subsections.  Pointers to "api:" subsections will be
 * added to a provided list for later processing.
 */
//--------------------------------------------------------------------------------------------------
static void AddRequiredItems
(
    model::App_t* appPtr,
    std::list<const parseTree::CompoundItem_t*>& apiSubsections, ///< [OUT] List of api subsections.
    const parseTree::Content_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto subsectionPtr : ToCompoundItemListPtr(sectionPtr)->Contents())
    {
        auto& subsectionName = subsectionPtr->firstTokenPtr->text;

        if (subsectionName == "api")
        {
            apiSubsections.push_back(subsectionPtr);
        }
        else if (subsectionName == "file")
        {
            for (auto itemPtr : parseTree::ToCompoundItemListPtr(subsectionPtr)->Contents())
            {
                auto fileSpecPtr = parseTree::ToTokenListPtr(itemPtr);

                appPtr->requiredFiles.push_back(GetRequiredFileOrDir(fileSpecPtr));
            }
        }
        else if (subsectionName == "dir")
        {
            for (auto itemPtr : parseTree::ToCompoundItemListPtr(subsectionPtr)->Contents())
            {
                auto dirSpecPtr = parseTree::ToTokenListPtr(itemPtr);

                appPtr->requiredDirs.push_back(GetRequiredFileOrDir(dirSpecPtr));
            }
        }
        else if (subsectionName == "device")
        {
            for (auto itemPtr : parseTree::ToCompoundItemListPtr(subsectionPtr)->Contents())
            {
                auto deviceSpecPtr = parseTree::ToTokenListPtr(itemPtr);

                appPtr->requiredDevices.push_back(GetRequiredDevice(deviceSpecPtr));
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
        else
        {
            subsectionPtr->ThrowException("INTERNAL ERROR: Unrecognized section '"
                                          + subsectionName + "'.");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Makes the application a member of groups listed in a given "groups" section in the parse tree.
 */
//--------------------------------------------------------------------------------------------------
static void AddGroups
(
    model::App_t* appPtr,
    const parseTree::TokenListSection_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto tokenPtr : sectionPtr->Contents())
    {
        appPtr->groups.insert(tokenPtr->text);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets whether the Supervisor will start the application automatically at system start-up,
 * or only when asked to do so, based on the contents of a "start:" section in the parse tree.
 */
//--------------------------------------------------------------------------------------------------
static void SetStart
(
    model::App_t* appPtr,
    const parseTree::SimpleSection_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto& mode = sectionPtr->Text();

    if (mode == "auto")
    {
        appPtr->startTrigger = model::App_t::AUTO;
    }
    else if (mode == "manual")
    {
        appPtr->startTrigger = model::App_t::MANUAL;
    }
    else
    {
        sectionPtr->Contents()[0]->ThrowException("Internal error: unexpected startup option.");
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add processes to a process environment, based on the contents of a given run section in
 * the parse tree.
 */
//--------------------------------------------------------------------------------------------------
static void AddProcesses
(
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
            itemPtr->ThrowException("Internal error: '" + itemPtr->TypeName()
                                    + "'' is not a RunProcess_t.");
        }

        auto procPtr = new model::Process_t(processSpecPtr);
        procEnvPtr->processes.push_back(procPtr);

        // If the first token is an open parenthesis, then no process name was specified and
        // the first content token is the executable path, which also is used as the process name.
        // Otherwise, the first content token is the process name, followed by the exe path.
        auto tokens = processSpecPtr->Contents();
        auto i = tokens.begin();
        procPtr->SetName((*i)->text);
        if (processSpecPtr->firstTokenPtr->type != parseTree::Token_t::OPEN_PARENTHESIS)
        {
            i++;
        }
        procPtr->exePath = path::Unquote((*i)->text);

        for (i++ ; i != tokens.end() ; i++)
        {
            procPtr->commandLineArgs.push_back(path::Unquote((*i)->text));
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
            AddProcesses(procEnvPtr, ToCompoundItemListPtr(subsectionPtr));
        }
        else if (subsectionName == "envVars")
        {
            // Each item in this section is a token list with one content item (the value).
            for (auto itemPtr : ToCompoundItemListPtr(subsectionPtr)->Contents())
            {
                auto envVarPtr = ToTokenListPtr(itemPtr);
                auto& name = envVarPtr->firstTokenPtr->text;
                auto& value = envVarPtr->Contents()[0]->text;

                procEnvPtr->envVars[name] = path::Unquote(envVars::DoSubstitution(value));
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
        else
        {
            subsectionPtr->ThrowException("INTERNAL ERROR: Unrecognized section '"
                                          + subsectionName + "'.");
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
 * Mark server-side interface instances as exported for other apps to use, as specified in a
 * given list of "api" subsections from one or more "provides" sections.
 */
//--------------------------------------------------------------------------------------------------
static void ExportInterfaces
(
    model::App_t* appPtr,
    const std::list<const parseTree::CompoundItem_t*>& apiSections
)
//--------------------------------------------------------------------------------------------------
{
    // Set of external interface names used to check for duplicates.
    std::set<std::string> externalNames;

    for (auto sectionPtr : apiSections)
    {
        // Each item in a section is a token list.
        for (auto itemPtr : ToComplexSectionPtr(sectionPtr)->Contents())
        {
            auto tokens = ToTokenListPtr(itemPtr)->Contents();
            model::ApiServerInterfaceInstance_t* ifInstancePtr;
            const parseTree::Token_t* nameTokenPtr;

            // If there are 4 content tokens, the first token is the external name
            // to be used to identify the interface, and the remaining three tokens are the
            // exe, component, and interface names of the interface instance.
            if (tokens.size() == 4)
            {
                ifInstancePtr = appPtr->FindServerInterface(tokens[1], tokens[2], tokens[3]);
                nameTokenPtr = tokens[0];
            }
            // Otherwise, there are 3 content tokens and the interface is exported using the
            // internal name of the interface on the component.
            else
            {
                ifInstancePtr = appPtr->FindServerInterface(tokens[0], tokens[1], tokens[2]);
                nameTokenPtr = tokens[2];
            }
            ifInstancePtr->isExternal = true;
            ifInstancePtr->name = nameTokenPtr->text;

            // Check that there are no duplicates.
            if (externalNames.find(ifInstancePtr->name) != externalNames.end())
            {
                nameTokenPtr->ThrowException("Duplicate server-side (provided) external interface"
                                             " name: '" + ifInstancePtr->name + "'.");
            }
            externalNames.insert(ifInstancePtr->name);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark client-side interface instances as imported from other apps, as specified in a
 * given list of "api" subsections from one or more "provides" sections.
 */
//--------------------------------------------------------------------------------------------------
static void ImportInterfaces
(
    model::App_t* appPtr,
    const std::list<const parseTree::CompoundItem_t*>& apiSections
)
//--------------------------------------------------------------------------------------------------
{
    // Set of external interface names used to check for duplicates.
    std::set<std::string> externalNames;

    for (auto sectionPtr : apiSections)
    {
        // Each item in a section is a token list.
        for (auto itemPtr : ToComplexSectionPtr(sectionPtr)->Contents())
        {
            auto tokens = ToTokenListPtr(itemPtr)->Contents();
            model::ApiClientInterfaceInstance_t* ifInstancePtr;
            const parseTree::Token_t* nameTokenPtr;

            // If there are 4 content tokens, the first token is the external name
            // to be used to identify the interface, and the remaining three tokens are the
            // exe, component, and interface names of the interface instance.
            if (tokens.size() == 4)
            {
                ifInstancePtr = appPtr->FindClientInterface(tokens[1], tokens[2], tokens[3]);
                nameTokenPtr = tokens[0];
            }
            // Otherwise, there are 3 content tokens and the interface is exported using the
            // internal name of the interface on the component.
            else
            {
                ifInstancePtr = appPtr->FindClientInterface(tokens[0], tokens[1], tokens[2]);
                nameTokenPtr = tokens[2];
            }
            ifInstancePtr->isExternal = true;
            ifInstancePtr->name = nameTokenPtr->text;

            // Check that there are no duplicates.
            if (externalNames.find(ifInstancePtr->name) != externalNames.end())
            {
                nameTokenPtr->ThrowException("Duplicate client-side (required) external interface"
                                             " name: '" + ifInstancePtr->name + "'.");
            }
            externalNames.insert(ifInstancePtr->name);
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
    // Function to remove angle brackets from a non-app user name specification in an IPC_AGENT
    // token's text.
    auto removeAngleBrackets = [](const std::string& agentName)
        {
            return agentName.substr(1, agentName.length() - 2);
        };

    // startIndex   startIndex + 1  startIndex + 2
    // NAME         NAME            NAME            = internal binding
    // IPC_AGENT    NAME                            = external binding

    // External binding?
    if (tokens[startIndex]->type == parseTree::Token_t::IPC_AGENT)
    {
        auto& serverAgentName = tokens[startIndex]->text;
        bindingPtr->serverIfName = tokens[startIndex + 1]->text;

        if (serverAgentName[0] == '<')  // non-app user?
        {
            bindingPtr->serverType = model::Binding_t::EXTERNAL_USER;
            bindingPtr->serverAgentName = removeAngleBrackets(serverAgentName);
        }
        else // app
        {
            bindingPtr->serverType = model::Binding_t::EXTERNAL_APP;
            bindingPtr->serverAgentName = serverAgentName;
        }
    }
    else // Internal binding.
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
    // The bindings section is a complex section.
    auto sectionPtr = ToCompoundItemListPtr(bindingsSectionPtr);

    for (auto itemPtr : sectionPtr->Contents())
    {
        auto bindingPtr = new model::Binding_t();

        // Each binding specification inside the bindings section is a token list.
        auto bindingSpecPtr = ToTokenListPtr(itemPtr);
        bindingPtr->parseTreePtr = static_cast<const parseTree::Binding_t*>(bindingSpecPtr);
        auto tokens = bindingSpecPtr->Contents();

        // Is this a "wildcard binding" of all unspecified client interfaces with a given name?
        if (tokens[0]->type == parseTree::Token_t::STAR)
        {
            // 0    1    2         3    4
            // STAR NAME IPC_AGENT NAME      = external binding to user or app
            // STAR NAME NAME      NAME NAME = internal binding to exe
            bindingPtr->clientIfName = tokens[1]->text;
            GetBindingServerSide(bindingPtr, tokens, 2, appPtr);

            // Check for multiple bindings of the same client-side wildcard.
            if (   appPtr->wildcardBindings.find(bindingPtr->clientIfName)
                != appPtr->wildcardBindings.end())
            {
                tokens[1]->ThrowException("Duplicate wildcard binding.");
            }

            // Add to list of wildcard bindings.
            appPtr->wildcardBindings[bindingPtr->clientIfName] = bindingPtr;
        }
        else // Specific client interface binding (not a wildcard binding).
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
                tokens[0]->ThrowException("Client interface bound more than once.");
            }

            // Record the binding in the client-side interface object.
            clientIfPtr->bindingPtr = bindingPtr;
        }

        appPtr->bindings.push_back(bindingPtr);
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
 * Set the app-level watchdog action setting.
 */
//--------------------------------------------------------------------------------------------------
static void SetWatchdogAction
(
    model::App_t* appPtr,
    const parseTree::SimpleSection_t* sectionPtr  ///< Ptr to section in parse tree.
)
//--------------------------------------------------------------------------------------------------
{
    if (appPtr->watchdogAction.IsSet())
    {
        sectionPtr->ThrowException("Only one watchdogAction section allowed.");
    }
    appPtr->watchdogAction = sectionPtr->Text();
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the app-level watchdog timeout setting.
 */
//--------------------------------------------------------------------------------------------------
static void SetWatchdogTimeout
(
    model::App_t* appPtr,
    const parseTree::SimpleSection_t* sectionPtr  ///< Ptr to section in parse tree.
)
//--------------------------------------------------------------------------------------------------
{
    if (appPtr->watchdogTimeout.IsSet())
    {
        sectionPtr->ThrowException("Only one watchdogTimeout section allowed.");
    }

    auto tokenPtr = sectionPtr->Contents()[0];
    if (tokenPtr->type == parseTree::Token_t::NAME)
    {
        appPtr->watchdogTimeout = -1;   // Never timeout (watchdog disabled).
    }
    else
    {
        appPtr->watchdogTimeout = GetInt(sectionPtr);
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
    std::cout << std::endl << "== '" << appPtr->name << "' application summary ==" << std::endl
              << std::endl;

    if (!appPtr->components.empty())
    {
        std::cout << "  Uses components:" << std::endl;

        for (auto componentPtr : appPtr->components)
        {
            std::cout << "    '" << componentPtr->name << "'" << std::endl;
        }
    }

    if (!appPtr->executables.empty())
    {
        std::cout << "  Builds executables:" << std::endl;

        for (auto exePtr : appPtr->executables)
        {
            std::cout << "    '" << exePtr->name << "'" << std::endl;

            if (!exePtr->componentInstances.empty())
            {
                std::cout << "      Instantiates components:" << std::endl;

                for (const auto& componentInstancePtr : exePtr->componentInstances)
                {
                    std::cout << "        '" << componentInstancePtr->componentPtr->name << "'"
                              << std::endl;
                }
            }
        }
    }

    if (!appPtr->bundledFiles.empty())
    {
        std::cout << "  Includes files from the build host:" << std::endl;

        for (auto itemPtr : appPtr->bundledFiles)
        {
            std::cout << "    '" << itemPtr->srcPath << "':" << std::endl;
            std::cout << "      appearing inside app as: '" << itemPtr->destPath
                                                           << "'" << std::endl;
            std::cout << "      permissions:";
            PrintPermissions(itemPtr->permissions);
            std::cout << std::endl;
        }
    }

    if (!appPtr->bundledDirs.empty())
    {
        std::cout << "  Includes directories from the build host:" << std::endl;

        for (auto itemPtr : appPtr->bundledDirs)
        {
            std::cout << "    '" << itemPtr->srcPath << "':" << std::endl;
            std::cout << "      appearing inside app as: '" << itemPtr->destPath
                                                           << "'" << std::endl;
            std::cout << "      permissions:";
            PrintPermissions(itemPtr->permissions);
            std::cout << std::endl;
        }
    }

    if (!appPtr->isSandboxed)
    {
        std::cout << "  WARNING: This application is UNSANDBOXED." << std::endl;
    }
    else
    {
        std::cout << "  Runs inside a sandbox." << std::endl;
        if (!appPtr->requiredFiles.empty())
        {
            std::cout << "  Imports the following files from the target host:" << std::endl;

            for (auto itemPtr : appPtr->requiredFiles)
            {
                std::cout << "    '" << itemPtr->srcPath << "':" << std::endl;
                std::cout << "      appearing inside app as: '" << itemPtr->destPath
                                                               << "'" << std::endl;
            }
        }

        if (!appPtr->requiredDirs.empty())
        {
            std::cout << "  Imports the following directories from the target host:" << std::endl;

            for (auto itemPtr : appPtr->requiredDirs)
            {
                std::cout << "    '" << itemPtr->srcPath << "':" << std::endl;
                std::cout << "      appearing inside app as: '" << itemPtr->destPath
                                                               << "'" << std::endl;
            }
        }

        std::cout << "  Has the following limits:" << std::endl;
        std::cout << "    maxThreads: " << appPtr->maxThreads.Get() << std::endl;
        std::cout << "    maxMQueueBytes: " << appPtr->maxMQueueBytes.Get() << std::endl;
        std::cout << "    maxQueuedSignals: " << appPtr->maxQueuedSignals.Get() << std::endl;
        std::cout << "    maxMemoryBytes: " << appPtr->maxMemoryBytes.Get() << std::endl;
        std::cout << "    cpuShare: " << appPtr->cpuShare.Get() << std::endl;
        std::cout << "    maxFileSystemBytes: " << appPtr->maxFileSystemBytes.Get() << std::endl;

        // Config Tree access.
        std::cout << "  Has access to the following configuration trees:" << std::endl;
        std::cout << "    Its own tree: read + write" << std::endl;
        for (const auto& mapEntry : appPtr->configTrees)
        {
            std::cout << "    " << mapEntry.first << ": ";

            if (mapEntry.second.IsWriteable())
            {
                std::cout << "read + write" << std::endl;
            }
            else
            {
                std::cout << "read only" << std::endl;
            }
        }
    }

    // Start trigger.
    if (appPtr->startTrigger == model::App_t::AUTO)
    {
        std::cout << "  Will be started automatically when the Legato framework starts."
                  << std::endl;
    }
    else
    {
        std::cout << "  Will only start when requested to start." << std::endl;
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
                std::cout << "  When started, will run process:"
                             " '" << procPtr->GetName() << "'" << std::endl;

                // Exe path.
                std::cout << "    Executing file: '" << procPtr->exePath << "'" << std::endl;

                // Command-line args.
                if (procPtr->commandLineArgs.empty())
                {
                    std::cout << "    Without any command line arguments." << std::endl;
                }
                else
                {
                    std::cout << "    With the following command line arguments:" << std::endl;
                    for (const auto& arg : procPtr->commandLineArgs)
                    {
                        std::cout << "      '" << arg << "'" << std::endl;
                    }
                }

                // Priority.
                if (procEnvPtr->GetStartPriority().IsSet())
                {
                    std::cout << "    At priority: "
                              << procEnvPtr->GetStartPriority().Get() << std::endl;
                }

                // Environment variables.
                std::cout << "    With the following environment variables:" << std::endl;
                for (const auto& pair : procEnvPtr->envVars)
                {
                    std::cout << "      " << pair.first << "=" << pair.second << std::endl;
                }

                // Fault action.
                if (procEnvPtr->faultAction.IsSet())
                {
                    std::cout << "    Fault recovery action: "
                              << procEnvPtr->faultAction.Get() << std::endl;
                }
                else
                {
                    std::cout << "    Fault recovery action: ignore (default)" << std::endl;
                }

                // Watchdog.
                if (procEnvPtr->watchdogTimeout.IsSet())
                {
                    std::cout << "    Watchdog timeout: " << procEnvPtr->watchdogTimeout.Get()
                              << std::endl;
                }
                else if (appPtr->watchdogTimeout.IsSet())
                {
                    std::cout << "    Watchdog timeout: " << appPtr->watchdogTimeout.Get()
                              << std::endl;
                }
                if (procEnvPtr->watchdogAction.IsSet())
                {
                    std::cout << "    Watchdog action: " << procEnvPtr->watchdogAction.Get()
                              << std::endl;
                }
                else if (appPtr->watchdogAction.IsSet())
                {
                    std::cout << "    Watchdog action: " << appPtr->watchdogAction.Get()
                              << std::endl;
                }
                if (   (!procEnvPtr->watchdogTimeout.IsSet())
                    && (!procEnvPtr->watchdogAction.IsSet())
                    && (!appPtr->watchdogTimeout.IsSet())
                    && (!appPtr->watchdogAction.IsSet())  )
                {
                    std::cout << "    Watchdog timeout: disabled" << std::endl;
                }

                // Limits.
                if (appPtr->isSandboxed)
                {
                    std::cout << "    With the following limits:" << std::endl;
                    std::cout << "      Max. core dump file size: "
                              << procEnvPtr->maxCoreDumpFileBytes.Get() << " bytes" << std::endl;
                    std::cout << "      Max. file size: "
                              << procEnvPtr->maxFileBytes.Get() << " bytes" << std::endl;
                    std::cout << "      Max. locked memory size: "
                              << procEnvPtr->maxLockedMemoryBytes.Get() << " bytes" << std::endl;
                    std::cout << "      Max. number of file descriptors: "
                              << procEnvPtr->maxFileDescriptors.Get() << std::endl;
                }
            }
        }
    }
    if ((!containsAtLeastOneProcess) && appPtr->isSandboxed)
    {
        std::cout << "  When \"started\", will create a sandbox without running anything in it."
                  << std::endl;
    }

    // Groups
    if (appPtr->isSandboxed && !appPtr->groups.empty())
    {
        std::cout << "  Will be a member of the following access control groups:" << std::endl;
        for (auto& group : appPtr->groups)
        {
            std::cout << "    " << group << std::endl;
        }
    }

    // IPC interfaces and bindings.
    std::list<const model::ApiClientInterfaceInstance_t*> requiredClientIfs;
    std::list<const model::ApiClientInterfaceInstance_t*> boundClientIfs;
    std::list<const model::ApiServerInterfaceInstance_t*> serverIfs;
    for (auto exePtr : appPtr->executables)
    {
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
            std::cout << "  Serves the following IPC API interfaces:" << std::endl;
        }
        for (auto ifPtr : serverIfs)
        {
            std::cout << "    '" << ifPtr->name << "'" << std::endl
                      << "      API defined in: '" << ifPtr->ifPtr->apiFilePtr->path << "'"
                      << std::endl;
        }
        if (!(requiredClientIfs.empty()) || !(boundClientIfs.empty()))
        {
            std::cout << "  Has the following client-side IPC API interfaces:" << std::endl;

            for (auto ifPtr : boundClientIfs)
            {
                std::cout << "    '" << ifPtr->name
                          << "' -> bound to: '" << ifPtr->bindingPtr->serverIfName << "'";
                switch (ifPtr->bindingPtr->serverType)
                {
                    case model::Binding_t::INTERNAL:
                        std::cout << " on another exe inside the same app.";
                        break;
                    case model::Binding_t::EXTERNAL_APP:
                        std::cout << " served by app '"
                                  << ifPtr->bindingPtr->serverAgentName << "'.";
                        break;
                    case model::Binding_t::EXTERNAL_USER:
                        std::cout << " served by user <"
                                  << ifPtr->bindingPtr->serverAgentName << ">.";
                        break;
                }
                std::cout << std::endl
                          << "      API defined in: '" << ifPtr->ifPtr->apiFilePtr->path << "'"
                          << std::endl;
            }

            for (auto ifPtr : requiredClientIfs)
            {
                std::cout << "    '" << ifPtr->name << "' -> UNBOUND." << std::endl
                          << "      API defined in: '" << ifPtr->ifPtr->apiFilePtr->path << "'"
                          << std::endl;
            }
        }
    }

    std::cout << std::endl;
}


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
    std::cerr << "** Warning: application '" << appPtr->name << "': " << warning << std::endl;
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
            std::stringstream warning;
            warning << "maxLockedMemoryBytes (" << maxLockedMemoryBytes
                    << ") will be limited by the maxMemoryBytes limit (" << maxMemoryBytes << ").";
            PrintWarning(appPtr, warning.str());
        }

        size_t maxFileBytes = procEnvPtr->maxFileBytes.Get();
        size_t maxCoreDumpFileBytes = procEnvPtr->maxCoreDumpFileBytes.Get();

        if (maxCoreDumpFileBytes > maxFileBytes)
        {
            std::stringstream warning;
            warning << "maxCoreDumpFileBytes (" << maxCoreDumpFileBytes
                    << ") will be limited by the maxFileBytes limit (" << maxFileBytes << ").";
            PrintWarning(appPtr, warning.str());
        }

        if (maxCoreDumpFileBytes > maxFileSystemBytes)
        {
            std::stringstream warning;
            warning << "maxCoreDumpFileBytes (" << maxCoreDumpFileBytes
                    << ") will be limited by the maxFileSystemBytes limit ("
                    << maxFileSystemBytes << ") if the core file is inside the sandbox temporary"
                    " file system.";
            PrintWarning(appPtr, warning.str());
        }

        if (maxFileBytes > maxFileSystemBytes)
        {
            std::stringstream warning;
            warning << "maxFileBytes (" << maxFileBytes
                    << ") will be limited by the maxFileSystemBytes limit ("
                    << maxFileSystemBytes << ") if the file is inside the sandbox temporary"
                    " file system.";
            PrintWarning(appPtr, warning.str());
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
        defaultPath = "/opt/legato/apps/" + appPtr->name + "/bin:" + defaultPath;
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
 * Get a conceptual model for a single application whose .adef file can be found at a given path.
 *
 * @return Pointer to the application object.
 */
//--------------------------------------------------------------------------------------------------
model::App_t* GetApp
(
    const std::string& adefPath,    ///< Path to the application's .adef file.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Parse the .adef file.
    const auto adefFilePtr = parser::adef::Parse(adefPath, buildParams.beVerbose);

    // Create a new App_t object for this app.
    auto appPtr = new model::App_t(adefFilePtr);

    if (buildParams.beVerbose)
    {
        std::cout << "Modelling application: '" << appPtr->name << "'" << std::endl
                  << "  defined in: '" << adefFilePtr->path << "'" << std::endl;
    }

    // Lists of sections that need to be modelled near the end.
    std::list<const parseTree::CompoundItem_t*> processesSections;
    std::list<const parseTree::CompoundItem_t*> bindingsSections;
    std::list<const parseTree::CompoundItem_t*> requiredApiSections;
    std::list<const parseTree::CompoundItem_t*> providedApiSections;

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
        else if (sectionName == "cpuShare")
        {
            appPtr->cpuShare = GetPositiveInt(ToSimpleSectionPtr(sectionPtr));
        }
        else if (sectionName == "executables")
        {
            AddExecutables(appPtr, sectionPtr, buildParams);
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
        else if (sectionName == "processes")
        {
            processesSections.push_back(sectionPtr);
        }
        else if (sectionName == "provides")
        {
            AddProvidedItems(appPtr, providedApiSections, sectionPtr);
        }
        else if (sectionName == "requires")
        {
            AddRequiredItems(appPtr, requiredApiSections, sectionPtr);
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
            appPtr->version = ToSimpleSectionPtr(sectionPtr)->Text();
        }
        else if (sectionName == "watchdogAction")
        {
            SetWatchdogAction(appPtr, ToSimpleSectionPtr(sectionPtr));
        }
        else if (sectionName == "watchdogTimeout")
        {
            SetWatchdogTimeout(appPtr, ToSimpleSectionPtr(sectionPtr));
        }
        else
        {
            sectionPtr->ThrowException("Internal error: Unrecognized section '" + sectionName
                                       + "'.");
        }
    }

    // Model all process environments and processes.
    AddProcessesSections(appPtr, processesSections);

    // Process IPC API exports and imports.
    ExportInterfaces(appPtr, providedApiSections);
    ImportInterfaces(appPtr, requiredApiSections);

    // Process bindings.
    AddBindings(appPtr, bindingsSections);

    // Ensure that all processes have a PATH environment variable.
    EnsurePathIsSet(appPtr);

    return appPtr;
}



} // namespace modeller
