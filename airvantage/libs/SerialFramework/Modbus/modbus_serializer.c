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

#include "modbus_serializer.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MODBUS_MAX_READ_COILS                    2000
#define MODBUS_MAX_READ_REGISTERS                 125
#define MODBUS_MAX_NUMBER_OF_WRITTEN_COILS       1968
#define MODBUS_MAX_NUMBER_OF_WRITTEN_REGISTERS    123
#define MODBUS_ASCII_START_SEQUENCE               ":"
#define MODBUS_ASCII_START_CHAR                   ':'
#define MODBUS_ASCII_END_SEQUENCE              "\r\n"
#define MODBUS_ASCII_END_CHAR1                   '\r'
#define MODBUS_ASCII_END_CHAR2                   '\n'

// statics
static const char* exceptionMessages[] = { "no exception", "illegal function", "illegal data address", "illegal data value", "slave device failure", "acknowledge", "slave device busy",
        "memory parity error", "gateway path unavailable", "target device failed to respond", "unresolved exception" };

static const uint8_t cRCHi[] = { 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
        0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80,
        0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80,
        0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
        0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80,
        0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
        0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
        0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80,
        0x41, 0x00, 0xC1, 0x81, 0x40 };

static const uint8_t cRCLo[] = { 0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB,
        0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13,
        0xD3, 0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B,
        0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26, 0x22, 0xE2, 0xE3,
        0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B,
        0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5, 0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3,
        0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B,
        0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C, 0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43,
        0x83, 0x41, 0x81, 0x80, 0x40 };

static SerialStatus CreateRequest(Serializer* pSerializer);
static SerialStatus ASCIICreateRequest(Serializer* pSerializer);
static SerialStatus ASCIIParseResponse(Serializer* pSerializer);
static uint8_t ASCIIComputeLRC(uint8_t* pFrame, uint16_t length);
static uint8_t ASCIIValidateLRC(uint8_t* pBuffer, uint16_t bufferSize, uint8_t lCR);
static uint8_t HexDigit2Dec(char hexDigit);
static SerialStatus RTUCreateRequest(Serializer* pSerializer);
static SerialStatus RTUParseResponse(Serializer* pSerializer);
static uint16_t RTUComputeCRC(uint8_t* pFrame, uint16_t length);
static uint8_t RTUValidateCRC(uint8_t* pBuffer, uint16_t bufferSize, uint16_t cRC);
static SerialStatus TCPCreateRequest(Serializer* pSerializer);
static SerialStatus TCPParseResponse(Serializer* pSerializer);
static ModbusFunctionCode ParseFunction(Serializer* pSerializer);
static SerialStatus ParseReadInputs(Serializer* pSerializer);
static SerialStatus ParseReadRegisters(Serializer* pSerializer);
static SerialStatus ParseWriteObject(Serializer* pSerializer);
static SerialStatus ParseRawData(Serializer* pSerializer);

SerialStatus TCPCreateRequest(Serializer* pSerializer) {
    if (pSerializer == NULL) {
        return SERIAL_STATUS_STACK_NOT_READY;
    }

    // fill MBAP
    ((ModbusSpecifics*) pSerializer->pSpecifics)->requestTrId = (((ModbusSpecifics*) pSerializer->pSpecifics)->requestTrId + 1) % 0xFFFF;
    pSerializer->pRequestBuffer[0] = (uint8_t)((((ModbusSpecifics*) pSerializer->pSpecifics)->requestTrId >> 8) & 0xFF);
    pSerializer->pRequestBuffer[1] = (uint8_t)(((ModbusSpecifics*) pSerializer->pSpecifics)->requestTrId & 0xFF);
    pSerializer->pRequestBuffer[2] = 0;
    pSerializer->pRequestBuffer[3] = 0;
    pSerializer->pRequestBuffer[4] = (uint8_t)((pSerializer->requestBufferLength >> 8) & 0xFF);
    pSerializer->pRequestBuffer[5] = (uint8_t)(pSerializer->requestBufferLength & 0xFF);

    pSerializer->requestBufferLength = pSerializer->requestBufferLength + 6;

    return SERIAL_STATUS_OK;
}

SerialStatus TCPParseResponse(Serializer* pSerializer) {
    uint16_t trId;
    uint16_t length;

    if (pSerializer == NULL) {
        return SERIAL_STATUS_STACK_NOT_READY;
    }

    trId = (pSerializer->pResponseBuffer[0] << 8) + pSerializer->pResponseBuffer[1];
    length = (pSerializer->pResponseBuffer[4] << 8) + pSerializer->pResponseBuffer[5];
    //TRACE((1 , "transactionID[%d/%d] - length[%d/%d]", trId, ((ModbusSpecifics*)pSerializer->pSpecifics)->requestTrId, length, pSerializer->responseBufferLength));
    if ((((ModbusSpecifics*) pSerializer->pSpecifics)->requestTrId != trId) || (pSerializer->responseBufferLength != length + 6)) {
        return SERIAL_STATUS_RESPONSE_INVALID_FRAME;
    }

    // process frame
    if ((pSerializer->pResponseBuffer[2] == 0 && pSerializer->pResponseBuffer[3] == 0) && (pSerializer->responseBufferLength >= 8) && (pSerializer->responseBufferLength <= MODBUS_TCP_MAX_FRAME_SIZE)
            && ((((ModbusSpecifics*) pSerializer->pSpecifics)->request.function == pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset + 1])
                    || (((ModbusSpecifics*) pSerializer->pSpecifics)->request.function == ((pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset + 1])
                            ^ MODBUS_MASK_ERROR)))) {

        return SERIAL_STATUS_OK;
    } else {
        if (pSerializer->responseBufferLength < 8) {
            return SERIAL_STATUS_RESPONSE_SHORT_FRAME;
        }

        if (((ModbusSpecifics*) pSerializer->pSpecifics)->request.slaveId != pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset]) {
            return SERIAL_STATUS_RESPONSE_BAD_SLAVE;
        }

        if ((((ModbusSpecifics*) pSerializer->pSpecifics)->request.function != pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset + 1])
                && (((ModbusSpecifics*) pSerializer->pSpecifics)->request.function != ((pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset + 1])
                        ^ MODBUS_MASK_ERROR))) {
            return SERIAL_STATUS_RESPONSE_BAD_FUNCTION;
        }

        return SERIAL_STATUS_RESPONSE_INVALID_FRAME;
    }
}

