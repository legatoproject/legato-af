/** @file updateDaemon.c
 *
 * Update-daemon is one of the system processes that is responsible for update(install/remove) of
 * legato framework, applications, systems and firmware. It implements API for update client; update
 * client can invoke those APIs to accomplish update process in legato. For API signatures, please
 * look at the le_update.api file documentation.
 *
 * Update-daemon follows event based asynchronous fashion while doing update. Client of the update-
 * daemon can request to create an update handle via supplying a file descriptor of update file.
 * Update-daemon will start update process when client requests to start update process via
 * supplying a valid update handle. It maintains a state machine to keep track of current update
 * process and the current state of this state machine is exposed to client via client's callback
 * function. Client may request daemon to cancel currently ongoing update task but daemon will
 * only cancel when it is safe to do. For details and step by step interaction sequence between
 * daemon and client, please look at the le_update.api documentation.
 *
 * Update follows broker architectural pattern where update-daemon works as broker. Update-daemon
 * parses the manifest string of update file, determines the update task types(install/remove)
 * and the entity(i.e. app, firmware, framework, system) where this update task should be carried
 * on. Based on the information from manifest, update-daemon invokes appropriate tools/api(e.g. app
 * tool for legato application install/remove, modemService api for firmware update etc) to
 * accomplish the update task.
 *
 * @note Update daemon only support single update task at a time. If two client request for update
 *    task(by calling le_update_Create()), update daemon will only serve the first client and
 *    refuse(by returning NULL as UpdateHandle) the second client. The second need to request again
 *    when update daemon is free.
 *
 * @warning Currently only firmware update and app install/remove are supported by update daemon.
 *    Should be extended for framework & system.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "limit.h"
#include "manifest.h"
#include "fileDescriptor.h"
#include "user.h"
#include "killProc.h"


//--------------------------------------------------------------------------------------------------
/**
 * App control tool information.
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define APP_TOOL_PATH                     "/usr/local/bin/appTool"
#define APP_TOOL                          "appTool"
// @}


//--------------------------------------------------------------------------------------------------
/**
 * App unpack tool information.
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define APP_UNPACK_TOOL_PATH              "/usr/local/bin/appsUnpack"
#define APP_UNPACK                        "appsUnpack"
// @}


//--------------------------------------------------------------------------------------------------
/**
 * Directory where apps will be unpacked by app unpack tool.
 */
//--------------------------------------------------------------------------------------------------
#define APP_UNPACK_DIR_PATH               "/opt/legato/appsUnpack/"


//--------------------------------------------------------------------------------------------------
/**
 * Security unpack tool & user information.
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define SECURE_UNPACK_TOOL_PATH           "/usr/local/bin/security-unpack"
#define SECURE_UNPACK                     "security-unpack"
#define SECURITY_UNPACK_USER              "SecurityUnpack"
// @}


//--------------------------------------------------------------------------------------------------
/**
 * Cleanup & restore tool information.
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define CLEANUP_RESTORE_TOOL_PATH   "/usr/local/bin/cleanupRestoreTool"
#define CLEANUP_RESTORE_TOOL        "cleanupRestoreTool"
// @}


//--------------------------------------------------------------------------------------------------
/**
 * Supported commands by update daemon.
 * @{
 */
//--------------------------------------------------------------------------------------------------
#define CMD_STR_INSTALL         "install"
#define CMD_STR_REMOVE          "remove"
// @}


//--------------------------------------------------------------------------------------------------
/**
 * Minimum buffer size for file/stream read & write.
 *
 * @todo Tune this if needed. It might be specified in .config file.
 */
//--------------------------------------------------------------------------------------------------
#define BUFFER_SIZE                    4096


//--------------------------------------------------------------------------------------------------
/**
 * UpdateObj memory pool.  Must be initialized before creating any UpdateObj object.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t UpdateObjMemPoolRef = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * The Safe Reference Map to be used to create UpdateObj References.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t UpdateObjSafeRefMap = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Reference to update daemon main thread.
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t MainThreadRef;


//--------------------------------------------------------------------------------------------------
/**
 * Cleanup and restore process id. This process is kicked off to remove temporary files in apps
 * unpack directory and restore apps in apps backup directory.
 */
//--------------------------------------------------------------------------------------------------
static pid_t CleanupProcId;


//--------------------------------------------------------------------------------------------------
/**
 * ProcessID of app unpacker process.
 */
//--------------------------------------------------------------------------------------------------
static pid_t UnpackerPid;


//--------------------------------------------------------------------------------------------------
/**
 * ProcessID of update(/remove) process(e.g. app script).
 */
//--------------------------------------------------------------------------------------------------
static pid_t InstallerPid;


//--------------------------------------------------------------------------------------------------
/**
 * Security unpack process id.
 */
//--------------------------------------------------------------------------------------------------
static pid_t SecUnpackPid;


//--------------------------------------------------------------------------------------------------
/**
 * Output file descriptor of security unpack process.
 */
//--------------------------------------------------------------------------------------------------
static int SecUnpackFd;


//--------------------------------------------------------------------------------------------------
/**
 * Monitor to update file descriptor.
 */
//-------------------------------------------------------------------------------------------------
static le_fdMonitor_Ref_t FdMonRef;


//--------------------------------------------------------------------------------------------------
/**
 * Write end available to update-daemon to pipe update data. Please note, read end of the pipe is
 * tied with the input of the unpack process. Update-daemon pipes data by writing on the write end
 * of the pipe.
 */
//-------------------------------------------------------------------------------------------------
static int UnpackerInputFd;


//--------------------------------------------------------------------------------------------------
/**
 * Item data passed via pipe.
 */
//-------------------------------------------------------------------------------------------------
static size_t ItemDataPassed;


//--------------------------------------------------------------------------------------------------
/**
 * Client callback function.
 */
//-------------------------------------------------------------------------------------------------
static le_update_ProgressHandlerFunc_t ProgressHandlerFunc;


//--------------------------------------------------------------------------------------------------
/**
 * Context pointer supplied by IPC layer. Used while invoking client callback function.
 */
//-------------------------------------------------------------------------------------------------
static void* ContextPtr;



//--------------------------------------------------------------------------------------------------
/**
 * Flag to keep track of deletion request of current update from client.
 */
//-------------------------------------------------------------------------------------------------
static bool IsDeletionRequested = false;


//--------------------------------------------------------------------------------------------------
/**
 * Flag to keep track whether manifest thread terminated.
 */
//-------------------------------------------------------------------------------------------------
static bool IsManifestThreadDone = true;

//-------------------------------------------------------------------------------------------------
/**
 *  The Update object structure. Contains all information of current update task.
 */
//-------------------------------------------------------------------------------------------------
typedef struct
{
    manifest_Ref_t manRef;            ///< Reference to manifest data structure.
    manifest_ItemRef_t itemRef;       ///< Update item references.
    le_update_HandleRef_t handle;     ///< Safe reference to update object data structure.

    size_t totalPayLoad;              ///< Total payload size specified in manifest.
    size_t payloadPassed;             ///< Total data passed to unpacker process.
    uint   percentDone;               ///< Percent done for current state. As example: at state
                                      ///< LE_UPDATE_STATE_UNPACKING, percentDone=80 means,
                                      ///< 80% of the update file data is already transferred to
                                      ///< unpack process.

    le_update_ErrorCode_t errorCode;  ///< Error code when update failed.
    le_update_State_t state;          ///< Current state in update state machine.
}
Update_t;


//--------------------------------------------------------------------------------------------------
/**
 * Current update object
 */
//--------------------------------------------------------------------------------------------------
static Update_t* CurUpdatePtr = NULL;


//--------------------------------------------------------------------------------------------------
/**
 * Validate the supplied file descriptor.
 *
 * @return
 *      - true: Supplied file descriptor is valid.
 *      - false: Supplied file descriptor is invalid..
 */
//--------------------------------------------------------------------------------------------------
static bool IsValidFileDesc
(
    int fileDesc                   ///<[IN] File descriptor to be validated.
)
{
    return fcntl(fileDesc, F_GETFL) != -1 || errno != EBADF;
}


//--------------------------------------------------------------------------------------------------
/**
 * Clear all update related static variables.
 */
