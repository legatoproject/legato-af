//--------------------------------------------------------------------------------------------------
/**
 * Implementation of the routines for building Applications.
 *
 * Copyright (C) 2013-2014, Sierra Wireless Inc.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include "ApplicationBuilder.h"
#include "InterfaceBuilder.h"
#include "ComponentBuilder.h"
#include "ExecutableBuilder.h"
#include "Utilities.h"


//--------------------------------------------------------------------------------------------------
/**
 * Generate the application version.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateAppVersionConfig
(
    std::ofstream& cfgStream,
    const legato::App& app
)
//--------------------------------------------------------------------------------------------------
{
    if (app.Version() != "")
    {
        cfgStream << "  \"version\" \"" << app.Version() << "\"" << std::endl;
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
    const legato::App& app
)
//--------------------------------------------------------------------------------------------------
{
    if (app.IsSandboxed() == false)
    {
        cfgStream << "  \"sandboxed\" !f" << std::endl;
    }

    if (app.Debug() == true)
    {
        cfgStream << "  \"debug\" !t" << std::endl;
    }

    if (app.StartMode() == legato::App::MANUAL)
    {
        cfgStream << "  \"deferLaunch\" !t" << std::endl;
    }

    if (app.NumProcs() != SIZE_MAX)
    {
        cfgStream << "  \"numProcessesLimit\" [" << app.NumProcs() << "]" << std::endl;
    }

    if (app.MqueueSize() != SIZE_MAX)
    {
        cfgStream << "  \"totalPosixMsgQueueSizeLimit\" [" << app.MqueueSize() << "]" << std::endl;
    }

    if (app.RtSignalQueueSize() != SIZE_MAX)
    {
        cfgStream << "  \"rtSignalQueueSizeLimit\" [" << app.RtSignalQueueSize() << "]" << std::endl;
    }

    if (app.MemLimit() != SIZE_MAX)
    {
        cfgStream << "\"memLimit\" [" << app.MemLimit() << "]" << std::endl;
    }

    if (app.CpuShare() != SIZE_MAX)
    {
        cfgStream << "\"cpuShare\" [" << app.CpuShare() << "]" << std::endl;
    }

    if (app.FileSystemSize() != SIZE_MAX)
    {
        // This is not supported for unsandboxed apps.
        if (app.IsSandboxed() == false)
        {
            std::cerr << "**** Warning: File system size limit being ignored for unsandboxed"
                      << " application '" << app.Name() << "'." << std::endl;
        }
        else
        {
            cfgStream << "  \"fileSystemSizeLimit\" [" << app.FileSystemSize() << "]" << std::endl;
        }
    }

    if (app.WatchdogTimeout().IsValid())
    {
        cfgStream << "  \"watchdogTimeout\" [" << app.WatchdogTimeout().Get() << "]" << std::endl;
    }

    if (app.WatchdogAction().IsValid())
    {
        cfgStream << "  \"watchdogAction\" \"" << app.WatchdogAction().Get() << "\"" << std::endl;
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
    const legato::App& app
)
//--------------------------------------------------------------------------------------------------
{
    const auto& groupsList = app.Groups();

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
        cfgStream << "  \"" << groupName << "\" \"\"" << std::endl;
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
    const legato::FileMapping& mapping  ///< The file mapping.
)
//--------------------------------------------------------------------------------------------------
{
    cfgStream << "    \"" << index++ << "\"" << std::endl;
    cfgStream << "    {" << std::endl;
    cfgStream << "      \"src\" \"" << mapping.m_SourcePath << "\"" << std::endl;
    cfgStream << "      \"dest\" \"" << mapping.m_DestPath << "\"" << std::endl;
    cfgStream << "    }" << std::endl;
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
    const legato::FileMapping& mapping  ///< The mapping.
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

    legato::FileMapping bindMountMapping;

    // Copy the permissions and destination path as-is.
    bindMountMapping.m_PermissionFlags = mapping.m_PermissionFlags;
    bindMountMapping.m_DestPath = mapping.m_DestPath;

    // The first step of constructing the source path from the dest path is to remove the
    // leading '/'.
    bindMountMapping.m_SourcePath = mapping.m_DestPath.substr(1);

    // If the on-target source path we created doesn't yet include a name on the end, then copy
    // the source name from the orginal object in the build host file system.
    if (   bindMountMapping.m_SourcePath.empty()
        || (bindMountMapping.m_SourcePath.back() == '/') )
    {
        bindMountMapping.m_SourcePath += legato::GetLastPathNode(mapping.m_SourcePath);
    }

    GenerateSingleFileMappingConfig(cfgStream, index, bindMountMapping);
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
    const legato::App& app
)
//--------------------------------------------------------------------------------------------------
{
    size_t index = 0;

    // Create nodes under "files", where each node is named with an index, starting a 0,
    // and contains a "src" node and a "dest" node.
    cfgStream << "  \"files\"" << std::endl;
    cfgStream << "  {" << std::endl;

    // Import the files specified in the .adef file.
    for (const auto& mapping : app.RequiredFiles())
    {
        GenerateSingleFileMappingConfig(cfgStream, index++, mapping);
    }

    // Bundled files also need to be imported into the application sandbox.
    for (const auto& mapping : app.BundledFiles())
    {
        GenerateBundledObjectMappingConfig(cfgStream, index++, mapping);
    }

    // Bundled directories also need to be imported into the application sandbox.
    for (const auto& mapping : app.BundledDirs())
    {
        GenerateBundledObjectMappingConfig(cfgStream, index++, mapping);
    }

    // Map into the sandbox all the files for all the components.
    for (const auto& pair : app.ComponentMap())
    {
        const auto& componentPtr = pair.second;

        // External files...
        for (const auto& mapping : componentPtr->RequiredFiles())
        {
            GenerateSingleFileMappingConfig(cfgStream, index++, mapping);
        }

        // External directories...
        for (const auto& mapping : componentPtr->RequiredDirs())
        {
            GenerateSingleFileMappingConfig(cfgStream, index++, mapping);
        }

        // NOTE: Bundled files and directories also need to be mapped into the application sandbox
        // because the application's on-target install directory is outside its runtime sandbox.

        // Bundled files...
        for (const auto& mapping : componentPtr->BundledFiles())
        {
            GenerateBundledObjectMappingConfig(cfgStream, index++, mapping);
        }

        // Bundled directories...
        for (const auto& mapping : componentPtr->BundledDirs())
        {
            GenerateBundledObjectMappingConfig(cfgStream, index++, mapping);
        }
    }

    // Map into the sandbox all the app's own executable binaries.
    for (const auto& mapEntry : app.Executables())
    {
        const auto& exe = mapEntry.second;

        legato::FileMapping importMapping =
                                    {
                                        legato::PERMISSION_READABLE | legato::PERMISSION_EXECUTABLE,
                                        std::string("bin/") + exe.OutputPath(),
                                        "/bin/"
                                    };

        GenerateSingleFileMappingConfig(cfgStream, index++, importMapping);
    }

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
    const legato::App& app,
    const legato::ProcessEnvironment& procEnv
)
//--------------------------------------------------------------------------------------------------
{
    // The PATH environment variable has to be handled specially.  If no PATH variable is
    // specified in the .adef, we must provide one.
    bool pathSpecified = false;

    // Any environment variables are declared under a node called "envVars".
    // Each env var has its own node, with the name of the node being the name of
    // the environment variable.
    cfgStream << "      \"envVars\"" << std::endl;
    cfgStream << "      {" << std::endl;
    for (const auto& pair : procEnv.EnvVarList())
    {
        if (pair.first == "PATH")
        {
            pathSpecified = true;
        }

        cfgStream << "        \"" << pair.first << "\" \"" << pair.second << "\""
                  << std::endl;
    }

    if (!pathSpecified)
    {
        // The default path depends on whether the application is sandboxed or not.
        std::string path = "/usr/local/bin:/usr/bin:/bin";
        if (app.IsSandboxed() == false)
        {
            path = "/opt/legato/apps/" + app.Name() + "/bin:" + path;
        }
        cfgStream << "        \"PATH\" \"" << path << "\"" << std::endl;
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
    const legato::App& app
)
//--------------------------------------------------------------------------------------------------
{
    // Create nodes under "procs", where each process has its own node, named after the process.
    cfgStream << "  \"procs\"" << std::endl;
    cfgStream << "  {" << std::endl;

    for (const auto& procEnv : app.ProcEnvironments())
    {
        for (const auto& process : procEnv.ProcessList())
        {
            cfgStream << "    \"" << process.Name() << "\"" << std::endl;
            cfgStream << "    {" << std::endl;

            // If the process has debugging enabled, then add a "debug" configuration
            // node for this process that contains the debug port number to be used.
            if (process.IsDebuggingEnabled())
            {
                cfgStream << "      \"debug\" [" << process.DebugPort() << "]" << std::endl;
            }

            // The command-line argument list is an indexed list of arguments under a node called
            // "args", where the first argument (0) must be the executable to run.
            cfgStream << "      \"args\"" << std::endl;
            cfgStream << "      {" << std::endl;
            cfgStream << "        \"0\" \"" << process.ExePath() << "\"" << std::endl;
            int argIndex = 1;
            for (const auto& arg : process.CommandLineArgs())
            {
                cfgStream << "        \"" << argIndex << "\" \"" << arg << "\"" << std::endl;
                argIndex++;
            }
            cfgStream << "      }" << std::endl;

            GenerateProcessEnvVarsConfig(cfgStream, app, procEnv);

            // Generate the priority, fault action, and limits configuration.
            if (procEnv.FaultAction() != "")
            {
                cfgStream << "      \"faultAction\" \"" << procEnv.FaultAction() << "\""
                          << std::endl;
            }
            if (procEnv.Priority() != "")
            {
                cfgStream << "      \"priority\" \"" << procEnv.Priority() << "\""
                          << std::endl;
            }
            if (procEnv.CoreFileSize() != SIZE_MAX)
            {
                cfgStream << "      \"coreDumpFileSizeLimit\" [" << procEnv.CoreFileSize() << "]"
                          << std::endl;
            }
            if (procEnv.MaxFileSize() != SIZE_MAX)
            {
                cfgStream << "      \"maxFileSizeLimit\" [" << procEnv.MaxFileSize() << "]"
                          << std::endl;
            }
            if (procEnv.MemLockSize() != SIZE_MAX)
            {
                cfgStream << "      \"memLockSizeLimit\" [" << procEnv.MemLockSize() << "]"
                          << std::endl;
            }
            if (procEnv.NumFds() != SIZE_MAX)
            {
                cfgStream << "      \"numFileDescriptorsLimit\" [" << procEnv.NumFds() << "]"
                          << std::endl;
            }
            if (procEnv.WatchdogTimeout().IsValid())
            {
                cfgStream << "      \"watchdogTimeout\" [" << procEnv.WatchdogTimeout().Get() << "]"
                          << std::endl;
            }
            if (procEnv.WatchdogAction().IsValid())
            {
                cfgStream << "      \"watchdogAction\" \"" << procEnv.WatchdogAction().Get() << "\""
                          << std::endl;
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
    const std::string& protocolId,      ///< Protocol identifier string.
    const std::string& serverUserName,  ///< User name of the server.
    const std::string& serviceName      ///< Service instance name the server will advertise.
)
//--------------------------------------------------------------------------------------------------
{
    cfgStream << "    \"" << clientInterface << "\"" << std::endl;
    cfgStream << "    {" << std::endl;
    cfgStream << "      \"protocol\" \"" << protocolId << "\"" << std::endl;
    cfgStream << "      \"server\"" << std::endl;
    cfgStream << "      {" << std::endl;
    cfgStream << "        \"user\" \"" << serverUserName << "\"" << std::endl;
    cfgStream << "        \"interface\" \"" << serviceName << "\"" << std::endl;
    cfgStream << "      }" << std::endl;
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
    const std::string& protocolId,      ///< Protocol identifier string.
    const std::string& serverAppName,   ///< Name of the application running the server.
    const std::string& serviceName      ///< Service instance name the server will advertise.
)
//--------------------------------------------------------------------------------------------------
{
    cfgStream << "    \"" << clientInterface << "\"" << std::endl;
    cfgStream << "    {" << std::endl;
    cfgStream << "      \"protocol\" \"" << protocolId << "\"" << std::endl;
    cfgStream << "      \"server\"" << std::endl;
    cfgStream << "      {" << std::endl;
    cfgStream << "        \"app\" \"" << serverAppName << "\"" << std::endl;
    cfgStream << "        \"interface\" \"" << serviceName << "\"" << std::endl;
    cfgStream << "      }" << std::endl;
    cfgStream << "    }" << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generates the configuration for an External API Bind object for a given App.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateExternalApiBindConfig
(
    std::ofstream& cfgStream,       ///< Stream to send the configuration to.
    legato::App& app,               ///< The application being built.
    const legato::ExternalApiBind& binding  ///< Binding to external user name and service name.
)
//--------------------------------------------------------------------------------------------------
{
    std::cout << "under app '" << app.Name() << "'." << std::endl;

    const auto& interface = app.FindClientInterface(binding.ClientInterface());

    // If there is no server user name,
    if (binding.ServerUserName().empty())
    {
        // Make sure there's a server app name.
        if (binding.ServerAppName().empty())
        {
            throw legato::Exception("INTERNAL ERROR: Neither user name nor app name provided"
                                    " for server in binding of '" + binding.ClientInterface()
                                    + "'.");
        }

        GenerateSingleApiBindingToApp( cfgStream,
                                       interface.ServiceInstanceName(),
                                       interface.Api().Hash(),
                                       binding.ServerAppName(),
                                       binding.ServerServiceName() );
    }
    // If there is a server user name,
    else
    {
        // Make sure there isn't also a server app name.
        if (!binding.ServerAppName().empty())
        {
            throw legato::Exception("INTERNAL ERROR: Both user name and app name provided"
                                    " for server in binding of '" + binding.ClientInterface()
                                    + "'.");
        }

        GenerateSingleApiBindingToUser( cfgStream,
                                        interface.ServiceInstanceName(),
                                        interface.Api().Hash(),
                                        binding.ServerUserName(),
                                        binding.ServerServiceName() );
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration for all the IPC bindings for this application's client interfaces.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateIpcBindingConfig
(
    std::ofstream& cfgStream,
    legato::App& app,
    const legato::BuildParams_t& buildParams ///< Build parameters, such as the "is verbose" flag.
)
//--------------------------------------------------------------------------------------------------
{
    // Create nodes under "bindings", where each binding has its own node, named with the client
    // interface service name.
    cfgStream << "  \"bindings\"" << std::endl;
    cfgStream << "  {" << std::endl;

    // If cross-building for an embedded target (not "localhost"),
    if (buildParams.Target() != "localhost")
    {
        // Add a bind to the Log Client interface of the Log Control Daemon (which runs as root).
        GenerateSingleApiBindingToUser(cfgStream,
                                       "LogClient",
                                       "LogControlProtocol",
                                       "root",
                                       "LogClient");
    }

    // Add all the binds that were specified in the .adef file.
    for (const auto& mapEntry : app.ExternalApiBinds())
    {
        GenerateExternalApiBindConfig(cfgStream, app, mapEntry.second);
    }

    // Add binds for framework APIs that are used.
    if (app.UsesConfig())
    {
        GenerateSingleApiBindingToUser(cfgStream,
                                       "le_cfg",
                                       mk::ConfigApiHash(buildParams),
                                       "root",
                                       "le_cfg");
    }
    if (app.UsesWatchdog())
    {
        GenerateSingleApiBindingToUser(cfgStream,
                                       "le_wdog",
                                       mk::WatchdogApiHash(buildParams),
                                       "root",
                                       "le_wdog");
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
    legato::App& app
)
//--------------------------------------------------------------------------------------------------
{
    const char readable[] = "read";
    const char writeable[] = "write";
    const char* accessModePtr;

    // Create nodes under "configLimits/acl", where each tree has its own node, named with the
    // tree name, that contains either the word "read" or the word "write".
    cfgStream << "  \"configLimits\"" << std::endl;
    cfgStream << "  {" << std::endl;
    cfgStream << "    \"acl\"" << std::endl;
    cfgStream << "    {" << std::endl;

    // Add all the trees that were specified in the .adef file.
    for (const auto& mapEntry : app.ConfigTrees())
    {
        if (mapEntry.second & legato::PERMISSION_WRITEABLE)
        {
            accessModePtr = writeable;
        }
        else
        {
            accessModePtr = readable;
        }
        cfgStream << "      \"" << mapEntry.first << "\" \"" << accessModePtr << "\"" << std::endl;
    }

    cfgStream << "    }" << std::endl << std::endl;
    cfgStream << "  }" << std::endl << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Generate the configuration that the Supervisor needs for this app.  This is the configuration
 * that will be installed in the root configuration tree by the installer when the app is
 * installed on the target.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateSupervisorConfig
(
    const std::string& stagingDirPath,  ///< Path to the root of the app's staging directory.
    legato::App& app,                   ///< The app to generate the configuration for.
    const legato::BuildParams_t& buildParams ///< Build parameters, such as the "is verbose" flag.
)
//--------------------------------------------------------------------------------------------------
{
    std::string path = stagingDirPath + "/root.cfg";

    if (buildParams.IsVerbose())
    {
        std::cout << "Generating Supervisor configuration for app '" << app.Name() << "' under '"
                  << path << "'." << std::endl;
    }

    std::ofstream cfgStream(path, std::ofstream::trunc);

    cfgStream << "{" << std::endl;

    GenerateAppVersionConfig(cfgStream, app);

    GenerateAppLimitsConfig(cfgStream, app);

    GenerateGroupsConfig(cfgStream, app);

    GenerateFileMappingConfig(cfgStream, app);

    GenerateProcessConfig(cfgStream, app);

    GenerateIpcBindingConfig(cfgStream, app, buildParams);

    GenerateConfigTreeAclConfig(cfgStream, app);

    cfgStream << "}" << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Build a component and all its sub-components (unless they've already been built).
 */
