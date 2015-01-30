//--------------------------------------------------------------------------------------------------
/**
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#include "LegatoObjectModel.h"
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
std::string App::AppNameFromDefFilePath
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

    if (appName.empty())
    {
        throw Exception("Application name missing from file name '" + path + "'.");
    }

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
    m_StartMode(AUTO),
    m_MaxThreads(20),
    m_MaxMQueueBytes(512),
    m_MaxQueuedSignals(100),
    m_MaxMemoryBytes(40000 * 1024), // 40 MB
    m_CpuShare(1024),
    m_MaxFileSystemBytes(128 * 1024)    // 128 KB
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
    m_StartMode(std::move(application.m_StartMode)),
    m_Components(std::move(application.m_Components)),
    m_Executables(std::move(application.m_Executables)),
    m_ProcEnvironments(std::move(application.m_ProcEnvironments)),
    m_BundledFiles(std::move(application.m_BundledFiles)),
    m_BundledDirs(std::move(application.m_BundledDirs)),
    m_RequiredFiles(std::move(application.m_RequiredFiles)),
    m_RequiredDirs(std::move(application.m_RequiredDirs)),
    m_RequiredInterfaces(std::move(application.m_RequiredInterfaces)),
    m_ProvidedInterfaces(std::move(application.m_ProvidedInterfaces)),
    m_InternalApiBinds(std::move(application.m_InternalApiBinds)),
    m_ExternalApiBinds(std::move(application.m_ExternalApiBinds)),
    m_Groups(std::move(application.m_Groups)),
    m_MaxThreads(std::move(application.m_MaxThreads)),
    m_MaxMQueueBytes(std::move(application.m_MaxMQueueBytes)),
    m_MaxQueuedSignals(std::move(application.m_MaxQueuedSignals)),
    m_MaxMemoryBytes(std::move(application.m_MaxMemoryBytes)),
    m_CpuShare(std::move(application.m_CpuShare)),
    m_MaxFileSystemBytes(std::move(application.m_MaxFileSystemBytes)),
    m_WatchdogTimeout(std::move(application.m_WatchdogTimeout)),
    m_WatchdogAction(std::move(application.m_WatchdogAction)),
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
        m_StartMode = std::move(application.m_StartMode);
        m_Components = std::move(application.m_Components);
        m_Executables = std::move(application.m_Executables);
        m_ProcEnvironments = std::move(application.m_ProcEnvironments);
        m_BundledFiles = std::move(application.m_BundledFiles);
        m_BundledDirs = std::move(application.m_BundledDirs);
        m_RequiredFiles = std::move(application.m_RequiredFiles);
        m_RequiredDirs = std::move(application.m_RequiredDirs);
        m_RequiredInterfaces = std::move(application.m_RequiredInterfaces);
        m_ProvidedInterfaces = std::move(application.m_ProvidedInterfaces);
        m_InternalApiBinds = std::move(application.m_InternalApiBinds);
        m_ExternalApiBinds = std::move(application.m_ExternalApiBinds);
        m_Groups = std::move(application.m_Groups);
        m_MaxThreads = std::move(application.m_MaxThreads);
        m_MaxMQueueBytes = std::move(application.m_MaxMQueueBytes);
        m_MaxQueuedSignals = std::move(application.m_MaxQueuedSignals);
        m_MaxMemoryBytes = std::move(application.m_MaxMemoryBytes);
        m_CpuShare = std::move(application.m_CpuShare);
        m_MaxFileSystemBytes = std::move(application.m_MaxFileSystemBytes);
        m_WatchdogTimeout = std::move(application.m_WatchdogTimeout);
        m_WatchdogAction = std::move(application.m_WatchdogAction);
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

    // Currently bundled directories cannot be written to because we do not support disk quotas yet.
    if (mapping.m_PermissionFlags & legato::PERMISSION_WRITEABLE)
    {
        throw legato::Exception("Bundled directories cannot have write permission.");
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
ExeToUserApiBind& App::AddExternalApiBind
(
    const std::string& clientInterfaceSpec  ///< Client interface specifier (exe.comp.interface)
)
//--------------------------------------------------------------------------------------------------
{
    if (   (m_InternalApiBinds.find(clientInterfaceSpec) != m_InternalApiBinds.end())
        || (m_ExternalApiBinds.find(clientInterfaceSpec) != m_ExternalApiBinds.end()) )
    {
        throw Exception("Multiple bindings of the same API client interface: '"
                        + clientInterfaceSpec + "'.");
    }

    ExeToUserApiBind& binding = m_ExternalApiBinds[clientInterfaceSpec];

    binding.ClientInterface(clientInterfaceSpec);

    return binding;
}


//--------------------------------------------------------------------------------------------------
/**
 * Override the .adef level binding of a client-side interface with another binding.
 **/
