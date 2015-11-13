/** @file le_uart.h
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */
#ifndef LEGATO_UART_INCLUDE_GUARD
#define LEGATO_UART_INCLUDE_GUARD

#include "legato.h"

uint32_t le_uart_open (const char* port);

int le_uart_read    (uint32_t Handle, char* buf, uint32_t buf_len);
int le_uart_write   (uint32_t Handle, const char* buf, uint32_t buf_len);
int le_uart_ioctl   (uint32_t Handle, uint32_t Cmd, void* pParam );
int le_uart_close   (uint32_t Handle);

void le_uart_defaultConfig(int fd);

#endif /* LEGATO_UART_INCLUDE_GUARD */
