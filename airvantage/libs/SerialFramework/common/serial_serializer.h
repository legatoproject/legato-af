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
 * serializer.h
 *
 *  Created on: 22 fevr 2011
 *      Author: Gilles
 */

#ifndef SERIAL_SERIALIZER_H_
#define SERIAL_SERIALIZER_H_

#include <stdint.h>
#include "serial_types.h"

struct Serializer_;
typedef struct Serializer_ Serializer;

/* initialize */
typedef SerialStatus (* SRLFWK_SER_InitSerializer)(Serializer* pSerializer, void* pSerializerData);
/* release all allocated resources */
typedef void (*SRLFWK_SER_ReleaseSerializer)(Serializer* pSerializer);
/* builds requests */
typedef SerialStatus (*SRLFWK_SER_RequestBuilder)(Serializer* pSerializer, void* pRequestData);
/* verify on the fly the response buffer and returns true if this buffer contains a complete response */
typedef uint8_t (*SRLFWK_SER_IsResponseComplete)(Serializer* pSerializer);
/* check the response buffer and compute a status */
typedef SerialStatus (*SRLFWK_SER_ResponseChecker)(Serializer* pSerializer);
/* analyze the response buffer to compute a status and extract all required data */
typedef SerialStatus (*SRLFWK_SER_ResponseAnalyzer)(Serializer* pSerializer, SerialStatus status);
/* retrieve request and response buffers */
typedef SerialStatus (* SRLFWK_SER_GetRequestPDU)(Serializer* pSerializer, uint8_t** ppBuffer, uint16_t* pBufferLength);
typedef SerialStatus (* SRLFWK_SER_GetResponsePDU)(Serializer* pSerializer, uint8_t** ppBuffer, uint16_t* pBufferLength);
/* get length of an anticipated response */
typedef SerialStatus (* SRLFWK_SER_GetExpectedResponseLength)(Serializer* pSerializer);

/* protocol types */
typedef enum {
    SRLFWK_SER_REQ_RSP_STRICT, // strict request response protocols (no unsollicited response), ex: modbus
    SRLFWK_SER_REQ_RSP,        // request response protocols allowing unsollicited response, ex: atlas
    SRLFWK_SER_RSP_ONLY,       // strict unsollicited response protocols, ex: teleinfo
    SRLFWK_SER_CUSTOM,         // none of the above...
} SRLFWK_SER_ProtocolType;

struct Serializer_ {
    /* buffers */
    uint16_t requestBufferLength; // request buffer data length
    uint16_t responseBufferLength; // response buffer data length
    uint8_t* pRequestBuffer; // request buffer
    uint8_t* pResponseBuffer; // response buffer
    uint8_t* pTempBuffer; // temp buffer
    uint16_t maxSize; //PDU max size

    /* functions */
    SRLFWK_SER_ReleaseSerializer releaseSerializer;
    SRLFWK_SER_RequestBuilder requestBuilder;
    SRLFWK_SER_IsResponseComplete isResponseComplete;
    SRLFWK_SER_ResponseChecker responseChecker;
    SRLFWK_SER_ResponseAnalyzer responseAnalyzer;

    /* protocol specifics */
    SRLFWK_SER_ProtocolType type;
    void* pSpecifics;
};

#endif /* SERIAL_SERIALIZER_H_ */
