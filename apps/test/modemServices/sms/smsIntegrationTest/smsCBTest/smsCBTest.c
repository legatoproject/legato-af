/**
 * This module implements the le_sms's unit tests.
  *
  * Copyright (C) Sierra Wireless Inc.
  *
  */

#include "legato.h"
#include "interfaces.h"
#include <semaphore.h>

static le_sms_RxMessageHandlerRef_t HandlerRef;

static bool CdmaTest = true;
static bool GsmTest = true;
static le_log_TraceRef_t TraceRefPdu;
static le_log_TraceRef_t TraceRefSms;

//--------------------------------------------------------------------------------------------------
/**
 * Dump the PDU
 */
//--------------------------------------------------------------------------------------------------
static void Dump
(
    const char      *label,     ///< [IN] label
    const uint8_t   *buffer,    ///< [IN] buffer
    size_t           bufferSize ///< [IN] buffer size
)
{
    uint32_t index = 0, i = 0;
    char output[65] = {0};

    LE_DEBUG("%s:",label);
    for (i=0;i<bufferSize;i++)
    {
        index += sprintf(&output[index],"%02X",buffer[i]);
        if ( !((i+1)%32) )
        {
            LE_DEBUG("%s",output);
            index = 0;
        }
    }
    LE_DEBUG("%s",output);
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SMS message reception.
 */
//--------------------------------------------------------------------------------------------------
static void TestRxHandler
(
    le_sms_MsgRef_t msg,
    void* contextPtr
)
{
    le_sms_Format_t       myformat;
    le_sms_Type_t         myType;
    uint16_t              myMessageId = 0;
    uint16_t              myMessageSerialNumber = 0;
    le_result_t           res;

    size_t                pduLen;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_BYTES];
    char                  text[LE_SMS_TEXT_MAX_BYTES] = {0};
    uint8_t               pdu[LE_SMS_PDU_MAX_BYTES] = {0};
    uint16_t              ucs2[LE_SMS_UCS2_MAX_CHARS] = {0};
    uint8_t               bin[50] = {0};

    LE_INFO("-TEST- New SMS message received ! msg.%p", msg);

    myformat = le_sms_GetFormat(msg);
    myType = le_sms_GetType(msg);

    LE_INFO("-TEST- New SMS message format %d, Type %d", myformat, myType);

    if (myType != LE_SMS_TYPE_BROADCAST_RX)
    {
        LE_ERROR("-TEST  1- Check le_sms_GetType failure! %d", myType);
        return;
    }
    else
    {
        LE_INFO("-TEST  1- Check le_sms_GetType LE_SMS_TYPE_CB");
    }

    res = le_sms_GetCellBroadcastId(msg, &myMessageId);
    if (res != LE_OK)
    {
        LE_ERROR("-TEST  2- Check le_sms_GetMessageIdCellBroadcast failure! %d", res);
        return;
    }
    else
    {
        LE_INFO("-TEST  2- Check le_sms_GetMessageIdCellBroadcast OK Message Id 0x%04X (%d)",
            myMessageId, myMessageId);
    }

    res = le_sms_GetCellBroadcastSerialNumber(msg, &myMessageSerialNumber);
    if (res != LE_OK)
    {
        LE_ERROR("-TEST  3- Check le_sms_GetCellBroadcastSerialNumber failure! %d", res);
        return;
    }
    else
    {
        LE_INFO("-TEST  3- Check le_sms_GetCellBroadcastSerialNumber OK Message Id 0x%04X (%d)",
            myMessageSerialNumber, myMessageSerialNumber);
    }

    if (myformat == LE_SMS_FORMAT_TEXT)
    {
        LE_INFO("SMS Cell Broadcast in text format");
        res = le_sms_GetTimeStamp(msg, timestamp, sizeof(timestamp));
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  4- Check le_sms_GetTimeStamp failure! %d", res);
            return;
        }
        else
        {
            LE_INFO("-TEST  4- Check le_sms_GetTimeStamp LE_NOT_PERMITTED");
        }

        res = le_sms_GetText(msg, text, sizeof(text));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  5- Check le_sms_GetText failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("SMS CB text=> '%s'",text);
            LE_INFO("-TEST  5- Check le_sms_GetText OK.");
        }

        pduLen = le_sms_GetPDULen(msg);
        if( (pduLen <= 0) || (pduLen > LE_SMS_PDU_MAX_BYTES))
        {
            LE_ERROR("-TEST  6 Check le_sms_GetPDULen failure!");
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("SMS CB Pdu len %d", (int) pduLen);
            LE_INFO("-TEST  6- Check le_sms_GetPDULen OK.");
        }

        res = le_sms_GetPDU(msg, pdu, &pduLen);
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  7 Check le_sms_GetPDU failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            Dump("SMS CB PDU", pdu, pduLen);
            LE_INFO("-TEST  7 Check le_sms_GetPDU OK.");
        }

        res = le_sms_DeleteFromStorage(msg);
        if(res != LE_NO_MEMORY)
        {
            LE_ERROR("-TEST  8 Check le_sms_DeleteFromStorage failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            Dump("SMS CB PDU", pdu, pduLen);
            LE_INFO("-TEST  8 Check le_sms_DeleteFromStorage LE_NO_MEMORY.");
        }

        res = le_sms_SetText(msg, "TOTO");
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  9 Check le_sms_SetText failure !%d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  9 Check le_sms_SetText OK.");
        }

        res = le_sms_SetDestination(msg, "0123456789");
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  10 Check le_sms_SetDestination failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  10 Check le_sms_SetDestination LE_NOT_PERMITTED.");
        }

        res = le_sms_SetBinary(msg, bin, sizeof(bin));
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  11 Check le_sms_SetBinary failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  11 Check le_sms_SetBinary LE_NOT_PERMITTED.");
        }

        size_t binLen = sizeof(bin);
        res = le_sms_GetBinary(msg, bin, &binLen);
        if(res != LE_FORMAT_ERROR)
        {
            LE_ERROR("-TEST  12 Check le_sms_GetBinary failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  12 Check le_sms_GetBinary LE_FORMAT_ERROR.");
        }
    }
    else if (myformat == LE_SMS_FORMAT_BINARY)
    {
        LE_INFO("SMS Cell Broadcast in binary format");
        res = le_sms_GetTimeStamp(msg, timestamp, sizeof(timestamp));
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  4- Check le_sms_GetTimeStamp failure! %d", res);
            return;
        }
        else
        {
            LE_INFO("-TEST  4- Check le_sms_GetTimeStamp LE_NOT_PERMITTED");
        }

        res = le_sms_GetText(msg, text, sizeof(text));
        if(res != LE_FORMAT_ERROR)
        {
            LE_ERROR("-TEST  5- Check le_sms_GetText failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("SMS CB text=> '%s'",text);
            LE_INFO("-TEST  5- Check le_sms_GetText LE_FORMAT_ERROR.");
        }

        pduLen = le_sms_GetPDULen(msg);
        if( (pduLen <= 0) || (pduLen > LE_SMS_PDU_MAX_BYTES))
        {
            LE_ERROR("-TEST  6 Check le_sms_GetPDULen failure!");
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("SMS CB Pdu len %d", (int) pduLen);
            LE_INFO("-TEST  6- Check le_sms_GetPDULen OK.");
        }

        res = le_sms_GetPDU(msg, pdu, &pduLen);
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  7 Check le_sms_GetPDU failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            Dump("SMS CB PDU", pdu, pduLen);
            LE_INFO("-TEST  7 Check le_sms_GetPDU OK.");
        }

        res = le_sms_DeleteFromStorage(msg);
        if(res != LE_NO_MEMORY)
        {
            LE_ERROR("-TEST  8 Check le_sms_DeleteFromStorage failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            Dump("SMS CB PDU", pdu, pduLen);
            LE_INFO("-TEST  8 Check le_sms_DeleteFromStorage LE_NO_MEMORY.");
        }

        res = le_sms_SetText(msg, "TOTO");
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  9 Check le_sms_SetText failure !%d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  9 Check le_sms_SetText LE_NOT_PERMITTED.");
        }

        res = le_sms_SetDestination(msg, "0123456789");
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  10 Check le_sms_SetDestination failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  10 Check le_sms_SetDestination LE_NOT_PERMITTED.");
        }

        res = le_sms_SetBinary(msg, bin, sizeof(bin));
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  11 Check le_sms_SetBinary failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  11 Check le_sms_SetBinary LE_NOT_PERMITTED.");
        }

        size_t binLen = sizeof(bin);
        res = le_sms_GetBinary(msg, bin, &binLen);
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  12 Check le_sms_GetBinary failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {

            bin[binLen] = 0;
            LE_INFO("SMS CB binary (%d)=> '%s'",(int) binLen, bin);
            LE_INFO("-TEST  12 Check le_sms_GetBinary OK.");
        }
    }
    else if (myformat == LE_SMS_FORMAT_UCS2)
    {
        const uint16_t ucs2Pattern[]= { 0x3100, 0x3200, 0x3300 };

        LE_INFO("SMS Cell Broadcast in UCS2 format");
        res = le_sms_GetTimeStamp(msg, timestamp, sizeof(timestamp));
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  4- Check le_sms_GetTimeStamp failure! %d", res);
            return;
        }
        else
        {
            LE_INFO("-TEST  4- Check le_sms_GetTimeStamp LE_NOT_PERMITTED");
        }

        res = le_sms_GetText(msg, text, sizeof(text));
        if(res != LE_FORMAT_ERROR)
        {
            LE_ERROR("-TEST  5- Check le_sms_GetUCS2 failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("SMS CB text=> '%s'",text);
            LE_INFO("-TEST  5- Check le_sms_GetUCS2 LE_FORMAT_ERROR.");
        }

        pduLen = le_sms_GetPDULen(msg);
        if( (pduLen <= 0) || (pduLen > LE_SMS_UCS2_MAX_CHARS))
        {
            LE_ERROR("-TEST  6 Check le_sms_GetPDULen failure!");
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("SMS CB Pdu len %d", (int) pduLen);
            LE_INFO("-TEST  6- Check le_sms_GetPDULen OK.");
        }

        res = le_sms_GetPDU(msg, pdu, &pduLen);
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  7 Check le_sms_GetPDU failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            Dump("SMS CB PDU", pdu, pduLen);
            LE_INFO("-TEST  7 Check le_sms_GetPDU OK.");
        }

        res = le_sms_DeleteFromStorage(msg);
        if(res != LE_NO_MEMORY)
        {
            LE_ERROR("-TEST  8 Check le_sms_DeleteFromStorage failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            Dump("SMS CB PDU", pdu, pduLen);
            LE_INFO("-TEST  8 Check le_sms_DeleteFromStorage LE_NO_MEMORY.");
        }

        res = le_sms_SetUCS2(msg, ucs2Pattern, sizeof(ucs2Pattern) / 2);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  9 Check le_sms_SetUCS2 failure !%d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  9 Check le_sms_SetUCS2 LE_NOT_PERMITTED.");
        }

        res = le_sms_SetDestination(msg, "0123456789");
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  10 Check le_sms_SetDestination failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  10 Check le_sms_SetDestination LE_NOT_PERMITTED.");
        }

        res = le_sms_SetBinary(msg, bin, sizeof(bin));
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  11 Check le_sms_SetBinary failure! %d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            LE_INFO("-TEST  11 Check le_sms_SetBinary LE_NOT_PERMITTED.");
        }

        size_t binLen = LE_SMS_UCS2_MAX_CHARS;
        res = le_sms_GetUCS2(msg, ucs2, &binLen);
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  12 Check le_sms_GetUCS2 failure !%d", res);
            LE_ERROR("FAILED !!");
            return;
        }
        else
        {
            Dump("UCS2 Dump: ", (uint8_t *) ucs2, binLen * 2);
            LE_INFO("-TEST  12 Check le_sms_GetUCS2 LE_OK");
        }
    }
    else
    {
        LE_INFO("SMS Cell Broadcast not in test format");
    }
    le_sms_Delete(msg);

    LE_INFO("smsCBTest sequence PASSED");
}



