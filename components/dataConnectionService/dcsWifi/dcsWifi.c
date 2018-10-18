//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's C code implementation of its southbound interfaces with the Wifi
 *  component.
 *
 *  Copyright (C) Sierra Wireless Inc.
 *
 */
//--------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include "legato.h"
#include "interfaces.h"
#include "le_print.h"
#include "dcsWifi.h"


//--------------------------------------------------------------------------------------------------
/**
 * Function to call Wifi to get the list of all available channels
 *
 * @return
 *     - The 1st input argument is to be filled out with the retrieved list where the 2nd input
 *       specifies how long this given list's size is for filling out
 *     - The function returns LE_OK upon a successful retrieval; otherwise, some other
 *       le_result_t failure cause
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dcsWifi_GetChannelList
(
    le_dcs_ChannelInfo_t *channelList,
    int16_t *listSize
)
{
    le_result_t ret = LE_UNSUPPORTED;
    *listSize = 0;
    if (ret != LE_OK)
    {
        LE_ERROR("Failed to get Wifi's channel list; error: %d", ret);
    }
    return ret;
}

// ToDo: Further functions & support for Wifi are to be added


//--------------------------------------------------------------------------------------------------
/**
 *  Wifi handlers component initialization
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    LE_INFO("Data Channel Service's Wifi Handlers component is ready");
}
