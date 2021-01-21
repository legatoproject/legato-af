/**
 * @file pa_remote.h
 *
 * AT Proxy platform adaptor interface for remote end.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef _AT_PROXY_PA_REMOTE_H_INCLUDE_GUARD_
#define _AT_PROXY_PA_REMOTE_H_INCLUDE_GUARD_

#include "legato.h"
#include "atProxyCmdHandler.h"

//--------------------------------------------------------------------------------------------------
/**
 * Process unsolicited data/messages if there are any.
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void pa_remote_ProcessUnsolicitedMsg
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Send AT command or data to remote end
 *
 * @return
 *      - LE_OK         Successfully send data
 *      - LE_FAULT      Sending failed
 */
//--------------------------------------------------------------------------------------------------
le_result_t pa_remote_Send
(
    char* data,         ///< [IN] Data to be sent
    int len             ///< [IN] Length of data in byte
);

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
);

//--------------------------------------------------------------------------------------------------
/**
 * Wait until remote command process finishes
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void pa_remote_WaitCmdFinish
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Signal that remote command process has finished
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void pa_remote_SignalCmdFinish
(
    void
);

#endif // _AT_PROXY_PA_REMOTE_H_INCLUDE_GUARD_
