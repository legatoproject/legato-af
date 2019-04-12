//--------------------------------------------------------------------------------------------------
/**
 *  Data Connection Server's .h file
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DCSSERVER_H_INCLUDE_GUARD
#define LEGATO_DCSSERVER_H_INCLUDE_GUARD

//--------------------------------------------------------------------------------------------------
/**
 * Definitions for sending API commands to data thread
 */
//--------------------------------------------------------------------------------------------------
#define REQUEST_COMMAND 1   // used by le_data APIs
#define RELEASE_COMMAND 2   // used by le_data APIs
#define START_COMMAND 3     // used by le_dcs APIs
#define STOP_COMMAND 4      // used by le_dcs APIs

typedef struct
{
    uint32_t command;
    char channelName[64];
    le_dcs_Technology_t technology;
} CommandData_t;


// extern from dcsServer.c
LE_SHARED le_event_Id_t dcs_GetCommandEventId(void);
LE_SHARED le_ref_MapRef_t dcs_GetRequestRefMap(void);
bool dcs_ChannelIsConnected(void);

#endif
