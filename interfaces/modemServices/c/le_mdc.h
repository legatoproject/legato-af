/**
 * @page c_mdc Modem Data Control API
 *
 * @ref le_mdc.h "Click here for the API Reference documentation."
 *
 * <HR>
 *
 *@ref le_mdc_profile <br>
 *@ref le_mdc_session <br>
 *@ref le_mdc_dataStatistics <br>
 *@ref le_mdc_configdb <br>
 *
 *
 * A data session is useful for  applications that need to send or receive data over a network
 * where SMS messages are insufficient.  To start a data session, a data profile must
 * be configured as specified by the target network.
 *
 * The Modem Data Control (mdc) API is used to manage data profiles and data sessions.
 *
 *
 * @section le_mdc_profile Data Profiles
 *
 * If a pre-defined data profile has been configured then this profile can
 * be loaded using @ref le_mdc_LoadProfile. @ref le_mdc_LoadProfile will try to read the data profile configuration
 * from the configuration tree.
 * If one data profile is currently in use and one of its parameters changes in the configuration
 * tree, the new value will be loaded automatically.
 * The maximum number of data profiles supported is modem dependent.
 *
 * The following data profile parameters can be retrieved:
 *  - Profile name using @ref le_mdc_GetProfileName.
 *  - IP Preference (i.e. PDP type) can be checked with @c le_mdc_IsIPV4() or
 *  @c le_mdc_IsIPV6()
 *
 * @todo
 *  - IP preference (i.e. PDP_type) is hard-coded to IPv4, but will be configurable in the future
 *  - Other profile parameters will be configurable in a future version.
 *
 * @section le_mdc_session Data Sessions
 *
 * A data session can be started using @ref le_mdc_StartSession.  To start a data session, a
 * data profile must be created and written to the modem, or  an existing data profile
 * can be used.  A data session can be stopped using @ref le_mdc_StopSession.  The number of
 * simultaneous data sessions supported is dependent on the modem, but cannot be more than the
 * maximum number of supported profiles.
 *
 * The current state of a data session can be queried using @ref le_mdc_GetSessionState. An application
 * can also a register handler to be notified when the session state changes. The handler
 * can be managed using @ref le_mdc_AddSessionStateHandler and @ref le_mdc_RemoveSessionStateHandler.
 *
 * Once a data session starts, a Linux network interface is created.  It's the application's responsibility
 * to configure the network interface, usually through a DHCP client. Query the
 * interface name using @ref le_mdc_GetInterfaceName. The IP address for the current data session
 * can be retreived by  @ref le_mdc_GetIPAddress. The Gateway and DNS
 * addresses can be retrieved using @ref le_mdc_GetGatewayAddress and @ref le_mdc_GetDNSAddresses.
 * The Access Point Name can be retrieved by @ref le_mdc_GetAccessPointName. The Data bearer
 * Technology can be retreived by @ref le_mdc_GetDataBearerTechnology.
 *
 * @section le_mdc_dataStatistics Data Statistics
 *
 * Data bytes received/transmitted can be access through @ref le_mdc_GetBytesCounters.
 * These values correspond to the number of bytes received/transmitted since the last software reset
 * or the last @ref le_mdc_ResetBytesCounter called.
 * Making these value persistent after a software reboot is the client responsibility.
 *
 * @section le_mdc_configdb Data configuration tree
 *
 * The configuration database path for the Modem Data Control is:
 * @verbatim
   /
       modemServices/
           modemDataConnection/
               <ProfileName_1>/
                   accessPointName<string> == <ADDR>
                   packetDataProtocol<string> == <PDP_TYPE>
                   authentication/
                       pap/
                           enable<bool> == <true/false>
                           userName<string> == <USERNAME>
                           password<string> == <PWD>
                       chap/
                           enable<bool> == <true/false>
                           userName<string> == <USERNAME>
                           password<string> == <PWD>

               <ProfileName_2>/
                   accessPointName<string> == <ADDR>
               ...
               <ProfileName_5>/
                   accessPointName<string> == <ADDR>
  @endverbatim
 *
 *  - 'ProfileName_*' is the name that @ref le_mdc_LoadProfile can load.
 *
 *  - 'ADDR' is an address like xxx.xxx.xxx.xxx .
 *
 *  - 'USERNAME' is the name used for authentication
 *
 *  - 'PWD' is the password used for authentication
 *
 *  - 'PDP_TYPE' is the protocol:
 *    - IPV4
 *    - IPV6
 *
 *  If //modemServices/modemDataConnection/<ProfileName>/packetDataProtocol is omitted, IPV4 will
 *  be the default protocol.
 * @note PAP and CHAP authentication are not usable at the same time, the first authentication
 * enabled found in the configDB will be used.
 *
 * <HR>
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */


