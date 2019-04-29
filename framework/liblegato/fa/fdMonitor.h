/**
 * @file fdMonitor.h
 *
 * File descriptor monitor interface that must be implemented by a Legato framework adaptor.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef FA_FDMONITOR_H_INCLUDE_GUARD
#define FA_FDMONITOR_H_INCLUDE_GUARD

#include "legato.h"
#include "limit.h"

/// Maximum number of bytes in a File Descriptor Monitor's name, including the null terminator.
#define MAX_FD_MONITOR_NAME_BYTES  LIMIT_MAX_MEM_POOL_NAME_BYTES

//--------------------------------------------------------------------------------------------------
/**
 * File Descriptor Monitor
 *
 * These keep track of file descriptors that are being monitored by a particular thread.
 * They are allocated from a per-thread FD Monitor Sub-Pool and are kept on the thread's
 * FD Monitor List.  In addition, each has a Safe Reference created from the
 * FD Monitor Reference Map.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dls_Link_t            link;              ///< Used to link onto a thread's FD Monitor List.
    int                      fd;                ///< File descriptor being monitored.
    le_fdMonitor_Ref_t       safeRef;           ///< Safe Reference for this object.
    event_PerThreadRec_t    *threadRecPtr;      ///< Ptr to per-thread data for monitoring thread.
    uint32_t                 eventFlags;        ///< Event flags in the style of poll().

    le_fdMonitor_HandlerFunc_t   handlerFunc;   ///< Handler function.
    void                        *contextPtr;    ///< The context pointer for this handler.

#if LE_CONFIG_FD_MONITOR_NAMES_ENABLED
    char name[MAX_FD_MONITOR_NAME_BYTES];       ///< UTF-8 name of this object.
#endif
}
fdMon_t;

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the FD Monitor module.
 *
 * This function must be called exactly once at process start-up, before any other FD Monitor
 * functions are called.
 *
 * @return The memory pool from which to allocate FD monitor instances.
 */
//--------------------------------------------------------------------------------------------------
le_mem_PoolRef_t fa_fdMon_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the platform-specific part of an FD monitor instance.
 *
 * @note The global monitor mutex will be locked for the duration of this function.
 */
//--------------------------------------------------------------------------------------------------
void fa_fdMon_Create
(
    fdMon_t *fdMonitorPtr ///< Monitor instance to initialize.
);

//--------------------------------------------------------------------------------------------------
/**
 * Delete an FD Monitor object for a given thread.  This frees resources associated with the
 * monitor, but not the monitor instance itself.
 */
//--------------------------------------------------------------------------------------------------
void fa_fdMon_Delete
(
    fdMon_t *fdMonitorPtr ///< [in] Pointer to the FD Monitor.
);

//--------------------------------------------------------------------------------------------------
/**
 * Enable monitoring for events on a file descriptor.
 *
 * @return Events filtered for those that can be enabled.
 */
//--------------------------------------------------------------------------------------------------
short fa_fdMon_Enable
(
    fdMon_t *monitorPtr,        ///< FD monitor for which events are being enabled.
    fdMon_t *handlerMonitorPtr, ///< FD monitor for the handler which is calling the enable function.
                                ///< May be NULL if not enabling from a handler.
    short    events             ///< Bitmap of events.
);

//--------------------------------------------------------------------------------------------------
/**
 * Disable monitoring for events on a file descriptor.
 *
 * @return Events filtered for those that can be disabled.
 */
//--------------------------------------------------------------------------------------------------
short fa_fdMon_Disable
(
    fdMon_t *monitorPtr,        ///< FD monitor for which events are being disabled.
    fdMon_t *handlerMonitorPtr, ///< FD monitor for the handler which is calling the disable
                                ///< function.  May be NULL if not disabling from a handler.
    short    events             ///< Bitmap of events.
);

//--------------------------------------------------------------------------------------------------
/**
 * Set if processing of events on a given fd is deferrable (the system is allowed to go to
 * sleep while there are monitored events pending for this fd) or urgent (the system will be kept
 * awake until there are no monitored events waiting to be handled for this fd).
 */
//--------------------------------------------------------------------------------------------------
void fa_fdMon_SetDeferrable
(
    fdMon_t *monitorPtr,  ///< FD monitor instance.
    bool     isDeferrable ///< Deferrable (true) or urgent (false).
);

//--------------------------------------------------------------------------------------------------
/**
 * Dispatch an FD Event to the appropriate registered handler function.
 */
//--------------------------------------------------------------------------------------------------
void fa_fdMon_DispatchToHandler
(
    fdMon_t     *fdMonitorPtr,  ///< FD Monitor instance.
    uint32_t     flags          ///< Event flags.
);

#endif /* end FA_FDMONITOR_H_INCLUDE_GUARD */
