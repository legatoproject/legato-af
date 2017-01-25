/**
 * @page c_pa_adc ADC Platform Adapter API
 *
 * @ref pa_adc.h "API Reference"
 *
 * <HR>
 *
 * These APIs are independent of platform. The implementation is in a platform specific adaptor
 * and applications that use these APIs can be trivially ported between platforms that implement
 * the APIs.
 *
 * These functions are blocking and return when the modem has replied to the request or a timeout
 * has occurred.
 *
 * The values returned are in units appropriate to the channel read.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2015.
 */


/** @file pa_adc.h
 *
 * Legato @ref c_pa_adc include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PAADC_INCLUDE_GUARD
#define LEGATO_PAADC_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Read the value of a given ADC channel in units appropriate to that channel.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_adc_ReadValue
(
    const char* adcNamePtr,
        ///< [IN]
        ///< Name of the ADC to read.

    int32_t* adcValuePtr
        ///< [OUT]
        ///< The adc value
);

#endif // LEGATO_PAADC_INCLUDE_GUARD