uint16_t RTUComputeCRC(uint8_t* pFrame, uint16_t length) {
    uint8_t ucCRCHi = 0xFF;
    uint8_t ucCRCLo = 0xFF;
    int16_t iIndex;

    while (length--) {
        iIndex = ucCRCLo ^ *(pFrame++);
        ucCRCLo = (uint8_t)(ucCRCHi ^ cRCHi[iIndex]);
        ucCRCHi = cRCLo[iIndex];
    }
    return (uint16_t)(ucCRCHi << 8 | ucCRCLo);
}

uint8_t RTUValidateCRC(uint8_t* pBuffer, uint16_t bufferSize, uint16_t cRC) {
    uint16_t tempCRC = RTUComputeCRC(pBuffer, bufferSize);
    return (tempCRC == cRC);
}

SerialStatus RTUCreateRequest(Serializer* pSerializer) {
    uint16_t cRC;

    if (pSerializer == NULL) {
        return SERIAL_STATUS_STACK_NOT_READY;
    }

    // add CRC
    cRC = RTUComputeCRC((uint8_t*) pSerializer->pRequestBuffer, pSerializer->requestBufferLength);

    // the low-order byte is appended first
    pSerializer->pRequestBuffer[pSerializer->requestBufferLength] = (uint8_t)(cRC & 0xFF);
    pSerializer->pRequestBuffer[pSerializer->requestBufferLength + 1] = (uint8_t)((cRC >> 8) & 0xFF);

    pSerializer->requestBufferLength = pSerializer->requestBufferLength + 2;

    return SERIAL_STATUS_OK;
}

SerialStatus RTUParseResponse(Serializer* pSerializer) {
    uint16_t cRC;
    uint8_t cRCCheck;

    if (pSerializer == NULL) {
        return SERIAL_STATUS_STACK_NOT_READY;
    }

    // process frame
    if ((pSerializer->responseBufferLength >= 4) && (pSerializer->responseBufferLength <= MODBUS_RTU_MAX_FRAME_SIZE) && (((ModbusSpecifics*) pSerializer->pSpecifics)->request.slaveId
            == pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset]) && ((((ModbusSpecifics*) pSerializer->pSpecifics)->request.function
            == pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset + 1]) || (((ModbusSpecifics*) pSerializer->pSpecifics)->request.function
            == ((pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset + 1]) ^ MODBUS_MASK_ERROR)))) {

        // retrieve the checksum
        cRC = ((uint16_t)(pSerializer->pResponseBuffer[pSerializer->responseBufferLength - 1])) << 8;
        cRC |= (uint16_t)(pSerializer->pResponseBuffer[pSerializer->responseBufferLength - 2]);

        // verify CRC
        cRCCheck = RTUValidateCRC(pSerializer->pResponseBuffer, pSerializer->responseBufferLength - 2, cRC);

        if (!cRCCheck) {
            return SERIAL_STATUS_RESPONSE_BAD_CHECKSUM;
        } else {
            return SERIAL_STATUS_OK;
        }
    } else {
        if (pSerializer->responseBufferLength < 4) {
            return SERIAL_STATUS_RESPONSE_SHORT_FRAME;
        }

        if (((ModbusSpecifics*) pSerializer->pSpecifics)->request.slaveId != pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset]) {
            return SERIAL_STATUS_RESPONSE_BAD_SLAVE;
        }

        if ((((ModbusSpecifics*) pSerializer->pSpecifics)->request.function != pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset + 1])
                && (((ModbusSpecifics*) pSerializer->pSpecifics)->request.function != ((pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset + 1])
                        ^ MODBUS_MASK_ERROR))) {
            return SERIAL_STATUS_RESPONSE_BAD_FUNCTION;
        }

        return SERIAL_STATUS_RESPONSE_INVALID_FRAME;
    }
}

uint8_t ASCIIComputeLRC(uint8_t* pFrame, uint16_t length) {
    // on a RTU PDU
    uint8_t ucLRC = 0; /* LRC char initialized */

    while (length--) {
        ucLRC += *pFrame++; /* add buffer byte without carry */
    }

    /* return twos complement */
    ucLRC = (uint8_t)(-((signed char) ucLRC));
    return ucLRC;
}

uint8_t ASCIIValidateLRC(uint8_t* pBuffer, uint16_t bufferSize, uint8_t lCR) {
    uint8_t tempLCR = ASCIIComputeLRC(pBuffer, bufferSize);
    return (tempLCR == lCR);
}

uint8_t HexDigit2Dec(char hexDigit) {
    uint8_t dec = 0;

    if ((hexDigit >= 'A') && (hexDigit <= 'F'))
        dec = hexDigit - 'A' + 10;

    else if ((hexDigit >= 'a') && (hexDigit <= 'f'))
        dec = hexDigit - 'a' + 10;

    else if ((hexDigit >= '0') && (hexDigit <= '9'))
        dec = hexDigit - '0';

    return dec;
}

