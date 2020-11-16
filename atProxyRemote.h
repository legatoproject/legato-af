/**
 * @file atProxyRemote.h
 *
 * AT Proxy remote interface.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_AT_PROXY_REMOTE_H_INCLUDE_GUARD
#define LE_AT_PROXY_REMOTE_H_INCLUDE_GUARD

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Process unsolicited data/messages if there are any.
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
void atProxyRemote_processUnsolicitedMsg
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
le_result_t atProxyRemote_send
(
    char* data,         /// [IN] Data to be sent
    int len             /// [IN] Length of data in byte
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize the remote end
 *
 * @return none.
 */
//--------------------------------------------------------------------------------------------------
void atProxyRemote_init
(
    void
);

#endif // LE_AT_PROXY_REMOTE_H_INCLUDE_GUARD
