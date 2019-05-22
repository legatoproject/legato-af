//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's header file for the C code implementation of the le_dcs APIs
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DCS_H_INCLUDE_GUARD
#define LEGATO_DCS_H_INCLUDE_GUARD


//--------------------------------------------------------------------------------------------------
/**
 * The config tree path and node definitions.
 */
//--------------------------------------------------------------------------------------------------
#define DCS_CONFIG_TREE_ROOT_DIR    "dataConnectionService:"

#define CFG_PATH_ROUTING            "routing"
#define CFG_NODE_DEFAULTROUTE       "useDefaultRoute"
#define CFG_PATH_WIFI               "wifi"
#define CFG_NODE_SSID               "SSID"
#define CFG_PATH_CELLULAR           "cellular"
#define CFG_NODE_PROFILEINDEX       "profileIndex"
#define CFG_PATH_TIME               "time"
#define CFG_NODE_PROTOCOL           "protocol"
#define CFG_NODE_SERVER             "server"


#define LE_DCS_TECHNOLOGY_MAX_COUNT 3  // max # of technologies supported
#define LE_DCS_TECH_MAX_NAME_LEN 16    // max length of the name of a technology
#define LE_DCS_CHANNELDBS_MAX LE_DCS_CHANNEL_LIST_ENTRY_MAX // max # of channels supported
#define LE_DCS_CHANNELDB_EVTHDLRS_MAX 20 // max # of channel monitoring event handlers
#define LE_DCS_APPNAME_MAX_LEN 32      // max length of an app's name
#define LE_DCS_CHANNEL_QUERY_HDLRS_MAX 20 // max # of channel query requester handlers
#define LE_DCF_START_REQ_REF_MAP_SIZE  20 // reference map size for Start Requests

//--------------------------------------------------------------------------------------------------
/**
 * The following struct is an element of the list of all the technologies in action being tracked
 * by DCS
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dcs_Technology_t techEnum;
    char techName[LE_DCS_TECH_MAX_NAME_LEN];
    uint16_t channelCount;
    uint16_t reqCount;
} TechListDb_t;


//--------------------------------------------------------------------------------------------------
/**
 * The following is DCS's gobal data structure to keep track of global lists, counts, info, etc
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_msg_SessionRef_t internalSessionRef;
    uint16_t reqCount;                            // the request count for the use of le_dcs APIs
    TechListDb_t techListDb[LE_DCS_TECHNOLOGY_MAX_COUNT];   // list of all technologies in action
} DcsInfo_t;


//--------------------------------------------------------------------------------------------------
/**
 * The following is an event handler data structure to track each registered event handler's event
 * ID, handler obj ref, owner app's name, callback function, etc
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_event_Id_t channelEventId;                 ///< channel event ID of the app
    le_dcs_EventHandlerFunc_t channelEventHdlr;   ///< channel event handler of the app
    le_dcs_EventHandlerRef_t hdlrRef;             ///< handler reference as identifier upon removal
    le_msg_SessionRef_t appSessionRef;            ///< session ref of the app owning this handler
    le_dls_Link_t hdlrLink;
} le_dcs_channelDbEventHdlr_t;


//--------------------------------------------------------------------------------------------------
/**
 * channel database structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char channelName[LE_DCS_CHANNEL_NAME_MAX_LEN]; ///< channel name
    le_dcs_ChannelRef_t channelRef;                ///< channel's safe reference
    le_dcs_Technology_t technology;                ///< technology type
    void *techRef;                                 ///< technology specific db's reference
    uint16_t refCount;                             ///< refcount: # of apps using this channel
    le_dls_List_t evtHdlrs;                        ///< evtHdlrs list storing event ID, handler, etc
    le_dls_List_t startRequestRefList;             ///< list of Start Request references
} le_dcs_channelDb_t;


//--------------------------------------------------------------------------------------------------
/**
 * The following is DCS's data structure for posting a channel event to an app's channel event
 * handler to be used with le_event_report()
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dcs_channelDb_t *channelDb;               ///< channel db for which the event is
    le_dcs_Event_t event;                        ///< event to be posted to corresponding handler
} le_dcs_channelDbEventReport_t;


//--------------------------------------------------------------------------------------------------
/**
 * The following is DCS's data structure for posting the results of a technology's channel list
 * queried by an app to be used with le_event_report()
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_result_t result;                          ///< result to be posted to corresponding handler
    le_dcs_ChannelInfo_t *channelList;           ///< list of channels returned
    size_t listSize;                             ///< # of entries returned on the list of channels
} le_dcs_channelQueryReport_t;


//--------------------------------------------------------------------------------------------------
/**
 * The following is a data structure to record the object reference of each Start Request on a
 * given channel.  It is an element to be added onto a double link list on its channelDb.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_dcs_ReqObjRef_t ref;
    le_dls_Link_t refLink;
} le_dcs_startRequestRefDb_t;


//--------------------------------------------------------------------------------------------------
/**
 * The following are externs for various files
 */
//--------------------------------------------------------------------------------------------------

// from dcs.c
uint16_t le_dcs_GetChannelCount(le_dcs_Technology_t tech);
uint16_t le_dcs_IncrementChannelCount(le_dcs_Technology_t tech);
le_result_t le_dcs_DecrementChannelCount(le_dcs_Technology_t tech, uint16_t *newCount);
void le_dcs_AdjustReqCount(le_dcs_channelDb_t *channelDb, bool up);
LE_SHARED le_dcs_EventHandlerRef_t le_dcs_AddEventHandler(
    le_dcs_ChannelRef_t channelRef, le_dcs_EventHandlerFunc_t channelHandlerPtr, void *contextPtr);
