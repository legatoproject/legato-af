/**
 * @file pa_portService.c
 *
 * AT Proxy default platform adaptor for port services.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "pa_portService.h"

//--------------------------------------------------------------------------------------------------
/**
 * This function opens the specified device.
 *
 * @return
 *      - Reference to the device.
 *      - NULL if the device is not available.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_port_DeviceRef_t pa_portService_Request
(
    const char* deviceNamePtr  ///< [IN] Device name to be requested.
)
{
    LE_UNUSED(deviceNamePtr);

    return NULL;
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
LE_SHARED le_result_t pa_portService_SetDataMode
(
    le_port_DeviceRef_t devRef,   ///< [IN] Device reference.
    int* fdPtr                    ///< [OUT] File descriptor of the device.
)
{
    LE_UNUSED(devRef);
    LE_UNUSED(fdPtr);

    return LE_NOT_IMPLEMENTED;
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
LE_SHARED le_result_t pa_portService_SetCommandMode
(
    le_port_DeviceRef_t devRef,           ///< [IN] Device reference.
    le_atServer_DeviceRef_t* deviceRefPtr ///< [OUT] AT server device reference.
)
{
    LE_UNUSED(devRef);
    LE_UNUSED(deviceRefPtr);

    return LE_NOT_IMPLEMENTED;
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
LE_SHARED le_result_t pa_portService_Release
(
    le_port_DeviceRef_t devRef     ///< [IN] Device reference of port service.
)
{
    LE_UNUSED(devRef);

    return LE_NOT_IMPLEMENTED;
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
LE_SHARED le_result_t pa_portService_GetPortReference
(
    le_atServer_DeviceRef_t atServerDevRef,   ///< [IN] Device reference from AT server.
    le_port_DeviceRef_t* devRefPtr            ///< [OUT] Device reference from port service.
)
{
    LE_UNUSED(atServerDevRef);
    LE_UNUSED(devRefPtr);

    return LE_NOT_IMPLEMENTED;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function retrieves the AT Command Session for the specified device reference.
 *
 * @return
 *      - Pointer to AT Command Sesison    Function succeeded.
 *      - NULL                             Function failed.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_atProxy_AtCommandSession_t* pa_portService_GetAtCommandSession
(
    le_port_DeviceRef_t devRef     ///< [IN] Device reference of port service.
)
{
    LE_UNUSED(devRef);

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function output the status string to indicate successfully entered data mode
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_portService_SendDataModeStatus
(
    int fd
)
{
    LE_UNUSED(fd);

    return;
}
