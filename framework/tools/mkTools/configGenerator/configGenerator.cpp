//--------------------------------------------------------------------------------------------------
/**
 * @file configGenerator.cpp
 *
 * Copyright (C) Sierra Wireless Inc.  Use of this work is subject to license.
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
            std::cerr << "**** Warning: File system size limit being ignored for unsandboxed"
                      << " application '" << appPtr->name << "'." << std::endl;
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

    for (auto groupName : groupsList)
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

    // For example, if the app is installed under /opt/legato/apps/myApp/
    // then the file /opt/legato/apps/myApp/usr/share/beep.wav would appear inside the sandbox
    // under the directory /usr/share/.

    // The mapping object for such a thing would contain the build host path as the source
    // path (which could be anything) and the sandbox path as the destination path
    // which could be either "/usr/share/" or "/usr/share/beep.wav".

    // But, for the bind-mount configuratoin, what we want is a source path relative to the
    // application's install directory.

    // Copy the permissions and destination path as-is.
    model::FileSystemObject_t bindMountMapping(*mappingPtr);

    // The first step of constructing the source path from the dest path is to remove the
    // leading '/'.
    bindMountMapping.srcPath = mappingPtr->destPath.substr(1);

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
            GenerateSingleFileMappingConfig(cfgStream, index++, mappingPtr);
        }
    }

    // .adef
    for (auto mappingPtr : appPtr->requiredFiles)
    {
        GenerateSingleFileMappingConfig(cfgStream, index++, mappingPtr);
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
            GenerateSingleFileMappingConfig(cfgStream, index++, mappingPtr);
        }
    }

    // .adef
    for (auto mappingPtr : appPtr->requiredDirs)
    {
        GenerateSingleFileMappingConfig(cfgStream, index++, mappingPtr);
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
            GenerateSingleFileMappingConfig(cfgStream, index++, mappingPtr);
        }
    }

    // .adef
    for (auto mappingPtr : appPtr->requiredDevices)
    {
        GenerateSingleFileMappingConfig(cfgStream, index++, mappingPtr);
    }

    cfgStream << "    }" << std::endl;

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
            GenerateBundledObjectMappingConfig(cfgStream, index++, mappingPtr);
        }
    }

    // .adef
    for (auto mappingPtr : appPtr->bundledFiles)
    {
        GenerateBundledObjectMappingConfig(cfgStream, index++, mappingPtr);
    }

    cfgStream << "    }" << std::endl << std::endl;

    // Create nodes under "dirs", where each node is named with an index, starting at 0.
    cfgStream << "    \"dirs\"" << std::endl;
    cfgStream << "    {" << std::endl;

    index = 0;

    // .adef
    for (auto mappingPtr : appPtr->bundledDirs)
    {
        GenerateBundledObjectMappingConfig(cfgStream, index++, mappingPtr);
    }

    // .cdef
    for (auto componentPtr : appPtr->components)
    {
        for (auto mappingPtr : componentPtr->bundledDirs)
        {
            GenerateBundledObjectMappingConfig(cfgStream, index++, mappingPtr);
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
            cfgStream << "        \"0\" \"" << path::EscapeQuotes(procPtr->exePath) << "\""
                      << std::endl;
            int argIndex = 1;
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
    for (const auto bindingPtr : appPtr->bindings)
    {
        GenerateBindingConfig(cfgStream, bindingPtr);
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
    if (appPtr->watchdogAction.IsSet())
    {
        cfgStream << "  \"watchdogAction\" \"" << appPtr->watchdogAction.Get()
                  << "\"" << std::endl;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Convert an asset action type into a permission string suitable for the config data.
 *
 * @return The converted string.
 **/