SerialStatus ASCIICreateRequest(Serializer* pSerializer) {
    uint16_t lCR;
    char pHexToAscii[3];
    uint16_t index;
    uint16_t length;

    if (pSerializer == NULL) {
        return SERIAL_STATUS_STACK_NOT_READY;
    }

    // compute LCR
    lCR = ASCIIComputeLRC((uint8_t*) pSerializer->pRequestBuffer, pSerializer->requestBufferLength);

    // The high-order byte is appended first
    pSerializer->pRequestBuffer[pSerializer->requestBufferLength] = lCR;

    pSerializer->requestBufferLength = pSerializer->requestBufferLength + 1;

    // copy request buffer in pTempBuffer
    memcpy(pSerializer->pTempBuffer, pSerializer->pRequestBuffer, pSerializer->requestBufferLength);
    length = pSerializer->requestBufferLength;
    // create char buffer
    pSerializer->pRequestBuffer[0] = MODBUS_ASCII_START_CHAR;
    pSerializer->requestBufferLength = 1;
    for (index = 0; index < length; index++) {
        sprintf(pHexToAscii, "%02X", pSerializer->pTempBuffer[index]);
        pSerializer->pRequestBuffer[pSerializer->requestBufferLength] = pHexToAscii[0];
        pSerializer->pRequestBuffer[pSerializer->requestBufferLength + 1] = pHexToAscii[1];
        pSerializer->requestBufferLength += 2;
    }
    pSerializer->pRequestBuffer[pSerializer->requestBufferLength] = MODBUS_ASCII_END_CHAR1;
    pSerializer->pRequestBuffer[pSerializer->requestBufferLength + 1] = MODBUS_ASCII_END_CHAR2;

    pSerializer->requestBufferLength += 2;

    return SERIAL_STATUS_OK;
}

SerialStatus ASCIIParseResponse(Serializer* pSerializer) {
    char* pStartSequence = NULL;
    char* pEndSequence = NULL;
    uint16_t messageSize;
    uint16_t index;
    uint8_t lRCCheck;

    if (pSerializer == NULL) {
        return SERIAL_STATUS_STACK_NOT_READY;
    }

    // start sequence detected
    pSerializer->pResponseBuffer[(pSerializer->responseBufferLength < pSerializer->maxSize) ? pSerializer->responseBufferLength : (pSerializer->maxSize - 1)] = 0;
    pStartSequence = strstr((char*) pSerializer->pResponseBuffer, MODBUS_ASCII_START_SEQUENCE);
    if (pStartSequence != NULL) {
        pEndSequence = strstr((char*) pStartSequence, MODBUS_ASCII_END_SEQUENCE);
    }
    if ((pStartSequence != NULL) && (pEndSequence != NULL) && (pStartSequence < pEndSequence)) {
        // retrieved the char data
        messageSize = pEndSequence - (pStartSequence + 1);

        // copy to temp buffer
        memcpy(pSerializer->pTempBuffer, pStartSequence + 1, messageSize);

        pSerializer->responseBufferLength = 0;
        for (index = 0; index < messageSize - 1; index = index + 2) {
            pSerializer->pResponseBuffer[pSerializer->responseBufferLength] = 16 * HexDigit2Dec(pSerializer->pTempBuffer[index]) + HexDigit2Dec(pSerializer->pTempBuffer[index + 1]);
            pSerializer->responseBufferLength++;
        }

        // verify checksum
        lRCCheck = ASCIIValidateLRC(pSerializer->pResponseBuffer, pSerializer->responseBufferLength - 1, pSerializer->pResponseBuffer[pSerializer->responseBufferLength - 1]);

        if (!lRCCheck) {
            return SERIAL_STATUS_RESPONSE_BAD_CHECKSUM;
        } else {
            // response has been parsed verify is response is related to request
            if ((pSerializer->responseBufferLength >= 4) && (pSerializer->responseBufferLength <= MODBUS_RTU_MAX_FRAME_SIZE) && (((ModbusSpecifics*) pSerializer->pSpecifics)->request.slaveId
                    == pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset]) && ((((ModbusSpecifics*) pSerializer->pSpecifics)->request.function
                    == pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset + 1]) || (((ModbusSpecifics*) pSerializer->pSpecifics)->request.function
                    == ((pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset + 1]) ^ MODBUS_MASK_ERROR)))) {

                return SERIAL_STATUS_OK;
            } else {
                if (pSerializer->responseBufferLength < 4) {
                    return SERIAL_STATUS_RESPONSE_SHORT_FRAME;
                }

                if (((ModbusSpecifics*) pSerializer->pSpecifics)->request.slaveId != pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset]) {
                    return SERIAL_STATUS_RESPONSE_BAD_SLAVE;
                }

                if ((((ModbusSpecifics*) pSerializer->pSpecifics)->request.function != pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset + 1])
                        && (((ModbusSpecifics*) pSerializer->pSpecifics)->request.function != ((pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset + 1])
                                ^ MODBUS_MASK_ERROR))) {
                    return SERIAL_STATUS_RESPONSE_BAD_FUNCTION;
                }

                return SERIAL_STATUS_RESPONSE_INVALID_FRAME;
            }
        }
    }

    return SERIAL_STATUS_RESPONSE_INCOMPLETE_FRAME;
}

