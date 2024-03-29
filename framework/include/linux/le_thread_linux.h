//--------------------------------------------------------------------------------------------------
/** @file thread_linux.h
 *
 * Linux-specific Legato Thread include file.
 *
 * The Thread module is part of the @ref c_threading implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_THREAD_LINUX_H_INCLUDE_GUARD
#define LEGATO_THREAD_LINUX_H_INCLUDE_GUARD

#include <linux/param.h>

//--------------------------------------------------------------------------------------------------
/**
 *  Extra space to allocate with the stack area.
 */
//--------------------------------------------------------------------------------------------------
#define LE_THREAD_STACK_EXTRA_SIZE  0

//--------------------------------------------------------------------------------------------------
/**
 *  Minimum static stack size.  Smaller stacks will be clamped to this value.
 */
//--------------------------------------------------------------------------------------------------
#define LE_THREAD_STACK_MIN_SIZE  16384  /* static value of PTHREAD_STACK_MIN */

//--------------------------------------------------------------------------------------------------
/**
 *  Required alignment for the stack, in bytes.
 */
//--------------------------------------------------------------------------------------------------
#define LE_THREAD_STACK_ALIGNMENT   EXEC_PAGESIZE

#endif /* end LEGATO_THREAD_LINUX_H_INCLUDE_GUARD */
