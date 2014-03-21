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

/*
 * lposixsignal.h
 *
 *  Created on: 21 fev 2012
 *      Author: Gilles
 */

#ifndef __LPOSIXSIGNAL_H__
#define __LPOSIXSIGNAL_H__

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

int luaopen_sched_posixsignal(lua_State* L);

#endif /*__LPOSIXSIGNAL_H__*/
