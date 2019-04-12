//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's header file for the C code implementation of its southbound interfaces
 *  with the cellular component.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DCSCELLULAR_H_INCLUDE_GUARD
#define LEGATO_DCSCELLULAR_H_INCLUDE_GUARD

typedef void* le_dcs_CellularConnectionRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * The following is cellular's connection db for tracking each connection's state, info, details,
 * etc. Later after each technology is made pluggable, these will be saved within the technology
 * component & retrieved via southbound API calls
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dcs_CellularConnectionRef_t connRef; ///< cellular connection's safe reference
    uint32_t index;                         ///< profile index
    char netIntf[LE_DCS_INTERFACE_NAME_MAX_LEN]; ///< network interface name
    uint16_t opState;                       ///< technology defined operational state
    uint16_t retries;                       ///< # of retries attempted in a sequence
    uint16_t backoff;                       ///< latest backoff duration to use in next retry
    le_mdc_SessionStateHandlerRef_t evtHdlrRef; ///< cellular connection event handler reference
    le_timer_Ref_t retryTimer;              ///< retry timer with backoff
} cellular_connDb_t;


//--------------------------------------------------------------------------------------------------
/**
 * The following are LE_SHARED functions
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_dcsCellular_GetChannelList(void);
LE_SHARED le_result_t le_dcsCellular_GetNetInterface(void *techRef, char *intfName, int nameSize);
LE_SHARED le_result_t le_dcsCellular_Start(void *techRef);
LE_SHARED le_result_t le_dcsCellular_Stop(void *techRef);
LE_SHARED void *le_dcsCellular_CreateConnDb(const char *conn);
LE_SHARED void le_dcsCellular_ReleaseConnDb(void *techRef);
LE_SHARED bool le_dcsCellular_GetOpState(void *techRef);
LE_SHARED le_result_t le_dcsCellular_RetryConn(void *techRef);
LE_SHARED void le_dcsCellular_GetNameFromIndex(uint32_t index,
                                               char channelName[LE_DCS_CHANNEL_NAME_MAX_LEN]);
LE_SHARED le_result_t le_dcsCellular_GetDefaultGWAddress(void *techRef,
                                                         char *v4GwAddrPtr, size_t v4GwAddrSize,
                                                         char *v6GwAddrPtr, size_t v6GwAddrSize);
LE_SHARED le_result_t le_dcsCellular_GetDNSAddrs(void *techRef, char *v4DnsAddrs,
                                                 size_t v4DnsAddrSize, char *v6DnsAddr,
                                                 size_t v6DnsAddrSize);
LE_SHARED uint32_t le_dcsCellular_GetProfileIndex(int32_t mdcIndex);
LE_SHARED le_result_t le_dcsCellular_SetProfileIndex(int32_t mdcIndex);
LE_SHARED le_result_t le_dcsCellular_AllowChannelStart(void *techRef);

#endif
