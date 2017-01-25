//--------------------------------------------------------------------------------------------------
/**
 * @file pa_uartMode_default.c
 *
 * Default implementation of @ref c_pa_uartMode interface
 *
 * Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "pa_uartMode.h"


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
)
{
    LE_ERROR("Unsupported function.");
    return LE_FAULT;
}


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
)
{
    LE_ERROR("Unsupported function.");
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Init this component
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
}

