/**
 * @file atProxyPortService.c
 *
 * AT Proxy Port Service interface.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "pa_portService.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function requests to open a configured device. If the device was not opened, it opens
 * the device.
 *
 * @return
 *      - Reference to the device.
 *      - NULL if the device is not available.
 */
//--------------------------------------------------------------------------------------------------
le_port_DeviceRef_t le_port_Request
(
    const char* deviceNamePtr  ///< [IN] Device name to be requested.
)
{
    return pa_portService_Request(deviceNamePtr);
}


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
le_result_t le_port_SetDataMode
(
    le_port_DeviceRef_t devRef,   ///< [IN] Device reference.
    int* fdPtr                    ///< [OUT] File descriptor of the device.
)
{
    return pa_portService_SetDataMode(devRef, fdPtr);
}


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
le_result_t le_port_SetCommandMode
(
    le_port_DeviceRef_t devRef,           ///< [IN] Device reference.
    le_atServer_DeviceRef_t* deviceRefPtr ///< [OUT] AT server device reference.
)
{
    return pa_portService_SetCommandMode(devRef, deviceRefPtr);
}


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
le_result_t le_port_Release
(
    le_port_DeviceRef_t devRef     ///< [IN] Device reference of port service.
)
{
    return pa_portService_Release(devRef);
}

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
le_result_t le_port_GetPortReference
(
    le_atServer_DeviceRef_t atServerDevRef,   ///< [IN] Device reference from AT server.
    le_port_DeviceRef_t* devRefPtr            ///< [OUT] Device reference from port service.
)
{
    return pa_portService_GetPortReference(atServerDevRef, devRefPtr);
}
