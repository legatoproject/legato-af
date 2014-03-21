//--------------------------------------------------------------------------------------------------
/**
 *  Implementation of the App class.  This class holds all the information specific to an
 *  application.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
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

        std::string m_DefFilePath;      ///< Path to the .adef file.

        bool m_IsSandboxed;         ///< Run in a sandbox?

        bool m_Debug;               ///< True if the application should be started in debug mode.

        StartMode_t m_StartMode;    ///< Start automatically or only when asked?

        /// List of components used in the app, keyed by component path.
        std::map<std::string, Component> m_Components;

        /// List of executables created in the app, keyed by name.
        std::map<std::string, Executable> m_Executables;

        /// List of "process environments" that exist in this app.
        std::list<ProcessEnvironment> m_ProcEnvironments;

        /// List of files to be included in (bundled with) the app.  The source path is in the
        /// build host's file system, while the destination path is inside the application
        /// sandbox's runtime file system.  NOTE: This list doesn't include the
        /// included files required by any components included in this app.
        std::list<FileMapping> m_IncludedFiles;

        /// List of files to be imported into the application's sandbox from the target's root
        /// file system (outside of the sandbox).  The source path is outside the sandbox and
        /// the destination path is inside the sandbox.  NOTE: This list doesn't include the
        /// imported files required by any components included in this app.
        std::list<FileMapping> m_ImportedFiles;

        /// List of the names of groups that this application's user should be a member of.
        /// Duplicates should be filtered out.  Order doesn't matter.
        std::set<std::string> m_Groups;

        // RLimits:
        size_t m_NumProcs;
        size_t m_MqueueSize;
        size_t m_RtSignalQueueSize;
        size_t m_FileSystemSize;

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

        void DefFilePath(const std::string& path);
        void DefFilePath(std::string&& path);
        const std::string& DefFilePath() const { return m_DefFilePath; }

        void IsSandboxed(bool isSandboxed) { m_IsSandboxed = isSandboxed; }
        bool IsSandboxed() const { return m_IsSandboxed; }

        void SetDebug() { m_Debug = true; }
        bool Debug() const { return m_Debug; }

        void StartMode(StartMode_t mode) { m_StartMode = mode; }
        StartMode_t StartMode() const { return m_StartMode; }

        void NumProcs(size_t limit)       { m_NumProcs = limit; }
        size_t NumProcs() const { return m_NumProcs; }

        void MqueueSize(size_t limit)     { m_MqueueSize = limit; }
        size_t MqueueSize() const { return m_MqueueSize; }

        void RtSignalQueueSize(size_t limit)   { m_RtSignalQueueSize = limit; }
        size_t RtSignalQueueSize() const { return m_RtSignalQueueSize; }

        void FileSystemSize(size_t limit) { m_FileSystemSize = limit; }
        size_t FileSystemSize() const { return m_FileSystemSize; }

        Component& CreateComponent(const std::string& name);
        const std::map<std::string, Component>& ComponentMap() const { return m_Components; }
        std::map<std::string, Component>& ComponentMap() { return m_Components; }

        Executable& CreateExecutable(const std::string& path);
        std::map<std::string, Executable>& Executables() { return m_Executables; }

        ProcessEnvironment& CreateProcEnvironment() { m_ProcEnvironments.emplace_back(); return m_ProcEnvironments.back(); }
        std::list<ProcessEnvironment>& ProcEnvironments() { return m_ProcEnvironments; }
        const std::list<ProcessEnvironment>& ProcEnvironments() const { return m_ProcEnvironments; }

        void AddIncludedFile(FileMapping&& mapping);  ///< Include a file from the host file system.
        const std::list<FileMapping>& IncludedFiles() const { return m_IncludedFiles; }

        void AddImportedFile(FileMapping&& mapping);  ///< Import a file from outside the sandbox.
        const std::list<FileMapping>& ImportedFiles() const { return m_ImportedFiles; }

        void AddGroup(const std::string& groupName);  ///< Add the app's user to a group.
        void AddGroup(std::string&& groupName);  ///< Add the app's user to a group.
        const std::set<std::string>& Groups() const { return m_Groups; }

        /// Bind two interfaces (left and right) together.  Each interface specification is
        /// a string in the form "process.interface".
        void Bind(const std::string& importProcess, const std::string& importInterface,
                  const std::string& exportProcess, const std::string& exportInterface);
};




}

#endif  // APP_H_INCLUDE_GUARD
