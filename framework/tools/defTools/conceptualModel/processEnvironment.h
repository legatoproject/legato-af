//--------------------------------------------------------------------------------------------------
/**
 * Definition of the Process Environment class, which is used to hold details, such as environment
 * variable settings and limits to be imposed on one or more processes at runtime.  Essentially,
 * it holds everything in a single "processes:" section, except for the contents of any "run:"
 * subsections.
 *
 * Copyright (C) Sierra Wireless Inc.
 **/
//--------------------------------------------------------------------------------------------------

#ifndef PROCESS_ENVIRONMENT_H_INCLUDE_GUARD
#define PROCESS_ENVIRONMENT_H_INCLUDE_GUARD


struct ProcessEnv_t
{
    ProcessEnv_t();

    /// List of processes to run in this environment.
    /// NB: It is NOT permitted to have multiple sharing the same name.
    std::list<Process_t*> processes;

    /// The environment variable list is a map with the variable name as the key.
    std::map<std::string, std::string> envVars;

    /// Action to take when a process dies with a failure exit code.
    FaultAction_t faultAction;

    // Per-process rlimits:
    NonNegativeIntLimit_t maxFileBytes;         ///< Maximum file size in bytes.
    NonNegativeIntLimit_t maxCoreDumpFileBytes; ///< Maximum core dump file size in bytes.
    NonNegativeIntLimit_t maxLockedMemoryBytes; ///< Maximum bytes that can be locked in RAM.
    PositiveIntLimit_t    maxFileDescriptors;   ///< Maximum number of open file descriptors.
    NonNegativeIntLimit_t maxStackBytes;        ///< Maximum number of bytes allowed for the stack

    // Watchdog
    WatchdogAction_t  watchdogAction;
    WatchdogTimeout_t watchdogTimeout;
    WatchdogTimeout_t maxWatchdogTimeout;

    void SetStartPriority(const std::string& priority);
    const Priority_t& GetStartPriority() const { return startPriority; }

    void SetMaxPriority(const std::string& priority);
    const Priority_t& GetMaxPriority() const { return maxPriority; }

    bool AreRealTimeThreadsPermitted() const;

private:

    /// Priority to start processes at.
    Priority_t startPriority;

    /// Maximum priority that any of the threads are allowed to run at.
    Priority_t maxPriority;
};


#endif // PROCESS_ENVIRONMENT_H_INCLUDE_GUARD
