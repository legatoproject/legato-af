//--------------------------------------------------------------------------------------------------
/**
 * @file configGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "mkTools.h"

namespace config
{

//--------------------------------------------------------------------------------------------------
/**
 * Generate the application version.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateAppVersionConfig
(
    std::ofstream& cfgStream,
    const model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (appPtr->version != "")
    {
        cfgStream << "  \"version\" \"" << appPtr->version << "\"" << std::endl;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for the application-wide limits (including the start-up modes).
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateAppLimitsConfig
(
    std::ofstream& cfgStream,
    const model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (appPtr->isSandboxed == false)
    {
        cfgStream << "  \"sandboxed\" !f" << std::endl;
    }

    if (appPtr->startTrigger == model::App_t::MANUAL)
    {
        cfgStream << "  \"startManual\" !t" << std::endl;
    }

    cfgStream << "  \"maxSecureStorageBytes\" [" << appPtr->maxSecureStorageBytes.Get() << "]" << std::endl;

    cfgStream << "  \"maxThreads\" [" << appPtr->maxThreads.Get() << "]" << std::endl;

    cfgStream << "  \"maxMQueueBytes\" [" << appPtr->maxMQueueBytes.Get() << "]"
              << std::endl;

    cfgStream << "  \"maxQueuedSignals\" [" << appPtr->maxQueuedSignals.Get() << "]"
              << std::endl;

    cfgStream << "  \"maxMemoryBytes\" [" << appPtr->maxMemoryBytes.Get() << "]" << std::endl;

    cfgStream << "  \"cpuShare\" [" << appPtr->cpuShare.Get() << "]" << std::endl;

    if (appPtr->maxFileSystemBytes.IsSet())
    {
        // This is not supported for unsandboxed apps.
        if (appPtr->isSandboxed == false)
        {
            std::cerr << mk::format(LE_I18N("** WARNING: File system size limit being ignored for"
                                            " unsandboxed application '%s'."), appPtr->name)
                      << std::endl;
        }
        else
        {
            cfgStream << "  \"maxFileSystemBytes\" [" << appPtr->maxFileSystemBytes.Get() << "]"
                      << std::endl;
        }
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for the list of groups that the application's user should be a
 * member of.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateGroupsConfig
(
    std::ofstream& cfgStream,
    const model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    const auto& groupsList = appPtr->groups;

    // If the groups list is empty, nothing needs to be done.
    if (groupsList.empty())
    {
        return;
    }

    // Group names are specified by inserting empty leaf nodes under the "groups" branch
    // of the application's configuration tree.
    cfgStream << "  \"groups\"" << std::endl;
    cfgStream << "  {" << std::endl;

    for (auto const &groupName : groupsList)
    {
        cfgStream << "    \"" << groupName << "\" \"\"" << std::endl;
    }

    cfgStream << "  }" << std::endl << std::endl;
}



//--------------------------------------------------------------------------------------------------
/**
 * Generates the configuration for a single file mapping.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateSingleFileMappingConfig
(
    std::ofstream& cfgStream,   ///< Stream to send the configuration to.
    size_t          index,      ///< The index of the file in the files list in the configuration.
    const model::FileSystemObject_t* mappingPtr  ///< The file mapping.
)
//--------------------------------------------------------------------------------------------------
{
    cfgStream << "      \"" << index++ << "\"" << std::endl;
    cfgStream << "      {" << std::endl;
    cfgStream << "        \"src\" \"" << path::EscapeQuotes(mappingPtr->srcPath) << "\""
              << std::endl;
    cfgStream << "        \"dest\" \"" << path::EscapeQuotes(mappingPtr->destPath) << "\""
              << std::endl;
    if (mappingPtr->permissions.IsReadable())
    {
        cfgStream << "        \"isReadable\" !t" << std::endl;
    }
    if (mappingPtr->permissions.IsWriteable())
    {
        cfgStream << "        \"isWritable\" !t" << std::endl;
    }
    if (mappingPtr->permissions.IsExecutable())
    {
        cfgStream << "        \"isExecutable\" !t" << std::endl;
    }
    cfgStream << "      }" << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for a single bundled file's or directory's bind-mount mapping.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateBundledObjectMappingConfig
(
    std::ofstream& cfgStream,   ///< Stream to send the configuration to.
    size_t          index,      ///< Index of the mapping in the files list in the configuration.
    const model::FileSystemObject_t* mappingPtr  ///< The mapping.
)
//--------------------------------------------------------------------------------------------------
{
    // The File Mapping object for a bundled object is different from the File Mapping object
    // for a required object.  The bundled object's source path is a build host file system path.
    // But, we want the Supervisor to bind-mount the file from where it is installed in the
    // target file system.  So, we have to change the source path to an on-target file system
    // path that is relative to the application's install directory.

    // For example, if the app is installed under /legato/systems/current/apps/myApp/
    // then the file /legato/systems/current/apps/myApp/usr/share/beep.wav would appear inside the
    // sandbox under the directory /usr/share/.

    // The mapping object for such a thing would contain the build host path as the source
    // path (which could be anything) and the sandbox path as the destination path
    // which could be either "/usr/share/" or "/usr/share/beep.wav".

    // But, for the bind-mount configuratoin, what we want is a source path relative to the
    // application's install directory.

    // Copy the permissions and destination path as-is.
    model::FileSystemObject_t bindMountMapping(*mappingPtr);

    // The first step of constructing the source path from the dest path is to remove the
    // leading '/'.
    if (mappingPtr->destPath[0] == '/')
    {
        bindMountMapping.srcPath = mappingPtr->destPath.substr(1);
    }
    else
    {
        bindMountMapping.srcPath = mappingPtr->destPath;
    }

    // If the on-target source path we created doesn't yet include a name on the end, then copy
    // the source name from the orginal object in the build host file system.
    if (   bindMountMapping.srcPath.empty()
        || (bindMountMapping.srcPath.back() == '/') )
    {
        bindMountMapping.srcPath += path::GetLastNode(mappingPtr->srcPath);
    }

    GenerateSingleFileMappingConfig(cfgStream, index, &bindMountMapping);
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for all file mappings from outside the application sandbox to inside
 * the sandbox.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateFileMappingConfig
(
    std::ofstream& cfgStream,
    const model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Create the "requires" section.
    cfgStream << "  \"requires\"" << std::endl;
    cfgStream << "  {" << std::endl;

    // Create nodes under "files", where each node is named with an index, starting a 0,
    // and contains a "src" node and a "dest" node.
    cfgStream << "    \"files\"" << std::endl;
    cfgStream << "    {" << std::endl;

    size_t index = 0;

    // .cdef
    for (auto componentPtr : appPtr->components)
    {
        // External files...
        for (auto mappingPtr : componentPtr->requiredFiles)
        {
            GenerateSingleFileMappingConfig(cfgStream, index++, mappingPtr.get());
        }
    }

    // .adef
    for (auto mappingPtr : appPtr->requiredFiles)
    {
        GenerateSingleFileMappingConfig(cfgStream, index++, mappingPtr.get());
    }

    cfgStream << "    }" << std::endl << std::endl;

    // Create nodes under "dirs", where each node is named with an index, starting a 0,
    // and contains a "src" node and a "dest" node.
    cfgStream << "    \"dirs\"" << std::endl;
    cfgStream << "    {" << std::endl;

    index = 0;

    // .cdef
    for (auto componentPtr : appPtr->components)
    {
        for (auto mappingPtr : componentPtr->requiredDirs)
        {
            GenerateSingleFileMappingConfig(cfgStream, index++, mappingPtr.get());
        }
    }

    // .adef
    for (auto mappingPtr : appPtr->requiredDirs)
    {
        GenerateSingleFileMappingConfig(cfgStream, index++, mappingPtr.get());
    }

    cfgStream << "    }" << std::endl;

    // Create nodes under "devices", where each node is named with an index, starting at 0,
    // and contains a "src" node, a "dest" node, and optional permission nodes.
    cfgStream << "    \"devices\"" << std::endl;
    cfgStream << "    {" << std::endl;

    index = 0;

    // .cdef
    for (auto componentPtr : appPtr->components)
    {
        for (auto mappingPtr : componentPtr->requiredDevices)
        {
            GenerateSingleFileMappingConfig(cfgStream, index++, mappingPtr.get());
        }
    }

    // .adef
    for (auto mappingPtr : appPtr->requiredDevices)
    {
        GenerateSingleFileMappingConfig(cfgStream, index++, mappingPtr.get());
    }

    cfgStream << "    }" << std::endl;

    cfgStream << "    \"kernelModules\"" << std::endl;
    cfgStream << "    {" << std::endl;

    // For each parameter in required kernel modules list
    int kmodnum = 1;
    for (auto reqKMod : appPtr->requiredModules)
    {
        auto modulePtr = model::Module_t::GetModule(reqKMod);
        if (modulePtr == NULL)
        {
            throw mk::Exception_t(
                mk::format(LE_I18N("INTERNAL ERROR: '%s' module name not found."), reqKMod)
            );
        }

        if (modulePtr->moduleBuildType == model::Module_t::Prebuilt)
        {
            int kmodprebuiltnum = 1;
            for (auto &mapEntrykoFiles : modulePtr->koFiles)
            {
                cfgStream << "       \"" << "kernelModule" << kmodprebuiltnum << "\" \""
                           << path::GetLastNode(mapEntrykoFiles.first) << "\"\n";
                kmodprebuiltnum++;
            }
        }
        else
        {
            cfgStream << "       \"" << "kernelModule" << kmodnum << "\" \""
                      << reqKMod << ".ko" << "\"\n";
            kmodnum++;
        }
    }

    cfgStream << "    }\n";

    cfgStream << "  }" << std::endl << std::endl;

    // Create the "bundles" section.
    cfgStream << "  \"bundles\"" << std::endl;
    cfgStream << "  {" << std::endl;

    // Create nodes under "files", where each node is named with an index, starting at 0.
    cfgStream << "    \"files\"" << std::endl;
    cfgStream << "    {" << std::endl;

    index = 0;

    // .cdef
    for (auto componentPtr : appPtr->components)
    {
        for (auto mappingPtr : componentPtr->bundledFiles)
        {
            GenerateBundledObjectMappingConfig(cfgStream, index++, mappingPtr.get());
        }
    }

    // .adef
    for (auto mappingPtr : appPtr->bundledFiles)
    {
        GenerateBundledObjectMappingConfig(cfgStream, index++, mappingPtr.get());
    }

    cfgStream << "    }" << std::endl << std::endl;

    // Create nodes under "dirs", where each node is named with an index, starting at 0.
    cfgStream << "    \"dirs\"" << std::endl;
    cfgStream << "    {" << std::endl;

    index = 0;

    // .adef
    for (auto mappingPtr : appPtr->bundledDirs)
    {
        GenerateBundledObjectMappingConfig(cfgStream, index++, mappingPtr.get());
    }

    // .cdef
    for (auto componentPtr : appPtr->components)
    {
        for (auto mappingPtr : componentPtr->bundledDirs)
        {
            GenerateBundledObjectMappingConfig(cfgStream, index++, mappingPtr.get());
        }
    }

    cfgStream << "    }" << std::endl;

    cfgStream << "  }" << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for the environment variable settings for a process.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateProcessEnvVarsConfig
(
    std::ofstream& cfgStream,
    const model::App_t* appPtr,
    const model::ProcessEnv_t* procEnvPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Any environment variables are declared under a node called "envVars".
    // Each env var has its own node, with the name of the node being the name of
    // the environment variable.
    cfgStream << "      \"envVars\"" << std::endl;
    cfgStream << "      {" << std::endl;
    for (const auto& pair : procEnvPtr->envVars)
    {
        cfgStream << "        \"" << pair.first << "\" \"" << path::EscapeQuotes(pair.second)
                  << "\"" << std::endl;
    }

    cfgStream << "      }" << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Search for an executable definition of the given name.
 *
 * @return: The executable definition from the application if found, null if not.
 **/
