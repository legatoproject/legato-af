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
#include "serial_oat_adapter.h"

#include "adl_global.h"

#define PERSISTENT_COUNTER "testModbusCounter"

const u16 wm_apmCustomStackSize = 3 * 1024;
static ascii ltsc_Temp[128] = { 0 };
static int choice = 0;
typedef struct ModbusUserData_ {
    SerialContext* pSerialContext; // context
    ModbusRequest request; // request
    void* allocated;
} ModbusUserData;

static ModbusUserData modbusData;

static void MAIN_StartAppTimerHandler(u8 id, void* pContext);
static void MAIN_PollingTimerHandler(u8 id, void* pContext);
static void ModbusHandler(SerialContext * pModbusContext, SerialStatus status, void* pUserData);
static void UART_Init(void);
static s16 UART_DummyAtCmdRespHandler(adl_atResponse_t* apu_Rsp);

s16 UART_DummyAtCmdRespHandler(adl_atResponse_t* apu_Rsp) {
    wm_sprintf(ltsc_Temp, "\r\nUART2 closed");
    adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
    return TRUE;
}

void UART_Init(void) {
    // release COM2 as it will be used by the modbus stack
    adl_atCmdCreate("AT+WMFM=0,0,2", FALSE, (adl_atRspHandler_t) UART_DummyAtCmdRespHandler, "*", NULL);
}

