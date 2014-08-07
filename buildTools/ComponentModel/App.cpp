//--------------------------------------------------------------------------------------------------
/**
 *  Copyright (C) Sierra Wireless, Inc. 2013-2014.  Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
#include "WatchdogConfig.h"
#include "limit.h"

namespace legato
{



//--------------------------------------------------------------------------------------------------
/**
 * Extract the name of an application from the path of its .adef file.
 *
 * @return the app name.
 */
//--------------------------------------------------------------------------------------------------
static std::string AppNameFromDefFilePath
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    auto endPos = path.rfind(".");
    auto startPos = path.rfind("/");

    if ((endPos == std::string::npos) || (path.substr(endPos) != ".adef"))
    {
        throw Exception("'" + path + "' does not appear to be an application file path.");
    }

    if (startPos == std::string::npos)
    {
        startPos = 0;
    }
    else
    {
        startPos++;
    }

    std::string appName = path.substr(startPos, endPos - startPos);

    if (appName.length() > LIMIT_MAX_APP_NAME_LEN)
    {
        throw Exception("Application name " + appName +
                        " is too long.  Application names must be a maximum of " +
                        std::to_string(LIMIT_MAX_APP_NAME_LEN) + " characters.");
    }

    return appName;
}


//--------------------------------------------------------------------------------------------------
/**
 *  Default constructor, create a new blank app object.
 */
//--------------------------------------------------------------------------------------------------
App::App
(
)
//--------------------------------------------------------------------------------------------------
:   m_Name("untitled"),
    m_Version(""),
    m_IsSandboxed(true),
    m_Debug(false),
    m_StartMode(AUTO),
    m_NumProcs(SIZE_MAX),
    m_MqueueSize(SIZE_MAX),
    m_RtSignalQueueSize(SIZE_MAX),
    m_MemLimit(SIZE_MAX),
    m_CpuShare(SIZE_MAX),
    m_FileSystemSize(SIZE_MAX),
    m_UsesWatchdog(false),
    m_UsesConfig(false)
//--------------------------------------------------------------------------------------------------
{
}



//--------------------------------------------------------------------------------------------------
/**
 *  Move constructor.
 */
//--------------------------------------------------------------------------------------------------
App::App
(
    App&& application  /// The object to move.
)
//--------------------------------------------------------------------------------------------------
:   m_Name(std::move(application.m_Name)),
    m_Version(std::move(application.m_Version)),
    m_DefFilePath(std::move(application.m_DefFilePath)),
    m_IsSandboxed(std::move(application.m_IsSandboxed)),
    m_Debug(std::move(application.m_Debug)),
    m_StartMode(std::move(application.m_StartMode)),
    m_Components(std::move(application.m_Components)),
    m_Executables(std::move(application.m_Executables)),
    m_ProcEnvironments(std::move(application.m_ProcEnvironments)),
    m_BundledFiles(std::move(application.m_BundledFiles)),
    m_BundledDirs(std::move(application.m_BundledDirs)),
    m_RequiredFiles(std::move(application.m_RequiredFiles)),
    m_RequiredDirs(std::move(application.m_RequiredDirs)),
    m_InternalApiBinds(std::move(application.m_InternalApiBinds)),
    m_ExternalApiBinds(std::move(application.m_ExternalApiBinds)),
    m_Groups(std::move(application.m_Groups)),
    m_NumProcs(std::move(application.m_NumProcs)),
    m_MqueueSize(std::move(application.m_MqueueSize)),
    m_RtSignalQueueSize(std::move(application.m_RtSignalQueueSize)),
    m_MemLimit(std::move(application.m_MemLimit)),
    m_CpuShare(std::move(application.m_CpuShare)),
    m_FileSystemSize(std::move(application.m_FileSystemSize)),
    m_WatchdogTimeout(std::move(application.m_WatchdogTimeout)),
    m_WatchdogAction(std::move(application.m_WatchdogAction)),
    m_UsesWatchdog(false),
    m_UsesConfig(false),
    m_ConfigTrees(application.m_ConfigTrees)
{
}




//--------------------------------------------------------------------------------------------------
/**
 * Move assignment operator.
 */
