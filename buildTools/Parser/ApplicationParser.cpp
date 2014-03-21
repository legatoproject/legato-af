//--------------------------------------------------------------------------------------------------
/**
 * Main file for the Application Parser.
 *
 * Copyright (C) 2013 Sierra Wireless Inc., all rights reserved.
 **/
//--------------------------------------------------------------------------------------------------

#include "../ComponentModel/LegatoObjectModel.h"
#include "Parser.h"

extern "C"
{
    #include "ApplicationParser.tab.h"
    #include "ParserCommonInternals.h"
    #include "ApplicationParserInternals.h"
    #include <string.h>
    #include <stdio.h>
}


//--------------------------------------------------------------------------------------------------
/**
 * Non-zero if verbose operation is requested.
 */
//--------------------------------------------------------------------------------------------------
int ayy_IsVerbose = 0;


//--------------------------------------------------------------------------------------------------
/**
 * Build parameters received via the command line.
 */
//--------------------------------------------------------------------------------------------------
static const legato::BuildParams_t* BuildParamsPtr;


//--------------------------------------------------------------------------------------------------
/**
 * Pointer to the Component object for the component that is currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
static legato::App* AppPtr;


//--------------------------------------------------------------------------------------------------
/**
 * Executable object for the executable currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
static legato::Executable* ExePtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Process Environment object for the processes: section currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
static legato::ProcessEnvironment* ProcEnvPtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Process object for the run: subsection currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
static legato::Process* ProcessPtr = NULL;


namespace legato
{

namespace parser
{


//--------------------------------------------------------------------------------------------------
/**
 * Parses an Application Definition (.adef) and populates an App object with the information
 * garnered.
 *
 * @note    Expects the Application's definition (.adef) file path to be set.
 */
