//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's header file for the C code implementation of its southbound interfaces
 *  with the Ethernet component.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DCSETHERNET_H_INCLUDE_GUARD
#define LEGATO_DCSETHERNET_H_INCLUDE_GUARD

typedef void* le_dcs_EthernetConnectionRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * The following is Ethernet's connection db for tracking each connection's state, info, details,
 * etc. Later after each technology is made pluggable, these will be saved within the technology
 * component & retrieved via southbound API calls
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dcs_EthernetConnectionRef_t connRef;         ///< Ethernet connection's safe reference
    char netIntf[LE_DCS_INTERFACE_NAME_MAX_LEN];    ///< Network interface name
    bool opState;                                   ///< Technology defined operational state
} ethernet_connDb_t;


//--------------------------------------------------------------------------------------------------
/**
 * The following are LE_SHARED functions
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_dcsEthernet_GetChannelList(void);
LE_SHARED le_result_t le_dcsEthernet_GetNetInterface(void *techRef, char *intfName, int nameSize);
LE_SHARED le_result_t le_dcsEthernet_Start(void *techRef);
LE_SHARED le_result_t le_dcsEthernet_Stop(void *techRef);
LE_SHARED void       *le_dcsEthernet_CreateConnDb(const char *conn);
LE_SHARED void        le_dcsEthernet_ReleaseConnDb(void *techRef);
LE_SHARED bool        le_dcsEthernet_GetOpState(void *techRef);
LE_SHARED le_result_t le_dcsEthernet_AllowChannelStart(void *techRef);

#endif