//--------------------------------------------------------------------------------------------------
static const model::Exe_t* FindExecutable
(
    const model::App_t* appPtr,
    const std::string& executablePath
)
//--------------------------------------------------------------------------------------------------
{
    auto iter = appPtr->executables.find(executablePath);

    if (iter != appPtr->executables.end())
    {
        return iter->second;
    }

    return nullptr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate a Java class path for the given executable definition.
 **/
//--------------------------------------------------------------------------------------------------
static std::string GenerateClassPath
(
    const model::Exe_t* exePtr
)
//--------------------------------------------------------------------------------------------------
{
    std::string classPath = "lib/legato.jar";

    for (auto componentInstPtr : exePtr->componentInstances)
    {
        auto componentPtr = componentInstPtr->componentPtr;

        if (componentPtr->HasJavaCode())
        {
            std::list<std::string> bundledJars;
            componentPtr->GetBundledFilesOfType(model::BundleAccess_t::Dest, ".jar", bundledJars);

            for (const auto& jarFile : bundledJars)
            {
                classPath += ":" + jarFile;
            }

            classPath += ":lib/" +
                path::GetLastNode(componentPtr->getTargetInfo<target::LinuxComponentInfo_t>()->lib);
        }
    }

    classPath += ":bin/" + exePtr->name + ".jar";

    return classPath;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for all the processes that the Supervisor should start when the
 * application is started.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateProcessConfig
(
    std::ofstream& cfgStream,
    const model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Create nodes under "procs", where each process has its own node, named after the process.
    cfgStream << "  \"procs\"" << std::endl;
    cfgStream << "  {" << std::endl;

    for (auto procEnvPtr : appPtr->processEnvs)
    {
        for (auto procPtr : procEnvPtr->processes)
        {
            cfgStream << "    \"" << procPtr->GetName() << "\"" << std::endl;
            cfgStream << "    {" << std::endl;

            // The command-line argument list is an indexed list of arguments under a node called
            // "args", where the first argument (0) must be the executable to run.
            cfgStream << "      \"args\"" << std::endl;
            cfgStream << "      {" << std::endl;


            // Look try to find a matching executable definition in the model.  If it is found then
            // check to see if it's a Java executable.  If it is a Java executable, then modify the
            // run parameters to properly invoke the JVM.  Note that it is possible to run an
            // executable that is not defined in the model, like for instance you might want to
            // bind in a web server to serve web pages from your app.
            int argIndex;
            auto exePtr = FindExecutable(appPtr, procPtr->exePath);

            if (   (exePtr != nullptr)
                && (exePtr->hasJavaCode))
            {
                cfgStream << "        \"0\" \"java\"" << std::endl
                          << "        \"1\" \"-cp\"" << std::endl
                          << "        \"2\" \"" << GenerateClassPath(exePtr) << "\"" << std::endl
                          << "        \"3\" \"io.legato.generated.exe."
                          << procPtr->exePath << ".Main" << "\"" << std::endl;
                argIndex = 4;
            }
            else
            {
                cfgStream << "        \"0\" \"" << path::EscapeQuotes(procPtr->exePath) << "\""
                          << std::endl;
                argIndex = 1;
            }

            for (const auto& arg : procPtr->commandLineArgs)
            {
                cfgStream << "        \"" << argIndex << "\" \"" << path::EscapeQuotes(arg) << "\""
                          << std::endl;
                argIndex++;
            }
            cfgStream << "      }" << std::endl;

            GenerateProcessEnvVarsConfig(cfgStream, appPtr, procEnvPtr);

            // Generate the priority, fault action, and limits configuration.
            if (procEnvPtr->faultAction.IsSet())
            {
                cfgStream << "      \"faultAction\" \"" << procEnvPtr->faultAction.Get() << "\""
                          << std::endl;
            }
            auto& startPriority = procEnvPtr->GetStartPriority();
            if (startPriority.IsSet())
            {
                cfgStream << "      \"priority\" \"" << startPriority.Get() << "\""
                          << std::endl;
            }
            cfgStream << "      \"maxCoreDumpFileBytes\" ["
                      << procEnvPtr->maxCoreDumpFileBytes.Get()
                      << "]" << std::endl;

            cfgStream << "      \"maxFileBytes\" [" << procEnvPtr->maxFileBytes.Get() << "]"
                      << std::endl;

            cfgStream << "      \"maxLockedMemoryBytes\" ["
                      << procEnvPtr->maxLockedMemoryBytes.Get() << "]"
                      << std::endl;

            cfgStream << "      \"maxFileDescriptors\" ["
                      << procEnvPtr->maxFileDescriptors.Get() << "]"
                      << std::endl;

            if (procEnvPtr->watchdogTimeout.IsSet())
            {
                cfgStream << "      \"watchdogTimeout\" [" << procEnvPtr->watchdogTimeout.Get()
                          << "]" << std::endl;
            }
            if (procEnvPtr->maxWatchdogTimeout.IsSet())
            {
                cfgStream << "      \"maxWatchdogTimeout\" ["
                          << procEnvPtr->maxWatchdogTimeout.Get()
                          << "]" << std::endl;
            }
            if (procEnvPtr->watchdogAction.IsSet())
            {
                cfgStream << "      \"watchdogAction\" \"" << procEnvPtr->watchdogAction.Get()
                          << "\"" << std::endl;
            }

            cfgStream << "    }" << std::endl;
        }
    }

    cfgStream << "  }" << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generates the configuration for a single IPC binding to a non-app server running under a given
 * user account name.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateSingleApiBindingToUser
(
    std::ofstream& cfgStream,           ///< Stream to send the configuration to.
    const std::string& clientInterface, ///< Client interface name.
    const std::string& serverUserName,  ///< User name of the server.
    const std::string& serviceName      ///< Service instance name the server will advertise.
)
//--------------------------------------------------------------------------------------------------
{
    cfgStream << "    \"" << clientInterface << "\"" << std::endl;
    cfgStream << "    {" << std::endl;
    cfgStream << "      \"user\" \"" << serverUserName << "\"" << std::endl;
    cfgStream << "      \"interface\" \"" << serviceName << "\"" << std::endl;
    cfgStream << "    }" << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generates the configuration for a single IPC binding to a server running in a given application.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateSingleApiBindingToApp
(
    std::ofstream& cfgStream,           ///< Stream to send the configuration to.
    const std::string& clientInterface, ///< Client interface name.
    const std::string& serverAppName,   ///< Name of the application running the server.
    const std::string& serviceName      ///< Service instance name the server will advertise.
)
//--------------------------------------------------------------------------------------------------
{
    cfgStream << "    \"" << clientInterface << "\"" << std::endl;
    cfgStream << "    {" << std::endl;
    cfgStream << "      \"app\" \"" << serverAppName << "\"" << std::endl;
    cfgStream << "      \"interface\" \"" << serviceName << "\"" << std::endl;
    cfgStream << "    }" << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generates the configuration for a binding for a given app.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateBindingConfig
(
    std::ofstream& cfgStream,       ///< Stream to send the configuration to.
    const model::Binding_t* bindingPtr  ///< Binding to internal exe.component.interface.
)
//--------------------------------------------------------------------------------------------------
{
    switch (bindingPtr->serverType)
    {
        case model::Binding_t::INTERNAL:        // Binding to another exe inside the same app.
        case model::Binding_t::EXTERNAL_APP:    // Binding to an executable inside another app.

            GenerateSingleApiBindingToApp( cfgStream,
                                           bindingPtr->clientIfName,
                                           bindingPtr->serverAgentName,
                                           bindingPtr->serverIfName );
            break;


        case model::Binding_t::EXTERNAL_USER:  // Binding to an executable running outside all apps.

            GenerateSingleApiBindingToUser( cfgStream,
                                            bindingPtr->clientIfName,
                                            bindingPtr->serverAgentName,
                                            bindingPtr->serverIfName );
            break;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for all the IPC bindings for this application's client interfaces.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateBindingsConfig
(
    std::ofstream& cfgStream,
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Create nodes under "bindings", where each binding has its own node, named with the client
    // interface service name.
    cfgStream << "  \"bindings\"" << std::endl;
    cfgStream << "  {" << std::endl;

    // If cross-building for an embedded target (not "localhost"),
    if (buildParams.target != "localhost")
    {
        // Add a bind to the Log Client interface of the Log Control Daemon (which runs as root).
        GenerateSingleApiBindingToUser(cfgStream,
                                       "LogClient",
                                       "root",
                                       "LogClient");
    }

    // Add all the binds that were specified in the .adef file or .sdef file for this app.
    for (const auto mapItem : appPtr->executables)
    {
        const auto exePtr = mapItem.second;

        for (const auto componentInstancePtr : exePtr->componentInstances)
        {
            for (const auto interfacePtr : componentInstancePtr->clientApis)
            {
                if (interfacePtr->bindingPtr != NULL)
                {
                    GenerateBindingConfig(cfgStream, interfacePtr->bindingPtr);
                }
            }
        }
    }
    for (const auto& mapEntry: appPtr->preBuiltClientInterfaces)
    {
        const auto interfacePtr = mapEntry.second;

        if (interfacePtr->bindingPtr == NULL)
        {
            throw mk::Exception_t("Binary app '" + appPtr->name + "' interface binding '" +
                                  interfacePtr->ifPtr->apiFilePtr->defaultPrefix + "' missing.");
        }

        GenerateBindingConfig(cfgStream, interfacePtr->bindingPtr);
    }

    cfgStream << "  }" << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for access control settings for configuration trees.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateConfigTreeAclConfig
(
    std::ofstream& cfgStream,
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Create nodes under "configLimits/acl", where each tree has its own node, named with the
    // tree name, that contains either the word "read" or the word "write".
    cfgStream << "  \"configLimits\"" << std::endl;
    cfgStream << "  {" << std::endl;
    cfgStream << "    \"acl\"" << std::endl;
    cfgStream << "    {" << std::endl;

    // Add all the trees that were specified in the .adef file.
    for (const auto& mapEntry : appPtr->configTrees)
    {
        cfgStream << "      \"" << mapEntry.first << "\" \"";

        if (mapEntry.second.IsWriteable())
        {
            cfgStream << "write";
        }
        else
        {
            cfgStream << "read";
        }

        cfgStream << "\"" << std::endl;
    }

    cfgStream << "    }" << std::endl << std::endl;
    cfgStream << "  }" << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for watchdog settings at the application level.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateAppWatchdogConfig
(
    std::ofstream& cfgStream,
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    if (appPtr->watchdogTimeout.IsSet())
    {
        cfgStream << "  \"watchdogTimeout\" [" << appPtr->watchdogTimeout.Get()
                  << "]" << std::endl;
    }
    if (appPtr->maxWatchdogTimeout.IsSet())
    {
        cfgStream << "  \"maxWatchdogTimeout\" [" << appPtr->maxWatchdogTimeout.Get()
                  << "]" << std::endl;
    }
    if (appPtr->watchdogAction.IsSet())
    {
        cfgStream << "  \"watchdogAction\" \"" << appPtr->watchdogAction.Get()
                  << "\"" << std::endl;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration that the framework needs for a given app.  This is the configuration
 * that will be installed in the system configuration tree by the installer when the app is
 * installed on the target.  It will be output to a file called "root.cfg" in the app's staging
 * directory.
 **/
//--------------------------------------------------------------------------------------------------
void Generate
(
    model::App_t* appPtr,       ///< The app to generate the configuration for.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Combine(buildParams.workingDir, appPtr->ConfigFilePath());

    file::MakeDir(path::GetContainingDir(filePath));

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Generating system configuration data for"
                                        " app '%s' in file '%s'."), appPtr->name, filePath)
                  << std::endl;
    }

    std::ofstream cfgStream(filePath, std::ofstream::trunc);

    if (cfgStream.is_open() == false)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Could not open '%s' for writing."), filePath)
        );
    }

    cfgStream << "{" << std::endl;

    GenerateAppVersionConfig(cfgStream, appPtr);

    GenerateAppLimitsConfig(cfgStream, appPtr);

    GenerateGroupsConfig(cfgStream, appPtr);

    GenerateFileMappingConfig(cfgStream, appPtr);

    GenerateProcessConfig(cfgStream, appPtr);

    GenerateBindingsConfig(cfgStream, appPtr, buildParams);

    GenerateConfigTreeAclConfig(cfgStream, appPtr);

    GenerateAppWatchdogConfig(cfgStream, appPtr);

    cfgStream << "}" << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate kernel module configuration for each module.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateConfigEachModuleFile
(
    model::System_t* systemPtr,
    model::Module_t* modulePtr,
    std::ofstream& cfgStream
)
{
    cfgStream << "  {\n";

    if (modulePtr->loadTrigger == model::Module_t::MANUAL)
    {
        cfgStream << "    \"loadManual\" !t" << std::endl;
    }

    cfgStream << "    \"params\"\n";
    cfgStream << "    {\n";

    // For each parameter in module parameter list
    for (auto mapEntry : modulePtr->params)
    {
        cfgStream << "       \"" << mapEntry.first << "\" \""
                  << mapEntry.second << "\"\n";
    }

    cfgStream << "    }\n";

    cfgStream << "    \"requires\"" << std::endl;
    cfgStream << "    {" << std::endl;
    cfgStream << "      \"kernelModules\"" << std::endl;
    cfgStream << "      {" << std::endl;

    // For each parameter in required kernel modules list
    int kmodnum = 1;
    for (auto setEntry : modulePtr->requiredModules)
    {
        auto mapEntry = systemPtr->modules.find(setEntry);
        if (mapEntry->second->moduleBuildType == model::Module_t::Prebuilt)
        {
            int kmodprebuiltnum = 1;
            for (auto &mapEntrykoFiles : mapEntry->second->koFiles)
            {
                 cfgStream << "         \"" << "kernelModule" << kmodprebuiltnum << "\" \""
                           << path::GetLastNode(mapEntrykoFiles.first) << "\"\n";
                kmodprebuiltnum++;
            }
        }
        else
        {
            cfgStream << "         \"" << "kernelModule" << kmodnum << "\" \""
                      << mapEntry->first << ".ko" << "\"\n";
            kmodnum++;
        }
    }

    cfgStream << "      }\n";
    cfgStream << "    }\n";

    // Create the "bundles" section.
    cfgStream << "    \"bundles\"" << std::endl;
    cfgStream << "    {" << std::endl;

    // Create nodes under "files", where each node is named with an index, starting at 0.
    cfgStream << "      \"file\"" << std::endl;
    cfgStream << "      {" << std::endl;

    size_t index = 0;

    for (auto mappingPtr : modulePtr->bundledFiles)
    {
        GenerateBundledObjectMappingConfig(cfgStream, index++, mappingPtr.get());
    }

    cfgStream << "      }" << std::endl << std::endl;

    // Create nodes under "dirs", where each node is named with an index, starting at 0.
    cfgStream << "      \"dir\"" << std::endl;
    cfgStream << "      {" << std::endl;

    index = 0;

    for (auto mappingPtr : modulePtr->bundledDirs)
    {
        GenerateBundledObjectMappingConfig(cfgStream, index++, mappingPtr.get());
    }

    cfgStream << "      }" << std::endl;

    cfgStream << "    }" << std::endl << std::endl;


    // Create "scripts" section:
    cfgStream << "    \"scripts\"" << std::endl;
    cfgStream << "    {" << std::endl;

    std::string scriptFirstFilePath = "/legato/systems/current/modules/files/";
    std::string scriptSecondFilePath = path::Combine(modulePtr->name, "/scripts/");
    std::string scriptFilePath = path::Combine(scriptFirstFilePath, scriptSecondFilePath);

    if (!modulePtr->installScript.empty())
    {
        std::string installScriptPath = path::Combine(scriptFilePath,
                                                      path::GetLastNode(modulePtr->installScript));
        cfgStream << "      \"install\" \"" << installScriptPath << "\"" << std::endl;
    }
    else
    {
        cfgStream << "      \"install\" \"" << modulePtr->installScript << "\"" << std::endl;
    }

    if (!modulePtr->removeScript.empty())
    {
        std::string removeScriptPath = path::Combine(scriptFilePath,
                                                     path::GetLastNode(modulePtr->removeScript));
        cfgStream << "      \"remove\" \"" << removeScriptPath << "\"" << std::endl;
    }
    else
    {
        cfgStream << "      \"remove\" \"" << modulePtr->removeScript << "\"" << std::endl;
    }

    cfgStream << "    }" << std::endl;

    cfgStream << "  }\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate kernel module configuration in a file called config/modules.cfg under
 * the system's staging directory.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateModulesConfig
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Combine(buildParams.workingDir, "staging/config/modules.cfg");

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Generating module configuration data in file "
                                        "'%s'."), filePath)
                  << std::endl;
    }

    std::ofstream cfgStream(filePath, std::ofstream::trunc);

    if (cfgStream.is_open() == false)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Could not open '%s' for writing."), filePath)
        );
    }

    cfgStream << "{\n";

    // For each module in the system's list of modules,
    for (auto& mapEntry : systemPtr->modules)
    {
        auto modulePtr = mapEntry.second;

        if (modulePtr->moduleBuildType == model::Module_t::Prebuilt)
        {
            for (auto &mapEntrykoFiles : modulePtr->koFiles)
            {
                cfgStream << "  \"" << path::GetLastNode(mapEntrykoFiles.first) << "\"\n";
                GenerateConfigEachModuleFile(systemPtr, modulePtr, cfgStream);
            }
        }
        else
        {
            cfgStream << "  \"" << modulePtr->name << ".ko" << "\"\n";
            GenerateConfigEachModuleFile(systemPtr, modulePtr, cfgStream);
        }
    }

    cfgStream << "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate user binding configuration for non-app users in a file called config/users.cfg under
 * the system's staging directory.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateUsersConfig
(
    model::System_t* systemPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Combine(buildParams.workingDir, "staging/config/users.cfg");

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Generating non-app users' binding configuration data "
                                        "in file '%s'."), filePath)
                  << std::endl;
    }

    std::ofstream cfgStream(filePath, std::ofstream::trunc);

    if (cfgStream.is_open() == false)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Could not open '%s' for writing."), filePath)
        );
    }

    cfgStream << "{\n";

    // For each user in the system's list of non-app users,
    for (auto& mapEntry : systemPtr->users)
    {
        auto userPtr = mapEntry.second;

        cfgStream << "  \"" << userPtr->name << "\"\n";
        cfgStream << "  {\n";
        cfgStream << "    \"bindings\"\n";
        cfgStream << "    {\n";

        // For each binding in the user's list of non-app user bindings,
        for (auto mapEntry : userPtr->bindings)
        {
            GenerateBindingConfig(cfgStream, mapEntry.second);
        }

        cfgStream << "    }\n";
        cfgStream << "  }\n";
    }

    cfgStream << "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a given app's configuration settings to a system configuration output stream.
 *
 * @throw mk::Exception_t if something goes wrong.
 */
