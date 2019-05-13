/**
 * @page c_fdMonitor File Descriptor Monitor API
 *
 * @subpage le_fdMonitor.h "API Reference"
 *
 * <HR>
 *
 * In a POSIX environment, like Linux, file descriptors (fds) are used for most process I/O.
 * Many components need to be notified when one or more fds are
 * ready to read from or write to, or if there's an error or hang-up.
 *
 *Although it's common to block a thread on a call to @c read(), @c write(),
 * @c accept(), @c select(), @c poll() (or some variantion of these), if that's done in a thread
 * shared with other components, the other components won't run when needed. To avoid this, Legato
 * has methods to monitor fds reporting related events so they won't interfere with other software
 * sharing the same thread.
 *
 * @section c_fdMonitorStartStop Start/Stop Monitoring
 *
 * le_fdMonitor_Create() creates a <b> File Descriptor Monitor </b> and starts monitoring an fd.
 * A handler function and set of events is also provided to le_fdMonitor_Create().
 *
 * @code
    // Monitor for data available to read.
    le_fdMonitor_Ref_t fdMonitor = le_fdMonitor_Create("Serial Port",      // Name for diagnostics
                                                       fd,                 // fd to monitor
                                                       SerialPortHandler,  // Handler function
                                                       POLLIN);            // Monitor readability
 * @endcode
 *
 * When an fd no longer needs to be monitored, the File Descriptor Monitor object
 * is deleted by calling le_fdMonitor_Delete().
 *
 * @code
 *
    le_fdMonitor_Delete(fdMonitor);

 * @endcode
 *
 * @warning Always delete the Monitor object for an fd <b> before closing the fd </b>. After an
 *          fd is closed, it could get reused for something completely different. If monitoring of
 *          the new fd incarnation is started before the old Monitor object is deleted, deleting
 *          the old Monitor will cause monitoring of the new incarnation to fail.
 *
 *
 * @section c_fdMonitorEvents Event Types
 *
 * Events that can be handled:
 *
 * - @c POLLIN = Data available to read.
 * - @c POLLPRI = Urgent data available to read (e.g., out-of-band data on a socket).
 * - @c POLLOUT = Writing to the fd should accept some data now.
 * - @c POLLRDHUP = Other end of stream socket closed or shutdown.
 * - @c POLLERR = Error occurred.
 * - @c POLLHUP = Hang up.
 *
 * These are bitmask values and can be combined using the bit-wise OR operator ('|') and tested
 * for using the bit-wise @e and ('&') operator.
 *
 * @note @c POLLRDHUP, @c POLLERR and @c POLLHUP can't be disabled.
 * Monitoring these events is always enabled as soon as the File Descriptor
 * Monitor is created regardless of the set of events given to le_fdMonitor_Create().
 *
 * @section c_fdTypes FD Types
 *
 * The fd type affects how events are monitored:
 *
 * - @ref c_fdTypes_files
 * - @ref c_fdTypes_pipes
 * - @ref c_fdTypes_sockets
 * - @ref c_fdTypes_terminals
 *
 * @subsection c_fdTypes_files Files
 *
 * - POLLIN and POLLOUT are always SET
 * - NONE of the other EVENTS are ever set
 *
 * @subsection c_fdTypes_pipes Pipes
 *
 * Pipe fd events indicate two conditions for reading from a pipe and two conditions for writing to
 * a pipe.
 *
 * |                      |  Event            |   Condition                                     |
 * | ---------------------| ----------------- | ----------------------------------------------- |
 * | READING from a pipe  | POLLHUP           | NO DATA in the pipe and the WRITE END is closed |
 * |                      | POLLIN            | DATA in the pipe and the WRITE END is open      |
 * |                      | POLLIN + POLLHUP  | DATA in the pipe BUT the WRITE END is closed    |
 * | WRITING to the pipe  | POLLERR           | NO SPACE in the pipe and the READ END is closed |
 * |                      | POLLOUT           | SPACE in the pipe and the READ END is open      |
 * |                      | POLLOUT + POLLERR | SPACE in the pipe BUT the READ END is closed    |
 *
 * @subsection c_fdTypes_sockets Sockets
 *
 * Socket activity (establishing/closing) is monitored for connection-orientated sockets including
 * SOCK_STREAM and SOCK_SEQPACKET. Input and output data availability for all socket types is
 * monitored.
 * | Event                        | Condition                                                    |
 * | ---------------------------- | ------------------------------------------------------------ |
 * | POLLIN                       | Input is available from the socket                           |
 * | POLLOUT                      | Possible to send data on the socket                          |
 * | POLLIN                       | Incoming connection being established on the listen port     |
 * | POLLPRI                      | Out of band data received only on TCP                        |
 * | POLLIN + POLLOUT + POLLRDHUP | Peer closed the connection in a connection-orientated socket |
 *
 *
 * @subsection c_fdTypes_terminals Terminals and Pseudo-Terminals
 *
 * Terminals and pseudo-terminals operate in pairs. When one terminal pair closes, an event is
 * generated to indicate the closure. POLLIN, POLLOUT and POLLPRI are the event indicators related
 * to terminal status.
 *
 * | Event   | Condition             |
 * | ------- | --------------------- |
 * | POLLIN  | Ready to receive data |
 * | POLLOUT | Ready to send data    |
 * | POLLPRI | Master/pseudo terminal detects slave state has changed (in packet mode only). |
 * | POLLHUP | Either half of the terminal pair has closed. |
 *
 *
 * @section c_fdMonitorHandlers Handler Functions
 *
 * Parameters to the fd event handler functions are the fd and the events active for the
 * fd. The events are passed as a bit mask; the bit-wise AND operator ('&') must be used to
 * check for specific events.
 *
 * @code

COMPONENT_INIT
{
    // Open the serial port.
    int fd = open("/dev/ttyS0", O_RDWR|O_NONBLOCK);
    LE_FATAL_IF(fd == -1, "open failed with errno %d", errno);

    // Create a File Descriptor Monitor object for the serial port's file descriptor.
    // Monitor for data available to read.
    le_fdMonitor_Ref_t fdMonitor = le_fdMonitor_Create("Serial Port",      // Name for diagnostics
                                                       fd,                 // fd to monitor
                                                       SerialPortHandler,  // Handler function
                                                       POLLIN);            // Monitor readability
}

static void SerialPortHandler(int fd, short events)
{
    if (events & POLLIN)    // Data available to read?
    {
        char buff[MY_BUFF_SIZE];

        ssize_t bytesRead = read(fd, buff, sizeof(buff));

        ...
    }

    if ((events & POLLERR) || (events & POLLHUP) || (events & POLLRDHUP))   // Error or hang-up?
    {
        ...
    }
}

 * @endcode
 *
 * @section c_fdMonitorEnableDisable Enable/Disable Event Monitoring
 *
 * The set of fd events being monitored can be adjusted using le_fdMonitor_Enable()
 * and le_fdMonitor_Disable().  However, @c POLLRDHUP, @c POLLERR and @c POLLHUP can't be disabled.
 *
 * CPU cycles (and power) can be saved by disabling monitoring when not needed.
 * For example, @c POLLOUT monitoring should be disabled while nothing needs to be written
 * to the fd, so that the event handler doesn't keep getting called with a @c POLLOUT event
 * because the fd is writeable.
 *
 * @code
static void StartWriting()
{
    // Enable monitoring for POLLOUT.  When connection is ready, handler will be called.
    le_fdMonitor_Enable(FdMonitorRef, POLLOUT);
}

static void ConnectionEventHandler(int fd, int event)
{
    if (event & POLLOUT)
    {
        // Connection is ready for us to send some data.
        le_result_t result = SendWaitingData();
        if (result == LE_NOT_FOUND)
        {
            // Buffer empty, stop monitoring POLLOUT so handler doesn't keep getting called.
            le_fdMonitor_Disable(le_fdMonitor_GetMonitor(), POLLOUT);
        }
        ...
    }
    ...
}
 * @endcode
 *
 * If an event occurs on an fd while monitoring of that event is disabled,
 * the event will be ignored. If that event is later enabled, and that event's
 * trigger condition is still true (e.g., the fd still has data available to be read), then the
 * event will be reported to the handler at that time. If the event trigger condition is gone
 * (e.g., the fd no longer has data available to read), then the event will not be reported
 * until its trigger condition becomes true again.
 *
 * If events occur on different fds at the same time, the order in which the handlers
 * are called is implementation-dependent.
 *
 *
 * @section c_fdMonitorHandlerContext Handler Function Context
 *
 * Calling le_fdMonitor_GetMonitor() inside the handler function fetches a reference to the
 * File Descriptor Monitor object for the event being handled.  This is handy to enable and
 * disable event monitoring from inside the handler.
 *
 * If additional data needs to be passed to the handler function, the context pointer can be set
 * to use le_fdMonitor_SetContextPtr() and retrieved inside the handler function with
 * le_fdMonitor_GetContextPtr(). le_event_GetContextPtr() can also be used, but
 * le_fdMonitor_GetContextPtr() is preferred as it double checks it's being called inside
 * a File Descriptor Monitor's handler function.
 *
 * @code
 *
 * static void SerialPortHandler(int fd, short events)
 * {
 *     MyContext_t* contextPtr = le_fdMonitor_GetContextPtr();
 *
 *     // Process the fd event(s).
 *     ...
 * }
 *
 * static void StartDataTransmission(const char* port, uint8_t* txBuffPtr, size_t txBytes)
 * {
 *     // Open the serial port.
 *     int fd = open(port, O_RDWR|O_NONBLOCK);
 *     LE_FATAL_IF(fd == -1, "open failed with errno %d", errno);
 *
 *     // Create a File Descriptor Monitor object for the serial port's file descriptor.
 *     // Monitor for write buffer space availability.
 *     le_fdMonitor_Ref_t fdMonitor = le_fdMonitor_Create("Port", fd, SerialPortHandler, POLLOUT);
 *
 *     // Allocate a data block and populate with stuff we need in SerialPortHandler().
 *     MyContext_t* contextPtr = le_mem_ForceAlloc(ContextMemPool);
 *     contextPtr->txBuffPtr = txBuffPtr;
 *     contextPtr->bytesRemaining = txBytes;
 *
 *     // Make this available to SerialPortHandler() via le_fdMonitor_GetContextPtr().
 *     le_fdMonitor_SetContextPtr(fdMonitor, contextPtr);
 * }
 *
 * @endcode
 *
 *
 * @section c_fdMonitorPowerManagement Power Management
 *
 * If your process has the privilege of being able to block the system from going to sleep,
 * whenever the fd that is being monitored has a pending event, the system will be kept
 * awake.  To allow the system to go to sleep while this fd has a pending event, you can call
 * le_fdMonitor_SetDeferrable() with @c isDeferrable flag set to 'true'.
 *
 *
 * @section c_fdMonitorThreading Threading
 *
 * fd monitoring is performed by the Event Loop of the thread that
 * created the Monitor object for that fd. If that the is blocked, events
 * won't be detected for that fd until the thread is unblocked and returns to its
 * Event Loop. Similarly, if the thread that creates a File Descriptor Monitor object doesn't
 * run an Event Loop at all, no events will be detected for that fd.
 *
 * It's not recommended to monitor the same fd in two threads at the same time, because the threads
 * will race to handle any events on that fd.
 *
 *
 * @section c_fdMonitorTroubleshooting Troubleshooting
 *
 * The "fdMonitor" logging keyword can be enabled to view fd monitoring activity.
 *
 * <hr>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/** @file le_fdMonitor.h
 *
 * Legato @ref c_fdMonitor include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_FDMONITOR_INCLUDE_GUARD
#define LEGATO_FDMONITOR_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * File Descriptor Monitor reference.
 *
 * Used to refer to File Descriptor Monitor objects.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_fdMonitor* le_fdMonitor_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 * Prototype for file descriptor event handler functions.
 *
 * Events that can be received:
 *
 * - @c POLLIN = Data available to read.
 * - @c POLLPRI = Urgent data available to read (e.g., out-of-band data on a socket).
 * - @c POLLOUT = Writing to the fd should accept some data now.
 * - @c POLLRDHUP = Other end of stream socket closed or shutdown.
 * - @c POLLERR = Error occurred.
 * - @c POLLHUP = Hang up.
 *
 * These are bitmask values that may appear in the @c events parameter.  Use the bit-wise AND
 * operator ('&') to test for specific events.
 *
 * @code
 *
 * if (events & POLLIN)
 * {
 *     // Data available to read.
 *     ...
 * }
 *
 * if (events & POLLERR)
 * {
 *     // An error occured.
 *     ...
 * }
 *
 * ...
 *
 * @endcode
 *
 * @param fd        [in] File descriptor.
 *
 * @param events    [in] Bit map of events that occurred.  Use bitwise AND ('&') to test for events.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_fdMonitor_HandlerFunc_t)
(
    int   fd,
    short events
);


#if LE_CONFIG_FD_MONITOR_NAMES_ENABLED
//--------------------------------------------------------------------------------------------------
/**
 * Creates a File Descriptor Monitor.
 *
 * Creates an object that will monitor a given file descriptor for events.
 *
 * The monitoring will be performed by the event loop of the thread that created the Monitor object.
 * If that thread is blocked, no events will be detected for that file descriptor until that
 * thread is unblocked and returns to its event loop.
 *
 * Events that can be enabled for monitoring:
 *
 * - @c POLLIN = Data available to read.
 * - @c POLLPRI = Urgent data available to read (e.g., out-of-band data on a socket).
 * - @c POLLOUT = Writing to the fd should accept some data now.
 *
 * These are bitmask values and can be combined using the bit-wise OR operator ('|').
 *
 * The following events are always monitored, even if not requested:
 *
 * - @c POLLRDHUP = Other end of stream socket closed or shutdown.
 * - @c POLLERR = Error occurred.
 * - @c POLLHUP = Hang up.
 *
 *  @param[in]  name        Name of the object (for diagnostics).
 *  @param[in]  fd          File descriptor to be monitored for events.
 *  @param[in]  handlerFunc Handler function.
 *  @param[in]  events      Initial set of events to be monitored.
 *
 * @return
 *      Reference to the object, which is needed for later deletion.
 *
 * @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
le_fdMonitor_Ref_t le_fdMonitor_Create
(
    const char                  *name,
    int                          fd,
    le_fdMonitor_HandlerFunc_t   handlerFunc,
    short                        events
);
#else /* if not LE_CONFIG_FD_MONITOR_NAMES_ENABLED */
/// @cond HIDDEN_IN_USER_DOCS
//--------------------------------------------------------------------------------------------------
/**
 * Internal function used to implement le_fdMonitor_Create().
 */