// serializer
SerialStatus MODBUS_SER_InitSerializer(Serializer* pSerializer, /*ModbusRequestMode*/void* mode) {
    if (mode == NULL) {
        return SERIAL_STATUS_WRONG_PARAMS;
    }
    pSerializer->pSpecifics = malloc(sizeof(ModbusSpecifics));
    if (pSerializer->pSpecifics == NULL) {
        return SERIAL_STATUS_ALLOC_FAILED;
    }
    ModbusSpecifics* pSpecifics = (ModbusSpecifics*) pSerializer->pSpecifics;

    // buffers and states variables
    pSerializer->requestBufferLength = 0;
    pSerializer->responseBufferLength = 0;
    pSerializer->pRequestBuffer = NULL;
    pSerializer->pResponseBuffer = NULL;
    pSerializer->pTempBuffer = NULL;
    pSerializer->type = SRLFWK_SER_REQ_RSP_STRICT;

    pSpecifics->requestTrId = 0;
    switch ((ModbusRequestMode) mode) {
    case MODBUS_ASCII:
        pSerializer->maxSize = MODBUS_ASCII_MAX_FRAME_SIZE;
        pSpecifics->slaveAddrOffset = 0;
        pSpecifics->requestMode = MODBUS_ASCII;
        break;
    default:
    case MODBUS_RTU:
        pSerializer->maxSize = MODBUS_RTU_MAX_FRAME_SIZE;
        pSpecifics->slaveAddrOffset = 0;
        pSpecifics->requestMode = MODBUS_RTU;
        break;
    case MODBUS_TCP:
        pSerializer->maxSize = MODBUS_TCP_MAX_FRAME_SIZE;
        pSpecifics->slaveAddrOffset = 6;
        pSpecifics->requestMode = MODBUS_TCP;
        break;
    }
    pSpecifics->request.slaveId = 0;
    pSpecifics->request.function = MODBUS_MASK_ERROR;
    pSpecifics->request.byteCount = 0;
    pSpecifics->request.value.pValues = NULL;
    pSpecifics->response.slaveId = 0;
    pSpecifics->response.function = MODBUS_MASK_ERROR;
    pSpecifics->response.exception = MODBUS_NO_EXCEPTION;
    pSpecifics->response.byteCount = 0;
    pSpecifics->response.value.pValues = NULL;

    // initialize buffers
    if (pSerializer->pRequestBuffer == NULL && pSerializer->pResponseBuffer == NULL && pSerializer->pTempBuffer == NULL) {
        pSerializer->pRequestBuffer = malloc(sizeof(uint8_t) * pSerializer->maxSize);
        pSerializer->pResponseBuffer = malloc(sizeof(uint8_t) * pSerializer->maxSize);
        pSerializer->pTempBuffer = malloc(sizeof(uint8_t) * pSerializer->maxSize);
    } else {
        return SERIAL_STATUS_CORRUPTED_CONTEXT;
    }
    if (pSerializer->pRequestBuffer == NULL || pSerializer->pResponseBuffer == NULL || pSerializer->pTempBuffer == NULL) {
        return SERIAL_STATUS_ALLOC_FAILED;
    }

    pSerializer->releaseSerializer = MODBUS_SER_ReleaseSerializer;
    pSerializer->requestBuilder = MODBUS_SER_CreateRequest;
    pSerializer->isResponseComplete = MODBUS_SER_IsResponseComplete;
    pSerializer->responseChecker = MODBUS_SER_CheckResponse;
    pSerializer->responseAnalyzer = MODBUS_SER_AnalyzeResponse;

    return SERIAL_STATUS_OK;
}

void MODBUS_SER_ReleaseSerializer(Serializer* pSerializer) {
    // release specifics
    if (pSerializer->pSpecifics != NULL) {
        free(pSerializer->pSpecifics);
        pSerializer->pSpecifics = NULL;
    }

    // release request buffer
    if (pSerializer->pRequestBuffer != NULL) {
        free(pSerializer->pRequestBuffer);
        pSerializer->pRequestBuffer = NULL;
    }

    // release response buffer
    if (pSerializer->pResponseBuffer != NULL) {
        free(pSerializer->pResponseBuffer);
        pSerializer->pResponseBuffer = NULL;
    }

    // release temp buffer
    if (pSerializer->pTempBuffer != NULL) {
        free(pSerializer->pTempBuffer);
        pSerializer->pTempBuffer = NULL;
    }
}

SerialStatus CreateRequest(Serializer* pSerializer) {
    switch (((ModbusSpecifics*) pSerializer->pSpecifics)->requestMode) {
    default:
    case MODBUS_RTU:
        return RTUCreateRequest(pSerializer);

    case MODBUS_ASCII:
        return ASCIICreateRequest(pSerializer);

    case MODBUS_TCP:
        return TCPCreateRequest(pSerializer);
    }
}

SerialStatus MODBUS_SER_GetRequestPDU(Serializer* pSerializer, uint8_t** ppBuffer, uint16_t* pBufferLength) {
    if (pSerializer->pRequestBuffer == NULL) {
        return SERIAL_STATUS_STACK_NOT_READY;
    }

    *pBufferLength = pSerializer->requestBufferLength;
    *ppBuffer = pSerializer->pRequestBuffer;

    return SERIAL_STATUS_OK;
}

SerialStatus MODBUS_SER_GetResponsePDU(Serializer* pSerializer, uint8_t** ppBuffer, uint16_t* pBufferLength) {
    if (pSerializer->pResponseBuffer == NULL) {
        return SERIAL_STATUS_STACK_NOT_READY;
    }

    *pBufferLength = pSerializer->responseBufferLength;
    *ppBuffer = pSerializer->pResponseBuffer;

    return SERIAL_STATUS_OK;
}