//--------------------------------------------------------------------------------------------------
static void BuildComponent
(
    legato::Component& component,
    const legato::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Make a copy of the Build Params for use by the Component Builder.
    legato::BuildParams_t componentBuildParams(buildParams);

    // Generate the path to the directory in which the generated files will go
    // and store it in the Build Params object that will be given to the Component Builder.
    componentBuildParams.ObjOutputDir(buildParams.ObjOutputDir()
                                      + "/components/"
                                      + component.Name());

    // Build the component.
    ComponentBuilder_t componentBuilder(componentBuildParams);
    componentBuilder.Build(component);
}


//--------------------------------------------------------------------------------------------------
/**
 * Builds all the components in an application and copies all their files into the staging area.
 **/
//--------------------------------------------------------------------------------------------------
static void BuildComponents
(
    legato::App& app,
    const legato::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // For each component in the application.
    auto& map = app.ComponentMap();
    for (auto& mapEntry : map)
    {
        auto& componentPtr = mapEntry.second;

        BuildComponent(*componentPtr, buildParams);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Builds all the executables in an application and their IPC interface libs and copies all their
 * files into the staging area.
 **/
//--------------------------------------------------------------------------------------------------
static void BuildExecutables
(
    legato::App& app,
    const legato::BuildParams_t& buildParams
)
//--------------------------------------------------------------------------------------------------
{
    // Create an Executable Builder object.
    ExecutableBuilder_t exeBuilder(buildParams);

    // For each executable,
    auto& exeList = app.Executables();
    for (auto i = exeList.begin(); i != exeList.end(); i++)
    {
        legato::Executable& exe = i->second;

        // Auto-generate the source code file containing main() and add it to the default component.
        exeBuilder.GenerateMain(exe);

        // Build the executable.
        exeBuilder.Build(exe);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Builds a given application, producing an application bundle file for the appropriate target
 * device type (.ar7, .localhost, etc.).
 */
//--------------------------------------------------------------------------------------------------
void ApplicationBuilder_t::Build
(
    legato::App& app,
    std::string outputDirPath   ///< Directory into which the generated app bundle should be put.
)
//--------------------------------------------------------------------------------------------------
{
    // Construct the working directory structure, which consists of an "work" directory and
    // a "staging" directory.  Inside the "staging" directory, there is "lib", "bin", and any
    // other directories required to hold files bundled by the application or one of its components.
    // The "work" directory is for intermediate build output, like generated .c files and .o files.
    // The "staging" directory will get tar-compressed to become the actual application file.

    if (m_Params.IsVerbose())
    {
        std::cout << "Creating working directories under '" << m_Params.ObjOutputDir() << "'."
                  << std::endl;
    }

    legato::BuildParams_t buildParams(m_Params);

    const std::string& stagingDirPath = m_Params.StagingDir();
    buildParams.LibOutputDir(stagingDirPath + "/lib");
    buildParams.ExeOutputDir(stagingDirPath + "/bin");
    buildParams.ObjOutputDir(m_Params.ObjOutputDir() + "/work");

    // Clean the staging area.
    legato::CleanDir(stagingDirPath);

    // Create directories.
    legato::MakeDir(buildParams.ObjOutputDir());
    legato::MakeDir(buildParams.LibOutputDir());
    legato::MakeDir(buildParams.ExeOutputDir());

    // Build all the components and copy their files into the staging area.
    BuildComponents(app, buildParams);

    // Build all the executables and their IPC libs.
    BuildExecutables(app, buildParams);

    // Copy in any bundled files and directories from the "bundles:" section of the .adef.
    // Note: do the directories first, in case the files list adds files to those directories.
    for (auto& fileMapping : app.BundledDirs())
    {
        mk::CopyToStaging(  fileMapping.m_SourcePath,
                            stagingDirPath,
                            fileMapping.m_DestPath,
                            m_Params.IsVerbose()    );
    }
    for (auto& fileMapping : app.BundledFiles())
    {
        mk::CopyToStaging(  fileMapping.m_SourcePath,
                            stagingDirPath,
                            fileMapping.m_DestPath,
                            m_Params.IsVerbose()    );
    }

    // Generate the configuration.
    GenerateSupervisorConfig(stagingDirPath, app, m_Params);

    // TODO: Generate the application's configuration tree (containing all its pool sizes,
    //       and anything else listed under the "config:" section of the .adef.)

    // TODO: Copy in the metadata (.adef and Component.cdef) files so they can be retrieved
    //       by Developer Studio.

    // Zip it all up.
    std::string outputPath = legato::CombinePath(   outputDirPath,
                                                    app.Name() + "." + buildParams.Target() );
    if (!legato::IsAbsolutePath(outputPath))
    {
        outputPath = legato::GetWorkingDir() + "/" + outputPath;
    }
    std::string tarCommandLine = "tar cjf \"" + outputPath + "\" -C \"" + stagingDirPath + "\" .";
    if (m_Params.IsVerbose())
    {
        std::cout << "Packaging application into '" << outputPath << "'." << std::endl;
        std::cout << std::endl << "$ " << tarCommandLine << std::endl << std::endl;
    }

    mk::ExecuteCommandLine(tarCommandLine);
}


