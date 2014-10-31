 /**
  * This module implements the le_sms's unit tests.
  *
  * Copyright (C) Sierra Wireless, Inc. 2013-2014. Use of this work is subject to license.
  *
  */

#include "legato.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <semaphore.h>

// Header files for CUnit
#include "Console.h"
#include <Basic.h>

#include "interfaces.h"
#include "main.h"

// TODO: How to automatically retrieve the target's telephone number ?
#define VOID_PATTERN  ""

#define SHORT_TEXT_TEST_PATTERN  "Short"
#define LARGE_TEXT_TEST_PATTERN  "Large Text Test pattern Large Text Test pattern Large Text Test pattern Large Text Test pattern Large Text Test pattern Large Text Test patt"
#define TEXT_TEST_PATTERN        "Text Test pattern"

#define FAIL_TEXT_TEST_PATTERN  "Fail Text Test pattern Fail Text Test pattern Fail Text Test pattern Fail Text Test pattern Fail Text Test pattern Fail Text Test pattern Fail Text Test pattern Text Test pattern "

#if 0
static int8_t PDU_TEST_PATTERN_8BITS[]={0x00,0x01,0x00,0x0A,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                      0x11,0x54,0x65,0x78,0x74,0x20,0x54,0x65,0x73,0x74,0x20,0x70,
                                      0x61,0x74,0x74,0x65,0x72,0x6E};
static int8_t PDU_TEST_PATTERN_7BITS[]={0x00,0x01,0x00,0x0A,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                      0x11,0xD4,0x32,0x9E,0x0E,0xA2,0x96,0xE7,0x74,0x10,0x3C,0x4C,
                                      0xA7,0x97,0xE5,0x6E};
#endif

static uint8_t PDU_TEST_PATTERN_7BITS[]={0x00,0x01,0x00,0x0A,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                         0x11,0xD4,0x32,0x9E,0x0E,0xA2,0x96,0xE7,0x74,0x10,0x3C,0x4C,
                                         0xA7,0x97,0xE5,0x6E};

static uint8_t BINARY_TEST_PATTERN[]={0x05,0x01,0x00,0x0A};

