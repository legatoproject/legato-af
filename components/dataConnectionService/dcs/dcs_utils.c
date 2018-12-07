//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's C code implementation of the utilties for the le_dcs APIs
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include "legato.h"
#include "interfaces.h"
#include "dcs.h"

static const char *DcsTechnologyNames[LE_DCS_TECH_MAX] = {"", "wifi", "cellular"};


//--------------------------------------------------------------------------------------------------
/**
 * Utility for converting a technology type enum into its name
 *
 * @return
 *     - The string name of the given technology in the function's return value
 */
//--------------------------------------------------------------------------------------------------
const char *le_dcs_ConvertTechEnumToName
(
    le_dcs_Technology_t tech
)
{
    if (tech < LE_DCS_TECH_MAX)
    {
        return DcsTechnologyNames[tech];
    }
    return "";
}


//--------------------------------------------------------------------------------------------------
/**
 * Utility for retrieving the technology type of the given channel in the 1st argument
 *
 * @return
 *     -  The technology type in enum of the given channel will be returned in the 2nd argument
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcsGetTechnology
(
    le_dcs_ChannelRef_t channelRef,
    le_dcs_Technology_t *technology
)
{
    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Failed to find channel with reference %p to get technology type",
                 channelRef);
        *technology = LE_DCS_TECH_UNKNOWN;
        return LE_FAULT;
    }

    *technology = channelDb->technology;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Utility for retrieving the current channel state of the given channel in the 1st argument
 *
 * @return
 *     - The retrieved channel state will be returned in the 2nd argument
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t dcsGetAdminState
(
    le_dcs_ChannelRef_t channelRef,
    le_dcs_State_t *state
)
{
    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Failed to find channel with reference %p to get state",
                 channelRef);
        *state = LE_DCS_STATE_DOWN;
        return LE_FAULT;
    }

    *state = (channelDb->refCount > 0) ? LE_DCS_STATE_UP : LE_DCS_STATE_DOWN;
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Utility for checking if the given channel wasn't initiated & managed via le_data APIs
 *
 * @return
 *     - true if the given channel wasn't initiated & managed via le_data APIs; false otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_dcs_ChannelManagedbyLeData
(
    le_dcs_ChannelRef_t channelRef
)
{
    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromRef(channelRef);
    if (!channelDb)
    {
        LE_ERROR("Failed to find channel with reference %p to get le_data info", channelRef);
        return false;
    }

    return channelDb->managed_by_le_data;
}


//--------------------------------------------------------------------------------------------------
/**
 * Utility for use by the le_data component to the given channel's sharing status, which is a
 * status about whether it's used by at least 1 app via the le_data APIs.  The first argument is
 * the name of the channel, the second its technology type, while the third indicates whether this
 * moment is during a start or a stop.
 */
//--------------------------------------------------------------------------------------------------
void le_dcs_MarkChannelSharingStatus
(
    const char *channelName,
    le_dcs_Technology_t tech,
    bool starting
)
{
    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromName(channelName, tech);
    if (!channelDb)
    {
        LE_ERROR("Failed to find channelDb for channel %s to update", channelName);
        return;
    }

    if (!starting)
    {
        channelDb->managed_by_le_data = false;
        channelDb->shared_with_le_data = false;
    }
    else
    {
        channelDb->shared_with_le_data = true;
        if (channelDb->refCount == 0)
        {
            channelDb->managed_by_le_data = starting;
        }
    }
    LE_DEBUG("Sharing flags: managed by le_data %d, shared with le_data %d",
             channelDb->managed_by_le_data, channelDb->shared_with_le_data);
}


//--------------------------------------------------------------------------------------------------
/**
 * Utility for use by the le_data component to check if a given channel is in use via the
 * le_dcs APIs or not.  This is reflected via the channel's refCount in its channelDb.
 * More precisely, it means the administrative state in having at least 1 app having requested for
 * use of the channel regardless of its operational state being up or down.
 *
 * @return
 *     - It returns true if at least 1 app is found to have been using this given channel via
 *       the le_dcs APIs; otherwise, false
 */
//--------------------------------------------------------------------------------------------------
bool le_dcs_ChannelIsInUse
(
    const char *channelName,
    le_dcs_Technology_t tech
)
{
    le_dcs_channelDb_t *channelDb = le_dcs_GetChannelDbFromName(channelName, tech);
    if (!channelDb)
    {
        LE_ERROR("Failed to find channelDb for channel %s to get in use status",
                 channelName);
        return false;
    }

    return (channelDb->refCount > 0) ? true : false;
}
