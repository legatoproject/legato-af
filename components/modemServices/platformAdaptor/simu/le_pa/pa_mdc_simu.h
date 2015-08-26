/** @file pa_mdc_simu.h
 *
 * Legato @ref pa_mdc_simu include file.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#ifndef PA_MDC_SIMU_H_INCLUDE_GUARD
#define PA_MDC_SIMU_H_INCLUDE_GUARD

#include "pa_mdc.h"

//--------------------------------------------------------------------------------------------------
/**
 * Set the profile data
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mdcSimu_SetProfile
(
    uint32_t profileIndex,                  ///< [IN] The profile to read
    pa_mdc_ProfileData_t* profileDataPtr    ///< [OUT] The profile data
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the name of the network interface for the given profile
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mdcSimu_SetInterfaceName
(
    uint32_t profileIndex,
    char*  interfaceNameStr
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the primary/secondary DNS addresses for the given profile
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mdcSimu_SetDNSAddresses
(
    uint32_t profileIndex,
    le_mdmDefs_IpVersion_t ipVersion,
    char*  dns1AddrStr,
    char*  dns2AddrStr
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the gateway IP address for the given profile.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mdcSimu_SetGatewayAddress
(
    uint32_t profileIndex,                 ///< [IN] The profile to use
    le_mdmDefs_IpVersion_t ipVersion,      ///< [IN] IP Version
    char*  gatewayAddrStr                  ///< [IN] The gateway IP address in dotted format
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the IP address for the given profile.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mdcSimu_SetIPAddress
(
    uint32_t profileIndex,
    le_mdmDefs_IpVersion_t ipVersion,
    char*  ipAddrStr
);

//--------------------------------------------------------------------------------------------------
/**
 * Set the name of the network interface for the given profile
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mdcSimu_SetInterfaceName
(
    uint32_t profileIndex,
    char*  interfaceNameStr
);



//--------------------------------------------------------------------------------------------------
/**
 * Free all profiles
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mdcSimu_DeleteProfiles
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Set data flow statistics.
 *
 */
//--------------------------------------------------------------------------------------------------
void pa_mdcSimu_SetDataFlowStatistics
(
    pa_mdc_PktStatistics_t *dataStatisticsPtr ///< [OUT] Statistics data
);

//--------------------------------------------------------------------------------------------------
/**
 * simu init
 *
 **/
//--------------------------------------------------------------------------------------------------
le_result_t pa_mdcSimu_Init
(
    void
);


#endif // PA_MDC_SIMU_H_INCLUDE_GUARD

