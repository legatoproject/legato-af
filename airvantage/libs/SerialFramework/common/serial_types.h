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
 * serial_types.h
 *
 *  Created on: 18 mars 2010
 *      Author: Gilles
 */

#ifndef SERIAL_TYPES_H_
#define SERIAL_TYPES_H_

#include <stdint.h>

struct SerialContext_;
typedef struct SerialContext_ SerialContext;

/**
 * @enum SerialUARTId
 * \brief the serial uart identity
 */
typedef enum {
    SERIAL_UART1 = 1,
    SERIAL_UART2 = 2,
    SERIAL_UART3 = 3
} SerialUARTId;

/**
 * @enum SerialUARTBaudrate
 * \brief the serial uart baudrate
 */
typedef enum {
    SERIAL_UART_BAUDRATE_300 = 3,
    SERIAL_UART_BAUDRATE_600 = 6,
    SERIAL_UART_BAUDRATE_1200 = 12,
    SERIAL_UART_BAUDRATE_2400 = 24,
    SERIAL_UART_BAUDRATE_4800 = 48,
    SERIAL_UART_BAUDRATE_9600 = 96,
    SERIAL_UART_BAUDRATE_19200 = 192,
    SERIAL_UART_BAUDRATE_38400 = 384,
    SERIAL_UART_BAUDRATE_57600 = 576,
    SERIAL_UART_BAUDRATE_115200 = 1152
} SerialUARTBaudrate;

/**
 * @enum SerialUARTParity
 * \brief the serial uart parity
 */
typedef enum {
    SERIAL_UART_NO_PARITY = 0, SERIAL_UART_ODD_PARITY = 1, SERIAL_UART_EVEN_PARITY = 2
} SerialUARTParity;

/**
 * @enum SerialUARTStop
 * \brief the serial uart stop bit length
 */
typedef enum {
    SERIAL_UART_STOP_1 = 1, SERIAL_UART_STOP_2 = 2
} SerialUARTStop;

/**
 * @enum SerialUARTData
 * \brief the serial uart data length
 */
typedef enum {
    SERIAL_UART_DATA_7 = 7, SERIAL_UART_DATA_8 = 8
} SerialUARTData;

/**
 * @enum SerialUARTFControl
 * \brief the serial uart flow control
 */
typedef enum {
    SERIAL_UART_FC_NONE = 0, SERIAL_UART_FC_XON_XOFF = 1, SERIAL_UART_FC_RTS_CTS = 2
} SerialUARTFControl;


/**
 * @enum SerialGPIOWriteModeLevel
 * \brief the serial gpio write level
 */
typedef enum {
    SERIAL_GPIO_HIGH = 1, SERIAL_GPIO_LOW = 0
} SerialGPIOWriteModeLevel;

/**
 * @enum SerialStatus
 * \brief the serial status code
 */
//when adding status code here, also add it in SerialStatusString
//and update statusToString function in lua_serial_fwk.c
typedef enum {
    SERIAL_STATUS_OK = 0,
    SERIAL_STATUS_STACK_NOT_READY = 1,
    SERIAL_STATUS_RESPONSE_INVALID_FRAME = 2,
    SERIAL_STATUS_RESPONSE_SHORT_FRAME = 3,
    SERIAL_STATUS_RESPONSE_BAD_SLAVE = 4,
    SERIAL_STATUS_RESPONSE_BAD_FUNCTION = 5,
    SERIAL_STATUS_RESPONSE_BAD_CHECKSUM = 6,
    SERIAL_STATUS_RESPONSE_INCOMPLETE_FRAME = 7,
    SERIAL_STATUS_RESPONSE_EXCEPTION = 8,
    SERIAL_STATUS_RESPONSE_TIMEOUT = 9,
    SERIAL_STATUS_ALLOC_FAILED = 10,
    SERIAL_STATUS_WRONG_PARAMS = 11,
    SERIAL_STATUS_CORRUPTED_CONTEXT = 12,
    SERIAL_STATUS_REQUEST_PARAMETER_ERROR = 13,
    SERIAL_STATUS_UNEXPECTED_ERROR = 14
} SerialStatus;


/**
 * @struct SerialConfig_
 */
typedef struct SerialConfig_ {
    /* line parameters */
    SerialUARTId identity;
    SerialUARTBaudrate baudrate;
    SerialUARTParity parity;
    SerialUARTStop stop;
    SerialUARTData data;
    SerialUARTFControl flowControl;

    /* gpio modes */
    SerialGPIOWriteModeLevel gpioLevel; // gpio write level (non-mandatory)

    /* id */
    uint32_t stackId;         // must be unique among all procotocols (for oat message passing)

    /* transport behaviour */
    uint8_t  sendASAP;        // if true request can be sent without waiting for previous request's response (0 means false)
    uint8_t  flushBeforeSend; // if true flush UART service provider transmission and reception FIFO before each send.
    uint8_t  listenASAP;      // if true transport will listen for incoming data as startup and as soon as a valid response as been processed (0 means false)
    uint8_t  noTimeoutOnUnso; // if true and if listenASAP is true, timeout will be disabled on unsollicited response ( [response]<-timeout-> )

    /* transport options */
    uint16_t timeout;         // [request]<-timeout->[response], request-response timeout value x100ms (0 means no timeout)
                              // if listenASAP is true and timeoutOnUnso is true: [response]<-timeout->,  response timeout value x100ms (0 means no timeout)
    uint16_t retry;           // request max retry (0 means no retry)
    uint16_t maxBufferSize;   // reception and send buffer max length (generally equal to serializer buffers' maxsize)
} SerialConfig;

#endif /* SERIAL_TYPES_H_ */
