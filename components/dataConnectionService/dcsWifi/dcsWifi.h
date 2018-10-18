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


//--------------------------------------------------------------------------------------------------
/**
 * The following are LE_SHARED functions
 */
//--------------------------------------------------------------------------------------------------
LE_SHARED le_result_t le_dcsWifi_GetChannelList(le_dcs_ChannelInfo_t *channelList,
                                                int16_t *listSize);

#endif
