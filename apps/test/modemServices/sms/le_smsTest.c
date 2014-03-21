 /**
  * This module implements the le_sms's unit tests.
  *
  * Copyright (C) Sierra Wireless, Inc. 2013.  All rights reserved. Use of this work is subject to license.
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

#include "le_sms.h"
#include "pa_sms.h"
#include "main.h"


// TODO: How to automatically retrieve the target's telephone number ?
#define VOID_PATTERN  ""

#define SHORT_TEXT_TEST_PATTERN  "Short"
#define LARGE_TEXT_TEST_PATTERN  "Large Text Test pattern Large Text Test pattern Large Text Test pattern Large Text Test pattern Large Text Test pattern Large Text Test patt"
#define TEXT_TEST_PATTERN        "Text Test pattern"

#define FAIL_DEST_TEST_PATTERN  "+3360607080910111213"
#define FAIL_TEXT_TEST_PATTERN  "Fail Text Test pattern Fail Text Test pattern Fail Text Test pattern Fail Text Test pattern Fail Text Test pattern Fail Text Test pattern Fail Text Test pattern Text Test pattern "


// static int8_t PDU_TEST_PATTERN_8BITS[]={0x00,0x01,0x00,0x0A,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                                       0x11,0x54,0x65,0x78,0x74,0x20,0x54,0x65,0x73,0x74,0x20,0x70,
//                                       0x61,0x74,0x74,0x65,0x72,0x6E};
// static int8_t PDU_TEST_PATTERN_7BITS[]={0x00,0x01,0x00,0x0A,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                                       0x11,0xD4,0x32,0x9E,0x0E,0xA2,0x96,0xE7,0x74,0x10,0x3C,0x4C,
//                                       0xA7,0x97,0xE5,0x6E};

static uint8_t PDU_TEST_PATTERN_7BITS[]={0x00,0x01,0x00,0x0A,0x81,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                         0x11,0xD4,0x32,0x9E,0x0E,0xA2,0x96,0xE7,0x74,0x10,0x3C,0x4C,
                                         0xA7,0x97,0xE5,0x6E};

static uint8_t BINARY_TEST_PATTERN[]={0x05,0x01,0x00,0x0A};

static uint8_t FAIL_BINARY_TEST_PATTERN[]={0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,
                                           0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,
                                           0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,
                                           0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,
                                           0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,
                                           0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,
                                           0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,
                                           0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,
                                           0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,
                                           0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A,0x05,0x01,0x00,0x0A};

#ifndef AUTOMATIC
static char DEST_TEST_PATTERN[LE_SMS_TEL_NMBR_MAX_LEN];
#else
#define DEST_TEST_PATTERN  "XXXXXXXXXXXX"
#endif



static le_sms_msg_Ref_t              receivedTextMsg;
static le_sms_msg_RxMessageHandlerRef_t testHdlrRef;

//--------------------------------------------------------------------------------------------------
/**
 * Handler function for SMS message reception.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TestRxHandler(le_sms_msg_Ref_t msg, void* contextPtr)
{
    le_sms_msg_Format_t   myformat;
    le_sms_msg_Status_t   mystatus;
    le_result_t           res;
    char                  tel[LE_SMS_TEL_NMBR_MAX_LEN];
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_LEN];
    char                  text[LE_SMS_TEXT_MAX_LEN];
    size_t                uintval;

    LE_INFO("-TEST- New SMS message received ! msg.%p", msg);
    myformat=le_sms_msg_GetFormat(msg);
    if (myformat == LE_SMS_FORMAT_TEXT)
    {
        receivedTextMsg=msg;

        res=le_sms_msg_GetSenderTel(msg, tel, 1);
        if(res != LE_OVERFLOW)
        {
            LE_ERROR("-TEST 1/13- Check le_sms_msg_GetSenderTel failure (LE_OVERFLOW expected) !");
        }
        else
        {
            LE_INFO("-TEST 1/13- Check le_sms_msg_GetSenderTel passed (LE_OVERFLOW expected).");
        }

        res=le_sms_msg_GetSenderTel(msg, tel, sizeof(tel));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST 2/13- Check le_sms_msg_GetSenderTel failure (LE_OK expected) !");
        }
        else
        {
            LE_INFO("-TEST 2/13- Check le_sms_msg_GetSenderTel passed (%s) (LE_OK expected).", tel);
        }

        if(strncmp(&tel[strlen(tel)-4], &DEST_TEST_PATTERN[strlen(DEST_TEST_PATTERN)-4], 4))
        {
            LE_ERROR("-TEST  3/13- Check le_sms_msg_GetSenderTel, bad Sender Telephone number! (%s)", tel);
        }
        else
        {
            LE_INFO("-TEST  3/13- Check le_sms_msg_GetSenderTel, Sender Telephone number OK.");
        }

        uintval=le_sms_msg_GetUserdataLen(msg);
        if((uintval != strlen(TEXT_TEST_PATTERN)) &&
           (uintval != strlen(SHORT_TEXT_TEST_PATTERN)) &&
           (uintval != strlen(LARGE_TEXT_TEST_PATTERN)))
        {
            LE_ERROR("-TEST  4/13- Check le_sms_msg_GetLen, bad expected text length! (%zd)", uintval);
        }
        else
        {
            LE_INFO("-TEST  4/13- Check le_sms_msg_GetLen OK.");
        }


        res=le_sms_msg_GetTimeStamp(msg, timestamp, 1);
        if(res != LE_OVERFLOW)
        {
            LE_ERROR("-TEST  5/13- Check le_sms_msg_GetTimeStamp -LE_OVERFLOW error- failure!");
        }
        else
        {
            LE_INFO("-TEST  5/13- Check le_sms_msg_GetTimeStamp -LE_OVERFLOW error- OK.");
        }
        res=le_sms_msg_GetTimeStamp(msg, timestamp, sizeof(timestamp));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  6/13- Check le_sms_msg_GetTimeStamp failure!");
        }
        else
        {
            LE_INFO("-TEST  6/13- Check le_sms_msg_GetTimeStamp OK (%s).", timestamp);
        }

        res=le_sms_msg_GetText(msg, text, sizeof(text));
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  7/13- Check le_sms_msg_GetText failure!");
        }
        else
        {
            LE_INFO("-TEST  7/13- Check le_sms_msg_GetText OK.");
        }

        if((strncmp(text, TEXT_TEST_PATTERN, strlen(TEXT_TEST_PATTERN)) != 0) &&
           (strncmp(text, SHORT_TEXT_TEST_PATTERN, strlen(SHORT_TEXT_TEST_PATTERN)) != 0) &&
           (strncmp(text, LARGE_TEXT_TEST_PATTERN, strlen(LARGE_TEXT_TEST_PATTERN) != 0))
        )
        {
            LE_ERROR("-TEST  8/13- Check le_sms_msg_GetText, bad expected received text! (%s)", text);
        }
        else
        {
            LE_INFO("-TEST  8/13- Check le_sms_msg_GetText, received text OK.");
        }

        // Verify that the message is read-only
        res=le_sms_msg_SetDestination(msg, DEST_TEST_PATTERN);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  9/13- Check le_sms_msg_SetDestination, parameter check failure!");
        }
        else
        {
            LE_INFO("-TEST  9/13- Check le_sms_msg_SetDestination OK.");
        }

        res=le_sms_msg_SetText(msg, TEXT_TEST_PATTERN);
        if(res != LE_NOT_PERMITTED)
        {
            LE_ERROR("-TEST  10/13- Check le_sms_msg_SetText, parameter check failure!");
        }
        else
        {
            LE_INFO("-TEST  10/13- Check le_sms_msg_SetText OK.");
        }

        // Verify Mark Read/Unread functions
        le_sms_msg_MarkRead(msg);

        mystatus=le_sms_msg_GetStatus(msg);
        if(mystatus != LE_SMS_RX_READ)
        {
            LE_ERROR("-TEST  11/13- Check le_sms_msg_GetStatus, bad status (%d)!", mystatus);
        }
        else
        {
            LE_INFO("-TEST  11/13- Check le_sms_msg_GetStatus, status OK.");
        }

        le_sms_msg_MarkUnread(msg);

        mystatus=le_sms_msg_GetStatus(msg);
        if(mystatus != LE_SMS_RX_UNREAD)
        {
            LE_ERROR("-TEST  12/13- Check le_sms_msg_GetStatus, bad status (%d)!", mystatus);
        }
        else
        {
            LE_INFO("-TEST  12/13- Check le_sms_msg_GetStatus, status OK.");
        }

        res=le_sms_msg_DeleteFromStorage(msg);
        if(res != LE_OK)
        {
            LE_ERROR("-TEST  13/13- Check le_sms_msg_DeleteFromStorage failure!");
        }
        else
        {
            LE_INFO("-TEST  13/13- Check le_sms_msg_DeleteFromStorage OK.");
        }
    }
    else
    {
        LE_WARN("-TEST- I check only Text message!");
    }

    le_sms_msg_Delete(msg);
}

#ifndef AUTOMATIC
//--------------------------------------------------------------------------------------------------
/**
 * This function gets the telephone number from the User (interactive case).
 *
 */