void ModbusHandler(SerialContext* pSerialContext, SerialStatus status, void* pUserData) {
    u8* pduBuff = NULL;
    u16 bufferLength = 0;
    int index;
    ModbusSpecifics* pSpecifics = SRLFWK_ADP_GetProtocolData(pSerialContext);
    ModbusUserData* pModbusData = (ModbusUserData*) pUserData;

    if (/*status != SERIAL_STATUS_RESPONSE_TIMEOUT*/1) {
        wm_sprintf(ltsc_Temp, "\r\n >>>>");
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        SRLFWK_ADP_GetRequestPDU(pSerialContext, &pduBuff, &bufferLength);
        for (index = 0; index < bufferLength; index++) {
            wm_sprintf(ltsc_Temp, " %02X ", pduBuff[index]);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        }

        bufferLength = 0;
        pduBuff = NULL;
        wm_sprintf(ltsc_Temp, "\r\n <<<<");
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        if (status != SERIAL_STATUS_RESPONSE_TIMEOUT) {
            SRLFWK_ADP_GetResponsePDU(pSerialContext, &pduBuff, &bufferLength);
            for (index = 0; index < bufferLength; index++) {
                wm_sprintf(ltsc_Temp, " %02X ", pduBuff[index]);
                adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            }
        }
    }

    switch (status) {
    case SERIAL_STATUS_OK:
        switch (pSpecifics->response.function) {
        case MODBUS_FUNC_READ_COILS:
            wm_sprintf(ltsc_Temp, "\r\n <<<< READ COILS");
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            wm_sprintf(ltsc_Temp, " ---> OK '%d' coil(s) read on '%d' bytes starting at '%d'", pSpecifics->response.numberOfObjects, pSpecifics->response.byteCount,
                    pSpecifics->response.startingAddress);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            for (index = 0; index < pSpecifics->response.byteCount; index++) {
                wm_sprintf(ltsc_Temp, "\r\n byte '%d' : %02X", index, ((u8*) (pSpecifics->response.value.pValues))[index]);
                adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            }
            break;

        case MODBUS_FUNC_READ_DISCRETE_INPUTS:
            wm_sprintf(ltsc_Temp, "\r\n <<<< READ DISCRETE INPUTS");
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            wm_sprintf(ltsc_Temp, " ---> OK '%d' discrete input(s) read on '%d' bytes starting at '%d'", pSpecifics->response.numberOfObjects, pSpecifics->response.byteCount,
                    pSpecifics->response.startingAddress);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            for (index = 0; index < pSpecifics->response.byteCount; index++) {
                wm_sprintf(ltsc_Temp, "\r\n byte '%d' : %02X", index, ((u8*) (pSpecifics->response.value.pValues))[index]);
                adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            }
            break;

        case MODBUS_FUNC_READ_HOLDING_REGISTERS:
            wm_sprintf(ltsc_Temp, "\r\n <<<< READ HOLDING REGISTERS");
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);

            wm_sprintf(ltsc_Temp, " ---> OK '%d' holding register(s) read", pSpecifics->response.numberOfObjects);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            for (index = 0; index < pSpecifics->response.numberOfObjects; index++) {
                wm_sprintf(ltsc_Temp, "\r\n value '%d' : %d", index, ((u16*) (pSpecifics->response.value.pValues))[index]);
                adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            }
            break;

        case MODBUS_FUNC_READ_INPUT_REGISTERS:
            wm_sprintf(ltsc_Temp, "\r\n <<<< READ INPUT REGISTERS");
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            wm_sprintf(ltsc_Temp, " ---> OK '%d' input register(s) read", pSpecifics->response.numberOfObjects);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            for (index = 0; index < pSpecifics->response.numberOfObjects; index++) {
                wm_sprintf(ltsc_Temp, "\r\n value '%d' : %d", index, ((u16*) (pSpecifics->response.value.pValues))[index]);
                adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            }
            break;

        case MODBUS_FUNC_WRITE_SINGLE_COIL:
            wm_sprintf(ltsc_Temp, "\r\n <<<< WRITE COILS");
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            wm_sprintf(ltsc_Temp, " ---> OK address: %d - value: %d", pSpecifics->response.startingAddress, (int) pSpecifics->response.value.pValues);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;

        case MODBUS_FUNC_WRITE_SINGLE_REGISTER:
            wm_sprintf(ltsc_Temp, "\r\n <<<< WRITE REGISTER");
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            wm_sprintf(ltsc_Temp, " ---> OK address: %d - value: %d", pSpecifics->response.startingAddress, (int) pSpecifics->response.value.pValues);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;

        case MODBUS_FUNC_WRITE_MULTIPLE_COILS:
            wm_sprintf(ltsc_Temp, "\r\n <<<< WRITE MULTIPLE COILS");
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            wm_sprintf(ltsc_Temp, " ---> OK address: %d - number: %d", pSpecifics->response.startingAddress, pSpecifics->response.numberOfObjects);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;

        case MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS:
            wm_sprintf(ltsc_Temp, "\r\n <<<< WRITE MULTIPLE REGISTERS");
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            wm_sprintf(ltsc_Temp, " ---> OK address: %d - number: %d", pSpecifics->response.startingAddress, pSpecifics->response.numberOfObjects);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;

        default:
            wm_sprintf(ltsc_Temp, "\r\n <<<< UNKNOWN FUNCTION");
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;
        }
        break;

    case SERIAL_STATUS_RESPONSE_TIMEOUT:
        wm_sprintf(ltsc_Temp, "\r\n <<<< ERROR TIMEOUT");
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        break;

    case SERIAL_STATUS_RESPONSE_EXCEPTION:
        switch (pSpecifics->response.function) {
        case MODBUS_FUNC_READ_COILS:
            wm_sprintf(ltsc_Temp, "\r\n <<<< ERROR READ COILS  - exception '%02X'", pSpecifics->response.exception);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;

        case MODBUS_FUNC_READ_DISCRETE_INPUTS:
            wm_sprintf(ltsc_Temp, "\r\n <<<< ERROR READ DISCRETE INPUTS  - exception '%02X'", pSpecifics->response.exception);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;

        case MODBUS_FUNC_READ_HOLDING_REGISTERS:
            wm_sprintf(ltsc_Temp, "\r\n <<<< ERROR READ HOLDING REGISTERS  - exception '%02X'", pSpecifics->response.exception);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;

        case MODBUS_FUNC_READ_INPUT_REGISTERS:
            wm_sprintf(ltsc_Temp, "\r\n <<<< ERROR READ INPUT REGISTERS  - exception '%02X'", pSpecifics->response.exception);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;

        case MODBUS_FUNC_WRITE_SINGLE_COIL:
            wm_sprintf(ltsc_Temp, "\r\n <<<< ERROR WRITE COILS  - exception '%02X'", pSpecifics->response.exception);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;

        case MODBUS_FUNC_WRITE_SINGLE_REGISTER:
            wm_sprintf(ltsc_Temp, "\r\n <<<< ERROR WRITE REGISTER  - exception '%02X'", pSpecifics->response.exception);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;

        case MODBUS_FUNC_WRITE_MULTIPLE_COILS:
            wm_sprintf(ltsc_Temp, "\r\n <<<< ERROR WRITE MULTIPLE COILS  - exception '%02X'", pSpecifics->response.exception);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;

        case MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS:
            wm_sprintf(ltsc_Temp, "\r\n <<<< ERROR WRITE MULTIPLE REGISTERS  - exception '%02X'", pSpecifics->response.exception);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;

        default:
            wm_sprintf(ltsc_Temp, "\r\n <<<< ERROR UNKNOWN FUNCTION  - exception '%02X'", pSpecifics->response.exception);
            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
            break;
        }
        break;

    case SERIAL_STATUS_RESPONSE_BAD_CHECKSUM:
        wm_sprintf(ltsc_Temp, "\r\n <<<< ERROR CHECKSUM BAD");
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        break;

    default:
    case SERIAL_STATUS_RESPONSE_INVALID_FRAME:
        wm_sprintf(ltsc_Temp, "\r\n <<<< ERROR INVALID FRAME");
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        break;

    }

    wm_sprintf(ltsc_Temp, "\r\n ------------------------------------------------------------------------------");
    adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);

    // free user result buffer
    if (pModbusData->allocated != NULL) {
        adl_memRelease(pModbusData->allocated);
        pModbusData->allocated = NULL;
    }
}

