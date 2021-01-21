/**
 * @file pa_remote.c
 *
 * AT Proxy remote PA implementation
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "atProxyCmdHandler.h"

//--------------------------------------------------------------------------------------------------
/**
 * Process unsolicited data/messages if there are any.
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_remote_ProcessUnsolicitedMsg
(
    void
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Send AT command or data to remote end
 *
 * @return
 *      - LE_OK         Successfully send data
 *      - LE_FAULT      Sending failed
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED  le_result_t pa_remote_Send
(
    char* data,         ///< [IN] Data to be sent
    int len             ///< [IN] Length of data in byte
)
{
    LE_UNUSED(data);
    LE_UNUSED(len);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Open the remote end
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void pa_remote_Open
(
    void
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Wait until remote command process finishes
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_remote_WaitCmdFinish
(
    void
)
{
    return;
}

//--------------------------------------------------------------------------------------------------
/**
 * Signal that remote command process has finished
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED void pa_remote_SignalCmdFinish
(
    void
)
{
    return;
}
