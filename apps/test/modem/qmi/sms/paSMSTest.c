/*
 * Simple SMS demo/test program
 */

#include "legato.h"
#include "pa.h"
#include "pa_sms.h"

// Include print macros
#include "le_print.h"


void static Banner(const char *testName)
{
    int i;
    char banner[41];

    for (i=0; i<sizeof(banner)-1; i++)
        banner[i]='=';
    banner[sizeof(banner)-1] = '\0';

    LE_INFO("\n%s %s %s", banner, testName, banner);
}

//---------------------------------------------------------------------------------------------

void static TestListSMS(void)
{
    int i;
    le_result_t  result;
    uint32_t     numOfUnread, numOfRead;
    uint32_t     idxArrayRead[256]={0};
    uint32_t     idxArrayUnread[256]={0};

    Banner(__func__);

    /* Get Indexes */
    Banner("Unread List");
    result = pa_sms_ListMsgFromMem(LE_SMS_RX_UNREAD, &numOfUnread, idxArrayUnread);
    if (result != LE_OK)
    {
        LE_ERROR("pa_sms_ListMsgFromMem failed for status LE_SMS_RX_UNREAD");
    }
    else
    {
        LE_PRINT_VALUE("%u", numOfUnread);
        LE_PRINT_ARRAY("%u", numOfUnread, idxArrayUnread);
    }

    Banner("Read List");
    result = pa_sms_ListMsgFromMem(LE_SMS_RX_READ, &numOfRead, idxArrayRead);
    if (result != LE_OK)
    {
        LE_ERROR("pa_sms_ListMsgFromMem failed for status LE_SMS_RX_READ");
    }
    else
    {
        LE_PRINT_VALUE("%u", numOfRead);
        LE_PRINT_ARRAY("%u", numOfRead, idxArrayRead);
    }

    // Change the status from READ to UNREAD
    for (i=0; i<numOfRead; i++)
    {
        result = pa_sms_ChangeMessageStatus(idxArrayRead[i], LE_SMS_RX_UNREAD);
        if (result != LE_OK)
        {
            LE_ERROR("pa_sms_ChangeMessageStatus failed for index = %i", idxArrayRead[i]);
        }
    }

    // Change the status from UNREAD to READ
    for (i=0; i<numOfUnread; i++)
    {
        result = pa_sms_ChangeMessageStatus(idxArrayUnread[i], LE_SMS_RX_READ);
        if (result != LE_OK)
        {
            LE_ERROR("pa_sms_ChangeMessageStatus failed for index = %i", idxArrayUnread[i]);
        }
    }
}

//---------------------------------------------------------------------------------------------

void static StartTests
(
    void* param1Ptr,
    void* param2Ptr
)
{
    LE_INFO("Starting SMS QMI PA Test");

    TestListSMS();
}


//---------------------------------------------------------------------------------------------


COMPONENT_INIT
{
    LE_ASSERT(pa_Init() == LE_OK);

    // Start the test once the Event Loop is running.
    le_event_QueueFunction(StartTests, NULL, NULL);
}

