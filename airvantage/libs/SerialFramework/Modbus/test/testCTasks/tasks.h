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

#ifndef TASKS_H_
#define TASKS_H_

#include "modbus_serializer.h"
#include "serial_oat_adapter.h"

#include "adl_global.h"

typedef struct ModbusUserData_ {
    SerialContext* pSerialContext; // context
    ModbusRequest request; // request
    void* allocated;
} ModbusUserData;


#endif /* TASKS_H_ */
