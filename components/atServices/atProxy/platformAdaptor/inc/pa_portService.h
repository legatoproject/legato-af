/**
 * @file pa_portService.h
 *
 * AT Proxy platform adaptor interface for port services.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef _AT_PROXY_PA_PORTSERVICE_H_INCLUDE_GUARD
#define _AT_PROXY_PA_PORTSERVICE_H_INCLUDE_GUARD

#include "atProxyCmdHandler.h"


//--------------------------------------------------------------------------------------------------
/**
 * This function opens the specified device.
 *
 * @return
 *      - Reference to the device.
 *      - NULL if the device is not available.
 */
//--------------------------------------------------------------------------------------------------
le_port_DeviceRef_t pa_portService_Request
(
    const char* deviceNamePtr  ///< [IN] Device name to be requested.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function switches the device into data mode.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 *      - LE_UNAVAILABLE   JSON parsing is not completed.
 *      - LE_DUPLICATE     Device already opened in data mode
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_portService_SetDataMode
(
    le_port_DeviceRef_t devRef,   ///< [IN] Device reference.
    int* fdPtr                    ///< [OUT] File descriptor of the device.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function switches the device into AT command mode and returns At server device reference.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_portService_SetCommandMode
(
    le_port_DeviceRef_t devRef,           ///< [IN] Device reference.
    le_atServer_DeviceRef_t* deviceRefPtr ///< [OUT] AT server device reference.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function closes the device and releases the resources.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 *      - LE_UNAVAILABLE   JSON parsing is not completed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_portService_Release
(
    le_port_DeviceRef_t devRef     ///< [IN] Device reference of port service.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the device reference regarding to a given reference coming from the AT server.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_portService_GetPortReference
(
    le_atServer_DeviceRef_t atServerDevRef,   ///< [IN] Device reference from AT server.
    le_port_DeviceRef_t* devRefPtr            ///< [OUT] Device reference from port service.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function retrieves the AT Command Session for the specified device reference.
 *
 * @return
 *      - Pointer to AT Command Sesison    Function succeeded.
 *      - NULL                             Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_atProxy_AtCommandSession_t* pa_portService_GetAtCommandSession
(
    le_port_DeviceRef_t devRef     ///< [IN] Device reference of port service.
);

#endif /* _AT_PROXY_PA_PORTSERVICE_H_INCLUDE_GUARD */