#ifndef AUTOMATIC
static char DEST_TEST_PATTERN[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
#else
#define DEST_TEST_PATTERN  "XXXXXXXXXXXX"
#endif

static le_sms_MsgRef_t              receivedTextMsg;
static le_sms_RxMessageHandlerRef_t testHdlrRef;

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SMS message reception.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestRxHandler(le_sms_MsgRef_t msg, void* contextPtr)
{
    le_sms_Format_t       myformat;
    le_sms_Status_t       mystatus;
    le_result_t           res;
    char                  tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_LEN];
    char                  text[LE_SMS_TEXT_MAX_BYTES] = {0};
    size_t                uintval;

    LE_INFO("-TEST- New SMS message received ! msg.%p", msg);
    myformat=le_sms_GetFormat(msg);
    if (myformat == LE_SMS_FORMAT_TEXT)
    {
        receivedTextMsg=msg;

        res=le_sms_GetSenderTel(msg, tel, 1);
        if(res != LE_OVERFLOW)
        {
            LE_ERROR("-TEST 1/13- Check le_sms_GetSenderTel failure (LE_OVERFLOW expected) !");
        }
        else
        {
            LE_INFO("-TEST 1/13- Check le_sms_GetSenderTel passed (LE_OVERFLOW expected).");
        }

        res=le_sms_GetSenderTel(msg, tel, sizeof(tel));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST 2/13- Check le_sms_GetSenderTel failure (LE_OK expected) !");
        }
        else
        {
            LE_INFO("-TEST 2/13- Check le_sms_GetSenderTel passed (%s) (LE_OK expected).", tel);
        }

        if(strncmp(&tel[strlen(tel)-4], &DEST_TEST_PATTERN[strlen(DEST_TEST_PATTERN)-4], 4))
        {
            LE_ERROR("-TEST  3/13- Check le_sms_GetSenderTel, bad Sender Telephone number! (%s)", tel);
        }
        else
        {
            LE_INFO("-TEST  3/13- Check le_sms_GetSenderTel, Sender Telephone number OK.");
        }

        uintval=le_sms_GetUserdataLen(msg);
        if((uintval != strlen(TEXT_TEST_PATTERN)) &&
           (uintval != strlen(SHORT_TEXT_TEST_PATTERN)) &&
           (uintval != strlen(LARGE_TEXT_TEST_PATTERN)))
        {
            LE_ERROR("-TEST  4/13- Check le_sms_GetLen, bad expected text length! (%zd)", uintval);
        }
        else
        {
            LE_INFO("-TEST  4/13- Check le_sms_GetLen OK.");
        }


        res=le_sms_GetTimeStamp(msg, timestamp, 1);
        if(res != LE_OVERFLOW)
        {
            LE_ERROR("-TEST  5/13- Check le_sms_GetTimeStamp -LE_OVERFLOW error- failure!");
        }
        else
        {
            LE_INFO("-TEST  5/13- Check le_sms_GetTimeStamp -LE_OVERFLOW error- OK.");
        }
        res=le_sms_GetTimeStamp(msg, timestamp, sizeof(timestamp));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  6/13- Check le_sms_GetTimeStamp failure!");
        }
        else
        {
            LE_INFO("-TEST  6/13- Check le_sms_GetTimeStamp OK (%s).", timestamp);
        }

        res=le_sms_GetText(msg, text, sizeof(text));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  7/13- Check le_sms_GetText failure!");
        }
        else
        {
            LE_INFO("-TEST  7/13- Check le_sms_GetText OK.");
        }

        if((strncmp(text, TEXT_TEST_PATTERN, strlen(TEXT_TEST_PATTERN)) != 0) &&
           (strncmp(text, SHORT_TEXT_TEST_PATTERN, strlen(SHORT_TEXT_TEST_PATTERN)) != 0) &&
           (strncmp(text, LARGE_TEXT_TEST_PATTERN, strlen(LARGE_TEXT_TEST_PATTERN)) != 0)
        )
        {
            LE_ERROR("-TEST  8/13- Check le_sms_GetText, bad expected received text! (%s)", text);
        }
        else
        {
            LE_INFO("-TEST  8/13- Check le_sms_GetText, received text OK.");
        }

        // Verify that the message is read-only
        res=le_sms_SetDestination(msg, DEST_TEST_PATTERN);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  9/13- Check le_sms_SetDestination, parameter check failure!");
        }
        else
        {
            LE_INFO("-TEST  9/13- Check le_sms_SetDestination OK.");
        }

        res=le_sms_SetText(msg, TEXT_TEST_PATTERN);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  10/13- Check le_sms_SetText, parameter check failure!");
        }
        else
        {
            LE_INFO("-TEST  10/13- Check le_sms_SetText OK.");
        }

        // Verify Mark Read/Unread functions
        le_sms_MarkRead(msg);

        mystatus=le_sms_GetStatus(msg);
        if(mystatus != LE_SMS_RX_READ)
        {
            LE_ERROR("-TEST  11/13- Check le_sms_GetStatus, bad status (%d)!", mystatus);
        }
        else
        {
            LE_INFO("-TEST  11/13- Check le_sms_GetStatus, status OK.");
        }

        le_sms_MarkUnread(msg);

        mystatus=le_sms_GetStatus(msg);
        if(mystatus != LE_SMS_RX_UNREAD)
        {
            LE_ERROR("-TEST  12/13- Check le_sms_GetStatus, bad status (%d)!", mystatus);
        }
        else
        {
            LE_INFO("-TEST  12/13- Check le_sms_GetStatus, status OK.");
        }

        res=le_sms_DeleteFromStorage(msg);
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  13/13- Check le_sms_DeleteFromStorage failure!");
        }
        else
        {
            LE_INFO("-TEST  13/13- Check le_sms_DeleteFromStorage OK.");
        }
    }
    else
    {
        LE_WARN("-TEST- I check only Text message!");
    }

    le_sms_Delete(msg);
}

#ifndef AUTOMATIC
//--------------------------------------------------------------------------------------------------
/**
 * This function gets the telephone number from the User (interactive case).
 *
 */
