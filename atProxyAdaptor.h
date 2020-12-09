/**
 * @file atProxyAdaptor.h
 *
 * AT Proxy platform adaptor interface.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_AT_PROXY_ADAPTOR_H_INCLUDE_GUARD
#define LE_AT_PROXY_ADAPTOR_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Perform platform initialization once for all component instances
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void le_atProxy_initOnce
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Perform platform initialization
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void le_atProxy_init
(
    void
);

#endif // LE_AT_PROXY_ADAPTOR_H_INCLUDE_GUARD
