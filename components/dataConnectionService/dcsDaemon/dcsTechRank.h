//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Service's header file for Technology Rank Manager.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#ifndef LEGATO_DCS_PROFILES_H_INCLUDE_GUARD
#define LEGATO_DCS_PROFILES_H_INCLUDE_GUARD

le_data_Technology_t le_data_GetFirstUsedTechnology(void);
le_data_Technology_t le_data_GetNextUsedTechnology(void);

le_dcs_ChannelRef_t dcs_ChannelGetCurrentReference(void);
void dcs_ChannelSetCurrentReference(le_dcs_ChannelRef_t channelRef);
le_data_Technology_t dcs_ChannelGetCurrentTech(void);
void dcs_ChannelSetCurrentTech(le_data_Technology_t technology);
void dcs_ChannelSetChannelName(const char *channelName);

void dcsTechRank_Init(void);
le_data_Technology_t dcsTechRank_GetNextTech(le_data_Technology_t technology);
le_result_t dcsTechRank_SelectDataChannel(le_data_Technology_t technology);
le_dcs_Technology_t dcsTechRank_ConvertToDcsTechEnum(le_data_Technology_t leDataTech);

#endif
