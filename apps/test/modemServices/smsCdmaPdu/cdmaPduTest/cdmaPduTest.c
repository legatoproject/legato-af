/**
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include <legato.h>
#include "cdmaPdu.h"


//--------------------------------------------------------------------------------------------------
/**
 * Test sequence Structure list
 */
//--------------------------------------------------------------------------------------------------

typedef le_result_t (*TestFunc)(void);

typedef struct
{
        char * name;
        TestFunc ptrfunc;
} my_struct;

typedef struct
{
    cdmaPdu_t cdmaMessage;
    struct
    {
        const size_t    length;
        const uint8_t   data[256];
    } pduEncoded;
} PDUAssoc_t;

const PDUAssoc_t PDUAssocDb[] =
{
        /* 0 */
                {
                .pduEncoded =
                {
                        .length = 40,
                        .data =
                        {
                            0x00, 0x00, 0x02, 0x10, 0x02, 0x02, 0x07, 0x02, 0x8C, 0xE9,
                            0x5D, 0xCC, 0x65, 0x80, 0x06, 0x01, 0xFC, 0x08, 0x15, 0x00,
                            0x03, 0x16, 0x8D, 0x30, 0x01, 0x06, 0x10, 0x24, 0x18, 0x30,
                            0x60, 0x80, 0x03, 0x06, 0x10, 0x10, 0x04, 0x04, 0x48, 0x47,
                        },
                },
                .cdmaMessage =
                {
                      .messageFormat = CDMAPDU_MESSAGEFORMAT_POINTTOPOINT,
                      .message =
                      {
                          .parameterMask = CDMAPDU_PARAMETERMASK_TELESERVICE_ID |
                                           CDMAPDU_PARAMETERMASK_ORIGINATING_ADDR |
                                           CDMAPDU_PARAMETERMASK_BEARER_REPLY_OPTION |
                                           CDMAPDU_PARAMETERMASK_BEARER_DATA,
                          .teleServiceId = 0x1002 ,
                          .originatingAddr =
                          {
                              .digitMode = 0,
                              .numberMode = 0,
                              .numberType = 0,
                              .numberPlan = 0,
                              .fieldsNumber = 10,
                              .chari = {
                                  0x33, 0xA5, 0x77, 0x31, 0x96,
                              },
                          },
                          .bearerReplyOption =
                          {
                              .replySeq = 0x3F,
                          },
                          .bearerData =
                          {
                              .subParameterMask = CDMAPDU_SUBPARAMETERMASK_MESSAGE_IDENTIFIER |
                                                  CDMAPDU_SUBPARAMETERMASK_USER_DATA |
                                                  CDMAPDU_SUBPARAMETERMASK_MESSAGE_CENTER_TIME_STAMP,
                              .messageIdentifier =
                              {
                                  .messageType = CDMAPDU_MESSAGETYPE_DELIVER,
                                  .messageIdentifier = 26835,
                                  .headerIndication = 0,
                              },
                              .userData =
                              {
                                  .messageEncoding = CDMAPDU_ENCODING_7BIT_ASCII,
                                  .messageType = 0x00,
                                  .fieldsNumber = 0x04,
                                  .chari = {
                                      0x83, 0x06, 0x0C, 0x10,
                                  },
                              },
                              .messageCenterTimeStamp =
                              {
                                  .year = 0x10,
                                  .month = 0x10,
                                  .day = 0x04,
                                  .hours = 0x04,
                                  .minutes = 0x48,
                                  .seconds = 0x47,
                              },
                          },
                      },
                },
        },
        /* 1 */ {
                .pduEncoded =
                {
                        .length = 54,
                        .data =
                        {
                            0x00, 0x00, 0x02, 0x10, 0x02, 0x02, 0x07, 0x02, 0x8C, 0xD9,
                            0x85, 0x94, 0x61, 0x80, 0x06, 0x01, 0xFC, 0x08, 0x23, 0x00,
                            0x03, 0x16, 0x8D, 0x30, 0x01, 0x14, 0x10, 0xA5, 0x4C, 0xBC,
                            0xFA, 0x20, 0xE7, 0x97, 0x76, 0x4D, 0x3B, 0xB3, 0xA0, 0xDB,
                            0x97, 0x9F, 0x3C, 0x39, 0xF2, 0x80, 0x03, 0x06, 0x14, 0x07,
                            0x07, 0x17, 0x44, 0x28, 0x00
                        },
                },
                .cdmaMessage =
                {
                      .messageFormat = CDMAPDU_MESSAGEFORMAT_POINTTOPOINT,
                      .message =
                      {
                          .parameterMask = CDMAPDU_PARAMETERMASK_TELESERVICE_ID |
                                           CDMAPDU_PARAMETERMASK_ORIGINATING_ADDR |
                                           CDMAPDU_PARAMETERMASK_BEARER_REPLY_OPTION |
                                           CDMAPDU_PARAMETERMASK_BEARER_DATA,
                          .teleServiceId = 0x1002 ,
                          .originatingAddr =
                          {
                              .digitMode = 0,
                              .numberMode = 0,
                              .numberType = 0,
                              .numberPlan = 0,
                              .fieldsNumber = 10,
                              .chari = {
                                  0x33, 0x66, 0x16, 0x51, 0x86,
                              },
                          },
                          .bearerReplyOption =
                          {
                              .replySeq = 0x3F,
                          },
                          .bearerData =
                          {
                              .subParameterMask = CDMAPDU_SUBPARAMETERMASK_MESSAGE_IDENTIFIER |
                                                  CDMAPDU_SUBPARAMETERMASK_USER_DATA |
                                                  CDMAPDU_SUBPARAMETERMASK_MESSAGE_CENTER_TIME_STAMP,
                              .messageIdentifier =
                              {
                                  .messageType = CDMAPDU_MESSAGETYPE_DELIVER,
                                  .messageIdentifier = 26835,
                                  .headerIndication = 0,
                              },
                              .userData =
                              {
                                  .messageEncoding = CDMAPDU_ENCODING_7BIT_ASCII,
                                  .messageType = 0x00,
                                  .fieldsNumber = 0x14,
                                  .chari =
                                  {
                                      0xa9, 0x97, 0x9f, 0x44, 0x1c, 0xf2, 0xee, 0xc9, 0xa7, 0x76,
                                      0x74, 0x1b, 0x72, 0xf3, 0xe7, 0x87, 0x3e, 0x50
                                  },
                              },
                              .messageCenterTimeStamp =
                              {
                                  .year = 0x14,
                                  .month = 0x07,
                                  .day = 0x07,
                                  .hours = 0x17,
                                  .minutes = 0x44,
                                  .seconds = 0x28,
                              },
                          },
                      },
                },
        },
};

