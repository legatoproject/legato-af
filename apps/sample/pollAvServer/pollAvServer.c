#include "legato.h"
#include "swi_airvantage.h"

// -------------------------------------------------------------------------------------------------
/**
 *  Default timeout that will determine how often we will poll the AirVantage server.
 */
// -------------------------------------------------------------------------------------------------
#define DEFAULT_TIMEOUT 90 // seconds

// -------------------------------------------------------------------------------------------------
/**
 *  Boolean to check whether AV library has been initialized.
 */
// -------------------------------------------------------------------------------------------------
static bool IsInitialized = false;

//--------------------------------------------------------------------------------------------------
/**
 * Expiry handler function that will poll the AirVantage server.
 */
//--------------------------------------------------------------------------------------------------
static void PollServer
(
    le_timer_Ref_t timerRef
)
{
    rc_ReturnCode_t res;

    if (!IsInitialized)
    {
        res = swi_av_Init();

        if (res == RC_OK)
        {
            IsInitialized = true;
        }
        else
        {
            return;
        }
    }

    LE_INFO("Polling AirVantage server.");
    res = swi_av_ConnectToServer(SWI_AV_CX_SYNC);

    if (res != RC_OK)
    {
        LE_INFO("Failed to poll AirVantage server.");
        return;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Start polling AirVantage server.
 */
//--------------------------------------------------------------------------------------------------
static void StartServerPoll
(
    void
)
{
    le_result_t res = LE_FAULT;

    le_timer_Ref_t timerRef = NULL;
    le_clk_Time_t interval = { DEFAULT_TIMEOUT, 0 };

    timerRef = le_timer_Create("PollAvServerTimer");

    res = le_timer_SetInterval(timerRef, interval);
    LE_FATAL_IF(res != LE_OK, "Unable to set timer interval.");

    res = le_timer_SetRepeat(timerRef, 0);
    LE_FATAL_IF(res != LE_OK, "Unable to set repeat for timer.");

    res = le_timer_SetHandler(timerRef, PollServer);
    LE_FATAL_IF(res != LE_OK, "Unable to set timer handler.");

    res = le_timer_Start(timerRef);
    LE_FATAL_IF(res != LE_OK, "Unable to start timer.");
}


COMPONENT_INIT
{
    StartServerPoll();
}
