//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Process Environment class, which is used to hold details, such as environment
 * variable settings and limits to be imposed on one or more processes at runtime.  Essentially,
 * it holds everything in a single "processes:" section, except for the contents of any "run:"
 * subsections.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef PROCESS_ENVIRONMENT_H_INCLUDE_GUARD
#define PROCESS_ENVIRONMENT_H_INCLUDE_GUARD

#include "Process.h"


namespace legato
{

class ProcessEnvironment
{
    public:
        ProcessEnvironment();
        ProcessEnvironment(ProcessEnvironment&& original);
        ProcessEnvironment(const ProcessEnvironment& original) = delete;
        virtual ~ProcessEnvironment() {}

    public:
        ProcessEnvironment& operator =(const ProcessEnvironment&) = delete;

    private:
        /// List of processes to run in this environment.
        /// NB: It is permitted to have multiple sharing the same name.
        std::list<Process> m_ProcessList;

        /// The environment variable list is a map with the variable name as the key.
        std::map<std::string, std::string> m_EnvVarList;

        /// Priority to start processes at.
        Priority_t m_StartPriority;

        /// Maximum priority that any of the threads are allowed to run at.
        Priority_t m_MaxPriority;

        /// Action to take when a process dies with a failure exit code.
        FaultAction_t m_FaultAction;

        // Per-process rlimits:
        NonNegativeIntLimit_t m_MaxFileBytes;         ///< Maximum file size in bytes.
        NonNegativeIntLimit_t m_MaxCoreDumpFileBytes; ///< Maximum core dump file size in bytes.
        NonNegativeIntLimit_t m_MaxLockedMemoryBytes; ///< Maximum bytes that can be locked in RAM.
        PositiveIntLimit_t    m_MaxFileDescriptors;   ///< Maximum number of open file descriptors.

        /// Watchdog
        WatchdogTimeout_t m_WatchdogTimeout;
        WatchdogAction_t  m_WatchdogAction;

    public:
        Process& CreateProcess() { m_ProcessList.emplace_back(); return m_ProcessList.back(); }
        std::list<Process>& ProcessList() { return m_ProcessList; }
        const std::list<Process>& ProcessList() const { return m_ProcessList; }

        void AddEnvVar(const std::string& name, std::string&& value) { m_EnvVarList[name] = value; }
        const std::map<std::string, std::string>& EnvVarList() const { return m_EnvVarList; }

        void FaultAction(std::string&& action) { m_FaultAction = action; }
        const FaultAction_t& FaultAction() const { return m_FaultAction; }

        void StartPriority(const std::string& priority);
        const Priority_t& StartPriority() const { return m_StartPriority; }

        void MaxPriority(const std::string& priority);
        const Priority_t& MaxPriority() const { return m_MaxPriority; }

        bool AreRealTimeThreadsPermitted() const;

        void MaxCoreDumpFileBytes(int limit) { m_MaxCoreDumpFileBytes = limit; }
        const NonNegativeIntLimit_t& MaxCoreDumpFileBytes() const { return m_MaxCoreDumpFileBytes; }

        void MaxFileBytes(int limit) { m_MaxFileBytes = limit; }
        const NonNegativeIntLimit_t& MaxFileBytes() const { return m_MaxFileBytes; }

        void MaxLockedMemoryBytes(int limit) { m_MaxLockedMemoryBytes = limit; }
        const NonNegativeIntLimit_t& MaxLockedMemoryBytes() const { return m_MaxLockedMemoryBytes; }

        void MaxFileDescriptors(int limit) { m_MaxFileDescriptors = limit; }
        const PositiveIntLimit_t& MaxFileDescriptors() const { return m_MaxFileDescriptors; }

        void WatchdogTimeout(int timeout) { m_WatchdogTimeout = timeout; }
        void WatchdogTimeout(const std::string& timeout) { m_WatchdogTimeout = timeout; }
        const WatchdogTimeout_t& WatchdogTimeout() const { return m_WatchdogTimeout; }

        void WatchdogAction(const std::string& action) { m_WatchdogAction = action; }
        const WatchdogAction_t& WatchdogAction() const { return m_WatchdogAction; }
};


}

#endif // PROCESS_ENVIRONMENT_H_INCLUDE_GUARD
