/**
 * This module is for unit testing of the modemServices MDC component.
 *
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include <stdio.h>
#include <pthread.h>

#include "interfaces.h"
#include "le_print.h"

uint8_t NbConnection = 1;
#define NB_CONNECTION_MAX 4

le_mdc_ProfileRef_t ProfileRef[NB_CONNECTION_MAX];
bool TaskStarted[NB_CONNECTION_MAX];

static le_mdc_MtPdpSessionStateHandlerRef_t MtPdpSessionStateHandlerRef;


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
    char systemCmd[200];

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
    LE_ASSERT(snprintf(systemCmd, sizeof(systemCmd), "route add default gateway %s dev %s",
                                                     gatewayAddr, interfaceName)
              <= sizeof(systemCmd));

    if ( system(systemCmd) != 0 )
    {
        LE_INFO("system '%s' failed", systemCmd);
        UNLOCK
        return false;
    }
    LE_INFO("system '%s' called", systemCmd);


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

    snprintf(systemCmd, sizeof(systemCmd), "route del default gw");

    if ( system(systemCmd) != 0 )
    {
        LE_INFO("system '%s' failed", systemCmd);
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
    char systemCmd[200];

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
    snprintf(systemCmd, sizeof(systemCmd), "route -A inet6 add default gw %s", gatewayAddr);
    if ( system(systemCmd) != 0 )
    {
        LE_INFO("system '%s' failed", systemCmd);
        UNLOCK
        return false;
    }
    LE_INFO("system '%s' called", systemCmd);

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

    snprintf(systemCmd, sizeof(systemCmd), "route -A inet6 del default gw %s", gatewayAddr);

    if ( system(systemCmd) != 0 )
    {
        LE_INFO("system '%s' failed", systemCmd);
        UNLOCK
        return false;
    }

    UNLOCK

    return true;
}



static void* TestThread(void* contextPtr)
{
    le_mdc_ProfileRef_t profileRef = (le_mdc_ProfileRef_t) contextPtr;

    le_mdc_ConnectService();

    le_mdc_ConState_t state;

    LOCK

    if ((le_mdc_GetSessionState(profileRef, &state) != LE_OK) || (state != LE_MDC_DISCONNECTED))
    {
        LE_INFO("le_mdc_GetSessionState failed (%d)", state);
        UNLOCK
        return NULL;
    }

    if ( le_mdc_StartSession(profileRef) != LE_OK )
    {
        LE_INFO("Start failed");
        UNLOCK
        return NULL;
    }

    UNLOCK

    LE_INFO("Start called");

    LE_INFO("waiting a few seconds");
    sleep(20*NbConnection);

    LOCK
    // Check returned error code if Data session is already started
    LE_INFO("Restart tested as duplicated");
    LE_ASSERT(le_mdc_StartSession(profileRef)== LE_DUPLICATE);

    if ( le_mdc_StopSession(profileRef) != LE_OK )
    {
        LE_INFO("Stop failed");
        UNLOCK
        return NULL;
    }

    UNLOCK

    LE_INFO("Stop called");

    // Wait a bit and then restart the data session and configure the network interface.

    LE_INFO("waiting a few more seconds");
    sleep(10*NbConnection);

    LOCK

    if ( le_mdc_StartSession(profileRef) != LE_OK )
    {
        LE_INFO("Start failed");
        return NULL;
    }

    LE_INFO("Start called");

    UNLOCK

    sleep(10*NbConnection);

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


static void* TestThreadMtPdp(void* contextPtr)
{
    le_mdc_ProfileRef_t profileRef = (le_mdc_ProfileRef_t) contextPtr;
    le_mdc_ConState_t state;
    le_result_t res;

    le_mdc_ConnectService();

    LOCK

    if ((le_mdc_GetSessionState(profileRef, &state) != LE_OK) || (state != LE_MDC_DISCONNECTED))
    {
        LE_ERROR("le_mdc_GetSessionState failed (%d)", state);
        UNLOCK
        return NULL;
    }

    if ( le_mdc_StartSession(profileRef) != LE_OK )
    {
        LE_INFO("Start failed");
        UNLOCK
        return NULL;
    }

    UNLOCK

    LE_INFO("Start called");

    /* Get Context information */
    char apn[10];
    le_mdc_Auth_t auth;
    char userName[10]={0};
    char password[10]={0};
    le_mdc_Pdp_t pdp;

    res = le_mdc_GetAPN(profileRef, apn, sizeof(apn));
    LE_INFO("le_mdc_GetAPN %d", res);
    res = le_mdc_GetAuthentication(profileRef,&auth, userName, sizeof(userName), password,
                                                                                sizeof(password));
    LE_INFO("le_mdc_GetAuthentication %d", res);
    pdp = le_mdc_GetPDP(profileRef);

    LE_INFO("MT-PDP APN: %s", apn);
    LE_INFO("MT-PDP PDP type: %d", pdp);
    LE_INFO("MT-PDP Authentication: %d", auth);
    LE_INFO("MT-PDP userName: %s", userName);
    LE_INFO("MT-PDP password: %s", password);


    LE_INFO("waiting a few seconds");
    sleep(20);

    LOCK

    LE_INFO("Restart tested as duplicated");
    // Check returned error code if Data session is already started
    LE_ASSERT(le_mdc_StartSession(profileRef) == LE_DUPLICATE);

    LE_INFO("waiting a few seconds");
    sleep(10);

    if ( le_mdc_StopSession(profileRef) != LE_OK )
    {
        LE_INFO("Stop failed");
        UNLOCK
        return NULL;
    }

    UNLOCK
    LE_INFO("Stop called");

    LE_INFO("TESTS PASS FOR MT-PDP PROFILE %d", le_mdc_GetProfileIndex(profileRef));

    return NULL;
}


