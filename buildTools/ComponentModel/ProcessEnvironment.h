//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Process Environment class, which is used to hold details, such as environment
 * variable settings and limits to be imposed on one or more processes at runtime.  Essentially,
 * it holds everything in a single "processes:" section, except for the contents of any "run:"
 * subsections.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef PROCESS_ENVIRONMENT_H_INCLUDE_GUARD
#define PROCESS_ENVIRONMENT_H_INCLUDE_GUARD

#include "Process.h"
#include "WatchdogConfig.h"


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
        std::string m_Priority;

        /// Action to take when a process dies with a failure exit code.
        std::string m_FaultAction;

        // RLimits:
        size_t m_CoreFileSize;
        size_t m_MaxFileSize;
        size_t m_MemLockSize;
        size_t m_NumFds;

        /// Watchdog
        WatchdogTimeoutConfig m_WatchdogTimeout;
        WatchdogActionConfig m_WatchdogAction;

    public:
        Process& CreateProcess() { m_ProcessList.emplace_back(); return m_ProcessList.back(); }
        std::list<Process>& ProcessList() { return m_ProcessList; }
        const std::list<Process>& ProcessList() const { return m_ProcessList; }

        void AddEnvVar(const std::string& name, std::string&& value) { m_EnvVarList[name] = value; }
        const std::map<std::string, std::string>& EnvVarList() const { return m_EnvVarList; }

        void FaultAction(std::string&& action) { m_FaultAction = action; }
        const std::string& FaultAction() const { return m_FaultAction; }

        void Priority(std::string&& priority) { m_Priority = priority; }
        const std::string& Priority() const { return m_Priority; }

        void CoreFileSize(size_t limit) { m_CoreFileSize = limit; }
        size_t CoreFileSize() const { return m_CoreFileSize; }

        void MaxFileSize(size_t limit)       { m_MaxFileSize = limit; }
        size_t MaxFileSize() const { return m_MaxFileSize; }

        void MemLockSize(size_t limit) { m_MemLockSize = limit; }
        size_t MemLockSize() const { return m_MemLockSize; }

        void NumFds(size_t limit) { m_NumFds = limit; }
        size_t NumFds() const { return m_NumFds; }

        WatchdogTimeoutConfig& WatchdogTimeout() { return m_WatchdogTimeout; }
        const WatchdogTimeoutConfig& WatchdogTimeout() const { return m_WatchdogTimeout; }

        WatchdogActionConfig& WatchdogAction() { return m_WatchdogAction; }
        const WatchdogActionConfig& WatchdogAction() const { return m_WatchdogAction; }
};


}

#endif // PROCESS_ENVIRONMENT_H_INCLUDE_GUARD
