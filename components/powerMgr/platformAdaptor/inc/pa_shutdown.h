/**
 * @page c_pa_shutdown Power shutdown Platform Adaptor API
 *
 * @ref pa_shutdown.h "API Reference"
 *
 * <HR>
 *
 * These APIs are independent of platform. The implementation is in a platform specific adaptor
 * and applications that use these APIs can be trivially ported between platforms that implement
 * the APIs.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc..
 */


/** @file pa_shutdown.h
 *
 * Legato @ref c_pa_shutdown include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PWR_SHUTDOWN_INCLUDE_GUARD
#define LEGATO_PWR_SHUTDOWN_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Issue a fast shutdown.
 *
 * @return
 *  - LE_OK            Function success
 *  - LE_FAULT         Internal error
 *  - LE_UNSUPPORTED   Feature not supported
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_shutdown_IssueFast
(
    void
);

#endif // LEGATO_PWR_SHUTDOWN_INCLUDE_GUARD