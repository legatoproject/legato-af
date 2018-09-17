/**
 * This module implements the unit tests for MDC API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "le_mdc_local.h"
#include "le_sim_local.h"
#include "log.h"
#include "pa_mdc.h"
#include "pa_mdc_simu.h"
#include "pa_mrc_simu.h"
#include "pa_sim_simu.h"
#include "le_cfg_simu.h"

#define NB_PROFILE  5
#define IP_STR_SIZE     16

typedef void (*StartStopAsyncFunc_t) (le_mdc_ProfileRef_t,le_mdc_SessionHandlerFunc_t,void*);

static le_sem_Ref_t  ThreadSemaphore;
static le_sem_Ref_t  SimRefreshSemaphore;
static le_mdc_ProfileRef_t ProfileRef[NB_PROFILE];
static le_mdc_SessionStateHandlerRef_t SessionStateHandler[NB_PROFILE];
static le_mdc_ProfileRef_t ProfileRefReceivedByHandler = NULL;
static bool ConnectionStateReceivedByHandler = false;

//--------------------------------------------------------------------------------------------------
/**
 * Server Service Reference
 */
//--------------------------------------------------------------------------------------------------
static le_msg_ServiceRef_t _ServerServiceRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Client Session Reference for the current message received from a client
 */
//--------------------------------------------------------------------------------------------------
static le_msg_SessionRef_t _ClientSessionRef = NULL;

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_mrc_GetServiceRef
(
    void
)
{
    return _ServerServiceRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_mrc_GetClientSessionRef
(
    void
)
{
    return _ClientSessionRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the server service reference (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_ServiceRef_t le_sim_GetServiceRef
(
    void
)
{
    return _ServerServiceRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the client session reference for the current message (STUBBED FUNCTION)
 */
//--------------------------------------------------------------------------------------------------
le_msg_SessionRef_t le_sim_GetClientSessionRef
(
    void
)
{
    return _ClientSessionRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * SIM Refresh handler: this handler is called on STK event
 *
 */
//--------------------------------------------------------------------------------------------------
static void SimRefreshHandler
(
    le_sim_Id_t simId,
    le_sim_StkEvent_t stkEvent,
    void* contextPtr
)
{
    LE_INFO("SIM refresh performed");
    le_sem_Post(SimRefreshSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Add a handler to monitor SIM refresh events
 *
 */
//--------------------------------------------------------------------------------------------------
static void InitSimRefresh
(
    void
)
{
    // Create a semaphore to coordinate the test when SIM is refreshed
    SimRefreshSemaphore = le_sem_Create("SimRefreshSem", 0);

    le_sim_AddSimToolkitEventHandler(SimRefreshHandler, NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function should be called to trigger a refresh in le_sim side when a SIM information has
 * changed in PA level
 *
 */
//--------------------------------------------------------------------------------------------------
static void TriggerSimRefresh
(
    void
)
{
    pa_simSimu_SetRefreshMode(LE_SIM_REFRESH_FCN);
    pa_simSimu_SetRefreshStage(LE_SIM_STAGE_END_WITH_SUCCESS);
    pa_simSimu_ReportSTKEvent(LE_SIM_REFRESH);
    le_sem_Wait(SimRefreshSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * The goal of this test is to:
 * - Allocate profile.
 * - Test the configuration of profiles.
 *
 * API tested:
 * - le_mdc_GetProfile
 * - le_mdc_GetProfileFromApn
 * - le_mdc_GetProfileIndex
 * - le_mdc_GetAuthentication / le_mdc_SetAuthentication
 * - le_mdc_GetPDP / le_mdc_SetPDP
 * - le_mdc_GetAPN / le_mdc_SetAPN
 * - le_mdc_SetDefaultAPN
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void TestMdc_Configuration( void )
{
    pa_mrcSimu_SetRadioAccessTechInUse(LE_MRC_RAT_GSM);

    pa_mdc_ProfileData_t profileData;
    char newAPN[] = "NewAPN";
    int i;

    /* Configure the pa_mdcSimu */
    for (i = 0; i < NB_PROFILE; i++)
    {
        char tstAPN[10];
        snprintf(tstAPN,10,"TstAPN%d", i);
        memset(&profileData,0,sizeof(pa_mdc_ProfileData_t));
        strcpy( &profileData.apn[0], tstAPN);
        profileData.authentication.type = LE_MDC_AUTH_NONE;
        profileData.pdp = LE_MDC_PDP_IPV4;
        pa_mdcSimu_SetProfile(i+1, &profileData);
    }

    /* Allocate profiles: use le_mdc_GetProfileFromApn or le_mdc_GetProfile */
    for (i = 0; i < NB_PROFILE; i++)
    {
        char tstAPN[10];
        snprintf(tstAPN,10,"TstAPN%d", i);

        if ( (i/2)*2 == i )
        {
            LE_ASSERT_OK(le_mdc_GetProfileFromApn(tstAPN, &ProfileRef[i]));
        }
        else
        {
            ProfileRef[i] = le_mdc_GetProfile(i+1);
        }

        /* expected value: ProfileRef not null */
        LE_ASSERT(ProfileRef[i] != NULL);

        /* Check the index*/
        LE_ASSERT((i+1) == le_mdc_GetProfileIndex(ProfileRef[i]));
    }

    /* Map profile on network interface */
    LE_ASSERT(le_mdc_MapProfileOnNetworkInterface(ProfileRef[0], "rmnet_data0") == LE_OK);

    /* Get and change APN of 1st profile */
    char apn[30];
    LE_ASSERT_OK(le_mdc_GetAPN(ProfileRef[0], apn, sizeof(apn)));
    LE_ASSERT(0 == strcmp("TstAPN0", apn));
    LE_ASSERT_OK(le_mdc_SetAPN(ProfileRef[0], newAPN));
    LE_ASSERT_OK(le_mdc_GetAPN(ProfileRef[0], apn, sizeof(apn)));
    LE_ASSERT(0 == strcmp(newAPN, apn));

    /* Check to get a profile thanks to its APN */
    le_mdc_ProfileRef_t profile;
    LE_ASSERT(LE_NOT_FOUND == le_mdc_GetProfileFromApn("TstAPN0", &profile));
    LE_ASSERT_OK(le_mdc_GetProfileFromApn(newAPN, &profile));
    LE_ASSERT(profile == ProfileRef[0]);

    /* Get and change authentication */
    le_mdc_Auth_t auth;
    char userName[10];
    char password[10];
    char myUserName[]="myName";
    char myPassword[]="myPwd";
    LE_ASSERT_OK(le_mdc_GetAuthentication(ProfileRef[0], &auth, userName, sizeof(userName),
                                          password, sizeof(password)));
    LE_ASSERT(LE_MDC_AUTH_NONE == auth);
    LE_ASSERT_OK(le_mdc_SetAuthentication(ProfileRef[0], LE_MDC_AUTH_PAP,
                                          myUserName, myPassword));
    LE_ASSERT_OK(le_mdc_GetAuthentication(ProfileRef[0], &auth, userName, sizeof(userName),
                                          password, sizeof(password)));
    LE_ASSERT(LE_MDC_AUTH_PAP == auth);
    LE_ASSERT(0 == strcmp(userName, myUserName));
    LE_ASSERT(0 == strcmp(password, myPassword));

    /* Get PDP type and change it */
    LE_ASSERT(LE_MDC_PDP_IPV4 == le_mdc_GetPDP(ProfileRef[0]));
    LE_ASSERT_OK(le_mdc_SetPDP(ProfileRef[0], LE_MDC_PDP_IPV6));
    LE_ASSERT(LE_MDC_PDP_IPV6 == le_mdc_GetPDP(ProfileRef[0]));

    /* start a session: profile can't be modified when a session is activated */
    LE_ASSERT_OK(le_mdc_StartSession(ProfileRef[0]));

    /* Try to set APN (error is expected as a connection is in progress) */
    LE_ASSERT(LE_FAULT == le_mdc_SetAPN(ProfileRef[0], "TstAPN0"));
    /* Get is possible */
    LE_ASSERT_OK(le_mdc_GetAPN(ProfileRef[0], apn, sizeof(apn)));
    LE_ASSERT(0 == strcmp(newAPN, apn));

    /* Try to set authentication (error is expected as a connection is in progress) */
    LE_ASSERT(LE_FAULT == le_mdc_SetAuthentication(ProfileRef[0], LE_MDC_AUTH_CHAP,
                                                   myUserName, myPassword));
    /* Get is possible */
    LE_ASSERT_OK(le_mdc_GetAuthentication(ProfileRef[0], &auth, userName, sizeof(userName),
                                          password, sizeof(password)));
    LE_ASSERT(LE_MDC_AUTH_PAP == auth);
    LE_ASSERT(0 == strcmp(userName, myUserName));
    LE_ASSERT(0 == strcmp(password, myPassword));

    /* Try to set PDP type (error is expected as a connection is in progress) */
    LE_ASSERT(LE_FAULT == le_mdc_SetPDP(ProfileRef[0], LE_MDC_PDP_IPV4V6));
    /* Get is possible */
    LE_ASSERT(LE_MDC_PDP_IPV6 == le_mdc_GetPDP(ProfileRef[0]));

    /* Check that other profiles didn't change */
    for (i = 1; i < NB_PROFILE; i++)
    {
        char tstAPN[10];
        snprintf(tstAPN,10,"TstAPN%d", i);

        /* Check APN */
        LE_ASSERT_OK(le_mdc_GetAPN(ProfileRef[i], apn, sizeof(apn)));
        LE_ASSERT(0 == strcmp(tstAPN, apn));

        /* Check auth */
        LE_ASSERT_OK(le_mdc_GetAuthentication(ProfileRef[i], &auth, userName, sizeof(userName),
                                              password, sizeof(password)));
        LE_ASSERT(LE_MDC_AUTH_NONE == auth);

        /* Check PDP type */
        LE_ASSERT(LE_MDC_PDP_IPV4 == le_mdc_GetPDP(ProfileRef[i]));

        /* Check to get a profile thanks to its APN */
        LE_ASSERT_OK(le_mdc_GetProfileFromApn(tstAPN, &profile));
        LE_ASSERT(profile == ProfileRef[i]);
    }

    /* stop the session */
    LE_ASSERT_OK(le_mdc_StopSession(ProfileRef[0]));

    /* Test default APNs */
    char homeMcc[] = "000";
    char homeMnc[] = "00";
    pa_simSimu_ReportSIMState(LE_SIM_READY);
    pa_simSimu_SetHomeNetworkMccMnc(homeMcc, homeMnc);
    pa_simSimu_SetCardIdentification("");
    TriggerSimRefresh();

    LE_ASSERT(LE_FAULT == le_mdc_SetDefaultAPN(ProfileRef[2]));

    /* Set default APN based on MCC and MNC */
    strcpy(homeMcc, "208");
    strcpy(homeMnc, "01");
    pa_simSimu_SetHomeNetworkMccMnc(homeMcc, homeMnc);
    LE_ASSERT_OK(le_mdc_SetDefaultAPN(ProfileRef[2]));
    /* Check APN */
    LE_ASSERT_OK(le_mdc_GetAPN(ProfileRef[2], apn, sizeof(apn)));
    LE_ASSERT(0 == strcmp("orange", apn));

    /* Set default APN based on ICCID, MCC and MNC */
    char iccid[] = "89332422217010081060";

    pa_simSimu_SetCardIdentification(iccid);
    TriggerSimRefresh();

    LE_ASSERT_OK(le_mdc_SetDefaultAPN(ProfileRef[2]));
    /* Check APN */
    LE_ASSERT_OK(le_mdc_GetAPN(ProfileRef[2], apn, sizeof(apn)));
    LE_ASSERT(0 == strcmp("internet.sierrawireless.com", apn));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function checks if the given profileRef is disconnected by testing IP APIs
 * (used by TestMdc_Connection test).
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void DisconnectedProfile
(
    le_mdc_ProfileRef_t profileRef
)
{
    char ipAddrStr[IP_STR_SIZE];
    char dns1AddrStr[IP_STR_SIZE];
    char dns2AddrStr[IP_STR_SIZE];
    char gatewayAddrStr[IP_STR_SIZE];
    char interfaceName[10];

    /* Expected value: LE_FAULT as the frofile is supposed to be disconnected */

    LE_ASSERT(le_mdc_GetInterfaceName(profileRef, interfaceName, sizeof(interfaceName))
                                                                                    == LE_FAULT);
    LE_ASSERT(le_mdc_GetIPv4Address(profileRef, ipAddrStr, IP_STR_SIZE) == LE_FAULT);
    LE_ASSERT(le_mdc_GetIPv6Address(profileRef, ipAddrStr, IP_STR_SIZE) == LE_FAULT);
    LE_ASSERT(le_mdc_GetIPv4DNSAddresses(   profileRef,
                                            dns1AddrStr,
                                            IP_STR_SIZE,
                                            dns2AddrStr,
                                            IP_STR_SIZE) == LE_FAULT);
    LE_ASSERT(le_mdc_GetIPv6DNSAddresses(   profileRef,
                                            dns1AddrStr,
                                            IP_STR_SIZE,
                                            dns2AddrStr,
                                            IP_STR_SIZE) == LE_OVERFLOW);
    LE_ASSERT(le_mdc_GetIPv4GatewayAddress( profileRef, gatewayAddrStr, IP_STR_SIZE )
                                                                                    == LE_FAULT);
    LE_ASSERT(le_mdc_GetIPv6GatewayAddress( profileRef, gatewayAddrStr, IP_STR_SIZE )
                                                                                    == LE_FAULT);

}

//--------------------------------------------------------------------------------------------------
/**
 * The goal of this test is to test the IP adresses APIs (for IPv4, IPv6, IPv4v6)
 * API tested:
 * - le_mdc_GetSessionState
 * - le_mdc_StartSession / le_mdc_StopSession
 * - le_mdc_IsIPv4 / le_mdc_IsIPv6
 * - le_mdc_GetIPv4Address / le_mdc_GetIPv6Address
 * - le_mdc_GetIPv4DNSAddresses / le_mdc_GetIPv6DNSAddresses
 * - le_mdc_GetIPv4GatewayAddress / le_mdc_GetIPv6GatewayAddress
 *
 * Exit if failed
 */
//--------------------------------------------------------------------------------------------------
static void TestMdc_Connection ( void )
{
    int i;
    le_result_t res;
    le_mdc_Pdp_t pdp = LE_MDC_PDP_IPV4;
    char ipAddrStrIpv4[]="192.168.1.100";
    char dns1AddrStrIpv4[]="10.40.50.1";
    char dns2AddrStrIpv4[]="10.40.50.2";
    char gatewayAddrStrIpv4[]="192.168.100.123";
    char ipAddrStrIpv6[]="2001:0000:3238:DFE1:63::FEFB";
    char dns1AddrStrIpv6[]="2001:4860:4860::8888";
    char dns2AddrStrIpv6[]="2001:4860:4860::8844";
    char gatewayAddrStrIpv6[]="2001:CDBA:0:0:0:0:3257:9652";
    char interfaceName[]="rmnet0";
    char addr[LE_MDC_IPV6_ADDR_MAX_BYTES]="";
    char addr2[LE_MDC_IPV6_ADDR_MAX_BYTES]="";

    /* All profile are disconnected: check connectivities API returns LE_FAULT */
    for (i=0; i < NB_PROFILE; i++)
    {
        DisconnectedProfile(ProfileRef[i]);
    }

    /* Test for all IP type: IPv4, IPv6, IPv4v6 */
    while(pdp <= LE_MDC_PDP_IPV4V6)
    {
        /* check connection status: supposed to be disconnected */
        le_mdc_ConState_t state;
        res = le_mdc_GetSessionState(ProfileRef[0], &state);
        LE_ASSERT(res == LE_OK);
        LE_ASSERT(state == LE_MDC_DISCONNECTED);

        /* Set the new pdp type */
        res = le_mdc_SetPDP(ProfileRef[0], pdp);
        LE_ASSERT(res == LE_OK);

        /* start a session*/
        res = le_mdc_StartSession(ProfileRef[0]);
        LE_ASSERT(res == LE_OK);

        /* Check connection status: supposed to be connected */
        res = le_mdc_GetSessionState(ProfileRef[0], &state);
        LE_ASSERT(res == LE_OK);
        LE_ASSERT(state == LE_MDC_CONNECTED);

        /* check connection status for other profile (supposed to be disconnected) */
        for (i = 1; i < NB_PROFILE; i++)
        {
            res = le_mdc_GetSessionState(ProfileRef[i], &state);
            LE_ASSERT(res == LE_OK);
            LE_ASSERT(state == LE_MDC_DISCONNECTED);

            /* check connectivty parameters */
            DisconnectedProfile(ProfileRef[i]);
        }

        /* configure an interfacen ame in the pa_mdcSimu, and test the API */
        char interfaceNameTmp[20];
        pa_mdcSimu_SetInterfaceName(1, interfaceName);
        LE_ASSERT(le_mdc_GetInterfaceName(ProfileRef[0],interfaceNameTmp,sizeof(interfaceNameTmp))
                                                                                        == LE_OK);
        LE_ASSERT( strcmp(interfaceNameTmp, interfaceName) == 0);

        /* Check IP type */
        switch (pdp)
        {
            case LE_MDC_PDP_IPV4:
                /* configure the pa_mdcSimu with IPv4 addresses */
                pa_mdcSimu_SetIPAddress(1, LE_MDMDEFS_IPV4, ipAddrStrIpv4);
                pa_mdcSimu_SetDNSAddresses(1, LE_MDMDEFS_IPV4, dns1AddrStrIpv4, dns2AddrStrIpv4);
                pa_mdcSimu_SetGatewayAddress(1, LE_MDMDEFS_IPV4, gatewayAddrStrIpv4);

                /* Check IPv4 and IPv6 connectivities:
                 * all IPv6 APIs return an error
                 * IPv4 APIs return expected value
                 */
                LE_ASSERT(le_mdc_IsIPv4(ProfileRef[0]) == true);
                LE_ASSERT(le_mdc_IsIPv6(ProfileRef[0]) == false);

                LE_ASSERT(le_mdc_GetIPv4Address(ProfileRef[0], addr, sizeof(addr) ) == LE_OK);
                LE_ASSERT( strcmp(addr,ipAddrStrIpv4)==0 );
                LE_ASSERT(le_mdc_GetIPv6Address(ProfileRef[0], addr, sizeof(addr) ) == LE_FAULT);
                LE_ASSERT(le_mdc_GetIPv4DNSAddresses(ProfileRef[0], addr,
                                                                    sizeof(addr),
                                                                    addr2,
                                                                    sizeof(addr) ) == LE_OK);
                LE_ASSERT(le_mdc_GetIPv4DNSAddresses(ProfileRef[0], addr,
                                                                    3,
                                                                    addr2,
                                                                    sizeof(addr) ) == LE_OVERFLOW);
                LE_ASSERT(le_mdc_GetIPv4DNSAddresses(ProfileRef[0], addr,
                                                                    sizeof(addr),
                                                                    addr2,
                                                                    3 ) == LE_OVERFLOW);

                LE_ASSERT( strcmp(addr,dns1AddrStrIpv4)==0 );
                LE_ASSERT( strcmp(addr2,dns2AddrStrIpv4)==0 );
                LE_ASSERT(le_mdc_GetIPv6DNSAddresses(ProfileRef[0], addr,
                                                                    sizeof(addr),
                                                                    addr2,
                                                                    sizeof(addr2) ) == LE_FAULT);

                LE_ASSERT(le_mdc_GetIPv6DNSAddresses(ProfileRef[0], addr,
                                                                    5,
                                                                    addr2,
                                                                    sizeof(addr2) ) == LE_OVERFLOW);

                LE_ASSERT(le_mdc_GetIPv6DNSAddresses(ProfileRef[0], addr,
                                                                    sizeof(addr),
                                                                    addr2,
                                                                    5 ) == LE_OVERFLOW);

                LE_ASSERT(le_mdc_GetIPv4GatewayAddress(ProfileRef[0], addr, sizeof(addr)) == LE_OK);
                LE_ASSERT( strcmp(addr,gatewayAddrStrIpv4)==0 );
                LE_ASSERT(le_mdc_GetIPv6GatewayAddress(ProfileRef[0], addr, sizeof(addr)) ==
                                                                                        LE_FAULT);
            break;
            case LE_MDC_PDP_IPV6:
                /* configure the pa_mdcSimu with IPv6 addresses */
                pa_mdcSimu_SetIPAddress(1, LE_MDMDEFS_IPV6, ipAddrStrIpv6);
                pa_mdcSimu_SetDNSAddresses(1, LE_MDMDEFS_IPV6, dns1AddrStrIpv6, dns2AddrStrIpv6);
                pa_mdcSimu_SetGatewayAddress(1, LE_MDMDEFS_IPV6, gatewayAddrStrIpv6);

                /* Check IPv4 and IPv6 connectivities:
                 * all IPv4 APIs return an error
                 * IPv6 APIs return expected value
                 */
                LE_ASSERT(le_mdc_IsIPv4(ProfileRef[0]) == false);
                LE_ASSERT(le_mdc_IsIPv6(ProfileRef[0]) == true);

                LE_ASSERT(le_mdc_GetIPv6Address(ProfileRef[0], addr, sizeof(addr) ) == LE_OK);
                LE_ASSERT( strcmp(addr,ipAddrStrIpv6)==0 );
                LE_ASSERT(le_mdc_GetIPv4Address(ProfileRef[0], addr, sizeof(addr) ) == LE_FAULT);
                LE_ASSERT(le_mdc_GetIPv6DNSAddresses(ProfileRef[0], addr,
                                                                    sizeof(addr),
                                                                    addr2,
                                                                    sizeof(addr2) ) == LE_OK);
                LE_ASSERT( strcmp(addr,dns1AddrStrIpv6)==0 );
                LE_ASSERT( strcmp(addr2,dns2AddrStrIpv6)==0 );
                LE_ASSERT(le_mdc_GetIPv4DNSAddresses(ProfileRef[0], addr,
                                                                    sizeof(addr),
                                                                    addr2,
                                                                    sizeof(addr2) ) == LE_FAULT);
                LE_ASSERT(le_mdc_GetIPv6GatewayAddress(ProfileRef[0], addr, sizeof(addr)) == LE_OK);
                LE_ASSERT( strcmp(addr,gatewayAddrStrIpv6)==0 );
                LE_ASSERT(le_mdc_GetIPv4GatewayAddress(ProfileRef[0], addr, sizeof(addr)) ==
                                                                                        LE_FAULT);
            break;
            case LE_MDC_PDP_IPV4V6:
                /* Check IPv4 and IPv6 connectivities:
                 * IPv4 and IPv6 APIs return expected value
                 */
                LE_ASSERT(le_mdc_IsIPv4(ProfileRef[0]) == true);
                LE_ASSERT(le_mdc_IsIPv6(ProfileRef[0]) == true);

                LE_ASSERT(le_mdc_GetIPv6Address(ProfileRef[0], addr, sizeof(addr) ) == LE_OK);
                LE_ASSERT( strcmp(addr,ipAddrStrIpv6) == 0 );
                LE_ASSERT(le_mdc_GetIPv4Address(ProfileRef[0], addr, sizeof(addr) ) == LE_OK);
                LE_ASSERT( strcmp(addr,ipAddrStrIpv4) == 0 );
                LE_ASSERT(le_mdc_GetIPv6DNSAddresses(ProfileRef[0], addr,
                                                                    sizeof(addr),
                                                                    addr2,
                                                                    sizeof(addr2) ) == LE_OK);
                LE_ASSERT( strcmp(addr,dns1AddrStrIpv6) == 0 );
                LE_ASSERT( strcmp(addr2,dns2AddrStrIpv6) == 0 );
                LE_ASSERT(le_mdc_GetIPv4DNSAddresses(ProfileRef[0], addr,
                                                                    sizeof(addr),
                                                                    addr2,
                                                                    sizeof(addr2) ) == LE_OK);
                LE_ASSERT( strcmp(addr,dns1AddrStrIpv4) == 0 );
                LE_ASSERT( strcmp(addr2,dns2AddrStrIpv4) == 0 );
                LE_ASSERT(le_mdc_GetIPv6GatewayAddress(ProfileRef[0], addr, sizeof(addr)) == LE_OK);
                LE_ASSERT( strcmp(addr,gatewayAddrStrIpv6) == 0 );
                LE_ASSERT(le_mdc_GetIPv4GatewayAddress(ProfileRef[0], addr, sizeof(addr)) ==
                                                                                        LE_OK);
                LE_ASSERT( strcmp(addr,gatewayAddrStrIpv4) == 0 );
            break;
            default:
                LE_ASSERT(0);
            break;
        }

        /* stop the session */
        res = le_mdc_StopSession(ProfileRef[0]);
        LE_ASSERT(res == LE_OK);

        /* next pdp type */
        pdp++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Test disconnection reason for a specific session
 */
//--------------------------------------------------------------------------------------------------
static void CheckDisconnectionReason
(
    le_mdc_ProfileRef_t profileRef,
    le_mdc_Pdp_t pdpType,
    bool isDualPdpProfile
)
{
    int32_t failureType, failureCode;

    // Test le_mdc_GetDisconnectionReasonExt() API
    LE_ASSERT(LE_MDC_DISC_UNDEFINED == le_mdc_GetDisconnectionReasonExt(NULL, pdpType));

    if (isDualPdpProfile)
    {
        LE_ASSERT(LE_MDC_DISC_UNDEFINED ==
                  le_mdc_GetDisconnectionReasonExt(profileRef, LE_MDC_PDP_UNKNOWN));
    }
    else
    {
        LE_ASSERT(LE_MDC_DISC_REGULAR_DEACTIVATION ==
                  le_mdc_GetDisconnectionReasonExt(profileRef, LE_MDC_PDP_UNKNOWN));
    }

    LE_ASSERT(LE_MDC_DISC_REGULAR_DEACTIVATION ==
              le_mdc_GetDisconnectionReasonExt(profileRef, pdpType));

    // Test le_mdc_GetPlatformSpecificDisconnectionCodeExt() API
    LE_ASSERT(INT32_MAX == le_mdc_GetPlatformSpecificDisconnectionCodeExt(NULL, pdpType));

    if (isDualPdpProfile)
    {
        LE_ASSERT(INT32_MAX ==
                  le_mdc_GetPlatformSpecificDisconnectionCodeExt(profileRef, LE_MDC_PDP_UNKNOWN));
    }
    else
    {
        LE_ASSERT(LE_MDC_END_FAILURE_CODE ==
                  le_mdc_GetPlatformSpecificDisconnectionCodeExt(profileRef, LE_MDC_PDP_UNKNOWN));
    }

    LE_ASSERT(LE_MDC_END_FAILURE_CODE ==
              le_mdc_GetPlatformSpecificDisconnectionCodeExt(profileRef, pdpType));


    // Test le_mdc_GetPlatformSpecificFailureConnectionReasonExt() API
    le_mdc_GetPlatformSpecificFailureConnectionReasonExt(NULL, pdpType, &failureType, &failureCode);
    LE_ASSERT(LE_MDC_DISC_UNDEFINED == failureType);
    LE_ASSERT(INT32_MAX == failureCode);

    le_mdc_GetPlatformSpecificFailureConnectionReasonExt(profileRef, pdpType, NULL, &failureCode);
    LE_ASSERT(INT32_MAX == failureCode);

    le_mdc_GetPlatformSpecificFailureConnectionReasonExt(profileRef, pdpType, &failureType, NULL);
    LE_ASSERT(LE_MDC_DISC_UNDEFINED == failureType);

    if (isDualPdpProfile)
    {
        le_mdc_GetPlatformSpecificFailureConnectionReasonExt(profileRef,
                                                             LE_MDC_PDP_UNKNOWN,
                                                             &failureType,
                                                             &failureCode);
        LE_ASSERT(LE_MDC_DISC_UNDEFINED == failureType);
        LE_ASSERT(INT32_MAX == failureCode);
    }
    else
    {
        le_mdc_GetPlatformSpecificFailureConnectionReasonExt(profileRef,
                                                             LE_MDC_PDP_UNKNOWN,
                                                             &failureType,
                                                             &failureCode);
        LE_ASSERT(LE_MDC_DISC_REGULAR_DEACTIVATION == failureType);
        LE_ASSERT(LE_MDC_END_FAILURE_CODE == failureCode);
    }

    le_mdc_GetPlatformSpecificFailureConnectionReasonExt(profileRef,
                                                         pdpType,
                                                         &failureType,
                                                         &failureCode);

    LE_ASSERT(LE_MDC_DISC_REGULAR_DEACTIVATION == failureType);
    LE_ASSERT(LE_MDC_END_FAILURE_CODE == failureCode);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler of connection: it saves the input parameter in global variables for testing in the main
 * thread.
 * The main thread is waiting for its call using a semaphore, the handler posts a semaphore to
 * unlock it.
 *
 */
//--------------------------------------------------------------------------------------------------
static void HandlerFunc
(
    le_mdc_ProfileRef_t profileRef,
    le_mdc_ConState_t ConnectionStatus,
    void* contextPtr
)
{
    ConnectionStateReceivedByHandler = ( ConnectionStatus == LE_MDC_CONNECTED ) ? true : false;
    ProfileRefReceivedByHandler = (le_mdc_ProfileRef_t) profileRef;

    if (ConnectionStatus == LE_MDC_DISCONNECTED)
    {
        switch (le_mdc_GetPDP(profileRef))
        {
            case LE_MDC_PDP_IPV4V6:
                CheckDisconnectionReason(profileRef, LE_MDC_PDP_IPV4, true);
                CheckDisconnectionReason(profileRef, LE_MDC_PDP_IPV6, true);
                break;

            case LE_MDC_PDP_IPV4:
                CheckDisconnectionReason(profileRef, LE_MDC_PDP_IPV4, false);
                break;

            case LE_MDC_PDP_IPV6:
                CheckDisconnectionReason(profileRef, LE_MDC_PDP_IPV6, false);
                break;

            case LE_MDC_PDP_UNKNOWN:
                exit(EXIT_FAILURE);
                break;
        }
    }

    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove the handler
 * The main thread is waiting for its call using a semaphore, the handler posts a semaphore to
 * unlock it.
 *
 */
//--------------------------------------------------------------------------------------------------
static void RemoveHandler
(
    void* param1Ptr,
    void* param2Ptr
)
{
    le_mdc_RemoveSessionStateHandler(param1Ptr);
    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread to test the handler
 * Handlers are called by the event loop, so we need a thread to test the calls.
 * The thread subscribes a handler for each profiles, and then run the event loop.
 * The test is done by the main thread.
 *
 */
//--------------------------------------------------------------------------------------------------
static void* ThreadTestHandler( void* ctxPtr )
{

    int i;

    for (i=0; i < NB_PROFILE; i++)
    {
        SessionStateHandler[i] = le_mdc_AddSessionStateHandler (ProfileRef[i],
                                                                HandlerFunc,
                                                                ProfileRef[i]);

        LE_ASSERT(SessionStateHandler[i] != NULL);
    }

    le_sem_Post(ThreadSemaphore);

    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler subscribed for start and stop session status
 *
 */
//--------------------------------------------------------------------------------------------------
static void SessionHandlerFunc
(
    le_mdc_ProfileRef_t profileRef,
    le_result_t result,
    void* contextPtr
)
{
    LE_ASSERT(result == LE_OK);

    le_sem_Post(ThreadSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread used to test asynchronous start and stop session APIs
 *
 */
//--------------------------------------------------------------------------------------------------
static void* AsyncStartStopSessionThread
(
    void* contextPtr
)
{
    StartStopAsyncFunc_t startStopAsyncFunc = contextPtr;

    startStopAsyncFunc(ProfileRef[0], SessionHandlerFunc, NULL);

    // Run the event loop
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the connection handler calls.
 * API tested:
 * - le_mdc_AddSessionStateHandler / le_mdc_RemoveSessionStateHandler
 * - handler called
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestMdc_Handler ( void )
{
    le_result_t res;

    /* Create the thread to subcribe and call the handlers */
    ThreadSemaphore = le_sem_Create("HandlerSem",0);
    le_thread_Ref_t thread = le_thread_Create("Threadhandler", ThreadTestHandler, NULL);
    le_thread_Start(thread);
    int i;
    le_clk_Time_t   timeToWait;
    timeToWait.sec = 0;
    timeToWait.usec = 1000000;

    /* Wait the thread to be ready */
    le_sem_Wait(ThreadSemaphore);

    for (i = 0; i < NB_PROFILE; i++)
    {
        /* Start a session for the current profile: the handler should be called */
        res = le_mdc_StartSession(ProfileRef[i]);
        LE_ASSERT(res == LE_OK);
        /* Wait for the call of the handler (error if timeout) */
        LE_ASSERT(le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait) == LE_OK);
        /* Check the the handler parameters */
        LE_ASSERT(ProfileRefReceivedByHandler == ProfileRef[i]);
        LE_ASSERT(ConnectionStateReceivedByHandler == true);
        ConnectionStateReceivedByHandler = false;
    }

    for (i = 0; i < NB_PROFILE; i++)
    {
        /* Stop a session for the current profile: the handler should be called */
        res = le_mdc_StopSession(ProfileRef[i]);
        LE_ASSERT(res == LE_OK);
        /* Wait for the call of the handler (error if timeout) */
        LE_ASSERT(le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait) == LE_OK);
        /* Check the the handler parameters */
        LE_ASSERT(ProfileRefReceivedByHandler == ProfileRef[i]);
        LE_ASSERT(ConnectionStateReceivedByHandler == false);
        ConnectionStateReceivedByHandler = true;
    }

    /* Remove the handler of the profile 1: ther handler can be removed only by the thread which
     * subscribed to the handler, so put the RemoveHandler() function in queue of this thread */
    le_event_QueueFunctionToThread(         thread,
                                            RemoveHandler,
                                            SessionStateHandler[1],
                                            NULL);
    le_sem_Wait(ThreadSemaphore);
    /* Start a session for the current profile: no handler should be called */
    res = le_mdc_StartSession(ProfileRef[1]);
    /* No semaphore post is waiting, we are expecting a timeout */
    LE_ASSERT(le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait) == LE_TIMEOUT);
    res = le_mdc_StopSession(ProfileRef[1]);
    /* No semaphore post is waiting, we are expecting a timeout */
    LE_ASSERT(le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait) == LE_TIMEOUT);

}

//--------------------------------------------------------------------------------------------------
/**
 * Test the default profile
 *
 * API tested:
 * - le_mdc_GetProfile with LE_MDC_DEFAULT_PROFILE in argument
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestMdc_DefaultProfile
(
    void
)
{
    /* Change the rat to have a CDMA default profile */
    pa_mrcSimu_SetRadioAccessTechInUse(LE_MRC_RAT_CDMA);
    pa_mdc_ProfileData_t profileData;
    memset(&profileData,0,sizeof(pa_mdc_ProfileData_t));

    pa_mdcSimu_SetProfile(PA_MDC_MIN_INDEX_3GPP2_PROFILE, &profileData);

    /* Get the default profile, and check profile index */
    le_mdc_ProfileRef_t profile = le_mdc_GetProfile(LE_MDC_DEFAULT_PROFILE);

    LE_ASSERT(profile!=NULL);
    LE_ASSERT(le_mdc_GetProfileIndex(profile) == PA_MDC_MIN_INDEX_3GPP2_PROFILE);

    pa_mrcSimu_SetRadioAccessTechInUse(LE_MRC_RAT_GSM);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test the data statistics API
 *
 * API tested:
 * - le_mdc_GetBytesCounters
 * - le_mdc_ResetBytesCounter
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestMdc_Stat
(
    void
)
{
    uint64_t rxBytes;
    uint64_t txBytes;
    pa_mdc_PktStatistics_t dataStatistics;

    /* Set the statistics value to the pa */
    dataStatistics.transmittedBytesCount = 123456789;
    dataStatistics.receivedBytesCount = 369258147;
    pa_mdcSimu_SetDataFlowStatistics(&dataStatistics);

    /* Get the statistics and check the values */
    LE_ASSERT_OK(le_mdc_GetBytesCounters(&rxBytes, &txBytes));
    LE_ASSERT(rxBytes == dataStatistics.receivedBytesCount);
    LE_ASSERT(txBytes == dataStatistics.transmittedBytesCount);

    /* Reset counter, check again statistics (0 values expected) */
    LE_ASSERT_OK(le_mdc_ResetBytesCounter());
    LE_ASSERT_OK(le_mdc_GetBytesCounters(&rxBytes, &txBytes));
    LE_ASSERT(rxBytes == 0);
    LE_ASSERT(txBytes == 0);

    /* Stop and start statistics counters */
    LE_ASSERT_OK(le_mdc_StopBytesCounter());
    LE_ASSERT_OK(le_mdc_StartBytesCounter());
}



//--------------------------------------------------------------------------------------------------
/**
 * Test the start and stop asynchrnous API
 *
 * API tested:
 * - le_mdc_StartSessionAsync
 * - le_mdc_StopSessionAsync
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestMdc_StartStopAsync
(
    void
)
{
    le_clk_Time_t   timeToWait;
    timeToWait.sec = 0;
    timeToWait.usec = 1000000;

    const StartStopAsyncFunc_t testFunc[] = { le_mdc_StartSessionAsync,
                                              le_mdc_StopSessionAsync,
                                              NULL
                                            };
    int i=0;

    while (testFunc[i])
    {
        le_thread_Ref_t testThread = le_thread_Create("AsyncStartStopSessionThread",
                                                      AsyncStartStopSessionThread,
                                                      testFunc[i]);

        // Start the thread
        le_thread_Start(testThread);

        LE_ASSERT(le_sem_WaitWithTimeOut(ThreadSemaphore, timeToWait) != LE_TIMEOUT);

        le_thread_Cancel(testThread);

        i++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread used to run MDC unit tests
 *
 */
//--------------------------------------------------------------------------------------------------
static void* TestThread
(
    void* context
)
{

    LE_INFO("======== Start UnitTest of MDC API ========");

    /* Test configuration */
    TestMdc_Configuration();

    /* Test Connection */
    TestMdc_Connection();

    /* Test handler */
    TestMdc_Handler();

    /* Test default profile */
    TestMdc_DefaultProfile();

    /* Test asynchronous start and stop session */
    TestMdc_StartStopAsync();

    /* Test statistics */
    TestMdc_Stat();

    LE_INFO("======== UnitTest of MDC API ends with SUCCESS ========");

    exit(EXIT_SUCCESS);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * main of the test
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // To reactivate for all DEBUG logs
    le_log_SetFilterLevel(LE_LOG_DEBUG);

    /* init the pa_simu */
    pa_simSimu_Init();

    // Init le_sim
    le_sim_Init();

    // pa simu init */
    pa_mdcSimu_Init();

    /* init the le_mdc service */
    le_mdc_Init();

    /* Add a handler to monitor SIM refresh and synchronize the tests */
    InitSimRefresh();

    // Start unit tests
    le_thread_Start(le_thread_Create("TestThread", TestThread,NULL));

}