uint16_t MODBUS_SER_GetExpectedResponseLength(Serializer* pSerializer) {
    if (pSerializer == NULL) {
        return SERIAL_STATUS_STACK_NOT_READY;
    }

    if(((ModbusSpecifics*) pSerializer->pSpecifics)->isCustom) {
        return pSerializer->maxSize;
    }

    if (((ModbusSpecifics*) pSerializer->pSpecifics)->requestMode == MODBUS_RTU) {
        switch (((ModbusSpecifics*) pSerializer->pSpecifics)->request.function) {
        case MODBUS_FUNC_READ_COILS:
        case MODBUS_FUNC_READ_DISCRETE_INPUTS:
        case MODBUS_FUNC_READ_HOLDING_REGISTERS:
        case MODBUS_FUNC_READ_INPUT_REGISTERS:
            /* slaveId + function + byteCount + values + crc */
            return 1 + 1 + 1 + ((ModbusSpecifics*) pSerializer->pSpecifics)->request.byteCount + 2;

        case MODBUS_FUNC_WRITE_SINGLE_COIL:
        case MODBUS_FUNC_WRITE_SINGLE_REGISTER:
        case MODBUS_FUNC_WRITE_MULTIPLE_COILS:
        case MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS:
            /* slaveId + function + outputadd + outputvalue + crc */
            return 1 + 1 + 2 + 2 + 2;

        default:
            return MODBUS_RTU_MAX_FRAME_SIZE;
        }
    } else if (((ModbusSpecifics*) pSerializer->pSpecifics)->requestMode == MODBUS_ASCII) {
        switch (((ModbusSpecifics*) pSerializer->pSpecifics)->request.function) {
        case MODBUS_FUNC_READ_COILS:
        case MODBUS_FUNC_READ_DISCRETE_INPUTS:
        case MODBUS_FUNC_READ_HOLDING_REGISTERS:
        case MODBUS_FUNC_READ_INPUT_REGISTERS:
            /* start + slaveId + function + byteCount + values + crc + end*/
            return 1 + 2 + 2 + 2 + 2 * ((ModbusSpecifics*) pSerializer->pSpecifics)->request.byteCount + 2 + 2;

        case MODBUS_FUNC_WRITE_SINGLE_COIL:
        case MODBUS_FUNC_WRITE_SINGLE_REGISTER:
        case MODBUS_FUNC_WRITE_MULTIPLE_COILS:
        case MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS:
            /* start + slaveId + function + outputadd + outputvalue + crc + end */
            return 1 + 2 + 2 + 4 + 4 + 2 + 2;

        default:
            return MODBUS_ASCII_MAX_FRAME_SIZE;
        }
    } else if (((ModbusSpecifics*) pSerializer->pSpecifics)->requestMode == MODBUS_TCP) {
        switch (((ModbusSpecifics*) pSerializer->pSpecifics)->request.function) {
        case MODBUS_FUNC_READ_COILS:
        case MODBUS_FUNC_READ_DISCRETE_INPUTS:
        case MODBUS_FUNC_READ_HOLDING_REGISTERS:
        case MODBUS_FUNC_READ_INPUT_REGISTERS:
            /* MBAP + function + byteCount + values*/
            return 7 + 1 + 1 + ((ModbusSpecifics*) pSerializer->pSpecifics)->request.byteCount;

        case MODBUS_FUNC_WRITE_SINGLE_COIL:
        case MODBUS_FUNC_WRITE_SINGLE_REGISTER:
        case MODBUS_FUNC_WRITE_MULTIPLE_COILS:
        case MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS:
            /* MBAP + function + outputadd + outputvalue */
            return 7 + 1 + 2 + 2;

        case MODBUS_FUNC_SEND_RAW_DATA:
        default:
            return MODBUS_TCP_MAX_FRAME_SIZE;
        }
    } else {
        return MODBUS_RTU_MAX_FRAME_SIZE;
    }
}

SerialStatus MODBUS_SER_CreateCustomRequest(Serializer* pSerializer, /*ModbusRequest*/void* pRequestData) {
    ModbusSpecifics* pSpecifics = (ModbusSpecifics*) pSerializer->pSpecifics;
    ModbusRequest* pRequest = (ModbusRequest*) pRequestData;
    uint16_t index;

    if (pSerializer->pRequestBuffer == NULL) {
        return SERIAL_STATUS_STACK_NOT_READY;
    }

    if (pRequest->slaveId <= 0) {
        return SERIAL_STATUS_REQUEST_PARAMETER_ERROR;
    }

    pSpecifics->isCustom = 1;
    pSpecifics->request.slaveId = pRequest->slaveId;
    pSpecifics->request.function = pRequest->function;
    pSpecifics->request.value.pValues = pRequest->value.pValues;
    pSpecifics->request.byteCount = pRequest->byteCount;

    pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset] = pSpecifics->request.slaveId;
    pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 1] = pSpecifics->request.function;

    for (index = 0; index < pSpecifics->request.byteCount; index++) {
        pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 2 + index] = (uint8_t)((((uint8_t*) pSpecifics->request.value.pValues)[index]) & 0xFF);
    }

    pSerializer->requestBufferLength = 2 + pSpecifics->request.byteCount;

    return CreateRequest(pSerializer);
}

