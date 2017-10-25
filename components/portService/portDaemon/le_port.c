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
 * Maximum number of client applications.
 */
//--------------------------------------------------------------------------------------------------
#define MAX_APPS 2

//--------------------------------------------------------------------------------------------------
/**
 * Max length for error string
 */
//--------------------------------------------------------------------------------------------------
#define ERR_MSG_MAX 256

//--------------------------------------------------------------------------------------------------
/**
 * Device mode flag for blocking / non-blocking.
 */
//--------------------------------------------------------------------------------------------------
#define BLOCKING_MODE     true
#define NON_BLOCKING_MODE false

//--------------------------------------------------------------------------------------------------
/**
 * Client application request object structure.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char                    deviceNamePtr[LE_PORT_MAX_LEN_DEVICE_NAME]; ///< The device name.
    int32_t                 fd;                                         ///< The device indentifier.
    int32_t                 dataModeFd;                                 ///< The device indentifier
                                                                        ///< specific to data mode.
    le_port_DeviceRef_t     deviceRef;                                  ///< Device reference for
                                                                        ///< client.
    le_atServer_DeviceRef_t atServerDevRef;                             ///< AT server device
                                                                        ///< reference.
    le_msg_SessionRef_t     sessionRef;                                 ///< Client session
                                                                        ///< identifier.
    le_dls_Link_t           link;                                       ///< Object node link.
}
DeviceObject_t;


//--------------------------------------------------------------------------------------------------
/**
 * Memory Pool for Port Service Client Handler.
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t PortPoolRef;

//--------------------------------------------------------------------------------------------------
/**
 * Safe Reference Map for service activation requests.
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t DevicesRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Device object list.
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t DeviceList;


//--------------------------------------------------------------------------------------------------
/**
 * This function opens the device.
 *
 * @return
 *      - -1 if function failed.
 *      - Valid file descriptor.
 *
 * @note This is a stub code to test the port service APIs.
 */
