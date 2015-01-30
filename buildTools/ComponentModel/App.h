//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the App class.  This class holds all the information specific to an
 *  application.
 *
 *  Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------

#ifndef APP_H_INCLUDE_GUARD
#define APP_H_INCLUDE_GUARD


namespace legato
{


//----------------------------------------------------------------------------------------------
/**
 * Represents a single Application.
 */
//----------------------------------------------------------------------------------------------
class App
{
    public:
        typedef enum {AUTO, MANUAL} StartMode_t;

    private:
        std::string m_Name;             ///< Name of the application.

        std::string m_Version;          ///< Version of the application.

        std::string m_DefFilePath;      ///< Path to the .adef file.

        bool m_IsSandboxed;         ///< Run in a sandbox?

        StartMode_t m_StartMode;    ///< Start automatically or only when asked?

        /// List of pointers to components used in the app, keyed by component path.
        std::map<std::string, Component*> m_Components;

        /// List of executables created in the app, keyed by name.
        std::map<std::string, Executable> m_Executables;

        /// List of "process environments" that exist in this app.
        std::list<ProcessEnvironment> m_ProcEnvironments;

        /// Set of files to be included in (bundled with) the app.  The source path is in the
        /// build host's file system, while the destination path is inside the application
        /// sandbox's runtime file system.  NOTE: This list doesn't include the
        /// included files required by any components included in this app.
        std::set<FileMapping> m_BundledFiles;

        /// Set of directories to be included in (bundled with) the app.  The source path is in the
        /// build host's file system, while the destination path is inside the application
        /// sandbox's runtime file system.  NOTE: This list doesn't include the
        /// included directories required by any components included in this app.
        std::set<FileMapping> m_BundledDirs;

        /// Set of files to be imported into the application's sandbox from the target's root
        /// file system (outside of the sandbox).  The source path is outside the sandbox and
        /// the destination path is inside the sandbox.  NOTE: This list doesn't include the
        /// external files required by any components included in this app.
        std::set<FileMapping> m_RequiredFiles;

        /// Set of directories to be imported into the application's sandbox from the target's root
        /// file system (outside of the sandbox).  The source path is outside the sandbox and
        /// the destination path is inside the sandbox.  NOTE: This list doesn't include the
        /// external directories required by any components included in this app.
        std::set<FileMapping> m_RequiredDirs;

        /// Map of client-side external interface names to pointers to interface instances.
        std::map<std::string, ClientInterface*> m_RequiredInterfaces;

        /// Map of server-side external interface names to pointers to interface instances.
        std::map<std::string, ServerInterface*> m_ProvidedInterfaces;

        /// Map of client-side interface names (exe.comp.interface) to internal IPC API binds.
        std::map<std::string, ExeToExeApiBind> m_InternalApiBinds;

        /// Map of client-side interface names (exe.comp.interface) to external IPC API binds.
        std::map<std::string, ExeToUserApiBind> m_ExternalApiBinds;

        /// Set of the names of groups that this application's user should be a member of.
        std::set<std::string> m_Groups;

        // Per-user limits:
        PositiveIntLimit_t      m_MaxThreads;        ///< Number of threads.
        NonNegativeIntLimit_t   m_MaxMQueueBytes;    ///< Total bytes in all POSIX MQueues.
        NonNegativeIntLimit_t   m_MaxQueuedSignals;  ///< Total number of queued signals.
        PositiveIntLimit_t      m_MaxMemoryBytes;    ///< Total bytes of RAM.
        PositiveIntLimit_t      m_CpuShare;          ///< Relative share value
        NonNegativeIntLimit_t   m_MaxFileSystemBytes;///< Total bytes in sandbox tmpfs file system.

        // Watchdog
        WatchdogTimeout_t   m_WatchdogTimeout;
        WatchdogAction_t    m_WatchdogAction;

        /// Map of configuration tree names to access permissions (see Permissions.h).
        std::map<std::string, int> m_ConfigTrees;

    public:
        App();
        App(App&& application);
        App(const App& application) = delete;
        virtual ~App() {};

    public:
        App& operator =(App&& application);
        App& operator =(const App& application) = delete;

    public:
        static std::string AppNameFromDefFilePath(const std::string& path);

    public:
        void Name(const std::string& name) { m_Name = name; }
        void Name(std::string&& name) { m_Name = std::move(name); }
        const std::string& Name() const { return m_Name; }

        void Version(const std::string& version) { m_Version = version; }
        void Version(std::string&& version) { m_Version = std::move(version); }
        const std::string& Version() const { return m_Version; }
        std::string& Version() { return m_Version; }

        void DefFilePath(const std::string& path);
        void DefFilePath(std::string&& path);
        const std::string& DefFilePath() const { return m_DefFilePath; }

        const std::string Path() const
                                    { return std::move(legato::GetContainingDir(m_DefFilePath)); }

        void IsSandboxed(bool isSandboxed) { m_IsSandboxed = isSandboxed; }
        bool IsSandboxed() const { return m_IsSandboxed; }

        void StartMode(StartMode_t mode) { m_StartMode = mode; }
        StartMode_t StartMode() const { return m_StartMode; }

