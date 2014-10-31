//--------------------------------------------------------------------------------------------------
/**
 * Main file for the System Parser.
 *
 * Copyright (C) 2014 Sierra Wireless Inc.  Use of this work is subject to license.
 **/
//--------------------------------------------------------------------------------------------------

#include "../ComponentModel/LegatoObjectModel.h"
#include "Parser.h"

extern "C"
{
    #include "SystemParser.tab.h"
    #include "ParserCommonInternals.h"
    #include "SystemParserInternals.h"
    #include <string.h>
    #include <stdio.h>
    #include "lex.syy.h"
}


//--------------------------------------------------------------------------------------------------
/**
 * Non-zero if verbose operation is requested.
 */
//--------------------------------------------------------------------------------------------------
int syy_IsVerbose = 0;


//--------------------------------------------------------------------------------------------------
/**
 * Build parameters received via the command line.
 */
//--------------------------------------------------------------------------------------------------
static const legato::BuildParams_t* BuildParamsPtr;


//--------------------------------------------------------------------------------------------------
/**
 * Pointer to the System object for the system that is currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
static legato::System* SystemPtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Pointer to the Application object for the app: section that is currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
static legato::App* AppPtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Get a reference to the current application object.
 *
 * @throw legato::Exception if not currently inside an "app:" section.
 */
//--------------------------------------------------------------------------------------------------
static legato::App& GetCurrentApp
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (AppPtr == NULL)
    {
        throw legato::Exception("Attempt to set an application parameter outside of an \"app:\""
                                " section.");
    }

    return *AppPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Do final processing of the system's object model.
 *
 * @note syy_error() will be called if something goes wrong.
 **/
//--------------------------------------------------------------------------------------------------
static void FinalizeSystem
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        // For each application,
        for (auto& appMapEntry : SystemPtr->Apps())
        {
            auto& app = appMapEntry.second;

            // Warn if cpuShare and real-time are used together.
            if (app.CpuShare().IsSet() && app.AreRealTimeThreadsPermitted())
            {
                yy_WarnAboutRealTimeAndCpuShare();
                std::cerr << "App '" << app.Name()
                          << "' has a cpuShare limit and is allowed real-time threads."
                          << std::endl;
            }

            // For each executable,
            for (auto& exeMapEntry : app.Executables())
            {
                auto& exe = exeMapEntry.second;

                // For each component instance in the executable,
                for (auto& componentInstance : exe.ComponentInstances())
                {
                    // For each required (client-side) interface in the component instance,
                    for (auto& interfaceMapEntry : componentInstance.RequiredApis())
                    {
                        auto& interface = interfaceMapEntry.second;

                        // If the interface is not satisfied (bound to something), it's an error.
                        // Note: Don't need to worry about APIs that we only use the typedefs from.
                        if (!interface.IsBound() && !interface.TypesOnly())
                        {
                            // If this is one of the app's external interfaces,
                            if (interface.IsExternalToApp())
                            {
                                throw legato::Exception(" Client-side (required) external"
                                                        " interface '" + interface.ExternalName()
                                                        + "' of application '" + app.Name() + "' is"
                                                        " unsatisfied.  It has not been bound to"
                                                        " any server (in the \"bindings:\" section"
                                                        " of either the .adef or .sdef).");
                            }
                            else
                            {
                                throw legato::Exception(" Client-side (required) interface '"
                                                        + interface.AppUniqueName() + "' of"
                                                        " application '" + app.Name() + "' is"
                                                        " unsatisfied.  It has not been bound to"
                                                        " any server (in the \"bindings:\" section"
                                                        " of either the .adef or .sdef).");
                            }
                        }
                    }
                }
            }
        }
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}


