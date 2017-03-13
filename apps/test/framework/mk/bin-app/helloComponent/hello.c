
#include "legato.h"
#include "interfaces.h"




le_timer_Ref_t MyTimerRef;


static void GreetFunc(void)
{
    le_cfg_IteratorRef_t iterRef = le_cfg_CreateReadTxn("");
    bool stopPrinting = le_cfg_GetBool(iterRef, "stopPrinting", false);

    if (!stopPrinting)
    {
        char nameStr[LE_CFG_STR_LEN_BYTES] = "";
        le_result_t result;

        result = le_cfg_GetString(iterRef, "greetMessage", nameStr, sizeof(nameStr), "world");

        if (result != LE_OK)
        {
            LE_ERROR("Message string could not be read.");
        }
        else
        {
            greet_CustomGreet(nameStr);
        }
    }

    le_cfg_CancelTxn(iterRef);
}


static void OnTimerTick(le_timer_Ref_t timerRef)
{
    GreetFunc();
}


static void ShowInfo(void)
{
     char imeiStr[LE_INFO_IMEI_MAX_BYTES] = "--";
     char versionStr[LE_INFO_MAX_VERS_BYTES] = "--";
     le_result_t result;

     // Read some info about the modem core, and print it to the user.
     result = le_info_GetImei(imeiStr, sizeof(imeiStr));
     LE_ERROR_IF(result != LE_OK, "Failed to get IMEI, %s", LE_RESULT_TXT(result));

     result = le_info_GetFirmwareVersion(versionStr, sizeof(versionStr));
     LE_ERROR_IF(result != LE_OK, "Failed to get FW version, %s", LE_RESULT_TXT(result));

     LE_INFO("Hello App running on FW: %s, and modem IMEI: %s", versionStr, imeiStr);
}


void greet_CustomGreet(const char* nameStr)
{
    LE_INFO("Greetings, %s.", nameStr);
}


void greet_StandardGreet(void)
{
    GreetFunc();
}


COMPONENT_INIT
{
     MyTimerRef = le_timer_Create("My Timer");

     // Create a timer that ticks forever, at 30 second intervals.
     LE_ASSERT(le_timer_SetMsInterval(MyTimerRef, 30000) == LE_OK);
     LE_ASSERT(le_timer_SetRepeat(MyTimerRef, 0) == LE_OK);
     LE_ASSERT(le_timer_SetHandler(MyTimerRef, OnTimerTick) == LE_OK);

     LE_ASSERT(le_timer_Start(MyTimerRef) == LE_OK);

     ShowInfo();
}