//--------------------------------------------------------------------------------------------------
le_fdMonitor_Ref_t _le_fdMonitor_Create
(
    int                          fd,
    le_fdMonitor_HandlerFunc_t   handlerFunc,
    short                        events
);
/// @endcond
//--------------------------------------------------------------------------------------------------
/**
 * Creates a File Descriptor Monitor.
 *
 * Creates an object that will monitor a given file descriptor for events.
 *
 * The monitoring will be performed by the event loop of the thread that created the Monitor object.
 * If that thread is blocked, no events will be detected for that file descriptor until that
 * thread is unblocked and returns to its event loop.
 *
 * Events that can be enabled for monitoring:
 *
 * - @c POLLIN = Data available to read.
 * - @c POLLPRI = Urgent data available to read (e.g., out-of-band data on a socket).
 * - @c POLLOUT = Writing to the fd should accept some data now.
 *
 * These are bitmask values and can be combined using the bit-wise OR operator ('|').
 *
 * The following events are always monitored, even if not requested:
 *
 * - @c POLLRDHUP = Other end of stream socket closed or shutdown.
 * - @c POLLERR = Error occurred.
 * - @c POLLHUP = Hang up.
 *
 *  @param[in]  name        Name of the object (for diagnostics).
 *  @param[in]  fd          File descriptor to be monitored for events.
 *  @param[in]  handlerFunc Handler function.
 *  @param[in]  events      Initial set of events to be monitored.
 *
 * @return
 *      Reference to the object, which is needed for later deletion.
 *
 * @note Doesn't return on failure, there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
LE_DECLARE_INLINE le_fdMonitor_Ref_t le_fdMonitor_Create
(
    const char                  *name,
    int                          fd,
    le_fdMonitor_HandlerFunc_t   handlerFunc,
    short                        events
)
{
    LE_UNUSED(name);
    return _le_fdMonitor_Create(fd, handlerFunc, events);
}
#endif /* end LE_CONFIG_FD_MONITOR_NAMES_ENABLED */


