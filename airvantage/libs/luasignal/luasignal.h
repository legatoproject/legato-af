/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Sierra Wireless - initial API and implementation
 *******************************************************************************/
/*
 * signal.h
 *
 *  Created on: Aug 21, 2009
 *      Author: cbugot
 */

#ifndef SIGNAL_H_
#define SIGNAL_H_

#include <stdint.h>
#include "returncodes.h"

/*
 * Usage notes:
 * The API is NOT thread safe, but in the standard usage it is not a prb.
 * Do not call the Destroy function in the callback thread, instead use another thread.
 * The const char arrays are terminated with a null value
 *
 * The standard usage is:
 * Init the LUASIGNAL with a direct LuaVM call (lua function in C)
 * Receive and Send signals in the callback that was given in the Init function
 * Destroy the LUASIGNAL with a direct LuaVM call. This call will be blocking until the callback finishes.
 */

typedef void (*HookCB)(const char* emitter, const char* event, const char* args[], const uint16_t args_length[]);
typedef struct LuaSignalCtx LuaSignalCtx;

rc_ReturnCode_t LUASIGNAL_Init(LuaSignalCtx** ctx, int port, const char* emitters[], HookCB hook);
rc_ReturnCode_t LUASIGNAL_SignalT(LuaSignalCtx* ctx, const char* emitter, const char* event, const char* args[]);
rc_ReturnCode_t LUASIGNAL_SignalB(LuaSignalCtx* ctx, const char* emitter, const char* event, const char* args[], const uint16_t args_length[]);
rc_ReturnCode_t LUASIGNAL_Destroy(LuaSignalCtx* ctx);

#endif /* SIGNAL_H_ */
