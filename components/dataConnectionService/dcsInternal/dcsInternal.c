//--------------------------------------------------------------------------------------------------
/**
 *  Data Channel Server's internal component for internal use which includes triggering an initial
 *  channel list query during Legato startup and generating an internal client session reference
 *  for le_dcs that will be used by le_data.
 *
 *  This internal component and process is also intended for more use in the future by le_dcs for
 *  handling slow per-connection processing, e.g. DNS config completion, route settling, as well as
 *  platform-specific quiet time between Down & Up states, etc. The legacy le_data implementation
 *  performs additional sleeps in the main DCS process to handle such specific needs, but that is
 *  not a good design to allow one channel to hold off all the others. With this internal process,
 *  any of such per-channel needs can be serviced independently.
 *
 *  Copyright (C) Sierra Wireless Inc.
 */
//--------------------------------------------------------------------------------------------------

#include "legato.h"
#include "interfaces.h"
#include "watchdogChain.h"


//--------------------------------------------------------------------------------------------------
/**
 * Timer values:
 * MS_WDOG_INTERVAL - interval to kick the watchdog
 * STARTUP_CHANNEL_SCAN_WAIT - startup initial channel list query
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 60
#define STARTUP_CHANNEL_SCAN_WAIT 5


//--------------------------------------------------------------------------------------------------
/**
 *  This is the event handler to be added for getting the initial list of channels created. But
 *  inside there's no need of any logic, since the wanted initialization to be done is inside
 *  le_dcs.
 */
//--------------------------------------------------------------------------------------------------
static void dcsInternalChannelQueryHandler
(
    le_result_t result,
    const le_dcs_ChannelInfo_t *channelList,
    size_t channelListSize,
    void *contextPtr
)
{
    LE_INFO("Internal list of data channels in le_dcs initialized");
}


//--------------------------------------------------------------------------------------------------
/**
 *  Server initialization
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_dcs_ConnectService();

    sleep(STARTUP_CHANNEL_SCAN_WAIT);

    LE_INFO("Initializing data channels");
    le_dcs_GetChannels(dcsInternalChannelQueryHandler, NULL);

    // Register main loop with watchdog chain
    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);
}