// read/write request
SerialStatus MODBUS_SER_CreateRequest(Serializer* pSerializer, /*ModbusRequest*/void* pRequestData) {
    ModbusSpecifics* pSpecifics = (ModbusSpecifics*) pSerializer->pSpecifics;
    ModbusRequest* pRequest = (ModbusRequest*) pRequestData;
    uint16_t index;

    if (pSerializer->pRequestBuffer == NULL) {
        return SERIAL_STATUS_STACK_NOT_READY;
    }

    pSpecifics->isCustom = 0;
    pSpecifics->request.slaveId = pRequest->slaveId;
    pSpecifics->request.startingAddress = pRequest->startingAddress;
    pSpecifics->request.function = pRequest->function;
    pSpecifics->request.value.pValues = pRequest->value.pValues;
    pSpecifics->request.numberOfObjects = pRequest->numberOfObjects;

    if (pSpecifics->request.slaveId <= 0) {
        return SERIAL_STATUS_REQUEST_PARAMETER_ERROR;
    }

    switch (pSpecifics->request.function) {
    case MODBUS_FUNC_READ_DISCRETE_INPUTS:
    case MODBUS_FUNC_READ_COILS:
        if ((pSpecifics->request.value.pValues != NULL) && (pSpecifics->request.numberOfObjects > 0) && (pSpecifics->request.numberOfObjects <= MODBUS_MAX_READ_COILS)) {
            pSpecifics->request.byteCount = ((pRequest->numberOfObjects >> 3) + (((pRequest->numberOfObjects % 8) != 0) ? 1 : 0));

            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset] = pSpecifics->request.slaveId;
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 1] = pSpecifics->request.function;
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 2] = (uint8_t)((pSpecifics->request.startingAddress >> 8) & 0xFF);
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 3] = (uint8_t)(pSpecifics->request.startingAddress & 0xFF);
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 4] = (uint8_t)((pSpecifics->request.numberOfObjects >> 8) & 0xFF);
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 5] = (uint8_t)(pSpecifics->request.numberOfObjects & 0xFF);

            pSerializer->requestBufferLength = 6;

            return CreateRequest(pSerializer);
        } else {
            return SERIAL_STATUS_REQUEST_PARAMETER_ERROR;
        }

    case MODBUS_FUNC_READ_INPUT_REGISTERS:
    case MODBUS_FUNC_READ_HOLDING_REGISTERS:
        if ((pSpecifics->request.value.pValues != NULL) && (pSpecifics->request.numberOfObjects > 0) && (pSpecifics->request.numberOfObjects <= MODBUS_MAX_READ_REGISTERS)) {
            pSpecifics->request.byteCount = pRequest->numberOfObjects << 1;

            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset] = pSpecifics->request.slaveId;
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 1] = pSpecifics->request.function;
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 2] = (uint8_t)((pSpecifics->request.startingAddress >> 8) & 0xFF);
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 3] = (uint8_t)(pSpecifics->request.startingAddress & 0xFF);
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 4] = (uint8_t)((pSpecifics->request.numberOfObjects >> 8) & 0xFF);
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 5] = (uint8_t)(pSpecifics->request.numberOfObjects & 0xFF);

            pSerializer->requestBufferLength = 6;

            return CreateRequest(pSerializer);
        } else {
            return SERIAL_STATUS_REQUEST_PARAMETER_ERROR;
        }

    case MODBUS_FUNC_WRITE_SINGLE_REGISTER:
    case MODBUS_FUNC_WRITE_SINGLE_COIL:
        pSpecifics->request.byteCount = pRequest->byteCount;

        pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset] = pSpecifics->request.slaveId;
        pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 1] = pSpecifics->request.function;
        pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 2] = (uint8_t)((pSpecifics->request.startingAddress >> 8) & 0xFF);
        pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 3] = (uint8_t)(pSpecifics->request.startingAddress & 0xFF);
        pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 4] = (uint8_t)((pSpecifics->request.value.iValue >> 8) & 0xFF);
        pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 5] = (uint8_t)(pSpecifics->request.value.iValue & 0xFF);

        pSerializer->requestBufferLength = 6;

        return CreateRequest(pSerializer);

    case MODBUS_FUNC_WRITE_MULTIPLE_COILS:
        if ((pSpecifics->request.numberOfObjects > 0) && (pSpecifics->request.numberOfObjects <= MODBUS_MAX_NUMBER_OF_WRITTEN_COILS)) {
            pSpecifics->request.byteCount = pRequest->byteCount;

            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset] = pSpecifics->request.slaveId;
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 1] = pSpecifics->request.function;
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 2] = (uint8_t)((pSpecifics->request.startingAddress >> 8) & 0xFF);
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 3] = (uint8_t)(pSpecifics->request.startingAddress & 0xFF);
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 4] = (uint8_t)((pSpecifics->request.numberOfObjects >> 8) & 0xFF);
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 5] = (uint8_t)(pSpecifics->request.numberOfObjects & 0xFF);
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 6] = pSpecifics->request.byteCount;

            if ((pSpecifics->slaveAddrOffset + 7 + pRequest->byteCount - 1) >= pSerializer->maxSize) {
                return SERIAL_STATUS_REQUEST_PARAMETER_ERROR;
            }

            for (index = 0; index < pSpecifics->request.byteCount; index++) {
                pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 7 + index] = ((uint8_t*) pSpecifics->request.value.pValues)[index];
            }

            pSerializer->requestBufferLength = 7 + pRequest->byteCount;

            return CreateRequest(pSerializer);
        } else {
            return SERIAL_STATUS_REQUEST_PARAMETER_ERROR;
        }
        break;

    case MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS:
        if ((pSpecifics->request.numberOfObjects > 0) && (pSpecifics->request.numberOfObjects <= MODBUS_MAX_NUMBER_OF_WRITTEN_REGISTERS)) {
            pSpecifics->request.byteCount = pRequest->numberOfObjects << 1;

            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset] = pSpecifics->request.slaveId;
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 1] = pSpecifics->request.function;
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 2] = (uint8_t)((pSpecifics->request.startingAddress >> 8) & 0xFF);
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 3] = (uint8_t)(pSpecifics->request.startingAddress & 0xFF);
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 4] = 0;
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 5] = pSpecifics->request.numberOfObjects & 0xFF;
            pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 6] = pSpecifics->request.byteCount;

            if ((pSpecifics->slaveAddrOffset + 7 + (pSpecifics->request.numberOfObjects - 1) * 2 + 1) >= pSerializer->maxSize) {
                return SERIAL_STATUS_REQUEST_PARAMETER_ERROR;
            }

            for (index = 0; index < pSpecifics->request.numberOfObjects; index++) {
                uint16_t val = ((uint16_t*) pSpecifics->request.value.pValues)[index];
                pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 7 + index * 2 + 0] = (uint8_t)((val >> 8) & 0xFF);
                pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 7 + index * 2 + 1] = (uint8_t)(val & 0xFF);
            }

            pSerializer->requestBufferLength = 7 + pRequest->numberOfObjects * 2;

            return CreateRequest(pSerializer);
        } else {
            return SERIAL_STATUS_REQUEST_PARAMETER_ERROR;
        }
        break;

    case MODBUS_FUNC_SEND_RAW_DATA:
        if((pRequest->byteCount > 0) && (pRequest->byteCount <= MODBUS_TCP_MAX_DATA_SIZE))
        {
          pSpecifics->request.byteCount = pRequest->byteCount;

          pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset] = pSpecifics->request.slaveId;
          pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 1] = pSpecifics->request.function;

          for (index = 0; index < pSpecifics->request.byteCount; index++) {
              pSerializer->pRequestBuffer[pSpecifics->slaveAddrOffset + 2 + index] = (uint8_t)((((uint8_t*) pSpecifics->request.value.pValues)[index]) & 0xFF);
          }

          pSerializer->requestBufferLength = 2 + pSpecifics->request.byteCount;

          return CreateRequest(pSerializer);
        } else {
            return SERIAL_STATUS_REQUEST_PARAMETER_ERROR;
        }

    default:
        return SERIAL_STATUS_REQUEST_PARAMETER_ERROR;
    }
}