static void StateChangeHandler
(
    le_mdc_ProfileRef_t profileRef,
    le_mdc_ConState_t ConnectionStatus,
    void* contextPtr
)
{
    intptr_t profileIndex = (intptr_t) contextPtr;

    char name[LE_MDC_INTERFACE_NAME_MAX_BYTES];

    le_mdc_GetInterfaceName(profileRef, name, sizeof(name));

    LE_DEBUG("\n================================================");
    LE_PRINT_VALUE("%d", (int) le_mdc_GetProfileIndex(profileRef));
    LE_PRINT_VALUE("%s", name);
    LE_PRINT_VALUE("%u", ConnectionStatus);

    if (ConnectionStatus == LE_MDC_DISCONNECTED)
    {
        if (LE_MDC_PDP_IPV4V6 == le_mdc_GetPDP(profileRef))
        {
            LE_PRINT_VALUE("%d", le_mdc_GetDisconnectionReasonExt(profileRef, LE_MDC_PDP_IPV4));
            LE_PRINT_VALUE("%d", le_mdc_GetPlatformSpecificDisconnectionCodeExt(profileRef,
                                                                                LE_MDC_PDP_IPV4));
            LE_PRINT_VALUE("%d", le_mdc_GetDisconnectionReasonExt(profileRef, LE_MDC_PDP_IPV6));
            LE_PRINT_VALUE("%d", le_mdc_GetPlatformSpecificDisconnectionCodeExt(profileRef,
                                                                                LE_MDC_PDP_IPV6));
        }
        else
        {
            LE_PRINT_VALUE("%d", le_mdc_GetDisconnectionReasonExt(profileRef, 0));
            LE_PRINT_VALUE("%d", le_mdc_GetPlatformSpecificDisconnectionCodeExt(profileRef, 0));
        }
    }
    LE_DEBUG("\n================================================");

    if ((ConnectionStatus == LE_MDC_CONNECTED) &&
        !(TaskStarted[profileIndex+1]) && (profileIndex < (NbConnection-1)))
    {
        char string[50]="\0";
        snprintf(string,50,"MDC%d_Test", (int) profileIndex+2);

        LE_INFO("Start %s", string);

        le_thread_Start(le_thread_Create(string, TestThread, ProfileRef[profileIndex+1]));
        TaskStarted[profileIndex+1] = true;
    }
}

