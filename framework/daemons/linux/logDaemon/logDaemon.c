/** @file logDaemon.c
 *
 * This module contains the Legato Log Control Daemon implementation.  This daemon keeps track
 * of all logging sessions of all Legato processes and components and sends updates to those
 * sessions' filter settings when instructed to do so by the log control tool.
 *
 * The Log Control Daemon advertises a service using the Log Control Protocol via the Legato
 * @ref c_messaging.
 *
 * All processes open this service and provide the Log Control Daemon with their log sessions'
 * identification, which includes the process name and component name.
 *
 * A command line log control tool is used to set the log settings for components in the system.
 * However, the log control tool does not talk directly with the components, instead the log control
 * tool sends its commands to the Log Control Daemon.  The Log Control Daemon saves a copy of those
 * commands, and if the target component is currently connected to the Log Control Daemon, the
 * command is immediately forwarded to the component.  If the target component is not yet connected,
 * the Log Control Daemon will forward the stored log control command to the component when it
 * later connects.
 *
 * For more details on how the log daemon and the log control tool communicate see the help file for
 * the log control tool.
 *
 * On startup the log daemon reads a configuration file and populates its list of commands from the
 * file ensuring that registered components will receive the initialized configuration from the
 * file.
 *
 * @todo Need to add reading of initial configuration file.
 *
 * @verbatim
                                                   Trace Name <--+-- Enabled Traces List
                                                                             ^
                                                                             |
 Process Name Map ---+---> Process Name ---> Component Name List --+--> Component Name
                               |     ^                                       |
                               v     |                                       v
                        Process List |                                    Settings
                               |     |
 IPC Session Map ----------+   |     |
                           |   |     |
                           v   v     |
 Process ID Map ---+---> Running Process ---> Session List --+--> Log Session ---> Settings
                                                                       |
                                                                       v
                                       isEnabled <--- Trace <--+-- Trace List
@endverbatim
 *
 * The Process Name, Component Name, and Trace Name objects are used to hold state for processes
 * and components that have had settings applied using the process name to identify the process.
 * These settings persist as long as the Log Control Daemon remains running, even if the other
 * processes die or aren't running at the time that the settings are applied.
 *
 * The Running Process objects are used to hold state for actual running processes.  Each of these
 * has a list of active log sessions that can be controlled within that running process.
 * Furthermore, each Log Session object has a list of traces that can be enabled or
 * disabled for that process (identified by trace keyword).
 *
 * There's one IPC session for each running process.  The IPC Session Map is used to find the
 * running process that belongs to an IPC session reference when the IPC system reports that
 * a session closed.  This is how the Log Control Daemon finds out that a client process died.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"

#include "logDaemon.h"

#include "fileDescriptor.h"
#include "limit.h"
#include "linux/logPlatform.h"
#include "log.h"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of processes that we expect to see.  Used to set the hashmap and pool sizes.
 *
 * @todo Make this configurable.
 **/
//--------------------------------------------------------------------------------------------------
#define MAX_EXPECTED_PROCESSES 32

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of components that we expect to see.  Used to set the pool sizes.
 *
 * @todo Make this configurable.
 **/
//--------------------------------------------------------------------------------------------------
#define MAX_EXPECTED_COMPONENTS 128

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of traces that we expect to see.  Used to set the pool sizes.
 *
 * @todo Make this configurable.
 **/
//--------------------------------------------------------------------------------------------------
#define MAX_EXPECTED_TRACES 20


//--------------------------------------------------------------------------------------------------
/**
 * Hash map of Process Name objects, keyed by process name string.
 *
 * Value pointer points to a ProcessName_t.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t ProcessNameMapRef;


//--------------------------------------------------------------------------------------------------
/**
 * Hash map of Running Process objects, keyed by IPC session reference.
 *
 * Value pointer points to a RunningProcess_t.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t IpcSessionMapRef;


//--------------------------------------------------------------------------------------------------
/**
 * Hash map of Running Process objects, keyed by PID.
 *
 * Value pointer points to a RunningProcess_t.
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t ProcessIdMapRef;


//--------------------------------------------------------------------------------------------------
/**
 * Component Name objects are used to store the log level setting associated with a component
 * name for all processes sharing the same process name.
 *
 * Each of these objects is kept on a Process Name object's Component Name List.  They are
 * keyed by component name.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t       link;                   ///< Link in the Process Name's component name list.
    char name[LIMIT_MAX_COMPONENT_NAME_BYTES];  ///< The component name.
    le_log_Level_t      level;                  ///< The log level setting.
    le_dls_List_t       enabledTracesList;      ///< List of enabled trace keywords.
}
ComponentName_t;


//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Component Name objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ComponentNamePoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Objects of this type are used to keep track of settings that are applied to all processes
 * that share a given process name.  They are kept in the Process Name Map.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char    name[LIMIT_MAX_PROCESS_NAME_BYTES]; ///< The process name.
    le_dls_List_t   componentNameList;          ///< List of component names with settings.
    le_dls_List_t   runningProcessesList;       ///< List of running processes with this name.
}
ProcessName_t;


//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Process Name objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ProcessNamePoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Trace Name objects are used to hold trace keywords that have been enabled for all processes
 * that share the same process name.
 *
 * These objects are each kept on a single Component Name object's list of enabled traces.
 *
 * They are keyed by trace keyword string.
 *
 * If a keyword is disabled, it is deleted, so only enabled trace names appear in the list.
 **/
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t   link;          ///< Used to link into Component Name's list of enabled keywords.
    char            name[LIMIT_MAX_LOG_KEYWORD_BYTES];   ///< The keyword.
}
TraceName_t;


//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Trace Name objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t TraceNamePoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Objects of this type keep track of the log filtering state of a single running process.
 *
 * These objects are kept in the Process ID Map and the IPC Session Map.  In addition, if the
 * process's name is known, its Running Process object is also kept in a Process Name object's
 * list of running processes.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t       link;           ///< Used to link into a Process Name object's list
                                        ///  of running processes.
    ProcessName_t*      procNameObjPtr; ///< Ptr to the Process Name object whose list
                                        ///  this Running Process is on (or NULL).
    pid_t               pid;            ///< The process ID.
    le_msg_SessionRef_t ipcSessionRef;  ///< Reference to the IPC session connected to this process.
    le_dls_List_t       logSessionList; ///< List of log sessions in this process.
/* TODO: Implement shared memory.
    void*               sharedMemAddr;  ///< Address of base of memory region shared with
                                        ///  this process.
 */
}
RunningProcess_t;


//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Running Process objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t RunningProcessPoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Log Session objects are used to store the log session details for a single, active log session
 * in a running process.  Each of these objects is kept on a Running Process object's
 * Log Session List.  They are keyed by component name.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t       link;               ///< Link in the Running Process's log session list.
    char componentName[LIMIT_MAX_COMPONENT_NAME_BYTES];  ///< The component name.
    le_log_Level_t      level;              ///< This session's log level.
    le_dls_List_t       traceList;          ///< List of Trace objects for this log session.
}
LogSession_t;


//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Log Session objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t LogSessionPoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Trace objects are used to hold trace keywords and their associated information for
 * active log sessions.
 *
 * These objects are each kept on a Log Session object's list of traces.
 *
 * They are keyed by trace keyword string.
 **/
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t   link;           ///< Used to link into the Running Process's trace flag list.
    char            name[LIMIT_MAX_LOG_KEYWORD_BYTES];   ///< The keyword.
    bool            isEnabled;      ///< true = the keyword is enabled.
}
Trace_t;


//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Trace objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t TracePoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of the data portion in a command packet.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_CMD_DATA_BYTES     (  LOG_MAX_CMD_PACKET_BYTES       \
                                - LIMIT_MAX_PROCESS_NAME_LEN     \
                                - LIMIT_MAX_COMPONENT_NAME_LEN )


//--------------------------------------------------------------------------------------------------
/**
 * File descriptor logging object.
 *
 * Stores info about a file descriptor to be logged.
 **/
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char            appName[LIMIT_MAX_APP_NAME_BYTES];      ///< App name.
    char            procName[LIMIT_MAX_PROCESS_NAME_BYTES]; ///< Process name.
    int             pid;                                    ///< PID of the process.
    le_log_Level_t  level;                                  ///< Log level.
    le_fdMonitor_Ref_t monitorRef;                          ///< Monitor object.
}
FdLog_t;


//--------------------------------------------------------------------------------------------------
/**
 * Pool for file descriptor logging objects.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t FdLogPoolRef;


//--------------------------------------------------------------------------------------------------
/**
 * Maximum length of log messages.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_MSG_SIZE            256



// ========================================
//  FUNCTIONS
// ========================================

//--------------------------------------------------------------------------------------------------
/**
 * Hash computation function for IPC Session IDs.
 *
 * @return  The hash value computed from a IPC Session Reference.
 **/
//--------------------------------------------------------------------------------------------------
static size_t IpcSessionHash
(
    const void* hashKeyPtr
)
{
    return (size_t)(*((le_msg_SessionRef_t*)hashKeyPtr));
}


//--------------------------------------------------------------------------------------------------
/**
 * Equality comparison function for IPC Session IDs.
 *
 * @return  true = the two Session IDs are the same.
 **/
//--------------------------------------------------------------------------------------------------
static bool IpcSessionEquals
(
    const void* hashKey1Ptr,
    const void* hashKey2Ptr
)
{
    return (*((le_msg_SessionRef_t*)hashKey1Ptr) == *((le_msg_SessionRef_t*)hashKey2Ptr));
}


//--------------------------------------------------------------------------------------------------
/**
 * Hash computation function for Process IDs.
 *
 * @return  The hash value computed from a Process ID.
 **/
//--------------------------------------------------------------------------------------------------
static size_t ProcessIdHash
(
    const void* hashKeyPtr
)
{
    return (size_t)(*((pid_t*)hashKeyPtr));
}


//--------------------------------------------------------------------------------------------------
/**
 * Equality comparison function for Process IDs.
 *
 * @return  true = the two Process IDs are the same.
 **/