//--------------------------------------------------------------------------------------------------
static int32_t OpenDevice
(
    char* deviceName,  ///< [IN] File descriptor.
    bool blokingMode   ///< [IN] Device blocking mode. 0 -> non-blocking mode, 1 -> blocking mode.
)
{
    int32_t fd;

    fd = le_tty_Open(deviceName, O_RDWR | O_NOCTTY);
    if (-1 == fd)
    {
        LE_ERROR("Failed to open device");
        return -1;
    }

    if (LE_OK != le_tty_SetBaudRate(fd, LE_TTY_SPEED_115200))
    {
        LE_ERROR("Failed to configure TTY baud rate");
        goto error;
    }

    if (LE_OK != le_tty_SetFraming(fd, 'N', 8, 1))
    {
        LE_ERROR("Failed to configure TTY framing");
        goto error;
    }

    if (LE_OK != le_tty_SetFlowControl(fd, LE_TTY_FLOW_CONTROL_NONE))
    {
        LE_ERROR("Failed to configure TTY flow control");
        goto error;
    }

    if (blokingMode == false)
    {
        // Set serial port into raw (non-canonical) mode. Disables conversion of EOL characters,
        // disables local echo, numChars = 0 and timeout = 0: Read will be completetly non-blocking.
        if (LE_OK != le_tty_SetRaw(fd, 0, 0))
        {
            LE_ERROR("Failed to configure TTY raw");
            goto error;
        }
    }
    else
    {
        // Set serial port into raw (non-canonical) mode. Disables conversion of EOL characters,
        // disables local echo, numChars = 50 and timeout = 50: Read will be completetly blocking.
        if (LE_OK != le_tty_SetRaw(fd, 50, 50))
        {
            LE_ERROR("Failed to configure TTY raw");
            goto error;
        }
    }

    return fd;

error:
    // Close the TTY
    le_tty_Close(fd);

    return -1;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function requests to open a configured device. If the device was not opened, it opens
 * the device.
 *
 * @return
 *      - Reference to the device.
 *      - NULL if the device is not available.
 *
 * @note This is a stub code to test the port service APIs.
 */
//--------------------------------------------------------------------------------------------------
le_port_DeviceRef_t le_port_Request
(
    const char* deviceNamePtr  ///< [IN] Device name to be requested.
)
{
    if ((NULL == deviceNamePtr) || !(strcmp(deviceNamePtr, "")))
    {
        LE_ERROR("deviceNamePtr is not valid !");
        return NULL;
    }

    DeviceObject_t* deviceObjectPtr = le_mem_ForceAlloc(PortPoolRef);

    // Initialize the data mode file descriptor.
    deviceObjectPtr->dataModeFd = 0;

    le_port_DeviceRef_t devRef = le_ref_CreateRef(DevicesRefMap, deviceObjectPtr);
    if (NULL == devRef)
    {
        LE_ERROR("devRef is NULL !");
        return NULL;
    }

    // Save the device name.
    le_utf8_Copy(deviceObjectPtr->deviceNamePtr, "/dev/ttyHSL1", LE_PORT_MAX_LEN_DEVICE_NAME, NULL);

    // Save the client session.
    deviceObjectPtr->sessionRef = le_port_GetClientSessionRef();
    deviceObjectPtr->deviceRef = devRef;

    // Open the device (for example: /dev/ttyHSL1)
    deviceObjectPtr->fd = OpenDevice("/dev/ttyHSL1", NON_BLOCKING_MODE);
    if (0 > deviceObjectPtr->fd)
    {
        LE_ERROR("Error in opening the device '%s': %m", deviceNamePtr);
        return NULL;
    }

    deviceObjectPtr->atServerDevRef = le_atServer_Open(deviceObjectPtr->fd);
    le_dls_Queue(&DeviceList, &(deviceObjectPtr->link));

    return devRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function switches the device into data mode.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 *
 * @note This is a stub code to test the port service APIs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_port_SetDataMode
(
    le_port_DeviceRef_t devRef,   ///< [IN] Device reference.
    int32_t* fdPtr                ///< [OUT] File descriptor of the device.
)
{
    le_result_t result;

    DeviceObject_t* deviceObjectPtr = le_ref_Lookup(DevicesRefMap, devRef);
    if (NULL == deviceObjectPtr)
    {
        LE_ERROR("devRef is invalid !");
        return LE_BAD_PARAMETER;
    }

    if (NULL == fdPtr)
    {
        LE_ERROR("fdPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    le_atServer_DeviceRef_t atServerDeviceRef = deviceObjectPtr->atServerDevRef;
    result = le_atServer_Suspend(atServerDeviceRef);
    if (LE_FAULT == result)
    {
        LE_ERROR("Device is not able to switch into data mode");
        return LE_FAULT;
    }

    deviceObjectPtr->dataModeFd = OpenDevice(deviceObjectPtr->deviceNamePtr, BLOCKING_MODE);

    if (deviceObjectPtr->dataModeFd != -1)
    {
        *fdPtr = (deviceObjectPtr->dataModeFd);
        return LE_OK;
    }
    else
    {
        *fdPtr = -1;
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function switches the device into AT command mode.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 *
 * @note This is a stub code to test the port service APIs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_port_SetCommandMode
(
    le_port_DeviceRef_t devRef    ///< [IN] Device reference.
)
{
    le_result_t result;

    DeviceObject_t* deviceObjectPtr = le_ref_Lookup(DevicesRefMap, devRef);
    if (NULL == deviceObjectPtr)
    {
        LE_ERROR("devRef is invalid !");
        return LE_BAD_PARAMETER;
    }

    le_atServer_DeviceRef_t atServerDeviceRef = deviceObjectPtr->atServerDevRef;
    result = le_atServer_Resume(atServerDeviceRef);
    if (LE_FAULT == result)
    {
        LE_ERROR("Device is not able to switch into command mode");
        return LE_FAULT;
    }

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
 *
 * @note This is a stub code to test the port service APIs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_port_Release
(
    le_port_DeviceRef_t devRef    ///< [IN] Device reference.
)
{
    le_result_t result;

    DeviceObject_t* deviceObjectPtr = le_ref_Lookup(DevicesRefMap, devRef);
    if (NULL == deviceObjectPtr)
    {
        LE_ERROR("devRef is invalid !");
        return LE_BAD_PARAMETER;
    }

    le_atServer_DeviceRef_t atServerDeviceRef = deviceObjectPtr->atServerDevRef;
    result = le_atServer_Close(atServerDeviceRef);
    if (LE_FAULT == result)
    {
        LE_ERROR("Device is not able to close");
        return LE_FAULT;
    }

    le_ref_DeleteRef(DevicesRefMap, devRef);
    le_mem_Release(deviceObjectPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function gets the device reference regarding to a given reference coming from the AT server.
 *
 * @return
 *      - LE_OK            Function succeeded.
 *      - LE_FAULT         Function failed.
 *      - LE_BAD_PARAMETER Invalid parameter.
 *
 * @note This is a stub code to test the port service APIs.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_port_GetPortReference
(
    le_atServer_DeviceRef_t atServerDevRef,   ///< [IN] Device reference from AT server.
    le_port_DeviceRef_t* devRefPtr            ///< [OUT] Device reference from port service.
)
{
    if (NULL == devRefPtr)
    {
        LE_ERROR("devRefPtr is NULL !");
        return LE_BAD_PARAMETER;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(&DeviceList);

    while (linkPtr)
    {
        DeviceObject_t* deviceObjectPtr = CONTAINER_OF(linkPtr, DeviceObject_t, link);
        linkPtr = le_dls_PeekNext(&DeviceList, linkPtr);

        if (deviceObjectPtr->atServerDevRef == atServerDevRef)
        {
            *devRefPtr = deviceObjectPtr->deviceRef;
            return LE_OK;
        }
    }

    *devRefPtr = NULL;
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the port service.
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Create safe reference map for request references.
    DevicesRefMap = le_ref_CreateMap("DeviceRef", MAX_APPS);

    // Create a pool for client objects.
    PortPoolRef = le_mem_CreatePool("PortPoolRef", sizeof(DeviceObject_t));
    le_mem_ExpandPool(PortPoolRef, MAX_APPS);

    DeviceList = LE_DLS_LIST_INIT;
}
