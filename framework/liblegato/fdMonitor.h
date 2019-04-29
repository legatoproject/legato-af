//--------------------------------------------------------------------------------------------------
/** @file fdMonitor.h
 *
 * Inter-module interface definitions exported by the File Descriptor Monitor module to other
 * modules within the framework.
 *
 * The File Descriptor Monitor module is part of the @ref c_eventLoop implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_FD_MONITOR_H_INCLUDE_GUARD
#define LEGATO_FD_MONITOR_H_INCLUDE_GUARD

#include "eventLoop.h"
#include "fa/fdMonitor.h"

//--------------------------------------------------------------------------------------------------
/**
 * Insert a string name variable if configured or a placeholder string if not.
 *
 *  @param  nameVar Name variable to insert.
 *
 *  @return Name variable or a placeholder string depending on configuration.
 **/
//--------------------------------------------------------------------------------------------------
#if LE_CONFIG_FD_MONITOR_NAMES_ENABLED
#   define  FDMON_NAME(var) (var)
#else
#   define  FDMON_NAME(var) "<omitted>"
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Get a human readable string describing the fd events in a given bit map.
 *
 * @return buffPtr.
 */
//--------------------------------------------------------------------------------------------------
const char *fdMon_GetEventsText
(
    char    *buffPtr,   ///< [out] Pointer to buffer to put the string in.
    size_t   buffSize,  ///< [in]  Size of the buffer, in bytes.
    short    events     ///< [in]  Bit map of events (see 'man 2 poll').
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the FD Monitor module.
 *
 * This function must be called exactly once at process start-up, before any other FD Monitor
 * functions are called.
 */
//--------------------------------------------------------------------------------------------------
void fdMon_Init
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize the FD Monitor part of the Event Loop API's per-thread record.
 *
 * This function must be called exactly once at thread start-up, before any other FD Monitor
 * functions are called by that thread.
 */
//--------------------------------------------------------------------------------------------------
void fdMon_InitThread
(
    event_PerThreadRec_t* perThreadRecPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Report FD Events.
 *
 * This is called by the Event Loop when it detects events on a file descriptor that is being
 * monitored.
 */
//--------------------------------------------------------------------------------------------------
void fdMon_Report
(
    void*       safeRef,        ///< [in] Safe Reference for the FD Monitor object for the fd.
    uint32_t    eventFlags      ///< [in] OR'd together event flags from epoll_wait().
);


//--------------------------------------------------------------------------------------------------
/**
 * Delete all FD Monitor objects for the calling thread.
 */
//--------------------------------------------------------------------------------------------------
void fdMon_DestructThread
(
    event_PerThreadRec_t* perThreadRecPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Signal a file descriptor has pending events.
 *
 * @note: This should only be used on customized file descriptors.  Other file descriptors are
 *        checked by the event loop automatically.
 **/
//--------------------------------------------------------------------------------------------------
void fdMon_SignalFd
(
    int         fd,         ///< [in] Reference to the File Descriptor Monitor object
    uint32_t    eventFlags  ///< [in] Event flags to signal
);

#endif /* end LEGATO_FD_MONITOR_H_INCLUDE_GUARD */
