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

#include "lua_modbus_serializer.h"
#include "modbus_serializer.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MODULE_NAME "MODBUS_SERIALIZER"

static const char* MODE_RTU = "RTU";
static const char* MODE_ASCII = "ASCII";
static const char* MODE_TCP = "TCP";

typedef struct ModbusUserData_ {
    Serializer serializer;     // serializer
    ModbusRequest request;     // request
    void* allocated;
} ModbusUserData;

static int ProcessRequest(lua_State *L, ModbusUserData* pModbusUserData, uint8_t isCustom);

static int l_MODBUS_InitContext(lua_State *L) {
    SerialStatus status = SERIAL_STATUS_UNEXPECTED_ERROR;
    ModbusRequestMode mode = MODBUS_RTU;

    // store a modbuscontext address
    ModbusUserData* pModbusUserData = (ModbusUserData*) lua_newuserdata(L, sizeof(ModbusUserData));
    memset(pModbusUserData, 0, sizeof(ModbusUserData));

    // set its metatable
    luaL_getmetatable(L, MODULE_NAME);
    lua_setmetatable(L, -2);

    if (!lua_isnil(L, 1)) {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "'mode' should be 'RTU', 'ASCII' or 'TCP'\n");
        } else {
            const char* strMode = lua_tostring(L, 1);
            if (strcmp(strMode, MODE_RTU) == 0) {
                mode = MODBUS_RTU;
            } else if (strcmp(strMode, MODE_ASCII) == 0) {
                mode = MODBUS_ASCII;
            } else if (strcmp(strMode, MODE_TCP) == 0) {
                mode = MODBUS_TCP;
            } else {
                luaL_error(L, "'mode' should be 'RTU', 'ASCII' or 'TCP'\n");
            }
        }
    }

    status = MODBUS_SER_InitSerializer(&(pModbusUserData->serializer), (void*)mode);
    if (status != SERIAL_STATUS_OK) {
        MODBUS_SER_ReleaseSerializer(&(pModbusUserData->serializer));
        lua_pushnil(L);
        lua_pushstring(L, statusToString(status));
        return 2;
    }
    else {
        return 1;
    }
}

static int l_MODBUS_ReleaseContext(lua_State *L) {
    ModbusUserData* pModbusUserData = (ModbusUserData *) luaL_checkudata(L, 1, MODULE_NAME);

    if (pModbusUserData->allocated != NULL) {
        free(pModbusUserData->allocated);
        pModbusUserData->allocated = NULL;
    }

    MODBUS_SER_ReleaseSerializer(&(pModbusUserData->serializer));
    return 0;
}

static int ProcessRequest(lua_State *L, ModbusUserData* pModbusUserData, uint8_t isCustom) {
    uint8_t* pBuffer = NULL;
    uint16_t bufferLength = 0;
    SerialStatus status;

    if (isCustom) {
        status = MODBUS_SER_CreateCustomRequest(&(pModbusUserData->serializer), &(pModbusUserData->request));
    } else {
        status = MODBUS_SER_CreateRequest(&(pModbusUserData->serializer), &(pModbusUserData->request));
    }
    if (status != SERIAL_STATUS_OK) {
        if (pModbusUserData->allocated != NULL) {
            free(pModbusUserData->allocated);
            pModbusUserData->allocated = NULL;
        }
        lua_pushnil(L);
        lua_pushstring(L, statusToString(status));
        return 2;
    } else {
        MODBUS_SER_GetRequestPDU(&(pModbusUserData->serializer), &pBuffer, &bufferLength);
        lua_pushlstring(L, (const char*) pBuffer, (size_t) bufferLength);
        lua_pushinteger(L, MODBUS_SER_GetExpectedResponseLength(&(pModbusUserData->serializer)));
        return 2;
    }
}

static int l_MODBUS_ReadCoils(lua_State *L) {
    ModbusUserData* pModbusUserData = (ModbusUserData *) luaL_checkudata(L, 1, MODULE_NAME);

    pModbusUserData->request.slaveId = luaL_checkint(L, 2);
    pModbusUserData->request.function = MODBUS_FUNC_READ_COILS;
    pModbusUserData->request.startingAddress = luaL_checkint(L, 3);
    pModbusUserData->request.numberOfObjects = luaL_checkint(L, 4);
    if (pModbusUserData->request.numberOfObjects > 0) {
        pModbusUserData->request.byteCount = ((pModbusUserData->request.numberOfObjects / 8) + (((pModbusUserData->request.numberOfObjects % 8) != 0) ? 1 : 0));
        pModbusUserData->allocated = malloc(pModbusUserData->request.byteCount * sizeof(uint8_t));
        pModbusUserData->request.value.pValues = pModbusUserData->allocated;
    }

    return ProcessRequest(L, pModbusUserData, 0);
}

