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
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "limit.h"
#include "signals.h"
#include "backtrace.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <ucontext.h>
#include <syslog.h>

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
 * Port to use for start and attach a gdbserver(1) to itself. If 0, no gdbserver(1) is started
 */
//--------------------------------------------------------------------------------------------------
static uint32_t GdbServerPort = 0;


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
 * Our show stack signal handler. This signal handler is called only when SEGV, ILL, BUS, FPE, ABRT
 * TRAP are raised. It will show useful informations: signal, fault address, fault PC, registers,
 * stack and back-trace. It also dumps the process maps.
 * Note: Because these signals are raised from low-level, we should avoid any usage of malloc(3),
 * syslog(3) and others services like these from stdio(3).
 *
 * @note This code is architecture dependant, and supports arm, x86_64, i586 and i686.
 * @note Some unsafe functions are used:
 *        - snprintf
 *        - backtrace (not on arm)
 *        - sigsetjmp/siglongjmp
 */
//--------------------------------------------------------------------------------------------------
static void ShowStackSignalHandler
(
    int sigNum,
    siginfo_t* sigInfoPtr,
    void* sigVoidPtr
)
{
    char sigString[256];
    int fd;
    struct sigcontext* ctxPtr = (struct sigcontext *)&(((ucontext_t*)sigVoidPtr)->uc_mcontext);
    pid_t tid = syscall(SYS_gettid);
    void* pcPtr = NULL;

#if defined(__arm__)
    pcPtr = (void*)ctxPtr->arm_pc;
#elif defined(__i586__) || defined(__i686__)
    pcPtr = (void*)ctxPtr->eip;
#elif defined(__x86_64__)
    pcPtr = (void*)ctxPtr->rip;
#elif defined(__mips__)
    pcPtr = (void*)ctxPtr->sc_pc;
#else
# warning "Architecture is not supported"
#endif

    // Show process, pid and tid
    snprintf(sigString, sizeof(sigString), "PROCESS: %d ,TID %d\n", getpid(), tid);
    SIG_WRITE(sigString, strlen(sigString));

    // Show signal, fault address and fault PC
    snprintf(sigString, sizeof(sigString), "SIGNAL: %d, ADDR %p, AT %p SI_CODE 0x%08x\n",
             sigNum, (SIGABRT == sigNum) ? NULL : sigInfoPtr->si_addr,
             pcPtr, sigInfoPtr->si_code);
    SIG_WRITE(sigString, strlen(sigString));

    // Explain signal
    switch( sigNum )
    {
        case SIGSEGV:
                    snprintf(sigString, sizeof(sigString), "ILLEGAL ADDRESS %p\n",
                             (void*)sigInfoPtr->si_addr);
                    break;
        case SIGFPE:
                    snprintf(sigString, sizeof(sigString), "FLOATING POINT EXCEPTION AT %p\n",
                             (void*)sigInfoPtr->si_addr);
                    break;
        case SIGTRAP:
                    snprintf(sigString, sizeof(sigString), "TRAP AT %p\n",
                             (void*)sigInfoPtr->si_addr);
                    break;
        case SIGABRT:
                    snprintf(sigString, sizeof(sigString), "ABORT\n");
                    break;
        case SIGILL:
                    snprintf(sigString, sizeof(sigString), "ILLEGAL INSTRUCTION AT %p\n",
                             (void*)sigInfoPtr->si_addr);
                    break;
        case SIGBUS:
                    snprintf(sigString, sizeof(sigString), "BUS ERROR AT %p\n",
                             (void*)sigInfoPtr->si_addr);
                    break;
        default:
                    snprintf(sigString, sizeof(sigString), "UNEXPECTED SIGNAL %d\n",
                             sigNum);
                    break;
    }
    SIG_WRITE(sigString, strlen(sigString));

    // Dump the legato version
    snprintf(sigString, sizeof(sigString), "LEGATO VERSION\n");
    SIG_WRITE(sigString, strlen(sigString));
    fd = open("/legato/systems/current/version", O_RDONLY);
    if (-1 != fd)
    {
        int rc;
        // We cannot use stdio(3) services. Print line by line
        rc = read( fd, sigString, sizeof(sigString) );
        close(fd);
        if (0 < rc)
        {
            SIG_WRITE(sigString, rc);
        }
        SIG_WRITE("\n", 1);
    }

    // Dump some process command line
    snprintf(sigString, sizeof(sigString), "PROCESS COMMAND LINE\n");
    SIG_WRITE(sigString, strlen(sigString));
    snprintf(sigString, sizeof(sigString), "/proc/%d/cmdline", getpid());
    fd = open(sigString, O_RDONLY);
    if (-1 != fd)
    {
        int rc, len;
        // We cannot use stdio(3) services. Print line by line
        do
        {
            rc = read( fd, sigString, sizeof(sigString) );
            // In /proc/<pid>/cmdline, replace '\0' by ' ' in the string
            for (len = 0; len < rc; len++)
            {
                if ('\0' == sigString[len])
                {
                    sigString[len] = ' ';
                }
            }
            if (0 < rc)
            {
                SIG_WRITE(sigString, rc);
            }
        }
        while( 0 < rc );
        close(fd);
        SIG_WRITE("\n", 1);
    }

    // Dump the process map. Useful for usage with objdump(1) and gdb(1)
    snprintf(sigString, sizeof(sigString), "PROCESS MAP\n");
    SIG_WRITE(sigString, strlen(sigString));
    snprintf(sigString, sizeof(sigString), "/proc/%d/maps", getpid());
    fd = open(sigString, O_RDONLY);
    if (-1 != fd)
    {
        int rc, len;
        // We cannot use stdio(3) services. Print line by line
        do
        {
            for (len = 0; len < sizeof(sigString); len++)
            {
                rc = read( fd, sigString + len, 1 );
                if (0 >= rc)
                {
                     break;
                }
                if ('\n' == sigString[len])
                {
                    SIG_WRITE(sigString, len + 1);
                    break;
                }
            }
        }
        while( 0 < rc );
        close(fd);
    }

    // Dump the back-trace, registers and stack
    backtrace_DumpContextStack(sigVoidPtr, 2, sigString, sizeof(sigString));

    // Check if a gdbserver(1) port is set (not zero). If yes, try to launch a
    // gdbserver(1) attached to ourself.
    if (GdbServerPort)
    {
        char gdbServerPortString[10], pidString[10];
        int gdbPid, gdbStatus;
        snprintf(gdbServerPortString, sizeof(gdbServerPortString), ":%u", GdbServerPort);
        snprintf(pidString, sizeof(pidString), "%d", getpid());
        char *gdbArg[] =
        {
             "gdbserver",
             gdbServerPortString,
             "--attach",
             pidString,
             NULL,
        };
        if (0 == (gdbPid = fork()))
        {
            execvpe( gdbArg[0], gdbArg, NULL );
        }
        wait(&gdbStatus);
    }

    // Raise this signal to our self to produce a core, if configured.
    raise(sigNum);
}

