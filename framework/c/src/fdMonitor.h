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

//--------------------------------------------------------------------------------------------------
/** Fallback definition of EPOLLWAKEUP
 *
 * Definition of EPOLLWAKEUP for kernel versions that do not support it.
 */
//--------------------------------------------------------------------------------------------------
#ifndef EPOLLWAKEUP
#pragma message "EPOLLWAKEUP unsupported. Power management features may fail."
#define EPOLLWAKEUP 0x0
#endif


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

#endif // LEGATO_FD_MONITOR_H_INCLUDE_GUARD
