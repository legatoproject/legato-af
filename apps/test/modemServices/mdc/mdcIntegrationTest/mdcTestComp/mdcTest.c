/**
 * This module is for unit testing of the modemServices MDC component.
 *
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>

#include "interfaces.h"
#include "le_print.h"

static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;   // POSIX "Fast" mutex.

/// Locks the mutex.
#define LOCK    LE_ASSERT(pthread_mutex_lock(&Mutex) == 0);

/// Unlocks the mutex.
#define UNLOCK  LE_ASSERT(pthread_mutex_unlock(&Mutex) == 0);




static le_result_t UpdateResolvConf
(
    char* dns1AddrPtr,
    char* dns2AddrPtr
)
{
    FILE*  resolvFilePtr;
    mode_t oldMask;

    // allow fopen to create file with mode=644
    oldMask = umask(022);

    resolvFilePtr = fopen("/var/run/resolv.conf", "w");
    if (resolvFilePtr == NULL)
    {
        // restore old mask
        umask(oldMask);

        LE_INFO("fopen on /var/run/resolv.conf failed");
        return LE_FAULT;
    }
    LE_INFO("fopen on /var/run/resolv.conf called");

    if ( (fprintf(resolvFilePtr, "nameserver %s\n", dns1AddrPtr) < 0) ||
         (fprintf(resolvFilePtr, "nameserver %s\n", dns2AddrPtr) < 0) )
    {
        LE_INFO("fprintf failed");
        // restore old mask
        umask(oldMask);

        fclose(resolvFilePtr);
        return LE_FAULT;
    }
    LE_INFO("fprintf called");

    // restore old mask
    umask(oldMask);

    if ( fclose(resolvFilePtr) != 0 )
    {
        LE_INFO("fclose failed");
        return LE_FAULT;
    }
    LE_INFO("fclose called");

    return LE_OK;
}

static void StateChangeHandler
(
    bool  isConnected,
    void* contextPtr
)
{
    le_mdc_ProfileRef_t ProfileRef = contextPtr;
    char name[LE_MDC_INTERFACE_NAME_MAX_BYTES];

    le_mdc_GetInterfaceName(ProfileRef, name, sizeof(name));

    LE_DEBUG("\n================================================");
    LE_PRINT_VALUE("%s", name);
    LE_PRINT_VALUE("%u", isConnected);
    LE_PRINT_VALUE("%d", le_mdc_GetProfileIndex(ProfileRef));
    LE_DEBUG("\n================================================");
}


static bool TestIpv4Connectivity
(
    le_mdc_ProfileRef_t ProfileRef
)
{
    char interfaceName[100];
    char gatewayAddr[100];
    char ipAddr[100];
    char dns1Addr[100];
    char dns2Addr[100];
    char system_cmd[200];

    if ( !le_mdc_IsIPv4(ProfileRef) )
    {
        LE_INFO("The interface do not provide the Ipv4 connectivity");
        return false;
    }

    if ( le_mdc_GetInterfaceName(ProfileRef, interfaceName, sizeof(interfaceName)) != LE_OK )
    {
        LE_INFO("le_mdc_GetInterfaceName failed");
        return false;
    }

    LE_INFO("le_mdc_GetInterfaceName called");

    if (le_mdc_GetIPv4Address (ProfileRef, ipAddr, sizeof(ipAddr)) != LE_OK)
    {
        LE_INFO("le_mdc_GetIPv4Address failed");
        return false;
    }

    LE_INFO("le_mdc_GetIPv4Address called");

    LE_INFO("%s %s", interfaceName, ipAddr);

    if ( le_mdc_GetIPv4GatewayAddress(ProfileRef, gatewayAddr, sizeof(gatewayAddr)) != LE_OK )
    {
        LE_INFO("le_mdc_GetGatewayAddress failed");
        return false;
    }

    LE_INFO("le_mdc_GetGatewayAddress called");
    LE_PRINT_VALUE("%s", gatewayAddr);

    LE_INFO("waiting a few seconds before setting the route for the default gateway");
    sleep(5);

    LOCK
    snprintf(system_cmd, sizeof(system_cmd), "route add default gateway %s dev %s",
                                                                        gatewayAddr, interfaceName);
    if ( system(system_cmd) != 0 )
    {
        LE_INFO("system '%s' failed", system_cmd);
        UNLOCK
        return false;
    }
    LE_INFO("system '%s' called", system_cmd);


    if ( le_mdc_GetIPv4DNSAddresses(ProfileRef,
                                    dns1Addr, sizeof(dns1Addr),
                                    dns2Addr, sizeof(dns2Addr)) != LE_OK )
    {
        LE_INFO("le_mdc_GetDNSAddresses failed");
        UNLOCK
        return false;
    }

    LE_INFO("le_mdc_GetDNSAddresses called");
    LE_PRINT_VALUE("%s", dns1Addr);
    LE_PRINT_VALUE("%s", dns2Addr);

    if (UpdateResolvConf(dns1Addr, dns2Addr) != LE_OK)
    {
        UNLOCK
        return false;
    }

    // finally, test the data connection
    if ( system("ping -c 5 www.sierrawireless.com") != 0 )
    {
        LE_INFO("system ping failed");
        UNLOCK
        return false;
    }
    LE_INFO("system ping called");

    snprintf(system_cmd, sizeof(system_cmd), "route del default gw");

    if ( system(system_cmd) != 0 )
    {
        LE_INFO("system '%s' failed", system_cmd);
        UNLOCK
        return false;
    }

    UNLOCK

    return true;
}

static bool TestIpv6Connectivity
(
    le_mdc_ProfileRef_t ProfileRef
)
{
    char interfaceName[100];
    char gatewayAddr[100];
    char ipAddr[100];
    char dns1Addr[100];
    char dns2Addr[100];
    char system_cmd[200];

    if ( !le_mdc_IsIPv6(ProfileRef) )
    {
        LE_INFO("The interface do not provide the Ipv6 connectivity");
        return false;
    }

    if ( le_mdc_GetInterfaceName(ProfileRef, interfaceName, sizeof(interfaceName)) != LE_OK )
    {
        LE_INFO("le_mdc_GetInterfaceName failed");
        return false;
    }

    LE_INFO("le_mdc_GetInterfaceName called");

    if (le_mdc_GetIPv6Address (ProfileRef, ipAddr, sizeof(ipAddr)) != LE_OK)
    {
        LE_INFO("le_mdc_GetIPv6Address failed");
        return false;
    }

    LE_INFO("le_mdc_GetIPv6Address called");

    LE_INFO("%s %s", interfaceName, ipAddr);


    if ( le_mdc_GetIPv6GatewayAddress(ProfileRef, gatewayAddr, sizeof(gatewayAddr)) != LE_OK )
    {
        LE_INFO("le_mdc_GetGatewayAddress failed");
        return false;
    }

    LE_INFO("le_mdc_GetGatewayAddress called");
    LE_PRINT_VALUE("%s", gatewayAddr);

    LE_INFO("waiting a few seconds before setting the route for the default gateway");
    sleep(5);

    LOCK
    snprintf(system_cmd, sizeof(system_cmd), "route -A inet6 add default gw %s", gatewayAddr);
    if ( system(system_cmd) != 0 )
    {
        LE_INFO("system '%s' failed", system_cmd);
        UNLOCK
        return false;
    }
    LE_INFO("system '%s' called", system_cmd);

    if ( le_mdc_GetIPv6DNSAddresses(ProfileRef,
                                    dns1Addr, sizeof(dns1Addr),
                                    dns2Addr, sizeof(dns2Addr)) != LE_OK )
    {
        LE_INFO("le_mdc_GetDNSAddresses failed");
        UNLOCK
        return false;
    }
    LE_INFO("le_mdc_GetDNSAddresses called");
    LE_PRINT_VALUE("%s", dns1Addr);
    LE_PRINT_VALUE("%s", dns2Addr);

    if (UpdateResolvConf(dns1Addr, dns2Addr) != LE_OK)
    {
        UNLOCK
        return false;
    }

    // finally, test the data connection
    if ( system("ping6 -c 5 www.sierrawireless.com") != 0 )
    {
        LE_INFO("system ping failed");
        UNLOCK
        return false;
    }
    LE_INFO("system ping called");

    snprintf(system_cmd, sizeof(system_cmd), "route -A inet6 del default gw %s", gatewayAddr);

    if ( system(system_cmd) != 0 )
    {
        LE_INFO("system '%s' failed", system_cmd);
        UNLOCK
        return false;
    }

    UNLOCK

    return true;
}

static void* HandlerThread(void* contextPtr)
{
    le_mdc_ConnectService();

    le_mdc_ProfileRef_t profileRef = (le_mdc_ProfileRef_t) contextPtr;
    le_mdc_AddSessionStateHandler(profileRef, StateChangeHandler, profileRef);

    le_event_RunLoop();
}


static void* TestThread(void* contextPtr)
{
    le_mdc_ProfileRef_t profileRef = (le_mdc_ProfileRef_t) contextPtr;

    le_mdc_ConnectService();

    bool isConnected;

    if ((le_mdc_GetSessionState(profileRef, &isConnected) != LE_OK) || (isConnected != false))
    {
        LE_INFO("le_mdc_GetSessionState failed");
        return NULL;
    }

    if ( le_mdc_StartSession(profileRef) != LE_OK )
    {
        LE_INFO("Start failed");
        return NULL;
    }

    LE_INFO("Start called");

    LE_INFO("waiting a few seconds");
    sleep(10);

    // Check returned error code if Data session is already started
    LE_ASSERT(le_mdc_StartSession(profileRef)== LE_DUPLICATE);

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

    TestIpv4Connectivity(profileRef);

    TestIpv6Connectivity(profileRef);

    if ( le_mdc_StopSession(profileRef) != LE_OK )
    {
        LE_INFO("Stop failed");
        return NULL;
    }

    LE_INFO("Stop called");

    LE_INFO("TESTS PASS FOR PROFILE %d", le_mdc_GetProfileIndex(profileRef));

    return NULL;
}


COMPONENT_INIT
{
    // Hard coded, second profile.
    le_mdc_ProfileRef_t defaultProfileRef = le_mdc_GetProfile(LE_MDC_DEFAULT_PROFILE);

    if ( defaultProfileRef == NULL )
    {
        LE_INFO("load failed");
        exit(1);
        return;
    }
    LE_INFO("Load called");

    uint32_t defaultIndex = le_mdc_GetProfileIndex(defaultProfileRef);
    uint32_t secondaryIndex = defaultIndex + 1 % le_mdc_NumProfiles();
    char apn[50];
    memset(apn, 0, 50);

    le_mdc_ProfileRef_t secondaryProfileRef = le_mdc_GetProfile(secondaryIndex);

    // Start handler threads
    le_thread_Start(le_thread_Create("MDC1_handler", HandlerThread, defaultProfileRef));
    le_thread_Start(le_thread_Create("MDC2_handler", HandlerThread, secondaryProfileRef));
    sleep(1);

    // Start the test threads.
    le_thread_Start(le_thread_Create("MDC1_Test", TestThread, defaultProfileRef));
    le_thread_Start(le_thread_Create("MDC2_Test", TestThread, secondaryProfileRef));

}

