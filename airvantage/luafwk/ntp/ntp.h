/*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Laurent Barthelemy for Sierra Wireless - initial API and implementation
 *******************************************************************************/

#ifndef NTP_H_
#define NTP_H_
#include "lua.h"
#include "stdint.h"

// Time conversion constants
// number of seconds between ntp timestamp day 0 (1900) and unix timestamp day 0 (1970)
#define NTP_TIME_BASE  (2208988800UL)

//function use to get/set time
//system specific
int internal_getntptime(uint64_t *time);
int internal_setntptime(uint64_t time);

int luaopen_ntp_core(lua_State *L);

#endif /* NTP_H_ */