namespace legato
{

namespace parser
{


//--------------------------------------------------------------------------------------------------
/**
 * Parses a System Definition (.sdef) and populates a System object with the information
 * garnered.
 *
 * @note    Expects the System's definition (.sdef) file path to be set.
 */
//--------------------------------------------------------------------------------------------------
void ParseSystem
(
    System* systemPtr,                  ///< The object to be populated.
    const BuildParams_t& buildParams    ///< Build parameters obtained from the command line.
)
//--------------------------------------------------------------------------------------------------
{
    SystemPtr = systemPtr;
    BuildParamsPtr = &buildParams;

    const std::string& path = SystemPtr->DefFilePath();

    // Open the .sdef file for reading.
    FILE* file = fopen(path.c_str(), "r");
    if (file == NULL)
    {
        int error = errno;
        std::stringstream errorMessage;
        errorMessage << "Failed to open file '" << path << "'." <<
                        " Errno = " << error << "(" << strerror(error) << ").";
        throw Exception(errorMessage.str());
    }

    if (buildParams.IsVerbose())
    {
        std::cout << "Parsing '" << path << "'\n";
    }

    // Tell the parser to reset itself and connect to the new file stream for future parsing.
    syy_FileName = path.c_str();

    syy_IsVerbose = (BuildParamsPtr->IsVerbose() ? 1 : 0);
    syy_EndOfFile = 0;
    syy_ErrorCount = 0;
    syy_set_lineno(1);
    syy_restart(file);

    // Until the parsing is done,
    int parsingResult;
    do
    {
        // Start parsing.
        parsingResult = syy_parse();
    }
    while ((parsingResult != 0) && (!syy_EndOfFile));

    // Close the file
    fclose(file);

    // Do final processing.
    FinalizeSystem();

    // Halt if there were errors.
    if (syy_ErrorCount > 0)
    {
        throw Exception("Errors encountered while parsing '" + path + "'.");
    }

    SystemPtr = NULL;
    BuildParamsPtr = NULL;

    if (buildParams.IsVerbose())
    {
        std::cout << "Finished parsing '" << path << "'\n";
    }
}


}

}

//--------------------------------------------------------------------------------------------------
// NOTE: The following functions are called from C code inside the bison-generated parser code.
//--------------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------------
/**
 * Set the system version.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetVersion
(
    const char* version         ///< The version string.
)
//--------------------------------------------------------------------------------------------------
{
    SystemPtr->Version(version);
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds an application to the system, making it the "current application".
 */
