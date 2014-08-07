//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the App class.  This class holds all the information specific to an
 *  application.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013-2014.  Use of this work is subject to license.
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

        bool m_Debug;               ///< True if the application should be started in debug mode.

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

        /// Map of client-side interface names to internal IPC API binds.
        std::map<std::string, InternalApiBind> m_InternalApiBinds;

        /// Map of client-side interface names to external IPC API binds.
        std::map<std::string, ExternalApiBind> m_ExternalApiBinds;

        /// Set of the names of groups that this application's user should be a member of.
        std::set<std::string> m_Groups;

        // Limits:
        size_t m_NumProcs;          ///< number of processes
        size_t m_MqueueSize;        ///< bytes
        size_t m_RtSignalQueueSize; ///< number of signals
        size_t m_MemLimit;          ///< kilobytes
        size_t m_CpuShare;          ///< relative share value
        size_t m_FileSystemSize;    ///< bytes

        // Watchdog
        WatchdogTimeoutConfig m_WatchdogTimeout;
        WatchdogActionConfig m_WatchdogAction;
        bool m_UsesWatchdog;

        // Config
        bool m_UsesConfig;

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

        void SetDebug() { m_Debug = true; }
        bool Debug() const { return m_Debug; }

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

        ExternalApiBind& AddExternalApiBind(const std::string& clientInterface);
        const std::map<std::string, ExternalApiBind>& ExternalApiBinds() const
                                                                   { return m_ExternalApiBinds; }

        void AddInternalApiBind(const std::string& clientInterface,
                                const std::string& serverInterface);
        const std::map<std::string, InternalApiBind>& InternalApiBinds() const
                                                                   { return m_InternalApiBinds; }

        ImportedInterface& FindClientInterface( const std::string& exeName,
                                                const std::string& componentName,
                                                const std::string& interfaceName    );
        ExportedInterface& FindServerInterface( const std::string& exeName,
                                                const std::string& componentName,
                                                const std::string& interfaceName    );

        // These lookup interfaces based on interface specs, of the form "exe.component.interface".
        ImportedInterface& FindClientInterface(const std::string& name);
        ExportedInterface& FindServerInterface(const std::string& name);

        void AddGroup(const std::string& groupName);  ///< Add the app's user to a group.
        void AddGroup(std::string&& groupName);  ///< Add the app's user to a group.
        const std::set<std::string>& Groups() const { return m_Groups; }

        void NumProcs(size_t limit)       { m_NumProcs = limit; }
        size_t NumProcs() const { return m_NumProcs; }

        void MqueueSize(size_t limit)     { m_MqueueSize = limit; }
        size_t MqueueSize() const { return m_MqueueSize; }

        void RtSignalQueueSize(size_t limit)   { m_RtSignalQueueSize = limit; }
        size_t RtSignalQueueSize() const { return m_RtSignalQueueSize; }

        void MemLimit(size_t limit)   { m_MemLimit = limit; }
        size_t MemLimit() const { return m_MemLimit; }

        void CpuShare(size_t limit)   { m_CpuShare = limit; }
        size_t CpuShare() const { return m_CpuShare; }

        void FileSystemSize(size_t limit) { m_FileSystemSize = limit; }
        size_t FileSystemSize() const { return m_FileSystemSize; }

        WatchdogTimeoutConfig& WatchdogTimeout() { return m_WatchdogTimeout; }
        const WatchdogTimeoutConfig& WatchdogTimeout() const { return m_WatchdogTimeout; }
        WatchdogActionConfig& WatchdogAction() { return m_WatchdogAction; }
        const WatchdogActionConfig& WatchdogAction() const { return m_WatchdogAction; }

        void SetUsesWatchdog() { m_UsesWatchdog = true; } ///< Tell App it uses the watchdog API.
        bool UsesWatchdog() const { return m_UsesWatchdog; }

        void SetUsesConfig() { m_UsesConfig = true; } ///< Tell the App that it uses the Config API
        bool UsesConfig() const { return m_UsesConfig; }

        void AddConfigTreeAccess(const std::string& tree, int flags);
        const std::map<std::string, int>& ConfigTrees() const { return m_ConfigTrees; }
        std::map<std::string, int>& ConfigTrees() { return m_ConfigTrees; }
};




}

#endif  // APP_H_INCLUDE_GUARD
