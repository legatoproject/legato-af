 /*******************************************************************************
 * Copyright (c) 2012 Sierra Wireless and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *   http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php
 *
 * Contributors:
 *     Sierra Wireless - initial API and implementation
 *******************************************************************************/

#ifndef AGENT_PLATFORM_CORE_H_
#define AGENT_PLATFORM_CORE_H_

void agent_platform_core_log_init(lua_State *L);
void agent_platform_core_event_init(lua_State *L);
void agent_platform_core_nm_init(lua_State *L);

#endif /* AGENT_PLATFORM_CORE_H_ */