//--------------------------------------------------------------------------------------------------
/**
 * Dump a data pointer
 */
//--------------------------------------------------------------------------------------------------
static void DumpPDU
(
    const uint8_t * dataPtr,
    size_t length
)
{
    int i;
    const int columns = 32;
    char logDump[255] = { 0 };
    char dump[5] = { 0 };

    logDump[0] = 0;

    for( i = 0; i < length+1; i++)
    {
        snprintf(dump,  5, "%02X ", dataPtr[i]);
        strncat(logDump, dump, 255);
        if(i % columns == (columns-1) )
        {
            strncat(logDump, " ", 255);
        }
    }
    LE_INFO("%s", logDump);
}

static le_result_t TestEncodePdu
(
    void
)
{
    uint8_t     pduResult[256] = {0};
    uint32_t    pduSize = 0;
    le_result_t res;
    int i;

    for( i = 0; i < sizeof(PDUAssocDb)/sizeof(PDUAssoc_t); i++)
    {
        const PDUAssoc_t * assoc = &PDUAssocDb[i];

        cdmaPdu_Dump(&assoc->cdmaMessage);

        /* Encode */
        res = cdmaPdu_Encode(&assoc->cdmaMessage ,pduResult , sizeof(pduResult), &pduSize);
        LE_INFO("pdu Size %d", pduSize);

        if (res != LE_OK)
        {
            return LE_FAULT;
        }

        DumpPDU(pduResult,pduSize);
        DumpPDU(assoc->pduEncoded.data,assoc->pduEncoded.length);

        if (memcmp(pduResult,assoc->pduEncoded.data, pduSize) != 0)
        {
            return LE_FAULT;
        }

        LE_INFO("------------------");
    }

    return LE_OK;
}

static le_result_t TestDecodePdu
(
    void
)
{
    cdmaPdu_t message = {0};
    le_result_t res;
    int i;

    for( i = 0; i < sizeof(PDUAssocDb)/sizeof(PDUAssoc_t); i++)
    {
        const PDUAssoc_t * assoc = &PDUAssocDb[i];

        LE_INFO("------------------");
        DumpPDU(assoc->pduEncoded.data,assoc->pduEncoded.length);
        LE_INFO("---------");

        /* Decode */
        res = cdmaPdu_Decode(assoc->pduEncoded.data ,assoc->pduEncoded.length , &message);
        if (res != LE_OK)
        {
            return LE_FAULT;
        }

        cdmaPdu_Dump(&message);
        cdmaPdu_Dump(&assoc->cdmaMessage);

        if (memcmp(&message, &assoc->cdmaMessage, sizeof(message)) != 0)
        {
            return LE_FAULT;
        }

        LE_INFO("------------------");
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/*
 * Check "logread -f | grep PduTest" log
 * Start app : app start cdmaPduTest
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    int i;
    le_result_t res;

    // Init the test case / test suite data structures

    my_struct testCases[] =
    {
            { "Test EncodePdu", TestEncodePdu },
            { "Test DecodePdu", TestDecodePdu },
            { "", NULL }
    };

    for (i=0; testCases[i].ptrfunc != NULL; i++)
    {
        LE_INFO("Test %s STARTED", testCases[i].name);
        res =  testCases[i].ptrfunc();
        if (res != LE_OK)
        {
            LE_ERROR("Test %s FAILED", testCases[i].name);
            LE_ERROR("cdmaPduTest FAILED and Exit");
            exit(EXIT_FAILURE);
        }
        else
        {
            LE_INFO("Test %s PASSED", testCases[i].name);
        }
    }

    LE_INFO("cdmaPduTest SUCCESS and Exit");
    exit(EXIT_SUCCESS);
}