//--------------------------------------------------------------------------------------------------
void GetTel(void)
{
    char *strPtr;

    do
    {
        fprintf(stderr, "Please enter the device's telephone number to perform the SMS tests: \n");
        strPtr=fgets ((char*)DEST_TEST_PATTERN, LE_MDMDEFS_PHONE_NUM_MAX_BYTES, stdin);
    }while (strlen(strPtr) == 0);

    DEST_TEST_PATTERN[strlen(DEST_TEST_PATTERN)-1]='\0';
}
#endif


//--------------------------------------------------------------------------------------------------
//                                       Test Functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/**
 * Test: Text Message Object Set/Get APIS.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sms_SetGetText()
{
    le_result_t           res;
    le_sms_MsgRef_t       myMsg;
    le_sms_Format_t       myformat;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_LEN];
    char                  tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    char                  text[LE_SMS_TEXT_MAX_BYTES] = {0};
    size_t                uintval;

    myMsg = le_sms_Create();
    CU_ASSERT_PTR_NOT_NULL(myMsg);

    res=le_sms_SetDestination(myMsg, DEST_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_SetText(myMsg, TEXT_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    myformat=le_sms_GetFormat(myMsg);
    CU_ASSERT_EQUAL(myformat, LE_SMS_FORMAT_TEXT);

    res=le_sms_GetSenderTel(myMsg, tel, sizeof(tel));
    CU_ASSERT_EQUAL(res, LE_NOT_PERMITTED);

    res=le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp));
    CU_ASSERT_EQUAL(res, LE_NOT_PERMITTED);

    uintval=le_sms_GetUserdataLen(myMsg);
    CU_ASSERT_EQUAL(uintval, strlen(TEXT_TEST_PATTERN));

    res=le_sms_GetText(myMsg, text, 1);
    CU_ASSERT_EQUAL(res, LE_OVERFLOW);

    res=le_sms_GetText(myMsg, text, sizeof(text));
    CU_ASSERT_EQUAL(res, LE_OK);
    CU_ASSERT_STRING_EQUAL(text, TEXT_TEST_PATTERN);

    res=le_sms_SetDestination(myMsg, VOID_PATTERN);
    CU_ASSERT_EQUAL(res, LE_BAD_PARAMETER);

    res=le_sms_SetText(myMsg, VOID_PATTERN);
    CU_ASSERT_EQUAL(res, LE_BAD_PARAMETER);

    le_sms_Delete(myMsg);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Raw binary Message Object Set/Get APIS.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sms_SetGetBinary()
{
    le_result_t           res;
    le_sms_MsgRef_t       myMsg;
    le_sms_Format_t       myformat;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_LEN];
    char                  tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    uint8_t               raw[LE_SMS_BINARY_MAX_LEN];
    size_t                uintval;
    uint32_t              i;

    myMsg = le_sms_Create();
    CU_ASSERT_PTR_NOT_NULL(myMsg);

    res=le_sms_SetDestination(myMsg, DEST_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, sizeof(BINARY_TEST_PATTERN));
    CU_ASSERT_EQUAL(res, LE_OK);

    myformat=le_sms_GetFormat(myMsg);
    CU_ASSERT_EQUAL(myformat, LE_SMS_FORMAT_BINARY);

    res=le_sms_GetSenderTel(myMsg, tel, sizeof(tel));
    CU_ASSERT_EQUAL(res, LE_NOT_PERMITTED);

    res=le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp));
    CU_ASSERT_EQUAL(res, LE_NOT_PERMITTED);

    uintval=le_sms_GetUserdataLen(myMsg);
    CU_ASSERT_EQUAL(uintval, sizeof(BINARY_TEST_PATTERN));

    uintval=1;
    res=le_sms_GetBinary(myMsg, raw, &uintval);
    CU_ASSERT_EQUAL(res, LE_OVERFLOW);

    uintval=sizeof(BINARY_TEST_PATTERN);
    res=le_sms_GetBinary(myMsg, raw, &uintval);
    CU_ASSERT_EQUAL(res, LE_OK);
    for(i=0; i<sizeof(BINARY_TEST_PATTERN) ; i++)
    {
        CU_ASSERT_EQUAL(raw[i], BINARY_TEST_PATTERN[i]);
    }
    CU_ASSERT_EQUAL(uintval, sizeof(BINARY_TEST_PATTERN));

    res=le_sms_SetDestination(myMsg, VOID_PATTERN);
    CU_ASSERT_EQUAL(res, LE_BAD_PARAMETER);

    res=le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, 0);
    CU_ASSERT_EQUAL(res, LE_BAD_PARAMETER);

    le_sms_Delete(myMsg);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: PDU Message Object Set/Get APIS.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sms_SetGetPDU()
{
    le_result_t           res;
    le_sms_MsgRef_t       myMsg;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_LEN];
    char                  tel[LE_MDMDEFS_PHONE_NUM_MAX_BYTES];
    uint8_t               pdu[LE_SMS_PDU_MAX_LEN];
    size_t                uintval;
    uint32_t              i;

    myMsg = le_sms_Create();
    CU_ASSERT_PTR_NOT_NULL(myMsg);

    res=le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]));
    LE_INFO("le_sms_SetPDU return %d",res);

    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_GetSenderTel(myMsg, tel, sizeof(tel));
    CU_ASSERT_EQUAL(res, LE_NOT_PERMITTED);

    res=le_sms_GetTimeStamp(myMsg, timestamp, sizeof(timestamp));
    CU_ASSERT_EQUAL(res, LE_NOT_PERMITTED);

    uintval=le_sms_GetPDULen(myMsg);
    CU_ASSERT_EQUAL(uintval, sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]));

    uintval=1;
    res=le_sms_GetPDU(myMsg, pdu, &uintval);
    CU_ASSERT_EQUAL(res, LE_OVERFLOW);

    uintval=sizeof(pdu);
    res=le_sms_GetPDU(myMsg, pdu, &uintval);
    CU_ASSERT_EQUAL(res, LE_OK);
    for(i=0; i<sizeof(PDU_TEST_PATTERN_7BITS) ; i++)
    {
        CU_ASSERT_EQUAL(pdu[i], PDU_TEST_PATTERN_7BITS[i]);
    }
    CU_ASSERT_EQUAL(uintval, sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]));

    res=le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, 0);
    CU_ASSERT_EQUAL(res, LE_BAD_PARAMETER);

    le_sms_Delete(myMsg);
}