//--------------------------------------------------------------------------------------------------
/**
 * Enables monitoring for events on a file descriptor.
 *
 * Events that can be enabled for monitoring:
 *
 * - @c POLLIN = Data available to read.
 * - @c POLLPRI = Urgent data available to read (e.g., out-of-band data on a socket).
 * - @c POLLOUT = Writing to the fd should accept some data now.
 *
 * These are bitmask values and can be combined using the bit-wise OR operator ('|').
 */
//--------------------------------------------------------------------------------------------------
void le_fdMonitor_Enable
(
    le_fdMonitor_Ref_t  monitorRef, ///< [in] Reference to the File Descriptor Monitor object.
    short               events      ///< [in] Bit map of events.
);


//--------------------------------------------------------------------------------------------------
/**
 * Disables monitoring for events on a file descriptor.
 *
 * Events that can be disabled for monitoring:
 *
 * - @c POLLIN = Data available to read.
 * - @c POLLPRI = Urgent data available to read (e.g., out-of-band data on a socket).
 * - @c POLLOUT = Writing to the fd should accept some data now.
 *
 * These are bitmask values and can be combined using the bit-wise OR operator ('|').
 */
//--------------------------------------------------------------------------------------------------
void le_fdMonitor_Disable
(
    le_fdMonitor_Ref_t  monitorRef, ///< [in] Reference to the File Descriptor Monitor object.
    short               events      ///< [in] Bit map of events.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets if processing of events on a given fd is deferrable (the system is allowed to go to
 * sleep while there are monitored events pending for this fd) or urgent (the system will be kept
 * awake until there are no monitored events waiting to be handled for this fd).
 *
 * If the process has @c CAP_EPOLLWAKEUP (or @c CAP_BLOCK_SUSPEND) capability, then fd events are
 * considered urgent by default.
 *
 * If the process doesn't have @c CAP_EPOLLWAKEUP (or @c CAP_BLOCK_SUSPEND) capability, then fd
 * events are always deferrable, and calls to this function have no effect.
 */