static int l_MODBUS_ReadDiscreteInputs(lua_State *L) {
    ModbusUserData* pModbusUserData = (ModbusUserData *) luaL_checkudata(L, 1, MODULE_NAME);

    pModbusUserData->request.slaveId = luaL_checkint(L, 2);
    pModbusUserData->request.function = MODBUS_FUNC_READ_DISCRETE_INPUTS;
    pModbusUserData->request.startingAddress = luaL_checkint(L, 3);
    pModbusUserData->request.numberOfObjects = luaL_checkint(L, 4);
    if (pModbusUserData->request.numberOfObjects > 0) {
        pModbusUserData->request.byteCount = ((pModbusUserData->request.numberOfObjects / 8) + (((pModbusUserData->request.numberOfObjects % 8) != 0) ? 1 : 0));
        pModbusUserData->allocated = malloc(pModbusUserData->request.byteCount * sizeof(uint8_t));
        pModbusUserData->request.value.pValues = pModbusUserData->allocated;
    }

    return ProcessRequest(L, pModbusUserData, 0);
}


static int l_MODBUS_ReadHoldingRegisters(lua_State *L) {
    ModbusUserData* pModbusUserData = (ModbusUserData *) luaL_checkudata(L, 1, MODULE_NAME);

    pModbusUserData->request.slaveId = luaL_checkint(L, 2);
    pModbusUserData->request.function = MODBUS_FUNC_READ_HOLDING_REGISTERS;
    pModbusUserData->request.startingAddress = luaL_checkint(L, 3);
    pModbusUserData->request.numberOfObjects = luaL_checkint(L, 4);
    if (pModbusUserData->request.numberOfObjects > 0) {
        pModbusUserData->request.byteCount = pModbusUserData->request.numberOfObjects << 1;
        pModbusUserData->allocated = malloc(pModbusUserData->request.numberOfObjects * sizeof(uint16_t));
        pModbusUserData->request.value.pValues = pModbusUserData->allocated;
    }

    return ProcessRequest(L, pModbusUserData, 0);
}


static int l_MODBUS_ReadInputRegisters(lua_State *L) {
    ModbusUserData* pModbusUserData = (ModbusUserData *) luaL_checkudata(L, 1, MODULE_NAME);

    pModbusUserData->request.slaveId = luaL_checkint(L, 2);
    pModbusUserData->request.function = MODBUS_FUNC_READ_INPUT_REGISTERS;
    pModbusUserData->request.startingAddress = luaL_checkint(L, 3);
    pModbusUserData->request.numberOfObjects = luaL_checkint(L, 4);
    if (pModbusUserData->request.numberOfObjects > 0) {
        pModbusUserData->request.byteCount = pModbusUserData->request.numberOfObjects << 1;
        pModbusUserData->allocated = malloc(pModbusUserData->request.numberOfObjects * sizeof(uint16_t));
        pModbusUserData->request.value.pValues = pModbusUserData->allocated;
    }

    return ProcessRequest(L, pModbusUserData, 0);
}


static int l_MODBUS_WriteSingleCoil(lua_State *L) {
    ModbusUserData* pModbusUserData = (ModbusUserData *) luaL_checkudata(L, 1, MODULE_NAME);

    pModbusUserData->request.slaveId = luaL_checkint(L, 2);
    pModbusUserData->request.function = MODBUS_FUNC_WRITE_SINGLE_COIL;
    pModbusUserData->request.startingAddress = luaL_checkint(L, 3);
    pModbusUserData->request.byteCount = 1;
    pModbusUserData->request.numberOfObjects = 1; // unused
    if (lua_isboolean(L, 4)) {
        switch (lua_toboolean(L, 4)) {
        default:
        case 1:
            pModbusUserData->request.value.iValue = (uint32_t) MODBUS_SINGLE_COIL_ON;
            break;
        case 0:
            pModbusUserData->request.value.iValue = (uint32_t) MODBUS_SINGLE_COIL_OFF;
            break;
        }
    } else {
        luaL_error(L, "'value' should be boolean\n");
    }

    return ProcessRequest(L, pModbusUserData, 0);
}


