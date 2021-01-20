/**
 * @file atProxyUnsolicitedRsp.h
 *
 * Header of unsolicited processor for AT Proxy.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_AT_PROXY_UNSOLICITED_RSP_H_INCLUDE_GUARD
#define LE_AT_PROXY_UNSOLICITED_RSP_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Output unsolicited responses to AT port
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void atProxyUnsolicitedRsp_output
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Process an incoming unsolicited character
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void atProxyUnsolicitedRsp_parse
(
    char ch             ///< [IN] Incoming character of an unsolicited response
);

//--------------------------------------------------------------------------------------------------
/**
 * Initialize unsolicited response processor
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void atProxyUnsolicitedRsp_init
(
    void
);

#endif // LE_AT_PROXY_UNSOLICITED_RSP_H_INCLUDE_GUARD