//--------------------------------------------------------------------------------------------------
void App::OverrideExternalApiBind
(
    ClientInterface& interface,     /// The interface to be bound.
    const UserToUserApiBind& bind   /// The binding to be used to override whatever exists already.
)
//--------------------------------------------------------------------------------------------------
{
    // If there is already a binding of this client interface, find it and update it.
    auto i = m_ExternalApiBinds.find(interface.AppUniqueName());
    if (i != m_ExternalApiBinds.end())
    {
        ExeToUserApiBind& oldBind = i->second;

        // Print informational message about the .adef binding being overridden by the .sdef.
        // and update the app's binding.
        std::cout << "Overriding binding of " << m_Name << "."
                  << interface.ExternalName() << " (" << interface.AppUniqueName() << ") to ";

        if (bind.IsServerAnApp())
        {
            std::cout << bind.ServerAppName();
        }
        else
        {
            std::cout << "<" << bind.ServerUserName() << ">";
        }
        std::cout << "." << bind.ServerInterfaceName() << "." << std::endl;

        oldBind.ServerUserName(bind.ServerUserName());
        oldBind.ServerAppName(bind.ServerAppName());
        oldBind.ServerInterfaceName(bind.ServerInterfaceName());
    }
    // If there isn't a binding yet, create one now.
    else
    {
        auto& newBind = AddExternalApiBind(interface.AppUniqueName());

        newBind.ServerUserName(bind.ServerUserName());
        newBind.ServerAppName(bind.ServerAppName());
        newBind.ServerInterfaceName(bind.ServerInterfaceName());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Make a client-side interface into an external interface for the application.
 **/
//--------------------------------------------------------------------------------------------------
void App::MakeInterfaceExternal
(
    ClientInterface& interface, ///< The interface to make external.
    const std::string& alias    ///< The name to give it outside the application.
)
//--------------------------------------------------------------------------------------------------
{
    if (   (m_RequiredInterfaces.find(alias) != m_RequiredInterfaces.end())
        || (m_ProvidedInterfaces.find(alias) != m_ProvidedInterfaces.end()) )
    {
        throw legato::Exception("Duplicate external interface name: '" + alias + "'.");
    }

    if (interface.IsExternalToApp())
    {
        throw legato::Exception("Interface '" + interface.AppUniqueName()
                                + "' is already an external interface.");
    }

    interface.MakeExternalToApp(alias);

    m_RequiredInterfaces[alias] = &interface;
}


//--------------------------------------------------------------------------------------------------
/**
 * Make a server-side interface into an external interface for the application.
 **/
//--------------------------------------------------------------------------------------------------
void App::MakeInterfaceExternal
(
    ServerInterface& interface, ///< The interface to make external.
    const std::string& alias    ///< The name to give it outside the application.
)
//--------------------------------------------------------------------------------------------------
{
    if (   (m_RequiredInterfaces.find(alias) != m_RequiredInterfaces.end())
        || (m_ProvidedInterfaces.find(alias) != m_ProvidedInterfaces.end()) )
    {
        throw legato::Exception("Duplicate external interface name: '" + alias + "'.");
    }

    if (interface.IsExternalToApp())
    {
        throw legato::Exception("Interface '" + interface.AppUniqueName()
                                + "' is already an external interface.");
    }

    interface.MakeExternalToApp(alias);

    m_ProvidedInterfaces[alias] = &interface;
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a binding from a client-side IPC API interface to a server offered by an app or user
 * outside this app.
 *
 * @return Reference to the newly created binding.
 **/
//--------------------------------------------------------------------------------------------------
ExeToUserApiBind& App::GetExternalApiBind
(
    const std::string& clientInterfaceSpec  ///< Client interface specifier (exe.comp.interface)
)
//--------------------------------------------------------------------------------------------------
{
    auto i = m_ExternalApiBinds.find(clientInterfaceSpec);

    if (i == m_ExternalApiBinds.end())
    {
        throw Exception("Binding of client-side interface'" + clientInterfaceSpec + "' not found.");
    }

    return i->second;
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

    ExeToExeApiBind& binding = m_InternalApiBinds[clientInterface];

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
ClientInterface& App::FindClientInterface
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
ServerInterface& App::FindServerInterface
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
ClientInterface& App::FindClientInterface
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
ServerInterface& App::FindServerInterface
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
 * Searches for an external client-side (required) interface on the application.
 *
 * @return Reference to the interface.
 *
 * @throw legato::Exception if the interface name is not one of the app's external interface names.
 **/
//--------------------------------------------------------------------------------------------------
ClientInterface& App::FindExternalClientInterface
(
    const std::string& interfaceName
)
//--------------------------------------------------------------------------------------------------
{
    auto i = m_RequiredInterfaces.find(interfaceName);

    if (i == m_RequiredInterfaces.end())
    {
        throw legato::Exception ("External client-side (required) IPC API interface '"
                                 + interfaceName + "' not found in app '" + m_Name + "'.");
    }

    return *(i->second);
}


//--------------------------------------------------------------------------------------------------
/**
 * Searches for an external server-side (provided) interface on the application.
 *
 * @return Reference to the interface.
 *
 * @throw legato::Exception if the interface name is not one of the app's external interface names.
 **/
//--------------------------------------------------------------------------------------------------
ServerInterface& App::FindExternalServerInterface
(
    const std::string& interfaceName
)
//--------------------------------------------------------------------------------------------------
{
    auto i = m_ProvidedInterfaces.find(interfaceName);

    if (i == m_ProvidedInterfaces.end())
    {
        throw legato::Exception ("External server-side (provided) IPC API interface '"
                                 + interfaceName + "' not found in app '" + m_Name + "'.");
    }

    return *(i->second);
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
 * Remove the app's user from all secondary groups.  (The user can't be removed from its primary
 * group, which is the group with the same name as the user.)
 **/
//--------------------------------------------------------------------------------------------------
void App::ClearGroups
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    m_Groups.clear();
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


//--------------------------------------------------------------------------------------------------
/**
 * @return  true if one or more of the application's processes are permitted to run threads at
 *          a real-time priority level.
 **/
//--------------------------------------------------------------------------------------------------
bool App::AreRealTimeThreadsPermitted
(
    void
)
const
//--------------------------------------------------------------------------------------------------
{
    // Loop through all the Process Environments and see if any of them is permitted to use
    // real-time priorities.
    for (const auto& env : m_ProcEnvironments)
    {
        if (env.AreRealTimeThreadsPermitted())
        {
            return true;
        }
    }

    return false;
}


} // namespace legato
