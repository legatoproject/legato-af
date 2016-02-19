
#include "legato.h"




static int Count = 5;
int A;
int B;




static void CountdownHandler(le_timer_Ref_t timerRef)
{
    if (Count > 0)
    {
        LE_INFO("Countdown: %d", Count);
        Count--;
    }
    else
    {
        LE_INFO("Something wicked this way comes.");
        int x = A / B;
        LE_INFO("Ain't gonna happen: %d / %d = %d.", A, B, x);
    }
}




COMPONENT_INIT
{
    LE_INFO("--=====  Bad executable has started.  =====--");

    le_timer_Ref_t theCountdown = le_timer_Create("The Final Countdown");
    le_clk_Time_t interval = { 1, 0 };

    LE_ASSERT(le_timer_SetHandler(theCountdown, CountdownHandler) == LE_OK);
    LE_ASSERT(le_timer_SetInterval(theCountdown, interval) == LE_OK);
    LE_ASSERT(le_timer_SetRepeat(theCountdown, 6) == LE_OK);

    LE_ASSERT(le_timer_Start(theCountdown) == LE_OK);

    A = 0;
    B = 0;
}