//--------------------------------------------------------------------------------------------------
static void ClearUpdateInfo
(
   void
)
{
    FdMonRef = NULL;
    SecUnpackPid = -1;
    SecUnpackFd = -1;
    InstallerPid = -1;
    UnpackerPid = -1;
    UnpackerInputFd = -1;
    ItemDataPassed = 0;
    IsDeletionRequested = false;
    ProgressHandlerFunc = NULL;
    ContextPtr = NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Clear update object member values. This function is called as part of init/release task.
 */
//--------------------------------------------------------------------------------------------------
static void ClearUpdateObj
(
    Update_t* updatePtr      ///<[IN] Pointer to update object for current update task.
)
{
    LE_DEBUG("Clearing update obj");
    updatePtr->manRef = NULL;
    updatePtr->itemRef = NULL;
    updatePtr->handle = NULL;
    updatePtr->errorCode = LE_UPDATE_ERR_NONE;
    updatePtr->totalPayLoad = 0;
    updatePtr->payloadPassed = 0;
    updatePtr->percentDone = 0;
    updatePtr->state = LE_UPDATE_STATE_NEW;
}


//--------------------------------------------------------------------------------------------------
/**
 * Clear signal masks.
 */
//--------------------------------------------------------------------------------------------------
static void ClearSigMasks
(
    void
)
{
    sigset_t sigSet;
    LE_FATAL_IF(sigfillset(&sigSet) == -1, "Can't fill sigset. %m");
    LE_FATAL_IF(pthread_sigmask(SIG_UNBLOCK, &sigSet, NULL) != 0,
                "Can't unblock process's signal");
}


//--------------------------------------------------------------------------------------------------
/**
 * Set uid and gid for the caller process.
 */
//--------------------------------------------------------------------------------------------------
static void SetProcUidGid
(
    const char* usrID                 ///<[IN] User ID to set.
)
{
    uid_t uid;
    gid_t gid;
    LE_FATAL_IF(user_Create(usrID, &uid, &gid) == LE_FAULT, "Can't create user: %s", usrID);

    // Clear our supplementary groups list.
    LE_FATAL_IF(setgroups(0, NULL) == -1, "Could not set the supplementary groups list.  %m.");
    // Set our process's primary group ID.
    LE_FATAL_IF(setgid(gid) == -1, "Could not set the group ID.  %m.");

    // Set our process's user ID.  This sets all of our user IDs (real, effective, saved).  This
    // call also clears all capabilities.  This function in particular MUST be called after all
    // the previous system calls because once we make this call we will lose root priviledges.
    LE_FATAL_IF(setuid(uid) == -1, "Could not set the user ID.  %m.");
}


//--------------------------------------------------------------------------------------------------
/**
 * Cleanup temporary files and restore backed up apps. This is called during initialization of
 * or any failure recovery time.
 */
//--------------------------------------------------------------------------------------------------
static void CleanupRestoreTask
(
    void
)
{

    pid_t pid;
    LE_FATAL_IF(((pid = fork()) == -1), "Can't create child process, errno: %d (%m)", errno);

    if (pid == 0)
    {
        // Clear the signal mask so the child does not inherit our signal mask.
        ClearSigMasks();
        // Exec the cleanup tool.
        execl(CLEANUP_RESTORE_TOOL_PATH, CLEANUP_RESTORE_TOOL, (char *)NULL);
        LE_FATAL("Error while exec: %s , errno: %d (%m)", CLEANUP_RESTORE_TOOL_PATH, errno);
    }
    else
    {
        CleanupProcId = pid;
        LE_DEBUG("Created cleanup/restore process, pid: %d", pid);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Call client callback function if it is registered.
 */
//--------------------------------------------------------------------------------------------------
static void ClientCallBackFunc
(
    Update_t* updatePtr           ///<[IN] Pointer to update object.
)
{

    if (ProgressHandlerFunc != NULL)
    {

        LE_DEBUG("State: %d, percentDone: %d",
                 updatePtr->state,
                 updatePtr->percentDone);

        ProgressHandlerFunc(updatePtr->state,
                            updatePtr->percentDone,
                            ContextPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Get associated update object by update task's process id.
 *
 * @return
 *      - A pointer to the update object if successful.
 *      - NULL if the update object is not found.
 */
//--------------------------------------------------------------------------------------------------
static Update_t* GetUpdateObj
(
    pid_t pid                      ///<[IN] The pid of the update daemon child process.
)
{
    // Following condition is valid as pid's are set to -1 as soon as it become invalid.
    if ((InstallerPid == pid) ||
        (SecUnpackPid == pid) ||
        (UnpackerPid == pid))
    {
        return CurUpdatePtr;
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get associated update object using supplied handle.
 *
 * @return
 *      - A pointer to the update object if successful.
 *      - NULL if the update object is not found.
 */
//--------------------------------------------------------------------------------------------------
static Update_t* GetUpdateObjUsingHandle
(
    le_update_HandleRef_t handle   ///<[IN] Handle for update package.
)
{
    if (handle == NULL)
    {
        return NULL;
    }

    // No lock needed as only one update task will be handled at a time.
    Update_t* updatePtr = le_ref_Lookup(UpdateObjSafeRefMap, handle);

    return updatePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get next item from manifest based on the flag given as parameter.
 *
 * @return
 *      - A pointer to the update item if successful.
 *      - NULL if no item is found.
 */
//--------------------------------------------------------------------------------------------------
static manifest_ItemRef_t NextItem
(
    manifest_Ref_t manRef,              ///<[IN] Reference to manifest object
    manifest_ItemRef_t itemRef,         ///<[IN] Reference to item object.
    bool unpackFlag                     ///<[IN] Flag based on which next item will be picked.
)
{
    manifest_ItemRef_t nextItemRef = manifest_GetNextItem(manRef, itemRef);

    while (nextItemRef != NULL)
    {
        if (unpackFlag == false)  // This means any valid item(i.e. install/remove).
        {
            return nextItemRef;
        }
        else if (manifest_GetItemCmd(nextItemRef) == LE_UPDATE_CMD_INSTALL)
        {
            // Currently only install command is associated with unpack data.
            // TODO: Add update command when it will be available as it may have some data to unpack.
            return nextItemRef;
        }
        nextItemRef = manifest_GetNextItem(manRef, nextItemRef);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get next unpack item from manifest.
 *
 * @return
 *      - Pointer to next item for unpack if successful.
 *      - NULL if no item is found.
 */
//--------------------------------------------------------------------------------------------------
static manifest_ItemRef_t NextItemForUnpack
(
    manifest_Ref_t manRef,              ///<[IN] Reference to manifest object
    manifest_ItemRef_t itemRef          ///<[IN] Reference to item object.
)
{

    manifest_ItemRef_t nextItemRef = NextItem(manRef, itemRef, true);

    if ( nextItemRef != NULL)
    {
        LE_DEBUG("Got itemRef: %p", nextItemRef);
        UnpackerPid = -1;
        ItemDataPassed = 0;
        InstallerPid = -1;
        UnpackerInputFd = -1;
    }

    return nextItemRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get next update item from manifest.
 *
 * @return
 *      - Pointer to next item for update if successful.
 *      - NULL if no item is found.
 */
//--------------------------------------------------------------------------------------------------
static manifest_ItemRef_t NextItemForUpdate
(
    manifest_Ref_t manRef,              ///<[IN] Reference to manifest object
    manifest_ItemRef_t itemRef          ///<[IN] Reference to item object.
)
{

    manifest_ItemRef_t nextItemRef = NextItem(manRef, itemRef, false);

    if ( nextItemRef != NULL)
    {
        LE_DEBUG("Got itemRef: %p", nextItemRef);
        UnpackerPid = -1;
        ItemDataPassed = 0;
        InstallerPid = -1;
        UnpackerInputFd = -1;
    }

    return nextItemRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Verify whether an update task is started or not.
 *
 * @return
 *      - true: if any update task already started.
 *      - false: if no update task not started.
 */
//--------------------------------------------------------------------------------------------------
static bool IsUpdateStarted
(
    Update_t* updatePtr             ///<[IN] Pointer to update object.
)
{
    return updatePtr->state != LE_UPDATE_STATE_NEW;
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks and close file descriptor if it is valid.
 */
//--------------------------------------------------------------------------------------------------
static void CloseValidFd
(
    int *fdPtr              ///<[IN] Pointer to target file descriptor.
)
{
    // NOTE: When we close file descriptor, we assign a negative value to avoid boundary condition
    // (If we don't assign negative value, OS might assign same file descriptor value to other;
    // and later we may end up closing wrong file descriptor).

    if (IsValidFileDesc(*fdPtr))
    {
        LE_DEBUG("Closing Fd: %d", *fdPtr);
        fd_Close(*fdPtr);
        *fdPtr = -1;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks and deletes file descriptor monitor if it is valid.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteValidFdMon
(
    void
)
{
    if (FdMonRef != NULL)
    {
        LE_DEBUG("Deleting FdMon: %p", FdMonRef);
        le_fdMonitor_Delete(FdMonRef);
        FdMonRef = NULL;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Closes relevant file descriptors and stops their file descriptor monitors based on the condition
 * of currently ongoing update task.
 */
//--------------------------------------------------------------------------------------------------
static void CloseOpenedFds
(
    void
)
{
    LE_DEBUG("Closing opened fds and fdMon");

    CloseValidFd(&UnpackerInputFd);
    DeleteValidFdMon();
    CloseValidFd(&SecUnpackFd);
}


//--------------------------------------------------------------------------------------------------
/**
 * Releases all allocated resources for a update task.
 */
//--------------------------------------------------------------------------------------------------
static void Release
(
    Update_t* updatePtr      ///<[IN] Pointer to update object.
)
{
    LE_ASSERT(updatePtr != NULL);

    LE_DEBUG("Cleaning allocated resources, handle: %p.", updatePtr->handle);

    if ( updatePtr->manRef != NULL)
    {
        manifest_Delete(updatePtr->manRef);
    }

    // Following check is needed to avoid a race condition(An update is finished but its deletion API
    // isn't called yet. In the meantime updateDaemon granted permission new client, then old update's
    // deletion API call will ended up closing fds & fDMon of new client)
    if (CurUpdatePtr == updatePtr)
    {
        // Close all opened file descriptor and their monitor to stop unwanted call of handlers.
        CloseOpenedFds();
    }

    if (updatePtr->handle != NULL)
    {
        le_ref_DeleteRef(UpdateObjSafeRefMap, updatePtr->handle);
    }

    // Clear all the members value.
    ClearUpdateObj(updatePtr);
    le_mem_Release(updatePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to kill a task with valid pid.
 */
//--------------------------------------------------------------------------------------------------
static void KillValidTask
(
    pid_t pid      ///<[IN] Process id of the task to be killed.
)
{
    if (pid > 0)
    {
        LE_DEBUG("Killing child, pid: %d", pid);

        // Kill the child, we no longer need it. SigChld handler will take care of resource release.
        kill_Hard(pid);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to kill update task underway.
 */
//--------------------------------------------------------------------------------------------------
static void KillAllTasks
(
    void
)
{
    KillValidTask(SecUnpackPid);
    KillValidTask(UnpackerPid);
    KillValidTask(InstallerPid);
}


//--------------------------------------------------------------------------------------------------
/**
 * Finishes the ongoing update.
 */
//--------------------------------------------------------------------------------------------------
static void FinishUpdateTask
(
    Update_t* updatePtr,               ///<[IN] Update object for update task.
    pid_t pid,                         ///<[IN] pid of child that died. Information purpose only.
                                       ///< Use negative value pid is not known.
    le_update_State_t endState,        ///<[IN] Final state of update task
    const char* exitMsg                ///<[IN] Message to print as finishing task.
)
{
    LE_ASSERT((endState == LE_UPDATE_STATE_SUCCESS) || (endState == LE_UPDATE_STATE_FAILED));

    if (endState == LE_UPDATE_STATE_FAILED)
    {
        if (pid > 0)  // pid is positive. Print the information which process died.
        {
            const char *procName;
            // Update task failed, get error reason and print it.
            if (pid == SecUnpackPid)
            {
                procName = SECURE_UNPACK;
                // Assigned negative value to avoid race condition( KillAllTask is called if-else block
                // If OS assign same pid to new process, KillAllTask ends up killing wrong process which
                // is a fatal error.)
                SecUnpackPid = -1;
            }
            else if (pid == UnpackerPid)
            {
                procName = APP_UNPACK;
                UnpackerPid = -1;
            }
            else if (pid == InstallerPid)
            {
                // TODO: Add procName for firmware/framework etc.
                procName = APP_TOOL;
                InstallerPid = -1;
            }

            LE_ERROR("%s process (PID: %d) %s, ErrorCode: %d, handle: %p",
                     procName,
                     pid,
                     exitMsg,
                     updatePtr->errorCode,
                     updatePtr->handle);
        }
        else
        {
            LE_ERROR("%s, ErrorCode: %d, handle: %p",
                     exitMsg,
                     updatePtr->errorCode,
                     updatePtr->handle);

        }

        KillAllTasks();

        // If Manifest thread isn't terminated, wait for its termination. This is needed when
        // deletion of update requested before manifest thread finished parsing. Please note,
        // it is only needed for failure scenario.
        while (IsManifestThreadDone == false);

        // Fork the cleanup process as there might be residual files.
        CleanupRestoreTask();
        // Failed, so set the percent done to zero.
        updatePtr->percentDone = 0;
    }
    else
    {
        LE_INFO("%s", exitMsg);
        // Success state, so set the percentDone to 100.
        updatePtr->percentDone = 100;
        ClientCallBackFunc(updatePtr);
    }

    CloseOpenedFds(); // This is needed if someone terminate update process
    updatePtr->state = endState;
    ClientCallBackFunc(updatePtr);

    // Set all pid to negative value.
    SecUnpackFd = -1;
    UnpackerPid = -1;
    InstallerPid = -1;
    CurUpdatePtr = NULL;


    if (IsDeletionRequested)
    {
        // Delete request is set via session closure or le_update_Delete() api call. Delete
        // allocated resources.
        Release(updatePtr);
        IsDeletionRequested = false;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to delete update object. This function will either delete the update object(if safe) or
 * mark it to delete in future.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteUpdateObj
(
    Update_t* updatePtr      ///<[IN] Pointer to update object for current update task.
)
{
    // Following check is needed to avoid a race condition(An update is finished but its deletion API
    // isn't called yet. In the meantime updateDaemon granted permission new client, then old update's
    // deletion API call will ended up closing fds & fDMon of new client)
    if (CurUpdatePtr == updatePtr)
    {
        // Modify flag as it will be used in subsequent section.
        IsDeletionRequested = true;
        // Following check is needed to avoid a boundary condition (Suppose the manifest thread
        // is reading data from secure unpack fd  and in that time if this method is called, it will
        // end up closing secure unpack fd)
        if (IsManifestThreadDone == true)
        {
            CloseOpenedFds();
        }
    }

    switch (updatePtr->state)
    {

        case LE_UPDATE_STATE_NEW:
        case LE_UPDATE_STATE_UNPACKING:
            // Finish the ongoing the update. No error code is set for this as handle will become
            // invalid after this call.
            FinishUpdateTask(updatePtr,
                             -1,
                             LE_UPDATE_STATE_FAILED,
                             "Encountered premature deletion");
            break;

        case LE_UPDATE_STATE_APPLYING:
            LE_ERROR("Busy. Stopping update task is not possible");
            break;

        case LE_UPDATE_STATE_SUCCESS:
        case LE_UPDATE_STATE_FAILED:
            Release(updatePtr);
            IsDeletionRequested = false;
            break;
    }
}




//--------------------------------------------------------------------------------------------------
/**
 * Handler function if client session is closed.
 */
//--------------------------------------------------------------------------------------------------
static void OnSessionCloseHandler
(
    le_msg_SessionRef_t sessionRef, ///<[IN] Client session reference.
    void*               contextPtr  ///<[IN] Client context pointer.
)
{
    le_update_HandleRef_t handle = le_msg_GetSessionContextPtr(sessionRef);
    LE_DEBUG("SessionRef: %p", sessionRef);
    Update_t* updatePtr = GetUpdateObjUsingHandle(handle);
    if (updatePtr == NULL)
    {
        // If client session get closed at le_update_create function call due to bad file descriptor
        // or bad manifest format, no session context will be set.
        // Also if client clears all allocated resources by calling le_update_Delete() api, then
        // context pointer will be NULL as well.
        LE_DEBUG("No cleanup needed");
        return;
    }

    LE_DEBUG("Session closing, handle: %p, updatePtr: %p", updatePtr->handle, updatePtr);
    DeleteUpdateObj(updatePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Writes a specified number of bytes from the provided buffer to the provided file descriptor.
 *
 * @return
 *      - LE_OK if write successful.
 *      - LE_IO_ERROR if faces any error while writing.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t WriteFile
(
    int fd,                               ///<[IN] File to write.
    void* bufPtr,                         ///<[IN] Buffer which will be written to file.
    size_t bufSize                        ///<[IN] Size of the buffer.
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(bufPtr != NULL);
    LE_ASSERT(fd >= 0);

    int bytesWr = 0;
    int tempBufSize = 0;
    int wrReq = bufSize;
    char *tempStr;

    // Requested zero bytes to write, returns immediately
    if (bufSize == 0)
    {
        return LE_OK;
    }

    do
    {
        tempStr = (char *)(bufPtr);
        tempStr = tempStr + tempBufSize;

        bytesWr= write(fd, tempStr, wrReq);

        if (bytesWr == -1)
        {
            switch(errno)
            {
                case EINTR:
                    bytesWr = 0; // Writing interrupted,  so do nothing, just break.
                    break;
                case EPIPE:    // Write end closed, log an error.
                    LE_ERROR("Read end of pipe is closed. Write end fd: %d.", fd);
                    return LE_IO_ERROR;
                default:       // other error, must log and error msg and return.
                    LE_ERROR("Error while writing file, errno: %d (%m)", errno);
                    return LE_IO_ERROR;
            }
        }

        tempBufSize += bytesWr;

        if(tempBufSize < bufSize)
        {
            wrReq = bufSize - tempBufSize;
        }
    }
    while (tempBufSize < bufSize);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Attempts to read up to specified amount of bytes from source file descriptor and write it into
 * destination file descriptor.
 *
 * @return
 *      - Bytes transferred if successful.
 *      - LE_WOULD_BLOCK if no data available in source fd.
 *      - LE_IO_ERROR if faces any error while transferring data.
 */
//--------------------------------------------------------------------------------------------------
static ssize_t TransferData
(
    int srcfd,                            ///<[IN] Source file to read.
    int destFd,                           ///<[IN] Destination file to write.
    size_t xferReqSize                    ///<[IN] Requested amount of data to transfer.
)
{
    char buffer[BUFFER_SIZE+1] = "";
    ssize_t readBytes;

    do
    {
        readBytes = read(srcfd, buffer, xferReqSize);
    }
    while ( (readBytes == -1) && (errno == EINTR) );

    // Should be a fatal error(EINVAL, EISDIR, EIO, EBADF, EFAULT).
    LE_FATAL_IF((readBytes == -1) && (errno != EAGAIN), "Read error. %m");

    if ((readBytes == -1) && (errno == EAGAIN))
    {
        // No data available, fd non-blocking but read would block.
        return LE_WOULD_BLOCK;
    }
    else if (readBytes == 0)
    {
        // Received unexpected end of file.
        LE_ERROR("Received unexpected EOF, fd: %d", destFd);
        return LE_IO_ERROR;
    }
    else
    {
        // Unix write might write less data than requested. So call our custom write function.
        ssize_t resultWr = WriteFile(destFd, buffer, readBytes);
        resultWr = (resultWr == LE_IO_ERROR) ? LE_IO_ERROR : readBytes;
        return resultWr;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for FD_READABLE(i.e. data available in file descriptor) event.
 *
 * @return
 *     - LE_OK if data transfer to update process is successful.
 *     - LE_IO_ERROR  if faces any error while transferring data.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t FdReadableHandler
(
    int fileDesc,               ///< [IN] File Descriptor of update data. Must be non-blocking.
    Update_t* updatePtr         ///< [IN] Pointer to update object.
)
{

    static uint PrevPercentDone = 0;
    ssize_t xferReqSize;
    ssize_t xferBytes;
    size_t itemSize = manifest_GetItemSize(updatePtr->itemRef);
    ssize_t payLoadLeft = itemSize - ItemDataPassed;

    // Loop until payload finished.
    while (payLoadLeft > 0)
    {
        xferReqSize = (payLoadLeft >= BUFFER_SIZE) ? BUFFER_SIZE : payLoadLeft;
        xferBytes = TransferData(fileDesc, UnpackerInputFd, xferReqSize);

        if (xferBytes == LE_WOULD_BLOCK)
        {
            // No data available, so exit from loop.
            break;
        }
        else if (xferBytes == LE_IO_ERROR)
        {
            return xferBytes;
        }
        else
        {
            ItemDataPassed += xferBytes;
            updatePtr->payloadPassed += xferBytes;
            payLoadLeft -= xferBytes;
            updatePtr->percentDone = ((uint64_t)(updatePtr->payloadPassed)*100)/updatePtr->totalPayLoad;

            LE_DEBUG("Data passed: %zd, payloadleft: %zd",
                      ItemDataPassed,
                      payLoadLeft);

            if (payLoadLeft == 0)
            {
                LE_DEBUG("Data transfer done");
                CloseValidFd(&UnpackerInputFd);

                if (FdMonRef != NULL)
                {
                    LE_DEBUG("Disabling FdMon");
                    le_fdMonitor_Disable(FdMonRef, POLLIN);
                }
                // Check whether any unpack item remaining. If not, then no data remaining to read
                // so close the fdMonitor.
                if (NextItem(updatePtr->manRef, updatePtr->itemRef, true) == NULL)
                {
                    DeleteValidFdMon();
                }

            }

            if (updatePtr->percentDone != PrevPercentDone)
            {
                ClientCallBackFunc(updatePtr);
                PrevPercentDone = updatePtr->percentDone;
            }
        }
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handler function for file descriptor events.
 */
//--------------------------------------------------------------------------------------------------
static void FdEventHandler
(
    int fileDesc,               ///< [IN] File Descriptor of update data. Must be non-blocking.
    short events                ///< [IN] Bitwise ORed set of events that occurred on the fd.
)
{
    static const int ErrMsgBytes = 256;    // Size of the error msg string.

    Update_t* updatePtr = le_fdMonitor_GetContextPtr();

    LE_ASSERT(updatePtr != NULL);

    LE_FATAL_IF(SecUnpackFd != fileDesc,
                "File descriptors don't match (%d != %d).",
                SecUnpackFd,
                fileDesc);

    // We can ignore errors and hangups on the file descriptor if the update finishes as a
    // result of the file descriptor being readable (POLLIN).
    // Set this to LE_FAULT and change it to LE_OK if handling POLLIN successfully transfer data.
    le_result_t result = LE_FAULT;

    if (events & POLLIN)
    {
        LE_DEBUG("New data available to read");
        result = FdReadableHandler(fileDesc, updatePtr);
    }

    // If only POLLHUP set --> following condition will be true as result was set to LE_FAULT.
    if ((events & POLLHUP) && (result != LE_IO_ERROR))
    {
        // If both POLLIN & POLLHUP set --> No I/O error, so file descriptor finished receiving data.
        LE_DEBUG("Encountered write hangup event");
        // Now delete fd monitor, file data will still be available at fd.
        DeleteValidFdMon();
        // If possible to pipe data to unpacker process, do it.
        if (IsValidFileDesc(UnpackerInputFd))
        {
            result = FdReadableHandler(fileDesc, updatePtr);
        }
    }

    if ((result == LE_IO_ERROR) || (events & (POLLRDHUP|POLLERR)))
    {
        char errMsg[ErrMsgBytes];

        if (events & POLLRDHUP)
        {
            snprintf(errMsg, sizeof(errMsg), "Read hang up error");
        }

        if (events & POLLERR)
        {
            snprintf(errMsg, sizeof(errMsg), "Error in file descriptor: %d", fileDesc);
        }

        if (result == LE_IO_ERROR)
        {
            snprintf(errMsg, sizeof(errMsg), "Data transfer error");
        }
        // Error, terminate the ongoing update.
        updatePtr->errorCode = LE_UPDATE_ERR_IO_ERROR;
        FinishUpdateTask(updatePtr,
                         -1,
                         LE_UPDATE_STATE_FAILED,
                         errMsg);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Set handlers for FD monitor.
 */
//--------------------------------------------------------------------------------------------------
static void SetFdMonitorHandlers
(
    Update_t* updatePtr      ///<[IN] Pointer to update object.
)
{

    le_fdMonitor_Ref_t fdMon = le_fdMonitor_Create("SecureUnpackFdMonitor",
                                                   SecUnpackFd,
                                                   FdEventHandler,
                                                   0);
    LE_DEBUG("Created fdMon: %p", fdMon);
    // Set event handler context pointer
    le_fdMonitor_SetContextPtr(fdMon, updatePtr);

    FdMonRef = fdMon;
}


//--------------------------------------------------------------------------------------------------
/**
 * Dup2 operation on file descriptors.
 */
//--------------------------------------------------------------------------------------------------
static void Dup2Fd
(
    int srcFd,     ///<[IN] Source file descriptor.
    int destFd     ///<[IN] Destination file descriptor.
)
{
    // Duplicate the srcFd to destFd.
    if (srcFd != destFd)
    {
        int r;
        do
        {
            r = dup2(srcFd, destFd);
        }
        while ( (r == -1)  && (errno == EINTR) );
        LE_FATAL_IF(r == -1, "dup2(%d, %d) failed. %m.", srcFd, destFd);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Enable data transfer either by enabling fd monitor(if it is open) or by calling FdMonitor POLLIN
 * handler directly(faking out the closure of fd monitor).
 */
//--------------------------------------------------------------------------------------------------
static void EnableDataTransfer
(
    Update_t* updatePtr               ///<[IN] Update object for current update task.
)
{
    if (FdMonRef != NULL)
    {
        // Enable POLLIN event in fdMonitor.
        le_fdMonitor_Enable(FdMonRef, POLLIN);
    }
    else
    {
        // Fd monitor for SecUnpackFd is closed. But still SecUnpackFd is valid. Call its POLLIN
        // handler directly for the remaining data..
        le_result_t result = FdReadableHandler(SecUnpackFd, updatePtr);
        if (result == LE_IO_ERROR)
        {
            // Error, terminate the ongoing update.
            updatePtr->errorCode = LE_UPDATE_ERR_IO_ERROR;
            FinishUpdateTask(updatePtr,
                             -1,
                             LE_UPDATE_STATE_FAILED,
                             "Encountered I/O error");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts firmware update specific stuffs. Please note, this is an asynchronous function, i.e. it
 * returns immediately after starting firmware update.
 *
 * @return
 *      - LE_OK if update starts successfully.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleFwUpdateCmds
(
    manifest_ItemRef_t itemRef,           ///<[IN] Reference to a firmware item.
    int* installProcId,                   ///<[OUT] PID of the firmware installer process.
    int* installerInputFd                 ///<[OUT] File descriptor to feed data into installer.
)
{
    int fildes[2];
    int pipeWriteFd;
    int pipeReadFd;
    pid_t pid;

    LE_FATAL_IF((pipe(fildes) == -1), "Can't create  pipe, errno: %d (%m)", errno);
    pipeReadFd = fildes[0];
    pipeWriteFd = fildes[1];
    LE_DEBUG("Created pipe, Readfd: %d, Writefd: %d", pipeReadFd, pipeWriteFd);

    LE_FATAL_IF(((pid = fork()) == -1), "Can't create child process, errno: %d (%m)", errno);

    if (pid == 0)
    {
        // Clear the signal mask so the child does not inherit our signal mask.
        ClearSigMasks();

        // The child does not need the write end of the pipe so close it.
        fd_Close(pipeWriteFd);

        // Start client for fwupdate. This is started only before fwupdate
        // TODO: Use try-connect when proper API is available
        le_fwupdate_ConnectService();

        LE_INFO("Updating firmware");
        le_result_t resultCode = le_fwupdate_Download(pipeReadFd);

        if (resultCode == LE_OK)
        {
            LE_INFO("Download successful; please wait for modem to reset");
        }
        else
        {
            LE_ERROR("Error in download, resultCode: %d", (int)resultCode);
        }

        exit(resultCode);
    }
    else
    {
        // Close the read end as parent process doesn't need it.
        fd_Close(pipeReadFd);
        LE_DEBUG("Firmware update task pid: %d", pid);

        *installerInputFd = pipeWriteFd;
        *installProcId = pid;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts App installation specific stuffs. Please note, this is an asynchronous function, i.e. it
 * returns immediately after starting install specific stuffs.
 *
 * @return
 *      - LE_OK if update starts successfully.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t HandleAppItem
(
    manifest_ItemRef_t itemRef,           ///<[IN] Reference to an App item.
    int* installProcId                    ///<[OUT] PID of the installer(/removal) process.
)
{

    // TODO: Move it to separate file and use as a library.
    pid_t pid;

    LE_FATAL_IF(((pid = fork()) == -1), "Can't create child process, errno: %d (%m)", errno);

    if (pid == 0)
    {
        // Clear the signal mask so the child does not inherit our signal mask.
        ClearSigMasks();

        char appUnpackPath[LIMIT_MAX_PATH_BYTES] = APP_UNPACK_DIR_PATH;
        const char* appName = manifest_GetAppItemName(itemRef);
        le_update_Command_t appCmd = manifest_GetItemCmd(itemRef);

        switch (appCmd)
        {
            case LE_UPDATE_CMD_INSTALL:
                // Build the app unpack path
                LE_FATAL_IF(
                    le_utf8_Append(appUnpackPath, appName, sizeof(appUnpackPath), NULL) != LE_OK,
                    "Error in building appUnpack path, appName: %s", appName);

                // Exec app tool with install command and app unpack path.
                execl(APP_TOOL_PATH,
                      APP_TOOL,
                      CMD_STR_INSTALL,
                      appName,
                      appUnpackPath,
                      (char *)NULL);
                break;
            case LE_UPDATE_CMD_REMOVE:
                // Exec app tool with remove command.
                execl(APP_TOOL_PATH,
                      APP_TOOL,
                      CMD_STR_REMOVE,
                      appName,
                      (char *)NULL);
                break;
        }

        // Exec must not return, any return should be a fatal error.
        LE_FATAL("Error while exec: %s , errno: %d (%m)", APP_TOOL_PATH, errno);
    }
    else
    {
        LE_DEBUG("Created app install process: %d", pid);
        *installProcId = pid;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts app unpacking stuffs. Please note, this is an asynchronous function, i.e. it
 * returns immediately after starting unpack specific stuffs.
 *
 * @return
 *      - LE_OK if unpacking process starts successfully.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AppUnpack
(
    manifest_ItemRef_t itemRef,           ///<[IN] Reference to an App item.
    int* unpackProcId,                    ///<[OUT] PID of the unpacker process.
    int* unpackProcInputFd                ///<[OUT] File descriptor to feed data into unpack process
)
{

 // TODO: Move it to separate file and use as a library.
    pid_t pid;
    int fildes[2];
    int pipeWriteFd;
    int pipeReadFd;

    LE_FATAL_IF((pipe(fildes) == -1), "Can't create  pipe, errno: %d (%m)", errno);
    pipeReadFd = fildes[0];
    pipeWriteFd = fildes[1];
    LE_DEBUG("Created pipe, Readfd: %d, Writefd: %d", pipeReadFd, pipeWriteFd);

    LE_FATAL_IF(((pid = fork()) == -1), "Can't create child process, errno: %d (%m)", errno);

    if (pid == 0)
    {
        // Clear the signal mask so the child does not inherit our signal mask.
        ClearSigMasks();
        Dup2Fd(pipeReadFd, STDIN_FILENO);
        fd_CloseAllNonStd();

        //Exec app unpack process
        execl(APP_UNPACK_TOOL_PATH,
              APP_UNPACK,
              manifest_GetAppItemName(itemRef),
              (char *)NULL);

        LE_FATAL("Error while exec: %s , errno: %d (%m)", APP_UNPACK_TOOL_PATH, errno);
    }
    else
    {
        // Close the read end as parent process doesn't need it.
        LE_DEBUG("Created app unpack process: %d", pid);
        fd_Close(pipeReadFd);
        *unpackProcInputFd= pipeWriteFd;
        *unpackProcId = pid;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts next update task from update item list.
 *
 * @return
 *      - LE_OK if remove starts successfully.
 *      - LE_NOT_FOUND if there is no update item to start.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartNextUpdateTask
(
    Update_t* updatePtr               ///<[IN] Update object for current update task.
)
{
    le_result_t result = LE_NOT_FOUND;
    updatePtr->itemRef = NextItemForUpdate(updatePtr->manRef,
                                           updatePtr->itemRef);

    if (updatePtr->itemRef != NULL)
    {
        le_update_ItemType_t itemType = manifest_GetItemType(updatePtr->itemRef);

        switch (itemType)
        {
            case LE_UPDATE_APP:
                HandleAppItem(updatePtr->itemRef,
                              &InstallerPid);

                // TODO: Add percentDone computation here.
                updatePtr->percentDone = (updatePtr->state == LE_UPDATE_STATE_UNPACKING)?
                                         0:
                                         updatePtr->percentDone;
                updatePtr->state = LE_UPDATE_STATE_APPLYING;
                ClientCallBackFunc(updatePtr);
                result = LE_OK;
                break;

            default:
                LE_ERROR("Unsupported platform: %d", itemType);
                result = LE_NOT_FOUND;
                break;
        }
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Starts next unpack task.
 *
 * @return
 *      - LE_OK if next unpack starts successfully.
 *      - LE_NOT_FOUND if there is no item to unpack.
 *
 * @assumption
 *      fdMonitor for POLLIN event is disabled before calling this function.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t StartNextUnpackTask
(
    Update_t* updatePtr               ///<[IN] Update object for current update task.
)
{

    updatePtr->itemRef = NextItemForUnpack(updatePtr->manRef,
                                           updatePtr->itemRef);

    if (updatePtr->itemRef != NULL)
    {
        switch (manifest_GetItemType(updatePtr->itemRef))
        {
            case LE_UPDATE_APP:
                AppUnpack(updatePtr->itemRef,
                          &UnpackerPid,
                          &UnpackerInputFd);
                updatePtr->state = LE_UPDATE_STATE_UNPACKING;
                break;

            case LE_UPDATE_FIRMWARE:
                HandleFwUpdateCmds(updatePtr->itemRef,
                                   &InstallerPid,
                                   &UnpackerInputFd);
                updatePtr->state = LE_UPDATE_STATE_APPLYING;
                break;

        }
        // Invoke client callback function to notify initial state information.
        ClientCallBackFunc(updatePtr);
        EnableDataTransfer(updatePtr);

        return LE_OK;
    }

    return LE_NOT_FOUND;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to manage state machine after update process exited on success.
 */
//--------------------------------------------------------------------------------------------------
static void OnSuccessHandler
(
    Update_t* updatePtr,                    ///<[IN] Pointer to update object.
    pid_t pid                               ///<[IN] Process id of the dead process.
)
{
    switch(updatePtr->state)
    {
        case LE_UPDATE_STATE_NEW:
            // Only security unpack process exists in this state.
            LE_ASSERT(pid == SecUnpackPid);
            SecUnpackPid = -1;
            break;

        case LE_UPDATE_STATE_UNPACKING:
            LE_ASSERT((pid == SecUnpackPid) || (pid == UnpackerPid));

            if (pid == SecUnpackPid)
            {
                SecUnpackPid = -1;
            }
            else if ((pid == UnpackerPid) && (StartNextUnpackTask(updatePtr) != LE_OK))
            {
                updatePtr->itemRef = NULL;
                UnpackerPid = -1;
                // All apps are unpacked, now start the update task.
                LE_FATAL_IF(StartNextUpdateTask(updatePtr) != LE_OK,
                             "BUG!! Must have a task to start");
            }
            break;

        case LE_UPDATE_STATE_APPLYING:
            LE_DEBUG("pid: %d, installerPid:%d, secUnpackPid: %d",
                     pid,
                     InstallerPid,
                     SecUnpackPid);

            if (pid == SecUnpackPid)   // This will be true in case of firmware/deletion only item.
            {
                SecUnpackPid = -1;
            }
            else
            {
                LE_ASSERT(pid == InstallerPid);
                if ((pid == InstallerPid) && (StartNextUpdateTask(updatePtr) != LE_OK))
                {
                    updatePtr->errorCode = LE_UPDATE_ERR_NONE;
                    FinishUpdateTask(updatePtr,
                                     pid,
                                     LE_UPDATE_STATE_SUCCESS,
                                     "Update Successful");
                }
            }
            break;

        case LE_UPDATE_STATE_FAILED:
        case LE_UPDATE_STATE_SUCCESS:
            LE_FATAL("BUG!! Bad state: %d, secureunpackPid: %d, received pid: %d, cleanupPid: %d",
                     updatePtr->state,
                     SecUnpackPid,
                     pid,
                     CleanupProcId);
            break;
    }

}


//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGCHLD, called via the Legato event loop.
 */
//--------------------------------------------------------------------------------------------------
static void SigChildHandler
(
    int sigNum
)
{

    static const int ErrMsgBytes = 256;  // Size of error msg.

    // More than one child may have changed state so keep checking until we get all of them.
    while (1)
    {
        // Wait for a terminated child.
        int status;
        pid_t pid;

        do
        {
            // No need to put WCONTINUED, as it will never happen as per current implementation.
            // In current implementation child process will be killed if it receives SIGSTOP signal.
            pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
        }
        while ( (pid < 0) && (errno == EINTR) );

        LE_FATAL_IF((pid == -1) && (errno == EINVAL), "Bug! Bad waitpid() call. %m.")

        LE_DEBUG("Received SigChild signal, sigNum: %d, pid: %d, CurUpdatePtr: %p",
                 sigNum,
                 pid,
                 CurUpdatePtr);

        if (pid <= 0)
        {
            // No more children have terminated.
            break;
        }

        if (pid == CleanupProcId)
        {
            LE_DEBUG("Cleanup process: %d terminated with status: %d", pid, status);
            CleanupProcId = 0;
            continue;
        }

        // Search the list of update processes.
        Update_t* updatePtr = GetUpdateObj(pid);

        if (updatePtr == NULL)
        {
            continue;
        }

        if (updatePtr->state == LE_UPDATE_STATE_FAILED)
        {
            LE_DEBUG("Already deallocated resources");
            continue;
        }

        // SigChildHandler must not get called after SUCCESS state. Should be a bug.
        LE_FATAL_IF(updatePtr->state == LE_UPDATE_STATE_SUCCESS,
            "BUG!! SigChildHanlder must not get called after SUCCESS state, pid: %d, handle: %p",
            pid,
            updatePtr->handle);

        char exitMsg[ErrMsgBytes];
        // As per current implementation, update-daemon kills child process if it receives stop signal.
        // TODO: As part of future update, check the reason of STOP signal and take correct action.
        if (WIFSTOPPED(status))
        {
            // One of the update process stopped unexpectedly, so terminate the update.

            updatePtr->errorCode = (updatePtr->errorCode == LE_UPDATE_ERR_NONE) ?
                                   LE_UPDATE_ERR_INTERNAL_ERROR :
                                   updatePtr->errorCode;

            snprintf(exitMsg, sizeof(exitMsg), "stopped unexpectedly");
            FinishUpdateTask(updatePtr,
                             pid,
                             LE_UPDATE_STATE_FAILED,
                             exitMsg);
        }
        else if (WIFSIGNALED(status))
        {
            // One of the update process killed by signal, do the cleanup and terminate the update.

            // Termination signal might be received either due to killing process from update daemon
            // to recover from error or from external entity. If update daemon send kill signal, then
            // it should already set the error code, so don't override the error code.
            updatePtr->errorCode = (updatePtr->errorCode == LE_UPDATE_ERR_NONE) ?
                                   LE_UPDATE_ERR_INTERNAL_ERROR :
                                   updatePtr->errorCode;

            snprintf(exitMsg, sizeof(exitMsg), "received signal: %d", WTERMSIG(status));
            FinishUpdateTask(updatePtr,
                             pid,
                             LE_UPDATE_STATE_FAILED,
                             exitMsg);
        }
        else if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status))
            {
                // One of the update process died with failure exit code, so terminate the update.
                updatePtr->errorCode = LE_UPDATE_ERR_INTERNAL_ERROR;
                snprintf(exitMsg, sizeof(exitMsg), "exited with failure code: %d", WEXITSTATUS(status));
                FinishUpdateTask(updatePtr,
                                 pid,
                                 LE_UPDATE_STATE_FAILED,
                                 exitMsg);
            }
            else
            {
                // Start next item or terminate.
                OnSuccessHandler(updatePtr, pid);
            }
        }
        else
        {
            LE_FATAL("Bad exit case of child, pid: %d, handle: %p", pid, updatePtr->handle);
        }
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Queued function to main thread from manifest thread. This function starts update if it receives
 * valid manifest.
 */
//-------------------------------------------------------------------------------------------------
static void ManifestHandler
(
    void* manifestRefPtr,        ///<[IN] Manifest reference as pointer.
    void* contextPtr             ///<[IN] Context pointer. Currently it is update handle.
)
{
    // Context pointer can't NULL be as update handle is passed via it.
    LE_ASSERT(contextPtr != NULL);

    Update_t* updatePtr = GetUpdateObjUsingHandle(contextPtr);
    manifest_Ref_t manifestRef = manifestRefPtr;

    // Following check is necessary to avoid a race condition(update terminated before event queue
    // schedule this method).
    if ((updatePtr == NULL) ||
        (updatePtr->state == LE_UPDATE_STATE_FAILED) ||
        (IsDeletionRequested == true))
    {
        LE_ERROR("Update process already terminated");
        // Must deallocate the manifest data, otherwise it will create memory leak.
        if (manifestRef != NULL)
        {
            manifest_Delete(manifestRef);
        }
        return;
    }

    // Now check the manifest.
    if (manifestRef == NULL)
    {
        updatePtr->errorCode = LE_UPDATE_ERR_BAD_MANIFEST;
        // Terminate ongoing update with failure.
        FinishUpdateTask(updatePtr,
                         SecUnpackPid,
                         LE_UPDATE_STATE_FAILED,
                         "returned bad manifest");
        return;
    }

    // Initialize update object.
    updatePtr->manRef = manifestRef;

    updatePtr->totalPayLoad = manifest_GetTotalPayLoad(manifestRef);

    if (updatePtr->totalPayLoad > 0)
    {
        // Change the file descriptor attribute at non-blocking, this is needed for update daemon
        // implementation of FD_READ_AVAILABLE event handler.
        fd_SetNonBlocking(SecUnpackFd);

        // Set event handlers for update file descriptor.
        SetFdMonitorHandlers(updatePtr);
    }

    // Try to unpack next task data
    le_result_t result = StartNextUnpackTask(updatePtr);

    // Data finished, so start next update task.
    if (result == LE_NOT_FOUND)
    {
        // Nothing to unpack, now start the update task.
        LE_FATAL_IF(StartNextUpdateTask(updatePtr) != LE_OK, "BUG!! Must have one task to start");
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Main function for the manifest thread. This function extracts the manifest from the file descriptor
 * received from client.
 *
 * @return
 *      - Null (always).
 */
//-------------------------------------------------------------------------------------------------
static void* ManifestThreadFunc
(
    void* updateHandle         ///<[IN] Update object for current update task.
)
{
    LE_ASSERT(updateHandle != NULL);

    Update_t* updatePtr = GetUpdateObjUsingHandle(updateHandle);

    if (updatePtr != NULL)
    {
        // TODO: Potential boundary condition, if update client/security-unpack tool dies prematurely
        // before sending the complete manifest, this thread may block at reading manifest.
        manifest_Ref_t manifestRef = manifest_Create(SecUnpackFd);
        // Queue a handler function of main thread for further processing.
        le_event_QueueFunctionToThread(MainThreadRef, ManifestHandler, manifestRef, updateHandle);
    }
    // Mark that manifest thread finished.
    IsManifestThreadDone = true;
    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Update daemon initialization function.
 */
//--------------------------------------------------------------------------------------------------
static void UpdateDaemonInit
(
    void
)
{

     /// Initial memory pool size for update objects.
    static const int DefaultUpdatePoolSize = 1;

    UpdateObjMemPoolRef = le_mem_CreatePool("UpdateObjPool", sizeof(Update_t));
    le_mem_ExpandPool(UpdateObjMemPoolRef, DefaultUpdatePoolSize);
    UpdateObjSafeRefMap = le_ref_CreateMap("UpdateSafeRefPool",
                                           DefaultUpdatePoolSize);
}


/**************************************************************************************************/
/***********************************Update daemon api definitions starts*************************/
/**************************************************************************************************/


//-------------------------------------------------------------------------------------------------
/**
 * Creates an update handle. This handle need to be used in subsequent api calls.
 *
 * @return
 *      - Null if already an update is going on.
 *      - Handle reference if successful.
 */
//-------------------------------------------------------------------------------------------------
le_update_HandleRef_t le_update_Create
(
    int fileDesc                   ///<[IN] File descriptor containing update package.
)
{

    if (fileDesc < 0)
    {
        LE_KILL_CLIENT("Passed invalid file descriptor");
        // Following return is used to avoid compilation error, it has no meaning to client
        // as client will be killed.
        return NULL;
    }

    // Check whether any update is active.
    if (CurUpdatePtr != NULL)
    {
        LE_ERROR("Busy, already requested an update. Retry after it finished");
        fd_Close(fileDesc);
        return NULL;
    }

    LE_DEBUG("Received request to create update handle");

    pid_t pid;
    int fildes[2];
    int writeFdSec;
    int readFdSec;

    // Remove old residue files.
    CleanupRestoreTask();

    LE_FATAL_IF((pipe(fildes) == -1), "Can't create  pipe, errno: %d (%m)", errno);
    readFdSec = fildes[0];
    writeFdSec = fildes[1];
    LE_DEBUG("Created pipe, Readfd: %d, Writefd: %d", readFdSec, writeFdSec);

    LE_FATAL_IF(((pid = fork()) == -1), "Can't create child process, errno: %d (%m)", errno);

    if (pid == 0)
    {
        // Clear the signal mask so the child does not inherit our signal mask.
        ClearSigMasks();
        // Tie the STDIN of child process to file descriptor passed via parameter
        Dup2Fd(fileDesc, STDIN_FILENO);
        // Tie the STDOUT of child to the write end of the pipe. In that case update daemon
        // will get secure unpack output on the read end of the pipe. This trick reduces lots of
        // complexity of transferring data via event based file descriptor monitor.
        Dup2Fd(writeFdSec, STDOUT_FILENO);
        // Close all fds except stdin, stdout, stderr.
        fd_CloseAllNonStd();
        // Change the user id and gid of the security unpack process.
        SetProcUidGid(SECURITY_UNPACK_USER);
        execl(SECURE_UNPACK_TOOL_PATH, SECURE_UNPACK, (char *)NULL);
        LE_FATAL("Error while exec: %s (%m)", SECURE_UNPACK_TOOL_PATH);
    }
    else
    {
        // Close the write end and file descriptor passed as parmater.
        fd_Close(fileDesc);
        fd_Close(writeFdSec);

        // Successfully created manifest, now create UpdateObj object
        Update_t* updatePtr = le_mem_ForceAlloc(UpdateObjMemPoolRef);
        // Clear all member values.
        ClearUpdateObj(updatePtr);
        ClearUpdateInfo();

        SecUnpackFd = readFdSec;
        SecUnpackPid = pid;

        // Create a safe reference for the UpdateObj object.
        updatePtr->handle = le_ref_CreateRef(UpdateObjSafeRefMap, updatePtr);

        // Set handle as current session context pointer. This is needed for session cleanup.
        le_msg_SetSessionContextPtr(le_update_GetClientSessionRef(), updatePtr->handle);

        // Set the current update pointer
        CurUpdatePtr = updatePtr;

        LE_DEBUG("Created handle: %p, updatePtr: %p, sessionRef: %p, Security-unpack proc, pid:%d ",
                  updatePtr->handle,
                  updatePtr,
                  le_update_GetClientSessionRef(),
                  pid);

        return updatePtr->handle;
    }
}


//-------------------------------------------------------------------------------------------------
/**
 * Starts update process(i.e. parsing manifest, doing update as per manifest). This is an
 * asynchronous function (it returns after the update process has started, but doesn't verify if it
 * finishes). Client will get status through registered call back function.
 *
 * @return
 *      - LE_OK if update starts successfully.
 *      - LE_FAULT if update failed to start.
 */
//-------------------------------------------------------------------------------------------------
le_result_t le_update_Start
(
    le_update_HandleRef_t handle   ///< Handle of requested update task.
)
{
    Update_t* updatePtr = GetUpdateObjUsingHandle(handle);
    if (updatePtr == NULL)
    {
        LE_KILL_CLIENT("Supplied bad (%p) handle", handle);
        return LE_FAULT;
    }

    LE_DEBUG("Received request to start update task, handle: %p", handle);

    if (IsUpdateStarted(updatePtr))
    {
        // Update already started or finished. Log an error and return.
        // Note: Don't use LE_KILL_CLIENT here as Update State might change to failed state even
        // before calling this function (example: security-unpack process exited immediately with
        // non-zero error code).
        LE_ERROR("Update already started or finished. Please see log for details.");
        return LE_FAULT;
    }

    // Notify Client the current state.
    ClientCallBackFunc(updatePtr);

    le_thread_Ref_t threadRef = le_thread_Create("Manifest",
                                                 ManifestThreadFunc,
                                                 updatePtr->handle);

    IsManifestThreadDone = false;

    // Start the manifest thread.
    le_thread_Start(threadRef);

    // Update state machine and inform client.
    updatePtr->state = LE_UPDATE_STATE_UNPACKING;
    updatePtr->percentDone = 0;
    // Notify client the updated state.
    ClientCallBackFunc(updatePtr);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to get error code when update fails.
 *
 * @return
 *      - Error code of encountered error.
 *      - ERR_NONE if update is in any other state.
 */
//--------------------------------------------------------------------------------------------------
le_update_ErrorCode_t le_update_GetErrorCode
(
    le_update_HandleRef_t handle   ///< Handle of update task.
)
{
    Update_t* updatePtr = GetUpdateObjUsingHandle(handle);
    if (updatePtr == NULL)
    {
        LE_KILL_CLIENT("Supplied bad (%p) handle", handle);
        return LE_UPDATE_ERR_NONE;
    }

    LE_DEBUG("Received request to send error code, handle: %p", handle);

    if (updatePtr->state == LE_UPDATE_STATE_FAILED)
    {
        return updatePtr->errorCode;
    }
    else
    {
        return LE_UPDATE_ERR_NONE;
    }

}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes an update task that's underway. Either deletes the update task (if safe) or marks to
 * delete it after current update current task is finished. If manifest contains multiple update
 * items, all of the subsequent update items are cancelled as part of deletion process.
 *
 * @note The handle becomes invalid after it has been deleted..
 */
//--------------------------------------------------------------------------------------------------
void le_update_Delete
(
    le_update_HandleRef_t handle   ///< Handle of the underway update task.
)
{
    Update_t* updatePtr = GetUpdateObjUsingHandle(handle);
    if (updatePtr == NULL)
    {
        LE_KILL_CLIENT("Supplied bad (%p) handle", handle);
        return;
    }

    LE_DEBUG("Received deletion request from client, handle: %p", handle);
    DeleteUpdateObj(updatePtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_update_Progress'
 *
 * This event is used for showing status of ongoing update process.
 */
//--------------------------------------------------------------------------------------------------
le_update_ProgressHandlerRef_t le_update_AddProgressHandler
(
    le_update_HandleRef_t handle,          ///<[IN] Update handle reference.
    le_update_ProgressHandlerFunc_t handlerPtr, ///<[IN] Pointer to client callback function.
    void* contextPtr                       ///<[IN] Context pointer.
)
{
    if (handlerPtr == NULL)
    {
        LE_KILL_CLIENT("Passed invalid Handler function reference!");
        return NULL;
    }

    if (handle == NULL)
    {
        LE_KILL_CLIENT("Passed invalid Update handle!");
        return NULL;
    }
    LE_DEBUG("Registering client callback function");

    // No lock needed as only one update task will be handled at a time.
    Update_t* updatePtr = le_ref_Lookup(UpdateObjSafeRefMap, handle);

    if (updatePtr == NULL)
    {
        LE_KILL_CLIENT("Update handle %p does not exist!", handle);
        return NULL;
    }

    // Following check is needed to avoid a race condition(An update is finished but its deletion API
    // isn't called yet. In the meantime updateDaemon granted permission new client, then new client's
    // callback function can be replaced by this API call)

    if (updatePtr != CurUpdatePtr)
    {
        LE_ERROR("Already finished update. Modifying callback function isn't possible");
        return NULL;
    }
    ProgressHandlerFunc = handlerPtr;
    ContextPtr = contextPtr;

    return (le_update_ProgressHandlerRef_t)handle;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_update_Progress'
 */
//--------------------------------------------------------------------------------------------------
void le_update_RemoveProgressHandler
(
    le_update_ProgressHandlerRef_t addHandlerRef   ///[IN] Reference to client callback function.
)
{

    if (addHandlerRef == NULL)
    {
        LE_ERROR("Passed invalid Handler function reference!");
        return;
    }
    // Handle and addHandlerRef should be same thing, please look at le_update_AddProgressHandler()
    le_update_HandleRef_t handle = (le_update_HandleRef_t)addHandlerRef;

    // No lock needed as only one update task will be handled at a time.
    Update_t* updatePtr = le_ref_Lookup(UpdateObjSafeRefMap, handle);

    // Following check is needed to avoid a race condition(An update is finished but its deletion API
    // isn't called yet. In the meantime updateDaemon granted permission new client, then new client's
    // callback function can be set NULL by this API call)
    if (updatePtr != CurUpdatePtr)
    {
        LE_ERROR(" Deletion of callback function isn't possible, handle: %p!", handle);
        return;
    }

    if(ProgressHandlerFunc == NULL)
    {
        LE_ERROR("No handler available to de-register!");
    }
    else
    {
        LE_DEBUG("De-registering client callback function");
        ProgressHandlerFunc = NULL;
    }
}


/**************************************************************************************************/
/***********************************Update daemon api definitions ends*****************************/
/**************************************************************************************************/


//--------------------------------------------------------------------------------------------------
/**
 * The main function for the update daemon.  Listens for commands from process/components and apply
 * update accordingly.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    UpdateDaemonInit();
    manifest_Init();

    // Block SIGCHLD signal
    // TODO: Need to register SIGPIPE handler as well. This is needed for boundary cases(e.g.
    // graceful handling of firmware update if modem service is stopped suddenly).

    sigset_t sigSet;
    LE_ASSERT(sigemptyset(&sigSet) == 0);
    LE_ASSERT(sigaddset(&sigSet, SIGCHLD) == 0);
    LE_ASSERT(sigaddset(&sigSet, SIGPIPE) == 0);
    LE_ASSERT(sigprocmask(SIG_BLOCK, &sigSet, NULL) == 0);

    // Set the umask so that files are not accidentally created with global permissions.
    // TODO: Put it under fork and exec of each process.
    umask(S_IWGRP | S_IWOTH);

    // Register session close handler.
    // TODO: Use le_msg_SetSessionCloseHandler() when server side implementation is available.
    le_msg_AddServiceCloseHandler(le_update_GetServiceRef(), OnSessionCloseHandler, NULL);
    // Get reference of update daemon main thread.
    MainThreadRef = le_thread_GetCurrent();

    // Clean up residue file and restore any backedup apps.
    CleanupRestoreTask();

    FILE* filePtr;
    do
    {
        filePtr = freopen("/dev/null", "r", stdin);
    }
    while ( (filePtr == NULL) && (errno == EINTR) );

    // Register a signal event handler for SIGCHLD so we know when child process dies.
    le_sig_SetEventHandler(SIGCHLD, SigChildHandler);
}
