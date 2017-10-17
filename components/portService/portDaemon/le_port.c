/**
 * This module implements the port service application.
 *
 * This application manages a list of serial links (physical or emulated).
 * It provides the APIs to open/close the devices.
 * Handles the devices which are opened by default.
 * Manages devices modes (AT command and data modes).
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include "interfaces.h"


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
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function switches the device into data mode.
 *
 * @return
 *      - LE_OK    Function succeeded.
 *      - LE_FAULT Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_port_SetDataMode
(
    le_port_DeviceRef_t devRef,   ///< [IN] Device reference.
    int8_t* fdPtr                 ///< [OUT] File descriptor of the device.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function switches the device into AT command mode.
 *
 * @return
 *      - LE_OK    Function succeeded.
 *      - LE_FAULT Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_port_SetCommandMode
(
    le_port_DeviceRef_t devRef    ///< [IN] Device reference.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function closes the device and releases the resources.
 *
 * @return
 *      - LE_OK    Function succeeded.
 *      - LE_FAULT Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_port_Release
(
    le_port_DeviceRef_t devRef    ///< [IN] Device reference.
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the device reference regarding to a given reference coming from the AT server.
 *
 * @return
 *      - LE_OK    Function succeeded.
 *      - LE_FAULT Function failed.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_port_GetPortReference
(
    le_atServer_DeviceRef_t atServerDevRef,   ///< [IN] Device reference from AT server.
    le_port_DeviceRef_t* devRefPtr            ///< [OUT] Device reference from port service.
)
{
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the port service.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{

}
