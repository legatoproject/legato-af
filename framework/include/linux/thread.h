//--------------------------------------------------------------------------------------------------
/** @file thread.h
 *
 * Linux-specific Legato Thread include file.
 *
 * The Thread module is part of the @ref c_threading implementation.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_FA_THREAD_H_INCLUDE_GUARD
#define LEGATO_FA_THREAD_H_INCLUDE_GUARD

#include <linux/param.h>

//--------------------------------------------------------------------------------------------------
/**
 *  Extra space to allocate with the stack area.
 */
//--------------------------------------------------------------------------------------------------
#define FA_THREAD_STACK_EXTRA_SIZE  0

//--------------------------------------------------------------------------------------------------
/**
 *  Minimum static stack size.  Smaller stacks will be clamped to this value.
 */
//--------------------------------------------------------------------------------------------------
#define FA_THREAD_STACK_MIN_SIZE    PTHREAD_STACK_MIN

//--------------------------------------------------------------------------------------------------
/**
 *  Required alignment for the stack, in bytes.
 */
//--------------------------------------------------------------------------------------------------
#define FA_THREAD_STACK_ALIGNMENT   EXEC_PAGESIZE

#endif // LEGATO_FA_THREAD_H_INCLUDE_GUARD