        const std::map<std::string, Component*>& ComponentMap() const { return m_Components; }
        std::map<std::string, Component*>& ComponentMap() { return m_Components; }

        Executable& CreateExecutable(const std::string& path);
        const std::map<std::string, Executable>& Executables() const { return m_Executables; }
        std::map<std::string, Executable>& Executables() { return m_Executables; }

        ProcessEnvironment& CreateProcEnvironment() { m_ProcEnvironments.emplace_back();
                                                      return m_ProcEnvironments.back(); }
        std::list<ProcessEnvironment>& ProcEnvironments() { return m_ProcEnvironments; }
        const std::list<ProcessEnvironment>& ProcEnvironments() const { return m_ProcEnvironments; }

        void AddBundledFile(FileMapping&& mapping);  ///< Include a file from the host file system.
        const std::set<FileMapping>& BundledFiles() const { return m_BundledFiles; }

        void AddBundledDir(FileMapping&& mapping);  ///< Include a directory from host file system.
        const std::set<FileMapping>& BundledDirs() const { return m_BundledDirs; }

        void AddRequiredFile(FileMapping&& mapping);  ///< Import a file from outside the sandbox.
        const std::set<FileMapping>& RequiredFiles() const { return m_RequiredFiles; }

        void AddRequiredDir(FileMapping&& mapping);  ///< Import directory from outside the sandbox.
        const std::set<FileMapping>& RequiredDirs() const { return m_RequiredDirs; }

        void MakeInterfaceExternal(ClientInterface& interface, const std::string& alias);
        void MakeInterfaceExternal(ServerInterface& interface, const std::string& alias);

        ExeToUserApiBind& AddExternalApiBind(const std::string& clientInterface);
        void OverrideExternalApiBind(ClientInterface& interface, const UserToUserApiBind& bind);
        const std::map<std::string, ExeToUserApiBind>& ExternalApiBinds() const
                                                                   { return m_ExternalApiBinds; }
        ExeToUserApiBind& GetExternalApiBind(const std::string& clientInterfaceSpec);

        void AddInternalApiBind(const std::string& clientInterface,
                                const std::string& serverInterface);
        const std::map<std::string, ExeToExeApiBind>& InternalApiBinds() const
                                                                   { return m_InternalApiBinds; }

        ClientInterface& FindClientInterface( const std::string& exeName,
                                                const std::string& componentName,
                                                const std::string& interfaceName    );
        ServerInterface& FindServerInterface( const std::string& exeName,
                                                const std::string& componentName,
                                                const std::string& interfaceName    );

        // These lookup interfaces based on interface specs, of the form "exe.component.interface".
        ClientInterface& FindClientInterface(const std::string& name);
        ServerInterface& FindServerInterface(const std::string& name);

        // These lookup external interfaces based on the name used outside the app.
        ClientInterface& FindExternalClientInterface(const std::string& interfaceName);
        ServerInterface& FindExternalServerInterface(const std::string& interfaceName);

        void AddGroup(const std::string& groupName);  ///< Add the app's user to a group.
        void AddGroup(std::string&& groupName);  ///< Add the app's user to a group.
        void ClearGroups();  ///< Remove the app's user from all groups except its own user group.
        const std::set<std::string>& Groups() const { return m_Groups; }

        void MaxThreads(int limit)       { m_MaxThreads = limit; }
        const PositiveIntLimit_t& MaxThreads() const { return m_MaxThreads; }

        void MaxMQueueBytes(int limit)     { m_MaxMQueueBytes = limit; }
        const NonNegativeIntLimit_t& MaxMQueueBytes() const { return m_MaxMQueueBytes; }

        void MaxQueuedSignals(int limit)   { m_MaxQueuedSignals = limit; }
        const NonNegativeIntLimit_t& MaxQueuedSignals() const { return m_MaxQueuedSignals; }

        void MaxMemoryBytes(int limit)   { m_MaxMemoryBytes = limit; }
        const PositiveIntLimit_t& MaxMemoryBytes() const { return m_MaxMemoryBytes; }

        void CpuShare(int limit)   { m_CpuShare = limit; }
        const PositiveIntLimit_t& CpuShare() const { return m_CpuShare; }

        void MaxFileSystemBytes(int limit) { m_MaxFileSystemBytes = limit; }
        const NonNegativeIntLimit_t& MaxFileSystemBytes() const { return m_MaxFileSystemBytes; }

        void WatchdogTimeout(int timeout) { m_WatchdogTimeout = timeout; }
        void WatchdogTimeout(const std::string& timeout) { m_WatchdogTimeout = timeout; }
        const WatchdogTimeout_t& WatchdogTimeout() const { return m_WatchdogTimeout; }

        void WatchdogAction(const std::string& action) { m_WatchdogAction = action; }
        const WatchdogAction_t& WatchdogAction() const { return m_WatchdogAction; }

        void AddConfigTreeAccess(const std::string& tree, int flags);
        const std::map<std::string, int>& ConfigTrees() const { return m_ConfigTrees; }
        std::map<std::string, int>& ConfigTrees() { return m_ConfigTrees; }

        bool AreRealTimeThreadsPermitted() const;
};




}

#endif  // APP_H_INCLUDE_GUARD
