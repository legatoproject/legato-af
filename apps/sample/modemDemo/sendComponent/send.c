#include "legato.h"
#include "interfaces.h"

#define MAX_TEL_NUMBER 15
#define MAX_SMS_TEXT   160

static void PrintUsage()
{
    int idx;
    bool sandboxed = (getuid() != 0);
    const char * usagePtr[] = {
            "Usage of the 'send' tool is:",
            "   send <destNumber> <textMessage>",
            "",
            "Warning:",
            "   If the <textMessage> is a known command, it will",
            "   only be handled locally and not sent to the requested",
            "   destination.",
    };

    for(idx = 0; idx < NUM_ARRAY_MEMBERS(usagePtr); idx++)
    {
        if(sandboxed)
        {
            LE_INFO("%s", usagePtr[idx]);
        }
        else
        {
            fprintf(stderr, "%s\n", usagePtr[idx]);
        }
    }
}

COMPONENT_INIT
{
    le_result_t result;

    send_StartClient("send");

    char destNum[MAX_TEL_NUMBER] = { 0 };
    char messageText[MAX_SMS_TEXT] = { 0 };

    if (le_arg_NumArgs() == 2)
    {
        le_arg_GetArg(0, destNum, MAX_TEL_NUMBER);
        le_arg_GetArg(1, messageText, MAX_SMS_TEXT);

        result = send_SendMessage(destNum, messageText);

        if(result != LE_OK)
        {
            LE_FATAL("Unable to send message.");
        }
    }
    else
    {
        PrintUsage();
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
