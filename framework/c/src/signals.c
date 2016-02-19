//--------------------------------------------------------------------------------------------------
/** @file signals.c
 *
 * This file implements the Legato Signal Events by making use of signalFd.  When the user sets a
 * signal event handler the handler is stored in a list of handlers and associated with a single
 * signal number.  The signal mask for the thread is then updated.
 *
 * Each thread has its own list of handlers and stores this list in the thread's local data.
 *
 * A monitor fd is created for each thread with atleast one handler but all monitor fds share a
 * single fd handler, OurSigHandler().  When OurSigHandler() is invoked it grabs the list of
 * handlers for the current thread and routes the signal to the proper user handler.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "limit.h"


//--------------------------------------------------------------------------------------------------
/**
 * The signal event monitor object.  There should be at most one of these per thread.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_fdMonitor_Ref_t  monitorRef;
    int                 fd;
    le_dls_List_t       handlerObjList;
}
MonitorObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler object.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int                         sigNum;
    le_sig_EventHandlerFunc_t   handler;
    le_dls_Link_t               link;
}
HandlerObj_t;


//--------------------------------------------------------------------------------------------------
/**
 * The signal event monitor object memory pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t MonitorObjPool;


//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler object memory pool.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t HandlerObjPool;


//--------------------------------------------------------------------------------------------------
/**
 * The thread local data's key for monitor objects.
 */
//--------------------------------------------------------------------------------------------------
static pthread_key_t SigMonKey;


//--------------------------------------------------------------------------------------------------
/**
 * Prefix for the monitor's name.  The monitor's name is this prefix plus the name of the thread.
 */
//--------------------------------------------------------------------------------------------------
#define SIG_STR     "Sig"


//--------------------------------------------------------------------------------------------------
/**
 * Returns the handler object with the matching sigNum from the list.
 *
 * @return
 *      A pointer to the handler object with a matching sigNum if found.
 *      NULL if a matching sigNum could not be found.
 */
//--------------------------------------------------------------------------------------------------
static HandlerObj_t* FindHandlerObj
(
    const int sigNum,
    le_dls_List_t* listPtr
)
{
    // Search for the sigNum from the list.
    le_dls_Link_t* handlerLinkPtr = le_dls_Peek(listPtr);

    while (handlerLinkPtr != NULL)
    {
        HandlerObj_t* handlerObjPtr = CONTAINER_OF(handlerLinkPtr, HandlerObj_t, link);

        if (handlerObjPtr->sigNum == sigNum)
        {
            return handlerObjPtr;
        }

        handlerLinkPtr = le_dls_PeekNext(listPtr, handlerLinkPtr);
    }

    return NULL;
}


//--------------------------------------------------------------------------------------------------
/**
 * Our signal handler.  This signal handler gets called whenever any unmasked signals are received.
 * This handler will read the signal info and call the appropriate user handler.
 */