//--------------------------------------------------------------------------------------------------
void ParseApp
(
    App& app,                           ///< The object to be populated.
    const BuildParams_t& buildParams    ///< Build parameters obtained from the command line.
)
//--------------------------------------------------------------------------------------------------
{
    AppPtr = &app;
    BuildParamsPtr = &buildParams;

    const std::string& path = app.DefFilePath();

    // Open the .adef file for reading.
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
    ayy_FileName = path.c_str();
    ayy_IsVerbose = (BuildParamsPtr->IsVerbose() ? 1 : 0);
    ayy_EndOfFile = 0;
    ayy_ErrorCount = 0;
    ayy_restart(file);

    // Until the parsing is done,
    int parsingResult;
    do
    {
        // Start parsing.
        parsingResult = ayy_parse();
    }
    while ((parsingResult != 0) && (!ayy_EndOfFile));

    // Close the file
    fclose(file);

    // Halt if there were errors.
    if (ayy_ErrorCount > 0)
    {
        throw Exception("Errors encountered while parsing '" + path + "'.");
    }

    AppPtr = NULL;
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
 * Set the application sandboxed or unsandboxed ("true" if sandboxed or "false" if unsandboxed).
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetSandboxed
(
    const char* mode    ///< The mode string.
)
//--------------------------------------------------------------------------------------------------
{
    if (strcmp(mode, "false") == 0)
    {
        AppPtr->IsSandboxed(false);
    }
    else if (strcmp(mode, "true") == 0)
    {
        AppPtr->IsSandboxed(true);
    }
    else
    {
        std::string msg = "Unrecognized content in 'sandboxed:' section: '";
        msg += mode;
        msg += "'.  Expected 'true' or 'false'.";
        ayy_error(msg.c_str());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the application start-up mode ("manual" or "auto"; default is "auto").
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetStartMode
(
    const char* mode    ///< The mode string.
)
//--------------------------------------------------------------------------------------------------
{
    if (strcmp(mode, "auto") == 0)
    {
        AppPtr->StartMode(legato::App::AUTO);
    }
    else if (strcmp(mode, "manual") == 0)
    {
        AppPtr->StartMode(legato::App::MANUAL);
    }
    else
    {
        std::string msg = "Unrecognized start mode: '";
        msg += mode;
        msg += "'";
        ayy_error(msg.c_str());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a component to the list of components used by this application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddComponent
(
    const char* name,  ///< Name of the component (or "" if the name should be derived
                        ///   from the file system path).
    const char* path   ///< File system path of the component.
)
//--------------------------------------------------------------------------------------------------
{
    ayy_error("components: section is not yet implemented.  Use component path directly in"
              " executables section for now.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a group name to the list of groups that this application's user should be a member of.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddGroup
(
    const char* name    ///< Name of the group.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        AppPtr->AddGroup(name);
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Add a component to the list of components used by this application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_FinishProcessesSection
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if ((ayy_IsVerbose) && (ProcEnvPtr != NULL))
    {
        std::cout << "-- end of processes section --" << std::endl;
    }

    ProcEnvPtr = NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the Process Environment object for the "processes:" subsection currently
 * being parsed.
 **/
//--------------------------------------------------------------------------------------------------
static legato::ProcessEnvironment& GetProcessEnvironment
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (ProcEnvPtr == NULL)
    {
        if (ayy_IsVerbose)
        {
            std::cout << "-- start of processes section --" << std::endl;
        }

        ProcEnvPtr = &(AppPtr->CreateProcEnvironment());
    }
    return *ProcEnvPtr;
}




//--------------------------------------------------------------------------------------------------
/**
 * Add a file to the application.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddFile
(
    const char* permissions,///< String representing the permissions.
    const char* sourcePath, ///< The file path in the build host's file system.
    const char* destPath    ///< The file path in the target file system, inside sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        printf("Adding file '%s' to the application bundle (at '%s' %s in the sandbox).\n",
               sourcePath,
               destPath,
               permissions);
    }

    try
    {
        AppPtr->AddIncludedFile( {  yy_GetPermissionFlags(permissions),
                                    yy_StripQuotes(sourcePath),
                                    yy_StripQuotes(destPath)   } );
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get a Component object for a given component path.
 *
 * @return reference to the Component object.
 */
//--------------------------------------------------------------------------------------------------
static legato::Component& GetComponent
(
    const std::string& name     ///< The name of the component.
)
//--------------------------------------------------------------------------------------------------
{
    auto& map = AppPtr->ComponentMap();

    // If the component is already in the application, just return a reference to that.
    auto i = map.find(name);
    if (i != map.end())
    {
        return map[name];
    }
    // Otherwise, create a new Component object inside the App for this component name,
    // tell the parser to parse it, and return a reference to it.
    else
    {
        legato::Component& component = AppPtr->CreateComponent(name);

        legato::parser::ParseComponent(component, *BuildParamsPtr);

        return component;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Add a new executable to the app.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddExecutable
(
    const char* exePath     ///< Exe file path (relative to executables output directory).
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  Creating executable '" << exePath << "'." << std::endl;
    }

    legato::Executable& exe = AppPtr->CreateExecutable(exePath);

    if (ayy_IsVerbose)
    {
        legato::Component& defaultComponent = exe.DefaultComponent();

        std::cout << "    Default component for '" << exePath << "' is '";
        std::cout << defaultComponent.Name() << "'." << std::endl;
    }

    ExePtr = &exe;
}



//--------------------------------------------------------------------------------------------------
/**
 * Add an item of content to the executable that is currently being parsed.
 */
//--------------------------------------------------------------------------------------------------
void ayy_AddExeContent
(
    const char* contentName
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        const char* contentType;

        if (legato::IsCSource(contentName))
        {
            contentType = "C source code";

            // Add the source code file to the default component.
            ExePtr->AddCSourceFile(contentName);
        }
        else if (legato::IsLibrary(contentName))
        {
            contentType = "library";

            // Add the library file to the list of libraries to be linked with the default
            // component.
            ExePtr->AddLibrary(contentName);
        }
        else if (legato::IsComponent(contentName, BuildParamsPtr->ComponentDirs()))
        {
            contentType = "component";

            // Find the component and add it to the executable's list of component instances.
            // NOTE: For now, we only support one instance of a component per executable.
            ExePtr->AddComponentInstance(GetComponent(contentName));
        }
        else
        {
            contentType = "** unknown **";

            std::string msg = "Executable '";
            msg += ExePtr->OutputPath();
            msg += "': Unable to identify content item '";
            msg += contentName;
            msg += "'.";

            throw legato::Exception(msg);
        }

        if (ayy_IsVerbose)
        {
            std::cout << "    Added '" << contentName << "' (" << contentType << ")"
                      << " to executable '" << ExePtr->OutputPath() << "'." << std::endl;
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Gets a pointer to the Process object for the "run:" subsection currently being parsed.
 **/
//--------------------------------------------------------------------------------------------------
static legato::Process& GetProcess
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    if (ProcessPtr == NULL)
    {
        ProcessPtr = &(GetProcessEnvironment().CreateProcess());
    }
    return *ProcessPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Wrap-up the processing of a (non-empty) "run:" subsection in the "processes:" section.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_FinalizeProcess
(
    const char* name    ///< The process name, or NULL if the exe name should be used.
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        if (name == NULL)
        {
            ProcessPtr->Name(legato::GetLastPathNode(ProcessPtr->ExePath()));
        }
        else
        {
            ProcessPtr->Name(name);
        }

        if (ayy_IsVerbose)
        {
            std::cout << "    Will start process '" << ProcessPtr->Name() << "' using command line: "
                      << "\"" << ProcessPtr->ExePath() << "\"";
            for (const auto& arg : ProcessPtr->CommandLineArgs())
            {
                std::cout << " \"" << arg << "\"";
            }
            std::cout << std::endl;
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }

    ProcessPtr = NULL;
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the path to the executable that is to be used to start the process.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_SetProcessExe
(
    const char* path
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        std::string exePath = yy_StripQuotes(path);

        if (!legato::IsAbsolutePath(exePath))
        {
            exePath = exePath;
        }

        GetProcess().ExePath(exePath);
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a command-line argument to a process.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddProcessArg
(
    const char* arg
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        GetProcess().AddCommandLineArg(yy_StripQuotes(arg));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Adds an environment variable to the Process Environment object associated with the processes:
 * section that is currently being parsed.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddEnvVar
(
    const char* name,
    const char* value
)
//--------------------------------------------------------------------------------------------------
{
    try
    {
        GetProcessEnvironment().AddEnvVar(name, yy_StripQuotes(value));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Add a file import mapping, which is the mapping of an object from the file system somewhere
 * outside the application sandbox to somewhere in the file system inside the application sandbox.
 **/
//--------------------------------------------------------------------------------------------------
void ayy_AddFileImport
(
    const char* permissions,///< String representing the permissions. (e.g., "[r]")
    const char* sourcePath, ///< The path in the target file system., outside the sandbox.
    const char* destPath    ///< The path in the target file system, inside the sandbox.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        printf("  Importing '%s' from the target file system to '%s' %s inside the sandbox.\n",
               sourcePath,
               destPath,
               permissions);
    }

    try
    {
        AppPtr->AddImportedFile( {  yy_GetPermissionFlags(permissions),
                                    yy_StripQuotes(sourcePath),
                                    yy_StripQuotes(destPath)    } );
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of processes that this application is allowed to have running at any
 * given time.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetNumProcsLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  Maximum number of processes: " << limit << std::endl;
    }

    try
    {
        if (limit < 0)
        {
            throw legato::Exception("Limit on number of processes must be a non-negative number.");
        }

        AppPtr->NumProcs(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of messages that are allowed to be queued-up in POSIX MQueues waiting for
 * processes in this application at any given time.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetMqueueSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  Maximum number of POSIX MQueue messages: " << limit << std::endl;
    }

    try
    {
        if (limit < 0)
        {
            throw legato::Exception("POSIX MQueue size limit must be a non-negative number.");
        }

        AppPtr->MqueueSize(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of real-time signals that are allowed to be queued-up waiting for
 * processes in this application at any given time.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetRTSignalQueueSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  Maximum number of real-time signals: " << limit << std::endl;
    }

    try
    {
        if (limit < 0)
        {
            throw legato::Exception("Real-time signal queue size limit must be a non-negative number.");
        }

        AppPtr->RtSignalQueueSize(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum amount of RAM (in bytes) that the application is allowed to consume through
 * usage of its temporary sandbox file system.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetFileSystemSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  Maximum size of sandbox temporary file system: " << limit << " (bytes)" << std::endl;
    }

    try
    {
        if (limit < 0)
        {
            throw legato::Exception("File system size limit must be a non-negative number of bytes.");
        }

        AppPtr->FileSystemSize(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * Sets the starting (and maximum) priority level of processes in the current processes section.
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
void ayy_SetPriority
(
    const char* priority
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Starting (and max) process priority: " << priority << std::endl;
    }

    try
    {
        legato::ProcessEnvironment& env = GetProcessEnvironment();

        if (   (strcmp(priority, "idle") == 0)
            || (strcmp(priority, "low") == 0)
            || (strcmp(priority, "medium") == 0)
            || (strcmp(priority, "high") == 0)
           )
        {
            env.Priority(priority);
        }
        else if ((priority[0] == 'r') && (priority[1] == 't'))
        {
            int number = yy_GetNumber(priority + 2);
            if ((number < 1) || (number > 32))
            {
                throw legato::Exception("Real-time priority level must be between rt1 and rt32, inclusive.");
            }
            env.Priority(priority);
        }
        else
        {
            throw legato::Exception(std::string("Unrecognized priority level '") + priority + "'.");
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }

}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) that a process in the current processes section can make
 * its virtual address space.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetVMemSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Maximum size of process virtual memory: " << limit << " (bytes)" << std::endl;
    }

    try
    {
        legato::ProcessEnvironment& env = GetProcessEnvironment();

        if (limit < 0)
        {
            throw legato::Exception("Maximum virtual memory size limit must be a non-negative number of bytes.");
        }

        env.VMemSize(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) of the core dump file that a process in the current processes
 * section can generate.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetCoreFileSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Maximum size of core dump files: " << limit << " (bytes)" << std::endl;
    }

    try
    {
        legato::ProcessEnvironment& env = GetProcessEnvironment();

        if (limit < 0)
        {
            throw legato::Exception("Maximum core dump file size limit must be a non-negative number of bytes.");
        }

        env.CoreFileSize(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) that a process in the current processes section can make files.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetMaxFileSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Maximum file size: " << limit << " (bytes)" << std::endl;
    }

    try
    {
        legato::ProcessEnvironment& env = GetProcessEnvironment();

        if (limit < 0)
        {
            throw legato::Exception("Maximum file size limit must be a non-negative number of bytes.");
        }

        env.MaxFileSize(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the maximum size (in bytes) that a process in this processes section is allowed to lock
 * into physical memory.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetMemLockSizeLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
                ///  Will be rounded down to the nearest system memory page size.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Maximum amount of locked physical memory: " << limit << " (bytes)" << std::endl;
    }

    try
    {
        legato::ProcessEnvironment& env = GetProcessEnvironment();

        if (limit < 0)
        {
            throw legato::Exception("Locked memory size limit must be a non-negative number of bytes.");
        }

        env.MemLockSize(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the maximum number of file descriptor that each process in the processes section are
 * allowed to have open at one time.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetNumFdsLimit
(
    int limit   ///< Must be a positive integer.  May be overridden by system-wide settings.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Maximum number of file descriptors: " << limit << std::endl;
    }

    try
    {
        legato::ProcessEnvironment& env = GetProcessEnvironment();

        if (limit < 0)
        {
            throw legato::Exception("Number of file descriptors must be positive.");
        }

        env.NumFds(static_cast<size_t>(limit));
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the action that should be taken if a process in the process group currently being parsed
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
void ayy_SetFaultAction
(
    const char* action
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Fault action: " << action << std::endl;
    }

    try
    {
        legato::ProcessEnvironment& env = GetProcessEnvironment();

        if (   (strcmp(action, "ignore") == 0)
            || (strcmp(action, "restart") == 0)
            || (strcmp(action, "restartApp") == 0)
            || (strcmp(action, "stopApp") == 0)
            || (strcmp(action, "reboot") == 0)
            || (strcmp(action, "pauseApp") == 0)
           )
        {
            env.FaultAction(action);
        }
        else
        {
            throw legato::Exception(std::string("Unknown fault action '") + action + "'.");
        }
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * Sets the action that should be taken if a process in the process group currently being parsed
 * terminates due to a watchdog time-out.
 *
 * @todo Implement watchdog actions.
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetWatchdogAction
(
    const char* action  ///< Accepted actions are TBD.
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "    Watchdog action: " << action << std::endl;
    }
    std::cerr << "**WARNING: Watchdog not yet implemented." << std::endl;
}



//--------------------------------------------------------------------------------------------------
/**
 * Set the size of a pool.
 *
 * The pool name is expected to be of the form "process.component.pool".
 */
//--------------------------------------------------------------------------------------------------
void ayy_SetPoolSize
(
    const char* poolName,
    int numBlocks
)
//--------------------------------------------------------------------------------------------------
{
    if (ayy_IsVerbose)
    {
        std::cout << "  Pool '" << poolName << " set to " << numBlocks << " blocks" << std::endl;
    }
    std::cerr << "**WARNING: Pool size configuration not yet implemented." << std::endl;
}



//--------------------------------------------------------------------------------------------------
/**
 * Create an IPC binding between one interface and another.
 *
 * Each interface definition is expected to be of the form "proc.interface".  One of the
 * two interfaces must be an imported interface, while the other must be an exported interface.
 */
//--------------------------------------------------------------------------------------------------
void ayy_CreateBind
(
    const char* importSpecifier,
    const char* exportSpecifier
)
//--------------------------------------------------------------------------------------------------
{
    std::cerr << "**WARNING: Binding not yet implemented." << std::endl;

    try
    {
        // Split the import interface specification into its component parts.
        std::string importSpec(importSpecifier);
        size_t pos;
        pos = importSpec.find('.');
        if (pos == std::string::npos)
        {
            throw legato::Exception("Import interface specifier is missing its '.' separator.");
        }
        if (pos == 0)
        {
            throw legato::Exception("Import process name missing before '.' separator.");
        }
        std::string importProcess = importSpec.substr(0, pos);
        std::string importInterface = importSpec.substr(pos + 1);
        if (importInterface.empty())
        {
            throw legato::Exception("Import interface instance name missing after '.' separator.");
        }

        // Split the export interface specification into its component parts.
        std::string exportSpec(exportSpecifier);
        pos = exportSpec.find('.');
        if (pos == std::string::npos)
        {
            throw legato::Exception("Export interface specifier is missing its '.' separator.");
        }
        if (pos == 0)
        {
            throw legato::Exception("Export process name missing before '.' separator.");
        }
        std::string exportProcess = exportSpec.substr(0, pos);
        std::string exportInterface = exportSpec.substr(pos + 1);
        if (exportInterface.empty())
        {
            throw legato::Exception("Export interface instance name missing after '.' separator.");
        }

        if (ayy_IsVerbose)
        {
            std::cout << "  Binding interface '" << importProcess << "." << importInterface << "'"
                      << " to interface '" << exportProcess << "." << exportInterface << "'"
                      << std::endl;
        }

        AppPtr->Bind(importProcess, importInterface, exportProcess, exportInterface);
    }
    catch (legato::Exception e)
    {
        ayy_error(e.what());
    }
}