//--------------------------------------------------------------------------------------------------
App& App::operator =
(
    App&& application  ///
)
{
    if (&application != this)
    {
        m_Name = std::move(application.m_Name);
        m_Version = std::move(application.m_Version);
        m_DefFilePath = std::move(application.m_DefFilePath);
        m_IsSandboxed = std::move(application.m_IsSandboxed);
        m_Debug = std::move(application.m_Debug);
        m_StartMode = std::move(application.m_StartMode);
        m_Components = std::move(application.m_Components);
        m_Executables = std::move(application.m_Executables);
        m_ProcEnvironments = std::move(application.m_ProcEnvironments);
        m_BundledFiles = std::move(application.m_BundledFiles);
        m_BundledDirs = std::move(application.m_BundledDirs);
        m_RequiredFiles = std::move(application.m_RequiredFiles);
        m_RequiredDirs = std::move(application.m_RequiredDirs);
        m_InternalApiBinds = std::move(application.m_InternalApiBinds);
        m_ExternalApiBinds = std::move(application.m_ExternalApiBinds);
        m_Groups = std::move(application.m_Groups);
        m_NumProcs = std::move(application.m_NumProcs);
        m_MqueueSize = std::move(application.m_MqueueSize);
        m_RtSignalQueueSize = std::move(application.m_RtSignalQueueSize);
        m_MemLimit = std::move(application.m_MemLimit);
        m_CpuShare = std::move(application.m_CpuShare);
        m_FileSystemSize = std::move(application.m_FileSystemSize);
        m_WatchdogTimeout = std::move(application.m_WatchdogTimeout);
        m_WatchdogAction = std::move(application.m_WatchdogAction);
        m_UsesWatchdog = std::move(application.m_UsesWatchdog);
        m_UsesConfig = std::move(application.m_UsesConfig);
        m_ConfigTrees = std::move(application.m_ConfigTrees);
    }

    return *this;
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the file system path of the application definition file.
 *
 * @note    The application name is automatically extracted from the file path.
 */
//--------------------------------------------------------------------------------------------------
void App::DefFilePath
(
    const std::string& path
)
//--------------------------------------------------------------------------------------------------
{
    m_DefFilePath = path;

    m_Name = AppNameFromDefFilePath(m_DefFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the file system path of the application definition file.
 *
 * @note    The application name is automatically extracted from the file path.
 */
//--------------------------------------------------------------------------------------------------
void App::DefFilePath
(
    std::string&& path
)
//--------------------------------------------------------------------------------------------------
{
    m_DefFilePath = std::move(path);

    m_Name = AppNameFromDefFilePath(m_DefFilePath);
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a new executable in the application.
 *
 * @return A reference to the Executable object.
 */
//--------------------------------------------------------------------------------------------------
Executable& App::CreateExecutable
(
    const std::string& path ///< Name of the executable (path relative to exe output dir).
)
//--------------------------------------------------------------------------------------------------
{
    if (path[0] == '\0')
    {
        throw Exception("Executable has no name.");
    }

    auto foundItem = m_Executables.find(path);

    if (foundItem != m_Executables.end())
    {
        throw Exception("Attempting to add multiple executables with the same name: '"
                                  + path + "'");
    }

    // Create a new Executable object in the App's list of executables.
    Executable& exe = m_Executables[path];
    exe.OutputPath(path);   // NOTE: This has the side effect of setting the exe's CName.

    return exe;
}



//--------------------------------------------------------------------------------------------------
/**
 * Adds a file from the build host's file system to an application (bundles it into the app),
 * making it appear at a specific location in the application sandbox file system.
 */
//--------------------------------------------------------------------------------------------------
void App::AddBundledFile
(
    FileMapping&& mapping
)
//--------------------------------------------------------------------------------------------------
{
    // If the build host path (source path) is not absolute, then we need to prefix it with
    // the application's directory path.
    if (!legato::IsAbsolutePath(mapping.m_SourcePath))
    {
        mapping.m_SourcePath = legato::CombinePath(Path(), mapping.m_SourcePath);
    }

    // Find the file in the host file system.
    if (!legato::FileExists(mapping.m_SourcePath))
    {
        if (legato::DirectoryExists(mapping.m_SourcePath))
        {
            throw legato::Exception("'" + mapping.m_SourcePath + "' is a directory, not a file.");
        }

        throw legato::Exception("File '" + mapping.m_SourcePath + "' not found.");
    }

    m_BundledFiles.insert(std::move(mapping));
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds all the files and subdirectories from a directory in the build host's file system to
 * an application (bundles it into the app), making it appear at a specific location in the
 * application sandbox file system.
 */
//--------------------------------------------------------------------------------------------------
void App::AddBundledDir
(
    FileMapping&& mapping
)
//--------------------------------------------------------------------------------------------------
{
    // If the build host path (source path) is not absolute, then we need to prefix it with
    // the application's directory path.
    if (!legato::IsAbsolutePath(mapping.m_SourcePath))
    {
        mapping.m_SourcePath = legato::CombinePath(Path(), mapping.m_SourcePath);
    }

    // Find the directory in the host file system.
    if (!legato::DirectoryExists(mapping.m_SourcePath))
    {
        if (legato::FileExists(mapping.m_SourcePath))
        {
            throw legato::Exception("'" + mapping.m_SourcePath + "' is a file, not a directory.");
        }

        throw legato::Exception("Directory '" + mapping.m_SourcePath + "' not found.");
    }

    m_BundledDirs.insert(std::move(mapping));
}


//--------------------------------------------------------------------------------------------------
/**
 * Imports a file from somewhere in the root target file system (outside the sandbox) to
 * somewhere inside the application sandbox filesystem.
 */
//--------------------------------------------------------------------------------------------------
void App::AddRequiredFile
(
    FileMapping&& mapping
)
//--------------------------------------------------------------------------------------------------
{
    m_RequiredFiles.insert(std::move(mapping));
}


//--------------------------------------------------------------------------------------------------
/**
 * Imports a directory from somewhere in the root target file system (outside the sandbox) to
 * somewhere inside the application sandbox filesystem.
 */
//--------------------------------------------------------------------------------------------------
void App::AddRequiredDir
(
    FileMapping&& mapping
)
//--------------------------------------------------------------------------------------------------
{
    if (!IsValidPath(mapping.m_DestPath))
    {
        throw Exception("'" + mapping.m_DestPath + "' is not a valid path.");
    }
    if (!IsAbsolutePath(mapping.m_DestPath))
    {
        throw Exception(std::string("External file system objects must be mapped to an absolute")
                        + " path inside the application sandbox ('"
                        + mapping.m_DestPath + "' is not).");
    }
    if (!IsValidPath(mapping.m_SourcePath))
    {
        throw Exception("'" + mapping.m_SourcePath + "' is not a valid path.");
    }

    // If there's a trailing slash, remove it.
    if (mapping.m_SourcePath.back() == '/')
    {
        mapping.m_SourcePath.erase(--mapping.m_SourcePath.end());
    }

    m_RequiredDirs.insert(std::move(mapping));
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a binding from a client-side IPC API interface to a server offered by an app or user
 * outside this app.
 *
 * @return Reference to the newly created binding.
 **/
//--------------------------------------------------------------------------------------------------
ExternalApiBind& App::AddExternalApiBind
(
    const std::string& clientInterface  ///< Client interface specifier (exe.comp.interface)
)
//--------------------------------------------------------------------------------------------------
{
    if (   (m_InternalApiBinds.find(clientInterface) != m_InternalApiBinds.end())
        || (m_ExternalApiBinds.find(clientInterface) != m_ExternalApiBinds.end()) )
    {
        throw Exception("Multiple bindings of the same API client interface: '"
                        + clientInterface + "'.");
    }

    ExternalApiBind& binding = m_ExternalApiBinds[clientInterface];

    binding.ClientInterface(clientInterface);

    return binding;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a binding from a client-side interface to a server-side interface in the same app.
 **/
//--------------------------------------------------------------------------------------------------
void App::AddInternalApiBind
(
    const std::string& clientInterface, ///< Client interface specifier (exe.comp.interface)
    const std::string& serverInterface  ///< Server interface specifier (exe.comp.interface)
)
//--------------------------------------------------------------------------------------------------
{
    if (   (m_InternalApiBinds.find(clientInterface) != m_InternalApiBinds.end())
        || (m_ExternalApiBinds.find(clientInterface) != m_ExternalApiBinds.end()) )
    {
        throw Exception("Multiple bindings of the same API client interface: '"
                        + clientInterface + "'.");
    }

    InternalApiBind& binding = m_InternalApiBinds[clientInterface];

    binding.ClientInterface(clientInterface);
    binding.ServerInterface(serverInterface);
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches for an instance of a client-side interface on any of the application's executables.
 *
 * @return Reference to the interface.
 *
 * @throw legato::Exception if the interface name is not found anywhere in the app.
 **/
//--------------------------------------------------------------------------------------------------
ImportedInterface& App::FindClientInterface
(
    const std::string& exeName,
    const std::string& componentName,
    const std::string& interfaceName
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        auto exeMapIter = m_Executables.find(exeName);
        if (exeMapIter == m_Executables.end())
        {
            throw legato::Exception("No such executable '" + exeName + "'.");
        }

        auto& componentInstance = exeMapIter->second.FindComponentInstance(componentName);

        return componentInstance.FindClientInterface(interfaceName);
    }
    catch (legato::Exception e)
    {
        std::string name = exeName + '.' + componentName + '.' + interfaceName;

        throw legato::Exception ("Client-side IPC API interface '" + name + "'"
                                " not found in app '" + m_Name + "'.  " + e.what() );
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches for an instance of a server-side interface on any of the application's executables.
 *
 * @return Reference to the interface.
 *
 * @throw legato::Exception if the interface name is not found anywhere in the app.
 **/
//--------------------------------------------------------------------------------------------------
ExportedInterface& App::FindServerInterface
(
    const std::string& exeName,
    const std::string& componentName,
    const std::string& interfaceName
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        auto exeMapIter = m_Executables.find(exeName);
        if (exeMapIter == m_Executables.end())
        {
            throw legato::Exception("No such executable '" + exeName + "'.");
        }

        auto& componentInstance = exeMapIter->second.FindComponentInstance(componentName);

        return componentInstance.FindServerInterface(interfaceName);
    }
    catch (legato::Exception e)
    {
        std::string name = exeName + '.' + componentName + '.' + interfaceName;

        throw legato::Exception ("Server-side IPC API interface '" + name + "'"
                                " not found in app '" + m_Name + "'.  " + e.what() );
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches for an instance of a client-side interface on any of the application's executables.
 *
 * @return Reference to the interface.
 *
 * @throw legato::Exception if the interface name is not found anywhere in the app.
 **/
//--------------------------------------------------------------------------------------------------
ImportedInterface& App::FindClientInterface
(
    const std::string& name ///< Interface specification of the form "exe.component.interface"
)
//--------------------------------------------------------------------------------------------------
{
    std::string exeName;
    std::string componentName;
    std::string interfaceName;

    try
    {
        legato::Interface::SplitAppUniqueName(name, exeName, componentName, interfaceName);
    }
    catch (legato::Exception e)
    {
        throw legato::Exception ("Client-side IPC API interface '" + name + "'"
                                " not found in app '" + m_Name + "'.  " + e.what() );
    }

    return FindClientInterface(exeName, componentName, interfaceName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches for an instance of a server-side interface on any of the application's executables.
 *
 * @return Reference to the interface.
 *
 * @throw legato::Exception if the interface name is not found anywhere in the app.
 **/
//--------------------------------------------------------------------------------------------------
ExportedInterface& App::FindServerInterface
(
    const std::string& name ///< Interface specification of the form "exe.component.interface"
)
//--------------------------------------------------------------------------------------------------
{
    std::string exeName;
    std::string componentName;
    std::string interfaceName;

    try
    {
        legato::Interface::SplitAppUniqueName(name, exeName, componentName, interfaceName);
    }
    catch (legato::Exception e)
    {
        throw legato::Exception ("Server-side IPC API interface '" + name + "'"
                                " not found in app '" + m_Name + "'.  " + e.what() );
    }

    return FindServerInterface(exeName, componentName, interfaceName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add the app's user to a group (copy the string in).
 **/
//--------------------------------------------------------------------------------------------------
void App::AddGroup
(
    const std::string& groupName
)
//--------------------------------------------------------------------------------------------------
{
    m_Groups.insert(groupName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add the app's user to a group (move the string in).
 **/
//--------------------------------------------------------------------------------------------------
void App::AddGroup
(
    std::string&& groupName
)
//--------------------------------------------------------------------------------------------------
{
    m_Groups.insert(groupName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add permission to access a given configuration tree.
 **/
//--------------------------------------------------------------------------------------------------
void App::AddConfigTreeAccess
(
    const std::string& tree,
    int flags
)
//--------------------------------------------------------------------------------------------------
{
    // If they've already specified permission for this same tree, it's most likely a mistake.
    if (m_ConfigTrees.find(tree) != m_ConfigTrees.end())
    {
        throw legato::Exception("Duplicate access specification for configuration tree '"
                                + tree + "'");
    }

    m_ConfigTrees[tree] = flags;
}


}
