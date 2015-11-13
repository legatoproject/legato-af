
/**
 * @page c_le_build_cfg Legato Build Configuration
 *
 * In the file @c le_build_conifg.h are a number of preprocessor macros.  Uncommenting these macros
 * enables a non-standard feature of the framework.
 *
 * <HR>
 *
 * @section bld_cfg_mem_trace LE_MEM_TRACE
 *
 * When @c LE_MEM_TRACE is defined, the memory subsystem will create a trace point for every memory
 * pool created.  The name of the tracepoint will be the same of the pool, and is of the form
 * "component.poolName".
 *
 * @section bld_cfg_mem_valgrind LE_MEM_VALGRIND
 *
 * When @c LE_MEM_VALGRIND is enabled the memory system doesn't use pools anymore but in fact
 * switches to use malloc/free per-block.  This way, tools like valgrind can be used on a Legato
 * executable.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

//--------------------------------------------------------------------------------------------------
/**
 * @file le_build_config.h
 *
 * This file includes preprocessor macros that can be used to fine tune the Legato framework.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
//--------------------------------------------------------------------------------------------------
#ifndef LE_BUILD_CONFIG_H_INCLUDE_GUARD
#define LE_BUILD_CONFIG_H_INCLUDE_GUARD



// Uncomment this define to enable memory tracing.
//#define LE_MEM_TRACE



// Uncomment this define to enable valgrind style memory tracking.
//#define LE_MEM_VALGRIND



#endif