//--------------------------------------------------------------------------------------------------
/*
 * Test:
 * - le_sms_AddCellBroadcastIds()
 * - le_sms_RemoveCellBroadcastIds()
 * - le_sms_ClearCellBroadcastIds()
 */
//--------------------------------------------------------------------------------------------------
static le_result_t TestAddRemoveCellBroadcastIds
(
    void
)
{
    le_result_t res;

    res = le_sms_AddCellBroadcastIds(0,50);
    if (res != LE_OK)
    {
        LE_ERROR("le_sms_AddCellBroadcastIds FAILED");
        return LE_FAULT;
    }

    res = le_sms_AddCellBroadcastIds(0,50);
    if (res != LE_FAULT)
    {
        LE_ERROR("le_sms_AddCellBroadcastIds FAILED");
        return LE_FAULT;
    }

    res = le_sms_RemoveCellBroadcastIds(0,100);
    if (res != LE_FAULT)
    {
        LE_ERROR("TestAddRemoveCellBroadcastIds FAILED");
        return LE_FAULT;
    }

    res = le_sms_RemoveCellBroadcastIds(0,50);
    if (res != LE_OK)
    {
        LE_ERROR("TestAddRemoveCellBroadcastIds FAILED");
        return LE_FAULT;
    }

    res = le_sms_RemoveCellBroadcastIds(0,50);
    if (res != LE_FAULT)
    {
        LE_ERROR("TestAddRemoveCellBroadcastIds FAILED");
        return LE_FAULT;
    }

    res = le_sms_AddCellBroadcastIds(60,110);
    if (res != LE_OK)
    {
        LE_ERROR("le_sms_AddCellBroadcastIds FAILED");
        return LE_FAULT;
    }

    res = le_sms_ClearCellBroadcastIds();
    if (res != LE_OK)
    {
        LE_ERROR("le_sms_ClearCellBroadcastIds FAILED");
        return LE_FAULT;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/*
 * Test:
 * - le_sms_AddCdmaCellBroadcastServices()
 * - le_sms_RemoveCdmaCellBroadcastServices()
 * - le_sms_ClearCdmaCellBroadcastServices()
 */
//--------------------------------------------------------------------------------------------------
static le_result_t TestAddRemoveCDMACellBroadcastIds
(
    void
)
{
    le_result_t res;

    res = le_sms_AddCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_MAX, LE_SMS_LANGUAGE_UNKNOWN);
    if (res != LE_BAD_PARAMETER)
    {
        LE_ERROR("le_sms_AddCdmaCellBroadcastServices FAILED");
        return LE_FAULT;
    }

    res = le_sms_AddCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_UNKNOWN, LE_SMS_LANGUAGE_MAX);
    if (res != LE_BAD_PARAMETER)
    {
        LE_ERROR("le_sms_AddCdmaCellBroadcastServices FAILED");
        return LE_FAULT;
    }

    res = le_sms_AddCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_UNKNOWN, LE_SMS_LANGUAGE_UNKNOWN);
    if (res != LE_OK)
    {
        LE_ERROR("le_sms_AddCdmaCellBroadcastServices FAILED");
        return LE_FAULT;
    }

    res = le_sms_AddCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_UNKNOWN, LE_SMS_LANGUAGE_UNKNOWN);
    if (res != LE_FAULT)
    {
        LE_ERROR("le_sms_AddCdmaCellBroadcastServices FAILED");
        return LE_FAULT;
    }

    res = le_sms_RemoveCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_UNKNOWN,
                    LE_SMS_LANGUAGE_ENGLISH);
    if (res != LE_FAULT)
    {
        LE_ERROR("le_sms_RemoveCdmaCellBroadcastServices FAILED");
        return LE_FAULT;
    }

    res = le_sms_RemoveCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_UNKNOWN,
                    LE_SMS_LANGUAGE_UNKNOWN);
    if (res != LE_OK)
    {
        LE_ERROR("le_sms_RemoveCdmaCellBroadcastServices FAILED");
        return LE_FAULT;
    }

    res = le_sms_RemoveCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_UNKNOWN,
                    LE_SMS_LANGUAGE_UNKNOWN);
    if (res != LE_FAULT)
    {
        LE_ERROR("le_sms_RemoveCdmaCellBroadcastServices FAILED");
        return LE_FAULT;
    }

    res = le_sms_ClearCdmaCellBroadcastServices();
    if (res != LE_OK)
    {
        LE_ERROR("le_sms_ClearCdmaCellBroadcastServices FAILED");
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The signal event handler function for SIGINT/SIGTERM when process dies.
 */
//--------------------------------------------------------------------------------------------------
static void SigHandler
(
    int sigNum
)
{
    le_result_t res;
    bool statusPassed = true;

    LE_INFO("Deactivated SMS CB");

    if (GsmTest)
    {
        res = le_sms_ClearCellBroadcastIds();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_ClearCellBroadcastIds FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_ClearCellBroadcastIds PASSED");
        }

        res = le_sms_DeactivateCellBroadcast();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_DeactivateCellBroadcast FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_DeactivateCellBroadcast PASSED");
        }
    }

    if (CdmaTest)
    {
        res = le_sms_ClearCdmaCellBroadcastServices();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_ClearCdmaCellBroadcastServices FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_ClearCdmaCellBroadcastServices PASSED");
        }

        res = le_sms_DeactivateCdmaCellBroadcast();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_DeactivateCdmaCellBroadcast FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_DeactivateCdmaCellBroadcast PASSED");
        }
    }


    if (statusPassed)
    {
        LE_INFO("smsCBTest sequence PASSED");
        exit(EXIT_SUCCESS);
    }
    else
    {
        LE_ERROR("smsCBTest sequence FAILED");
        exit(EXIT_FAILURE);
    }
}


