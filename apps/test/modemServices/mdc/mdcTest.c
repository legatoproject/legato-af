/**
 * This module is for unit testing of the modemServices MDC component.
 *
 *
 * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>

#include "le_ms.h"
#include "le_mdc.h"

// Include macros for printing out values
#include "le_print.h"



static void StateChangeHandler
(
    bool  isConnected,
    void* contextPtr
)
{
    le_mdc_Profile_Ref_t profileRef = (le_mdc_Profile_Ref_t)contextPtr;
    char name[30];

    le_mdc_GetProfileName(profileRef, name, sizeof(name));


    LE_DEBUG("\n================================================");
    LE_PRINT_VALUE("%s", name);
    LE_PRINT_VALUE("%u", isConnected);
    LE_DEBUG("\n================================================");
}


static void* HandlerThread(void* contextPtr)
{
    le_mdc_Profile_Ref_t profileRef = contextPtr;

    le_mdc_AddSessionStateHandler(profileRef, StateChangeHandler, profileRef);

    le_event_RunLoop();
    return NULL;
}


static void* TestStartDataSession(void* contextPtr)
{
    le_mdc_Profile_Ref_t profileRef;
    char interfaceName[100];
    char gatewayAddr[100];
    char dns1Addr[100];
    char dns2Addr[100];
    char system_cmd[200];
    FILE* resolvFilePtr;

    profileRef = le_mdc_LoadProfile("internet");
    if ( profileRef == NULL )
    {
        LE_INFO("load failed");
        return NULL;
    }
    LE_INFO("Load called");

    // Start the handler thread to monitor the state for the just created profile.
    le_thread_Start(le_thread_Create("MDC", HandlerThread, profileRef));

    LE_INFO("Store called");

    if ( le_mdc_StartSession(profileRef) != LE_OK )
    {
        LE_INFO("Start failed");
        return NULL;
    }
    LE_INFO("Start called");

    LE_INFO("waiting a few seconds");
    sleep(10);

    if ( le_mdc_StopSession(profileRef) != LE_OK )
    {
        LE_INFO("Stop failed");
        return NULL;
    }
    LE_INFO("Stop called");

    // Wait a bit and then restart the data session and configure the network interface.

    LE_INFO("waiting a few more seconds");
    sleep(10);

    if ( le_mdc_StartSession(profileRef) != LE_OK )
    {
        LE_INFO("Start failed");
        return NULL;
    }
    LE_INFO("Start called");

    if ( le_mdc_GetInterfaceName(profileRef, interfaceName, sizeof(interfaceName)) != LE_OK )
    {
        LE_INFO("le_mdc_GetInterfaceName failed");
        return NULL;
    }
    LE_INFO("le_mdc_GetInterfaceName called");
    LE_PRINT_VALUE("%s", interfaceName);

    if ( le_mdc_GetGatewayAddress(profileRef, gatewayAddr, sizeof(gatewayAddr)) != LE_OK )
    {
        LE_INFO("le_mdc_GetGatewayAddress failed");
        return NULL;
    }
    LE_INFO("le_mdc_GetGatewayAddress called");
    LE_PRINT_VALUE("%s", gatewayAddr);

    LE_INFO("waiting a few seconds before setting the route for the default gateway");
    sleep(5);

    snprintf(system_cmd, sizeof(system_cmd), "route add default gw %s", gatewayAddr);
    if ( system(system_cmd) != 0 )
    {
        LE_INFO("system '%s' failed", system_cmd);
        return NULL;
    }
    LE_INFO("system '%s' called", system_cmd);

    if ( le_mdc_GetDNSAddresses(profileRef,
                                    dns1Addr, sizeof(dns1Addr),
                                    dns2Addr, sizeof(dns2Addr)) != LE_OK )
    {
        LE_INFO("le_mdc_GetDNSAddresses failed");
        return NULL;
    }
    LE_INFO("le_mdc_GetDNSAddresses called");
    LE_PRINT_VALUE("%s", dns1Addr);
    LE_PRINT_VALUE("%s", dns2Addr);

    resolvFilePtr = fopen("/var/run/resolv.conf", "w");
    if (resolvFilePtr == NULL)
    {
        LE_INFO("fopen on /var/run/resolv.conf failed");
        return NULL;
    }
    LE_INFO("fopen on /var/run/resolv.conf called");

    if ( (fprintf(resolvFilePtr, "nameserver %s\n", dns1Addr) < 0) ||
         (fprintf(resolvFilePtr, "nameserver %s\n", dns2Addr) < 0) )
    {
        LE_INFO("fprintf failed");
        return NULL;
    }
    LE_INFO("fprintf called");

    if ( fclose(resolvFilePtr) != 0 )
    {
        LE_INFO("fclose failed");
        return NULL;
    }
    LE_INFO("fclose called");

    // finally, test the data connection
    if ( system("ping -c 5 www.sierrawireless.com") != 0 )
    {
        LE_INFO("system ping failed");
        return NULL;
    }
    LE_INFO("system ping called");

    if ( le_mdc_StopSession(profileRef) != LE_OK )
    {
        LE_INFO("Stop failed");
        return NULL;
    }
    LE_INFO("Stop called");

    LE_INFO("ALL TESTS PASS");

    le_event_RunLoop();
    return NULL;
}


LE_EVENT_INIT_HANDLER
{
    // Note that this init should be done in the main thread, and in particular, should not be done
    // in the same thread as the tests.
    le_ms_Init();

    le_thread_Start(le_thread_Create("TestMain", TestStartDataSession, NULL));
}

