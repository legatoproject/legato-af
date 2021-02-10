/**
 * @file atProxyPortService.h
 *
 * AT Proxy Port Service interface header.
 *
 * Copyright (C) Sierra Wireless Inc.
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
le_port_DeviceRef_t atProxyPortService_Request
(
    const char* deviceNamePtr  ///< [IN] Device name to be requested.
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
le_result_t atProxyPortService_Release
(
    le_port_DeviceRef_t devRef     ///< [IN] Device reference of port service.
);
