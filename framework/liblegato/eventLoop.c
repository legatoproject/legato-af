//--------------------------------------------------------------------------------------------------
/** @file eventLoop.c
 *
 * Legato @ref c_eventLoop implementation.
 *
 * ----
 *
 * @section eventLoop_EventIds  Event IDs
 *
 * Note that Event IDs are actually Safe References (see @ref c_safeRef).
 * Safe References are also used for Handler References.
 *
 * ----
 *
 * @section eventLoop_DataStructures    Data Structures
 *
 * The main classes in this implementation are:
 *
 *  - <b> Events </b> - One per Event ID, these keep track of the ID and what Handlers are
 *                  registered against them.  They also each have a pool from which their
 *                  Reports are allocated.  Note: Events are never deleted.
 *
 *  - <b> Handlers </b> - One per registered handler function.  These keep track of the function,
 *                  the diagnostic name, the context pointer, and what thread is supposed to run
 *                  the handler.
 *
 *  - <b> Reports </b> - Objects containing actual event report payload.  These objects are what
 *                  get queued onto a thread's Event Queue.
 *
 * In addition, thread-specific data structures are kept in a special <b> Per-Thread Record </b>,
 * which the thread module keeps inside the Thread object on our behalf (we can fetch a pointer
 * to it by calling thread_GetEventRecPtr() ).  See @ref eventLoop.h for more information.
 *
 * @verbatim
 *
 *      Thread ---> Per-Thread Record --+--> Event Queue --+--> Report
 *         ^                            |
 *         |                            +--> Handler List --+---------+
 *         |                                                          |
 *         +----------------------------------------------------+     |
 *                                                              |     |
 *                          +-------------------------------+   |     |
 *                          |                               |   |     |
 *                          V                               |   |     |
 *      Event List --+--> Event --+--> Handler List --+--> Handler <--+
 *                                |
 *                                +--> ID
 *                                |
 *                                +--> Report Pool
 * @endverbatim
 *
 *
 * ----
 *
 * @section eventLoop_Multithreading    Multithreading
 *
 * Everything can be shared between multiple threads, and therefore must be protected from
 * multithreaded race conditions.  A Mutex is provided for that purpose, and it can be locked
 * and unlocked using the functions event_Lock() and event_Unlock().  Framework adaptor
 * functions which end in _NoLock are called with the lock held so should not lock.
 *
 * ----
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "eventLoop.h"
#include "thread.h"
#include "fdMonitor.h"
#include "limit.h"

// ==============================================
//  PRIVATE DATA
// ==============================================

/// Maximum number of events that can be received from epoll_wait() at one time.
#define MAX_EPOLL_EVENTS 32

/// The largest report size to be allocted out of the standard report pool.  Reports larger
/// than this have a separate pool created for each report types
#define HIGH_REPORT_OBJECT_SIZE   512

//--------------------------------------------------------------------------------------------------
/**
 * Insert a string name variable if configured or a placeholder string if not.
 *
 *  @param  nameVar Name variable to insert.
 *
 *  @return Name variable or a placeholder string depending on configuration.
 **/
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_EVENT_NAMES_ENABLED
#   define  EVENT_NAME(var) (var)
#else
#   define  EVENT_NAME(var) "<omitted>"
#endif


//--------------------------------------------------------------------------------------------------
// Create definitions for inlineable functions
//
// See le_eventLoop.h for bodies & documentation
//--------------------------------------------------------------------------------------------------
#if !LE_CONFIG_EVENT_NAMES_ENABLED
LE_DEFINE_INLINE le_event_Id_t le_event_CreateId
(
    const char *name,
    size_t      payloadSize
);

LE_DEFINE_INLINE le_event_Id_t le_event_CreateIdWithRefCounting
(
    const char *name
);

LE_DEFINE_INLINE le_event_HandlerRef_t le_event_AddHandler
(
    const char              *name,
    le_event_Id_t            eventId,
    le_event_HandlerFunc_t   handlerFunc
);

LE_DEFINE_INLINE le_event_HandlerRef_t le_event_AddLayeredHandler
(
    const char                      *name,
    le_event_Id_t                    eventId,
    le_event_LayeredHandlerFunc_t    firstLayerFunc,
    void*                            secondLayerFunc
);
#endif


//--------------------------------------------------------------------------------------------------
/**
 * Event object
 *
 * These objects are allocated from the Event Pool and stored in the Event List whenever someone
 * creates a new Event ID.  They store the name of the event and the ID.  They also keep the
 * list of all Handlers that have been registered for that event.
 *
 * @warning Once this has been placed in the Event List, it can be accessed by multiple threads.
 *          After that, the Mutex must be used to protect it and everything in it from races.
 *
 * @note    These objects are never deleted.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sls_Link_t       link;                   ///< Used to link into the Event List.
    void*               id;                     ///< The Event ID (safe ref) assigned to this event.
    le_dls_List_t       handlerList;            ///< List of Handlers registered for this event.
#if LE_CONFIG_EVENT_NAMES_ENABLED
    char                name[LIMIT_MAX_EVENT_NAME_BYTES]; ///< The name of the event.
#endif
    le_mem_PoolRef_t    reportPoolRef;          ///< Pool for this event's Report objects.
    size_t              payloadSize;            ///< Size of the Report payload, in bytes.
    bool                isRefCounted;           ///< true = payload is a ref-counted object pointer.
}
Event_t;


//--------------------------------------------------------------------------------------------------
/**
 * Static pool for events.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(Events, LE_CONFIG_MAX_EVENT_POOL_SIZE, sizeof(Event_t));


//--------------------------------------------------------------------------------------------------
/**
 * Event Pool
 *
 * Pool from which Event objects are allocated.
 *
 * @note Pools are thread safe.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t EventPool;


//--------------------------------------------------------------------------------------------------
/**
 * Event List
 *
 * This stores all the Event objects in the process.  It is mainly here for diagnostics
 * tools to use.
 *
 * @warning This can be accessed by multiple threads.  Use the Mutex to protect it from races.
 */