//--------------------------------------------------------------------------------------------------
void getTel(void)
{
    char *strPtr;

    do
    {
        fprintf(stderr, "Please enter the device's telephone number to perform the SMS tests: \n");
        strPtr=fgets ((char*)DEST_TEST_PATTERN, LE_SMS_TEL_NMBR_MAX_LEN, stdin);
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
void Testle_sms_msg_SetGetText()
{
    le_result_t           res;
    le_sms_msg_Ref_t      myMsg;
    le_sms_msg_Format_t   myformat;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_LEN];
    char                  tel[LE_SMS_TEL_NMBR_MAX_LEN];
    char                  text[LE_SMS_TEXT_MAX_LEN];
    size_t                uintval;

    myMsg = le_sms_msg_Create();
    CU_ASSERT_PTR_NOT_NULL(myMsg);

    res=le_sms_msg_SetDestination(myMsg, DEST_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_msg_SetText(myMsg, TEXT_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    myformat=le_sms_msg_GetFormat(myMsg);
    CU_ASSERT_EQUAL(myformat, LE_SMS_FORMAT_TEXT);

    res=le_sms_msg_GetSenderTel(myMsg, tel, sizeof(tel));
    CU_ASSERT_EQUAL(res, LE_NOT_PERMITTED);

    res=le_sms_msg_GetTimeStamp(myMsg, timestamp, sizeof(timestamp));
    CU_ASSERT_EQUAL(res, LE_NOT_PERMITTED);

    uintval=le_sms_msg_GetUserdataLen(myMsg);
    CU_ASSERT_EQUAL(uintval, strlen(TEXT_TEST_PATTERN));

    res=le_sms_msg_GetText(myMsg, text, 1);
    CU_ASSERT_EQUAL(res, LE_OVERFLOW);

    res=le_sms_msg_GetText(myMsg, text, sizeof(text));
    CU_ASSERT_EQUAL(res, LE_OK);
    CU_ASSERT_STRING_EQUAL(text, TEXT_TEST_PATTERN);

    res=le_sms_msg_SetDestination(myMsg, VOID_PATTERN);
    CU_ASSERT_EQUAL(res, LE_BAD_PARAMETER);

    res=le_sms_msg_SetDestination(myMsg, FAIL_DEST_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OVERFLOW);

    res=le_sms_msg_SetText(myMsg, VOID_PATTERN);
    CU_ASSERT_EQUAL(res, LE_BAD_PARAMETER);

    res=le_sms_msg_SetText(myMsg, FAIL_TEXT_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OUT_OF_RANGE);

    le_sms_msg_Delete(myMsg);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Raw binary Message Object Set/Get APIS.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sms_msg_SetGetBinary()
{
    le_result_t           res;
    le_sms_msg_Ref_t      myMsg;
    le_sms_msg_Format_t   myformat;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_LEN];
    char                  tel[LE_SMS_TEL_NMBR_MAX_LEN];
    uint8_t               raw[LE_SMS_BINARY_MAX_LEN];
    size_t                uintval;
    uint32_t              i;

    myMsg = le_sms_msg_Create();
    CU_ASSERT_PTR_NOT_NULL(myMsg);

    res=le_sms_msg_SetDestination(myMsg, DEST_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_msg_SetBinary(myMsg, BINARY_TEST_PATTERN, sizeof(BINARY_TEST_PATTERN));
    CU_ASSERT_EQUAL(res, LE_OK);

    myformat=le_sms_msg_GetFormat(myMsg);
    CU_ASSERT_EQUAL(myformat, LE_SMS_FORMAT_BINARY);

    res=le_sms_msg_GetSenderTel(myMsg, tel, sizeof(tel));
    CU_ASSERT_EQUAL(res, LE_NOT_PERMITTED);

    res=le_sms_msg_GetTimeStamp(myMsg, timestamp, sizeof(timestamp));
    CU_ASSERT_EQUAL(res, LE_NOT_PERMITTED);

    uintval=le_sms_msg_GetUserdataLen(myMsg);
    CU_ASSERT_EQUAL(uintval, sizeof(BINARY_TEST_PATTERN));

    uintval=1;
    res=le_sms_msg_GetBinary(myMsg, raw, &uintval);
    CU_ASSERT_EQUAL(res, LE_OVERFLOW);

    uintval=sizeof(BINARY_TEST_PATTERN);
    res=le_sms_msg_GetBinary(myMsg, raw, &uintval);
    CU_ASSERT_EQUAL(res, LE_OK);
    for(i=0; i<sizeof(BINARY_TEST_PATTERN) ; i++)
    {
        CU_ASSERT_EQUAL(raw[i], BINARY_TEST_PATTERN[i]);
    }
    CU_ASSERT_EQUAL(uintval, sizeof(BINARY_TEST_PATTERN));

    res=le_sms_msg_SetDestination(myMsg, VOID_PATTERN);
    CU_ASSERT_EQUAL(res, LE_BAD_PARAMETER);

    res=le_sms_msg_SetDestination(myMsg, FAIL_DEST_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OVERFLOW);

    res=le_sms_msg_SetBinary(myMsg, BINARY_TEST_PATTERN, 0);
    CU_ASSERT_EQUAL(res, LE_BAD_PARAMETER);

    res=le_sms_msg_SetBinary(myMsg, BINARY_TEST_PATTERN, sizeof(FAIL_BINARY_TEST_PATTERN));
    CU_ASSERT_EQUAL(res, LE_OUT_OF_RANGE);

    le_sms_msg_Delete(myMsg);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: PDU Message Object Set/Get APIS.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sms_msg_SetGetPDU()
{
    le_result_t           res;
    le_sms_msg_Ref_t      myMsg;
    char                  timestamp[LE_SMS_TIMESTAMP_MAX_LEN];
    char                  tel[LE_SMS_TEL_NMBR_MAX_LEN];
    uint8_t               pdu[LE_SMS_PDU_MAX_LEN];
    size_t                uintval;
    uint32_t              i;

    myMsg = le_sms_msg_Create();
    CU_ASSERT_PTR_NOT_NULL(myMsg);

    res=le_sms_msg_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, sizeof(PDU_TEST_PATTERN_7BITS));
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_msg_GetSenderTel(myMsg, tel, sizeof(tel));
    CU_ASSERT_EQUAL(res, LE_NOT_PERMITTED);

    res=le_sms_msg_GetTimeStamp(myMsg, timestamp, sizeof(timestamp));
    CU_ASSERT_EQUAL(res, LE_NOT_PERMITTED);

    uintval=le_sms_msg_GetPDULen(myMsg);
    CU_ASSERT_EQUAL(uintval, sizeof(PDU_TEST_PATTERN_7BITS));

    uintval=1;
    res=le_sms_msg_GetPDU(myMsg, pdu, &uintval);
    CU_ASSERT_EQUAL(res, LE_OVERFLOW);

    uintval=sizeof(pdu);
    res=le_sms_msg_GetPDU(myMsg, pdu, &uintval);
    CU_ASSERT_EQUAL(res, LE_OK);
    for(i=0; i<sizeof(PDU_TEST_PATTERN_7BITS) ; i++)
    {
        CU_ASSERT_EQUAL(pdu[i], PDU_TEST_PATTERN_7BITS[i]);
    }
    CU_ASSERT_EQUAL(uintval, sizeof(PDU_TEST_PATTERN_7BITS));

    res=le_sms_msg_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, 0);
    CU_ASSERT_EQUAL(res, LE_BAD_PARAMETER);

    res=le_sms_msg_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, LE_SMS_PDU_MAX_LEN*2);
    CU_ASSERT_EQUAL(res, LE_OUT_OF_RANGE);

    le_sms_msg_Delete(myMsg);
}



//--------------------------------------------------------------------------------------------------
/**
 * Test: Send a Text message.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sms_msg_SendText()
{
    le_result_t           res;
    le_sms_msg_Ref_t      myMsg;

    myMsg = le_sms_msg_Create();
    CU_ASSERT_PTR_NOT_NULL(myMsg);

    LE_DEBUG("-TEST- Create Msg %p", myMsg);

    testHdlrRef=le_sms_msg_AddRxMessageHandler(TestRxHandler, NULL);
    CU_ASSERT_PTR_NOT_NULL(testHdlrRef);

    res=le_sms_msg_SetDestination(myMsg, DEST_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_msg_SetText(myMsg, LARGE_TEXT_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_msg_Send(myMsg);
    CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
    CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);

    res=le_sms_msg_SetText(myMsg, SHORT_TEXT_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_msg_Send(myMsg);
    CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
    CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);

    le_sms_msg_Delete(myMsg);
}

//--------------------------------------------------------------------------------------------------
/**
 * Test: Send a raw binary message.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sms_msg_SendBinary()
{
    le_result_t           res;
    le_sms_msg_Ref_t      myMsg;

    myMsg = le_sms_msg_Create();
    CU_ASSERT_PTR_NOT_NULL(myMsg);

    LE_DEBUG("-TEST- Create Msg %p", myMsg);

    res=le_sms_msg_SetDestination(myMsg, DEST_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_msg_SetBinary(myMsg, BINARY_TEST_PATTERN, sizeof(BINARY_TEST_PATTERN));
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_msg_Send(myMsg);
    CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
    CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);

    le_sms_msg_Delete(myMsg);
}

// //--------------------------------------------------------------------------------------------------
// /**
//  * Test: Send a Pdu message.
//  *
//  */
// //--------------------------------------------------------------------------------------------------
// void Testle_sms_msg_SendPdu()
// {
//     le_result_t           res;
//     le_sms_msg_Ref_t      myMsg;
//
//     myMsg = le_sms_msg_Create();
//     CU_ASSERT_PTR_NOT_NULL(myMsg);
//
//     LE_DEBUG("Create Msg %p", myMsg);
//
// //     le_result_t encode_smspdu_7bits(const char *message, int length, const char *address, pa_sms_Pdu_t * pdu);
//
//     res=le_sms_msg_SetPDU(myMsg, PDU_TEST_PATTERN_7BITS, sizeof(PDU_TEST_PATTERN_7BITS));
//     CU_ASSERT_EQUAL(res, LE_OK);
//
//     res=le_sms_msg_Send(myMsg);
//     CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
//     CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);
//
//     res=le_sms_msg_SetPDU(myMsg, PDU_TEST_PATTERN_8BITS, sizeof(PDU_TEST_PATTERN_8BITS));
//     CU_ASSERT_EQUAL(res, LE_OK);
//
//     res=le_sms_msg_Send(myMsg);
//     CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
//     CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);
//
//     le_sms_msg_Delete(myMsg);
// }

//--------------------------------------------------------------------------------------------------
/**
 * Test: Check Received List.
 *
 */
//--------------------------------------------------------------------------------------------------
void Testle_sms_msg_ReceivedList()
{
    le_result_t              res;
    le_sms_msg_Ref_t         myMsg;
    le_sms_msg_Ref_t         lMsg1, lMsg2;
    le_sms_msg_ListRef_t     receivedList;
    le_sms_msg_Status_t      mystatus;

    myMsg = le_sms_msg_Create();
    CU_ASSERT_PTR_NOT_NULL(myMsg);

    res=le_sms_msg_SetDestination(myMsg, DEST_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_msg_SetText(myMsg, TEXT_TEST_PATTERN);
    CU_ASSERT_EQUAL(res, LE_OK);

    res=le_sms_msg_Send(myMsg);
    CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
    CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);

    res=le_sms_msg_Send(myMsg);
    CU_ASSERT_NOT_EQUAL(res, LE_FAULT);
    CU_ASSERT_NOT_EQUAL(res, LE_FORMAT_ERROR);
    if (res == LE_OK)
    {
        uint32_t num=0, numtot=0, i=0;
        uint32_t idx[255];

        do
        {
            i++;
            sleep(1);
            res=pa_sms_ListMsgFromMem(LE_SMS_RX_READ, &num, idx);
            LE_INFO("-TEST- list read num=%d", num);
            numtot=num;
            res=pa_sms_ListMsgFromMem(LE_SMS_RX_UNREAD, &num, idx);
            LE_INFO("-TEST- list unread num=%d", num);
            numtot+=num;
        } while ((numtot<2) && (res==LE_OK) && (i<10));

        if ((res == LE_OK) && (i<10))
        {
            // List Received messages
            receivedList=le_sms_msg_CreateRxMsgList();
            CU_ASSERT_PTR_NOT_NULL(receivedList);

            lMsg1=le_sms_msg_GetFirst(receivedList);
            CU_ASSERT_PTR_NOT_NULL(lMsg1);
            if (lMsg1 != NULL)
            {
                mystatus=le_sms_msg_GetStatus(lMsg1);
                CU_ASSERT_TRUE((mystatus==LE_SMS_RX_READ)||(mystatus==LE_SMS_RX_UNREAD));
                LE_INFO("-TEST- Delete Rx message 1.%p", lMsg1);
                le_sms_msg_Delete(lMsg1);
            }

            lMsg2=le_sms_msg_GetNext(receivedList);
            CU_ASSERT_PTR_NOT_NULL(lMsg2);
            if (lMsg2 != NULL)
            {
                mystatus=le_sms_msg_GetStatus(lMsg2);
                CU_ASSERT_TRUE((mystatus==LE_SMS_RX_READ)||(mystatus==LE_SMS_RX_UNREAD));
                LE_INFO("-TEST- Delete Rx message 2.%p", lMsg2);
                le_sms_msg_Delete(lMsg2);
            }

            LE_INFO("-TEST- Delete the ReceivedList");
            le_sms_msg_DeleteList(receivedList);
        }
        else
        {
            LE_ERROR("-TEST- Unable to complete Testle_sms_msg_ReceivedList Test");
        }
    }
    else
    {
        LE_ERROR("-TEST- Unable to complete Testle_sms_msg_ReceivedList Test");
    }

    // Delete sent message
    le_sms_msg_Delete(myMsg);
}

