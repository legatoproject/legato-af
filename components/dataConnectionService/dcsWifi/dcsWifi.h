//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's header file for the C code implementation of its southbound interfaces
 *  with the Wifi component.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DCSWIFI_H_INCLUDE_GUARD
#define LEGATO_DCSWIFI_H_INCLUDE_GUARD

typedef void* le_dcs_WifiConnectionRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * The following is wifi's connection db for tracking each connection's state, info, details,
 * etc. Later after each technology is made pluggable, these will be saved within the technology
 * component & retrieved via southbound API calls
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dcs_WifiConnectionRef_t connRef;     ///< wifi connection's safe reference
    char ssid[LE_DCS_CHANNEL_NAME_MAX_LEN]; ///< SSID
    le_dls_Link_t dbLink;                   ///< double-link list on which are all wifi conn dbs
} wifi_connDb_t;


//--------------------------------------------------------------------------------------------------
/**
 * The following are LE_SHARED functions
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_dcsWifi_GetChannelList(void);
LE_SHARED void le_dcsWifi_ReleaseConnDb(void *techRef);
LE_SHARED void *le_dcsWifi_CreateConnDb(const char *conn);
LE_SHARED le_result_t le_dcsWifi_Start(void *techRef);
LE_SHARED le_result_t le_dcsWifi_Stop(void *techRef);
LE_SHARED bool le_dcsWifi_GetOpState(void *techRef);
LE_SHARED le_result_t le_dcsWifi_AllowChannelStart(void *techRef);
LE_SHARED le_result_t le_dcsWifi_GetNetInterface(void *techRef, char *intfName, int nameSize);

#endif