//--------------------------------------------------------------------------------------------------
static le_sls_List_t EventList = LE_SLS_LIST_INIT;


//--------------------------------------------------------------------------------------------------
/**
 * Handler object.
 *
 * This stores the registration information for a handler function.  They are allocated from the
 * Handler Pool and are stored on a thread's Handler List.
 *
 * @warning These can be accessed by multiple threads, and are in both the Event List structure
 *          and the Per-Thread structure.  Great care must be taken to prevent races when accessing
 *          these objects (use the Mutex).
 *
 * @note    The lifecycle of these objects is such that once they have been created, only their
 *          list links can be changed, until they are deleted.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t           eventLink;  ///< Used to link onto an event's Handler List.
    le_dls_Link_t           threadLink; ///< Used to link onto a thread's Handler List.
    event_PerThreadRec_t*   threadRecPtr;///< Ptr to per-thread rec of thread that will run this.
    Event_t*                eventPtr;   ///< Ptr to the Event obj for the event that this handles.
    void*                   contextPtr; ///< The context pointer for this handler.
    void*                   safeRef;    ///< Safe Reference for this object.
#if LE_CONFIG_EVENT_NAMES_ENABLED
    char                    name[LIMIT_MAX_EVENT_HANDLER_NAME_BYTES];///< UTF-8 name of the handler.
#endif

    le_event_LayeredHandlerFunc_t   firstLayerFunc;     ///< First-layer handler function.
    void*                           secondLayerFunc;    ///< Second-layer handler function.
}
Handler_t;


//--------------------------------------------------------------------------------------------------
/**
 * Static pool for event handlers
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(EventHandler,
                          LE_CONFIG_MAX_HANDLER_POOL_SIZE,
                          sizeof(Handler_t));

//--------------------------------------------------------------------------------------------------
/**
 * Handler Pool
 *
 * This is the pool from which Handler objects are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t HandlerPool;


//--------------------------------------------------------------------------------------------------
/**
 * Enumerates the different types of Event Report.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    LE_EVENT_REPORT_PLAIN,          ///< Publish-Subscribe Event Report containing plain-old data.

    LE_EVENT_REPORT_COUNTED_REF,    ///< Publish-Subscribe Event Report containing poiner to
                                    ///  reference-counted object allocated from a memory pool.

    LE_EVENT_REPORT_QUEUED_FUNC,    ///< Queued Function.
}
EventReportType_t;


//--------------------------------------------------------------------------------------------------
/**
 * Event Report base class.  This is the data that is common to all of the different types
 * of Event Report.
 *
 * @warning These can be accessed by multiple threads, via the Per-Thread structure.
 *          Great care must be taken to prevent races when accessing these objects.
 *
 * @note    The lifecycle of these objects is such that once they have been queued to an
 *          Event Queue, only the thread that is processing that Event Queue can access them.
 *
 * @note    Because an event's Handler can be deleted while a Report for that event is waiting
 *          in an Event Queue, the Report keeps a safe reference to that Handler instead of
 *          a pointer to that Handler.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_sls_Link_t           link;       ///< Used to link onto an Event Queue.
    EventReportType_t       type;       ///< Indicates what type of event report this is.
}
Report_t;


//--------------------------------------------------------------------------------------------------
/**
 * Publish-Subscribe Event Report.
 *
 * Each Handler has its own pool from which these types of Event Reports are allocated.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    Report_t                baseClass;  ///< Part that is common to all types of report.
    le_event_HandlerRef_t   handlerRef; ///< Safe Reference to the handler for this event.
    void*                   payload[];  ///< If the report has payload, it comes at the end.
}
PubSubEventReport_t;


//--------------------------------------------------------------------------------------------------
/**
 * Queued Function.
 *
 * This type of Event Report is allocated from the process-wide Queued Function Pool.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    Report_t                baseClass;  ///< Part that is common to all types of report.
    le_event_DeferredFunc_t function;   ///< Address of the function to be called.
    void*                   param1Ptr;  ///< First parameter to pass to the function.
    void*                   param2Ptr;  ///< Second parameter to pass to the function.
}
QueuedFunctionReport_t;


//--------------------------------------------------------------------------------------------------
/**
 * Static pool for Queued Function Event Reports.
 */
//--------------------------------------------------------------------------------------------------
LE_MEM_DEFINE_STATIC_POOL(ReportPool,
                          LE_CONFIG_MAX_QUEUED_FUNCTION_POOL_SIZE,
                          HIGH_REPORT_OBJECT_SIZE);

//--------------------------------------------------------------------------------------------------
/**
 * Pool from which Queued Function Event Reports and small PubSub Event Reports are allocated.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t ReportPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Static safe reference map to be used to create safe references to use as Event IDs.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(Events, LE_CONFIG_MAX_EVENT_POOL_SIZE);

//--------------------------------------------------------------------------------------------------
/**
 * The Safe Reference Map to be used to create Safe References to use as Event IDs.
 *
 * @warning This can be accessed by multiple threads.  Use the Mutex to protect it from races.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t EventRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * The static safe reference map to be used to create handler references.
 */
//--------------------------------------------------------------------------------------------------
LE_REF_DEFINE_STATIC_MAP(EventHandlers, LE_CONFIG_MAX_QUEUED_FUNCTION_POOL_SIZE);


//--------------------------------------------------------------------------------------------------
/**
 * The Safe Reference Map to be used to create Handler References.
 *
 * @warning This can be accessed by multiple threads.  Use the Mutex to protect it from races.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t HandlerRefMap;


//--------------------------------------------------------------------------------------------------
/**
 * Mutex is used to protect all data structures, other than the Init Handler List, from
 * multithreaded race conditions.  Threads wishing to access anything under the Event List or
 * the Per-Thread Records must hold this lock while doing so.
 */
//--------------------------------------------------------------------------------------------------
static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.


//--------------------------------------------------------------------------------------------------
/**
 * Guards against thread cancellation and locks the mutex.
 *
 * @return Old state of cancelability.
 **/