//--------------------------------------------------------------------------------------------------
void le_fdMonitor_SetDeferrable
(
    le_fdMonitor_Ref_t monitorRef,  ///< [in] Reference to the File Descriptor Monitor object.
    bool               isDeferrable ///< [in] true (deferrable) or false (urgent).
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the Context Pointer for File Descriptor Monitor's handler function.  This can be retrieved
 * by the handler using le_fdMonitor_GetContextPtr() (or le_event_GetContextPtr()) when the handler
 * function is running.
 */
//--------------------------------------------------------------------------------------------------
void le_fdMonitor_SetContextPtr
(
    le_fdMonitor_Ref_t  monitorRef, ///< [in] Reference to the File Descriptor Monitor.
    void*               contextPtr  ///< [in] Opaque context pointer value.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the Context Pointer for File Descriptor Monitor's handler function.
 *
 * @return  The context pointer set using le_fdMonitor_SetContextPtr(), or NULL if it hasn't been
 *          set.
 *
 * @note    This only works inside the handler function.  The difference between this function and
 *          le_event_GetContextPtr() is that le_fdMonitor_GetContextPtr() will double check that
 *          it's being called inside of a File Descriptor Monitor's handler function.
 */
//--------------------------------------------------------------------------------------------------
void* le_fdMonitor_GetContextPtr
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets the file descriptor that an FD Monitor object is monitoring.
 *
 * @return  The fd.
 */
//--------------------------------------------------------------------------------------------------
int le_fdMonitor_GetFd
(
    le_fdMonitor_Ref_t  monitorRef  ///< [in] Reference to the File Descriptor Monitor.
);


//--------------------------------------------------------------------------------------------------
/**
 * Gets a reference to the File Descriptor Monitor whose handler function is currently running.
 *
 * @return  File Descriptor Monitor reference.
 *
 * @note    This only works inside the handler function.
 **/
//--------------------------------------------------------------------------------------------------
le_fdMonitor_Ref_t le_fdMonitor_GetMonitor
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a file descriptor monitor object.
 */
//--------------------------------------------------------------------------------------------------
void le_fdMonitor_Delete
(
    le_fdMonitor_Ref_t monitorRef  ///< [in] Reference to the File Descriptor Monitor object.
);


#endif // LEGATO_FDMONITOR_INCLUDE_GUARD
