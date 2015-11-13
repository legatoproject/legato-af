/**
 * @page c_pa_adc Temperature Monitoring Platform Adapter API
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
 * Copyright (C) Sierra Wireless, Inc. 2015. Use of this work is subject to license.
 */


/** @file pa_adc.h
 *
 * Legato @ref c_pa_adc include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
    le_adc_AdcChannelInput_t  adcChannel,  ///< [IN] Channel to read
    int32_t*                  adcValuePtr  ///< [OUT] value returned by adc read
);

#endif // LEGATO_PAADC_INCLUDE_GUARD