//--------------------------------------------------------------------------------------------------
static void AddAppConfig
(
    std::ofstream& cfgStream,    ///< The configuration file being written to.
    model::App_t* appPtr,
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Combine(buildParams.workingDir, appPtr->ConfigFilePath());

    std::ifstream appCfgStream(filePath);

    if (appCfgStream.is_open() == false)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Could not open '%s' for reading."), filePath)
        );
    }

    char buffer[1024];

    while (!appCfgStream.eof())
    {
        appCfgStream.read(buffer, sizeof(buffer));
        cfgStream.write(buffer, appCfgStream.gcount());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the application configuration settings file "apps.cfg" in the "config" directory
 * of the system's staging directory.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateAppsConfig
(
    model::System_t* systemPtr,     ///< The system to generate the configuration for.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Combine(buildParams.workingDir, "staging/config/apps.cfg");

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Generating app configuration data in file '%s'."),
                                filePath)
                  << std::endl;
    }

    std::ofstream cfgStream(filePath, std::ofstream::trunc);

    if (cfgStream.is_open() == false)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Could not open '%s' for writing."), filePath)
        );
    }

    cfgStream << "{\n";

    // For each app in the system,
    for (auto& mapEntry : systemPtr->apps)
    {
        auto appPtr = mapEntry.second;

        cfgStream << "  \"" << appPtr->name << "\"\n";
        AddAppConfig(cfgStream, appPtr, buildParams);
    }

    cfgStream << "}\n";
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the externalWatchdogKick configuration.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateExternalWatchdogKickConfig
(
    std::ofstream& cfgStream,
    const model::System_t* systemPtr
)
{
    if (systemPtr->externalWatchdogKick != "")
    {
        cfgStream << "\"externalWatchdogKick\" [" << systemPtr->externalWatchdogKick << "]" << std::endl;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the framework watchdog configuration settings file "framework.cfg" in the "config"
 * directory of the system's staging directory.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateFrameworkConfig
(
    model::System_t* systemPtr,     ///< The system to generate the configuration for.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    std::string filePath = path::Combine(buildParams.workingDir, "staging/config/framework.cfg");

    if (buildParams.beVerbose)
    {
        std::cout << mk::format(LE_I18N("Generating watchdog configuration data in file '%s'."),
                                filePath)
                  << std::endl;
    }


    std::ofstream cfgStream(filePath, std::ofstream::trunc);

    if (cfgStream.is_open() == false)
    {
        throw mk::Exception_t(
            mk::format(LE_I18N("Could not open '%s' for writing."), filePath)
        );
    }

    cfgStream << "{" << std::endl;

    GenerateExternalWatchdogKickConfig(cfgStream, systemPtr);

    cfgStream << "}" << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration that the framework needs for a given system.  This is the
 * configuration that will be installed in the system configuration tree by the installer when
 * the system starts for the first time on the target.  It will be output to two files called
 * "apps.cfg" and "users.cfg" in the "config" directory under the system's staging directory.
 *
 * @note This assumes that the "root.cfg" config files for all the apps have already been
 *       generated in the apps' staging directories.
 **/
//--------------------------------------------------------------------------------------------------
void Generate
(
    model::System_t* systemPtr,     ///< The system to generate the configuration for.
    const mk::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    file::MakeDir(path::Combine(buildParams.workingDir, "staging/config"));

    GenerateModulesConfig(systemPtr, buildParams);

    GenerateUsersConfig(systemPtr, buildParams);

    GenerateAppsConfig(systemPtr, buildParams);

    GenerateFrameworkConfig(systemPtr, buildParams);
}



} // namespace config