//--------------------------------------------------------------------------------------------------
static std::string AssetActionTypeToStr
(
    model::AssetField_t::ActionType_t actionType
)
//--------------------------------------------------------------------------------------------------
{
    switch (actionType)
    {
        case model::AssetField_t::ActionType_t::TYPE_SETTING:
            return "r";

        case model::AssetField_t::ActionType_t::TYPE_VARIABLE:
            return "rw";

        case model::AssetField_t::ActionType_t::TYPE_COMMAND:
            return "x";

        case model::AssetField_t::ActionType_t::TYPE_UNSET:
            throw mk::Exception_t("Internal error, asset actionType has been left unset.");
    }

    throw mk::Exception_t("Internal error, unexpected value for asset actionType.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Convert the given default value string into something appropriate for the config file format.
 *
 * @return The filtered default value.
 **/
//--------------------------------------------------------------------------------------------------
static std::string FilterDefaultValue
(
    const std::string& dataType,
    const std::string& defaultValue
)
//--------------------------------------------------------------------------------------------------
{
    std::string newDefaultValue;

    if (dataType == "bool")
    {
        // If the value is on or off, convert to true or false.
        if (   (defaultValue == "true")
            || (defaultValue == "on"))
        {
            newDefaultValue = "!t";
        }
        else if (   (defaultValue == "false")
                 || (defaultValue == "off"))
        {
            newDefaultValue = "!f";
        }
    }
    else if (dataType == "int")
    {
        newDefaultValue = "[" + defaultValue + "]";
    }
    else if (dataType == "float")
    {
        newDefaultValue = "(" + defaultValue + ")";
    }
    else if (dataType == "string")
    {
        newDefaultValue = '"' + path::Unquote(defaultValue) + '"';
    }
    else
    {
        throw mk::Exception_t("Internal error, could not filter default value for unexpected data "
                              "type, '" + dataType + ".'");
    }

    return newDefaultValue;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create Legato configuration data for LWM2M objects.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateAssetConfig
(
    std::ofstream& cfgStream,    ///< The configuration file being written to.
    model::App_t* appPtr
)
//--------------------------------------------------------------------------------------------------
{
    // Start the "assets" section and add hard-coded object instances 0 and 1 (the "Application"
    // and "Process" standard objects).
    cfgStream << "  \"assets\"" << std::endl
              << "  {" << std::endl
              << "    \"0\"" << std::endl
              << "    {" << std::endl
              << "      \"name\" \"Application Object\"" << std::endl
              << "      \"fields\"" << std::endl
              << "      {" << std::endl
              << "        \"0\" { \"name\" \"Version\" \"type\" \"string\" \"access\" \"w\" }" << std::endl
              << "        \"1\" { \"name\" \"Name\" \"type\" \"string\" \"access\" \"w\" }" << std::endl
              << "        \"2\" { \"name\" \"State\" \"type\" \"int\" \"access\" \"w\" }" << std::endl
              << "        \"3\" { \"name\" \"StartMode\" \"type\" \"int\" \"access\" \"w\" }" << std::endl
              << "      }" << std::endl
              << "    }" << std::endl
              << "    \"1\"" << std::endl
              << "    {" << std::endl
              << "      \"name\" \"Process Object\"" << std::endl
              << "      \"fields\"" << std::endl
              << "      {" << std::endl
              << "        \"0\" { \"name\" \"Name\" \"type\" \"string\" \"access\" \"w\" }" << std::endl
              << "        \"1\" { \"name\" \"ExecName\" \"type\" \"string\"  \"access\" \"w\" }" << std::endl
              << "        \"2\" { \"name\" \"State\" \"type\" \"int\" \"access\" \"w\" }" << std::endl
              << "        \"3\" { \"name\" \"FaultAction\" \"type\" \"int\" \"access\" \"w\" }" << std::endl
              << "        \"4\" { \"name\" \"FaultCount\" \"type\" \"int\" \"access\" \"w\" }" << std::endl
              << "        \"5\" { \"name\" \"FaultLogs\" \"type\" \"string\" \"access\" \"w\" }" << std::endl
              << "      }" << std::endl
              << "    }" << std::endl;

    // Now, include any user defined assets.
    unsigned int assetId = 1000;
    for (const auto& componentPtr : appPtr->components)
    {
        for (const auto& asset : componentPtr->assets)
        {
            unsigned int fieldId = 0;

            cfgStream << "    \"" << assetId << "\"" << std::endl
                      << "    {" << std::endl
                      << "      \"name\" \"" << asset->GetName() << "\"" << std::endl
                      << "      \"fields\"" << std::endl
                      << "      {" << std::endl;

            for (const auto& fieldPtr : asset->fields)
            {
                cfgStream << "        \"" << fieldId
                          << "\" { \"name\" \"" << fieldPtr->GetName()
                          << "\" \"access\" \"" << AssetActionTypeToStr(fieldPtr->GetActionType())
                          << "\"";

                if (fieldPtr->GetDataType().size() != 0)
                {
                    cfgStream << " \"type\" \"" << fieldPtr->GetDataType() << "\"";
                }

                const auto& defaultValue = fieldPtr->GetDefaultValue();
                if (defaultValue.size() > 0)
                {
                    cfgStream << " \"default\" "
                              << FilterDefaultValue(fieldPtr->GetDataType(), defaultValue);
                }

                cfgStream << " }" << std::endl;
                ++fieldId;
            }

            cfgStream << "      }" << std::endl
                      << "    }" << std::endl;
            ++assetId;
        }
    }

    cfgStream << "  }" << std::endl;
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
    std::string dirPath = path::Combine(buildParams.workingDir, appPtr->workingDir) + "/staging";

    file::MakeDir(dirPath);

    std::string filePath = dirPath + "/root.cfg";

    if (buildParams.beVerbose)
    {
        std::cout << "Generating system configuration data for app '" << appPtr->name << "'"
                     " in file '" << filePath << "'." << std::endl;
    }

    std::ofstream cfgStream(filePath, std::ofstream::trunc);

    if (cfgStream.is_open() == false)
    {
        throw mk::Exception_t("Could not open, '" + filePath + ",' for writing.");
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

    GenerateAssetConfig(cfgStream, appPtr);

    cfgStream << "}" << std::endl;
}


} // namespace config
