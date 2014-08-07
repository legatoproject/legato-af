/**
 * This module is for unit testing of the modemServices MDC component.
 *
 *
 * Copyright (C) Sierra Wireless, Inc. 2013-2014. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>

#include "interfaces.h"

// Include macros for printing out values
#include "le_print.h"

static le_mdc_ProfileRef_t ProfileRef;

static void StateChangeHandler
(
    bool  isConnected,
    void* contextPtr
)
{
    le_mdc_ProfileRef_t ProfileRef = contextPtr;
    char name[LE_MDC_INTERFACE_NAME_MAX_LEN + 1];

    le_mdc_GetInterfaceName(ProfileRef, name, sizeof(name));


    LE_DEBUG("\n================================================");
    LE_PRINT_VALUE("%s", name);
    LE_PRINT_VALUE("%u", isConnected);
    LE_DEBUG("\n================================================");
}


static void* HandlerThread(void* contextPtr)
{
    le_mdc_ProfileRef_t ProfileRef = contextPtr;

    le_mdc_AddSessionStateHandler(ProfileRef, StateChangeHandler,contextPtr);

    le_event_RunLoop();
    return NULL;
}

static bool TestIpv4Connectivity
(
    le_mdc_ProfileRef_t ProfileRef
)
{
    char interfaceName[100];
    char gatewayAddr[100];
    char dns1Addr[100];
    char dns2Addr[100];
    char system_cmd[200];
    FILE* resolvFilePtr;

    if ( le_mdc_GetInterfaceName(ProfileRef, interfaceName, sizeof(interfaceName)) != LE_OK )
    {
        LE_INFO("le_mdc_GetInterfaceName failed");
        return false;
    }
    LE_INFO("le_mdc_GetInterfaceName called");
    LE_PRINT_VALUE("%s", interfaceName);

    if ( !le_mdc_IsIPv4(ProfileRef) )
    {
        LE_INFO("The interface do not provide the Ipv4 connectivity");
        return false;
    }

    if ( le_mdc_GetIPv4GatewayAddress(ProfileRef, gatewayAddr, sizeof(gatewayAddr)) != LE_OK )
    {
        LE_INFO("le_mdc_GetGatewayAddress failed");
        return false;
    }
    LE_INFO("le_mdc_GetGatewayAddress called");
    LE_PRINT_VALUE("%s", gatewayAddr);

    LE_INFO("waiting a few seconds before setting the route for the default gateway");
    sleep(5);

    snprintf(system_cmd, sizeof(system_cmd), "route add default gw %s", gatewayAddr);
    if ( system(system_cmd) != 0 )
    {
        LE_INFO("system '%s' failed", system_cmd);
        return false;
    }
    LE_INFO("system '%s' called", system_cmd);

    if ( le_mdc_GetIPv4DNSAddresses(ProfileRef,
                                    dns1Addr, sizeof(dns1Addr),
                                    dns2Addr, sizeof(dns2Addr)) != LE_OK )
    {
        LE_INFO("le_mdc_GetDNSAddresses failed");
        return false;
    }
    LE_INFO("le_mdc_GetDNSAddresses called");
    LE_PRINT_VALUE("%s", dns1Addr);
    LE_PRINT_VALUE("%s", dns2Addr);

    resolvFilePtr = fopen("/var/run/resolv.conf", "w");
    if (resolvFilePtr == NULL)
    {
        LE_INFO("fopen on /var/run/resolv.conf failed");
        return false;
    }
    LE_INFO("fopen on /var/run/resolv.conf called");

    if ( (fprintf(resolvFilePtr, "nameserver %s\n", dns1Addr) < 0) ||
         (fprintf(resolvFilePtr, "nameserver %s\n", dns2Addr) < 0) )
    {
        LE_INFO("fprintf failed");
        return false;
    }
    LE_INFO("fprintf called");

    if ( fclose(resolvFilePtr) != 0 )
    {
        LE_INFO("fclose failed");
        return false;
    }
    LE_INFO("fclose called");

    // finally, test the data connection
    if ( system("ping -c 5 www.sierrawireless.com") != 0 )
    {
        LE_INFO("system ping failed");
        return false;
    }
    LE_INFO("system ping called");
    return true;
}

static bool TestIpv6Connectivity
(
    le_mdc_ProfileRef_t ProfileRef
)
{
    char interfaceName[100];
    char gatewayAddr[100];
    char dns1Addr[100];
    char dns2Addr[100];
    char system_cmd[200];
    FILE* resolvFilePtr;

    if ( le_mdc_GetInterfaceName(ProfileRef, interfaceName, sizeof(interfaceName)) != LE_OK )
    {
        LE_INFO("le_mdc_GetInterfaceName failed");
        return false;
    }
    LE_INFO("le_mdc_GetInterfaceName called");
    LE_PRINT_VALUE("%s", interfaceName);

    if ( !le_mdc_IsIPv6(ProfileRef) )
    {
        LE_INFO("The interface do not provide the Ipv6 connectivity");
        return false;
    }

    if ( le_mdc_GetIPv6GatewayAddress(ProfileRef, gatewayAddr, sizeof(gatewayAddr)) != LE_OK )
    {
        LE_INFO("le_mdc_GetGatewayAddress failed");
        return false;
    }
    LE_INFO("le_mdc_GetGatewayAddress called");
    LE_PRINT_VALUE("%s", gatewayAddr);

    LE_INFO("waiting a few seconds before setting the route for the default gateway");
    sleep(5);

    snprintf(system_cmd, sizeof(system_cmd), "route -A inet6 add default gw %s", gatewayAddr);
    if ( system(system_cmd) != 0 )
    {
        LE_INFO("system '%s' failed", system_cmd);
        return false;
    }
    LE_INFO("system '%s' called", system_cmd);

    if ( le_mdc_GetIPv6DNSAddresses(ProfileRef,
                                    dns1Addr, sizeof(dns1Addr),
                                    dns2Addr, sizeof(dns2Addr)) != LE_OK )
    {
        LE_INFO("le_mdc_GetDNSAddresses failed");
        return false;
    }
    LE_INFO("le_mdc_GetDNSAddresses called");
    LE_PRINT_VALUE("%s", dns1Addr);
    LE_PRINT_VALUE("%s", dns2Addr);

    resolvFilePtr = fopen("/var/run/resolv.conf", "w");
    if (resolvFilePtr == NULL)
    {
        LE_INFO("fopen on /var/run/resolv.conf failed");
        return false;
    }
    LE_INFO("fopen on /var/run/resolv.conf called");

    if ( (fprintf(resolvFilePtr, "nameserver %s\n", dns1Addr) < 0) ||
         (fprintf(resolvFilePtr, "nameserver %s\n", dns2Addr) < 0) )
    {
        LE_INFO("fprintf failed");
        return false;
    }
    LE_INFO("fprintf called");

    if ( fclose(resolvFilePtr) != 0 )
    {
        LE_INFO("fclose failed");
        return false;
    }
    LE_INFO("fclose called");

    // finally, test the data connection
    if ( system("ping6 -c 5 www.sierrawireless.com") != 0 )
    {
        LE_INFO("system ping failed");
        return false;
    }
    LE_INFO("system ping called");
    return true;
}

static void* TestStartDataSession(void* contextPtr)
{
    // Hard coded, first profile.
    ProfileRef = le_mdc_GetProfile(1);
    if ( ProfileRef == NULL )
    {
        LE_INFO("load failed");
        return NULL;
    }
    LE_INFO("Load called");

    // Start the handler thread to monitor the state for the just created profile.
    le_thread_Start(le_thread_Create("MDC", HandlerThread,&ProfileRef));

    LE_INFO("Store called");

    if ( le_mdc_StartSession(ProfileRef) != LE_OK )
    {
        LE_INFO("Start failed");
        return NULL;
    }
    LE_INFO("Start called");

    LE_INFO("waiting a few seconds");
    sleep(10);

    if ( le_mdc_StopSession(ProfileRef) != LE_OK )
    {
        LE_INFO("Stop failed");
        return NULL;
    }
    LE_INFO("Stop called");

    // Wait a bit and then restart the data session and configure the network interface.

    LE_INFO("waiting a few more seconds");
    sleep(10);

    if ( le_mdc_StartSession(ProfileRef) != LE_OK )
    {
        LE_INFO("Start failed");
        return NULL;
    }
    LE_INFO("Start called");

    TestIpv4Connectivity(ProfileRef);

    TestIpv6Connectivity(ProfileRef);

    if ( le_mdc_StopSession(ProfileRef) != LE_OK )
    {
        LE_INFO("Stop failed");
        return NULL;
    }
    LE_INFO("Stop called");

    LE_INFO("ALL TESTS PASS");

    le_event_RunLoop();
    return NULL;
}


COMPONENT_INIT
{
    le_thread_Start(le_thread_Create("TestMain", TestStartDataSession, NULL));
}

