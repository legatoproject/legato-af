/**
 * @page c_pa_antenna Antenna Platform Adapter API
 *
 * @ref pa_antenna.h "API Reference"
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
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2015.
 */


/** @file pa_antenna.h
 *
 * Legato @ref c_pa_antenna include file.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PAANTENNA_INCLUDE_GUARD
#define LEGATO_PAANTENNA_INCLUDE_GUARD

#include "interfaces.h"

typedef struct
{
    le_antenna_Type_t    antennaType;
    le_antenna_Status_t  status;

} pa_antenna_StatusInd_t;

typedef void (*pa_antenna_StatusIndHandlerFunc_t)
(
    pa_antenna_StatusInd_t* msgRef
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to set the short limit
 *
 * @return
 * - LE_OK on success
 * - LE_FAULT on failure
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_antenna_SetShortLimit
(
    le_antenna_Type_t    antennaType,
    uint32_t                shortLimit
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to get the short limit
 *
 * @return
 * - LE_OK on success
 * - LE_FAULT on failure
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_antenna_GetShortLimit
(
    le_antenna_Type_t    antennaType,
    uint32_t*               shortLimitPtr
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to set the open limit
 *
 * @return
 * - LE_OK on success
 * - LE_FAULT on failure
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_antenna_SetOpenLimit
(
    le_antenna_Type_t    antennaType,
    uint32_t                openLimit
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to get the open limit
 *
 * @return
 * - LE_OK on success
 * - LE_FAULT on failure
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_antenna_GetOpenLimit
(
    le_antenna_Type_t    antennaType,
    uint32_t*               openLimitPtr
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to get the antenna status
 *
 * @return
 * - LE_OK on success
 * - LE_UNSUPPORTED if the antenna detection is not supported
 * - LE_FAULT on failure
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_antenna_GetStatus
(
    le_antenna_Type_t    antennaType,
    le_antenna_Status_t* statusPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the external ADC used to monitor the requested antenna.
 *
 * @return
 *      - LE_OK on success
 *      - LE_UNSUPPORTED request not supported
 *      - LE_FAULT on other failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_antenna_SetExternalAdc
(
    le_antenna_Type_t    antennaType,   ///< Antenna type
    int8_t               adcId          ///< The external ADC used to monitor the requested antenna
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the external ADC used to monitor the requested antenna.
 *
 * @return
 *      - LE_OK on success
 *      - LE_UNSUPPORTED request not supported
 *      - LE_FAULT on other failure
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_antenna_GetExternalAdc
(
    le_antenna_Type_t    antennaType,  ///< Antenna type
    int8_t*              adcIdPtr      ///< The external ADC used to monitor the requested antenna
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to set the status indication on a specific antenna
 *
 * * @return
 * - LE_OK on success
 * - LE_BUSY if the status indication is already set for the requested antenna
 * - LE_FAULT on other failure
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_antenna_SetStatusIndication
(
    le_antenna_Type_t antennaType
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to remove the status indication on a specific antenna
 *
 * @return
 * - LE_OK on success
 * - LE_FAULT on failure
 *
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_antenna_RemoveStatusIndication
(
    le_antenna_Type_t antennaType
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to add a status notification handler
 *
 * @return A handler reference, which is only needed for later removal of the handler.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_event_HandlerRef_t* pa_antenna_AddStatusHandler
(
    pa_antenna_StatusIndHandlerFunc_t   msgHandler
);

//--------------------------------------------------------------------------------------------------
/**
 *
 * This function is used to intialize the PA antenna
 * @return
 * - LE_OK on success
 * - LE_FAULT on failure
 *
 * @note This function should not be called from outside the platform adapter.
 *
 * @todo Move this prototype to another (internal) header.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_antenna_Init
(
    void
);


#endif // LEGATO_PAANTENNA_INCLUDE_GUARD
