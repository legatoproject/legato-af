/**
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

// Header files for CUnit
#include <CUnit/Console.h>
#include <CUnit/Basic.h>

#define PDU_MAX     256

#include "legato.h"
#include "cdmaPdu.h"

typedef struct {
    cdmaPdu_t cdmaMessage;
    struct {
        const size_t    length;
        const uint8_t   data[256];
    } pduEncoded;
} PDUAssoc_t;

const PDUAssoc_t PDUAssocDb[] = {
        /* 0 */ {
                .pduEncoded = {
                        .length = 40,
                        .data = {
                            0x00, 0x00, 0x02, 0x10, 0x02, 0x02, 0x07, 0x02, 0x8C, 0xE9,
                            0x5D, 0xCC, 0x65, 0x80, 0x06, 0x01, 0xFC, 0x08, 0x15, 0x00,
                            0x03, 0x16, 0x8D, 0x30, 0x01, 0x06, 0x10, 0x24, 0x18, 0x30,
                            0x60, 0x80, 0x03, 0x06, 0x10, 0x10, 0x04, 0x04, 0x48, 0x47,
                        },
                },
                .cdmaMessage = {
                      .messageFormat = CDMAPDU_MESSAGEFORMAT_POINTTOPOINT,
                      .message = {
                          .parameterMask = CDMAPDU_PARAMETERMASK_TELESERVICE_ID |
                                           CDMAPDU_PARAMETERMASK_ORIGINATING_ADDR |
                                           CDMAPDU_PARAMETERMASK_BEARER_REPLY_OPTION |
                                           CDMAPDU_PARAMETERMASK_BEARER_DATA,
                          .teleServiceId = 0x1002 ,
                          .originatingAddr = {
                              .digitMode = 0,
                              .numberMode = 0,
                              .numberType = 0,
                              .numberPlan = 0,
                              .fieldsNumber = 10,
                              .chari = {
                                  0x33, 0xA5, 0x77, 0x31, 0x96,
                              },
                          },
                          .bearerReplyOption = {
                              .replySeq = 0x3F,
                          },
                          .bearerData = {
                              .subParameterMask = CDMAPDU_SUBPARAMETERMASK_MESSAGE_IDENTIFIER |
                                                  CDMAPDU_SUBPARAMETERMASK_USER_DATA |
                                                  CDMAPDU_SUBPARAMETERMASK_MESSAGE_CENTER_TIME_STAMP,
                              .messageIdentifier = {
                                  .messageType = CDMAPDU_MESSAGETYPE_DELIVER,
                                  .messageIdentifier = 26835,
                                  .headerIndication = 0,
                              },
                              .userData = {
                                  .messageEncoding = CDMAPDU_ENCODING_7BIT_ASCII,
                                  .messageType = 0x00,
                                  .fieldsNumber = 0x04,
                                  .chari = {
                                      0x83, 0x06, 0x0C, 0x10,
                                  },
                              },
                              .messageCenterTimeStamp = {
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
                .pduEncoded = {
                        .length = 54,
                        .data = {
                            0x00, 0x00, 0x02, 0x10, 0x02, 0x02, 0x07, 0x02, 0x8C, 0xD9,
                            0x85, 0x94, 0x61, 0x80, 0x06, 0x01, 0xFC, 0x08, 0x23, 0x00,
                            0x03, 0x16, 0x8D, 0x30, 0x01, 0x14, 0x10, 0xA5, 0x4C, 0xBC,
                            0xFA, 0x20, 0xE7, 0x97, 0x76, 0x4D, 0x3B, 0xB3, 0xA0, 0xDB,
                            0x97, 0x9F, 0x3C, 0x39, 0xF2, 0x80, 0x03, 0x06, 0x14, 0x07,
                            0x07, 0x17, 0x44, 0x28, 0x00
                        },
                },
                .cdmaMessage = {
                      .messageFormat = CDMAPDU_MESSAGEFORMAT_POINTTOPOINT,
                      .message = {
                          .parameterMask = CDMAPDU_PARAMETERMASK_TELESERVICE_ID |
                                           CDMAPDU_PARAMETERMASK_ORIGINATING_ADDR |
                                           CDMAPDU_PARAMETERMASK_BEARER_REPLY_OPTION |
                                           CDMAPDU_PARAMETERMASK_BEARER_DATA,
                          .teleServiceId = 0x1002 ,
                          .originatingAddr = {
                              .digitMode = 0,
                              .numberMode = 0,
                              .numberType = 0,
                              .numberPlan = 0,
                              .fieldsNumber = 10,
                              .chari = {
                                  0x33, 0x66, 0x16, 0x51, 0x86,
                              },
                          },
                          .bearerReplyOption = {
                              .replySeq = 0x3F,
                          },
                          .bearerData = {
                              .subParameterMask = CDMAPDU_SUBPARAMETERMASK_MESSAGE_IDENTIFIER |
                                                  CDMAPDU_SUBPARAMETERMASK_USER_DATA |
                                                  CDMAPDU_SUBPARAMETERMASK_MESSAGE_CENTER_TIME_STAMP,
                              .messageIdentifier = {
                                  .messageType = CDMAPDU_MESSAGETYPE_DELIVER,
                                  .messageIdentifier = 26835,
                                  .headerIndication = 0,
                              },
                              .userData = {
                                  .messageEncoding = CDMAPDU_ENCODING_7BIT_ASCII,
                                  .messageType = 0x00,
                                  .fieldsNumber = 0x14,
                                  .chari = {
                                      0xa9, 0x97, 0x9f, 0x44, 0x1c, 0xf2, 0xee, 0xc9, 0xa7, 0x76,
                                      0x74, 0x1b, 0x72, 0xf3, 0xe7, 0x87, 0x3e, 0x50
                                  },
                              },
                              .messageCenterTimeStamp = {
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


/* The suite initialization function.
* Opens the temporary file used by the tests.
* Returns zero on success, non-zero otherwise.
*/
int Init_Suite(void)
{
    return 0;
}

/* The suite cleanup function.
* Closes the temporary file used by the tests.
* Returns zero on success, non-zero otherwise.
*/
int Clean_Suite(void)
{
    return 0;
}

void DumpPDU(const uint8_t * dataPtr, size_t length) {
    int i;
    const int columns = 128;

    for( i = 0; i < length+1; i++) {
        fprintf(stderr, "%02X ", dataPtr[i]);
        if(i % columns == (columns-1) )
            fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}

void testEncodePdu()
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
        fprintf(stderr, "pdu Size %d \n", pduSize);

        CU_ASSERT_EQUAL( res, LE_OK );

        DumpPDU(pduResult,pduSize);
        DumpPDU(assoc->pduEncoded.data,assoc->pduEncoded.length);

        CU_ASSERT_EQUAL( memcmp(pduResult,assoc->pduEncoded.data, pduSize) , 0);

        fprintf(stderr, "------------------\n");
        fprintf(stderr, "\n");
    }
}

void testDecodePdu()
{
    cdmaPdu_t message = {0};
    le_result_t res;
    int i;

    for( i = 0; i < sizeof(PDUAssocDb)/sizeof(PDUAssoc_t); i++)
    {
        const PDUAssoc_t * assoc = &PDUAssocDb[i];

        fprintf(stderr, "------------------\n");
        DumpPDU(assoc->pduEncoded.data,assoc->pduEncoded.length);
        fprintf(stderr, "---------\n");

        /* Decode */
        res = cdmaPdu_Decode(assoc->pduEncoded.data ,assoc->pduEncoded.length , &message);
        CU_ASSERT_EQUAL( res, LE_OK );

        cdmaPdu_Dump(&message);
        cdmaPdu_Dump(&assoc->cdmaMessage);

        CU_ASSERT_EQUAL( memcmp(&message, &assoc->cdmaMessage, sizeof(message)) , 0);

        fprintf(stderr, "------------------\n");
        fprintf(stderr, "\n");
    }
}

COMPONENT_INIT
{
    // Init the test case / test suite data structures


    CU_TestInfo testCases[] =
    {
            { "Test EncodePdu", testEncodePdu },
            { "Test DecodePdu", testDecodePdu },
            CU_TEST_INFO_NULL,
    };

    CU_SuiteInfo suites[] =
    {
            { "PDU Convert tests", Init_Suite, Clean_Suite, testCases },
            CU_SUITE_INFO_NULL,
    };

    // Initialize the CUnit test registry and register the test suite
    if (CUE_SUCCESS != CU_initialize_registry())
        exit(CU_get_error());

    if ( CUE_SUCCESS != CU_register_suites(suites))
    {
        CU_cleanup_registry();
        exit(CU_get_error());
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Output summary of failures, if there were any
    if ( CU_get_number_of_failures() > 0 )
    {
        fprintf(stdout,"\n [START]List of Failure :\n");
        CU_basic_show_failures(CU_get_failure_list());
        fprintf(stdout,"\n [STOP]List of Failure\n");

        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