//--------------------------------------------------------------------------------------------------
/**
 * @file le_mdc.h
 *
 * Legato @ref c_mdc include file.
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 *
 */

#ifndef LEGATO_LE_MDC_INCLUDE_GUARD
#define LEGATO_LE_MDC_INCLUDE_GUARD


#include "legato.h"
#include "le_mdm_defs.h"


//--------------------------------------------------------------------------------------------------
/**
 * Data profile object
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mdc_Profile* le_mdc_Profile_Ref_t;


//--------------------------------------------------------------------------------------------------
/**
 *  Reference type for Data Session State's Changes Handler references.
 */
//--------------------------------------------------------------------------------------------------
typedef struct le_mdc_SessionStateHandler* le_mdc_SessionStateHandlerRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * Prototype for data session state change handler.
 *
 * @param isConnected   Data session connection status.
 * @param contextPtr   Whatever context information the event handler may require.
 */
//--------------------------------------------------------------------------------------------------
typedef void (*le_mdc_SessionStateHandlerFunc_t)
(
    bool   isConnected,
    void*  contextPtr
);


//--------------------------------------------------------------------------------------------------
/**
 * Load an existing data profile
 *
 * Profile can either be pre-configured or stored on the modem
 *
 * @return
 *      - Reference to the data profile
 *      - NULL if the profile does not exist
 */
//--------------------------------------------------------------------------------------------------
le_mdc_Profile_Ref_t le_mdc_LoadProfile
(
    const char* nameStr                 ///< [IN] Profile name
);



//--------------------------------------------------------------------------------------------------
/**
 * Get profile name.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the name would not fit in buffer
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetProfileName
(
    le_mdc_Profile_Ref_t profileRef,        ///< [IN] Query this profile object
    char*  nameStr,                         ///< [OUT] Profile name
    size_t nameStrSize                      ///< [IN] Name buffer size in bytes
);


//--------------------------------------------------------------------------------------------------
/**
 * Start profile data session.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session is already connected for the given profile
 *      - LE_NOT_POSSIBLE for other failures
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_StartSession
(
    le_mdc_Profile_Ref_t profileRef     ///< [IN] Start data session for this profile object
);


//--------------------------------------------------------------------------------------------------
/**
 * Stop profile data session.
 *
 * @return
 *      - LE_OK on success
 *      - LE_DUPLICATE if the data session has already been stopped (i.e. it is disconnected)
 *      - LE_NOT_POSSIBLE for other failures
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_StopSession
(
    le_mdc_Profile_Ref_t profileRef     ///< [IN] Stop data session for this profile object
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the current data session state.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE on failure
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetSessionState
(
    le_mdc_Profile_Ref_t profileRef,        ///< [IN] Query this profile object
    bool* isConnectedPtr                    ///< [OUT] Data session state
);


//--------------------------------------------------------------------------------------------------
/**
 * Register a handler for session state changes on the given profile.
 *
 * @return
 *      A handler reference, which is only needed for later removal of the handler.
 *
 * @note
 *      Process exits on failure.
 */
//--------------------------------------------------------------------------------------------------
le_mdc_SessionStateHandlerRef_t le_mdc_AddSessionStateHandler
(
    le_mdc_Profile_Ref_t profileRef,        ///< [IN] Profile object of interest
    le_mdc_SessionStateHandlerFunc_t handler,   ///< [IN] Handler function
    void* contextPtr                        ///< [IN] Context pointer
);


//--------------------------------------------------------------------------------------------------
/**
 * Remove a handler for session state changes
 *
 * @note
 *      Process exits on failure.
 */