/*
 * Test case le_sms_GetSmsCenterAddress() and le_sms_SetSmsCenterAddress() APIs
 */
void Testle_sms_SetGetSmsCenterAddress
(
)
{
    le_result_t           res;
    char smscMdmRefStr[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};
    char smscMdmStr[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = {0};
    char smscStrs[LE_MDMDEFS_PHONE_NUM_MAX_BYTES] = "+33123456789";

    // Get current SMS service center address.
    // Check LE_OVERFLOW error case
    res = le_sms_GetSmsCenterAddress(smscMdmRefStr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES-2);
    CU_ASSERT_EQUAL(res, LE_OVERFLOW);

    // Get current SMS service center address.
    res = le_sms_GetSmsCenterAddress(smscMdmRefStr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
    CU_ASSERT_EQUAL(res, LE_OK);

    // Set "+33123456789" SMS service center address.
    res = le_sms_SetSmsCenterAddress(smscStrs);
    CU_ASSERT_EQUAL(res, LE_OK);

    // Get current SMS service center address.
    res = le_sms_GetSmsCenterAddress(smscMdmStr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES);
    CU_ASSERT_EQUAL(res, LE_OK);

    // Restore previous SMS service center address.
    res = le_sms_SetSmsCenterAddress(smscMdmRefStr);
    CU_ASSERT_EQUAL(res, LE_OK);

    // check if value get match with value set.
    CU_ASSERT_EQUAL(strncmp(smscStrs,smscMdmStr, LE_MDMDEFS_PHONE_NUM_MAX_BYTES), 0);
}


//--------------------------------------------------------------------------------------------------
/**
 * Test: Send a Text message.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sms_SendText()
{
    le_result_t           res;
    le_sms_MsgRef_t       myMsg;

    myMsg = le_sms_Create();
    CU_ASSERT_PTR_NOT_NULL(myMsg);

    LE_DEBUG("-TEST- Create Msg %p", myMsg);

    testHdlrRef=le_sms_AddRxMessageHandler(TestRxHandler, NULL);
    CU_ASSERT_PTR_NOT_NULL(testHdlrRef);

    res=le_sms_SetDestination(myMsg, DEST_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_SetText(myMsg, LARGE_TEXT_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_Send(myMsg);
    CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
    CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);

    res=le_sms_SetText(myMsg, SHORT_TEXT_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_Send(myMsg);
    CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
    CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);

    le_sms_Delete(myMsg);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Send a raw binary message.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sms_SendBinary()
{
    le_result_t           res;
    le_sms_MsgRef_t       myMsg;

    myMsg = le_sms_Create();
    CU_ASSERT_PTR_NOT_NULL(myMsg);

    LE_DEBUG("-TEST- Create Msg %p", myMsg);

    res=le_sms_SetDestination(myMsg, DEST_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_SetBinary(myMsg, BINARY_TEST_PATTERN, sizeof(BINARY_TEST_PATTERN)/sizeof(BINARY_TEST_PATTERN[0]));
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_Send(myMsg);
    CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
    CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);

    le_sms_Delete(myMsg);
}

#if 0
//--------------------------------------------------------------------------------------------------
/**
 * Test: Send a Pdu message.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sms_SendPdu()
{
    le_result_t           res;
    le_sms_MsgRef_t      myMsg;

    myMsg = le_sms_Create();
    CU_ASSERT_PTR_NOT_NULL(myMsg);

    LE_DEBUG("Create Msg %p", myMsg);

    res=le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, sizeof(PDU_TEST_PATTERN_7BITS)/sizeof(PDU_TEST_PATTERN_7BITS[0]));
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_Send(myMsg);
    CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
    CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);

    res=le_sms_SetPDU(myMsg, PDU_TEST_PATTERN_8BITS, sizeof(PDU_TEST_PATTERN_8BITS)/sizeof(PDU_TEST_PATTERN_8BITS[0]));
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_Send(myMsg);
    CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
    CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);

    le_sms_Delete(myMsg);
}
#endif

//--------------------------------------------------------------------------------------------------
/**
 * Test: Check Received List.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sms_ReceivedList()
{
    le_result_t              res;
    le_sms_MsgRef_t         myMsg;
    le_sms_MsgRef_t         lMsg1, lMsg2;
    le_sms_MsgListRef_t     receivedList;
    le_sms_Status_t         mystatus;

    myMsg = le_sms_Create();
    CU_ASSERT_PTR_NOT_NULL(myMsg);

    res=le_sms_SetDestination(myMsg, DEST_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_SetText(myMsg, TEXT_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_Send(myMsg);
    CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
    CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);

    res=le_sms_Send(myMsg);
    CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
    CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);
    if (res == LE_OK)
    {
        uint32_t i=0;

        fprintf(stderr, "\nPlease ensure that the device has received at least 2 messages, then press enter\n");
        while ( getchar() != '\n' );

        if ((res == LE_OK) && (i<10))
        {
            // List Received messages
            receivedList=le_sms_CreateRxMsgList();
            CU_ASSERT_PTR_NOT_NULL(receivedList);

            lMsg1=le_sms_GetFirst(receivedList);
            CU_ASSERT_PTR_NOT_NULL(lMsg1);
            if (lMsg1 != NULL)
            {
                mystatus=le_sms_GetStatus(lMsg1);
                CU_ASSERT_TRUE((mystatus==LE_SMS_RX_READ)||(mystatus==LE_SMS_RX_UNREAD));
                LE_INFO("-TEST- Delete Rx message 1.%p", lMsg1);
                le_sms_Delete(lMsg1);
            }

            lMsg2=le_sms_GetNext(receivedList);
            CU_ASSERT_PTR_NOT_NULL(lMsg2);
            if (lMsg2 != NULL)
            {
                mystatus=le_sms_GetStatus(lMsg2);
                CU_ASSERT_TRUE((mystatus==LE_SMS_RX_READ)||(mystatus==LE_SMS_RX_UNREAD));
                LE_INFO("-TEST- Delete Rx message 2.%p", lMsg2);
                le_sms_Delete(lMsg2);
            }

            LE_INFO("-TEST- Delete the ReceivedList");
            le_sms_DeleteList(receivedList);
        }
        else
        {
            LE_ERROR("-TEST- Unable to complete Testle_sms_ReceivedList Test");
        }
    }
    else
    {
        LE_ERROR("-TEST- Unable to complete Testle_sms_ReceivedList Test");
    }

    // Delete sent message
    le_sms_Delete(myMsg);
}

