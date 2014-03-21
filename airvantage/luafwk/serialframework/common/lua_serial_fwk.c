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

#include "lua_serial_fwk.h"

#include <string.h>

static const char* LEVEL_HIGH = "high";
static const char* LEVEL_LOW = "low";

static const char* IDENTITY_UART1 = "UART1";
static const char* IDENTITY_UART2 = "UART2";
static const char* IDENTITY_UART3 = "UART3";

static const char* PARITY_NONE = "none";
static const char* PARITY_EVEN = "even";
static const char* PARITY_ODD = "odd";

static const char* FC_NONE = "none";
static const char* FC_RTS_CTS = "rtscts";
static const char* FC_XON_XOFF = "xonxoff";

void GetConfigIdentity(lua_State* L, int index, SerialUARTId* identity) {
    if (!lua_isnil(L, index)) {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "'name' should be 'UART1' or 'UART2'\n");
        } else {
            const char* strIdentity = lua_tostring(L, index);
            if (strcmp(strIdentity, IDENTITY_UART1) == 0) {
                *identity = SERIAL_UART1;
            } else if (strcmp(strIdentity, IDENTITY_UART2) == 0) {
                *identity = SERIAL_UART2;
            } else if (strcmp(strIdentity, IDENTITY_UART3) == 0) {
                *identity = SERIAL_UART3;
            } else {
                luaL_error(L, "'name' should be 'UART1' or 'UART2'\n");
            }
        }
    }
}

void GetConfigUART(lua_State* L, int index, SerialConfig* serialConfig) {
    if (!lua_isnil(L, index)) {
        if (lua_istable(L, index)) {
            lua_getfield(L, index, "baudrate");
            GetConfigBaudrate(L, -1, &(serialConfig->baudrate));
            lua_pop(L, 1);

            lua_getfield(L, index, "parity");
            GetConfigParity(L, -1, &(serialConfig->parity));
            lua_pop(L, 1);

            lua_getfield(L, index, "data");
            GetConfigData(L, -1, &(serialConfig->data));
            lua_pop(L, 1);

            lua_getfield(L, index, "stop");
            GetConfigStop(L, -1, &(serialConfig->stop));
            lua_pop(L, 1);

            lua_getfield(L, index, "flowcontrol");
            GetConfigFlowControl(L, -1, &(serialConfig->flowControl));
            lua_pop(L, 1);

            lua_getfield(L, index, "timeout");
            GetConfigTimeout(L, -1, &(serialConfig->timeout));
            lua_pop(L, 1);

            lua_getfield(L, index, "retry");
            GetConfigRetry(L, -1, &(serialConfig->retry));
            lua_pop(L, 1);
        } else {
            luaL_error(L, "'cfg' should be a table");
        }
    }
}

void GetConfigBaudrate(lua_State* L, int index, SerialUARTBaudrate* baudrate) {
    if (!lua_isnil(L, index)) {
        int intBaudrate = luaL_checkint(L, index);
        switch (intBaudrate) {
        case 300:
            *baudrate = SERIAL_UART_BAUDRATE_300;
            break;
        case 600:
            *baudrate = SERIAL_UART_BAUDRATE_600;
            break;
        case 1200:
            *baudrate = SERIAL_UART_BAUDRATE_1200;
            break;
        case 2400:
            *baudrate = SERIAL_UART_BAUDRATE_2400;
            break;
        case 4800:
            *baudrate = SERIAL_UART_BAUDRATE_4800;
            break;
        case 9600:
            *baudrate = SERIAL_UART_BAUDRATE_9600;
            break;
        case 19200:
            *baudrate = SERIAL_UART_BAUDRATE_19200;
            break;
        case 38400:
            *baudrate = SERIAL_UART_BAUDRATE_38400;
            break;
        case 57600:
            *baudrate = SERIAL_UART_BAUDRATE_57600;
            break;
        case 115200:
            *baudrate = SERIAL_UART_BAUDRATE_115200;
            break;
        default:
            luaL_error(L, "'baudrate' should be 300, 600, 1200, 2400, 4800 ,9600, 19200, 38400, 57600 or 115200\n");
            break;
        }
    }
}

void GetConfigParity(lua_State* L, int index, SerialUARTParity* parity) {
    if (!lua_isnil(L, index)) {
        if (!lua_isstring(L, index)) {
            luaL_error(L, "'parity' should be 'none', 'even' or 'odd'\n");
        } else {
            const char* strParity = lua_tostring(L, -1);
            if (strcmp(strParity, PARITY_NONE) == 0) {
                *parity = SERIAL_UART_NO_PARITY;
            } else if (strcmp(strParity, PARITY_EVEN) == 0) {
                *parity = SERIAL_UART_EVEN_PARITY;
            } else if (strcmp(strParity, PARITY_ODD) == 0) {
                *parity = SERIAL_UART_ODD_PARITY;
            } else {
                luaL_error(L, "'parity' should be 'none', 'even' or 'odd'\n");
            }
        }
    }
}

