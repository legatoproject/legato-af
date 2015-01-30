/**
 * @page c_atdevice AT Device
 *
 * This module provide just a definition to @ref atdevice_Ref_t
 *
 * Because it it just use internally, it does not need anything more yet.
 *
 * @todo
 *  if needed create some API to manage an atdevice:
 * - set/get the open function
 * - set/get the close function
 * - set/get the read function
 * - set/get the write function
 *
 */

/** @file atDevice.h
 *
 * Legato @ref c_atdevice include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef LEGATO_ATDEVICE_INCLUDE_GUARD
#define LEGATO_ATDEVICE_INCLUDE_GUARD

#include "../devices/adapter_layer/inc/le_da.h"

//--------------------------------------------------------------------------------------------------
/**
 * This will be the reference for the device of one atmanager
 */
//--------------------------------------------------------------------------------------------------
typedef struct atdevice* atdevice_Ref_t;

#endif /* LEGATO_ATDEVICE_INCLUDE_GUARD */
