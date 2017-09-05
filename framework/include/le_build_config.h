
/**
 * @page c_le_build_cfg Build Configuration
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
 * @section bld_cfg_disable_SMACK LE_SMACK_DISABLE
 *
 * Legato provides the ability to disable the SMACK API. We don’t recommend disabling SMACK:
 * users do so at their own risk.
 *
 * By disabling SMACK, you essentially render the SMACK APIs to do nothing; SMACK labels aren’t set
 * during Legato runtime. On the Yocto side, disabling SMACK will not apply SMACK labels on certain
 * processes, files, and directories.
 *
 * If Legato's SMACK API is disabled, users must set SMACK labels for their own runtime environment
 * if they want to use SMACK security.
 *
 * To disable SMACK, follow these steps:
 * - Edit "framework/include/le_build_config.h" and uncomment "//#define LE_SMACK_DISABLE".
 * - Build Legato.
 * - Flash the resulting legato.cwe or legatoz.cwe with the fwupdate tool.  Do not install Legato with "instlegato".
 * - After the target reboots, it should have the file "/legato/SMACK_DISABLED".
 * - Reboot the target again.

 * To re-enable SMACK, follow these steps:
 * - Edit "framework/include/le_build_config.h" and comment "#define LE_SMACK_DISABLE".
 * - Build Legato.
 * - Flash the resulting legato.cwe or legatoz.cwe with the fwupdate tool.  Do not install Legato with "instlegato".
 * - After the target reboots, it should @b not have the file "/legato/SMACK_DISABLED".
 * - Reboot the target again.
 *
 * @section bld_cfg_disable_SEGV_HANDLER LE_SEGV_HANDLER_DISABLE
 *
 * When @c LE_SEGV_HANDLER_DISABLE is disabled, the ShowStackSignalHandler() will not use signal
 * derivation and sigsetjmp()/siglongjmp() to continue and try to survive to invalid memory access
 * while decoding the stack or the back-trace. This "2nd-level" handler is an ultimate protection
 * against SEGV. However this handler relies on undefined behaviour of sigsetjmp(), so is more
 * risky.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

//--------------------------------------------------------------------------------------------------
/**
 * @file le_build_config.h
 *
 * This file includes preprocessor macros that can be used to fine tune the Legato framework.
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------
#ifndef LE_BUILD_CONFIG_H_INCLUDE_GUARD
#define LE_BUILD_CONFIG_H_INCLUDE_GUARD



// Uncomment this define to enable memory tracing.
//#define LE_MEM_TRACE



// Uncomment this define to enable valgrind style memory tracking.
//#define LE_MEM_VALGRIND



// Uncomment this define to disable the "2nd SEGV handler" protection in ShowStackSignalHandler().
//#define LE_SEGV_HANDLER_DISABLE



#endif
