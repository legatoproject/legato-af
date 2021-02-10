/**
 * @file atProxyPortService.c
 *
 * AT Proxy Port Service interface.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "atProxyPortService.h"
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
    return atProxyPortService_Request(deviceNamePtr);
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
    if (fdPtr == NULL)
    {
        LE_ERROR("fdPtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    if (devRef == NULL)
    {
        LE_ERROR("devRef is NULL!");
        return LE_BAD_PARAMETER;
    }

    // Enable Data Mode
    le_result_t result = pa_portService_SetDataMode(devRef, fdPtr);
    if (result != LE_OK)
    {
        LE_ERROR("[%s] Error setting AT Data Mode, result [%d]", __FUNCTION__, result);
        return result;
    }

    // Retrieve the AT Command Session for the device reference
    le_atProxy_AtCommandSession_t* atCmdSessionPtr = pa_portService_GetAtCommandSession(devRef);
    if (atCmdSessionPtr == NULL)
    {
        LE_ERROR("[%s] AT Command Session is NULL", __FUNCTION__);
        return LE_FAULT;
    }

    // Suspend AT Command mode session
    atProxyCmdHandler_StartDataMode(atCmdSessionPtr);

    return LE_OK;
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
    if (devRef == NULL)
    {
        LE_ERROR("devRef is NULL!");
        return LE_BAD_PARAMETER;
    }

    if (deviceRefPtr == NULL)
    {
        LE_ERROR("deviceRefPtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    // Enable Command Mode
    le_result_t result = pa_portService_SetCommandMode(devRef, deviceRefPtr);
    if (result != LE_OK)
    {
        LE_ERROR("[%s] Error setting AT Command Mode, result [%d]", __FUNCTION__, result);
        return result;
    }

    // Retrieve the AT Command Session for the device reference
    le_atProxy_AtCommandSession_t* atCmdSessionPtr = pa_portService_GetAtCommandSession(devRef);
    if (atCmdSessionPtr == NULL)
    {
        LE_ERROR("[%s] AT Command Session is NULL", __FUNCTION__);
        return LE_FAULT;
    }

    // Resume AT Command mode session
    atProxyCmdHandler_StopDataMode(atCmdSessionPtr);

    return LE_OK;
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
    return atProxyPortService_Release(devRef);
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
    if (atServerDevRef == NULL)
    {
        LE_ERROR("atServerDevRef is NULL!");
        return LE_BAD_PARAMETER;
    }

    if (devRefPtr == NULL)
    {
        LE_ERROR("devRefPtr is NULL!");
        return LE_BAD_PARAMETER;
    }

    return pa_portService_GetPortReference(atServerDevRef, devRefPtr);
}


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
LE_SHARED le_port_DeviceRef_t atProxyPortService_Request
(
    const char* deviceNamePtr  ///< [IN] Device name to be requested.
)
{
    if ((NULL == deviceNamePtr) || !(strcmp(deviceNamePtr, "")))
    {
        LE_ERROR("deviceNamePtr is not valid!");
        return NULL;
    }

    return pa_portService_Request(deviceNamePtr);
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
LE_SHARED le_result_t atProxyPortService_Release
(
    le_port_DeviceRef_t devRef     ///< [IN] Device reference of port service.
)
{
    if (devRef == NULL)
    {
        LE_ERROR("devRef is NULL!");
        return LE_BAD_PARAMETER;
    }

    // Retrieve the AT Command Session for the device reference
    le_atProxy_AtCommandSession_t* atCmdSessionPtr = pa_portService_GetAtCommandSession(devRef);
    if (atCmdSessionPtr == NULL)
    {
        LE_ERROR("[%s] AT Command Session is NULL", __FUNCTION__);
        return LE_FAULT;
    }

    // Close the AT Command Session
    le_result_t result =
        atProxyCmdHandler_CloseSession(atCmdSessionPtr);

    if (result != LE_OK)
    {
        LE_ERROR("[%s] Error closing AT Command Session, result [%d]", __FUNCTION__, result);
        return result;
    }

    return pa_portService_Release(devRef);
}