//--------------------------------------------------------------------------------------------------
static void OurSigHandler
(
    int fd,         ///< The monitored file descriptor.
    short events    ///< The event or events (bit mask) that occurred on the fd.
)
{
    if (events & ~POLLIN)
    {
        LE_CRIT("Unexpected event set (0x%hx) from signal fd.", events);
        if ((events & POLLIN) == 0)
        {
            return;
        }
    }

    while(1)
    {
        // Do a read of the signal fd.
        struct signalfd_siginfo sigInfo;
        int numBytesRead = read(fd, &sigInfo, sizeof(sigInfo));

        if (numBytesRead  > 0)
        {
            // Get our thread's monitor object.
            MonitorObj_t* monitorObjPtr = pthread_getspecific(SigMonKey);
            LE_ASSERT(monitorObjPtr != NULL);

            // Find the handler object with the same signal as the one we just received.
            HandlerObj_t* handlerObjPtr = FindHandlerObj(sigInfo.ssi_signo, &(monitorObjPtr->handlerObjList));

            // Call the handler function.
            if ( (handlerObjPtr != NULL) && (handlerObjPtr->handler != NULL) )
            {
                handlerObjPtr->handler(sigInfo.ssi_signo);
            }
        }
        else if ( (numBytesRead == 0) || ((numBytesRead == -1) && (errno == EAGAIN)) )
        {
            // Nothing more to read.
            break;
        }
        else if ( (numBytesRead == -1) && (errno != EINTR) )
        {
            LE_FATAL("Could not read from signal fd: %m");
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * The signal event initialization function.  This must be called before any other functions in this
 * module is called.
 */
//--------------------------------------------------------------------------------------------------
void sig_Init
(
    void
)
{
    // Create the memory pools.
    MonitorObjPool = le_mem_CreatePool("SigMonitor", sizeof(MonitorObj_t));
    HandlerObjPool = le_mem_CreatePool("SigHandler", sizeof(HandlerObj_t));

    // Create the pthread local data key.
    LE_ASSERT(pthread_key_create(&SigMonKey, NULL) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Blocks a signal in the calling thread.
 *
 * Signals that an event handler will be set for must be blocked for all threads in the process.  To
 * ensure that the signals are blocked in all threads call this function in the process' first
 * thread, all subsequent threads will inherit the signal mask.
 *
 * @note Does not return on failure.
 */
//--------------------------------------------------------------------------------------------------
void le_sig_Block
(
    int sigNum              ///< [IN] Signal to block.
)
{
    // Check if the calling thread is the main thread.
    pid_t tid = syscall(SYS_gettid);

    LE_FATAL_IF(tid == -1, "Could not get tid of calling thread.  %m.");

    LE_WARN_IF(tid != getpid(), "Blocking signal %d (%s).  Blocking signals not in the main thread \
may result in unexpected behaviour.", sigNum, strsignal(sigNum));

    // Block the signal
    sigset_t sigSet;
    LE_ASSERT(sigemptyset(&sigSet) == 0);
    LE_ASSERT(sigaddset(&sigSet, sigNum) == 0);
    LE_ASSERT(pthread_sigmask(SIG_BLOCK, &sigSet, NULL) == 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set a signal event handler for the calling thread.  Each signal can only have a single event
 * handler.  The most recent event handler set will be called when the signal is received.
 * sigEventHandler can be set to NULL to remove a previously set handler.
 *
 * @param sigNum        Cannot be SIGKILL or SIGSTOP or any program error signals: SIGFPE, SIGILL,
 *                      SIGSEGV, SIGBUS, SIGABRT, SIGIOT, SIGTRAP, SIGEMT, SIGSYS.
 *
 * @note Does not return on failure.
 */
//--------------------------------------------------------------------------------------------------
void le_sig_SetEventHandler
(
    int sigNum,                                 ///< The signal to set the event handler for.  See
                                                ///  parameter documentation in comments above.
    le_sig_EventHandlerFunc_t sigEventHandler   ///< The event handler to call when a signal is
                                                ///  received.
)
{
    // Check parameters.
    if ( (sigNum == SIGKILL) || (sigNum == SIGSTOP) || (sigNum == SIGFPE) || (sigNum == SIGILL) ||
         (sigNum == SIGSEGV) || (sigNum == SIGBUS) || (sigNum == SIGABRT) || (sigNum == SIGIOT) ||
         (sigNum == SIGTRAP) || (sigNum == SIGSYS) )
    {
        LE_FATAL("Signal event handler for %s is not allowed.", strsignal(sigNum));
    }

    // Get the monitor object for this thread.
    MonitorObj_t* monitorObjPtr = pthread_getspecific(SigMonKey);

    if (monitorObjPtr == NULL)
    {
        if (sigEventHandler == NULL)
        {
            // Event handler already does not exist so we don't need to do anything, just return.
            return;
        }
        else
        {
            // Create the monitor object
            monitorObjPtr = le_mem_ForceAlloc(MonitorObjPool);
            monitorObjPtr->handlerObjList = LE_DLS_LIST_INIT;
            monitorObjPtr->fd = -1;
            monitorObjPtr->monitorRef = NULL;

            // Add it to the thread's local data.
            LE_ASSERT(pthread_setspecific(SigMonKey, monitorObjPtr) == 0);
        }
    }

    // See if a handler for this signal already exists.
    HandlerObj_t* handlerObjPtr = FindHandlerObj(sigNum, &(monitorObjPtr->handlerObjList));

    if (handlerObjPtr == NULL)
    {
        if (sigEventHandler == NULL)
        {
            // Event handler already does not exist so we don't need to do anything, just return.
            return;
        }
        else
        {
            // Create the handler object.
            handlerObjPtr = le_mem_ForceAlloc(HandlerObjPool);

            // Set the handler.
            handlerObjPtr->link = LE_DLS_LINK_INIT;
            handlerObjPtr->handler = sigEventHandler;
            handlerObjPtr->sigNum = sigNum;

            // Add the handler object to the list.
            le_dls_Queue(&(monitorObjPtr->handlerObjList), &(handlerObjPtr->link));
        }
    }
    else
    {
        if (sigEventHandler == NULL)
        {
            // Remove the handler object from the list.
            le_dls_Remove(&(monitorObjPtr->handlerObjList), &(handlerObjPtr->link));
        }
        else
        {
            // Just update the handler.
            handlerObjPtr->handler = sigEventHandler;
        }
    }

    // Recreate the signal mask.
    sigset_t sigSet;
    LE_ASSERT(sigemptyset(&sigSet) == 0);

    le_dls_Link_t* handlerLinkPtr = le_dls_Peek(&(monitorObjPtr->handlerObjList));
    while (handlerLinkPtr != NULL)
    {
        HandlerObj_t* handlerObjPtr = CONTAINER_OF(handlerLinkPtr, HandlerObj_t, link);

        LE_ASSERT(sigaddset(&sigSet, handlerObjPtr->sigNum) == 0);

        handlerLinkPtr = le_dls_PeekNext(&(monitorObjPtr->handlerObjList), handlerLinkPtr);
    }

    // Update or create the signal fd.
    monitorObjPtr->fd = signalfd(monitorObjPtr->fd, &sigSet, SFD_NONBLOCK);

    if (monitorObjPtr->fd == -1)
    {
        LE_FATAL("Could not set signal event handler: %m");
    }

    // Create a monitor fd if it doesn't already exist.
    if (monitorObjPtr->monitorRef == NULL)
    {
        // Create the monitor name using SIG_STR + thread name.
        char monitorName[LIMIT_MAX_THREAD_NAME_BYTES + sizeof(SIG_STR)] = SIG_STR;

        LE_ASSERT(le_utf8_Copy(monitorName + sizeof(SIG_STR),
                               le_thread_GetMyName(),
                               LIMIT_MAX_THREAD_NAME_BYTES + sizeof(SIG_STR),
                               NULL) == LE_OK);

        // Create the monitor.
        monitorObjPtr->monitorRef = le_fdMonitor_Create(monitorName,
                                                        monitorObjPtr->fd,
                                                        OurSigHandler,
                                                        POLLIN);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Removes all signal event handlers for the calling thread and cleans up any resources used for
 * signal events.  This should be called before the thread exits.
 */
//--------------------------------------------------------------------------------------------------
void le_sig_DeleteAll
(
    void
)
{
    // Get the monitor object for this thread.
    MonitorObj_t* monitorObjPtr = pthread_getspecific(SigMonKey);

    if (monitorObjPtr != NULL)
    {
        // Delete the monitor.
        le_fdMonitor_Delete(monitorObjPtr->monitorRef);

        // Close the file descriptor.
        while(1)
        {
            int n = close(monitorObjPtr->fd);

            if (n == 0)
            {
                break;
            }
            else if (errno != EINTR)
            {
                LE_FATAL("Could not close file descriptor.");
            }
        }

        // Remove all handler objects from the list and free them.
        le_dls_Link_t* handlerLinkPtr = le_dls_Pop(&(monitorObjPtr->handlerObjList));

        while (handlerLinkPtr != NULL)
        {
            le_mem_Release( CONTAINER_OF(handlerLinkPtr, HandlerObj_t, link) );

            handlerLinkPtr = le_dls_Pop(&(monitorObjPtr->handlerObjList));
        }

        // Release the monitor object.
        le_mem_Release(monitorObjPtr);
    }
}

