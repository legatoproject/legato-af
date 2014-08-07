/** @page c_APIs C APIs
 *
 * @section apiList Legato C APIs
 *
 * Legato has these C language APIs:
 *
 * @subpage c_basics  <br>
 * @subpage c_args  <br>
 * @subpage c_dir  <br>
 * @subpage c_doublyLinkedList  <br>
 * @subpage c_memory  <br>
 * @subpage c_eventLoop  <br>
 * @subpage c_flock  <br>
 * @subpage c_hashmap  <br>
 * @subpage c_hex  <br>
 * @subpage c_logging  <br>
 * @subpage c_messaging  <br>
 * @subpage c_mutex  <br>
 * @subpage c_path  <br>
 * @subpage c_pathIter  <br>
 * @subpage c_print  <br>
 * @subpage c_clock  <br>
 * @subpage c_safeRef  <br>
 * @subpage c_semaphore  <br>
 * @subpage c_signals  <br>
 * @subpage c_singlyLinkedList  <br>
 * @subpage c_threading  <br>
 * @subpage c_timer  <br>
 * @subpage c_test  <br>
 * @subpage c_utf8  <br>
 *
 * @section OOD Object-Oriented Design
 *
 * The Legato framework is constructed in an object-oriented manner.
 *
 * The C programming language was created before object-oriented programming became
 * popular so it has no native support for OOP features like inheritance, private object
 * members, member functions, and overloading.  Object-oriented designs can still be
 * implemented in C, though.
 *
 * In the Legato C APIs, classes are hidden behind opaque "reference" data types.
 * You can get references to objects created behind the scenes in Legato, but
 * you can never see the structure of those objects. The implementation is hidden from view.
 * Access to object properties is made available through accessor functions.
 *
 * @section opaqueTypes Opaque Types
 *
 * The basic "opaque data type" offered by the C programming language is the "void pointer" (void *).
 * The idea is that a pointer to an object of type @a T can be cast to point to a void type
 * before being passed outside of the module that implements @a T.
 *
 * This makes it impossible for anyone outside of the module that implements @a T to
 * dereference the pointer or access anything about the implementation of @a T.  This way,
 * the module that implements @a T is free to change the implementation of @a T in any
 * way needed without worrying about breaking code outside of that module.
 *
 * The problem with the void pointer type is that it throws away type information. At compile time, this makes it
 *  impossible to detect that a variable with opaque type @a T
 * has been passed into a function with some other pointer type @a P.
 *
 * To overcome this, Legato uses incomplete types to implement its opaque types.  For example, there are
 * declarations similar to the following in Legato C API header files:
 *
 * @code
 * // Declare a reference type for referring to Foo objects.
 * typedef struct le_foo* le_foo_Ref_t;
 * @endcode
 *
 * However, "struct le_foo" would @a not be defined in the API header or @a anywhere outside of the
 * hypothetical Foo API's implementation files. This makes "struct le_foo" an "incomplete type"
 * for all code outside of the Foo API implementation files. Incomplete types can't be used
 * because the compiler doesn't have enough information about them to generate any code
 * that uses them. But @a pointers to incomplete types @a can be passed around because the compiler
 * always knows the pointer size.  The compiler knows that one incomplete type
 * is @a not necessarily interchangeable with another.  It won't allow a pointer to
 * an incomplete type to be used where a pointer to another type is expected.
 *
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2014. All rights reserved. Use of this work is subject to license.
 */


/** @file legato.h
 *
 * This file includes all the commonly-used Legato API header files.
 * It's provided as a convenience to avoid including
 * dozens of header files in every source file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2012 - 2014. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_H_INCLUDE_GUARD
#define LEGATO_H_INCLUDE_GUARD

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
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

#ifdef __cplusplus
extern "C" {
#endif

#include "le_basics.h"
#include "le_doublyLinkedList.h"
#include "le_singlyLinkedList.h"
#include "le_utf8.h"
#include "le_mem.h"
#include "le_log.h"
#include "le_mutex.h"
#include "le_clock.h"
#include "le_semaphore.h"
#include "le_safeRef.h"
#include "le_thread.h"
#include "le_eventLoop.h"
#include "le_hashmap.h"
#include "le_signals.h"
#include "le_args.h"
#include "le_timer.h"
#include "le_messaging.h"
#include "le_test.h"
#include "le_path.h"
#include "le_pathIter.h"
#include "le_hex.h"
#include "le_dir.h"
#include "le_fileLock.h"

#ifdef __cplusplus
}
#endif

#endif // LEGATO_H_INCLUDE_GUARD
