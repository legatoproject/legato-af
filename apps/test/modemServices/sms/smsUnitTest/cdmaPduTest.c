/**
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "interfaces.h"

#include "pa_sms.h"
#include "cdmaPdu.h"
#include "smsPdu.h"

#include "main.h"

typedef struct
{
    cdmaPdu_t cdmaMessage;
    struct
    {
   const size_t    length;
   const uint8_t   data[256];
    } pduEncoded;
} PduAssoc_t;

static const PduAssoc_t PDUAssocDb[] =
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
                     .chari =
                     {
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
    /* 1 */
    {
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
                .chari =
                {
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
    /* 2 */
    {
    .pduEncoded =
    {
        .length = 119,
        .data =
        {
            0x00, 0x00, 0x02, 0x10, 0x02, 0x02, 0x02, 0x00,
            0x00, 0x06, 0x01, 0xFC, 0x08, 0x69, 0x00, 0x03,
            0x10, 0x00, 0x30, 0x01, 0x62, 0x21, 0x80, 0x02,
            0x90, 0x01, 0x30, 0x02, 0x98, 0x01, 0x00, 0x02,
            0x18, 0x02, 0x68, 0x02, 0xA8, 0x01, 0x00, 0x02,
            0x98, 0x03, 0x40, 0x03, 0x78, 0x03, 0x90, 0x03,
            0xA0, 0x01, 0x00, 0x02, 0x68, 0x03, 0x28, 0x03,
            0x98, 0x03, 0x98, 0x03, 0x08, 0x03, 0x38, 0x03,
            0x28, 0x01, 0x00, 0x02, 0x98, 0x03, 0x28, 0x03,
            0x90, 0x03, 0xB0, 0x03, 0x48, 0x03, 0x18, 0x03,
            0x28, 0x01, 0x00, 0x02, 0xA0, 0x03, 0x28, 0x03,
            0x98, 0x03, 0xA0, 0x01, 0x00, 0x01, 0x68, 0x01,
            0x00, 0x02, 0xA8, 0x03, 0x70, 0x03, 0x48, 0x03,
            0x18, 0x03, 0x78, 0x03, 0x20, 0x03, 0x28, 0x01,
            0x00, 0x02, 0x98, 0x02, 0x68, 0x02, 0x98,
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
             .teleServiceId = 0x1002,
             .originatingAddr =
             {
                 .digitMode = 0,
                 .numberMode = 0,
                 .numberType = 0,
                 .numberPlan = 0,
                 .fieldsNumber = 0,
                 .chari = { },
             },
             .bearerReplyOption =
             {
                 .replySeq = 0x3F,
              },
              .bearerData =
              {
                 .subParameterMask = CDMAPDU_SUBPARAMETERMASK_MESSAGE_IDENTIFIER |
                    CDMAPDU_SUBPARAMETERMASK_USER_DATA,
                 .messageIdentifier =
                 {
                     .messageType = CDMAPDU_MESSAGETYPE_DELIVER,
                     .messageIdentifier = 3,
                     .headerIndication = 0,
                 },
                 .userData =
                 {
                     .messageEncoding = CDMAPDU_ENCODING_UNICODE,
                     .messageType = 0x00,
                     .fieldsNumber = 0x30,
                     .chari =
                     {
                         0x00, 0x52, 0x00, 0x26, 0x00 ,0x53, 0x00 ,0x20,
                         0x00, 0x43, 0x00 ,0x4D, 0x00 ,0x55, 0x00 ,0x20,
                         0x00, 0x53, 0x00, 0x68, 0x00 ,0x6F, 0x00 ,0x72,
                         0x00, 0x74, 0x00 ,0x20, 0x00 ,0x4D, 0x00 ,0x65,
                         0x00, 0x73, 0x00, 0x73, 0x00 ,0x61, 0x00 ,0x67,
                         0x00, 0x65, 0x00 ,0x20, 0x00 ,0x53, 0x00 ,0x65,
                         0x00, 0x72, 0x00, 0x76, 0x00 ,0x69, 0x00 ,0x63,
                         0x00, 0x65, 0x00 ,0x20, 0x00 ,0x54, 0x00 ,0x65,
                         0x00, 0x73, 0x00, 0x74, 0x00 ,0x20, 0x00 ,0x2D,
                         0x00, 0x20, 0x00 ,0x55, 0x00 ,0x6E, 0x00 ,0x69,
                         0x00, 0x63, 0x00, 0x6F, 0x00 ,0x64, 0x00 ,0x65,
                         0x00, 0x20, 0x00 ,0x53, 0x00 ,0x4D, 0x00 ,0x53,
                     },
                 },
                 .messageCenterTimeStamp =
                 {
                     .year = 0x0,
                     .month = 0x0,
                     .day = 0x0,
                     .hours = 0x0,
                     .minutes = 0x0,
                     .seconds = 0x0,
                 },
              },
          },
       },
   },
};


static le_result_t TestEncodePdu
(
    void
)
{
    uint8_t     pduResult[256] = {0};
    uint32_t    pduSize = 0;
    le_result_t res;
    int i;

    for( i = 0; i < sizeof(PDUAssocDb)/sizeof(PduAssoc_t); i++)
    {
        const PduAssoc_t * assoc = &PDUAssocDb[i];

        cdmaPdu_Dump(&assoc->cdmaMessage);

        /* Encode */
        res = cdmaPdu_Encode(&assoc->cdmaMessage ,pduResult , sizeof(pduResult), &pduSize);
        LE_INFO("pdu Size %d", pduSize);

        if (res != LE_OK)
        {
            return LE_FAULT;
        }

        if (memcmp(pduResult,assoc->pduEncoded.data, pduSize) != 0)
        {
            DumpPdu("CdmaPdu:",pduResult, pduSize);
            DumpPdu("CdmaPdu:",assoc->pduEncoded.data, assoc->pduEncoded.length);
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

    for( i = 0; i < sizeof(PDUAssocDb)/sizeof(PduAssoc_t); i++)
    {
        const PduAssoc_t * assoc = &PDUAssocDb[i];

        LE_INFO("------------------");
        DumpPdu("Pdu:",assoc->pduEncoded.data,assoc->pduEncoded.length);
        LE_INFO("---------");

        /* Decode */
        res = cdmaPdu_Decode(assoc->pduEncoded.data ,assoc->pduEncoded.length , &message);
        if (res != LE_OK)
        {
            LE_ERROR("cdmaPdu_Decode Failed");
            return LE_FAULT;
        }

        if (memcmp(&message, &assoc->cdmaMessage, sizeof(message)) != 0)
        {
            int j;
            LE_INFO("------------------");
            cdmaPdu_Dump(&message);
            cdmaPdu_Dump(&assoc->cdmaMessage);
            LE_INFO("------------------");
            DumpPdu("PduSRC:",(uint8_t*) &assoc->cdmaMessage, sizeof(message));
            LE_INFO("------------------");
            DumpPdu("PduEnc:",(uint8_t*) &message, sizeof(message));
            LE_INFO("------------------");
            LE_ERROR("Comp Failed");
            LE_INFO("------------------");
            for( j = 0; j < sizeof(message); j++)
            {
                if( ((uint8_t*)&message)[j] != ((uint8_t*)&assoc->cdmaMessage)[j] )
                {
                        LE_ERROR("Failed len %d 0x%02X!=0x%02X", j,
                        ((uint8_t*)&message)[j],
                        ((uint8_t*)&assoc->cdmaMessage)[j] );
                    DumpPdu("PduSRC:",(uint8_t*) &assoc->cdmaMessage, j);
                    j = sizeof(message);
                }
            }

            return LE_FAULT;
        }

        LE_INFO("------------------");
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/*
 * CDMA SMS PDU encoding and decoding test
 *
 */
//--------------------------------------------------------------------------------------------------
void testle_sms_CdmaPduTest
(
    void
)
{
    LE_INFO("Test CDMA EncodePdu started");
    LE_ASSERT(TestEncodePdu() == LE_OK);

    LE_INFO("Test CDMA TestDecodePdu started");
    LE_ASSERT(TestDecodePdu() == LE_OK);

    LE_INFO("cdmaPduTest SUCCESS");
}