ModbusFunctionCode ParseFunction(Serializer* pSerializer) {
    ModbusFunctionCode function = pSerializer->pResponseBuffer[((ModbusSpecifics*) pSerializer->pSpecifics)->slaveAddrOffset + 1];
    if ((function & MODBUS_MASK_ERROR) != MODBUS_MASK_ERROR) {
        return (ModbusFunctionCode) function;
    }

    return MODBUS_MASK_ERROR;
}

uint8_t MODBUS_SER_IsResponseComplete(Serializer* pSerializer) {
    char* pStartSequence = NULL;
    char* pEndSequence = NULL;
    switch (((ModbusSpecifics*) pSerializer->pSpecifics)->requestMode) {
    default:
    case MODBUS_RTU: /* WARNING at low baudrates < 9600 */
        return 1;

    case MODBUS_ASCII:
        pSerializer->pResponseBuffer[(pSerializer->responseBufferLength < pSerializer->maxSize) ? pSerializer->responseBufferLength : (pSerializer->maxSize - 1)] = 0;
        pStartSequence = strstr((char*) pSerializer->pResponseBuffer, MODBUS_ASCII_START_SEQUENCE);
        if (pStartSequence != NULL) {
            pEndSequence = strstr((char*) pStartSequence, MODBUS_ASCII_END_SEQUENCE);
        } else {
            // start has not been found clear reception buffer
            pSerializer->responseBufferLength = 0;
            return 0;
        }
        if ((pStartSequence != NULL) && (pEndSequence != NULL) && (pStartSequence < pEndSequence)) {
            return 1;
        }
        return 0;
    }
}

SerialStatus MODBUS_SER_CheckResponse(Serializer* pSerializer) {
    switch (((ModbusSpecifics*) pSerializer->pSpecifics)->requestMode) {
    default:
    case MODBUS_RTU:
        return RTUParseResponse(pSerializer);

    case MODBUS_ASCII:
        return ASCIIParseResponse(pSerializer);

    case MODBUS_TCP:
        return TCPParseResponse(pSerializer);
    }
}

SerialStatus MODBUS_SER_AnalyzeResponse(Serializer* pSerializer, SerialStatus status) {
    // response has been parsed
    ModbusSpecifics* pSpecifics = (ModbusSpecifics*) pSerializer->pSpecifics;
    if (status == SERIAL_STATUS_OK) {
        // get function code
        pSpecifics->response.function = ParseFunction(pSerializer);

        if (pSpecifics->isCustom && (pSpecifics->response.function != MODBUS_MASK_ERROR)) {
            status = ParseRawData(pSerializer);
        } else {
            // process status
            switch (pSpecifics->response.function) {
            case MODBUS_FUNC_READ_COILS:
            case MODBUS_FUNC_READ_DISCRETE_INPUTS:
                status = ParseReadInputs(pSerializer);
                break;

            case MODBUS_FUNC_READ_HOLDING_REGISTERS:
            case MODBUS_FUNC_READ_INPUT_REGISTERS:
                status = ParseReadRegisters(pSerializer);
                break;

            case MODBUS_FUNC_WRITE_SINGLE_COIL:
            case MODBUS_FUNC_WRITE_SINGLE_REGISTER:
            case MODBUS_FUNC_WRITE_MULTIPLE_COILS:
            case MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS:
                status = ParseWriteObject(pSerializer);
                break;

            case MODBUS_FUNC_SEND_RAW_DATA:
                status = ParseRawData(pSerializer);
                break;

            case MODBUS_MASK_ERROR:
                status = SERIAL_STATUS_RESPONSE_EXCEPTION;
                pSpecifics->response.function = (ModbusFunctionCode)((pSpecifics->response.function) ^ MODBUS_MASK_ERROR);
                pSpecifics->response.exception = (ModbusExceptionCode) pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + 2];
                break;

            default:
                break;
            }
        }
    } else if (status == SERIAL_STATUS_RESPONSE_TIMEOUT) {
        pSpecifics->response.function = (ModbusFunctionCode) pSpecifics->request.function;
    }

    // dereference the context object buffer
    pSpecifics->request.value.pValues = NULL;
    return status;
}