//--------------------------------------------------------------------------------------------------
void syy_AddApp
(
    const char* adefPath       ///< File system path to the application's .adef file.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        std::string path(adefPath);

        // If the app path doesn't end in a ".adef", add it.
        if ((path.length() < 5) || (path.compare(path.length() - 5, 5, ".adef") != 0))
        {
            path += ".adef";
        }

        std::string resolvedPath = legato::FindFile(path, BuildParamsPtr->SourceDirs());

        if (resolvedPath.empty())
        {
            throw legato::Exception("Application definition file '" + path + "' not found.");
        }

        // Create a new App object in the System.
        AppPtr = &(SystemPtr->CreateApp(resolvedPath));

        if (syy_IsVerbose)
        {
            std::cout << "Adding application '" << AppPtr->Name() << "' to the system." << std::endl;
        }

        // Tell the parser to parse it.
        legato::parser::ParseApp(AppPtr, *BuildParamsPtr);
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Closes processing of an "app:" section.
 */
//--------------------------------------------------------------------------------------------------
void syy_FinalizeApp
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    AppPtr = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the current application sandboxed or unsandboxed ("true" if sandboxed or "false" if
 * unsandboxed).
 */
//--------------------------------------------------------------------------------------------------
void syy_SetSandboxed
(
    const char* mode    ///< The mode string.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (strcmp(mode, "false") == 0)
        {
            app.IsSandboxed(false);
        }
        else if (strcmp(mode, "true") == 0)
        {
            app.IsSandboxed(true);
        }
        else
        {
            throw legato::Exception("Unrecognized content in 'sandboxed:' section: '"
                                    + std::string(mode) + "'.  Expected 'true' or 'false'.");
        }
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the current application start-up mode ("manual" or "auto"; default is "auto").
 */
//--------------------------------------------------------------------------------------------------
void syy_SetStartMode
(
    const char* mode    ///< The mode string.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (strcmp(mode, "auto") == 0)
        {
            app.StartMode(legato::App::AUTO);
        }
        else if (strcmp(mode, "manual") == 0)
        {
            app.StartMode(legato::App::MANUAL);
        }
        else
        {
            throw legato::Exception("Unrecognized start mode: '" + std::string(mode) + "'");
        }
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a group name to the list of groups that the current application's user should be a member of.
 */
//--------------------------------------------------------------------------------------------------
void syy_AddGroup
(
    const char* name    ///< Name of the group.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        app.AddGroup(name);
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Clear the list of groups that the current application's user should be a member of.
 */
//--------------------------------------------------------------------------------------------------
void syy_ClearGroups
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        app.ClearGroups();
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of threads that the current application is allowed to have running at
 * any given time.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxThreads
(
    int limit   ///< Must be a positive integer.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "  Maximum number of threads for app '" << app.Name() << "': "
                      << limit << std::endl;
        }

        app.MaxThreads(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of bytes that can be allocated for POSIX MQueues for all processes
 * in the current application at any given time.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxMQueueBytes
(
    int limit   ///< Must be a positive integer or zero.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "  Maximum number of bytes for POSIX MQueues: " << limit << std::endl;
        }

        if (limit < 0)
        {
            throw legato::Exception("POSIX MQueue size limit must not be a negative number.");
        }

        app.MaxMQueueBytes(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of signals that are allowed to be queued-up by sigqueue() waiting for
 * processes in the current application at any given time.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxQueuedSignals
(
    int limit   ///< Must be a positive integer.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "  Maximum number of queued signals: " << limit << std::endl;
        }

        if (limit < 0)
        {
            throw legato::Exception("Queued signals limit must not be a negative number.");
        }

        app.MaxQueuedSignals(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum amount of memory (in bytes) the current application is allowed to use for
 * all of its processes.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxMemoryBytes
(
    int limit   ///< Must be a positive integer or zero.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "  Memory limit: " << limit << " bytes" << std::endl;
        }

        if (limit <= 0)
        {
            throw legato::Exception("Memory limit must be a positive number.");
        }

        app.MaxMemoryBytes(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the cpu share to be shared by all processes in the current application.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetCpuShare
(
    int limit   ///< Must be a positive integer.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "  CPU share: " << limit << std::endl;
        }

        if (limit <= 0)
        {
            throw legato::Exception("CPU share must be a positive number.");
        }

        app.CpuShare(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum amount of RAM (in bytes) that the current application is allowed to consume
 * through usage of its temporary sandbox file system.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxFileSystemBytes
(
    int limit   ///< Must be a positive integer or zero.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "  Maximum size of sandbox temporary (RAM) file system: " << limit
                      << " bytes" << std::endl;
        }

        if (limit < 0)
        {
            throw legato::Exception("File system size limit must not be a negative number.");
        }

        app.MaxFileSystemBytes(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum priority level of processes in the current application.
 * Does not set the starting priority, unless the application's .adef file is trying to start
 * a process at a priority higher than the one specified here, in which case the process's
 * starting priority will be lowered to this level.
 *
 * Allowable values are:
 *   "idle" - intended for very low priority processes that will only get CPU time if there are
 *            no other processes waiting for the CPU.
 *
 *   "low",
 *   "medium",
 *   "high" - intended for normal processes that contend for the CPU.  Processes with these
 *            priorities do not preempt each other but their priorities affect how they are
 *            inserted into the scheduling queue. ie. "high" will get higher priority than
 *            "medium" when inserted into the queue.
 *
 *   "rt1"
 *    ...
 *   "rt32" - intended for (soft) realtime processes. A higher realtime priority will pre-empt
 *            a lower realtime priority (ie. "rt2" would pre-empt "rt1"). Processes with any
 *            realtime priority will pre-empt processes with "high", "medium", "low" and "idle"
 *            (non-real-time) priorities.  Also, note that processes with these realtime priorities
 *            will pre-empt the Legato framework processes so take care to design realtime
 *            processes that relinguishes the CPU appropriately.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxPriority
(
    const char* priority
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "    Maximum thread priority: " << priority << std::endl;
        }

        // Apply the priority to all process environments in the current application.
        for (auto& env : app.ProcEnvironments())
        {
            env.MaxPriority(priority);
        }
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) of the core dump file that any process in the current
 * application can generate.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxCoreDumpFileBytes
(
    int limit   ///< Must be a positive integer or zero.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "    Maximum size of core dump files: " << limit << " (bytes)"
                      << std::endl;
        }

        // Apply the limit to all process environments in the current application.
        for (auto& env : app.ProcEnvironments())
        {
            env.MaxCoreDumpFileBytes(limit);
        }
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) that any process in the current application can make files.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxFileBytes
(
    int limit   ///< Must be a positive integer or zero.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "    Maximum file size: " << limit << " (bytes)" << std::endl;
        }

        // Apply the limit to all process environments in the current application.
        for (auto& env : app.ProcEnvironments())
        {
            env.MaxFileBytes(limit);
        }
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) that any process in the current application is allowed to lock
 * into physical memory.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxLockedMemoryBytes
(
    int limit   ///< Must be a positive integer or zero.
                ///  Will be rounded down to the nearest system memory page size.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "    Maximum amount of locked physical memory: " << limit << " (bytes)"
                      << std::endl;
        }

        // Apply the limit to all process environments in the current application.
        for (auto& env : app.ProcEnvironments())
        {
            env.MaxLockedMemoryBytes(limit);
        }
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of file descriptors that each process in the current application are
 * allowed to have open at one time.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetMaxFileDescriptors
(
    int limit   ///< Must be a positive integer.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "    Maximum number of file descriptors: " << limit << std::endl;
        }

        // Apply the limit to all process environments in the current application.
        for (auto& env : app.ProcEnvironments())
        {
            env.MaxFileDescriptors(limit);
        }
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the action that should be taken if any process in the current application
 * terminates with a non-zero exit code (i.e., any error code other than EXIT_SUCCESS).
 *
 * Accepted actions are:
 *  "ignore"        - Leave the process dead.
 *  "restart"       - Restart the process.
 *  "restartApp"    - Terminate and restart the whole application.
 *  "stopApp"       - Terminate the application and leave it stopped.
 *  "reboot"        - Reboot the device.
 *  "pauseApp"      - Send a SIGSTOP to all processes in the application, halting them in their
 *                    tracks, but not killing them.  This allows the processes to be inspected
 *                    for debugging purposes.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetFaultAction
(
    const char* action
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "    Fault action: " << action << std::endl;
        }

        // Apply the limit to all process environments in the current application.
        for (auto& env : app.ProcEnvironments())
        {
            env.FaultAction(action);
        }
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the action that should be taken if any process in the current application
 * terminates due to a watchdog time-out.
 *
 * Accepted actions are:
 *  "ignore"        - Leave the process dead.
 *  "restart"       - Restart the process.
 *  "stop"          - Terminate the process if it is still running.
 *  "restartApp"    - Terminate and restart the whole application.
 *  "stopApp"       - Terminate the application and leave it stopped.
 *  "reboot"        - Reboot the device.
 *  "pauseApp"      - Send a SIGSTOP to all processes in the application, halting them in their
 *                    tracks, but not killing them.  This allows the processes to be inspected
 *                    for debugging purposes.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetWatchdogAction
(
    const char* action
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "    Watchdog action: " << action << std::endl;
        }

        // Set the current application's default watchdog action.
        app.WatchdogAction(action);

        // Also apply it to all process environments in the current application.
        for (auto& env : app.ProcEnvironments())
        {
            env.WatchdogAction(action);
        }
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the timeout for the watchdogs in the current application.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetWatchdogTimeout
(
    const int timeout  ///< The time in milliseconds after which the watchdog expires if not
                       ///< kicked again before then. Only positive values are allowed.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "    Watchdog timeout: " << timeout << std::endl;
        }

        // Set the current application's default watchdog timeout.
        app.WatchdogTimeout(timeout);

        // Also apply it to all process environments in the current application.
        for (auto& env : app.ProcEnvironments())
        {
            env.WatchdogTimeout(timeout);
        }
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Disables the watchdog timeout in the current application.
 */
//--------------------------------------------------------------------------------------------------
void syy_SetWatchdogDisabled
(
    const char* never  ///< The only acceptable string is "never".
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        const std::string timeout(never);

        legato::App& app = GetCurrentApp();

        if (syy_IsVerbose)
        {
            std::cout << "    Watchdog timeout: " << timeout << std::endl;
        }

        // Disable the watchdog at the app level and in each of the app's process environments.
        app.WatchdogTimeout(timeout);
        for (auto& env : app.ProcEnvironments())
        {
            env.WatchdogTimeout(timeout);
        }
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the size of a pool in the current application.
 *
 * The pool name is expected to be of the form "process.component.pool".
 */
//--------------------------------------------------------------------------------------------------
void syy_SetPoolSize
(
    const char* poolName,
    int numBlocks           ///< [in] Must be non-negative (>= 0).
)
//--------------------------------------------------------------------------------------------------
{
    if (syy_IsVerbose)
    {
        std::cout << "  Pool '" << poolName << " set to " << numBlocks << " blocks" << std::endl;
    }

    std::cerr << "**WARNING: Pool size configuration not yet implemented." << std::endl;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create an IPC API binding between two apps.
 *
 * The both interface specifications are always expected to take the form "app.interface".
 */
//--------------------------------------------------------------------------------------------------
void syy_AddAppToAppBind
(
    const char* clientInterfaceSpec,    ///< [in] Client-side interface.
    const char* serverInterfaceSpec     ///< [in] Server-side interface.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        const std::string clientSpec(clientInterfaceSpec);
        const std::string serverSpec(serverInterfaceSpec);

        // Check that the client and server interface specifications are valid.
        size_t numParts = yy_CheckInterfaceSpec(clientSpec);
        if (numParts != 2)
        {
            throw legato::Exception("Too many parts in client interface specification '"
                                    + clientSpec + "'.  Should be of the form \"app.interface\""
                                    " or \"<user>.interface\".");
        }
        numParts = yy_CheckInterfaceSpec(serverSpec);
        if (numParts != 2)
        {
            throw legato::Exception("Too many parts in server interface specification '"
                                    + serverSpec + "'.  Should be of the form \"app.interface\""
                                    " or \"<user>.interface\".");
        }

        // Extract the client app name and interface name.
        size_t periodPos = clientSpec.find('.');
        std::string clientAppName = clientSpec.substr(0, periodPos);
        std::string clientInterfaceName = clientSpec.substr(periodPos + 1);

        // Extract the server app name and interface name.
        periodPos = serverSpec.find('.');
        std::string serverAppName = serverSpec.substr(0, periodPos);
        std::string serverInterfaceName = serverSpec.substr(periodPos + 1);

        if (syy_IsVerbose)
        {
            std::cout << "  Binding client interface '" << clientAppName << "."
                      << clientInterfaceName << "' to server interface '" << serverAppName << "."
                      << serverInterfaceName << "' (both client and server are apps)."
                      << std::endl;
        }

        legato::UserToUserApiBind bind;
        bind.ClientAppName(clientAppName);
        bind.ClientInterfaceName(clientInterfaceName);
        bind.ServerAppName(serverAppName);
        bind.ServerInterfaceName(serverInterfaceName);

        SystemPtr->AddApiBind(std::move(bind));
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create an IPC API binding from an application's client-side interface to a service offered
 * by a specific user account.
 *
 * The client interface specification is expected to be of the form "app.interface".
 */
//--------------------------------------------------------------------------------------------------
void syy_AddAppToUserBind
(
    const char* clientInterfaceSpec,    ///< [in] Client-side interface.
    const char* serverUserName,         ///< [in] Server-side user account name.
    const char* serverInterfaceName     ///< [in] Server-side interface name (with a '.' in front).
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        const std::string clientSpec(clientInterfaceSpec);

        // Check that the client interface specification is valid.
        size_t numParts = yy_CheckInterfaceSpec(clientSpec);
        if (numParts != 2)
        {
            throw legato::Exception("Too many parts in client interface specification '"
                                    + clientSpec + "'.  Should be of the form \"app.interface\""
                                    " or \"<user>.interface\".");
        }

        // Extract the client app name and interface name.
        size_t periodPos = clientSpec.find('.');
        std::string clientAppName = clientSpec.substr(0, periodPos);
        std::string clientInterfaceName = clientSpec.substr(periodPos + 1);

        // Make sure there's a leading '.' in front of the server interface name.
        if (serverInterfaceName[0] != '.')
        {
            throw legato::Exception("Missing '.' separator in server interface specification '<"
                                    + std::string(serverUserName) + ">" + serverInterfaceName
                                    + "'.");
        }

        // Make sure there's more than just a leading '.' in the server interface name.
        if (serverInterfaceName[1] == '\0')
        {
            throw legato::Exception("Missing interface name after '.' separator in server interface"
                                    " specification '<" + std::string(serverUserName) + ">"
                                    + serverInterfaceName + "'.");
        }

        // Remove the leading '.' from the server interface name.
        serverInterfaceName += 1;

        if (syy_IsVerbose)
        {
            std::cout << "  Binding client interface '" << clientAppName << "."
                      << clientInterfaceName << "' to server interface '" << serverUserName << "."
                      << serverInterfaceName << "' (client is an app, server is a non-app user)."
                      << std::endl;
        }

        legato::UserToUserApiBind bind;
        bind.ClientAppName(clientAppName);
        bind.ClientInterfaceName(clientInterfaceName);
        bind.ServerUserName(serverUserName);
        bind.ServerInterfaceName(serverInterfaceName);

        SystemPtr->AddApiBind(std::move(bind));
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Create an IPC API binding from a specific user account's client-side interface to a service
 * offered by an application.
 *
 * The server interface specification is expected to be of the form "app.interface".
 */
//--------------------------------------------------------------------------------------------------
void syy_AddUserToAppBind
(
    const char* clientUserName,         ///< [in] Client-side user account name.
    const char* clientInterfaceName,    ///< [in] Client-side interface name.
    const char* serverInterfaceSpec     ///< [in] Server-side interface.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        const std::string serverSpec(serverInterfaceSpec);

        // Make sure there's a leading '.' in front of the client interface name.
        if (clientInterfaceName[0] != '.')
        {
            throw legato::Exception("Missing '.' separator in client interface specification '<"
                                    + std::string(clientUserName) + ">" + clientInterfaceName
                                    + "'.");
        }

        // Make sure there's more than just a leading '.' in the client interface name.
        if (clientInterfaceName[1] == '\0')
        {
            throw legato::Exception("Missing interface name after '.' separator in client interface"
                                    " specification '<" + std::string(clientUserName) + ">"
                                    + clientInterfaceName + "'.");
        }

        // Remove the leading '.' from the client interface name.
        clientInterfaceName += 1;

        // Check that the server interface specification is valid.
        size_t numParts = yy_CheckInterfaceSpec(serverSpec);
        if (numParts != 2)
        {
            throw legato::Exception("Too many parts in server interface specification '"
                                    + serverSpec + "'.  Should be of the form \"app.interface\""
                                    " or \"<user>.interface\".");
        }

        // Extract the server app name and interface name.
        size_t periodPos = serverSpec.find('.');
        std::string serverAppName = serverSpec.substr(0, periodPos);
        std::string serverInterfaceName = serverSpec.substr(periodPos + 1);

        if (syy_IsVerbose)
        {
            std::cout << "  Binding client interface '<" << clientUserName << ">."
                      << clientInterfaceName << "' to server interface '" << serverAppName << "."
                      << serverInterfaceName << "' (client is a non-app user, server is an app)."
                      << std::endl;
        }

        legato::UserToUserApiBind bind;
        bind.ClientUserName(clientUserName);
        bind.ClientInterfaceName(clientInterfaceName);
        bind.ServerAppName(serverAppName);
        bind.ServerInterfaceName(serverInterfaceName);

        SystemPtr->AddApiBind(std::move(bind));
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Create an IPC API binding from a specific user account's client-side interface to a specific
 * user account's server-side interface.
 */
//--------------------------------------------------------------------------------------------------
void syy_AddUserToUserBind
(
    const char* clientUserName,         ///< [in] Client-side user account name.
    const char* clientInterfaceName,    ///< [in] Client-side interface name.
    const char* serverUserName,         ///< [in] Server-side user account name.
    const char* serverInterfaceName     ///< [in] Server-side interface name.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        // Make sure there's a leading '.' in front of the client interface name.
        if (clientInterfaceName[0] != '.')
        {
            throw legato::Exception("Missing '.' separator in client interface specification '<"
                                    + std::string(clientUserName) + ">" + clientInterfaceName
                                    + "'.");
        }

        // Make sure there's more than just a leading '.' in the client interface name.
        if (clientInterfaceName[1] == '\0')
        {
            throw legato::Exception("Missing interface name after '.' separator in client interface"
                                    " specification '<" + std::string(clientUserName) + ">"
                                    + clientInterfaceName + "'.");
        }

        // Remove the leading '.' from the client interface name.
        clientInterfaceName += 1;

        // Make sure there's a leading '.' in front of the server interface name.
        if (serverInterfaceName[0] != '.')
        {
            throw legato::Exception("Missing '.' separator in server interface specification '<"
                                    + std::string(serverUserName) + ">" + serverInterfaceName
                                    + "'.");
        }

        // Make sure there's more than just a leading '.' in the server interface name.
        if (serverInterfaceName[1] == '\0')
        {
            throw legato::Exception("Missing interface name after '.' separator in server interface"
                                    " specification '<" + std::string(serverUserName) + ">"
                                    + serverInterfaceName + "'.");
        }

        // Remove the leading '.' from the server interface name.
        serverInterfaceName += 1;

        if (syy_IsVerbose)
        {
            std::cout << "  Binding client interface '<" << clientUserName << ">."
                      << clientInterfaceName << "' to server interface '<"
                      << serverUserName << ">." << serverInterfaceName
                      << "' (both client and server are non-app users)."
                      << std::endl;
        }

        legato::UserToUserApiBind bind;
        bind.ClientUserName(clientUserName);
        bind.ClientInterfaceName(clientInterfaceName);
        bind.ServerUserName(serverUserName);
        bind.ServerInterfaceName(serverInterfaceName);

        SystemPtr->AddApiBind(std::move(bind));
    }
    catch (legato::Exception e)
    {
        syy_error(e.what());
    }
}