//--------------------------------------------------------------------------------------------------
/*
 * ME must be registered on Network with the SIM in ready state. Network have to broadcast SMS CB.
 * Test application delete all Rx SM
 * Check "logread -f | grep sms" log
 * Start app : app start smsCBTest
 * Execute app : app runProc smsCBTest --exe=smsCBTest
 * or Execute app : app runProc smsCBTest --exe=smsCBTest -- < cdma | gsm |   >
 * Wait for SMS Cell broadcast reception on the INFO trace Level.
 * Execute CTRL + C to exit from application
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_result_t res;
    bool statusPassed = true;

    TraceRefPdu = le_log_GetTraceRef( "smsPdu" );
    TraceRefSms = le_log_GetTraceRef( "sms" );

    le_log_SetFilterLevel(LE_LOG_DEBUG);
    le_log_EnableTrace(TraceRefPdu);
    le_log_EnableTrace(TraceRefSms);

    LE_INFO("PRINT USAGE => app runProc smsCBTest --exe=smsCBTest -- < cdma | gsm |   >");

    if (le_arg_NumArgs() == 1)
    {
        // This function gets the test band fom the User (interactive case).
        const char* testmode = le_arg_GetArg(0);
        if (testmode != NULL )
        {
            LE_INFO("smsCBTest argument %s", testmode);
            if ( strcmp(testmode, "cdma") == 0)
            {
                GsmTest = false;
            }
            else if ( strcmp(testmode, "gsm") == 0)
            {
                CdmaTest = false;
            }
        }
    }

    LE_INFO("smsCBTest started in CDMA %c GSM %c",
        (CdmaTest ? 'Y' : 'N'),
        (GsmTest ? 'Y' : 'N') );

    // Register a signal event handler for SIGINT when user interrupts/terminates process
    signal(SIGINT, SigHandler);

    HandlerRef = le_sms_AddRxMessageHandler(TestRxHandler, NULL);
    LE_ERROR_IF((HandlerRef == NULL), "le_sms_AddRxMessageHandler() Failed!!");


    if (GsmTest)
    {
        res = le_sms_ClearCellBroadcastIds();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_ClearCellBroadcastIds FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_ClearCellBroadcastIds PASSED");
        }

        res = le_sms_ActivateCellBroadcast();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_ActivateCellBroadcast FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_ActivateCellBroadcast PASSED");
        }

        res = le_sms_DeactivateCellBroadcast();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_DeactivateCellBroadcast FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_DeactivateCellBroadcast PASSED");
        }

        res = TestAddRemoveCellBroadcastIds();
        if (res != LE_OK)
        {
            LE_ERROR("TestAddRemoveCellBroadcastIds FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("TestAddRemoveCellBroadcastIds PASSED");
        }

        res = le_sms_ActivateCellBroadcast();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_ActivateCellBroadcast FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_ActivateCellBroadcast PASSED");
        }

        res = le_sms_AddCellBroadcastIds(1,100);
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_AddCellBroadcastIds FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_AddCellBroadcastIds PASSED");
        }

        res = le_sms_DeactivateCellBroadcast();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_DeactivateCellBroadcast FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_DeactivateCellBroadcast PASSED");
        }

        res = le_sms_ActivateCellBroadcast();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_ActivateCellBroadcast FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_ActivateCellBroadcast PASSED");
        }
    }

    if (CdmaTest)
    {
        res = le_sms_ClearCdmaCellBroadcastServices();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_ClearCdmaCellBroadcastServices FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_ClearCdmaCellBroadcastServices PASSED");
        }

        res = le_sms_ActivateCdmaCellBroadcast();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_ActivateCdmaCellBroadcast FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_ActivateCdmaCellBroadcast PASSED");
        }

        res = le_sms_DeactivateCdmaCellBroadcast();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_DeactivateCdmaCellBroadcast FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_DeactivateCdmaCellBroadcast PASSED");
        }

        res = TestAddRemoveCDMACellBroadcastIds();
        if (res != LE_OK)
        {
            LE_ERROR("TestAddRemoveCDMACellBroadcastIds FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("TestAddRemoveCDMACellBroadcastIds PASSED");
        }

        res = le_sms_AddCdmaCellBroadcastServices(LE_SMS_CDMA_SVC_CAT_UNKNOWN,
            LE_SMS_LANGUAGE_UNKNOWN);
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_AddCdmaCellBroadcastServices FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_AddCdmaCellBroadcastServices PASSED");
        }

        res = le_sms_ActivateCdmaCellBroadcast();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_ActivateCdmaCellBroadcast FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_ActivateCdmaCellBroadcast PASSED");
        }

        res = le_sms_DeactivateCdmaCellBroadcast();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_DeactivateCdmaCellBroadcast FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_DeactivateCdmaCellBroadcast PASSED");
        }

        res = le_sms_ActivateCdmaCellBroadcast();
        if (res != LE_OK)
        {
            LE_ERROR("le_sms_ActivateCdmaCellBroadcast FAILED");
            statusPassed = false;
        }
        else
        {
            LE_INFO("le_sms_ActivateCdmaCellBroadcast PASSED");
        }
    }

    if (statusPassed)
    {
        LE_INFO("smsCBTest sequence STARTED PASSED");
    }
    else
    {
        LE_ERROR("smsCBTest sequence STARTED FAILED");
    }
}