//--------------------------------------------------------------------------------------------------
void le_mdc_RemoveSessionStateHandler
(
    le_mdc_SessionStateHandlerRef_t    handlerRef ///< [IN] Handler reference.
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the network interface name, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the interface name can't fit in interfaceNameStr
 *      - LE_NOT_POSSIBLE on any other failure
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetInterfaceName
(
    le_mdc_Profile_Ref_t profileRef,        ///< [IN] Query this profile object
    char*  interfaceNameStr,                ///< [OUT] Network interface name
    size_t interfaceNameStrSize             ///< [IN] Name buffer size in bytes
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the IP address for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address would not fit in ipAddrStr
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetIPAddress
(
    le_mdc_Profile_Ref_t profileRef,   ///< [IN] Query this profile object
    char*  ipAddrStr,                  ///< [OUT] The IP address in dotted format
    size_t ipAddrStrSize               ///< [IN] The size in bytes of the address buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the gateway IP address, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address can't fit in gatewayAddrStr
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetGatewayAddress
(
    le_mdc_Profile_Ref_t profileRef,        ///< [IN] Query this profile object
    char*  gatewayAddrStr,                  ///< [OUT] Gateway IP address in dotted format
    size_t gatewayAddrStrSize               ///< [IN] Address buffer size in bytes
);


//--------------------------------------------------------------------------------------------------
/**
 * Get the primary/secondary DNS addresses, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the IP address can't fit in buffer
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      - If only one DNS address is available, it will be returned, and an empty string will
 *        be returned for the unavailable address.
 *      - The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetDNSAddresses
(
    le_mdc_Profile_Ref_t profileRef,        ///< [IN] Query this profile object
    char*  dns1AddrStr,                     ///< [OUT] Primary DNS IP address in dotted format
    size_t dns1AddrStrSize,                 ///< [IN] dns1AddrStr buffer size in bytes
    char*  dns2AddrStr,                     ///< [OUT] Secondary DNS IP address in dotted format
    size_t dns2AddrStrSize                  ///< [IN] dns2AddrStr buffer size in bytes
);

//--------------------------------------------------------------------------------------------------
/**
 * Allow the caller to know if the given profile is actually supporting IPv4.
 *
 * @return TRUE if PDP type is IPv4, FALSE otherwise.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
bool le_mdc_IsIPV4
(
    le_mdc_Profile_Ref_t profileRef        ///< [IN] Query this profile object
);


//--------------------------------------------------------------------------------------------------
/**
 * Allow the caller to know if the given profile is actually supporting IPv6.
 *
 * @return TRUE if PDP type is IPv6, FALSE otherwise.
 *
 * @note If the caller is passing a bad pointer into this function, it is a fatal error, the
 *       function will not return.
 */
//--------------------------------------------------------------------------------------------------
bool le_mdc_IsIPV6
(
    le_mdc_Profile_Ref_t profileRef        ///< [IN] Query this profile object
);

//--------------------------------------------------------------------------------------------------
/**
 * Get number of bytes received/transmitted without error since the last reset.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      - The process exits, if an invalid pointer is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetBytesCounters
(
    uint64_t *rxBytes,  ///< [OUT] bytes amount received since the last counter reset
    uint64_t *txBytes   ///< [OUT] bytes amount transmitted since the last counter reset
);

//--------------------------------------------------------------------------------------------------
/**
 * Reset received/transmitted data flow statistics
 *
 * * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_ResetBytesCounter
(
    void
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Access Point Name for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OVERFLOW if the Access Point Name would not fit in apnNameStr
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetAccessPointName
(
    le_mdc_Profile_Ref_t profileRef,   ///< [IN] Query this profile object
    char*  apnNameStr,                 ///< [OUT] The Access Point Name
    size_t apnNameStrSize              ///< [IN] The size in bytes of the address buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Get the Data Bearer Technology for the given profile, if the data session is connected.
 *
 * @return
 *      - LE_OK on success
 *      - LE_NOT_POSSIBLE for all other errors
 *
 * @note
 *      The process exits, if an invalid profile object is given
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_mdc_GetDataBearerTechnology
(
    le_mdc_Profile_Ref_t profileRef,                        ///< [IN] Query this profile object
    le_mdc_dataBearerTechnology_t* dataBearerTechnologyPtr  ///< [OUT] The data bearer technology
);


#endif // LEGATO_LE_MDC_INCLUDE_GUARD

