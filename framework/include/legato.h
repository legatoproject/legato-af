/**
 * @page c_APIs lib Legato
 *
 * The Legato Application Framework provide a set of APIs to simplify common C programming tasks.
 * @c lib @c Legato adds object oriented functions to simplify and enhance the safety and
 * reliability of your systems.
 *
 * These APIs are available automatically to your applications by including @c legato.h into your
 * code.
 *
 * It is also good practice to include @c interfaces.h. @c interfaces.h is a C header file that
 * automatically includes all client and server interface definition that your client imports and
 * exports. If you need to import or export any other API you will need to explicitly include
 * @c interfaces.h into your C or C++ code.
 *
 @verbatim
  #include "legato.h"
  #include "interfaces.h"
 @endverbatim
 *
 * | API Guide                    | API Reference               | File Name                | Description                                                                                                               |
 * | -----------------------------|-----------------------------| -------------------------| --------------------------------------------------------------------------------------------------------------------------|
 * | @subpage c_args              | @ref le_args.h              | @c le_args.h             | Provides the ability to add arguments from the command line                                                               |
 * | @subpage c_atomFile          | @ref le_atomFile.h          | @c le_atomFile.h         | Provides atomic file access mechanism that can be used to perform file operation (specially file write) in atomic fashion |
 * | @subpage c_basics            | @ref le_basics.h            | @c le_basics.h           | Provides error codes, portable integer types, and helpful macros that make things easier to use                           |
 * | @subpage c_clock             | @ref le_clock.h             | @c le_clock.h            | Gets/sets date and/or time values, and performs conversions between these values.                                         |
 * | @subpage c_crc               | @ref le_crc.h               | @c le_crc.h              | Provides the ability to compute the CRC of a binary buffer                                                                |
 * | @subpage c_dir               | @ref le_dir.h               | @c le_dir.h              | Provides functions to control directories                                                                                 |
 * | @subpage c_doublyLinkedList  | @ref le_doublyLinkedList.h  | @c le_doublyLinkedList.h | Provides a data structure that consists of data elements with links to the next node and previous nodes                   |
 * | @subpage c_eventLoop         | @ref le_eventLoop.h         | @c le_eventLoop.h        | Provides event loop functions to support the event-driven programming model                                               |
 * | @subpage c_fdMonitor         | @ref le_fdMonitor.h         | @c le_fdMonitor.h        | Provides monitoring of file descriptors, reporting, and related events                                                    |
 * | @subpage c_flock             | @ref le_fileLock.h          | @c le_fileLock.h         | Provides file locking, a form of IPC used to synchronize multiple processes' access to common files                       |
 * | @subpage c_fs                | @ref le_fs.h                | @c le_fs.h               | Provides a way to access the file system across different platforms                                                       |
 * | @subpage c_hashmap           | @ref le_hashmap.h           | @c le_hashmap.h          | Provides creating, iterating and tracing functions for a hashmap                                                          |
 * | @subpage c_hex               | @ref le_hex.h               | @c le_hex.h              | Provides conversion between Hex and Binary strings                                                                        |
 * | @subpage c_json              | @ref le_json.h              | @c le_json.h             | Provides fast parsing of a JSON data stream with very little memory required                                              |
 * | @subpage c_logging           | @ref le_log.h               | @c le_log.h              | Provides a toolkit allowing code to be instrumented with error, warning, informational, and debugging messages            |
 * | @subpage c_memory            | @ref le_mem.h               | @c le_mem.h              | Provides functions to create, allocate and release data from a memory pool                                                |
 * | @subpage c_messaging         | @ref le_messaging.h         | @c le_messaging.h        | Provides support to low level messaging within Legato                                                                     |
 * | @subpage c_mutex             | @ref le_mutex.h             | @c le_mutex.h            | Provides standard mutex functionality with added diagnostics capabilities                                                 |
 * | @subpage c_pack              | @ref le_pack.h              | @c le_pack.h             | Provides low-level pack/unpack functions to support the higher level IPC messaging system                                 |
 * | @subpage c_path              | @ref le_path.h              | @c le_path.h             | Provides support for UTF-8 null-terminated strings and multi-character separators                                         |
 * | @subpage c_pathIter          | @ref le_pathIter.h          | @c le_pathIter.h         | Iterate over paths, traverse the path node-by-node, or create and combine paths together                                  |
 * | @subpage c_rand              | @ref le_rand.h              | @c le_rand.h             | Used for cryptographic purposes such as encryption keys, initialization vectors, etc.                                     |
 * | @subpage c_safeRef           | @ref le_safeRef.h           | @c le_safeRef.h          | Protect from damaged or stale references being used by clients                                                            |
 * | @subpage c_semaphore         | @ref le_semaphore.h         | @c le_semaphore.h        | Provides standard semaphore functionality, but with added diagnostic capabilities                                         |
 * | @subpage c_signals           | @ref le_signals.h           | @c le_signals.h          | Provides software interrupts for running processes or threads                                                             |
 * | @subpage c_singlyLinkedList  | @ref le_singlyLinkedList.h  | @c le_singlyLinkedList.h | Provides a data structure consisting of a group of nodes linked together linearly                                         |
 * | @subpage c_test              | @ref le_test.h              | @c le_test.h             | Provides macros that are used to simplify unit testing                                                                    |
 * | @subpage c_threading         | @ref le_thread.h            | @c le_thread.h           | Provides controls for creating, ending and joining threads                                                                |
 * | @subpage c_timer             | @ref le_timer.h             | @c le_timer.h            | Provides functions for managing and using timers                                                                          |
 * | @subpage c_tty               | @ref le_tty.h               | @c le_tty.h              | Provides routines to configure serial ports                                                                               |
 * | @subpage c_utf8              | @ref le_utf8.h              | @c le_utf8.h             | Provides safe and easy to use string handling functions for null-terminated strings with UTF-8 encoding                   |
 *
 * @section cApiOverview Overview
 * Here is some background info on Legato's C Language APIs.
 *
 * @subsection cApiOverview_ood Object-Oriented Design
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
 *  @subsection cApiOverview_ot Opaque Types
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
 *  @subsection cApiOverview_handlersEvents Handlers and Event-Driven Programs
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

#if defined(__cplusplus) && !defined(__STDC_LIMIT_MACROS)
// stdint.h from before C++11 only defines MAX and MIN macros if __STDC_LIMIT_MACROS is defined
#define __STDC_LIMIT_MACROS
#endif

#include "le_config.h"

#if LE_CONFIG_LINUX
#   include "linux/legato.h"
#elif LE_CONFIG_CUSTOM_OS
#   include "custom_os/legato.h"
#else
#   error "Unsupported OS type"
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "le_basics.h"
#include "le_apiFeatures.h"

#include "le_doublyLinkedList.h"
#include "le_singlyLinkedList.h"
#include "le_utf8.h"
#include "le_backtrace.h"
#include "le_log.h"
#include "le_mem.h"
#include "le_mutex.h"
#include "le_clock.h"
#include "le_cdata.h"
#include "le_semaphore.h"
#include "le_hashmap.h"
#include "le_safeRef.h"
#include "le_thread.h"
#include "le_eventLoop.h"
#include "le_fdMonitor.h"
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
#include "le_fd.h"
#include "le_base64.h"
#include "le_process.h"

#ifdef __cplusplus
}
#endif

#endif // LEGATO_H_INCLUDE_GUARD