void GetConfigData(lua_State* L, int index, SerialUARTData* data) {
    if (!lua_isnil(L, index)) {
        int intData = luaL_checkint(L, index);
        switch (intData) {
        case 7:
            *data = SERIAL_UART_DATA_7;
            break;
        case 8:
            *data = SERIAL_UART_DATA_8;
            break;
        default:
            luaL_error(L, "'data' should be 7 or 8\n");
            break;
        }
    }
}

void GetConfigStop(lua_State* L, int index, SerialUARTStop* stop) {
    if (!lua_isnil(L, index)) {
        int intStop = luaL_checkint(L, index);
        switch (intStop) {
        case 1:
            *stop = SERIAL_UART_STOP_1;
            break;
        case 2:
            *stop = SERIAL_UART_STOP_2;
            break;
        default:
            luaL_error(L, "'stop' should be 1 or 2\n");
            break;
        }
    }
}

void GetConfigFlowControl(lua_State* L, int index, SerialUARTFControl* flowControl) {
    if (!lua_isnil(L, index)) {
        if (!lua_isstring(L, index)) {
            luaL_error(L, "'flowcontrol' should be 'rtscts', 'xonxoff' or 'none'\n");
        } else {
            const char* strFlowControl = lua_tostring(L, -1);
            if (strcmp(strFlowControl, FC_NONE) == 0) {
                *flowControl = SERIAL_UART_FC_NONE;
            } else if (strcmp(strFlowControl, FC_RTS_CTS) == 0) {
                *flowControl = SERIAL_UART_FC_RTS_CTS;
            } else if (strcmp(strFlowControl, FC_XON_XOFF) == 0) {
                *flowControl = SERIAL_UART_FC_XON_XOFF;
            } else {
                luaL_error(L, "'flowcontrol' should be 'rtscts', 'xonxoff' or 'none'\n");
            }
        }
    }
}

// WARNING x10 coeff du to timer in x100ms (openAT)
void GetConfigTimeout(lua_State* L, int index, uint16_t* timeout) {
    if (!lua_isnil(L, index)) {
        *timeout = (uint16_t) 10 * luaL_checkint(L, index);
    }
}

void GetConfigRetry(lua_State* L, int index, uint16_t* retry) {
    if (!lua_isnil(L, index)) {
        *retry = (uint16_t) luaL_checkint(L, index);
    }
}

void GetConfigLevel(lua_State* L, int index, SerialGPIOWriteModeLevel* level) {
    if (!lua_isstring(L, index)) {
        luaL_error(L, "'level' should be 'high' or 'low'\n");
    } else {
        const char* strLevel = lua_tostring(L, index);
        if (strcmp(strLevel, LEVEL_HIGH) == 0) {
            *level = SERIAL_GPIO_HIGH;
        } else if (strcmp(strLevel, LEVEL_LOW) == 0) {
            *level = SERIAL_GPIO_LOW;
        } else {
            luaL_error(L, "'level' should be 'high' or 'low'\n");
        }
    }
}

static const char *const SerialStatusString[] = {
    "SERIAL_STATUS_OK",
    "SERIAL_STATUS_STACK_NOT_READY",
    "SERIAL_STATUS_RESPONSE_INVALID_FRAME",
    "SERIAL_STATUS_RESPONSE_SHORT_FRAME",
    "SERIAL_STATUS_RESPONSE_BAD_SLAVE",
    "SERIAL_STATUS_RESPONSE_BAD_FUNCTION",
    "SERIAL_STATUS_RESPONSE_BAD_CHECKSUM",
    "SERIAL_STATUS_RESPONSE_INCOMPLETE_FRAME",
    "SERIAL_STATUS_RESPONSE_EXCEPTION",
    "SERIAL_STATUS_RESPONSE_TIMEOUT",
    "SERIAL_STATUS_ALLOC_FAILED",
    "SERIAL_STATUS_WRONG_PARAMS",
    "SERIAL_STATUS_CORRUPTED_CONTEXT",
    "SERIAL_STATUS_REQUEST_PARAMETER_ERROR",
    "SERIAL_STATUS_UNEXPECTED_ERROR"
};

const char* statusToString(SerialStatus status) {
    if(status < SERIAL_STATUS_OK || status > SERIAL_STATUS_UNEXPECTED_ERROR)
        return "Invalid SerialStatus";
    else
        return SerialStatusString[status];
}