static void StateChangeHandlerMtPdp
(
    le_mdc_ProfileRef_t profileRef,
    le_mdc_ConState_t ConnectionStatus,
    void* contextPtr
)
{
    char name[LE_MDC_INTERFACE_NAME_MAX_BYTES];

    le_mdc_GetInterfaceName(profileRef, name, sizeof(name));

    LE_DEBUG("\n====================MT-PDP============================");
    LE_PRINT_VALUE("%d", (int) le_mdc_GetProfileIndex(profileRef));
    LE_PRINT_VALUE("%s", name);
    LE_PRINT_VALUE("%u", ConnectionStatus);

    if (ConnectionStatus == LE_MDC_INCOMING)
    {
        LE_INFO("MT-PDP request received for Profile %d", le_mdc_GetProfileIndex(profileRef));
        // Start test thread for MT-PDP request
        le_thread_Start(le_thread_Create("MDC_MT-PDP_Test", TestThreadMtPdp, profileRef));
        // LE_ASSERT( le_mdc_StartSession(profileRef) == LE_OK )
    }

    if (ConnectionStatus == LE_MDC_CONNECTED)
    {
        LE_INFO("MT-PDP connected for Profile %d", le_mdc_GetProfileIndex(profileRef));
    }

    if (ConnectionStatus == LE_MDC_DISCONNECTED)
    {
        if (LE_MDC_PDP_IPV4V6 == le_mdc_GetPDP(profileRef))
        {
            LE_PRINT_VALUE("%d", le_mdc_GetDisconnectionReasonExt(profileRef, LE_MDC_PDP_IPV4));
            LE_PRINT_VALUE("%d", le_mdc_GetPlatformSpecificDisconnectionCodeExt(profileRef,
                                                                                LE_MDC_PDP_IPV4));
            LE_PRINT_VALUE("%d", le_mdc_GetDisconnectionReasonExt(profileRef, LE_MDC_PDP_IPV6));
            LE_PRINT_VALUE("%d", le_mdc_GetPlatformSpecificDisconnectionCodeExt(profileRef,
                                                                                LE_MDC_PDP_IPV6));
        }
        else
        {
            LE_PRINT_VALUE("%d", le_mdc_GetDisconnectionReasonExt(profileRef, 0));
            LE_PRINT_VALUE("%d", le_mdc_GetPlatformSpecificDisconnectionCodeExt(profileRef, 0));
        }
        // Remove handler
        le_mdc_RemoveMtPdpSessionStateHandler(MtPdpSessionStateHandlerRef);
    }

    LE_DEBUG("\n================================================");
}

static void* HandlerThread(void* contextPtr)
{
    le_mdc_ConnectService();

    le_mdc_ProfileRef_t profileRef = ProfileRef[(intptr_t) contextPtr];
    le_mdc_AddSessionStateHandler(profileRef, StateChangeHandler, contextPtr);

    le_event_RunLoop();
}

static void* HandlerThreadMtPdP(void* contextPtr)
{
    le_mdc_ConnectService();

    LE_INFO("AddMtPdpSessionStateHandler");

    MtPdpSessionStateHandlerRef = le_mdc_AddMtPdpSessionStateHandler(StateChangeHandlerMtPdp
                                                                    , contextPtr);

    le_event_RunLoop();
}

COMPONENT_INIT
{

    uint32_t defaultIndex;
    intptr_t i;

    if (le_arg_NumArgs() > 0)
    {
        const char* nbConnectionPtr = le_arg_GetArg(0);
        if (NULL == nbConnectionPtr)
        {
            LE_ERROR("nbConnectionPtr is NULL");
            exit(EXIT_FAILURE);
        }
        NbConnection = atoi(nbConnectionPtr);
    }

    LE_INFO("Nb connection %d", NbConnection);

    LE_ASSERT(NbConnection <= NB_CONNECTION_MAX);

    for (i=0; i < NbConnection; i++)
    {
        TaskStarted[i] = false;

        if (i==0)
        {
            ProfileRef[0]  = le_mdc_GetProfile(LE_MDC_DEFAULT_PROFILE);
            defaultIndex = le_mdc_GetProfileIndex(ProfileRef[0]);
        }
        else
        {
            ProfileRef[i] = le_mdc_GetProfile(defaultIndex + i % le_mdc_NumProfiles());
        }

        if ( ProfileRef[i] == NULL )
        {
            LE_INFO("load failed");
            exit(1);
            return;
        }

        char string[50]="\0";
        snprintf(string,50,"MDC%d_handler", (int) (i+1));
        le_thread_Start(le_thread_Create(string, HandlerThread, (void*) i));

        snprintf(string,50,"MDC%d_handlerMtPdP", (int) (i+1));
        le_thread_Start(le_thread_Create(string, HandlerThreadMtPdP, (void*) i));

    }

    sleep(1);

    // Start the first test thread.
    le_thread_Start(le_thread_Create("MDC1_Test", TestThread, ProfileRef[0]));
    TaskStarted[0] = true;

}