void MAIN_PollingTimerHandler(u8 id, void* pContext) {
    SERIAL_STATUS_t result = SERIAL_STATUS_UNEXPECTED_ERROR;
    ModbusUserData* pModbusData = (ModbusUserData*) pContext;
    pModbusData->request.slaveId = 1;

    int index = 0;
    u8 value = 0xFF;

    wm_sprintf(ltsc_Temp, "\r\n ------------------------------------------------------------------------------");
    adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);

    switch (choice) {
    case 0: // write single coil
        pModbusData->request.function = MODBUS_FUNC_WRITE_SINGLE_COIL;
        pModbusData->request.startingAddress = 0x0002;
        pModbusData->request.value.iValue = (uint32_t) MODBUS_SINGLE_COIL_ON;

        wm_sprintf(ltsc_Temp, "\r\n >>>> WRITE SINGLE COIL at '%04X' ON SLAVE '%d'", pModbusData->request.startingAddress, pModbusData->request.slaveId);
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        break;

    case 1: //read input registers
        pModbusData->request.function = MODBUS_FUNC_READ_INPUT_REGISTERS;
        pModbusData->request.startingAddress = 0x0000;
        pModbusData->request.numberOfObjects = 16;
        pModbusData->allocated = adl_memGet(pModbusData->request.numberOfObjects * sizeof(u16));
        pModbusData->request.value.pValues = pModbusData->allocated;

        wm_sprintf(ltsc_Temp, "\r\n >>>> READ '%d' INPUT REGISTERS starting at '%04X' ON SLAVE '%d'", pModbusData->request.numberOfObjects, pModbusData->request.startingAddress,
                pModbusData->request.slaveId);
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        break;

    case 2: // read coils
        pModbusData->request.function = MODBUS_FUNC_READ_COILS;
        pModbusData->request.startingAddress = 0x0000;
        pModbusData->request.numberOfObjects = 120;
        pModbusData->request.byteCount = ((pModbusData->request.numberOfObjects / 8) + (((pModbusData->request.numberOfObjects % 8) != 0) ? 1 : 0));
        pModbusData->allocated = adl_memGet(pModbusData->request.byteCount * sizeof(u8));
        pModbusData->request.value.pValues = pModbusData->allocated;

        wm_sprintf(ltsc_Temp, "\r\n >>>> READ '%d' COILS ('%d' bytes) starting at '%04X' ON SLAVE '%d'", pModbusData->request.numberOfObjects, pModbusData->request.byteCount,
                pModbusData->request.startingAddress, pModbusData->request.slaveId);
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        break;

    case 3: // read holding registers
        pModbusData->request.function = MODBUS_FUNC_READ_HOLDING_REGISTERS;
        pModbusData->request.startingAddress = 0x0000;
        pModbusData->request.numberOfObjects = 10;
        pModbusData->allocated = adl_memGet(pModbusData->request.numberOfObjects * sizeof(u16));
        pModbusData->request.value.pValues = pModbusData->allocated;

        wm_sprintf(ltsc_Temp, "\r\n >>>> READ '%d' HOLDING REGISTERS starting at '%04X' ON SLAVE '%d'", pModbusData->request.numberOfObjects, pModbusData->request.startingAddress,
                pModbusData->request.slaveId);
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        break;

    case 4: // write single register
        pModbusData->request.function = MODBUS_FUNC_WRITE_SINGLE_REGISTER;
        pModbusData->request.startingAddress = 0x0000;
        pModbusData->request.value.iValue = (uint32_t) 1;

        wm_sprintf(ltsc_Temp, "\r\n >>>> WRITE SINGLE REGISTER at '%04X' ON SLAVE '%d'", pModbusData->request.startingAddress, pModbusData->request.slaveId);
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        break;

    case 5: // write multiple coils
        pModbusData->request.function = MODBUS_FUNC_WRITE_MULTIPLE_COILS;
        pModbusData->request.startingAddress = 0x0000;
        pModbusData->request.numberOfObjects = 120;
        pModbusData->request.byteCount = ((pModbusData->request.numberOfObjects / 8) + (((pModbusData->request.numberOfObjects % 8) != 0) ? 1 : 0));
        pModbusData->allocated = adl_memGet(pModbusData->request.byteCount * sizeof(u8));
        pModbusData->request.value.pValues = pModbusData->allocated;
        for (index = 0; index < pModbusData->request.byteCount; index++) {
            value = value ^ 0xFF;
            ((u8*) pModbusData->request.value.pValues)[index] = value;
        }

        wm_sprintf(ltsc_Temp, "\r\n >>>> WRITE '%d' COILS starting at '%04X' ON SLAVE '%d'", pModbusData->request.numberOfObjects, pModbusData->request.startingAddress, pModbusData->request.slaveId);
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        break;

    case 6: // write multiple registers
        pModbusData->request.function = MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS;
        pModbusData->request.startingAddress = 0x0000;
        pModbusData->request.numberOfObjects = 120;
        pModbusData->allocated = adl_memGet(pModbusData->request.numberOfObjects * sizeof(u16));
        pModbusData->request.value.pValues = pModbusData->allocated;
        for (index = 0; index < pModbusData->request.numberOfObjects; index++) {
            ((u16*) pModbusData->request.value.pValues)[index] = 2 * index + 1;
        }

        wm_sprintf(ltsc_Temp, "\r\n >>>> WRITE MULTIPLE '%d' REGISTERS starting at '%04X' ON SLAVE '%d'", pModbusData->request.numberOfObjects, pModbusData->request.startingAddress,
                pModbusData->request.slaveId);
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        break;

    case 7: // read discrete input
        pModbusData->request.function = MODBUS_FUNC_READ_DISCRETE_INPUTS;
        pModbusData->request.startingAddress = 0x0000;
        pModbusData->request.numberOfObjects = 120;
        pModbusData->request.byteCount = ((pModbusData->request.numberOfObjects / 8) + (((pModbusData->request.numberOfObjects % 8) != 0) ? 1 : 0));
        pModbusData->allocated = adl_memGet(pModbusData->request.byteCount * sizeof(u8));
        pModbusData->request.value.pValues = pModbusData->allocated;

        wm_sprintf(ltsc_Temp, "\r\n >>>> READ '%d' DISCRETE INPUTS ('%d' bytes) starting at '%04X' ON SLAVE '%d'", pModbusData->request.numberOfObjects, pModbusData->request.byteCount,
                pModbusData->request.startingAddress, pModbusData->request.slaveId);
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        break;

    default:
        wm_sprintf(ltsc_Temp, "\r\n >>>> ???");
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        return;
    }

    result = SRLFWK_ADP_Request(pModbusData->pSerialContext, &pModbusData->request);
    if (result == SERIAL_STATUS_OK) {
        // print pModbusData->request
        wm_sprintf(ltsc_Temp, "\r\n >>>> polling DONE '%d'", result);
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
    } else {
        if (pModbusData->allocated != NULL) {
            adl_memRelease(pModbusData->allocated);
            pModbusData->allocated = NULL;
        }
        wm_sprintf(ltsc_Temp, "\r\n >>>> ERROR polling '%d'", result);
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
        wm_sprintf(ltsc_Temp, "\r\n ------------------------------------------------------------------------------");
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
    }

    choice = (choice + 1) % 8;
}