//--------------------------------------------------------------------------------------------------
static bool ProcessIdEquals
(
    const void* hashKey1Ptr,
    const void* hashKey2Ptr
)
{
    return (*((pid_t*)hashKey1Ptr) == *((pid_t*)hashKey2Ptr));
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches a pointer to a constant string containing the name of a log level.
 **/
//--------------------------------------------------------------------------------------------------
static const char* GetLevelString
(
    le_log_Level_t level
)
//--------------------------------------------------------------------------------------------------
{
    if (level == (le_log_Level_t)-1)
    {
        return "default";
    }

    const char* levelStr = log_SeverityLevelToStr(level);

    if (levelStr == NULL)
    {
        LE_CRIT("Invalid level %d.", level);
        levelStr = "<invalid>";
    }

    return levelStr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find a Process Name object for a given process name.
 *
 * @return
 *      A pointer to the Process Name object on success.
 *      NULL if the process name could not be found.
 */
//--------------------------------------------------------------------------------------------------
static inline ProcessName_t* FindProcessName
(
    const char* processNameStr
)
//--------------------------------------------------------------------------------------------------
{
    // Do a hash map lookup
    return le_hashmap_Get(ProcessNameMapRef, processNameStr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes all Component Name objects for a given a Process Name object.
 **/
//--------------------------------------------------------------------------------------------------
static void DeleteAllComponentNamesForProcessName
(
    ProcessName_t* procNameObjPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* componentLinkPtr;
    le_dls_Link_t* traceNameLinkPtr;

    while (NULL != (componentLinkPtr = le_dls_Pop(&procNameObjPtr->componentNameList)))
    {
        ComponentName_t* compNameObjPtr = CONTAINER_OF(componentLinkPtr, ComponentName_t, link);

        while (NULL != (traceNameLinkPtr = le_dls_Pop(&compNameObjPtr->enabledTracesList)))
        {
            TraceName_t* traceNameObjPtr = CONTAINER_OF(traceNameLinkPtr, TraceName_t, link);

            le_mem_Release(traceNameObjPtr);
        }

        le_mem_Release(compNameObjPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a Process Name object.
 **/
//--------------------------------------------------------------------------------------------------
static void DeleteProcessName
(
    ProcessName_t* procNameObjPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_hashmap_Remove(ProcessNameMapRef, procNameObjPtr->name);

    DeleteAllComponentNamesForProcessName(procNameObjPtr);

    le_mem_Release(procNameObjPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Search a given Process Name's list of Component Names.
 *
 * @return
 *      A pointer to the Component Name object on success.
 *      NULL if the component name could not be found.
 */
//--------------------------------------------------------------------------------------------------
static ComponentName_t* FindComponentName
(
    const ProcessName_t* procNameObjPtr,
    const char* componentNameStr
)
//--------------------------------------------------------------------------------------------------
{
    // Search for the component in the list.
    le_dls_Link_t* linkPtr = le_dls_Peek(&procNameObjPtr->componentNameList);

    while (linkPtr != NULL)
    {
        ComponentName_t* compPtr = CONTAINER_OF(linkPtr, ComponentName_t, link);

        if (strcmp(compPtr->name, componentNameStr) == 0)
        {
            return compPtr;
        }

        // Get the next component.
        linkPtr = le_dls_PeekNext(&procNameObjPtr->componentNameList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Component Name object for a given Process Name object.
 *
 * @return
 *      A pointer to the Component Name object.
 */
//--------------------------------------------------------------------------------------------------
static ComponentName_t* CreateComponentName
(
    ProcessName_t* procNameObjPtr,
    const char* componentNamePtr
)
//--------------------------------------------------------------------------------------------------
{
    ComponentName_t* objPtr = le_mem_ForceAlloc(ComponentNamePoolRef);

    if (le_utf8_Copy(objPtr->name, componentNamePtr, sizeof(objPtr->name), NULL) != LE_OK)
    {
        LE_WARN("Component name '%s' truncated to '%s'.", componentNamePtr, objPtr->name);
    }

    objPtr->level = -1;
    objPtr->enabledTracesList = LE_DLS_LIST_INIT;

    objPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&procNameObjPtr->componentNameList, &objPtr->link);

    return objPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Trace Name object for a given Component Name object.
 *
 * @return
 *      A pointer to the Trace Name object.
 */
//--------------------------------------------------------------------------------------------------
static TraceName_t* CreateTraceName
(
    ComponentName_t* compNameObjPtr,
    const char* keywordStr
)
//--------------------------------------------------------------------------------------------------
{
    TraceName_t* objPtr = le_mem_ForceAlloc(TraceNamePoolRef);

    if (le_utf8_Copy(objPtr->name, keywordStr, sizeof(objPtr->name), NULL) != LE_OK)
    {
        LE_WARN("Trace keyword '%s' truncated to '%s'.", keywordStr, objPtr->name);
    }

    objPtr->link = LE_DLS_LINK_INIT;

    le_dls_Stack(&compNameObjPtr->enabledTracesList, &objPtr->link);

    return objPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Process Name object for a given process name.
 *
 * @return
 *      A pointer to the Process Name object.
 */
//--------------------------------------------------------------------------------------------------
static ProcessName_t* CreateProcessName
(
    const char* processNameStr
)
//--------------------------------------------------------------------------------------------------
{
    ProcessName_t* objPtr = le_mem_ForceAlloc(ProcessNamePoolRef);

    if (le_utf8_Copy(objPtr->name, processNameStr, sizeof(objPtr->name), NULL) != LE_OK)
    {
        LE_WARN("Process name '%s' truncated to '%s'.", processNameStr, objPtr->name);
    }

    objPtr->componentNameList = LE_DLS_LIST_INIT;
    objPtr->runningProcessesList = LE_DLS_LIST_INIT;

    le_hashmap_Put(ProcessNameMapRef, objPtr->name, objPtr);

    return objPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Search a given Component Name's list of enabled traces.
 *
 * @return
 *      A pointer to the Trace Name object on success.
 *      NULL if the trace name could not be found.
 */
//--------------------------------------------------------------------------------------------------
static TraceName_t* FindTraceName
(
    const ComponentName_t* compNameObjPtr,
    const char* traceNameStr
)
//--------------------------------------------------------------------------------------------------
{
    // Search for the keyword in the list.
    le_dls_Link_t* linkPtr = le_dls_Peek(&compNameObjPtr->enabledTracesList);

    while (linkPtr != NULL)
    {
        TraceName_t* tracePtr = CONTAINER_OF(linkPtr, TraceName_t, link);

        if (strcmp(tracePtr->name, traceNameStr) == 0)
        {
            return tracePtr;
        }

        // Get the next component.
        linkPtr = le_dls_PeekNext(&compNameObjPtr->enabledTracesList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Find a Running Process object for a given process ID.
 *
 * @return
 *      A pointer to the Running Process object on success.
 *      NULL if the process could not be found.
 */
//--------------------------------------------------------------------------------------------------
static inline RunningProcess_t* FindProcessByPid
(
    pid_t pid
)
//--------------------------------------------------------------------------------------------------
{
    return le_hashmap_Get(ProcessIdMapRef, &pid);
}


//--------------------------------------------------------------------------------------------------
/**
 * Find a Running Process object for a given IPC session reference.
 *
 * @return
 *      A pointer to the Running Process object on success.
 *      NULL if the process could not be found.
 */
//--------------------------------------------------------------------------------------------------
static inline RunningProcess_t* FindProcessByIpcSession
(
    le_msg_SessionRef_t ipcSessionRef
)
//--------------------------------------------------------------------------------------------------
{
    return le_hashmap_Get(IpcSessionMapRef, &ipcSessionRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Running Process object.
 *
 * @return
 *      A pointer to the Running Process object.
 */
//--------------------------------------------------------------------------------------------------
static RunningProcess_t* CreateRunningProcess
(
    ProcessName_t*  procNameObjPtr,
    pid_t pid,
    le_msg_SessionRef_t ipcSessionRef
)
//--------------------------------------------------------------------------------------------------
{
    RunningProcess_t* objPtr = le_mem_ForceAlloc(RunningProcessPoolRef);

    objPtr->logSessionList = LE_DLS_LIST_INIT;
    objPtr->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&procNameObjPtr->runningProcessesList, &objPtr->link);
    objPtr->procNameObjPtr = procNameObjPtr;

    objPtr->pid = pid;
    objPtr->ipcSessionRef = ipcSessionRef;
//    objPtr->sharedMemAddr = NULL;   // TODO: Implement shared memory.

    le_hashmap_Put(ProcessIdMapRef, &objPtr->pid, objPtr);
    le_hashmap_Put(IpcSessionMapRef, &objPtr->ipcSessionRef, objPtr);

    return objPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Search a given Running Process's list of log sessions.
 *
 * @return
 *      A pointer to the Log Session object on success.
 *      NULL if the component name could not be found.
 */
//--------------------------------------------------------------------------------------------------
static LogSession_t* FindLogSession
(
    const RunningProcess_t* procPtr,
    const char* componentNameStr
)
//--------------------------------------------------------------------------------------------------
{
    // Search for the keyword in the list.
    le_dls_Link_t* linkPtr = le_dls_Peek(&procPtr->logSessionList);

    while (linkPtr != NULL)
    {
        LogSession_t* sessionPtr = CONTAINER_OF(linkPtr, LogSession_t, link);

        if (strcmp(sessionPtr->componentName, componentNameStr) == 0)
        {
            return sessionPtr;
        }

        // Get the next component.
        linkPtr = le_dls_PeekNext(&procPtr->logSessionList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Log Session object for a given Running Process object.
 *
 * @return
 *      A pointer to the Log Session object.
 */
//--------------------------------------------------------------------------------------------------
static LogSession_t* CreateLogSession
(
    RunningProcess_t* runningProcPtr,
    const char* componentNamePtr
)
//--------------------------------------------------------------------------------------------------
{
    LogSession_t* objPtr = le_mem_ForceAlloc(LogSessionPoolRef);

    if (   le_utf8_Copy(objPtr->componentName,
                        componentNamePtr,
                        sizeof(objPtr->componentName),
                        NULL )
        != LE_OK)
    {
        LE_WARN("Component name '%s' truncated to '%s'.", componentNamePtr, objPtr->componentName);
    }

    objPtr->level = -1;     // Indicates unknown state.
    objPtr->traceList = LE_DLS_LIST_INIT;
    // TODO: implement shared memory.

    objPtr->link = LE_DLS_LINK_INIT;
    le_dls_Queue(&runningProcPtr->logSessionList, &objPtr->link);

    return objPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Search a given Log Session's list of Traces.
 *
 * @return
 *      A pointer to the Trace object on success.
 *      NULL if the trace keyword could not be found.
 */
//--------------------------------------------------------------------------------------------------
static Trace_t* FindTrace
(
    const LogSession_t* logSessionPtr,
    const char* keywordStr
)
//--------------------------------------------------------------------------------------------------
{
    // Search for the keyword in the list.
    le_dls_Link_t* linkPtr = le_dls_Peek(&logSessionPtr->traceList);

    while (linkPtr != NULL)
    {
        Trace_t* traceObjPtr = CONTAINER_OF(linkPtr, Trace_t, link);

        if (strcmp(traceObjPtr->name, keywordStr) == 0)
        {
            return traceObjPtr;
        }

        linkPtr = le_dls_PeekNext(&logSessionPtr->traceList, linkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a Trace object for a given Log Session object.
 *
 * @return
 *      A pointer to the Trace object.
 */
//--------------------------------------------------------------------------------------------------
static Trace_t* CreateTrace
(
    LogSession_t* logSessionPtr,
    const char* keywordStr
)
//--------------------------------------------------------------------------------------------------
{
    Trace_t* objPtr = le_mem_ForceAlloc(TracePoolRef);

    if (   le_utf8_Copy(objPtr->name,
                        keywordStr,
                        sizeof(objPtr->name),
                        NULL )
        != LE_OK)
    {
        LE_WARN("Keyword '%s' truncated to '%s'.", keywordStr, objPtr->name);
    }

    objPtr->link = LE_DLS_LINK_INIT;
    objPtr->isEnabled = true;
    // TODO: implement shared memory.

    le_dls_Stack(&logSessionPtr->traceList, &objPtr->link);

    return objPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Converts a string into a process ID.
 *
 * @return  The PID, or -1 on error.
 **/
//--------------------------------------------------------------------------------------------------
static pid_t StringToPid
(
    const char* pidStr
)
//--------------------------------------------------------------------------------------------------
{
    char* endPtr;
    errno = 0;
    pid_t pid = strtol(pidStr, &endPtr, 10);
    if ((errno == ERANGE) || *endPtr != '\0')
    {
        pid = -1;
    }

    return pid;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parses a command packet, received from the command tool, to get the command code, process name,
 * component name and the command data.  The buffer sizes for the processName and componentName
 * must be LIMIT_MAX_PROCESS_NAME_BYTES and LIMIT_MAX_COMPONENT_NAME_SIZE, respectively.
 *
 * @return
 *      true if success.
 *      false if the packet was not formated correctly.
 */
//--------------------------------------------------------------------------------------------------
static bool ParseCmdPacket
(
    const char* cmdPacketPtr,   ///< [IN] The command packet.
    char* cmdPtr,               ///< [OUT] Ptr to where the command code should be copied.
                                ///        Can be NULL if not needed.
    char* processNamePtr,       ///< [OUT] Ptr to buffer to copy the process name into.
                                ///        Can be NULL if not needed.
    char* componentNamePtr,     ///< [OUT] Pointer buffer to copy the component name into.
                                ///        Can be NULL if not needed.
    const char** cmdDataPtrPtr  ///< [OUT] Set to point to the optional CommandData part at the end.
                                ///        Can be NULL if not needed.
)
{
    const char* packetPtr = cmdPacketPtr;
    size_t numBytes = 0;

    char commandCode = *cmdPacketPtr;

    // Check that the command code is there.
    if (commandCode == '\0')
    {
        LE_ERROR("Command byte missing from log command message '%s'.", cmdPacketPtr);
        return false;
    }

    // Get the command code.
    if (cmdPtr)
    {
        *cmdPtr = *packetPtr;
    }
    packetPtr++;

    // The "list" command has no parameters.
    if (commandCode == LOG_CMD_LIST_COMPONENTS)
    {
        return true;
    }

    // Get the process name.
    if (processNamePtr)
    {
        le_result_t result = le_utf8_CopyUpToSubStr(processNamePtr,
                                                    packetPtr,
                                                    "/",
                                                    LIMIT_MAX_PROCESS_NAME_BYTES,
                                                    &numBytes);
        if (result != LE_OK)
        {
            LE_ERROR("Process name too long in log command message '%s' (%s).",
                     cmdPacketPtr,
                     LE_RESULT_TXT(result));

            // Skip forward to '/' terminator.
            while ((packetPtr[numBytes] != '/') && (packetPtr[numBytes] != '\0'))
            {
                numBytes++;
            }
        }

        packetPtr += numBytes;
    }
    else
    {
        packetPtr = strchr(packetPtr, '/');

        if (!packetPtr)
        {
            LE_ERROR("No slash after process name in log command message '%s'.", cmdPacketPtr);
            return false;
        }
    }

    if (commandCode == LOG_CMD_FORGET_PROCESS)
    {
        // The forget process command has only a process name argument
        // (terminated by '/' for consistency with other commands).
        return true;
    }

    // Skip the '/' char.
    if (*packetPtr != '/')
    {
        LE_ERROR("Missing slash in log command message '%s'.", cmdPacketPtr);
        return false;
    }
    packetPtr += 1;
    if (*packetPtr == '\0')
    {
        LE_ERROR("Early terminator in log command message '%s'.", cmdPacketPtr);
        return false;
    }

    // Get the component name.
    if (componentNamePtr)
    {
        le_result_t result = le_utf8_CopyUpToSubStr(componentNamePtr,
                                                    packetPtr,
                                                    "/",
                                                    LIMIT_MAX_COMPONENT_NAME_BYTES,
                                                    &numBytes);
        if (result != LE_OK)
        {
            LE_ERROR("Component too long in log command message '%s' (%s).",
                     cmdPacketPtr,
                     LE_RESULT_TXT(result));
            LE_ERROR("Component name was being extracted from substring '%s'.", packetPtr);

            // Skip forward to '/' terminator.
            while ((packetPtr[numBytes] != '/') && (packetPtr[numBytes] != '\0'))
            {
                numBytes++;
            }
        }

        packetPtr += numBytes;
    }
    else
    {
        packetPtr = strchr(packetPtr, '/');

        if (!packetPtr)
        {
            LE_ERROR("No slash after component name in log command message '%s'.", cmdPacketPtr);
            return false;
        }
    }

    // Skip the '/' char.
    if (*packetPtr != '/')
    {
        LE_ERROR("Missing slash in log command message '%s'.", cmdPacketPtr);
        return false;
    }
    packetPtr += 1;
    if (*packetPtr == '\0')
    {
        LE_ERROR("Early terminator in log command message '%s'.", cmdPacketPtr);
        return false;
    }

    // Get the command data.
    if (cmdDataPtrPtr)
    {
        *cmdDataPtrPtr = packetPtr;
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends a message to a log control tool.
 **/
//--------------------------------------------------------------------------------------------------
static void SendToLogTool
(
    le_msg_SessionRef_t ipcSessionRef,
    const char* messageStr
)
//--------------------------------------------------------------------------------------------------
{
    le_result_t result;

    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(ipcSessionRef);

    char* payloadPtr = le_msg_GetPayloadPtr(msgRef);

    result = le_utf8_Copy(payloadPtr, messageStr, le_msg_GetMaxPayloadSize(msgRef), NULL);
    if (result == LE_OVERFLOW)
    {
        LE_WARN("Message truncated.");
    }

    le_msg_Send(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends a client an update to its log session settings.
 **/
//--------------------------------------------------------------------------------------------------
static void UpdateClientSessionSettings
(
    RunningProcess_t* runningProcObjPtr,
    LogSession_t* logSessionPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t msgRef;
    char* payloadPtr;
    size_t maxSize;
    size_t byteCount;

    // First send the level update, if it's not -1 (default).
    if (logSessionPtr->level != (le_log_Level_t)-1)
    {
        msgRef = le_msg_CreateMsg(runningProcObjPtr->ipcSessionRef);
        payloadPtr = le_msg_GetPayloadPtr(msgRef);
        maxSize = le_msg_GetMaxPayloadSize(msgRef);

        byteCount = snprintf(payloadPtr,
                             maxSize,
                             "%c%s/%s",
                             LOG_CMD_SET_LEVEL,
                             logSessionPtr->componentName,
                             GetLevelString(logSessionPtr->level));

        if (byteCount >= maxSize)
        {
            LE_CRIT("Message too long (%zu bytes) to send to component '%s' in process '%s' (pid %d).",
                    byteCount,
                    logSessionPtr->componentName,
                    runningProcObjPtr->procNameObjPtr->name,
                    runningProcObjPtr->pid);
            le_msg_ReleaseMsg(msgRef);
        }
        else
        {
            le_msg_Send(msgRef);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends a client an update to one of its trace settings.
 **/
//--------------------------------------------------------------------------------------------------
static void UpdateClientTraceSetting
(
    RunningProcess_t* runningProcObjPtr,
    LogSession_t* logSessionPtr,
    Trace_t* traceObjPtr
)
//--------------------------------------------------------------------------------------------------
{
    // First send the level update.
    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(runningProcObjPtr->ipcSessionRef);
    char* payloadPtr = le_msg_GetPayloadPtr(msgRef);
    size_t maxSize = le_msg_GetMaxPayloadSize(msgRef);

    char commandChar;

    if (traceObjPtr->isEnabled)
    {
        commandChar = LOG_CMD_ENABLE_TRACE;
    }
    else
    {
        commandChar = LOG_CMD_DISABLE_TRACE;
    }

    size_t byteCount = snprintf(payloadPtr,
                                maxSize,
                                "%c%s/%s",
                                commandChar,
                                logSessionPtr->componentName,
                                traceObjPtr->name);

    if (byteCount >= maxSize)
    {
        LE_CRIT("Message too long (%zu bytes) to send to component '%s' in process '%s' (pid %d).",
                byteCount,
                logSessionPtr->componentName,
                runningProcObjPtr->procNameObjPtr->name,
                runningProcObjPtr->pid);
        le_msg_ReleaseMsg(msgRef);
    }
    else
    {
        le_msg_Send(msgRef);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Applies settings from a Component Name object to a Log Session.
 **/
//--------------------------------------------------------------------------------------------------
static void CopyComponentSettings
(
    RunningProcess_t*   runningProcObjPtr,
    LogSession_t*       logSessionPtr,
    ComponentName_t*    compNameObjPtr
)
{
    logSessionPtr->level = compNameObjPtr->level;

    UpdateClientSessionSettings(runningProcObjPtr, logSessionPtr);

    // For all enabled traces for the component name.
    le_dls_Link_t* linkPtr = le_dls_Peek(&compNameObjPtr->enabledTracesList);
    while (linkPtr != NULL)
    {
        TraceName_t* traceNameObjPtr = CONTAINER_OF(linkPtr, TraceName_t, link);

        // Create an enabled keyword object for the log session.
        Trace_t* traceObjPtr = CreateTrace(logSessionPtr, traceNameObjPtr->name);
        traceObjPtr->isEnabled = true;

        // Notify the client that the keyword is enabled.
        UpdateClientTraceSetting(runningProcObjPtr, logSessionPtr, traceObjPtr);

        linkPtr = le_dls_PeekNext(&compNameObjPtr->enabledTracesList, linkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Updates the component settings for logSessionPtr in the runningProcObjPtr by copying the settings
 * from the component named componentName in procNameObjPtr, if it exists.  If the component named
 * componentName does not exist the component settings are copied from the wild card component.
 *
 * @return
 *      true if the setting was successfully updated.
 *      false otherwise.
 */
//--------------------------------------------------------------------------------------------------
static bool UpdateComponentSettings
(
    RunningProcess_t*   runningProcObjPtr,
    LogSession_t*       logSessionPtr,
    ProcessName_t*      procNameObjPtr,
    const char*         componentName
)
{
    // Check if this component name is already known to us and get a pointer to the
    // Component Name object for it, if it is known to us.
    ComponentName_t* compNameObjPtr = FindComponentName(procNameObjPtr, componentName);

    if (compNameObjPtr != NULL)
    {
        CopyComponentSettings(runningProcObjPtr, logSessionPtr, compNameObjPtr);
        return true;
    }

    // Try using the wild card instead.
    compNameObjPtr = FindComponentName(procNameObjPtr, "*");

    if (compNameObjPtr != NULL)
    {
        CopyComponentSettings(runningProcObjPtr, logSessionPtr, compNameObjPtr);
        return true;
    }

    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Updates the process and component settings for logSessionPtr in the runningProcObjPtr by copying
 * the settings from the procNameObjPtr/componentName.  If procNameObjPtr is NULL then the wild
 * card process is used.  If the componentName does not exist then the wild card component is used.
 */
//--------------------------------------------------------------------------------------------------
static void UpdateProcCompSettings
(
    RunningProcess_t*   runningProcObjPtr,
    LogSession_t*       logSessionPtr,
    ProcessName_t*      procNameObjPtr,
    const char*         componentName
)
{
    if (procNameObjPtr != NULL)
    {
        if (UpdateComponentSettings(runningProcObjPtr, logSessionPtr, procNameObjPtr, componentName))
        {
            return;
        }
    }

    // Get the wild card process.
    procNameObjPtr = FindProcessName("*");

    if (procNameObjPtr != NULL)
    {
        if (UpdateComponentSettings(runningProcObjPtr, logSessionPtr, procNameObjPtr, componentName))
        {
            return;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds the process/component to our registry if it is not already there.
 *
 * Searchs the configuration commands list to see if we have any commands for this process/component
 * and sends those commands to it.
 */
//--------------------------------------------------------------------------------------------------
static void RegComponent
(
    const char* processName,
    const char* componentName,
    const char* pidStr,
    le_msg_SessionRef_t ipcSessionRef
)
{
    ProcessName_t* procNameObjPtr;
    RunningProcess_t* runningProcObjPtr;
    LogSession_t* logSessionPtr;

    // The "*" name is special and cannot be used.
    if (strcmp(processName, "*") == 0)
    {
        LE_WARN("Invalid process name: '%s'", processName);
        return;
    }

    if (strcmp(componentName, "*") == 0)
    {
        LE_WARN("Invalid process name: '%s'", componentName);
        return;
    }

    // Convert the PID string into a number.
    pid_t pid = StringToPid(pidStr);
    if (pid < 0)
    {
        LE_ERROR("Invalid PID '%s' in registration for '%s/%s'.",
                 pidStr,
                 processName,
                 componentName);
        return;
    }

    LE_DEBUG("Process named '%s' with pid %d registered component '%s'.",
             processName,
             pid,
             componentName);

    // Look up this process name in our internal data structures.
    procNameObjPtr = FindProcessName(processName);

    // If the process name is not yet known to us, add the log session details to our structures.
    if (procNameObjPtr == NULL)
    {
        procNameObjPtr = CreateProcessName(processName);

        // The PID shouldn't be found associated with another process name.  If it is,
        // then either a client process is sending bogus registration requests or there's
        // a bug in the Log Control Daemon.
        runningProcObjPtr = FindProcessByPid(pid);
        if (runningProcObjPtr != NULL)
        {
            LE_CRIT("PID %d found with different process name (%s) than expected (%s).",
                    pid,
                    runningProcObjPtr->procNameObjPtr->name,
                    processName);
            return;
        }

        // The IPC session ID also shouldn't be found associated with another process name.
        // If it is, then either a client process is sending bogus registration requests or there's
        // a bug in the Log Control Daemon.
        runningProcObjPtr = FindProcessByIpcSession(ipcSessionRef);
        if (runningProcObjPtr != NULL)
        {
            LE_CRIT("IPC session %p found with different process name (%s) than expected (%s).",
                    ipcSessionRef,
                    runningProcObjPtr->procNameObjPtr->name,
                    processName);
            return;
        }

        // Add the running process and the active log session to our structures.
        runningProcObjPtr = CreateRunningProcess(procNameObjPtr, pid, ipcSessionRef);
        logSessionPtr = CreateLogSession(runningProcObjPtr, componentName);

        UpdateProcCompSettings(runningProcObjPtr, logSessionPtr, NULL, componentName);
    }
    // If the process name was already known to us,
    else
    {
        // Look up this process ID in our internal data structures.
        runningProcObjPtr = FindProcessByPid(pid);

        // If the process ID was found,
        if (runningProcObjPtr != NULL)
        {
            // The process ID should already be associated with this process name.
            // If not, then either a client process is sending bogus registration requests
            // or there's a bug in the Log Control Daemon (probably in this very function).
            // This can also happen if the process exec's another legato process.
            if (runningProcObjPtr->procNameObjPtr != procNameObjPtr)
            {
                LE_WARN("Process with PID %d associated with unexpected process name '%s'.",
                        pid,
                        runningProcObjPtr->procNameObjPtr->name);
                return;
            }

            // Check for a duplicate log session registration.
            if (FindLogSession(runningProcObjPtr, componentName) != NULL)
            {
                LE_WARN("Duplicate registration of '%s/%s' by PID %d.",
                         processName,
                         componentName,
                         pid);
                return;
            }
        }
        // If the process ID was not found,
        else
        {
            // Add the running process to our structures.
            runningProcObjPtr = CreateRunningProcess(procNameObjPtr, pid, ipcSessionRef);
        }

        // Create a log session object in the running process's list of log sessions.
        logSessionPtr = CreateLogSession(runningProcObjPtr, componentName);

        UpdateProcCompSettings(runningProcObjPtr, logSessionPtr, procNameObjPtr, componentName);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle the closing of a client IPC session, which signals the death of a process.
 **/
//--------------------------------------------------------------------------------------------------
static void ClientIpcSessionClosed
(
    le_msg_SessionRef_t ipcSessionRef,
    void* contextPtr    // not used.
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* linkPtr;

    RunningProcess_t* runningProcObjPtr = FindProcessByIpcSession(ipcSessionRef);

    if (runningProcObjPtr == NULL)
    {
        // This can happen if a client connects, but gets killed before it registers any
        // log sessions.
        LE_DEBUG("Unknown IPC session (%p) closed.", ipcSessionRef);
        return;
    }

    ProcessName_t* procNameObjPtr = runningProcObjPtr->procNameObjPtr;

    LE_DEBUG("Process named '%s' with pid %d disconnected.",
             procNameObjPtr->name,
             runningProcObjPtr->pid);

    // Remove the process from the PID and IPC Session hash maps.
    le_hashmap_Remove(ProcessIdMapRef, &runningProcObjPtr->pid);
    le_hashmap_Remove(IpcSessionMapRef, &ipcSessionRef);

    // Delete all the log sessions for this process.

    LE_CRIT_IF(le_dls_IsEmpty(&runningProcObjPtr->logSessionList),
               "Empty session list for process '%s' with pid %d!",
               procNameObjPtr->name,
               runningProcObjPtr->pid);

    while ((linkPtr = le_dls_Pop(&runningProcObjPtr->logSessionList)) != NULL)
    {
        LogSession_t* logSessionPtr = CONTAINER_OF(linkPtr, LogSession_t, link);

        // Delete all the traces for this log session.
        while ((linkPtr = le_dls_Pop(&logSessionPtr->traceList)) != NULL)
        {
            le_mem_Release(CONTAINER_OF(linkPtr, Trace_t, link));
        }

        le_mem_Release(logSessionPtr);
    }

    // Remove the process from the list of processes with this name.
    le_dls_Remove(&procNameObjPtr->runningProcessesList,
                  &runningProcObjPtr->link);
    runningProcObjPtr->procNameObjPtr = NULL;

    // Delete the Running Process object.
    le_mem_Release(runningProcObjPtr);

    // If the Process Name object now has no other running processes and no component names
    // associated with it, forget it.
    if (   le_dls_IsEmpty(&procNameObjPtr->runningProcessesList)
        && le_dls_IsEmpty(&procNameObjPtr->componentNameList) )
    {
        DeleteProcessName(procNameObjPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets log settings for a specific running process.
 **/
//--------------------------------------------------------------------------------------------------
static void SetForRunningProcess
(
    RunningProcess_t* runningProcObjPtr,
    const char* componentName,
    le_log_Level_t* levelPtr                ///< [IN] Ptr log level, or NULL if not being set.
)
//--------------------------------------------------------------------------------------------------
{
    // If the setting applies to all log sessions in the process,
    if (strcmp(componentName, "*") == 0)
    {
        // Iterate over the list of log sessions in the process and update them all.
        le_dls_Link_t* linkPtr = le_dls_Peek(&runningProcObjPtr->logSessionList);
        while (linkPtr != NULL)
        {
            LogSession_t* logSessionObjPtr = CONTAINER_OF(linkPtr, LogSession_t, link);

            if (levelPtr != NULL)
            {
                logSessionObjPtr->level = *levelPtr;
            }

            UpdateClientSessionSettings(runningProcObjPtr, logSessionObjPtr);

            linkPtr = le_dls_PeekNext(&runningProcObjPtr->logSessionList, linkPtr);
        }
    }
    // If the setting applies to a specific log session within the process,
    else
    {
        // Find that session and apply the setting to it.
        LogSession_t* logSessionObjPtr = FindLogSession(runningProcObjPtr, componentName);
        if (logSessionObjPtr != NULL)
        {
            if (levelPtr != NULL)
            {
                logSessionObjPtr->level = *levelPtr;
            }

            UpdateClientSessionSettings(runningProcObjPtr, logSessionObjPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets log settings for a specific process ID.
 **/
//--------------------------------------------------------------------------------------------------
static void SetByPid
(
    pid_t pid,
    const char* componentName,
    le_log_Level_t* levelPtr,               ///< [IN] Ptr log level, or NULL if not being set.
    le_msg_SessionRef_t toolIpcSessionRef   ///< [IN] Reference to log control tool's IPC session.
)
//--------------------------------------------------------------------------------------------------
{
    RunningProcess_t* runningProcObjPtr = le_hashmap_Get(ProcessIdMapRef, &pid);
    if (runningProcObjPtr == NULL)
    {
        char message[128];

        snprintf(message, sizeof(message), "***ERROR: PID %d not found.", pid);
        LE_WARN("%s", message);
        SendToLogTool(toolIpcSessionRef, message);
    }
    else
    {
        SetForRunningProcess(runningProcObjPtr, componentName, levelPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the log level for all processes that already exist in the Log Control Daemon's data
 * structures.
 **/
//--------------------------------------------------------------------------------------------------
static void SetForAllProcesses
(
    const char* componentName,
    le_log_Level_t* levelPtr                ///< [IN] Ptr log level, or NULL if not being set.
)
//--------------------------------------------------------------------------------------------------
{
    // Create the wild process if it does not already exist.
    ProcessName_t* wildProcPtr = FindProcessName("*");

    if (wildProcPtr == NULL)
    {
        wildProcPtr = CreateProcessName("*");
    }

    // Create the component in the wild process if it doesn't already exist.
    ComponentName_t* compPtr = FindComponentName(wildProcPtr, componentName);

    if (compPtr == NULL)
    {
        CreateComponentName(wildProcPtr, componentName);
    }

    // Update all the processes.
    const ProcessName_t* procNameObjPtr;
    le_hashmap_It_Ref_t iteratorRef = le_hashmap_GetIterator(ProcessNameMapRef);
    while (le_hashmap_NextNode(iteratorRef) == LE_OK)
    {
        procNameObjPtr = le_hashmap_GetValue(iteratorRef);

        // If the setting applies to all log sessions in the process.
        if (strcmp(componentName, "*") == 0)
        {
            // Iterate over the list of component names and update them all.
            le_dls_Link_t* linkPtr = le_dls_Peek(&procNameObjPtr->componentNameList);

            while (linkPtr != NULL)
            {
                ComponentName_t* compNameObjPtr = CONTAINER_OF(linkPtr, ComponentName_t, link);

                if (levelPtr != NULL)
                {
                    compNameObjPtr->level = *levelPtr;
                }

                linkPtr = le_dls_PeekNext(&procNameObjPtr->componentNameList, linkPtr);
            }
        }
        // If the setting applies to a single component,
        else
        {
            // If that component name is already listed under this process name, then update it.
            ComponentName_t* compNameObjPtr = FindComponentName(procNameObjPtr, componentName);
            if (compNameObjPtr != NULL)
            {
                if (levelPtr != NULL)
                {
                    compNameObjPtr->level = *levelPtr;
                }
            }
        }

        // Now update all the actual running processes that share this process name.
        le_dls_Link_t* linkPtr = le_dls_Peek(&procNameObjPtr->runningProcessesList);
        while (linkPtr != NULL)
        {
            RunningProcess_t* runningProcObjPtr = CONTAINER_OF(linkPtr, RunningProcess_t, link);

            SetForRunningProcess(runningProcObjPtr, componentName, levelPtr);

            linkPtr = le_dls_PeekNext(&procNameObjPtr->runningProcessesList, linkPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * If there's a running process for a given Process Name object, make sure there's a persistent
 * Component Name object attached to the Process Name object for each Log Session
 * attached to the running process(es).
 */
//--------------------------------------------------------------------------------------------------
static void CreateComponentNamesForRunningSessions
(
    ProcessName_t* procNameObjPtr
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Link_t* runningProcLinkPtr = le_dls_Peek(&procNameObjPtr->runningProcessesList);

    while (runningProcLinkPtr != NULL)
    {
        RunningProcess_t* runningProcObjPtr;
        runningProcObjPtr = CONTAINER_OF(runningProcLinkPtr, RunningProcess_t, link);

        le_dls_Link_t* sessionLinkPtr = le_dls_Peek(&runningProcObjPtr->logSessionList);

        while (sessionLinkPtr != NULL)
        {
            LogSession_t* sessionObjPtr = CONTAINER_OF(sessionLinkPtr, LogSession_t, link);

            if (FindComponentName(procNameObjPtr, sessionObjPtr->componentName) == NULL)
            {
                CreateComponentName(procNameObjPtr, sessionObjPtr->componentName);
            }

            sessionLinkPtr = le_dls_PeekNext(&runningProcObjPtr->logSessionList, sessionLinkPtr);
        }

        runningProcLinkPtr = le_dls_PeekNext(&procNameObjPtr->runningProcessesList,
                                             runningProcLinkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the log settings for all processes that share the same name.  This setting
 * will survive as long as the Log Control Daemon continues to run or until it is changed
 * at the request of the log control tool.
 **/
//--------------------------------------------------------------------------------------------------
static void SetByProcessName
(
    const char* processName,
    const char* componentName,
    le_log_Level_t* levelPtr                ///< [IN] Ptr log level, or NULL if not being set.
)
//--------------------------------------------------------------------------------------------------
{
    ProcessName_t* procNameObjPtr = FindProcessName(processName);
    if (procNameObjPtr == NULL)
    {
        procNameObjPtr = CreateProcessName(processName);
    }

    // If the setting applies to all log sessions in the process.
    if (strcmp(componentName, "*") == 0)
    {
        // Create a wild component in this process if it doesn't already exist.
        ComponentName_t* compPtr = FindComponentName(procNameObjPtr, componentName);

        if (compPtr == NULL)
        {
            CreateComponentName(procNameObjPtr, componentName);
        }

        // If there's a running process with this name, make sure there's a persistent
        // Component Name object attached to the Process Name object for each Log Session
        // attached to the running process(es).
        CreateComponentNamesForRunningSessions(procNameObjPtr);

        // Iterate over the list of component names and update them all.
        le_dls_Link_t* linkPtr = le_dls_Peek(&procNameObjPtr->componentNameList);

        while (linkPtr != NULL)
        {
            ComponentName_t* compNameObjPtr = CONTAINER_OF(linkPtr, ComponentName_t, link);

            if (levelPtr != NULL)
            {
                compNameObjPtr->level = *levelPtr;
            }

            linkPtr = le_dls_PeekNext(&procNameObjPtr->componentNameList, linkPtr);
        }
    }
    // If the setting applies to a single component,
    else
    {
        // If that component name is already listed under this process name, then update it.
        // Otherwise, create a new one and store the settings in there.
        ComponentName_t* compNameObjPtr = FindComponentName(procNameObjPtr, componentName);
        if (compNameObjPtr == NULL)
        {
            compNameObjPtr = CreateComponentName(procNameObjPtr, componentName);
        }
        if (levelPtr != NULL)
        {
            compNameObjPtr->level = *levelPtr;
        }
    }

    // Now update all the actual running processes that share this process name.
    le_dls_Link_t* linkPtr = le_dls_Peek(&procNameObjPtr->runningProcessesList);
    while (linkPtr != NULL)
    {
        RunningProcess_t* runningProcObjPtr = CONTAINER_OF(linkPtr, RunningProcess_t, link);

        SetForRunningProcess(runningProcObjPtr, componentName, levelPtr);

        linkPtr = le_dls_PeekNext(&procNameObjPtr->runningProcessesList, linkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Apply log settings.
 **/
//--------------------------------------------------------------------------------------------------
static void ApplySettings
(
    const char* processName,
    const char* componentName,
    le_log_Level_t* levelPtr,               ///< [IN] Ptr log level, or NULL if not being set.
    le_msg_SessionRef_t toolIpcSessionRef   ///< [IN] Reference to log control tool's IPC session.

)
//--------------------------------------------------------------------------------------------------
{
    // If a PID was used to specify that the settings apply to a specific running process,
    pid_t pid = StringToPid(processName);
    if (pid > 0)
    {
        SetByPid(pid, componentName, levelPtr, toolIpcSessionRef);
    }
    // If the process name is "*",
    else if (strcmp(processName, "*") == 0)
    {
        // This setting applies to ALL PROCESSES.
        SetForAllProcesses(componentName, levelPtr);
    }
    else
    {
        // This setting applies to processes sharing a specific name.
        SetByProcessName(processName, componentName, levelPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the log level for a given process/component.
 **/
//--------------------------------------------------------------------------------------------------
static void SetLevel
(
    const char* processName,
    const char* componentName,
    const char* levelStr,
    le_msg_SessionRef_t toolIpcSessionRef
)
//--------------------------------------------------------------------------------------------------
{
    char message[256];

    // Parse the command data payload to get the level setting.
    le_log_Level_t level = log_StrToSeverityLevel(levelStr);

    if (level == (le_log_Level_t)-1)
    {
        snprintf(message, sizeof(message), "***ERROR: Invalid log level '%s'.", levelStr);
        LE_WARN("%s", message);
        SendToLogTool(toolIpcSessionRef, message);
    }
    else
    {
        ApplySettings(processName, componentName, &level, toolIpcSessionRef);
        snprintf(message,
                 sizeof(message),
                 "Set filtering level for '%s/%s' to '%s'.",
                 processName,
                 componentName,
                 levelStr);
        SendToLogTool(toolIpcSessionRef, message);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets (enables or disables) a trace for a specific component name.
 **/
//--------------------------------------------------------------------------------------------------
static void SetTraceForComponentName
(
    ComponentName_t* compNameObjPtr,
    const char* keyword,
    bool isEnabled
)
//--------------------------------------------------------------------------------------------------
{
    TraceName_t* traceNameObjPtr = FindTraceName(compNameObjPtr, keyword);

    if (traceNameObjPtr == NULL)
    {
        if (isEnabled)
        {
            (void)CreateTraceName(compNameObjPtr, keyword);
        }
    }
    else
    {
        if (!isEnabled)
        {
            le_dls_Remove(&compNameObjPtr->enabledTracesList, &traceNameObjPtr->link);
            le_mem_Release(traceNameObjPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets (enables or disables) a trace for a specific log session.
 **/
//--------------------------------------------------------------------------------------------------
static void SetTraceForLogSession
(
    RunningProcess_t* runningProcObjPtr,
    LogSession_t* logSessionPtr,
    const char* keyword,
    bool isEnabled
)
//--------------------------------------------------------------------------------------------------
{
    Trace_t* tracePtr = FindTrace(logSessionPtr, keyword);

    if (tracePtr == NULL)
    {
        tracePtr = CreateTrace(logSessionPtr, keyword);
    }

    tracePtr->isEnabled = isEnabled;

    UpdateClientTraceSetting(runningProcObjPtr, logSessionPtr, tracePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets (enables or disables) a trace for a specific running process.
 **/
//--------------------------------------------------------------------------------------------------
static void SetTraceForRunningProcess
(
    RunningProcess_t* runningProcObjPtr,
    const char* componentName,
    const char* keyword,
    bool isEnabled
)
//--------------------------------------------------------------------------------------------------
{
    // If the setting applies to all log sessions in the process,
    if (strcmp(componentName, "*") == 0)
    {
        // Iterate over the list of log sessions in the process and update them all.
        le_dls_Link_t* linkPtr = le_dls_Peek(&runningProcObjPtr->logSessionList);
        while (linkPtr != NULL)
        {
            LogSession_t* logSessionObjPtr = CONTAINER_OF(linkPtr, LogSession_t, link);

            SetTraceForLogSession(runningProcObjPtr, logSessionObjPtr, keyword, isEnabled);

            linkPtr = le_dls_PeekNext(&runningProcObjPtr->logSessionList, linkPtr);
        }
    }
    // If the setting applies to a specific log session within the process,
    else
    {
        // Find that session and apply the setting to it.
        LogSession_t* logSessionObjPtr = FindLogSession(runningProcObjPtr, componentName);
        if (logSessionObjPtr != NULL)
        {
            SetTraceForLogSession(runningProcObjPtr, logSessionObjPtr, keyword, isEnabled);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets (enables or disables) a trace for a specific running process identified by process ID.
 **/
//--------------------------------------------------------------------------------------------------
static void SetTraceByPid
(
    pid_t pid,
    const char* componentName,
    const char* keyword,
    bool isEnabled,
    le_msg_SessionRef_t toolIpcSessionRef
)
//--------------------------------------------------------------------------------------------------
{
    RunningProcess_t* runningProcObjPtr = le_hashmap_Get(ProcessIdMapRef, &pid);
    if (runningProcObjPtr == NULL)
    {
        char message[128];

        snprintf(message, sizeof(message), "***ERROR: PID %d not found.", pid);
        LE_WARN("%s", message);
        SendToLogTool(toolIpcSessionRef, message);
    }
    else
    {
        SetTraceForRunningProcess(runningProcObjPtr, componentName, keyword, isEnabled);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets (enables or disables) a trace for ALL processes.
 **/
//--------------------------------------------------------------------------------------------------
static void SetTraceForAllProcesses
(
    const char* componentName,
    const char* keyword,
    bool isEnabled
)
//--------------------------------------------------------------------------------------------------
{
    // Create the wild process if it does not already exist.
    ProcessName_t* wildProcPtr = FindProcessName("*");

    if (wildProcPtr == NULL)
    {
        wildProcPtr = CreateProcessName("*");
    }

    // Create the component in the wild process if it doesn't already exist.
    ComponentName_t* compPtr = FindComponentName(wildProcPtr, componentName);

    if (compPtr == NULL)
    {
        CreateComponentName(wildProcPtr, componentName);
    }

    // Update all the processes.
    const ProcessName_t* procNameObjPtr;
    le_hashmap_It_Ref_t iteratorRef = le_hashmap_GetIterator(ProcessNameMapRef);
    while (le_hashmap_NextNode(iteratorRef) == LE_OK)
    {
        procNameObjPtr = le_hashmap_GetValue(iteratorRef);

        // If the setting applies to all log sessions in the process.
        if (strcmp(componentName, "*") == 0)
        {
            // Iterate over the list of component names and update them all.
            le_dls_Link_t* linkPtr = le_dls_Peek(&procNameObjPtr->componentNameList);

            while (linkPtr != NULL)
            {
                ComponentName_t* compNameObjPtr = CONTAINER_OF(linkPtr, ComponentName_t, link);

                SetTraceForComponentName(compNameObjPtr, keyword, isEnabled);

                linkPtr = le_dls_PeekNext(&procNameObjPtr->componentNameList, linkPtr);
            }
        }
        // If the setting applies to a single component,
        else
        {
            // If that component name is already listed under this process name, then update it.
            ComponentName_t* compNameObjPtr = FindComponentName(procNameObjPtr, componentName);
            if (compNameObjPtr != NULL)
            {
                SetTraceForComponentName(compNameObjPtr, keyword, isEnabled);
            }
        }

        // Now update all the actual running processes that share this process name.
        le_dls_Link_t* linkPtr = le_dls_Peek(&procNameObjPtr->runningProcessesList);
        while (linkPtr != NULL)
        {
            RunningProcess_t* runningProcObjPtr = CONTAINER_OF(linkPtr, RunningProcess_t, link);

            SetTraceForRunningProcess(runningProcObjPtr, componentName, keyword, isEnabled);

            linkPtr = le_dls_PeekNext(&procNameObjPtr->runningProcessesList, linkPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets (enables or disables) a trace for all processes sharing a particular process name.
 **/
//--------------------------------------------------------------------------------------------------
static void SetTraceByProcessName
(
    const char* processName,
    const char* componentName,
    const char* keyword,
    bool isEnabled
)
//--------------------------------------------------------------------------------------------------
{
    ProcessName_t* procNameObjPtr = le_hashmap_Get(ProcessNameMapRef, processName);
    if (procNameObjPtr == NULL)
    {
        procNameObjPtr = CreateProcessName(processName);
    }

    // If the setting applies to all log sessions in the process.
    if (strcmp(componentName, "*") == 0)
    {
        // Create a wild component in this process if it doesn't already exist.
        ComponentName_t* compPtr = FindComponentName(procNameObjPtr, componentName);

        if (compPtr == NULL)
        {
            CreateComponentName(procNameObjPtr, componentName);
        }

        // Iterate over the list of component names and update them all.
        le_dls_Link_t* linkPtr = le_dls_Peek(&procNameObjPtr->componentNameList);

        while (linkPtr != NULL)
        {
            ComponentName_t* compNameObjPtr = CONTAINER_OF(linkPtr, ComponentName_t, link);

            SetTraceForComponentName(compNameObjPtr, keyword, isEnabled);

            linkPtr = le_dls_PeekNext(&procNameObjPtr->componentNameList, linkPtr);
        }
    }
    // If the setting applies to a single component,
    else
    {
        // If that component name is already listed under this process name, then update it.
        // Otherwise, create a new one and store the settings in there.
        ComponentName_t* compNameObjPtr = FindComponentName(procNameObjPtr, componentName);
        if (compNameObjPtr == NULL)
        {
            compNameObjPtr = CreateComponentName(procNameObjPtr, componentName);
        }

        SetTraceForComponentName(compNameObjPtr, keyword, isEnabled);
    }

    // Now update all the actual running processes that share this process name.
    le_dls_Link_t* linkPtr = le_dls_Peek(&procNameObjPtr->runningProcessesList);
    while (linkPtr != NULL)
    {
        RunningProcess_t* runningProcObjPtr = CONTAINER_OF(linkPtr, RunningProcess_t, link);

        SetTraceForRunningProcess(runningProcObjPtr, componentName, keyword, isEnabled);

        linkPtr = le_dls_PeekNext(&procNameObjPtr->runningProcessesList, linkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Enable/disable a trace.
 **/
//--------------------------------------------------------------------------------------------------
static void SetTrace
(
    const char* processName,
    const char* componentName,
    const char* keyword,
    bool isEnabled,
    le_msg_SessionRef_t toolIpcSessionRef
)
//--------------------------------------------------------------------------------------------------
{
    // If a PID was used to specify that the settings apply to a specific running process,
    pid_t pid = StringToPid(processName);
    if (pid > 0)
    {
        SetTraceByPid(pid, componentName, keyword, isEnabled, toolIpcSessionRef);
    }
    // If the process name is "*",
    else if (strcmp(processName, "*") == 0)
    {
        // This setting applies to ALL PROCESSES.
        SetTraceForAllProcesses(componentName, keyword, isEnabled);
    }
    else
    {
        // This setting applies to processes sharing a specific name.
        SetTraceByProcessName(processName, componentName, keyword, isEnabled);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends a message to the log tool containing a printable, null-terminated, UTF-8 string
 * containing the name of a process.
 **/
//--------------------------------------------------------------------------------------------------
static void SendProcessNameToLogTool
(
    const ProcessName_t* procNameObjPtr,
    le_msg_SessionRef_t ipcSessionRef
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(ipcSessionRef);

    char* payloadPtr = le_msg_GetPayloadPtr(msgRef);

    snprintf(payloadPtr, le_msg_GetMaxPayloadSize(msgRef), "%s", procNameObjPtr->name);

    le_msg_Send(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends messages to the log tool containing printable, null-terminated, UTF-8 strings
 * containing the name of a component and any traces that are enabled for it.
 **/
//--------------------------------------------------------------------------------------------------
static void SendTraceNameToLogTool
(
    const char* name,
    le_msg_SessionRef_t ipcSessionRef
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(ipcSessionRef);

    char* payloadPtr = le_msg_GetPayloadPtr(msgRef);

    snprintf(payloadPtr,
             le_msg_GetMaxPayloadSize(msgRef),
             "            tracing \"%s\"",
             name);

    le_msg_Send(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends a message to the log tool containing a printable, null-terminated, UTF-8 string containing
 * the name of a component and its associated log level.
 **/
//--------------------------------------------------------------------------------------------------
static void SendComponentInfoToLogTool
(
    const char* componentName,
    le_log_Level_t level,
    le_msg_SessionRef_t ipcSessionRef
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(ipcSessionRef);

    char* payloadPtr = le_msg_GetPayloadPtr(msgRef);

    snprintf(payloadPtr,
             le_msg_GetMaxPayloadSize(msgRef),
             "      /%s @ %s",
             componentName,
             GetLevelString(level));

    le_msg_Send(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends messages to the log tool containing printable, null-terminated, UTF-8 strings
 * containing the name of a component, its associated log level and any traces that are enabled for
 * it.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateComponentList
(
    const ComponentName_t* compNameObjPtr,
    le_msg_SessionRef_t ipcSessionRef
)
//--------------------------------------------------------------------------------------------------
{
    SendComponentInfoToLogTool(compNameObjPtr->name,
                               compNameObjPtr->level,
                               ipcSessionRef);

    le_dls_Link_t* linkPtr = le_dls_Peek(&compNameObjPtr->enabledTracesList);
    while (linkPtr != NULL)
    {
        TraceName_t* traceNameObjPtr = CONTAINER_OF(linkPtr, TraceName_t, link);

        SendTraceNameToLogTool(traceNameObjPtr->name, ipcSessionRef);

        linkPtr = le_dls_PeekNext(&compNameObjPtr->enabledTracesList, linkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends messages to the log tool containing printable, null-terminated, UTF-8 strings
 * containing a log session's component name, its log filter level, and any trace settings for it.
 **/
//--------------------------------------------------------------------------------------------------
static void GenerateLogSessionList
(
    const LogSession_t* logSessionObjPtr,
    le_msg_SessionRef_t ipcSessionRef
)
//--------------------------------------------------------------------------------------------------
{
    SendComponentInfoToLogTool(logSessionObjPtr->componentName,
                               logSessionObjPtr->level,
                               ipcSessionRef);

    le_dls_Link_t* linkPtr = le_dls_Peek(&logSessionObjPtr->traceList);
    while (linkPtr != NULL)
    {
        Trace_t* traceObjPtr = CONTAINER_OF(linkPtr, Trace_t, link);

        SendTraceNameToLogTool(traceObjPtr->name, ipcSessionRef);

        linkPtr = le_dls_PeekNext(&logSessionObjPtr->traceList, linkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends a message to the log tool containing a printable, null-terminated, UTF-8 string
 * containing the PID of a running process and all its log session information.
 **/
//--------------------------------------------------------------------------------------------------
static void SendRunningProcessInfoToLogTool
(
    const RunningProcess_t* runningProcObjPtr,
    le_msg_SessionRef_t ipcSessionRef
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_MessageRef_t msgRef = le_msg_CreateMsg(ipcSessionRef);

    char* payloadPtr = le_msg_GetPayloadPtr(msgRef);

    snprintf(payloadPtr,
             le_msg_GetMaxPayloadSize(msgRef),
             "  pid %d",
             runningProcObjPtr->pid);

    le_msg_Send(msgRef);

    le_dls_Link_t* linkPtr = le_dls_Peek(&runningProcObjPtr->logSessionList);
    while (linkPtr != NULL)
    {
        LogSession_t* logSessionObjPtr = CONTAINER_OF(linkPtr, LogSession_t, link);

        GenerateLogSessionList(logSessionObjPtr, ipcSessionRef);

        linkPtr = le_dls_PeekNext(&runningProcObjPtr->logSessionList, linkPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends the list of processes, components, and enabled trace keywords to the log control tool.
 *
 * @note    Sends one message for each line item in the list.  The messages are null-terminated,
 *          printable UTF-8 strings.
 */
//--------------------------------------------------------------------------------------------------
static void GenerateList
(
    le_msg_SessionRef_t ipcSessionRef   ///< [IN] Log control tool's current IPC session.
)
//--------------------------------------------------------------------------------------------------
{
    const ProcessName_t* procNameObjPtr;
    le_hashmap_It_Ref_t iteratorRef = le_hashmap_GetIterator(ProcessNameMapRef);
    while (le_hashmap_NextNode(iteratorRef) == LE_OK)
    {
        procNameObjPtr = le_hashmap_GetValue(iteratorRef);

        SendProcessNameToLogTool(procNameObjPtr, ipcSessionRef);

        le_dls_Link_t* linkPtr = le_dls_Peek(&procNameObjPtr->componentNameList);
        while (linkPtr != NULL)
        {
            ComponentName_t* compNameObjPtr = CONTAINER_OF(linkPtr, ComponentName_t, link);

            GenerateComponentList(compNameObjPtr, ipcSessionRef);

            linkPtr = le_dls_PeekNext(&procNameObjPtr->componentNameList, linkPtr);
        }

        linkPtr = le_dls_Peek(&procNameObjPtr->runningProcessesList);
        while (linkPtr != NULL)
        {
            RunningProcess_t* runningProcObjPtr = CONTAINER_OF(linkPtr, RunningProcess_t, link);

            SendRunningProcessInfoToLogTool(runningProcObjPtr, ipcSessionRef);

            linkPtr = le_dls_PeekNext(&procNameObjPtr->runningProcessesList, linkPtr);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Clears the settings for a given process name out of the data structures.
 *
 * @note    Won't remove settings for processes that are currently running.
 */
//--------------------------------------------------------------------------------------------------
static void ForgetProcess
(
    const char* processName,
    le_msg_SessionRef_t toolIpcSessionRef
)
//--------------------------------------------------------------------------------------------------
{
    char message[128];

    if (strcmp(processName, "*") == 0)
    {
        SendToLogTool(toolIpcSessionRef, "Wildcard not supported for removal of processes.");
        LE_ERROR("Wildcard not supported for removal of processes.");
        return;
    }

    ProcessName_t* procNameObjPtr = FindProcessName(processName);

    if (procNameObjPtr == NULL)
    {
        snprintf(message, sizeof(message), "***ERROR: Process '%s' not found.", processName);
        LE_WARN("%s", message);
        SendToLogTool(toolIpcSessionRef, message);
        return;
    }

    if (!le_dls_IsEmpty(&procNameObjPtr->runningProcessesList))
    {
        DeleteAllComponentNamesForProcessName(procNameObjPtr);
        snprintf(message,
                 sizeof(message),
                 "Persistent settings for future processes named '%s' have been reset.",
                 processName);
        SendToLogTool(toolIpcSessionRef, message);
        return;
    }

    DeleteProcessName(procNameObjPtr);

    snprintf(message,
             sizeof(message),
             "Process name '%s' has been forgotten.",
             processName);
    SendToLogTool(toolIpcSessionRef, message);
}



//--------------------------------------------------------------------------------------------------
/**
 * Process a message received from a connected log session client.
 **/
//--------------------------------------------------------------------------------------------------
static void ClientMsgReceiveHandler
(
    le_msg_MessageRef_t msgRef,     ///< [IN] Reference to the message received.
    void*               contextPtr  ///< Not used.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_SessionRef_t ipcSessionRef = le_msg_GetSession(msgRef);
    const char* rxBuffPtr = le_msg_GetPayloadPtr(msgRef);

    char command;
    char processName[LIMIT_MAX_PROCESS_NAME_BYTES];
    char componentName[LIMIT_MAX_COMPONENT_NAME_BYTES];
    const char* commandDataPtr;

    // Parse the packet to get the process and component names.
    if (ParseCmdPacket(rxBuffPtr, &command, processName, componentName, &commandDataPtr))
    {
        // Process the data.
        switch (command)
        {
            case LOG_CMD_REG_COMPONENT:

                RegComponent(processName, componentName, commandDataPtr, ipcSessionRef);
                le_msg_Respond(msgRef);

                return;

            case LOG_CMD_SET_LEVEL:
            case LOG_CMD_ENABLE_TRACE:
            case LOG_CMD_DISABLE_TRACE:
            case LOG_CMD_LIST_COMPONENTS:
            case LOG_CMD_FORGET_PROCESS:

                LE_ERROR("Client attempted to issue a log control command (%c)!", command);

                le_msg_CloseSession(ipcSessionRef);

                break;

            default:

                LE_ERROR("Unknown command byte '%c' received from client.", command);

                le_msg_CloseSession(ipcSessionRef);

                break;
        }
    }
    else
    {
        le_msg_CloseSession(ipcSessionRef);
    }

    le_msg_ReleaseMsg(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Process a message received from a connected log control tool.
 **/
//--------------------------------------------------------------------------------------------------
static void ControlToolMsgReceiveHandler
(
    le_msg_MessageRef_t msgRef,     ///< [IN] Reference to the message received.
    void*               contextPtr  ///< Not used.
)
//--------------------------------------------------------------------------------------------------
{
    le_msg_SessionRef_t ipcSessionRef = le_msg_GetSession(msgRef);
    const char* rxBuffPtr = le_msg_GetPayloadPtr(msgRef);

    char command;
    char processName[LIMIT_MAX_PROCESS_NAME_BYTES];
    char componentName[LIMIT_MAX_COMPONENT_NAME_BYTES];
    const char* commandDataPtr;

    // Parse the packet to get the process and component names.
    if (ParseCmdPacket(rxBuffPtr, &command, processName, componentName, &commandDataPtr))
    {
        // Process the data.
        switch (command)
        {
            case LOG_CMD_SET_LEVEL:

                SetLevel(processName, componentName, commandDataPtr, ipcSessionRef);

                break;

            case LOG_CMD_ENABLE_TRACE:

                SetTrace(processName, componentName, commandDataPtr, true, ipcSessionRef);

                break;

            case LOG_CMD_DISABLE_TRACE:

                SetTrace(processName, componentName, commandDataPtr, false, ipcSessionRef);

                break;

            case LOG_CMD_REG_COMPONENT:

                LE_ERROR("Unexpected command '%c' from log control tool.", command);

                break;

            case LOG_CMD_LIST_COMPONENTS:

                GenerateList(ipcSessionRef);

                break;

            case LOG_CMD_FORGET_PROCESS:

                ForgetProcess(processName, ipcSessionRef);

                break;

            default:

                LE_ERROR("Unknown command byte '%c' received from log control tool.", command);
                le_msg_CloseSession(le_msg_GetSession(msgRef));

                break;
        }
    }

    le_msg_CloseSession(le_msg_GetSession(msgRef));
    le_msg_ReleaseMsg(msgRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes the fd log object and monitor.  Closes the associated fd.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteFdLog
(
    int fd,                     ///< [IN] Fd to close.
    FdLog_t* fdLogPtr           ///< [IN] Fd log object to delete.
)
{
    // Delete the fd monitor.
    le_fdMonitor_Delete(fdLogPtr->monitorRef);

    // Close the fd.
    fd_Close(fd);

    // Delete the fd log object.
    le_mem_Release(fdLogPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Logs message received from the fd.
 */
//--------------------------------------------------------------------------------------------------
static void LogFdMessages
(
    int   fd,
    short events
)
{
    FdLog_t* fdLogPtr = le_fdMonitor_GetContextPtr();

    if (events & POLLIN)
    {
        // Read the data from the fd.
        char msg[MAX_MSG_SIZE] = {'\0'};

        int c;

        do
        {
            c = read(fd, msg, sizeof(msg));
        }
        while ( (c == -1) && (errno == EINTR) );

        if (c == -1)
        {
            LE_ERROR("Could not read fd log message for app/process '%s/%s[%d]'.  %m.",
                     fdLogPtr->appName, fdLogPtr->procName, fdLogPtr->pid);

            DeleteFdLog(fd, fdLogPtr);
        }

        // Log the data.
        // TODO: Don't log the app name for now so that it matches all the other log formats.  Add
        //       the app name to all log messages at the same time.
        log_LogGenericMsg(fdLogPtr->level, fdLogPtr->procName, fdLogPtr->pid, msg);
    }

    if ( (events & POLLRDHUP) || (events & POLLERR) || (events & POLLHUP) )
    {
        LE_DEBUG("Error on app/proc '%s/%s' log fd, events=%d.  Cannot log from this fd.",
                fdLogPtr->appName, fdLogPtr->procName, events);

        DeleteFdLog(fd, fdLogPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Creates a monitor for an application process' file descriptor for logging.
 */
//--------------------------------------------------------------------------------------------------
static void CreateFdLogMonitor
(
    int fd,                     ///< [IN] File descriptor to log.
    const char* appNamePtr,     ///< [IN] Name of the application.
    const char* procNamePtr,    ///< [IN] Name of the process.
    pid_t pid,                  ///< [IN] PID of the process.
    le_log_Level_t logLevel,    ///< [IN] Level to log messages from this fd at.
    const char* monitorNamePtr  ///< [IN] Name of monitor.
)
{
    // Create fd log object.
    FdLog_t* fdLogPtr = le_mem_ForceAlloc(FdLogPoolRef);

    if (le_utf8_Copy(fdLogPtr->appName, appNamePtr, LIMIT_MAX_APP_NAME_BYTES, NULL) != LE_OK)
    {
        LE_KILL_CLIENT("App name '%s' too long.", appNamePtr);
    }

    if (le_utf8_Copy(fdLogPtr->procName, procNamePtr, LIMIT_MAX_PROCESS_NAME_BYTES, NULL) != LE_OK)
    {
        LE_KILL_CLIENT("Proc name '%s' too long.", procNamePtr);
    }

    fdLogPtr->level = logLevel;
    fdLogPtr->pid = pid;

    // Create the fd monitor.
    fdLogPtr->monitorRef = le_fdMonitor_Create(monitorNamePtr, fd, LogFdMessages, 0);

    // Set the fd monitor context.
    le_fdMonitor_SetContextPtr(fdLogPtr->monitorRef, fdLogPtr);

    // Enable the monitoring.
    le_fdMonitor_Enable(fdLogPtr->monitorRef, POLLIN);
}


//--------------------------------------------------------------------------------------------------
/**
 * Registers an application process' standard error for logging.  Messages from this file descriptor
 * will be logged at LE_LOG_ERR level.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
void logFd_StdErr
(
    int fd,
        ///< [IN]
        ///< stderr file descriptor.

    const char* appName,
        ///< [IN]
        ///< Name of the application.

    const char* procName,
        ///< [IN]
        ///< Name of the process.

    int pid
        ///< [IN]
        ///< PID of the process.
)
{
    // Create the fd monitor.
    char monitorName[LIMIT_MAX_PROCESS_NAME_BYTES + 6];
    LE_ASSERT(snprintf(monitorName, sizeof(monitorName), "%s%s", procName, "Stderr") < sizeof(monitorName));

    CreateFdLogMonitor(fd, appName, procName, pid, LE_LOG_ERR, monitorName);
}


//--------------------------------------------------------------------------------------------------
/**
 * Registers an application process' standard out for logging.  Messages from this file descriptor
 * will be logged at LE_LOG_INFO level.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
void logFd_StdOut
(
    int fd,
        ///< [IN]
        ///< stdout file descriptor.

    const char* appName,
        ///< [IN]
        ///< Name of the application.

    const char* procName,
        ///< [IN]
        ///< Name of the process.

    int pid
        ///< [IN]
        ///< PID of the process.
)
{
        // Create the fd monitor.
    char monitorName[LIMIT_MAX_PROCESS_NAME_BYTES + 6];
    LE_ASSERT(snprintf(monitorName, sizeof(monitorName), "%s%s", procName, "Stdout") < sizeof(monitorName));

    CreateFdLogMonitor(fd, appName, procName, pid, LE_LOG_INFO, monitorName);
}


//--------------------------------------------------------------------------------------------------
/**
 * The main function for the log daemon.  Listens for commands from process/components and log tools
 * and processes the commands.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Create the memory pools.
    ProcessNamePoolRef = le_mem_CreatePool("ProcessName", sizeof(ProcessName_t));
    ComponentNamePoolRef = le_mem_CreatePool("ComponentName", sizeof(ComponentName_t));
    TraceNamePoolRef = le_mem_CreatePool("TraceName", sizeof(TraceName_t));
    RunningProcessPoolRef = le_mem_CreatePool("RunningProcess", sizeof(RunningProcess_t));
    LogSessionPoolRef = le_mem_CreatePool("LogSession", sizeof(LogSession_t));
    TracePoolRef = le_mem_CreatePool("Traces", sizeof(Trace_t));
    FdLogPoolRef = le_mem_CreatePool("FdLogs", sizeof(FdLog_t));

    // Tune the pools' initial sizes to reduce warnings in the log at start-up.
    // TODO: Make this configurable.
    le_mem_ExpandPool(ProcessNamePoolRef, MAX_EXPECTED_PROCESSES);
    le_mem_ExpandPool(ComponentNamePoolRef, MAX_EXPECTED_COMPONENTS);
    le_mem_ExpandPool(TraceNamePoolRef, MAX_EXPECTED_TRACES);
    le_mem_ExpandPool(RunningProcessPoolRef, MAX_EXPECTED_PROCESSES);
    le_mem_ExpandPool(LogSessionPoolRef, MAX_EXPECTED_COMPONENTS);
    le_mem_ExpandPool(TracePoolRef, MAX_EXPECTED_TRACES);
    le_mem_ExpandPool(FdLogPoolRef, MAX_EXPECTED_PROCESSES * 2); // Generally 2 fds per process (stderr, stdout).

    // Create the hash maps.
    ProcessNameMapRef = le_hashmap_Create("ProcessName",
                                          MAX_EXPECTED_PROCESSES,
                                          le_hashmap_HashString,
                                          le_hashmap_EqualsString);
    IpcSessionMapRef  = le_hashmap_Create("IPCSession",
                                          MAX_EXPECTED_PROCESSES,
                                          IpcSessionHash,
                                          IpcSessionEquals);
    ProcessIdMapRef   = le_hashmap_Create("ProcessID",
                                          MAX_EXPECTED_PROCESSES,
                                          ProcessIdHash,
                                          ProcessIdEquals);

    // Get a reference to the Log Control Protocol identification.
    le_msg_ProtocolRef_t protocolRef = le_msg_GetProtocolRef(LOG_CONTROL_PROTOCOL_ID,
                                                             LOG_MAX_CMD_PACKET_BYTES);

    // Create and advertise the client service.
    le_msg_ServiceRef_t serviceRef = le_msg_CreateService(protocolRef, LOG_CLIENT_SERVICE_NAME);
    le_msg_SetServiceRecvHandler(serviceRef, ClientMsgReceiveHandler, NULL);
    le_msg_AddServiceCloseHandler(serviceRef, ClientIpcSessionClosed, NULL);
    le_msg_AdvertiseService(serviceRef);

    // Create and advertise the log control service (the one the control tool uses).
    serviceRef = le_msg_CreateService(protocolRef, LOG_CONTROL_SERVICE_NAME);
    le_msg_SetServiceRecvHandler(serviceRef, ControlToolMsgReceiveHandler, NULL);
    le_msg_AdvertiseService(serviceRef);

    // Close the fd that we inherited from the Supervisor.  This will let the Supervisor know that
    // we are initialized.  Then re-open it to /dev/null so that it cannot be reused later.
    FILE* filePtr;
    do
    {
        filePtr = freopen("/dev/null", "r", stdin);
    }
    while ( (filePtr == NULL) && (errno == EINTR) );

    LE_FATAL_IF(filePtr == NULL, "Failed to redirect standard in to /dev/null.  %m.");

    LE_INFO("Log daemon ready.");
}
