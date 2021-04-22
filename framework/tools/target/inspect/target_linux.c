/**
 * @file target_linux.c
 *
 * Target-specific functions for inspect tool.  Stops, restarts and inspects memory of the
 * target task.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "addr.h"
#include <sys/ptrace.h>

#include "inspect_target.h"

//--------------------------------------------------------------------------------------------------
/**
 * true = child process stopped
 */
//--------------------------------------------------------------------------------------------------
static bool IsChildStopped = false;


//--------------------------------------------------------------------------------------------------
/**
 * Local mapped address of liblegato.so
 */
//--------------------------------------------------------------------------------------------------
static uintptr_t LocalLibLegatoBaseAddr = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Child mapped address of liblegato.so
 */
//--------------------------------------------------------------------------------------------------
static uintptr_t ChildLibLegatoBaseAddr = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Signal to deliver when process is restarted
 */
//--------------------------------------------------------------------------------------------------
static int PendingChildSignal = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Gets the counterpart address of the specified local reference in the address space of the
 * specified process.
 *
 * @return
 *      Remote address that is the counterpart of the local address.
 */
//--------------------------------------------------------------------------------------------------
uintptr_t target_GetRemoteAddress
(
    pid_t pid,              ///< [IN] Remote process to to get the address for.
    void* localAddrPtr      ///< [IN] Local address to get the offset with.
)
{
    if (!LocalLibLegatoBaseAddr)
    {
        off_t localLibLegatoBaseAddrOff = 0;
        // Get the address of our framework library.
        if (addr_GetLibDataSection(0, "liblegato.so", &localLibLegatoBaseAddrOff) != LE_OK)
        {
            INTERNAL_ERR("Can't find our framework library address.");
        }

        LocalLibLegatoBaseAddr = localLibLegatoBaseAddrOff;
    }

    // Calculate the offset address of the local address by subtracting it by the start of our
    // own framework library address.
    uintptr_t offset = (uintptr_t)(localAddrPtr) - LocalLibLegatoBaseAddr;

    if (!ChildLibLegatoBaseAddr)
    {
        off_t childLibLegatoBaseAddrOff = 0;

        // Get the address of the framework library in the remote process.
        if (addr_GetLibDataSection(pid, "liblegato.so", &childLibLegatoBaseAddrOff) != LE_OK)
        {
            INTERNAL_ERR("Can't find address of the framework library in the remote process.");
        }

        ChildLibLegatoBaseAddr = childLibLegatoBaseAddrOff;
    }

    // Calculate the process-under-inspection's counterpart address to the local address  by adding
    // the offset to the start of their framework library address.
    return (ChildLibLegatoBaseAddr + offset);
}

//--------------------------------------------------------------------------------------------------
/**
 * Attach to the target process in order to gain control of its execution and access its memory
 * space.
 */
//--------------------------------------------------------------------------------------------------
void target_Attach
(
    pid_t pid              ///< [IN] Remote process to attach to
)
{
    if (ptrace(PTRACE_SEIZE, pid, NULL, (void*)0) == -1)
    {
        fprintf(stderr, "Failed to attach to pid %d: error %d\n", pid, errno);
        LE_FATAL("Failed to attach to pid %d: error %d\n", pid, errno);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Detach from a process that we had previously attached to.
 */
//--------------------------------------------------------------------------------------------------
__attribute__((noreturn))
void target_DetachAndExit
(
    pid_t pid              ///< [IN] Remote process to detach from
)
{
    if (ptrace(PTRACE_DETACH, pid, 0, 0) == -1)
    {
        fprintf(stderr, "Failed to detach from pid %d: error %d\n", pid, errno);
        LE_FATAL("Failed to detach from pid %d: error %d\n", pid, errno);
    }

    exit(0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Pause execution of a running process which we had previously attached to.
 */
//--------------------------------------------------------------------------------------------------
void target_Stop
(
    pid_t pid              ///< [IN] Remote process to stop.
)
{
    int waitStatus;

    if (ptrace(PTRACE_INTERRUPT, pid, 0, 0) == -1)
    {
        fprintf(stderr, "Failed to stop pid %d: error %d\n", pid, errno);
        LE_FATAL("Failed to stop pid %d: error %d\n", pid, errno);
    }

    if (waitpid(pid, &waitStatus, 0) != pid)
    {
        fprintf(stderr, "Failed to wait for stopping pid %d: error %d\n", pid, errno);
        LE_FATAL("Failed to wait for stopping pid %d: error %d\n", pid, errno);
    }

    if (WIFEXITED(waitStatus))
    {
        fprintf(stderr, "Inspected process %d exited\n", pid);
        LE_FATAL("Inspected process %d exited\n", pid);
    }
    else if (WIFSTOPPED(waitStatus))
    {
        if (WSTOPSIG(waitStatus) != SIGTRAP && !PendingChildSignal)
        {
            // Stopped for a reason other than PTRACE interrupt (above) and no pending child
            // signal.  So store signal to be delivered later.
            PendingChildSignal = WSTOPSIG(waitStatus);
        }
    }
    else if (WIFSIGNALED(waitStatus))
    {
        // Store signal to pass along to the child when we restart
        if (!PendingChildSignal)
        {
            PendingChildSignal = WTERMSIG(PendingChildSignal);
        }
    }

    IsChildStopped = true;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume execution of a previously paused process.
 */
//--------------------------------------------------------------------------------------------------
void target_Start
(
    pid_t pid              ///< [IN] Remote process to restart
)
{
    IsChildStopped = false;

    if (ptrace(PTRACE_CONT, pid, 0, (void *) (intptr_t) PendingChildSignal) == -1)
    {
        fprintf(stderr, "Failed to start pid %d: error %d\n", pid, errno);
        LE_FATAL("Failed to stop pid %d: error %d\n", pid, errno);
    }

    // Clear pending signal
    PendingChildSignal = 0;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read from the memory of an attached target process.
 */
//--------------------------------------------------------------------------------------------------
le_result_t target_ReadAddress
(
    pid_t pid,              ///< [IN] Remote process to read address
    uintptr_t remoteAddr,   ///< [IN] Remote address to read from target
    void* buffer,           ///< [OUT] Destination to read into
    size_t size             ///< [IN] Number of bytes to read
)
{
    LE_ASSERT(IsChildStopped);

    uintptr_t readWord;
    for (readWord = remoteAddr & ~(sizeof(long) - 1);
         size > 0;
         readWord += sizeof(long))
    {
        errno = 0;
        long peekWord = ptrace(PTRACE_PEEKDATA, pid, readWord, 0);

        // Check if ptrace was able to get memory
        if (errno != 0)
        {
            return LE_FAULT;
        }

        uintptr_t startOffset = (remoteAddr - readWord);
        size_t readSize = sizeof(long) - startOffset;
        LE_ASSERT(startOffset < sizeof(long));
        memcpy(buffer, ((char*)&peekWord) + startOffset, readSize);
        size -= readSize;
        remoteAddr += readSize;
        buffer = (char*)buffer + readSize;
    }

    return LE_OK;
}
