//--------------------------------------------------------------------------------------------------
/** @file eventLoop.h
 *
 * Linux-specific Legato Event Loop module inter-module include file.
 *
 * This file exposes interfaces that are for use by other modules inside the framework
 * implementation, but must not be used outside of the framework implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_LIBLEGATO_LINUX_EVENTLOOP_H_INCLUDE_GUARD
#define LEGATO_LIBLEGATO_LINUX_EVENTLOOP_H_INCLUDE_GUARD

#include "fa/eventLoop.h"

/**
 * Per-thread event queue data for Linux
 */
typedef struct
{
    event_PerThreadRec_t    portablePerThreadRec;   ///< Portable event queue structure
    int                     epollFd;                ///< epoll(7) file descriptor.
    int                     eventQueueFd;           ///< eventfd(2) file descriptor for the Event
                                                    ///< Queue.
}
event_LinuxPerThreadRec_t;


#endif /* end LEGATO_LIBLEGATO_LINUX_EVENTLOOP_H_INCLUDE_GUARD */