//--------------------------------------------------------------------------------------------------
int event_Lock
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    int oldState;

    int err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldState);

    LE_FATAL_IF(err != 0, "pthread_setcancelstate() failed (%s)", strerror(err));

    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0);

    return oldState;
}


//--------------------------------------------------------------------------------------------------
/**
 * Unlocks the mutex and releases the thread cancellation guard created by event_Lock().
 **/
//--------------------------------------------------------------------------------------------------
void event_Unlock
(
    int restoreTo   ///< Old state of cancellability to be restored.
)
//--------------------------------------------------------------------------------------------------
{
    int junk;

    LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);

    int err = pthread_setcancelstate(restoreTo, &junk);
    LE_FATAL_IF(err != 0, "pthread_setcancelstate() failed (%s)", strerror(err));
}


//--------------------------------------------------------------------------------------------------
/**
 * Wait for a condition to fire.
 *
 * The event lock must be held before calling this function.
 */
//--------------------------------------------------------------------------------------------------
void event_CondWait
(
    pthread_cond_t* cond      ///< [IN] Condition to wait for.
)
{
    LE_ASSERT(pthread_cond_wait(cond, &Mutex) == 0);
}

//--------------------------------------------------------------------------------------------------
/**
 * Wait for a condition to fire until timeout occurs.
 *
 * The event lock must be held before calling this function.
 */