static int l_MODBUS_WriteSingleRegister(lua_State *L) {
    ModbusUserData* pModbusUserData = (ModbusUserData *) luaL_checkudata(L, 1, MODULE_NAME);

    pModbusUserData->request.slaveId = luaL_checkint(L, 2);
    pModbusUserData->request.function = MODBUS_FUNC_WRITE_SINGLE_REGISTER;
    pModbusUserData->request.startingAddress = luaL_checkint(L, 3);
    pModbusUserData->request.byteCount = 1;
    pModbusUserData->request.value.iValue = (uint32_t) luaL_checkint(L, 4);
    pModbusUserData->request.numberOfObjects = 1; // unused

    return ProcessRequest(L, pModbusUserData, 0);
}


static int l_MODBUS_WriteMultipleCoils(lua_State *L) {
    size_t byteCount;

    ModbusUserData* pModbusUserData = (ModbusUserData *) luaL_checkudata(L, 1, MODULE_NAME);

    pModbusUserData->request.slaveId = luaL_checkint(L, 2);
    pModbusUserData->request.function = MODBUS_FUNC_WRITE_MULTIPLE_COILS;
    pModbusUserData->request.startingAddress = luaL_checkint(L, 3);
    pModbusUserData->request.numberOfObjects = luaL_checkint(L, 4);
    pModbusUserData->request.value.pValues = (void*) luaL_checklstring(L, 5, &byteCount);
    pModbusUserData->request.byteCount = byteCount;

    return ProcessRequest(L, pModbusUserData, 0);
}


static int l_MODBUS_WriteMultipleRegisters(lua_State *L) {
    size_t byteCount;

    ModbusUserData* pModbusUserData = (ModbusUserData *) luaL_checkudata(L, 1, MODULE_NAME);

    pModbusUserData->request.slaveId = luaL_checkint(L, 2);
    pModbusUserData->request.function = MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS;
    pModbusUserData->request.startingAddress = luaL_checkint(L, 3);
    pModbusUserData->request.value.pValues = (void*) luaL_checklstring(L, 4, &byteCount);
    pModbusUserData->request.numberOfObjects = byteCount >> 1;
    pModbusUserData->request.byteCount = byteCount;

    return ProcessRequest(L, pModbusUserData, 0);
}


static int l_MODBUS_SendRawData(lua_State *L) {
  size_t byteCount;
  void *pRequest;
  uint16_t i;

  ModbusUserData* pModbusUserData = (ModbusUserData *) luaL_checkudata(L, 1, MODULE_NAME);

  pModbusUserData->request.slaveId = luaL_checkint(L, 2);
  pModbusUserData->request.function = MODBUS_FUNC_SEND_RAW_DATA;

  pRequest = (void*) luaL_checklstring(L, 3, &byteCount);
  pModbusUserData->allocated = malloc(MODBUS_TCP_MAX_DATA_SIZE * sizeof(uint8_t));
  pModbusUserData->request.value.pValues = pModbusUserData->allocated;
  for( i=0; (i<byteCount) && (i<MODBUS_TCP_MAX_DATA_SIZE); i++)
    ((uint8_t *) pModbusUserData->request.value.pValues)[i] = (uint8_t)((uint8_t *)pRequest)[i];

  pModbusUserData->request.byteCount = byteCount;
  pModbusUserData->request.numberOfObjects = byteCount >> 1;
  pModbusUserData->request.startingAddress = 0; // DIS : Useless


  return ProcessRequest(L, pModbusUserData, 0);
}

static int l_MODBUS_CustomRequest(lua_State *L) {
  void *pRequest;
  size_t byteCount;
  ModbusUserData* pModbusUserData = (ModbusUserData *) luaL_checkudata(L, 1, MODULE_NAME);
  pModbusUserData->request.slaveId = luaL_checkint(L, 2);
  pModbusUserData->request.function = luaL_checkinteger(L, 3);
  pModbusUserData->request.startingAddress = 0;

  pRequest = (void*) luaL_optlstring(L, 4, "", &byteCount);
  pModbusUserData->allocated = malloc(pModbusUserData->serializer.maxSize * sizeof(uint8_t));
  pModbusUserData->request.value.pValues = pModbusUserData->allocated;
  pModbusUserData->request.byteCount = byteCount;
  memcpy(pModbusUserData->request.value.pValues, pRequest,
      byteCount > pModbusUserData->serializer.maxSize ? pModbusUserData->serializer.maxSize : byteCount);

  return ProcessRequest(L, pModbusUserData, 1);
}


