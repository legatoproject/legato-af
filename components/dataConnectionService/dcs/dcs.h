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

#define LE_DCS_TECHNOLOGY_MAX_COUNT 3  // max # of technologies supported
#define LE_DCS_TECH_MAX_NAME_LEN 16    // max length of the name of a technology
#define LE_DCS_CHANNELDBS_MAX 24       // max # of channels supported
#define LE_DCS_CHANNELDB_EVTHDLRS_MAX 20 // max # of channel monitoring event handlers
#define LE_DCS_APPNAME_MAX_LEN 16      // max length of an app's name


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
    bool managed_by_le_data;                       ///< this channel is to be managed by le_data
    bool shared_with_le_data;                      ///< this channel is shared with le_data
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
 * The following are externs for various files
 */
//--------------------------------------------------------------------------------------------------

//extern from dcs.c
extern DcsInfo_t DcsInfo;
extern le_result_t le_dcs_InitChannelList(le_dcs_ChannelInfo_t *channelList, size_t *listSize);

// extern from dcs_utils.c
LE_SHARED const char *le_dcs_ConvertTechEnumToName(le_dcs_Technology_t tech);
extern le_result_t dcsGetTechnology(le_dcs_ChannelRef_t channelRef,
                                    le_dcs_Technology_t *technology);
extern le_result_t dcsGetAdminState(le_dcs_ChannelRef_t channelRef, le_dcs_State_t *state);

// extern from dcs_db.c
extern void dcsCreateDbPool(void);
LE_SHARED le_dcs_ChannelRef_t le_dcs_CreateChannelDb(le_dcs_Technology_t tech,
                                                     const char *channelName);
LE_SHARED void le_dcs_ChannelEventNotifier(le_dcs_ChannelRef_t channelRef, le_dcs_Event_t evt);
LE_SHARED le_dcs_ChannelRef_t le_dcs_GetChannelRefFromTechRef(void *techRef);
LE_SHARED le_result_t le_dcs_GetChannelRefCountFromTechRef(void *techRef, uint16_t *refCount);
LE_SHARED le_dcs_channelDbEventHdlr_t *le_dcs_GetChannelAppEvtHdlr(
    le_dcs_channelDb_t *channelDb, le_msg_SessionRef_t appSessionRef);
LE_SHARED void le_dcs_EventNotifierTechStateTransition(le_dcs_Technology_t tech, bool tech_state);
LE_SHARED void le_dcs_MarkChannelSharingStatus(const char *channelName, le_dcs_Technology_t tech,
                                               bool starting);
LE_SHARED bool le_dcs_ChannelIsInUse(const char *channelName, le_dcs_Technology_t tech);
LE_SHARED le_dcs_channelDb_t *le_dcs_GetChannelDbFromRef(le_dcs_ChannelRef_t channelRef);
extern le_dcs_channelDb_t *dcsDelChannelEvtHdlr(le_dcs_EventHandlerRef_t hdlrRef);
extern le_dcs_channelDbEventHdlr_t *dcsChannelDbEvtHdlrInit(void);
extern le_dcs_channelDb_t *dcsGetChannelDbFromName(const char *channelName,
                                                   le_dcs_Technology_t tech);
extern void dcsChannelEvtHdlrSendNotice(le_dcs_channelDb_t *channelDb,
                                        le_msg_SessionRef_t appSessionRef, le_dcs_Event_t evt);

// extern from dcsTech.c
LE_SHARED le_result_t le_dcsTech_Start(const char *channelName, le_dcs_Technology_t tech);
LE_SHARED le_result_t le_dcsTech_Stop(const char *channelName, le_dcs_Technology_t tech);
LE_SHARED le_result_t le_dcsTech_GetDefaultGWAddress(le_dcs_Technology_t tech, void *techRef,
                                                     bool *isIpv6, char *gwAddr, size_t *gwAddrSize);
LE_SHARED le_result_t le_dcsTech_GetDNSAddresses(le_dcs_Technology_t tech, void *techRef,
                                                 bool *isIpv6, char *dns1Addr, size_t *addr1Size,
                                                 char *dns2Addr, size_t *addr2Size);
LE_SHARED le_result_t le_dcsTech_GetNetInterface(le_dcs_Technology_t tech,
                                                 le_dcs_ChannelRef_t channelRef, char *intfName,
                                                 int nameSize);
extern le_result_t le_dcsTech_GetChannelList(le_dcs_Technology_t tech,
                                             le_dcs_ChannelInfo_t *channelList, size_t *listSize);
extern void *le_dcsTech_CreateTechRef(le_dcs_Technology_t tech, const char *channelName);
extern void le_dcsTech_ReleaseTechRef(le_dcs_Technology_t tech, void *techRef);
extern int16_t le_dcsTech_GetListIndx(le_dcs_Technology_t technology);
extern bool le_dcsTech_GetOpState(le_dcs_channelDb_t *channelDb);
extern void le_dcsTech_RetryChannel(le_dcs_channelDb_t *channelDb);

#endif
