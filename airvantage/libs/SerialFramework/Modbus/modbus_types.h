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

#ifndef MODBUS_TYPES_H_
#define MODBUS_TYPES_H_

/**
 * @file modbus_types.h
 * @brief modbus public api.
 *
 * @ingroup modbus
 */

#include <stdint.h>
#include <stddef.h>

#define MODBUS_RTU_MAX_FRAME_SIZE                    256
#define MODBUS_ASCII_MAX_FRAME_SIZE                  514
#define MODBUS_TCP_MAX_FRAME_SIZE                    260

#define MODBUS_TCP_MAX_DATA_SIZE                    (MODBUS_TCP_MAX_FRAME_SIZE-6)  // TCP MAX - MBAP size

/**
 * @enum ModbusFunctionCode
 * \brief modbus function code
 *
 * This enum is 'statically' enumerated. It is FORBIDEN to change any of the following values.
 */
typedef enum {
    MODBUS_FUNC_READ_COILS = (0x01),
    MODBUS_FUNC_READ_DISCRETE_INPUTS = (0x02),
    MODBUS_FUNC_READ_HOLDING_REGISTERS = (0x03),
    MODBUS_FUNC_READ_INPUT_REGISTERS = (0x04),
    MODBUS_FUNC_WRITE_SINGLE_COIL = (0x05),
    MODBUS_FUNC_WRITE_SINGLE_REGISTER = (0x06),
    MODBUS_FUNC_READ_EXCEPTION_STATUS = (0x07),
    MODBUS_FUNC_DIAGNOSTICS = (0x08),
    MODBUS_FUNC_GET_COMM_EVENT_COUNTER = (0x0B),
    MODBUS_FUNC_GET_COMM_EVENT_LOG = (0x0C),
    MODBUS_FUNC_WRITE_MULTIPLE_COILS = (0x0F),
    MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS = (0x10),
    MODBUS_FUNC_REPORT_SLAVE_ID = (0x11),
    MODBUS_FUNC_READ_FILE_RECORD = (0x14),
    MODBUS_FUNC_WRITE_FILE_RECORD = (0x15),
    MODBUS_FUNC_MASK_WRITE_REGISTER = (0x16),
    MODBUS_FUNC_READWRITE_MULTIPLE_REGISTERS = (0x17),
    MODBUS_FUNC_READ_FIFO_QUEUE = (0x18),
    MODBUS_FUNC_ENCAPSULATED_INTERFACE_TRANSPORT = (0x2B),
    MODBUS_FUNC_SEND_RAW_DATA = (0x5A),

    MODBUS_MASK_ERROR = ((uint8_t) 0x80)
} ModbusFunctionCode;

/**
 * @enum ModbusExceptionCode
 * \brief modbus exception code
 *
 * This enum is 'statically' enumerated. It is FORBIDEN to change any of the following values.
 */
typedef enum {
    MODBUS_NO_EXCEPTION = (0x00),

    MODBUS_ILLEGAL_FUNCTION = (0x01),
    MODBUS_ILLEGAL_DATA_ADDRESS = (0x02),
    MODBUS_ILLEGAL_DATA_VALUE = (0x03),
    MODBUS_SLAVE_DEVICE_FAILURE = (0x04),
    MODBUS_ACKNOWLEDGE = (0x05),
    MODBUS_SLAVE_DEVICE_BUSY = (0x06),
    MODBUS_MEMORY_PARITY_ERROR = (0x08),
    MODBUS_GATEWAY_PATH_UNAVAILABLE = (0x0A),
    MODBUS_TARGET_DEVICE_FAILED_TO_RESPOND = (0x0B)
} ModbusExceptionCode;

/**
 * @enum ModbusCoilState
 * \brief modbus coil state
 *
 * This enum is 'statically' enumerated. It is FORBIDEN to change any of the following values.
 */
typedef enum {
    MODBUS_SINGLE_COIL_ON = ((uint16_t) 0xFF00), MODBUS_SINGLE_COIL_OFF = ((uint16_t) 0x0000)
} ModbusCoilState;

/**
 * @enum ModbusRequestMode
 * \brief the mode bus mode (RTU or ASCII)
 */
typedef enum {
    MODBUS_RTU = 1, MODBUS_ASCII = 2, MODBUS_TCP = 3
} ModbusRequestMode;

#endif /* MODBUS_TYPES_H_ */