static int l_MODBUS_ReceiveResponse(lua_State *L) {
    char charTab[80] = { 0 };
    uint8_t n = 0;
    size_t bufferLength = 0;

    ModbusUserData* pModbusUserData = (ModbusUserData *) luaL_checkudata(L, 1, MODULE_NAME);
    const char* buffer = luaL_checklstring(L, 2, &bufferLength);
    ModbusSpecifics *pSpecifics = (ModbusSpecifics*)pModbusUserData->serializer.pSpecifics;

    // copy to response buffer
    memset(pModbusUserData->serializer.pResponseBuffer, 0, pModbusUserData->serializer.responseBufferLength * sizeof(uint8_t));
    memcpy(pModbusUserData->serializer.pResponseBuffer, buffer, bufferLength);
    pModbusUserData->serializer.responseBufferLength = bufferLength;

    // extract data from response
    SerialStatus status = MODBUS_SER_AnalyzeResponse(&(pModbusUserData->serializer), MODBUS_SER_CheckResponse(&(pModbusUserData->serializer)));
    // push results
    switch (status) {
    case SERIAL_STATUS_OK:
      if (pSpecifics->isCustom) {
        lua_pushlstring(L, pSpecifics->response.value.pValues, pSpecifics->response.byteCount);
        n = 1;
        break;
      }
        switch (pSpecifics->response.function) {
        case MODBUS_FUNC_READ_COILS:
        case MODBUS_FUNC_READ_DISCRETE_INPUTS:
        case MODBUS_FUNC_READ_HOLDING_REGISTERS:
        case MODBUS_FUNC_READ_INPUT_REGISTERS:
        case MODBUS_FUNC_SEND_RAW_DATA:
            // push read result
            lua_pushlstring(L, pSpecifics->response.value.pValues, pSpecifics->response.byteCount);

            n = 1;
            break;

        case MODBUS_FUNC_WRITE_SINGLE_COIL:
        case MODBUS_FUNC_WRITE_SINGLE_REGISTER:
        case MODBUS_FUNC_WRITE_MULTIPLE_COILS:
        case MODBUS_FUNC_WRITE_MULTIPLE_REGISTERS:
            // push status
            lua_pushstring(L, "ok");

            n = 1;
            break;

        default:
            // push nil
            lua_pushnil(L);
            // push error string
            lua_pushstring(L, "modbus response error unhandled function");

            n = 2;
            break;
        }
        break;

    case SERIAL_STATUS_RESPONSE_EXCEPTION:
        // push nil
        lua_pushnil(L);
        // push error string
        sprintf(charTab, "%s - %s", statusToString(status), MODBUS_SER_GetExceptionString(pSpecifics->response.exception));
        lua_pushstring(L, charTab);

        n = 2;
        break;

    default:
        // push nil
        lua_pushnil(L);
        // push error string
        lua_pushstring(L, statusToString(status));

        n = 2;
        break;
    }

    // free value
    if (pModbusUserData->allocated != NULL) {
        free(pModbusUserData->allocated);
        pModbusUserData->allocated = NULL;
    }

    return n;
}

static const luaL_Reg modbus_methods[] = {
        { "readCoils", l_MODBUS_ReadCoils },
        { "readDiscreteInputs", l_MODBUS_ReadDiscreteInputs },
        { "readHoldingRegisters", l_MODBUS_ReadHoldingRegisters },
        { "readInputRegisters", l_MODBUS_ReadInputRegisters },
        { "writeSingleCoil", l_MODBUS_WriteSingleCoil },
        { "writeSingleRegister", l_MODBUS_WriteSingleRegister },
        { "writeMultipleCoils",    l_MODBUS_WriteMultipleCoils },
        { "writeMultipleRegisters", l_MODBUS_WriteMultipleRegisters },
        { "sendRawData", l_MODBUS_SendRawData },
        { "customRequest", l_MODBUS_CustomRequest },
        { "receiveResponse", l_MODBUS_ReceiveResponse },
        { "releaseContext", l_MODBUS_ReleaseContext },
        {"__gc", l_MODBUS_ReleaseContext },
        { NULL, NULL } };

static const luaL_Reg modbus_functions[] = { { "initContext", l_MODBUS_InitContext }, { NULL, NULL } };

int luaopen_modbus_serializer(lua_State* L) {
    luaL_newmetatable(L, MODULE_NAME);

    // metable.__index = metatable
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    // register modbus methods in the metatable
    luaL_register(L, NULL, modbus_methods);

    // register modbus functions
    luaL_register(L, "modbus.serializer", modbus_functions);
    return 1;
}