//--------------------------------------------------------------------------------------------------
int event_CondTimedWait
(
    pthread_cond_t* cond,           ///< [IN] Condition to wait for.
    const struct timespec* timePtr  ///< [IN] Time to wait for condition before returning.
)
{
    return pthread_cond_timedwait(cond, &Mutex, timePtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Trace reference used for controlling tracing in this module.
 **/
//--------------------------------------------------------------------------------------------------
static le_log_TraceRef_t TraceRef;

/// Macro used to generate trace output in this module.
/// Takes the same parameters as LE_DEBUG() et. al.
#define TRACE(...) LE_TRACE(TraceRef, ##__VA_ARGS__)


// ==============================================
//  PRIVATE FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
/**
 * Create a new Event object.
 *
 * @return
 *      Pointer to the Event object.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
static Event_t* CreateEvent
(
#if LE_CONFIG_EVENT_NAMES_ENABLED
    const char* name,       ///< [in] Name of the event ID.  (Named for diagnostic purposes.)
#endif
    size_t      payloadSize,///< [in] Data payload size (in bytes) of the event reports (can be 0).
    bool        isRefCounted///< [in] true = the payload will be a pointer to a ref-counted object.
)
//--------------------------------------------------------------------------------------------------
{
    Event_t*    eventPtr = le_mem_ForceAlloc(EventPool);
#if LE_CONFIG_EVENT_NAMES_ENABLED
    le_result_t result;
#endif

    eventPtr->link = LE_SLS_LINK_INIT;
    eventPtr->handlerList = LE_DLS_LIST_INIT;

#if LE_CONFIG_EVENT_NAMES_ENABLED
    result = le_utf8_Copy(eventPtr->name, name, sizeof(eventPtr->name), NULL);
    if(result == LE_OVERFLOW)
    {
        LE_WARN("Event name '%s' truncated to '%s'.", name, eventPtr->name);
    }
#endif /* end LE_CONFIG_EVENT_NAMES_ENABLED */

    eventPtr->payloadSize = payloadSize;
    eventPtr->isRefCounted = isRefCounted;

    // Create the memory pool from which reports for this event are to be allocated.
    // Note: We can't delete pools, so we don't allow Event Ids to be deleted.
    /// @todo Make this configurable.
    char poolNameStr[LIMIT_MAX_EVENT_NAME_BYTES + 8] = "evtPool";
#if LE_CONFIG_EVENT_NAMES_ENABLED
    size_t bytesCopied;
    le_utf8_Copy(poolNameStr, eventPtr->name, sizeof(poolNameStr), &bytesCopied);
    if (LE_OVERFLOW == le_utf8_Copy(poolNameStr + bytesCopied,
                                    "-reports",
                                    sizeof(poolNameStr) - bytesCopied,
                                    NULL) )
    {
        LE_WARN("Event report pool name truncated for '%s' events.", name);
    }
#endif /* end LE_CONFIG_EVENT_NAMES_ENABLED */
    if (sizeof(PubSubEventReport_t) + payloadSize <= HIGH_REPORT_OBJECT_SIZE)
    {
        // Use standard report pool
        eventPtr->reportPoolRef = ReportPoolRef;
    }
    else
    {
        eventPtr->reportPoolRef = le_mem_CreatePool(poolNameStr,
                                                    sizeof(PubSubEventReport_t) +
                                                    payloadSize);
    }

    // Up until now, we have not accessed anything that is available to anyone else; except for
    // the EventPool, but that is thread-safe.  But, now we need to touch the Safe Reference Map
    // and the Event List, and those are shared by other threads.  So, it's time to lock the Mutex.

    int oldState = event_Lock();

    // Create a Safe Reference to be used as the Event ID.
    eventPtr->id = le_ref_CreateRef(EventRefMap, eventPtr);

    // Add the Event object to the Event List.
    le_sls_Queue(&EventList, &eventPtr->link);

    event_Unlock(oldState);

    return eventPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deletes a Handler object.
 *
 * @warning Assumes that the Mutex lock is already held.
 */
//--------------------------------------------------------------------------------------------------
static void DeleteHandler
(
    Handler_t* handlerPtr   ///< [in] Pointer to the handler to be deleted.
)
//--------------------------------------------------------------------------------------------------
{
    le_dls_Remove(&handlerPtr->eventPtr->handlerList, &handlerPtr->eventLink);
    le_dls_Remove(&handlerPtr->threadRecPtr->handlerList, &handlerPtr->threadLink);
    le_ref_DeleteRef(HandlerRefMap, handlerPtr->safeRef);
    le_mem_Release(handlerPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Process one event report from the calling thread's Event Queue.
 **/
//--------------------------------------------------------------------------------------------------
void event_ProcessOneEventReport
(
    event_PerThreadRec_t* perThreadRecPtr   ///< [in] Ptr to the calling thread's per-thread record.
)
//--------------------------------------------------------------------------------------------------
{
    le_sls_Link_t* linkPtr;
    Report_t* reportObjPtr;
    Handler_t* handlerPtr;

    int oldState = event_Lock();

    // Pop an Event Report off the head of the Event Queue (inside a critical section).
    linkPtr = le_sls_Pop(&perThreadRecPtr->eventQueue);

    event_Unlock(oldState);

    if (linkPtr == NULL)
    {
        return;
    }

    // Convert the link pointer into a pointer to the Report base class.
    reportObjPtr = CONTAINER_OF(linkPtr, Report_t, link);

    // If it's a queued function report,
    if (reportObjPtr->type == LE_EVENT_REPORT_QUEUED_FUNC)
    {
        // Convert the Report base class pointer into a pointer to a Queued Function Report.
        QueuedFunctionReport_t* queuedFuncReportPtr;
        queuedFuncReportPtr = CONTAINER_OF(reportObjPtr, QueuedFunctionReport_t, baseClass);

        // Call the function.
        queuedFuncReportPtr->function(queuedFuncReportPtr->param1Ptr,
                                      queuedFuncReportPtr->param2Ptr);

    }
    // If it's a publish-subscribe event report,
    else
    {
        // Convert the Report base class pointer into a pointer to a Publish-Subscribe
        // Event Report.
        PubSubEventReport_t* pubSubReportPtr;
        pubSubReportPtr = CONTAINER_OF(reportObjPtr, PubSubEventReport_t, baseClass);

        oldState = event_Lock();

        // Get a pointer to the Handler object for this Event Report; unless it has been removed.
        handlerPtr = le_ref_Lookup(HandlerRefMap, pubSubReportPtr->handlerRef);
        if (handlerPtr == NULL)
        {
            // The handler has been removed, so this report should be discarded.
            event_Unlock(oldState);

            // If its payload is a pointer to a reference-counted memory pool object,
            // then that has to be released.
            if (reportObjPtr->type == LE_EVENT_REPORT_COUNTED_REF)
            {
                le_mem_Release(pubSubReportPtr->payload[0]);
            }
        }
        else
        {
            // The handler still exists, so grab the info we need from it and call
            // the first-layer handler function.
            perThreadRecPtr->contextPtr = handlerPtr->contextPtr;

            le_event_LayeredHandlerFunc_t firstLayerFunc = handlerPtr->firstLayerFunc;
            void* secondLayerFunc = handlerPtr->secondLayerFunc;

            // If it's a reference-counted report, then the payload is a pointer to the
            // report.  Otherwise, the report itself is in the payload.
            void* reportPtr;
            if (reportObjPtr->type == LE_EVENT_REPORT_COUNTED_REF)
            {
                reportPtr = pubSubReportPtr->payload[0];
            }
            else
            {
                reportPtr = pubSubReportPtr->payload;
            }

            event_Unlock(oldState);  // Unlock the mutex before calling the handler function.
                               // Don't access the Handler object anymore after this.

            firstLayerFunc(reportPtr, secondLayerFunc);
        }
    }

    // NOTE: The Mutex should be unlocked by this point.

    // We are done with this report.
    le_mem_Release(reportObjPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Process Event Reports from the calling thread's Event Queue until the queue is empty.
 */
//--------------------------------------------------------------------------------------------------
void event_ProcessEventReports
(
    event_PerThreadRec_t* perThreadRecPtr   ///< [in] Ptr to the calling thread's per-thread record.
)
//--------------------------------------------------------------------------------------------------
{
    // Read the eventfd to fetch the number of Reports on the Event Queue and reset the count
    // to zero.
    uint64_t numReports = fa_event_WaitForEvent(perThreadRecPtr);

    // Process only those event reports that are already on the queue.  Anything reported by the
    // event handlers will have to wait until next time ProcessEventReports() is called.
    // This approach ensures that event handlers that re-queue events to the event
    // queue don't cause fd events to be starved.
    for (; numReports > 0; numReports--)
    {
        event_ProcessOneEventReport(perThreadRecPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * First-layer handler function that is used to implement the single-layer API using the two-layer
 * API.
 *
 * When someone registers a one-layer handler function using le_event_AddHandler(),
 * le_event_AddHandler() calls le_event_AddLayeredHandler() with this function as the
 * first layer handler function and the actual (client) handler function as the second layer
 * function.
 */
//--------------------------------------------------------------------------------------------------
static void PubSubHandlerFunc
(
    void*   reportPtr,      ///< [in] Pointer to the report payload.
    void*   secondLayerFunc ///< [in] Address of the client's handler function.
)
//--------------------------------------------------------------------------------------------------
{
    le_event_HandlerFunc_t clientFunc = (le_event_HandlerFunc_t)secondLayerFunc;

    clientFunc(reportPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Queue a function onto a specific thread's Event Queue (could belong to the calling thread or
 * could belong to some other thread).
 *
 * @warning Assumes the mutex is locked and the thread is protected from cancellation.
 */
//--------------------------------------------------------------------------------------------------
static void QueueFunction_NoLock
(
    event_PerThreadRec_t*   perThreadRecPtr, ///< [in] Pointer to the thread's event data record.
    le_event_DeferredFunc_t func,       ///< [in] The function to be called later.
    void*                   param1Ptr,  ///< [in] Value to be passed to the function when called.
    void*                   param2Ptr   ///< [in] Value to be passed to the function when called.
)
//--------------------------------------------------------------------------------------------------
{
    // Allocate a Queued Function Report object.
    QueuedFunctionReport_t* reportPtr = le_mem_ForceAlloc(ReportPoolRef);

    // Initialize it.
    reportPtr->baseClass.link = LE_SLS_LINK_INIT;
    reportPtr->baseClass.type = LE_EVENT_REPORT_QUEUED_FUNC;
    reportPtr->function = func;
    reportPtr->param1Ptr = param1Ptr;
    reportPtr->param2Ptr = param2Ptr;

    // Queue it to the Event Queue.
    le_sls_Queue(&perThreadRecPtr->eventQueue, &reportPtr->baseClass.link);

    // Write to the eventfd to notify the Event Loop that there is something on the queue.
    fa_event_TriggerEvent_NoLock(perThreadRecPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Queued function that executes a component initialization handler function whose address
 * is passed in as the first parameter to the queued function.
 */
//--------------------------------------------------------------------------------------------------
void CallComponentInitializer
(
    void* param1Ptr,    ///< Pointer to the component's initialization function.
    void* param2Ptr     ///< not used.
)
//--------------------------------------------------------------------------------------------------
{
    void (*componentInitFunc)(void) = param1Ptr;

    LE_UNUSED(param2Ptr);

    componentInitFunc();
}


// ==============================================
//  INTER-MODULE FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Event Loop module.
 *
 * This function must be called exactly once at process start-up, before any other Event module
 * or Event Loop API functions are called.
 */
//--------------------------------------------------------------------------------------------------
void event_Init
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Create the Queued Function Pool from which all Queued Function objects are allocated.
    ReportPoolRef = le_mem_InitStaticPool(ReportPool,
                                          LE_CONFIG_MAX_QUEUED_FUNCTION_POOL_SIZE,
                                          HIGH_REPORT_OBJECT_SIZE);

    // Create the Handler Pool from which all Handler objects are to be allocated.
    HandlerPool = le_mem_InitStaticPool(EventHandler,
                                        LE_CONFIG_MAX_HANDLER_POOL_SIZE,
                                        sizeof(Handler_t));

    // Create the Event Pool from which Event objects are to be allocated.
    EventPool = le_mem_InitStaticPool(Events, LE_CONFIG_MAX_EVENT_POOL_SIZE, sizeof(Event_t));

    // Create the Safe Reference Maps.
    EventRefMap = le_ref_InitStaticMap(Events, LE_CONFIG_MAX_EVENT_POOL_SIZE);
    HandlerRefMap = le_ref_InitStaticMap(EventHandlers, LE_CONFIG_MAX_HANDLER_POOL_SIZE);

    // Get a reference to the trace keyword that is used to control tracing in this module.
    TraceRef = le_log_GetTraceRef("eventLoop");

    // Perform OS-specific initialization
    fa_event_Init();

    // Initialize the FD Monitor module.
    fdMon_Init();
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Event Loop for a given thread.
 *
 * This function must be called exactly once at thread start-up, before any other Event module
 * or Event Loop API functions (other than event_Init() ) are called by that thread.  The
 * return pointer must be stored in the thread's thread-specific info.
 *
 * @note    The process main thread must call event_Init() first, then event_CreatePerThreadInfo().
 */
//--------------------------------------------------------------------------------------------------
event_PerThreadRec_t* event_CreatePerThreadInfo
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // NOTE: This function doesn't touch any data structures that are shared with other threads yet,
    //       so it doesn't need to lock the mutex.  While it's true that the structures initialized
    //       here will eventually be shared with other threads, they will not be shared until this
    //       thread registers a handler function, which it can't do until after it has been
    //       initialized.
    event_PerThreadRec_t* recPtr = fa_event_CreatePerThreadInfo();

    // Initialize the various thread-specific lists and queues.
    recPtr->eventQueue = LE_SLS_LIST_INIT;
    recPtr->handlerList = LE_DLS_LIST_INIT;
    recPtr->fdMonitorList = LE_DLS_LIST_INIT;

    // Set the context pointer to NULL for safety's sake.
    recPtr->contextPtr = NULL;

    // Initialize the FD Monitor module's thread-specific stuff.
    fdMon_InitThread(recPtr);

    // Take note of the fact that the Event Loop for this thread has been initialized, but
    // not started.
    recPtr->state = LE_EVENT_LOOP_INITIALIZED;

    return recPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the Event Loop for a given thread.
 *
 * This function must be called exactly once at thread start-up, before any other Event module
 * or Event Loop API functions (other than event_Init() ) are called by that thread.  This
 * must be called in the thread -- thread_CreatePerThreadInfo is called in the parent thread.
 *
 * @note    The process main thread must call event_Init() first, then event_CreatePerThreadInfo().
 */
//--------------------------------------------------------------------------------------------------
void event_ThreadInit
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    // Perform OS-specific initialization
    fa_event_ThreadInit(thread_GetEventRecPtr());
}


//--------------------------------------------------------------------------------------------------
/**
 * Defer the component initializer for later execution.
 *
 * This function must be called at process start-up, before le_event_Runloop() is called for the
 * main thread.
 *
 * It takes a pointer to a Component Initialization Functions to be called when the event loop is
 * started (i.e., when le_event_RunLoop() is called) for the process's main thread.
 */
//--------------------------------------------------------------------------------------------------
void event_QueueComponentInit
(
    const event_ComponentInitFunc_t func    /// The initialization function to call.
)
//--------------------------------------------------------------------------------------------------
{
    le_event_QueueFunction(CallComponentInitializer, func, NULL);
}


//--------------------------------------------------------------------------------------------------
/**
 * Destruct the Event Loop for a given thread.
 *
 * This function must be called exactly once at thread shutdown, after any other Event module
 * or Event Loop API functions are called by that thread, and before the Thread object is
 * deleted.
 */
//--------------------------------------------------------------------------------------------------
void event_DestructThread
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    event_PerThreadRec_t* perThreadRecPtr = thread_GetEventRecPtr();
    le_dls_Link_t* doubleLinkPtr;
    le_sls_Link_t* singleLinkPtr;

    // Some other thread could be accessing the Event List or structures under it, and we need
    // to access those to remove all of this thread's Handlers from all Events objects'
    // Handler Lists.
    int oldState = event_Lock();

    // Mark the Event Loop as "destructed".  If anyone tries to add something to the Event Queue
    // now, it's a fatal error.
    perThreadRecPtr->state = LE_EVENT_LOOP_DESTRUCTED;

    // Delete all the handlers for this thread.
    while (NULL != (doubleLinkPtr = le_dls_Peek(&perThreadRecPtr->handlerList)))
    {
        Handler_t* handlerPtr = CONTAINER_OF(doubleLinkPtr, Handler_t, threadLink);
        DeleteHandler(handlerPtr);
    }

    // We are finished accessing the Event List and structures under it.  Furthermore,
    // we know that all the handlers have been deleted now, so there's no risk of anyone adding
    // anything to the Event Queue anymore (unless the API user has done something stupid and
    // tries to use another thread to queue something directly to this thread's Event Queue after
    // this thread has started shutting down).  Barring the aforementioned stupid actions,
    // it is now safe to unlock the mutex and allow other threads to run.
    event_Unlock(oldState);

    // Delete all the FD Monitors for this thread.
    fdMon_DestructThread(perThreadRecPtr);

    // Discard everything on the Event Queue.
    while (NULL != (singleLinkPtr = le_sls_Pop(&perThreadRecPtr->eventQueue)))
    {
        Report_t* reportPtr = CONTAINER_OF(singleLinkPtr, Report_t, link);

        // If it is carrying a pointer to a reference-counted object from a memory pool,
        // release that thing first.
        if (reportPtr->type == LE_EVENT_REPORT_COUNTED_REF)
        {
            PubSubEventReport_t* pubSubReportPtr = CONTAINER_OF(reportPtr,
                                                                PubSubEventReport_t,
                                                                baseClass);
            le_mem_Release(pubSubReportPtr->payload[0]);
        }

        le_mem_Release(reportPtr);
    }

    fa_event_DestructThread(perThreadRecPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the context pointer for the currently running thread.
 *
 * This can later be retrieved using le_event_GetContextPtr() from within the same thread.
 */
//--------------------------------------------------------------------------------------------------
void event_SetCurrentContextPtr
(
    void* contextPtr    ///< [in] Context pointer value.
)
//--------------------------------------------------------------------------------------------------
{
    // Don't have to lock before here because only the current running thread can delete
    // its own thread record and access its own contextPtr.

    thread_GetEventRecPtr()->contextPtr = contextPtr;
}


// ==============================================
//  PUBLIC API FUNCTIONS
// ==============================================

//--------------------------------------------------------------------------------------------------
/**
 * Create a new event ID.
 *
 * @return
 *      Event ID.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_EVENT_NAMES_ENABLED
le_event_Id_t le_event_CreateId
(
    const char* name,       ///< [in] Name of the event ID.  (Named for diagnostic purposes.)
    size_t      payloadSize ///< [in] Data payload size (in bytes) of the event reports (can be 0).
)
#else
le_event_Id_t _le_event_CreateId
(
    size_t      payloadSize ///< [in] Data payload size (in bytes) of the event reports (can be 0).
)
#endif
//--------------------------------------------------------------------------------------------------
{
#if LE_CONFIG_EVENT_NAMES_ENABLED
    return CreateEvent(name, payloadSize, false)->id;
#else
    return CreateEvent(payloadSize, false)->id;
#endif
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a new event ID that can be used to report events whose payload is a pointer to
 * a reference-counted memory pool object that was allocated using the @ref c_memory.
 *
 * @return
 *      Event ID.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_EVENT_NAMES_ENABLED
le_event_Id_t le_event_CreateIdWithRefCounting
(
    const char* name        ///< [in] Name of the event ID.  (Named for diagnostic purposes.)
)
#else
le_event_Id_t _le_event_CreateIdWithRefCounting
(
    void
)
#endif
//--------------------------------------------------------------------------------------------------
{
#if LE_CONFIG_EVENT_NAMES_ENABLED
    return CreateEvent(name, sizeof(void*), true)->id;
#else
    return CreateEvent(sizeof(void*), true)->id;
#endif
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a handler function for a publish-subscribe event ID.
 *
 * Tells the calling thread's event loop to call a given handler function when a given event
 * reaches the front of the event queue.
 *
 * @return
 *      A handler reference, which is only needed for later removal of the handler (using
 *      le_event_RemoveHandler() ).  Can be ignored if the handler will never be removed.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_EVENT_NAMES_ENABLED
le_event_HandlerRef_t le_event_AddHandler
(
    const char*             name,           ///< [in] The name of the handler.
    le_event_Id_t           eventId,        ///< [in] The event ID.
    le_event_HandlerFunc_t  handlerFunc     ///< [in] Handler function.
)
#else
le_event_HandlerRef_t _le_event_AddHandler
(
    le_event_Id_t eventId,
    le_event_HandlerFunc_t handlerFunc
)
#endif
//--------------------------------------------------------------------------------------------------
{
#if LE_CONFIG_EVENT_NAMES_ENABLED
    return le_event_AddLayeredHandler(name, eventId, PubSubHandlerFunc, handlerFunc);
#else
    return _le_event_AddLayeredHandler(eventId, PubSubHandlerFunc, handlerFunc);
#endif
}


//--------------------------------------------------------------------------------------------------
/**
 * Adds a layered handler function for a publish-subscribe event ID.
 *
 * Tells the calling thread's event loop to call a given handler function when a given event
 * reaches the front of the event queue.  Passes another handler function to that handler
 * function when it is called.
 *
 * This is intended for use in implementing @ref c_event_LayeredHandlers.
 *
 * @return
 *      A handler reference, which is only needed for later removal of the handler (using
 *      le_event_RemoveHandler() ).  Can be ignored if the handler will never be removed.
 *
 * @note Doesn't return on failure, so there's no need to check the return value for errors.
 */
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_EVENT_NAMES_ENABLED
le_event_HandlerRef_t le_event_AddLayeredHandler
(
    const char*                     name,           ///< [in] The name of the handler.
    le_event_Id_t                   eventId,        ///< [in] The event ID.
    le_event_LayeredHandlerFunc_t   firstLayerFunc, ///< [in] Pointer to first-layer handler func.
    void*                           secondLayerFunc ///< [in] Pointer to second-layer handler func.
)
#else
le_event_HandlerRef_t _le_event_AddLayeredHandler
(
    le_event_Id_t eventId,
    le_event_LayeredHandlerFunc_t firstLayerFunc,
    void* secondLayerFunc
)
#endif
//--------------------------------------------------------------------------------------------------
{
    int oldState = event_Lock();

    Event_t* eventPtr = le_ref_Lookup(EventRefMap, eventId);

    event_Unlock(oldState);

    LE_ASSERT(eventPtr != NULL);

    // Get a pointer to the thread-specific event data record for the calling thread.
    event_PerThreadRec_t* threadRecPtr = thread_GetEventRecPtr();

    // Allocate the Handler object.
    Handler_t*  handlerPtr = le_mem_ForceAlloc(HandlerPool);

    // Initialize the Handler object's members.
    handlerPtr->eventLink = LE_DLS_LINK_INIT;
    handlerPtr->threadLink = LE_DLS_LINK_INIT;
    handlerPtr->threadRecPtr = threadRecPtr;
    handlerPtr->eventPtr = eventPtr;
    handlerPtr->contextPtr = NULL;
    handlerPtr->firstLayerFunc = firstLayerFunc;
    handlerPtr->secondLayerFunc = secondLayerFunc;
#if LE_CONFIG_EVENT_NAMES_ENABLED
    if (le_utf8_Copy(handlerPtr->name, name, sizeof(handlerPtr->name), NULL) == LE_OVERFLOW)
    {
        LE_WARN("Event handler name '%s' truncated to '%s'.", name, handlerPtr->name);
    }
#endif /* end LE_CONFIG_EVENT_NAMES_ENABLED */

    // Put it on the Thread's Handler List.
    le_dls_Queue(&threadRecPtr->handlerList, &handlerPtr->threadLink);

    // NOTE: We are about to access structures that are shared by multiple threads.
    // Protect this critical section using the mutex.

    oldState = event_Lock();

    // Put it on the Event's Handler List.
    le_dls_Queue(&eventPtr->handlerList, &handlerPtr->eventLink);

    // Create a Safe Reference for the Handler.
    le_event_HandlerRef_t handlerRef = le_ref_CreateRef(HandlerRefMap, handlerPtr);
    handlerPtr->safeRef = handlerRef;

    event_Unlock(oldState);

    return handlerRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * Remove Handler
 *
 * Removes a previously added event handler function.
 */
//--------------------------------------------------------------------------------------------------
void le_event_RemoveHandler
(
    le_event_HandlerRef_t   handlerRef  ///< [in] Handler reference.
)
//--------------------------------------------------------------------------------------------------
{
    int oldState = event_Lock();

    Handler_t* handlerPtr = le_ref_Lookup(HandlerRefMap, handlerRef);
    LE_FATAL_IF(handlerPtr == NULL, "Handler %p not found.", handlerPtr);

    // To prevent races, only the thread that registered the handler can deregister it.
    LE_FATAL_IF(handlerPtr->threadRecPtr != thread_GetEventRecPtr(),
                "Thread '%s' tried to remove a handler owned by another thread.",
                le_thread_GetMyName());

    DeleteHandler(handlerPtr);

    event_Unlock(oldState);
}


//--------------------------------------------------------------------------------------------------
/**
 * Report an Event
 *
 * Queues an Event Report to any and all event loops that have handlers for that event.
 *
 * @note This copies the event report payload, so it is safe to release or reuse the buffer that
 *       payloadPtr points to as soon as le_event_Report() returns.
 */
//--------------------------------------------------------------------------------------------------
void le_event_Report
(
    le_event_Id_t   eventId,    ///< [in] The event ID.
    void*           payloadPtr, ///< [in] Pointer to the payload bytes to be copied into the report.
    size_t          payloadSize ///< [in] The number of bytes of payload to copy into the report.
)
//--------------------------------------------------------------------------------------------------
{
    int oldState = event_Lock();

    Event_t* eventPtr = le_ref_Lookup(EventRefMap, eventId);

    LE_FATAL_IF(eventPtr == NULL, "No such event %p.", eventId);

    LE_FATAL_IF(eventPtr->isRefCounted,
                "Attempt to use Event ID (%s) created using le_event_CreateIdWithRefCounting().",
                EVENT_NAME(eventPtr->name));

    LE_FATAL_IF(eventPtr->payloadSize < payloadSize,
                "Payload size too big for event '%s' (%" PRIuS " > %" PRIuS ").",
                EVENT_NAME(eventPtr->name),
                payloadSize,
                eventPtr->payloadSize);

    TRACE("Reporting event '%s'...", EVENT_NAME(eventPtr->name));

    // For each Handler registered for this Event,
    le_dls_Link_t* linkPtr = le_dls_Peek(&eventPtr->handlerList);
    while (linkPtr != NULL)
    {
        Handler_t* handlerPtr = CONTAINER_OF(linkPtr, Handler_t, eventLink);

        event_PerThreadRec_t* perThreadRecPtr = handlerPtr->threadRecPtr;

        TRACE("  ...to handler '%s'.",
            EVENT_NAME(handlerPtr->name));

        // Queue a report to the handler's thread's Event Queue.
        PubSubEventReport_t* reportObjPtr = le_mem_ForceAlloc(eventPtr->reportPoolRef);
        reportObjPtr->baseClass.link = LE_SLS_LINK_INIT;
        reportObjPtr->baseClass.type = LE_EVENT_REPORT_PLAIN;
        reportObjPtr->handlerRef = handlerPtr->safeRef;
        memset(reportObjPtr->payload, 0, eventPtr->payloadSize);
        memcpy(reportObjPtr->payload, payloadPtr, payloadSize);
        le_sls_Queue(&perThreadRecPtr->eventQueue, &reportObjPtr->baseClass.link);

        // Increment the eventfd for the handler's thread's Event Queue.
        // This will wake up the thread and tell it that it has something on its Event Queue.
        fa_event_TriggerEvent_NoLock(perThreadRecPtr);

        linkPtr = le_dls_PeekNext(&eventPtr->handlerList, linkPtr);
    }

    event_Unlock(oldState);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sends an Event Report with a pointer to a reference-counted object as its payload.
 * The pointer must have been obtained from a memory pool using the @ref c_memory.
 *
 * The Event Loop API will ensure that the reference is properly counted while it passes
 * through the event report dispatching system.  Each handler will receive one counted reference
 * to the object, which the handler will then be responsible for releasing.  Do not release the
 * reference that you pass into le_event_ReportWithRefCounting().
 */
//--------------------------------------------------------------------------------------------------
void le_event_ReportWithRefCounting
(
    le_event_Id_t   eventId,    ///< [in] The event ID.
    void*           objectPtr   ///< [in] Pointer to an object allocated from a memory pool
                                ///       (using the @ref c_memory).
)
//--------------------------------------------------------------------------------------------------
{
    int oldState = event_Lock();

    Event_t* eventPtr = le_ref_Lookup(EventRefMap, eventId);

    LE_FATAL_IF(eventPtr == NULL, "No such event %p.", eventId);

    LE_FATAL_IF(eventPtr->isRefCounted == false,
                "Attempt to use Event ID (%s) created using le_event_CreateId().",
                EVENT_NAME(eventPtr->name));

    TRACE("Reporting event '%s'...", EVENT_NAME(eventPtr->name));

    // For each Handler registered for this Event,
    le_dls_Link_t* linkPtr = le_dls_Peek(&eventPtr->handlerList);
    while (linkPtr != NULL)
    {
        Handler_t* handlerPtr = CONTAINER_OF(linkPtr, Handler_t, eventLink);

        event_PerThreadRec_t* perThreadRecPtr = handlerPtr->threadRecPtr;

        TRACE("  ...to handler '%s'.", EVENT_NAME(handlerPtr->name));

        // Queue a report to the handler's thread's Event Queue.
        PubSubEventReport_t* reportObjPtr = le_mem_ForceAlloc(eventPtr->reportPoolRef);
        reportObjPtr->baseClass.link = LE_SLS_LINK_INIT;
        reportObjPtr->baseClass.type = LE_EVENT_REPORT_COUNTED_REF;
        reportObjPtr->handlerRef = handlerPtr->safeRef;
        reportObjPtr->payload[0] = objectPtr;
        le_mem_AddRef(objectPtr);
        le_sls_Queue(&perThreadRecPtr->eventQueue, &reportObjPtr->baseClass.link);

        // Increment the eventfd for the handler's thread's Event Queue.
        // This will wake up the thread and tell it that it has something on its Event Queue.
        fa_event_TriggerEvent_NoLock(perThreadRecPtr);

        linkPtr = le_dls_PeekNext(&eventPtr->handlerList, linkPtr);
    }

    event_Unlock(oldState);

    // Release our original reference that the caller passed us.
    // Note: It's best to do this outside the critical section so that we don't accidentally
    //       create a deadlock due to the object having a destructor that calls the Event Loop API
    //       or something.
    le_mem_Release(objectPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * Sets the context pointer for a given event handler.
 *
 * This can later be retrieved using le_event_GetContextPtr() from within the handler function
 * when it is called.
 */
//--------------------------------------------------------------------------------------------------
void le_event_SetContextPtr
(
    le_event_HandlerRef_t   handlerRef, ///< [in] Handler whose context pointer is to be set.
    void*                   contextPtr  ///< [in] Context pointer value.
)
//--------------------------------------------------------------------------------------------------
{
    int oldState = event_Lock();

    Handler_t* handlerPtr = le_ref_Lookup(HandlerRefMap, handlerRef);
    LE_FATAL_IF(handlerPtr == NULL, "Handler %p not found.", handlerPtr);

    handlerPtr->contextPtr = contextPtr;

    event_Unlock(oldState);
}


//--------------------------------------------------------------------------------------------------
/**
 * Fetches the context pointer for the currently running event handler.
 *
 * Can only be called from within an event handler function.
 *
 * @return
 *      The context pointer that was provided when the event handler was "added".
 */
//--------------------------------------------------------------------------------------------------
void* le_event_GetContextPtr
(
    void
)
//--------------------------------------------------------------------------------------------------
{
    event_PerThreadRec_t* recPtr = thread_GetEventRecPtr();

    // Don't have to lock before here because only the current running thread can delete
    // its own thread record.

    return recPtr->contextPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Queue a function onto the calling thread's Event Queue.  When it reaches the head of the
 * Event Queue, it will be called by the calling thread's Event Loop.
 */
//--------------------------------------------------------------------------------------------------
void le_event_QueueFunction
(
    le_event_DeferredFunc_t func,       ///< [in] The function to be called later.
    void*                   param1Ptr,  ///< [in] Value to be passed to the function when called.
    void*                   param2Ptr   ///< [in] Value to be passed to the function when called.
)
//--------------------------------------------------------------------------------------------------
{
    int oldState = event_Lock();

    QueueFunction_NoLock(thread_GetEventRecPtr(), func, param1Ptr, param2Ptr);

    event_Unlock(oldState);
}


//--------------------------------------------------------------------------------------------------
/**
 * Queue a function onto a specific thread's Event Queue.  When it reaches the head of that
 * Event Queue, it will be called by that thread's Event Loop.
 */
//--------------------------------------------------------------------------------------------------
void le_event_QueueFunctionToThread
(
    le_thread_Ref_t         thread,     ///< [in] The thread to queue the function to.
    le_event_DeferredFunc_t func,       ///< [in] The function.
    void*                   param1Ptr,  ///< [in] Value to be passed to the function when called.
    void*                   param2Ptr   ///< [in] Value to be passed to the function when called.
)
//--------------------------------------------------------------------------------------------------
{
    int oldState = event_Lock();

    QueueFunction_NoLock(thread_GetOtherEventRecPtr(thread), func, param1Ptr, param2Ptr);

    event_Unlock(oldState);
}

//--------------------------------------------------------------------------------------------------
/**
 * Runs the event loop for the calling thread.
 *
 * This starts the processing of events by the calling thread.
 *
 * This function can only be called at most once for each thread, and must never be called in
 * the process's main thread.
 *
 * @note This function never returns.
 */
//--------------------------------------------------------------------------------------------------
void le_event_RunLoop
(
    void
)
{
    fa_event_RunLoop();
}
