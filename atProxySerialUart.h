/**
 * @file atProxySerialUart.h
 *
 * AT Proxy Serial UART interface.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_AT_PROXY_SERIAL_UART_H_INCLUDE_GUARD
#define LE_AT_PROXY_SERIAL_UART_H_INCLUDE_GUARD

// Initialize the AT Port External Serial UART
void atProxySerialUart_init(void);

// Write to te AT Port External Serial UART
uint32_t atProxySerialUart_write(char *buf, uint32_t len);

// Read from the AT Port External Serial UART
uint32_t atProxySerialUart_read(char *buf, uint32_t len);

// Disable monitoring events on AT Port External Serial UART
void atProxySerialUart_disable(void);

// Enable monitoring events on AT Port External Serial UART
void atProxySerialUart_enable(void);

#endif /* LE_AT_PROXY_SERIAL_UART_H_INCLUDE_GUARD */