//--------------------------------------------------------------------------------------------------
/**
 * Install the ShowStackSignalHandler to show information and dump stack
 */
//--------------------------------------------------------------------------------------------------
void le_sig_InstallShowStackHandler
(
    void
)
{
    int ret;
    struct sigaction sa;
    char* gdbPtr;
    char* signalShowInfoPtr;

    if ((signalShowInfoPtr = getenv("SIGNAL_SHOW_INFO")))
    {
        if ((0 == strcasecmp(signalShowInfoPtr, "disable")) ||
            (0 == strcasecmp(signalShowInfoPtr, "no")))
        {
            LE_WARN("Handle of SEGV/ILL/BUS/FPE/ABRT and show information disabled");
            return;
        }
    }
    sa.sa_sigaction = (void (*)(int, siginfo_t *, void *))ShowStackSignalHandler;
    sigemptyset(&sa.sa_mask);
#if LE_CONFIG_ENABLE_SEGV_HANDLER
    sa.sa_flags = SA_NOCLDSTOP | SA_SIGINFO | SA_RESETHAND | SA_NODEFER;
#else
    sa.sa_flags = SA_NOCLDSTOP | SA_SIGINFO | SA_RESETHAND;
#endif // LE_CONFIG_ENABLE_SEGV_HANDLER
    ret = sigaction( SIGSEGV, &sa, NULL );
    if( ret )
    {
        LE_CRIT( "Unable to install signal handler for SIGSEGV : %m\n" );
    }
    ret = sigaction( SIGBUS, &sa, NULL );
    if( ret )
    {
        LE_CRIT( "Unable to install signal handler for SIGBUS : %m\n" );
    }
    ret = sigaction( SIGILL, &sa, NULL );
    if( ret )
    {
        LE_CRIT( "Unable to install signal handler for SIGILL : %m\n" );
    }
    ret = sigaction( SIGFPE, &sa, NULL );
    if( ret )
    {
        LE_CRIT( "Unable to install signal handler for SIGFPE : %m\n" );
    }
    ret = sigaction( SIGABRT, &sa, NULL );
    if( ret )
    {
        LE_CRIT( "Unable to install signal handler for SIGABRT : %m\n" );
    }

    if ((gdbPtr = getenv("GDBSERVER_PORT")))
    {
        if (1 != sscanf(gdbPtr, "%u", &GdbServerPort))
        {
            LE_WARN("Incorrect GDBSERVER_PORT=%s. Discarded...", gdbPtr);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Minimal signal handler that exists the application if a SIGTERM has been received.
 */
//--------------------------------------------------------------------------------------------------
static void TermSignalHandler
(
    int sigNum      ///< [IN] The signal that was received.
)
{
    LE_CRIT("Terminated");
    exit(EXIT_SUCCESS);
}

//--------------------------------------------------------------------------------------------------
/**
 * Install a default handler to handle the SIGTERM signal.
 *
 * Called automatically by main().
 */
//--------------------------------------------------------------------------------------------------
void le_sig_InstallDefaultTermHandler
(
    void
)
{
    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, TermSignalHandler);
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
