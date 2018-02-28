/**
 * @page c_APIs C Runtime Library
 *
 * This section contains detailed info about Legato's C Language library used for low-level routines like
 * commonly used data structures and OS services APIs.
 *
 * <HR>
 *
 * The C APIs' @ref cApiOverview has high-level info.
 *
 * <HR>
 *
 * @subpage c_le_build_cfg <br>
 * @subpage c_basics <br>
 * @subpage c_args <br>
 * @subpage c_atomFile <br>
 * @subpage c_crc <br>
 * @subpage c_dir <br>
 * @subpage c_doublyLinkedList <br>
 * @subpage c_memory <br>
 * @subpage c_eventLoop <br>
 * @subpage c_fdMonitor <br>
 * @subpage c_flock <br>
 * @subpage c_fs <br>
 * @subpage c_hashmap <br>
 * @subpage c_hex <br>
 * @subpage c_json <br>
 * @subpage c_logging <br>
 * @subpage c_messaging <br>
 * @subpage c_mutex <br>
 * @subpage c_pack <br>
 * @subpage c_path <br>
 * @subpage c_pathIter <br>
 * @subpage c_print <br>
 * @subpage c_rand <br>
 * @subpage c_safeRef <br>
 * @subpage c_semaphore <br>
 * @subpage c_signals <br>
 * @subpage c_singlyLinkedList <br>
 * @subpage c_clock <br>
 * @subpage c_threading <br>
 * @subpage c_timer <br>
 * @subpage c_test <br>
 * @subpage c_utf8 <br>
 * @subpage c_tty
 *
 * @section cApiOverview Overview
 * Here is some background info on Legato's C Language APIs.
 *
 * @subsection Object-Oriented Design
 *
 * The Legato framework is constructed in an object-oriented manner.
 *
 * The C programming language was created before object-oriented programming was
 * popular so it doesn't have native support for OOP features like inheritance, private object
 * members, member functions, and overloading. But object-oriented designs can still be
 * implemented in C.
 *
 * In the Legato C APIs, classes are hidden behind opaque "reference" data types.
 * You can get references to objects created behind the scenes in Legato, but
 * you can never see the structure of those objects. The implementation is hidden from view.
 * Access to object properties is made available through accessor functions.
 *
 *  @subsection Opaque Types
 *
 * The basic "opaque data type" offered by the C programming language is the "void pointer"
 * <c>(void *)</c>.
 * The idea is that a pointer to an object of type @a T can be cast to point to a <c>void</c> type
 * before being passed outside of the module that implements @a T.
 *
 * This makes it impossible for anyone outside of the module that implements @a T to
 * dereference the pointer or access anything about the implementation of @a T.  This way,
 * the module that implements @a T is free to change the implementation of @a T in any
 * way needed without worrying about breaking code outside of that module.
 *
 * The problem with the void pointer type is that it throws away type information. At compile time,
 * this makes it impossible to detect that a variable with opaque type @a T
 * has been passed into a function with some other pointer type @a P.
 *
 * To overcome this, Legato uses "incomplete types" to implement its opaque types.  For example,
 * there are declarations similar to the following in Legato C API header files:
 *
 * @code
 * // Declare a reference type for referring to Foo objects.
 * typedef struct le_foo* le_foo_Ref_t;
 * @endcode
 *
 * But "struct le_foo" would @a not be defined in the API header or @a anywhere outside of the
 * hypothetical "Foo" API's implementation files. This makes "struct le_foo" an "incomplete type"
 * for all code outside of the Foo API implementation files. Incomplete types can't be used
 * because the compiler doesn't have enough information about them to generate any code
 * that uses them. But @a pointers to incomplete types @a can be passed around because the compiler
 * always knows the pointer size.  The compiler knows that one incomplete type
 * is @a not necessarily interchangeable with another, and it won't allow a pointer to
 * an incomplete type to be used where a pointer to another non-void type is expected.
 *
 *  @subsection Handlers and Event-Driven Programs
 *
 * "Handler" is an alias for "callback function".  Many APIs in the Legato world
 * use callback functions to notify their clients of asynchronous events.  For example, to
 * register a function called "SigTermHandler" for notification of receipt of a TERM signal,
 *
 * @code
 *
 * le_sig_SetEventHandler(SIGTERM, SigTermHandler);
 *
 * @endcode
 *
 * Instead of using "CallbackFunction", "CallbackFunc", or some abbreviation like "CbFn"
 * (Cub fun? Cubs fan? What?) that is difficult to read and/or remember, "Handler" is used.
 *
 * The term "handler" also fits with the event-driven programming style that is favoured in
 * the Legato world.  These callback functions are essentially event handler functions.
 * That is, the function is called when an event occurs to "handle" that event.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/**
 * @file legato.h
 *
 * This file includes all the commonly-used Legato API header files.
 * It's provided as a convenience to avoid including
 * dozens of header files in every source file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_H_INCLUDE_GUARD
#define LEGATO_H_INCLUDE_GUARD

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#if defined(__cplusplus) && !defined(__STDC_LIMIT_MACROS)
// stdint.h from before C++11 only defines MAX and MIN macros if __STDC_LIMIT_MACROS is defined
#define __STDC_LIMIT_MACROS
#endif

#include <stddef.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <syslog.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>
#include <dirent.h>
#include <sys/reboot.h>
#include <sys/timeb.h>
#include <sys/mount.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <mntent.h>
#include <grp.h>
#include <sys/xattr.h>
#include <fts.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/prctl.h>
#include <sched.h>
#include <semaphore.h>
#include <math.h>
#include <libgen.h>


#ifdef __cplusplus
extern "C" {
#endif

#include "le_build_config.h"
#include "le_basics.h"
#include "le_doublyLinkedList.h"
#include "le_singlyLinkedList.h"
#include "le_utf8.h"
#include "le_log.h"
#include "le_mem.h"
#include "le_mutex.h"
#include "le_clock.h"
#include "le_semaphore.h"
#include "le_safeRef.h"
#include "le_thread.h"
#include "le_eventLoop.h"
#include "le_fdMonitor.h"
#include "le_hashmap.h"
#include "le_signals.h"
#include "le_args.h"
#include "le_timer.h"
#include "le_messaging.h"
#include "le_test.h"
#include "le_pack.h"
#include "le_path.h"
#include "le_pathIter.h"
#include "le_hex.h"
#include "le_dir.h"
#include "le_fileLock.h"
#include "le_json.h"
#include "le_tty.h"
#include "le_atomFile.h"
#include "le_crc.h"
#include "le_fs.h"
#include "le_rand.h"

#ifdef __cplusplus
}
#endif

#endif // LEGATO_H_INCLUDE_GUARD