void MAIN_StartAppTimerHandler(u8 id, void* pContext) {
    wm_sprintf(ltsc_Temp, "\r\n Main application started beautifully");
    adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);

    // init modbustack
    SerialConfig serialConfig;
    serialConfig.identity = SERIAL_UART2;
    serialConfig.baudrate = SERIAL_UART_BAUDRATE_19200;
    serialConfig.parity = SERIAL_UART_ODD_PARITY;
    serialConfig.data = SERIAL_UART_DATA_8;
    serialConfig.stop = SERIAL_UART_STOP_1;
    serialConfig.flowControl = SERIAL_UART_FC_NONE;
    serialConfig.timeout = 10;
    serialConfig.retry = 0;
    modbusData.allocated = NULL;
    if (SERIAL_STATUS_OK == SRLFWK_ADP_InitAdapter(&(modbusData.pSerialContext), &serialConfig, ModbusHandler, MODBUS_SER_InitSerializer, (void*) MODBUS_RTU, &modbusData)) {
        wm_sprintf(ltsc_Temp, "\r\nOK Stack initialized");
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);

        //        if (SERIAL_STATUS_OK == SRLFWK_ADP_EnableHardwareSwitch(pSerialContext, 12, MODBUS_GPIO_HIGH)) {
        //            wm_sprintf(ltsc_Temp, "\r\nOK GPIO online");
        //            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp) ;
        //        } else {
        //            wm_sprintf(ltsc_Temp, "\r\nERROR failed to get GPIO");
        //            adl_atSendResponse(ADL_AT_UNS, ltsc_Temp) ;
        //        }
        adl_tmrSubscribeExt(TRUE, 30, ADL_TMR_TYPE_100MS, MAIN_PollingTimerHandler, &modbusData, TRUE);
    } else {
        wm_sprintf(ltsc_Temp, "\r\nERROR Failed to initialize stack");
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
    }
}

