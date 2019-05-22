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
 * Utility for retrieving the current channel state of the given channel in the 1st argument
 *
 * @return
 *     - The retrieved channel state will be returned in the 2nd argument
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other le_result_t
 *       failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcs_GetAdminState
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
 * Utility for converting an le_dcs event into a string for printing
 *
 * @return
 *     - The string name of the given le_dcs event
 */
//--------------------------------------------------------------------------------------------------
const char *le_dcs_ConvertEventToString
(
    le_dcs_Event_t event
)
{
    switch (event)
    {
        case LE_DCS_EVENT_UP:
            return "Up";
        case LE_DCS_EVENT_DOWN:
            return "Down";
        case LE_DCS_EVENT_TEMP_DOWN:
            return "Temporary Down";
        default:
            return "Unknown";
    }
}
