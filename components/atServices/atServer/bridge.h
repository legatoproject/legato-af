/** @file bridge.h
 *
 * Implementation of the AT commands server <-> AT commands client bridge.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_LE_BRIDGE_INCLUDE_GUARD
#define LEGATO_LE_BRIDGE_INCLUDE_GUARD

#include "legato.h"
#include "interfaces.h"
#include "le_atServer_local.h"

//--------------------------------------------------------------------------------------------------
/**
 * Create a modem AT command and return the reference of command description pointer
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to create the command.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t bridge_Create
(
    char*  atCmdPtr,
    void** descRefPtr   ///<OUT
);

//--------------------------------------------------------------------------------------------------
/**
 * Bridge initialization
 *
 *
 */
//--------------------------------------------------------------------------------------------------
void bridge_Init
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * This function opens a bridge with the modem.
 *
 * @return
 *      - Reference to the requested bridge.
 *      - NULL if the device is not available.
 */
//--------------------------------------------------------------------------------------------------
le_atServer_BridgeRef_t bridge_Open
(
    int fd
);

//--------------------------------------------------------------------------------------------------
/**
 * This function closes an open bridge.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to close the bridge.
 *      - LE_BUSY          The bridge is in use.
 */
//--------------------------------------------------------------------------------------------------
le_result_t bridge_Close
(
    le_atServer_BridgeRef_t bridgeRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a device to the bridge.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_BUSY          The device is already used by the bridge.
 *      - LE_FAULT         The function failed to create the command.
 */
//--------------------------------------------------------------------------------------------------
le_result_t bridge_AddDevice
(
    le_atServer_DeviceRef_t deviceRef,
    le_atServer_BridgeRef_t bridgeRef
);

//--------------------------------------------------------------------------------------------------
/**
 * This function removes a device from the bridge.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to create the command.
 *      - LE_NOT_FOUND     The requested device is not found.
 */
//--------------------------------------------------------------------------------------------------
le_result_t bridge_RemoveDevice
(
    le_atServer_DeviceRef_t deviceRef,
    le_atServer_BridgeRef_t bridgeRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Clean the bridge context when the close session service handler is invoked
 *
 */
//--------------------------------------------------------------------------------------------------
void bridge_CleanContext
(
    le_msg_SessionRef_t sessionRef
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the session reference of the bridge device
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t bridge_GetSessionRef
(
    le_atServer_BridgeRef_t  bridgeRef,
    le_msg_SessionRef_t* sessionRefPtr
);

//--------------------------------------------------------------------------------------------------
/**
 * Release AT modem bridge command
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t bridge_ReleaseModemCmd
(
    void* descRefPtr
);

#endif //LEGATO_LE_BRIDGE_INCLUDE_GUARD
