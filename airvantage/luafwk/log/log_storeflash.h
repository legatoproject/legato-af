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

#ifndef LOGSTOREFLASH_H_
#define LOGSTOREFLASH_H_

#include "lua.h"

int l_logflashinit(lua_State* L);
int l_logflashstore(lua_State* L);
int l_logflashgetsource(lua_State* L);
int l_logflashdebug(lua_State* L);

#endif /* LOGSTOREFLASH_H_ */