void adl_main(adl_InitType_e initType) {
    s8 flashHandle = -1;
    u8 counter = 1;

    flashHandle = adl_flhSubscribe(PERSISTENT_COUNTER, 1);
    if (adl_flhExist(PERSISTENT_COUNTER, 0) == 0) {
        adl_flhWrite(PERSISTENT_COUNTER, 0, sizeof(counter), &counter);
    } else {
        adl_flhRead(PERSISTENT_COUNTER, 0, sizeof(counter), &counter);
        counter++;
        adl_flhWrite(PERSISTENT_COUNTER, 0, sizeof(counter), &counter);
    }

    if (initType == ADL_INIT_REBOOT_FROM_EXCEPTION) {
        wm_sprintf(ltsc_Temp, "\r\nA Fatal Error Occurred Test is restarting (%d)", counter);
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
    } else {
        wm_sprintf(ltsc_Temp, "\r\nTest modbus starting(%d) in 15s...'%d'", counter, initType);
        adl_atSendResponse(ADL_AT_UNS, ltsc_Temp);
    }

    UART_Init();

    // The application will switch the UART to data mode n seconds later, so the user has 15s to enter
    adl_tmrSubscribe(FALSE, 150, ADL_TMR_TYPE_100MS, MAIN_StartAppTimerHandler);
}

