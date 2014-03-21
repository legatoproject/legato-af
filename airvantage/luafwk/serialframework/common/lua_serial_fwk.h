/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Gilles Cannenterre for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#ifndef __LUA_SERIAL_FWK_H__
#define __LUA_SERIAL_FWK_H__

#include "serial_types.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

// Retrieves serial port name
void GetConfigIdentity(lua_State* L, int index, SerialUARTId* identity);

// Retrieves baudrate, parity, data, stop, flowcontrol,t timeout, retry
void GetConfigUART(lua_State* L, int index, SerialConfig* serialConfig);


void GetConfigBaudrate(lua_State* L, int index, SerialUARTBaudrate* baudrate);
void GetConfigParity(lua_State* L, int index, SerialUARTParity* parity);
void GetConfigData(lua_State* L, int index, SerialUARTData* data);
void GetConfigStop(lua_State* L, int index, SerialUARTStop* stop);
void GetConfigFlowControl(lua_State* L, int index, SerialUARTFControl* flowControl);
void GetConfigTimeout(lua_State* L, int index, uint16_t* timeout);
void GetConfigRetry(lua_State* L, int index, uint16_t* retry);

// Retrieves the GPIO level
void GetConfigLevel(lua_State* L, int index, SerialGPIOWriteModeLevel* level);

const char* statusToString(SerialStatus status);

#endif /*__LUA_SERIAL_FWK_H__*/
