//--------------------------------------------------------------------------------------------------
/**
 * @file systemModeller.cpp
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
 * Updates an App_t object with the overrides specified for that app in the .sdef file.
 */
//--------------------------------------------------------------------------------------------------
static void ModelAppOverrides
(
    model::App_t* appPtr,
    const parseTree::App_t* appSectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    bool groupsOverridden = false;

    // Iterate over the list of contents of the app section in the parse tree.
    for (auto subsectionPtr : appSectionPtr->Contents())
    {
        auto& subsectionName = subsectionPtr->firstTokenPtr->text;

        if (subsectionName == "cpuShare")
        {
            appPtr->cpuShare = GetPositiveInt(ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "faultAction")
        {
            for (auto procEnvPtr : appPtr->processEnvs)
            {
                procEnvPtr->faultAction = ToSimpleSectionPtr(subsectionPtr)->Text();
            }
        }
        else if (subsectionName == "groups")
        {
            if (!groupsOverridden)
            {
                appPtr->groups.clear();
                groupsOverridden = true;
            }
            AddGroups(appPtr, ToTokenListSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "maxCoreDumpFileBytes")
        {
            for (auto procEnvPtr : appPtr->processEnvs)
            {
                procEnvPtr->maxCoreDumpFileBytes =
                                               GetNonNegativeInt(ToSimpleSectionPtr(subsectionPtr));
            }
        }
        else if (subsectionName == "maxFileBytes")
        {
            for (auto procEnvPtr : appPtr->processEnvs)
            {
                procEnvPtr->maxFileBytes = GetNonNegativeInt(ToSimpleSectionPtr(subsectionPtr));
            }
        }
        else if (subsectionName == "maxFileDescriptors")
        {
            for (auto procEnvPtr : appPtr->processEnvs)
            {
                procEnvPtr->maxFileDescriptors = GetPositiveInt(ToSimpleSectionPtr(subsectionPtr));
            }
        }
        else if (subsectionName == "maxFileSystemBytes")
        {
            appPtr->maxFileSystemBytes = GetNonNegativeInt(ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "maxLockedMemoryBytes")
        {
            for (auto procEnvPtr : appPtr->processEnvs)
            {
                procEnvPtr->maxLockedMemoryBytes =
                                               GetNonNegativeInt(ToSimpleSectionPtr(subsectionPtr));
            }
        }
        else if (subsectionName == "maxMemoryBytes")
        {
            appPtr->maxMemoryBytes = GetPositiveInt(ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "maxMQueueBytes")
        {
            appPtr->maxMQueueBytes = GetNonNegativeInt(ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "maxPriority")
        {
            for (auto procEnvPtr : appPtr->processEnvs)
            {
                procEnvPtr->SetMaxPriority(ToSimpleSectionPtr(subsectionPtr)->Text());
            }
        }
        else if (subsectionName == "maxQueuedSignals")
        {
            appPtr->maxQueuedSignals = GetNonNegativeInt(ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "maxThreads")
        {
            appPtr->maxThreads = GetPositiveInt(ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "maxSecureStorageBytes")
        {
            appPtr->maxSecureStorageBytes = GetNonNegativeInt(ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "sandboxed")
        {
            appPtr->isSandboxed = (ToSimpleSectionPtr(subsectionPtr)->Text() != "false");
        }
        else if (subsectionName == "maxStackBytes")
        {
            for (auto procEnvPtr : appPtr->processEnvs)
            {
                procEnvPtr->maxStackBytes = GetPositiveInt(ToSimpleSectionPtr(subsectionPtr));
            }
        }
        else if (subsectionName == "start")
        {
            SetStart(appPtr, ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "watchdogAction")
        {
            SetWatchdogAction(appPtr, ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "watchdogTimeout")
        {
            SetWatchdogTimeout(appPtr, ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "maxWatchdogTimeout")
        {
            SetMaxWatchdogTimeout(appPtr, ToSimpleSectionPtr(subsectionPtr));
        }
        else if (subsectionName == "preloaded")
        {
            const auto& tokenText = ToSimpleSectionPtr(subsectionPtr)->Text();
            if ((tokenText == "buildVersion") || (tokenText == "true"))
            {
                appPtr->preloadedMode = model::App_t::BUILD_VERSION;
            }
            else if (tokenText == "anyVersion")
            {
                appPtr->preloadedMode = model::App_t::ANY_VERSION;
            }
            else if (tokenText == "false")
            {
                appPtr->preloadedMode = model::App_t::NONE;
            }
            else
            {
                appPtr->preloadedMode = model::App_t::SPECIFIC_MD5;
                appPtr->preloadedMd5 = tokenText;
            }
        }
        else
        {
            subsectionPtr->ThrowException(
                mk::format(LE_I18N("Internal error: Unexpected subsection '%'."), subsectionName)
            );
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Run the tar command to decompress a given app file into the build directory.
 */
//--------------------------------------------------------------------------------------------------
static void UntarBinApp
(
    const std::string& appPath,
    const std::string& destPath,
    const parseTree::App_t* sectionPtr,
    bool isVerbose
)
//--------------------------------------------------------------------------------------------------
{
    std::string flags = isVerbose ? "xvf" : "xf";
    std::string cmdLine = "tar " + flags + " \"" + appPath + "\" -C " + destPath;

    file::MakeDir(destPath);
    int retVal = system(cmdLine.c_str());

    if (retVal != 0)
    {
        std::stringstream msg;
        msg << "Binary app '" << appPath << "' could not be extracted.";
        sectionPtr->ThrowException(msg.str());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Look for the binary app's .adef file in it's extraction directory.
 */
//--------------------------------------------------------------------------------------------------
static std::string FindBinAppAdef
(
    const parseTree::App_t* sectionPtr,
    const std::string& basePath
)
//--------------------------------------------------------------------------------------------------
{
    auto filePaths = file::ListFiles(basePath);

    for (auto const &fileName : filePaths)
    {
        auto pos = fileName.rfind('.');

        if (pos != std::string::npos)
        {
            std::string suffix = fileName.substr(pos);

            if (suffix == ".adef")
            {
                return path::MakeAbsolute(path::Combine(basePath, fileName));
            }
        }
    }

    sectionPtr->ThrowException(LE_I18N("Error could not find binary app .adef file."));
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an App_t object for a given app's subsection within an "apps:" section.
 */
//--------------------------------------------------------------------------------------------------
static void ModelApp
(
    model::System_t* systemPtr,
    const parseTree::App_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string appName;
    std::string filePath;

    // The first token in the app subsection could be the name of an app or a .adef/.app file path.
    // Find the app name and .adef/.app file.
    const auto appSpec = path::Unquote(DoSubstitution(sectionPtr->firstTokenPtr));

    // Build a proper .app suffix that includes the target that the app was built against.
    const std::string appSuffixSigned = "." + buildParams.target + ".signed.app";
    const std::string appSuffix = "." + buildParams.target + ".app";
    bool isBinApp = false;

    if (path::HasSuffix(appSpec, ".adef"))
    {
        appName = path::RemoveSuffix(path::GetLastNode(appSpec), ".adef");
        filePath = file::FindFile(appSpec, buildParams.appDirs);
    }
    else if (path::HasSuffix(appSpec, appSuffix))
    {
        appName = path::RemoveSuffix(path::GetLastNode(appSpec), appSuffix);
        filePath = file::FindFile(appSpec, buildParams.appDirs);
        isBinApp = true;
    }
    else if (path::HasSuffix(appSpec, appSuffixSigned))
    {
        appName = path::RemoveSuffix(path::GetLastNode(appSpec), appSuffixSigned);
        filePath = file::FindFile(appSpec, buildParams.appDirs);
        isBinApp = true;
    }
    else
    {
        appName = path::GetLastNode(appSpec);
        filePath = file::FindFile(appSpec + ".adef", buildParams.appDirs);

        if (filePath.empty())
        {
            filePath = file::FindFile(appSpec + appSuffix, buildParams.appDirs);
            isBinApp = true;
        }
    }

    // If neither adef nor app file has been found, report the error now.
    if (filePath.empty())
    {
        if (appSpec.empty())
        {
            std::cerr << LE_I18N("** Warning: Ignoring empty app specification") << std::endl;
            return;
        }

        std::string formattedMsg =
            mk::format(LE_I18N("Can't find definition file (%s.adef) or binary app (%s) "
                               "for app specification '%s'.\n"
                               "Note: Looked in the following places:\n"),
                               appName,
                               appName + appSuffix,
                               appSpec);

        for (auto& dir : buildParams.appDirs)
        {
            formattedMsg += "    '" + dir + "'\n";
        }

        sectionPtr->ThrowException(formattedMsg);
    }

    // Check for duplicates.
    auto appsIter = systemPtr->apps.find(appName);
    if (appsIter != systemPtr->apps.end())
    {
        std::stringstream msg;

        msg << "App '" << appName << "' added to the system more than once.\n"
            << appsIter->second->parseTreePtr->firstTokenPtr->GetLocation() << ": note: "
            "Previously added here.";
        sectionPtr->ThrowException(msg.str());
    }

    // If this is a binary-only app, then extract it now.
    if (isBinApp)
    {
        std::string dirPath = path::Combine(buildParams.workingDir, "binApps/" + appName);
        std::string newAdefPath = path::Combine(dirPath, appName + ".adef");

        if (buildParams.beVerbose)
        {
            std::cout << "Extracting binary-only app from '" << filePath << "', "
                      << "to '" << dirPath << "'." << std::endl;
        }

        if (!buildParams.readOnly)
        {
            UntarBinApp(filePath, dirPath, sectionPtr, buildParams.beVerbose);
        }

        filePath = FindBinAppAdef(sectionPtr, path::MakeAbsolute(dirPath) + "/");
        //filePath = newAdefPath;
    }

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("System contains app '%s'."), appName) << std::endl;
    }

    // Model this app.
    auto appPtr = GetApp(filePath, buildParams);
    appPtr->parseTreePtr = sectionPtr;

    systemPtr->apps[appName] = appPtr;

    // Now apply any overrides specified in the .sdef file.
    ModelAppOverrides(appPtr, sectionPtr, buildParams);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates an App_t object for each app listed in an "apps:" section.
 */
//--------------------------------------------------------------------------------------------------
static void ModelAppsSection
(
    model::System_t* systemPtr,
    const parseTree::CompoundItem_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto appsSectionPtr = dynamic_cast<const parseTree::CompoundItemList_t*>(sectionPtr);

    for (auto itemPtr : appsSectionPtr->Contents())
    {
        ModelApp(systemPtr, dynamic_cast<const parseTree::App_t*>(itemPtr), buildParams);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a Module_t object for a given kernel module within "kernelModule(s):" section.
 */
//--------------------------------------------------------------------------------------------------
static void ModelKernelModule
(
    model::System_t* systemPtr,
    const parseTree::RequiredModule_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string moduleName;
    std::string modulePath;

    const auto optionalSpec = sectionPtr->lastTokenPtr->text;

    // Tokens in the module subsection are paths to their .mdef file
    // Assume that modules are built outside of Legato
    const auto moduleSpec = path::Unquote(DoSubstitution(sectionPtr->firstTokenPtr));

    if (path::HasSuffix(moduleSpec, ".mdef"))
    {
        moduleName = path::RemoveSuffix(path::GetLastNode(moduleSpec), ".mdef");
        modulePath = file::FindFile(moduleSpec, buildParams.moduleDirs);
    }
    else
    {
        // Try by appending ".mdef" to path
        moduleName = path::GetLastNode(moduleSpec);
        modulePath = file::FindFile(moduleSpec + ".mdef", buildParams.moduleDirs);
    }

    if (modulePath.empty())
    {
        std::string formattedMsg =
            mk::format(LE_I18N("Can't find definition file (.mdef) "
                               "for module specification '%s'.\n"
                               "note: Looked in the following places:\n"), moduleSpec);
        for (auto& dir : buildParams.moduleDirs)
        {
            formattedMsg += "    '" + dir + "'\n";
        }
        sectionPtr->ThrowException(formattedMsg);
    }

    // Check for duplicates.
    auto modulesIter = systemPtr->modules.find(moduleName);
    if (modulesIter != systemPtr->modules.end())
    {
        sectionPtr->ThrowException(
            mk::format(LE_I18N("Module '%s' added to the system more than once.\n"
                               "%s: note: Previously added here."),
                               moduleName,
                            modulesIter->second.modPtr->parseTreePtr->firstTokenPtr->GetLocation())
        );
    }

    // Model this module.
    auto modulePtr = GetModule(modulePath, buildParams);
    modulePtr->parseTreePtr = sectionPtr;

    bool isOptional = false;

    // Check if the module is optional or not.
    if (optionalSpec.compare("[optional]") == 0)
    {
        isOptional = true;
    }

    model::Module_t::ModuleInfoOptional_t modPtrOptional;
    modPtrOptional.modPtr = modulePtr;
    modPtrOptional.isOptional = isOptional;

    systemPtr->modules[moduleName] = modPtrOptional;

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("System contains module '%s'."), moduleName) << std::endl;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a Module_t object for each kernel module listed in "kernelModule(s):" section.
 */
//--------------------------------------------------------------------------------------------------
static void ModelKernelModulesSection
(
    model::System_t* systemPtr,
    const parseTree::CompoundItem_t* sectionPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    auto moduleSectionPtr = dynamic_cast<const parseTree::CompoundItemList_t*>(sectionPtr);

    for (auto itemPtr : moduleSectionPtr->Contents())
    {
        ModelKernelModule(systemPtr,
                          dynamic_cast<const parseTree::RequiredModule_t*>(itemPtr),
                          buildParams);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Model all the kernel modules from all the "kernelModule(s):" sections and add them to a system.
 */
//--------------------------------------------------------------------------------------------------
static void ModelKernelModules
(
    model::System_t* systemPtr,
    const std::list<const parseTree::CompoundItem_t*>& kernelModulesSections,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    for (auto sectionPtr : kernelModulesSections)
    {
        ModelKernelModulesSection(systemPtr, sectionPtr, buildParams);
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
    const parseTree::Token_t* agentTokenPtr,
    const parseTree::Token_t* interfaceTokenPtr,
    model::System_t* systemPtr
)
//--------------------------------------------------------------------------------------------------
{
    const auto& agentName = agentTokenPtr->text;
    const auto& interfaceName = interfaceTokenPtr->text;

    // Set the server interface name.
    bindingPtr->serverIfName = interfaceName;

    // Set the server agent type and name.
    if (agentName[0] == '<')  // non-app user?
    {
        bindingPtr->serverType = model::Binding_t::EXTERNAL_USER;
        bindingPtr->serverAgentName = RemoveAngleBrackets(agentName);
    }
    else // app
    {
        bindingPtr->serverType = model::Binding_t::EXTERNAL_APP;
        bindingPtr->serverAgentName = agentName;

        // Make sure that the server interface actually exists on an app in the system.
        if (!systemPtr->FindServerInterface(agentTokenPtr, interfaceTokenPtr))
        {
            interfaceTokenPtr->ThrowException(
                    mk::format(LE_I18N("App '%s' has no external client-side interface named '%s'"),
                               agentTokenPtr->text,
                               interfaceTokenPtr->text));
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a binding to a non-app user's list of bindings.
 */
//--------------------------------------------------------------------------------------------------
static void AddNonAppUserBinding
(
    model::System_t* systemPtr,
    model::Binding_t* bindingPtr
)
//--------------------------------------------------------------------------------------------------
{
    auto& userName = bindingPtr->clientAgentName;
    auto& interfaceName = bindingPtr->clientIfName;
    model::User_t* userPtr = NULL;

    // Get a pointer to the user object, creating a new one if one doesn't exist for this user yet.
    auto userIter = systemPtr->users.find(userName);
    if (userIter == systemPtr->users.end())
    {
        userPtr = new model::User_t(userName);
        systemPtr->users[userName] = userPtr;
    }
    else
    {
        userPtr = userIter->second;
    }

    // Check to make sure this interface is not already bound to something.
    auto i = userPtr->bindings.find(interfaceName);
    if (i != userPtr->bindings.end())
    {
        std::stringstream msg;

        msg << "Duplicate binding of client-side interface '" << interfaceName
            << "' belonging to non-app user '" + userName + "'.\n"
            << i->second->parseTreePtr->firstTokenPtr->GetLocation()
            << "Previous binding was here.";

        bindingPtr->parseTreePtr->ThrowException(
            mk::format(LE_I18N("Duplicate binding of client-side interface '%s'"
                               " belonging to non-app user '%s'.\n"
                               "%s: note: Previous binding was here."),
                       interfaceName, userName,
                       i->second->parseTreePtr->firstTokenPtr->GetLocation())
        );
    }

    // Add the binding to the user.
    userPtr->bindings[interfaceName] = bindingPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Add all the IPC bindings from a single bindings section to a given system object.
 */
//--------------------------------------------------------------------------------------------------
static void ModelBindingsSection
(
    model::System_t* systemPtr,
    const parseTree::CompoundItem_t* bindingsSectionPtr,
    bool beVerbose
)
//--------------------------------------------------------------------------------------------------
{
    // The bindings section is a list of compound items.
    auto sectionPtr = dynamic_cast<const parseTree::CompoundItemList_t*>(bindingsSectionPtr);

    for (auto itemPtr : sectionPtr->Contents())
    {
        // Each binding specification inside the bindings section is a token list.
        auto bindingSpecPtr = dynamic_cast<const parseTree::Binding_t*>(itemPtr);
        auto tokens = bindingSpecPtr->Contents();

        // Create a new binding object to hold the contents of this binding.
        auto bindingPtr = new model::Binding_t(bindingSpecPtr);

        // The first token is an IPC_AGENT token, which is either an app name or a user name.
        // If it begins with a '<', then it's a user name.  Otherwise, it's an app name.
        if (tokens[0]->text[0] == '<')  // Client is a non-app user.
        {
            // 0         1    2         3
            // IPC_AGENT NAME IPC_AGENT NAME
            auto userName = RemoveAngleBrackets(tokens[0]->text);
            bindingPtr->clientType = model::Binding_t::EXTERNAL_USER;
            bindingPtr->clientAgentName = userName;
            bindingPtr->clientIfName = tokens[1]->text;
            GetBindingServerSide(bindingPtr, tokens[2], tokens[3], systemPtr);

            // Record the binding in the user's list of bindings.
            AddNonAppUserBinding(systemPtr, bindingPtr);
        }
        else // Client is an app.
        {
            // Find the client app.
            auto appPtr = systemPtr->FindApp(tokens[0]);

            // There are three different forms of client interface specifications:
            // - app.interface = set/override an external interface binding.
            // - app.exe.comp.interface = override an internal interface binding.
            // - app.*.interface = override an internal pre-built binding.
            if (tokens[1]->type == parseTree::Token_t::STAR) // pre-built interface binding
            {
                // 0         1    2    3         4
                // IPC_AGENT STAR NAME IPC_AGENT NAME
                bindingPtr->clientType = model::Binding_t::INTERNAL;
                bindingPtr->clientAgentName = appPtr->name;
                bindingPtr->clientIfName = tokens[2]->text;
                GetBindingServerSide(bindingPtr, tokens[3], tokens[4], systemPtr);

                auto i = appPtr->preBuiltClientInterfaces.find(bindingPtr->clientIfName);

                if (i == appPtr->preBuiltClientInterfaces.end())
                {
                    tokens[2]->ThrowException("App '" + appPtr->name + "'"
                                              " doesn't have a pre-built client-side interface"
                                              " named '" + bindingPtr->clientIfName + "'.");
                }

                auto interfacePtr = i->second;

                if (beVerbose)
                {
                    if (interfacePtr->bindingPtr != NULL)
                    {
                        std::cout << mk::format(LE_I18N("Overriding binding of pre-built interface"
                                                        " '%s.*.%s'."),
                                                bindingPtr->clientAgentName,
                                                bindingPtr->clientIfName)
                                  << std::endl;
                    }
                }

                // Update the interface's binding.
                interfacePtr->bindingPtr = bindingPtr;
            }
            else if (tokens.size() == 4) // external interface binding
            {
                // 0         1    2         3
                // IPC_AGENT NAME IPC_AGENT NAME
                auto interfaceTokenPtr = tokens[1];
                auto clientIfPtr = appPtr->FindClientInterface(interfaceTokenPtr);
                if (clientIfPtr == NULL)
                {
                    interfaceTokenPtr->ThrowException(
                        mk::format(LE_I18N("App '%s' has no external client-side "
                                   "interface named '%s'"),
                                   appPtr->name,
                                   interfaceTokenPtr->text));
                }

                bindingPtr->clientType = model::Binding_t::EXTERNAL_APP;
                bindingPtr->clientAgentName = appPtr->name;
                bindingPtr->clientIfName = clientIfPtr->name;
                GetBindingServerSide(bindingPtr, tokens[2], tokens[3], systemPtr);

                if (beVerbose)
                {
                    if (clientIfPtr->bindingPtr != NULL)
                    {
                        std::cout << mk::format(LE_I18N("Overriding binding of '%s.%s'."),
                                                bindingPtr->clientAgentName,
                                                bindingPtr->clientIfName)
                                  << std::endl;
                    }
                }

                // Record the binding in the client-side interface object.
                clientIfPtr->bindingPtr = bindingPtr;
            }
            else // internal interface override
            {
                // 0         1    2    3    4         5
                // IPC_AGENT NAME NAME NAME IPC_AGENT NAME
                auto clientIfPtr = appPtr->FindClientInterface(tokens[1], tokens[2], tokens[3]);
                bindingPtr->clientType = model::Binding_t::INTERNAL;
                bindingPtr->clientAgentName = appPtr->name;
                bindingPtr->clientIfName = clientIfPtr->name;
                GetBindingServerSide(bindingPtr, tokens[4], tokens[5], systemPtr);

                if (beVerbose)
                {
                    if (clientIfPtr->bindingPtr != NULL)
                    {
                        std::cout << mk::format(LE_I18N("Overriding binding of '%s.%s'."),
                                                bindingPtr->clientAgentName,
                                                bindingPtr->clientIfName)
                                  << std::endl;
                    }
                }

                // Record the binding in the client-side interface object.
                clientIfPtr->bindingPtr = bindingPtr;
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add all the RPC bindings.
 */
//--------------------------------------------------------------------------------------------------
static void ModelRpcBindings
(
    model::System_t* systemPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Traverse all external client apis and generate a RPC binding
    for (auto clientIfaceMapEntry : systemPtr->externClientInterfaces)
    {
        auto clientIfaceInstancePtr = clientIfaceMapEntry.second;

        // Create a Client-side binding
        model::Binding_t* bindingPtr = new model::Binding_t(NULL);

        bindingPtr->clientType = model::Binding_t::EXTERNAL_APP;
        bindingPtr->clientAgentName =
            clientIfaceInstancePtr->componentInstancePtr->exePtr->appPtr->name; // Client app name
        bindingPtr->clientIfName = clientIfaceInstancePtr->ifPtr->internalName;

        bindingPtr->serverType = model::Binding_t::EXTERNAL_USER;
        bindingPtr->serverAgentName = "root";
        bindingPtr->serverIfName = clientIfaceInstancePtr->ifPtr->internalName;

        std::cout << "RPC binding client interface: " << bindingPtr->clientIfName << std::endl;

        bindingPtr->parseTreePtr = NULL;

        clientIfaceInstancePtr->bindingPtr = bindingPtr;
    }

    // Traverse all external server apis and generate a RPC binding
    for (auto clientIfaceMapEntry : systemPtr->externServerInterfaces)
    {
        auto clientIfaceInstancePtr = clientIfaceMapEntry.second;

        // Create a Client-side binding
        model::Binding_t* bindingPtr = new model::Binding_t(NULL);

        bindingPtr->serverType = model::Binding_t::EXTERNAL_APP;
        bindingPtr->serverAgentName =
            clientIfaceInstancePtr->componentInstancePtr->exePtr->appPtr->name; // Client app name
        bindingPtr->serverIfName = clientIfaceInstancePtr->ifPtr->internalName;

        bindingPtr->clientType = model::Binding_t::EXTERNAL_USER;
        bindingPtr->clientAgentName = "root";
        bindingPtr->clientIfName = clientIfaceInstancePtr->ifPtr->internalName;

        std::cout << "RPC binding server interface: " << bindingPtr->serverIfName << std::endl;

        bindingPtr->parseTreePtr = NULL;

        // Record the binding in the user's list of bindings.
        AddNonAppUserBinding(systemPtr, bindingPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Model all the apps from all the "apps:" sections and add them to a system.
 */
//--------------------------------------------------------------------------------------------------
static void ModelApps
(
    model::System_t* systemPtr,
    const std::list<const parseTree::CompoundItem_t*>& appsSections,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    for (auto sectionPtr : appsSections)
    {
        ModelAppsSection(systemPtr, sectionPtr, buildParams);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add all the IPC bindings from a list of bindings sections to a given system object.
 */
//--------------------------------------------------------------------------------------------------
static void ModelBindings
(
    model::System_t* systemPtr,
    const std::list<const parseTree::CompoundItem_t*>& bindingsSections,
    bool beVerbose
)
//--------------------------------------------------------------------------------------------------
{
    for (auto bindingsSectionPtr : bindingsSections)
    {
        ModelBindingsSection(systemPtr, bindingsSectionPtr, beVerbose);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add all the commands from a single commands section to a given system.
 */
//--------------------------------------------------------------------------------------------------
static void ModelCommandsSection
(
    model::System_t* systemPtr,
    const parseTree::CompoundItem_t* commandsSectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    // The commands section is a list of compound items.
    auto sectionPtr = dynamic_cast<const parseTree::CompoundItemList_t*>(commandsSectionPtr);

    for (auto itemPtr : sectionPtr->Contents())
    {
        // Each command specification inside the section is a token list.
        auto commandSpecPtr = dynamic_cast<const parseTree::Command_t*>(itemPtr);
        auto tokens = commandSpecPtr->Contents();

        // Create a new command object.
        auto commandPtr = new model::Command_t(commandSpecPtr);

        // The first token is the command name.
        commandPtr->name = path::Unquote(DoSubstitution(tokens[0]));

        // Check for duplicates.
        auto commandIter = systemPtr->commands.find(commandPtr->name);
        if (commandIter != systemPtr->commands.end())
        {
            tokens[0]->ThrowException(
                mk::format(LE_I18N("Command name '%s' used more than once.\n"
                                   "%s: note: Previously used here."),
                           commandPtr->name,
                           commandIter->second->parseTreePtr->firstTokenPtr->GetLocation())
            );
        }

        // The second token is the app name.
        commandPtr->appPtr = systemPtr->FindApp(tokens[1]);

        // The third token is the path to the executable within the read-only section.
        commandPtr->exePath = tokens[2]->text;

        // Make sure the path is absolute.
        if (!path::IsAbsolute(commandPtr->exePath))
        {
            tokens[2]->ThrowException(LE_I18N("Command executable path inside app must begin"
                                              " with '/'."));
        }

        // NOTE: It would be nice to check that the exePath points to something that is executable
        // inside the app, but we don't actually know what's going to be in the app until it is
        // built by ninja, because of the way directory bundling is implemented right now.
        // This should be changed eventually so we can provide a better user experience.
        // commandPtr->appPtr->FindExecutable(tokens[2]);

        // Add the command to the system's map.
        systemPtr->commands[commandPtr->name] = commandPtr;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add all the commands from a list of commands sections to a given system object.
 */
//--------------------------------------------------------------------------------------------------
static void ModelCommands
(
    model::System_t* systemPtr,
    const std::list<const parseTree::CompoundItem_t*>& commandsSections
)
//--------------------------------------------------------------------------------------------------
{
    for (auto commandsSectionPtr : commandsSections)
    {
        ModelCommandsSection(systemPtr, commandsSectionPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get interface search directory paths from an "interfaceSearch:" section and add them to the
 * list in the buildParams object.
 *
 * Get search directory paths from a *Search: section, and add them to the specified list.
 */
//--------------------------------------------------------------------------------------------------
static void ReadSearchDirs
(
    std::list<std::string>& searchPathList,   ///< And add the new search paths to this list.
    const parseTree::TokenList_t* sectionPtr  ///< From the search paths from this section.
)
//--------------------------------------------------------------------------------------------------
{
    // An interfaceSearch section is a list of FILE_PATH tokens.
    for (const auto contentItemPtr : sectionPtr->Contents())
    {
        auto tokenPtr = dynamic_cast<const parseTree::Token_t*>(contentItemPtr);

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
 * Get flags from a "cflags:", "cxxflags:" or "ldflags:" section and add them to the
 * flags list in the buildParams object.
 */
//--------------------------------------------------------------------------------------------------
static void GetToolFlags
(
    std::string *toolFlagsPtr, ///< String to add flags to.
    const parseTree::TokenList_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    // An interfaceSearch section is a list of FILE_PATH tokens.
    for (const auto contentItemPtr : sectionPtr->Contents())
    {
        auto tokenPtr = dynamic_cast<const parseTree::Token_t*>(contentItemPtr);

        auto flag = path::Unquote(DoSubstitution(tokenPtr));

        // If environment variable substitution resulted in an empty string, just ignore this.
        if (!flag.empty())
        {
            *toolFlagsPtr += " ";
            *toolFlagsPtr += flag;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add external watchdog kick timer to system config
 */
//--------------------------------------------------------------------------------------------------
static void GetExternalWdogKick
(
    model::System_t* systemPtr,
    const parseTree::TokenList_t* sectionPtr
)
{
    auto& value = ToSimpleSectionPtr(sectionPtr)->Text();
    systemPtr->externalWatchdogKick = value;
}

//--------------------------------------------------------------------------------------------------
/**
 * Make sure that the required kernel modules listed in app and modules are in sdef.
 */
//--------------------------------------------------------------------------------------------------
static void EnsureRequiredKernelModuleinSystem
(
    model::System_t* systemPtr
)
//--------------------------------------------------------------------------------------------------
{
    for (auto& appMapEntry : systemPtr->apps)
    {
        auto appPtr = appMapEntry.second;

        for (auto const& it : appPtr->requiredModules)
        {
            auto searchModule = systemPtr->modules.find(it.first);
            if (searchModule == systemPtr->modules.end())
            {
                throw mk::Exception_t(
                    mk::format(
                        LE_I18N("Kernel module '%s.mdef' must be listed in sdef file."), it.first)
                );
            }
        }
    }

    for (auto& moduleMapEntry : systemPtr->modules)
    {
        auto modulePtr = moduleMapEntry.second.modPtr;

        for (auto const& it : modulePtr->requiredModules)
        {
            auto searchModule = systemPtr->modules.find(it.first);
            if (searchModule == systemPtr->modules.end())
            {
                throw mk::Exception_t(
                    mk::format(
                        LE_I18N("Kernel module '%s.mdef' must be listed in sdef file."), it.first)
                );
            }
        }

        for (auto const& itMod : modulePtr->requiredSubModules)
        {
            for (auto const& itSubModMap : itMod.second)
            {
                // If a module is required by sub kernel module, check if it is a sub kernel module
                // or not. If it is not a sub kernel module, it should be a mdef file. The mdef file
                // must be listed in the sdef file.
                auto searchSubModule = modulePtr->subKernelModules.find(itSubModMap.first);
                if (searchSubModule == modulePtr->subKernelModules.end())
                {
                    auto searchModule = systemPtr->modules.find(itSubModMap.first);
                    if (searchModule == systemPtr->modules.end())
                    {
                        throw mk::Exception_t(
                            mk::format(
                                LE_I18N("Required module '%s.mdef' must be listed in sdef file."),
                                itSubModMap.first)
                        );
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Mark an interface instance as externally visible for binding at the network level.
 */
//--------------------------------------------------------------------------------------------------
static void MarkInterfaceExternal
(
    model::System_t* systemPtr,
    model::ApiInterfaceInstance_t* ifInstancePtr, ///< Ptr to the interface instance object.
    const parseTree::Token_t* nameTokenPtr    ///< Ptr to token containing name to use outside app.
)
//--------------------------------------------------------------------------------------------------
{
    // Set the system extern flag
    ifInstancePtr->systemExtern = true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark a single API interface instance as externally visible to other systems.
 */
//--------------------------------------------------------------------------------------------------
static void MakeInterfaceExternal
(
    model::System_t* systemPtr,
    const parseTree::Token_t* nameTokenPtr,
    const parseTree::Token_t* applicationTokenPtr,
    const parseTree::Token_t* interfaceTokenPtr
)
//--------------------------------------------------------------------------------------------------
{
    const auto& appName = applicationTokenPtr->text;
    const auto& interfaceName = interfaceTokenPtr->text;

    // Check that there are no other external interfaces using the same name already.
    const auto& name = nameTokenPtr->text;
    if ((systemPtr->externServerInterfaces.find(name) != systemPtr->externServerInterfaces.end()) ||
        (systemPtr->externClientInterfaces.find(name) != systemPtr->externClientInterfaces.end()))
    {
        nameTokenPtr->ThrowException(
            mk::format(LE_I18N("Duplicate external interface name: '%s'."), name)
        );
    }

    // Find the App instance.
    auto appPtr = systemPtr->FindApp(applicationTokenPtr);

    // Retrieve the interface (look in both the client and server interface lists)
    auto componentServerIfPtr = appPtr->FindServerInterface(interfaceTokenPtr);
    auto componentClientIfPtr = appPtr->FindClientInterface(interfaceTokenPtr);

    // Mark the interface "external", and add it to the appropriate list of external interfaces.
    if (componentServerIfPtr && componentClientIfPtr)
    {
        interfaceTokenPtr->ThrowException(
            mk::format(LE_I18N("Internal error: Interface '%s' exported as"
                               " both client and server interface from app '%s'."),
                       interfaceName, appName));
    }
    else if (componentClientIfPtr != NULL)
    {
        MarkInterfaceExternal(systemPtr, componentClientIfPtr, nameTokenPtr);

        systemPtr->externClientInterfaces[name] = componentClientIfPtr;
    }
    else if (componentServerIfPtr != NULL)
    {
        MarkInterfaceExternal(systemPtr, componentServerIfPtr, nameTokenPtr);

        systemPtr->externServerInterfaces[name] = componentServerIfPtr;
    }
    else
    {
        interfaceTokenPtr->ThrowException(
            mk::format(LE_I18N("No such interface '%s' on app '%s'."),
                       interfaceName, appName));
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Mark API interface instances as externally visible to other systems.
 */
//--------------------------------------------------------------------------------------------------
static void MakeInterfacesExternal
(
    model::System_t* systemPtr,
    const std::list<const parseTree::ExternApiInterface_t*>& interfaces
)
//--------------------------------------------------------------------------------------------------
{
    for (auto ifPtr : interfaces)
    {
        // Each interface spec is a token list.
        auto tokens = dynamic_cast<const parseTree::TokenList_t*>(ifPtr)->Contents();

        // If there are 3 content tokens, the first token is the external name
        // to be used to identify the interface, and the remaining two tokens are the
        // appName and interface names of the interface instance.
        if (tokens.size() == 3)
        {
            MakeInterfaceExternal(systemPtr, tokens[0], tokens[1], tokens[2]);
        }
        // Otherwise, there are 2 content tokens and the interface is exported using the
        // internal name of the interface on the component.
        else
        {
            MakeInterfaceExternal(systemPtr, tokens[1], tokens[0], tokens[1]);
        }
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
    if (envVars::Get("LE_CONFIG_CONFIGURED") == "y" &&
        envVars::Get("LE_CONFIG_RPC") != "y")
    {
        // Warn the user if adding RPC interfaces without the appropriate config environment
        // variable set.  This is not an error because the user may be invoking the mk tools
        // directly and may not have set this configuration in the environment.
        sectionPtr->PrintWarning(
            LE_I18N("Declaring RPC interfaces, but LE_CONFIG_RPC is not set.  Are the KConfig "
                "values correctly configured?")
        );
    }

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
 * Add system links to the system
 */
//--------------------------------------------------------------------------------------------------
static void ModelLinks
(
    model::System_t* systemPtr,
    const std::list<const parseTree::TokenList_t*>& links,
    const mk::BuildParams_t &buildParams
)
//--------------------------------------------------------------------------------------------------
{
    for (auto linkPtr : links)
    {
        // Each link spec is a token list.
        auto tokens = dynamic_cast<const parseTree::TokenList_t*>(linkPtr)->Contents();

        model::Link_t *linkModelPtr =
            new model::Link_t { .name = tokens[0]->text,
                                .componentPtr = GetComponent(tokens[1],
                                                             buildParams,
                                                             std::list<std::string>(),
                                                             true) };

        // Throw an execption if the link component contains Java
        if (linkModelPtr->componentPtr->HasJavaCode())
        {
            tokens[0]->ThrowException(LE_I18N("Java is not supported on link components."));
        }

        // Build the Command Line Arguments
        for (size_t i = 2; i < tokens.size(); ++i)
        {
            linkModelPtr->args.push_back(tokens[i]->text);
        }

        systemPtr->links.insert({linkModelPtr->name, linkModelPtr});
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Walk the parse tree for an "links:" section looking for links.  Adds a pointer to each found
 * items to the list provided.
 */
//--------------------------------------------------------------------------------------------------
static void AddLinks
(
    std::list<const parseTree::TokenList_t*>& links, ///< [OUT] List of link items.
    const parseTree::ComplexSection_t* sectionPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (envVars::Get("LE_CONFIG_CONFIGURED") == "y" &&
        envVars::Get("LE_CONFIG_RPC") != "y")
    {
        // Warn the user if adding RPC interfaces without the appropriate config environment
        // variable set.  This is not an error because the user may be invoking the mk tools
        // directly and may not have set this configuration in the environment.
        sectionPtr->PrintWarning(
            LE_I18N("Adding RPC links, but LE_CONFIG_RPC is not set.  Are the KConfig "
                "values correctly configured?")
        );
    }

    // Each item in the section is a ComplexSection_t.
    for (auto itemPtr : sectionPtr->Contents())
    {
        if (itemPtr->type == parseTree::Content_t::TOKEN_LIST_SECTION)
        {
            // Add to the list of links to be processed later.
            links.push_back(dynamic_cast<parseTree::TokenList_t*>(itemPtr));
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Get a conceptual model for a system whose .sdef file can be found at a given path.
 *
 * @return Pointer to the system object.
 */
//--------------------------------------------------------------------------------------------------
model::System_t* GetSystem
(
    const std::string& sdefPath,    ///< Path to the system's .sdef file.
    mk::BuildParams_t& buildParams  ///< [in,out]
)
//--------------------------------------------------------------------------------------------------
{
    // Parse the .sdef file.
    const auto sdefFilePtr = parser::sdef::Parse(sdefPath, buildParams.beVerbose);

    // Create a new System_t object for this system.
    auto systemPtr = new model::System_t(sdefFilePtr);

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Modelling system: '%s'\n"
                                        "  defined in '%s'"),
                                systemPtr->name, sdefFilePtr->path)
                  << std::endl;
    }

    // Lists of things that need to be modelled near the end.
    std::list<const parseTree::CompoundItem_t*> appsSections;
    std::list<const parseTree::CompoundItem_t*> bindingsSections;
    std::list<const parseTree::CompoundItem_t*> commandsSections;
    std::list<const parseTree::CompoundItem_t*> kernelModulesSections;
    std::list<const parseTree::ExternApiInterface_t*> externApiInterfaces;
    std::list<const parseTree::TokenList_t*> linkSections;

    // Iterate over the .sdef file's list of sections, processing content items.
    for (auto sectionPtr : sdefFilePtr->sections)
    {
        auto& sectionName = sectionPtr->firstTokenPtr->text;

        if (sectionName == "apps")
        {
            // Remember for later, when we know all build variables have been added to the
            // environment.
            appsSections.push_back(sectionPtr);
        }
        else if (sectionName == "bindings")
        {
            // Remember for later, when we know all interfaces have been instantiated in all
            // executables.
            bindingsSections.push_back(sectionPtr);
        }
        else if (sectionName == "buildVars")
        {
            // Skip -- these have already been added to build environment env vars by the parser.
        }
        else if (sectionName == "cflags")
        {
            GetToolFlags(&buildParams.cFlags, ToTokenListPtr(sectionPtr));
        }
        else if (sectionName == "commands")
        {
            // Remember for later, when we know all apps have been instantiated.
            commandsSections.push_back(sectionPtr);
        }
        else if (sectionName == "cxxflags")
        {
            GetToolFlags(&buildParams.cxxFlags, ToTokenListPtr(sectionPtr));
        }
        else if (parser::IsNameSingularPlural(sectionName, "kernelModule"))
        {
            kernelModulesSections.push_back(sectionPtr);
        }
        else if (sectionName == "ldflags")
        {
            GetToolFlags(&buildParams.ldFlags, ToTokenListPtr(sectionPtr));
        }
        else if (sectionName == "interfaceSearch")
        {
            ReadSearchDirs(buildParams.interfaceDirs, ToTokenListPtr(sectionPtr));
        }
        else if (sectionName == "moduleSearch")
        {
            ReadSearchDirs(buildParams.moduleDirs, ToTokenListPtr(sectionPtr));
        }
        else if (sectionName == "appSearch")
        {
            ReadSearchDirs(buildParams.appDirs, ToTokenListPtr(sectionPtr));
        }
        else if (sectionName == "componentSearch")
        {
            ReadSearchDirs(buildParams.componentDirs, ToTokenListPtr(sectionPtr));
        }
        else if (sectionName == "externalWatchdogKick")
        {
            GetExternalWdogKick(systemPtr, ToTokenListPtr(sectionPtr));
        }
        else if (sectionName == "extern")
        {
            auto complexSectionPtr = dynamic_cast<parseTree::ComplexSection_t*>(sectionPtr);
            AddExternApiInterfaces(externApiInterfaces, complexSectionPtr);
        }
        else if (sectionName == "links")
        {
            auto complexSectionPtr = dynamic_cast<parseTree::ComplexSection_t*>(sectionPtr);
            AddLinks(linkSections, complexSectionPtr);
        }
        else
        {
            sectionPtr->ThrowException(
                mk::format(LE_I18N("Internal error: Unrecognized section '%s'."), sectionName)
            );
        }
    }

    // Process all the "apps:" sections.  This must be done after all interface search directories
    // have been parsed.
    ModelApps(systemPtr, appsSections, buildParams);

    // Process RPC API externs on executables built by the mk tools.
    // This must be done after all components and executables have been modelled.
    MakeInterfacesExternal(systemPtr, externApiInterfaces);

    // Process bindings.  This must be done after all the components and executables have been
    // modelled and all the external API interfaces have been processed.
    ModelBindings(systemPtr, bindingsSections, buildParams.beVerbose);

    // Add all RPC bindings
    ModelRpcBindings(systemPtr);

    // Ensure that all client-side interfaces have been bound to something.
    EnsureClientInterfacesBound(systemPtr);

    // Model commands.  This must be done after all apps have been modelled.
    ModelCommands(systemPtr, commandsSections);

    // Model kernel modules.  This must be done after all the build environment variable settings
    // have been parsed.
    ModelKernelModules(systemPtr, kernelModulesSections, buildParams);

    // Ensure that all required kernel modules are listed in the kernelModule(s): section of sdef
    EnsureRequiredKernelModuleinSystem(systemPtr);

    // Model system-links
    ModelLinks(systemPtr, linkSections, buildParams);

    return systemPtr;
}





} // namespace modeller
