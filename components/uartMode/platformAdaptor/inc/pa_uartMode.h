/**
 * @page c_pa_uartMode UART Mode Configuration Interface
 *
 * This module contains a platform independent API for setting UART ports' mode.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LEGATO_PA_UART_MODE_INCLUDE_GUARD
#define LEGATO_PA_UART_MODE_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * UART modes. Must numerically match the corresponding QMI modes.
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    UART_DISABLED      = 0,
    UART_AT_CMD        = 1,
    UART_DIAG_MSG      = 2,
    UART_NMEA          = 4,
    UART_LINUX_CONSOLE = 16,
    UART_LINUX_APP     = 17,
}
pa_uartMode_Mode_t;


//--------------------------------------------------------------------------------------------------
/**
 * Gets the current mode.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_uartMode_Get
(
    uint uartNum,                               ///< [IN] 1 for UART1, 2 for UART2.
    pa_uartMode_Mode_t* modePtr                 ///< [OUT] Port's mode.
);


//--------------------------------------------------------------------------------------------------
/**
 * Sets the current mode.
 *
 * @return
 *      LE_OK if successful.
 *      LE_FAULT if there was an error.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t pa_uartMode_Set
(
    uint uartNum,                               ///< [IN] 1 for UART1, 2 for UART2.
    pa_uartMode_Mode_t mode                     ///< [IN] Port's mode.
);


#endif // LEGATO_PA_UART_MODE_INCLUDE_GUARD
