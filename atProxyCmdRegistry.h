/**
 * @file atProxyCmdRegistry.h
 *
 * AT Proxy Command Registry.
 *
 * This file defines the "local" AT Command registry
 * for the AT Proxy running on the MCU processor.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#ifndef LE_AT_PROXY_CMD_REGISTRY_H_INCLUDE_GUARD
#define LE_AT_PROXY_CMD_REGISTRY_H_INCLUDE_GUARD

// AT Command Registry index
// NOTE: Add NEW AT Commands before AT_CMD_MAX
enum le_atProxy_atCommand
{
    AT_CMD_ORP,  ///< "AT+ORP" AT Command
    AT_CMD_MAX
};

#ifdef AT_PROXY_CMD_REGISTRY_IMPL
// Static AT Command Registry
static struct le_atProxy_StaticCommand  AtCmdRegistry[AT_CMD_MAX] =
{
    { "AT+ORP", NULL, NULL },   ///< AT+ORP AT Command
};
 #endif /* end AT_PROXY_CMD_REGISTRY_IMPL */


#endif /* LE_AT_PROXY_CMD_REGISTRY_H_INCLUDE_GUARD */