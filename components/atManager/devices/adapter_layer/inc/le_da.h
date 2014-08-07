/**
 * @page c_da Devices Adapter API
 *
 * This file contains the prototypes definitions wich are common to the Device Adapter APIs.
 *
 *
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


/** @file le_da.h
 *
 * Legato @ref c_da include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#ifndef LEGATO_DA_INCLUDE_GUARD
#define LEGATO_DA_INCLUDE_GUARD

#include "legato.h"

typedef le_result_t (*le_da_DeviceOpenFunc_t)( const char* path );
typedef le_result_t (*le_da_DeviceReadFunc_t)( uint32_t Handle, void* pData, uint32_t amount );
typedef le_result_t (*le_da_DeviceWriteFunc_t)( uint32_t Handle, const void* pData, uint32_t length );
typedef le_result_t (*le_da_DeviceIoControlFunc_t)( uint32_t Handle, uint32_t Cmd, void* pParam );
typedef le_result_t (*le_da_DeviceCloseFunc_t)( uint32_t Handle );

typedef struct
{
    le_da_DeviceOpenFunc_t      open;
    le_da_DeviceReadFunc_t      read;
    le_da_DeviceWriteFunc_t     write;
    le_da_DeviceIoControlFunc_t io_control;
    le_da_DeviceCloseFunc_t     close;
} le_da_Device_t, *le_da_DeviceRef_t;

#endif // LEGATO_DA_INCLUDE_GUARD