LE_SHARED void le_dcs_RemoveEventHandler(le_dcs_EventHandlerRef_t channelHandlerRef);
LE_SHARED le_dcs_ChannelRef_t le_dcs_GetReference(const char *name, le_dcs_Technology_t technology);
LE_SHARED le_dcs_ReqObjRef_t le_dcs_Start(le_dcs_ChannelRef_t channelRef);
LE_SHARED le_result_t le_dcs_Stop(le_dcs_ReqObjRef_t reqRef);

// from dcs_utils.c
LE_SHARED const char *le_dcs_ConvertTechEnumToName(le_dcs_Technology_t tech);
LE_SHARED const char *le_dcs_ConvertEventToString(le_dcs_Event_t event);
le_result_t le_dcs_GetAdminState(le_dcs_ChannelRef_t channelRef, le_dcs_State_t *state);

// from dcs_db.c
void dcsCreateDbPool(void);
LE_SHARED bool le_dcs_DeleteChannelDb(le_dcs_Technology_t tech, void *techRef);
LE_SHARED void le_dcs_ChannelEventNotifier(le_dcs_ChannelRef_t channelRef, le_dcs_Event_t evt);
LE_SHARED le_dcs_ChannelRef_t le_dcs_GetChannelRefFromTechRef(le_dcs_Technology_t tech,
                                                              void *techRef);
LE_SHARED le_result_t le_dcs_GetChannelRefCountFromTechRef(le_dcs_Technology_t tech, void *techRef,
                                                           uint16_t *refCount);
LE_SHARED void le_dcs_EventNotifierTechStateTransition(le_dcs_Technology_t tech, bool tech_state);
LE_SHARED le_dcs_channelDb_t *le_dcs_GetChannelDbFromRef(le_dcs_ChannelRef_t channelRef);
LE_SHARED le_dcs_channelDb_t *le_dcs_GetChannelDbFromName(const char *channelName,
                                                          le_dcs_Technology_t tech);
le_dcs_ChannelRef_t le_dcs_CreateChannelDb(le_dcs_Technology_t tech, const char *channelName);
le_dcs_channelDb_t *DcsDelChannelEvtHdlr(le_dcs_EventHandlerRef_t hdlrRef);
le_dcs_channelDbEventHdlr_t *dcsChannelDbEvtHdlrInit(void);
void dcsChannelEvtHdlrSendNotice(le_dcs_channelDb_t *channelDb, le_msg_SessionRef_t appSessionRef,
                                 le_dcs_Event_t evt);
le_dcs_channelDbEventHdlr_t *le_dcs_GetChannelAppEvtHdlr(le_dcs_channelDb_t *channelDb,
                                                         le_msg_SessionRef_t appSessionRef);
bool le_dcs_ChannelQueryIsRunning(void);
void le_dcs_ChannelQueryNotifier(le_result_t result, le_dcs_ChannelInfo_t *channelList,
                                 size_t listSize);
void le_dcs_AddChannelQueryHandlerDb(le_dcs_GetChannelsHandlerFunc_t channelQueryHandlerFunc,
                                     void *context);
bool le_dcs_AddStartRequestRef(le_dcs_ReqObjRef_t reqRef, le_dcs_channelDb_t *channelDb);
bool le_dcs_DeleteStartRequestRef(le_dcs_startRequestRefDb_t *refDb,
                                  le_dcs_channelDb_t *channelDb);
le_dcs_channelDb_t *le_dcs_GetChannelDbFromStartRequestRef(le_dcs_ReqObjRef_t reqRef,
                                                           le_dcs_startRequestRefDb_t **reqRefDb);

// from dcsTech.c
LE_SHARED le_result_t le_dcsTech_Start(const char *channelName, le_dcs_Technology_t tech);
LE_SHARED le_result_t le_dcsTech_Stop(const char *channelName, le_dcs_Technology_t tech);
LE_SHARED le_result_t le_dcsTech_GetDefaultGWAddress(le_dcs_Technology_t tech, void *techRef,
                                                     char *v4GwAddrPtr, size_t v4GwAddrSize,
                                                     char *v6GwAddrPtr, size_t v6GwAddrSize);
LE_SHARED le_result_t le_dcsTech_GetDNSAddresses(le_dcs_Technology_t tech, void *techRef,
                                                 char *v4DnsAddrs, size_t v4DnsAddrSize,
                                                 char *v6DnsAddrs, size_t v6DnsAddrSize);
LE_SHARED le_result_t le_dcsTech_GetNetInterface(le_dcs_Technology_t tech,
                                                 le_dcs_ChannelRef_t channelRef, char *intfName,
                                                 int nameSize);
LE_SHARED void le_dcsTech_CollectChannelQueryResults(le_dcs_Technology_t technology,
                                                     le_result_t result,
                                                     le_dcs_ChannelInfo_t *channelList,
                                                     size_t listSize);
le_result_t le_dcsTech_AllowChannelStart(le_dcs_Technology_t tech, const char *channelName);
le_result_t le_dcsTech_GetChannelList(le_dcs_Technology_t tech);
void *le_dcsTech_CreateTechRef(le_dcs_Technology_t tech, const char *channelName);
void le_dcsTech_ReleaseTechRef(le_dcs_Technology_t tech, void *techRef);
bool le_dcsTech_GetOpState(le_dcs_channelDb_t *channelDb);
void le_dcsTech_RetryChannel(le_dcs_channelDb_t *channelDb);
bool le_dcsTech_ChannelQueryIsPending(le_dcs_Technology_t tech);

#endif