SerialStatus ParseReadInputs(Serializer* pSerializer) {
    uint8_t index;
    ModbusSpecifics* pSpecifics = (ModbusSpecifics*) pSerializer->pSpecifics;
    // get address
    pSpecifics->response.startingAddress = ((pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + 2]) << 8) + (pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + 3]);

    // get byte count
    pSpecifics->response.byteCount = pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + 2];
    if (pSpecifics->request.byteCount != pSpecifics->response.byteCount) {
        return SERIAL_STATUS_UNEXPECTED_ERROR;
    }

    // fill tab
    pSpecifics->response.value.pValues = pSpecifics->request.value.pValues;

    for (index = 0; index < pSpecifics->response.byteCount; index++) {
        ((uint8_t*) pSpecifics->response.value.pValues)[index] = pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + 3 + index];
    }

    return SERIAL_STATUS_OK;
}

SerialStatus ParseReadRegisters(Serializer* pSerializer) {
    uint8_t index;
    ModbusSpecifics* pSpecifics = (ModbusSpecifics*) pSerializer->pSpecifics;
    // get address
    pSpecifics->response.startingAddress = ((pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + 2]) << 8) + (pSerializer-> pResponseBuffer[pSpecifics->slaveAddrOffset + 3]);

    // get byte count
    pSpecifics->response.byteCount = pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + 2];
    if (pSpecifics->request.byteCount != pSpecifics->response.byteCount) {
        return SERIAL_STATUS_UNEXPECTED_ERROR;
    }
    pSpecifics->response.numberOfObjects = (pSpecifics->response.byteCount >> 1);

    // fill tab
    pSpecifics->response.value.pValues = pSpecifics->request.value.pValues;

    for (index = 0; index < pSpecifics->response.numberOfObjects; index++) {
        ((uint16_t*) pSpecifics->response.value.pValues)[index] = ((pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + 3 + (index * 2 + 0)]) << 8)
                + (pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + 3 + (index * 2 + 1)]);
    }

    return SERIAL_STATUS_OK;
}

SerialStatus ParseWriteObject(Serializer* pSerializer) {
    ModbusSpecifics* pSpecifics = (ModbusSpecifics*) pSerializer->pSpecifics;
    // get address
    pSpecifics->response.startingAddress = ((pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + 2]) << 8) + (pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + 3]);

    // get number or value
    pSpecifics->response.numberOfObjects = ((pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + 4]) << 8) + (pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + 5]);

    pSpecifics->response.byteCount = 0;
    pSpecifics->response.value.pValues = NULL;
    return SERIAL_STATUS_OK;
}

SerialStatus ParseRawData(Serializer* pSerializer) {
    uint8_t index;

    ModbusSpecifics* pSpecifics = (ModbusSpecifics*) pSerializer->pSpecifics;
    // get address
    pSpecifics->response.startingAddress = 0;

    // get byte count
    pSpecifics->response.byteCount = pSerializer->responseBufferLength - pSpecifics->slaveAddrOffset;

    pSpecifics->response.numberOfObjects = (pSpecifics->response.byteCount >> 1);

    // fill tab
    pSpecifics->response.value.pValues = pSpecifics->request.value.pValues;

    for (index = 0; index < pSpecifics->response.byteCount; index++) {
        ((uint8_t*) pSpecifics->response.value.pValues)[index] = (pSerializer->pResponseBuffer[pSpecifics->slaveAddrOffset + index]);
    }

    return SERIAL_STATUS_OK;
}

const char* MODBUS_SER_GetExceptionString(ModbusExceptionCode exception) {
    switch (exception) {
    case MODBUS_NO_EXCEPTION:
        return exceptionMessages[0];

    case MODBUS_ILLEGAL_FUNCTION:
        return exceptionMessages[1];

    case MODBUS_ILLEGAL_DATA_ADDRESS:
        return exceptionMessages[2];

    case MODBUS_ILLEGAL_DATA_VALUE:
        return exceptionMessages[3];

    case MODBUS_SLAVE_DEVICE_FAILURE:
        return exceptionMessages[4];

    case MODBUS_ACKNOWLEDGE:
        return exceptionMessages[5];

    case MODBUS_SLAVE_DEVICE_BUSY:
        return exceptionMessages[6];

    case MODBUS_MEMORY_PARITY_ERROR:
        return exceptionMessages[7];

    case MODBUS_GATEWAY_PATH_UNAVAILABLE:
        return exceptionMessages[8];

    case MODBUS_TARGET_DEVICE_FAILED_TO_RESPOND:
        return exceptionMessages[9];

    default:
        return exceptionMessages[10];
    }
}
