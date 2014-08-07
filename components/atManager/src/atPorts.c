/**
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file atports.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "../inc/atPorts.h"
#include "atPortsInternal.h"
#include "atMachineDevice.h"
#include "../devices/adapter_layer/inc/le_da.h"
#include "../devices/uart/inc/le_uart.h"

/*
 * array of all Ports created.
 * This will correspond to ATPORT_MAX threads created
 */
static atmgr_Ref_t   AllPorts[ATPORT_MAX];

static bool IsInitialized = false;

/*
 * Create a Command Port for AT_COMMAND
 */
static void CreateAtPortCommand
(
    void
)
{
    struct atdevice atDevice;

    memset(&atDevice,0,sizeof(atDevice));

    le_utf8_Copy(atDevice.name,"ATCMD",sizeof(atDevice.name),0);
    le_utf8_Copy(atDevice.path,AT_COMMAND,sizeof(atDevice.path),0);
    atDevice.deviceItf.open = (le_da_DeviceOpenFunc_t)le_uart_open;
    atDevice.deviceItf.read = (le_da_DeviceReadFunc_t)le_uart_read;
    atDevice.deviceItf.write = (le_da_DeviceWriteFunc_t)le_uart_write;
    atDevice.deviceItf.io_control = (le_da_DeviceIoControlFunc_t)le_uart_ioctl;
    atDevice.deviceItf.close = (le_da_DeviceCloseFunc_t)le_uart_close;

    AllPorts[ATPORT_COMMAND] = atmgr_CreateInterface(&atDevice);

    LE_FATAL_IF(AllPorts[ATPORT_COMMAND]==NULL, "Could not create port for '%s'",AT_COMMAND);

    LE_DEBUG("Port %s [%s] is created","ATCMD",AT_COMMAND);
}

/*
 * Create a ppp Port for AT_PPP
 */
static void CreateAtPortPpp
(
    void
)
{
    struct atdevice atDevice;

    memset(&atDevice,0,sizeof(atDevice));

    le_utf8_Copy(atDevice.name,"PPP",sizeof(atDevice.name),0);
    le_utf8_Copy(atDevice.path,AT_PPP,sizeof(atDevice.path),0);
    atDevice.deviceItf.open = (le_da_DeviceOpenFunc_t)le_uart_open;
    atDevice.deviceItf.read = (le_da_DeviceReadFunc_t)le_uart_read;
    atDevice.deviceItf.write = (le_da_DeviceWriteFunc_t)le_uart_write;
    atDevice.deviceItf.io_control = (le_da_DeviceIoControlFunc_t)le_uart_ioctl;
    atDevice.deviceItf.close = (le_da_DeviceCloseFunc_t)le_uart_close;

    AllPorts[ATPORT_PPP] = atmgr_CreateInterface(&atDevice);

    LE_FATAL_IF(AllPorts[ATPORT_PPP]==NULL, "Could not create port for '%s'",AT_PPP);

    LE_DEBUG("Port %s [%s] is created","PPP",AT_PPP);
}

/*
 * Create a gnss Port for AT_GNSS
 */
static void CreateAtPortGnss
(
    void
)
{
    struct atdevice atDevice;

    memset(&atDevice,0,sizeof(atDevice));

    le_utf8_Copy(atDevice.name,"GNSS",sizeof(atDevice.name),0);
    le_utf8_Copy(atDevice.path,AT_GNSS,sizeof(atDevice.path),0);
    atDevice.deviceItf.open = (le_da_DeviceOpenFunc_t)le_uart_open;
    atDevice.deviceItf.read = (le_da_DeviceReadFunc_t)le_uart_read;
    atDevice.deviceItf.write = (le_da_DeviceWriteFunc_t)le_uart_write;
    atDevice.deviceItf.io_control = (le_da_DeviceIoControlFunc_t)le_uart_ioctl;
    atDevice.deviceItf.close = (le_da_DeviceCloseFunc_t)le_uart_close;

    AllPorts[ATPORT_GNSS] = atmgr_CreateInterface(&atDevice);

    LE_FATAL_IF(AllPorts[ATPORT_GNSS]==NULL, "Could not create port for '%s'",AT_GNSS);

    LE_DEBUG("Port %s [%s] is created","GNSS",AT_GNSS);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function Initialize the all ports that will be available.
 * Must be call only once
 *
 * @return LE_OK           The function succeeded.
 * @return LE_DUPLICATE    Module is already initialized
 */
//--------------------------------------------------------------------------------------------------
le_result_t atports_Init
(
)
{
    if ( IsInitialized ) {
        return LE_DUPLICATE;
    }

    memset(AllPorts,0,sizeof(AllPorts));

    CreateAtPortCommand();
    CreateAtPortPpp();
    CreateAtPortGnss();

    IsInitialized = true;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get an interface for the given name.
 *
 * @return a reference on the ATManager of this device
 */
//--------------------------------------------------------------------------------------------------
atmgr_Ref_t atports_GetInterface
(
    atports_t name
)
{
    return AllPorts[name];
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set an interface for the given name.
 *
 * @warning This function is a memory leak, it is just use for testing should not be used otherwise.
 *
 */
//--------------------------------------------------------------------------------------------------
inline void atports_SetInterface
(
    atports_t    name,
    atmgr_Ref_t  interfaceRef
)
{
    AllPorts[name] = interfaceRef;
}
