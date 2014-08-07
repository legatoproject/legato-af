/**
 *
 * Copyright (C) Sierra Wireless, Inc. 2013-2014. Use of this work is subject to license.
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

#include "le_sms_interface.h"
#include "smsPdu.h"

typedef struct {
    const char * dest;
    const char * text;
    pa_sms_MsgType_t type;
    struct {
        const size_t length;
        const uint8_t data[256];
        le_result_t conversionResult;
    } pdu_7bits, pdu_8bits;
} PduAssoc_t;

const PduAssoc_t PduAssocDb[] = {
        /* 0 */ {
                .dest = "+33661651866",
                .text = "Test sending message",
                .type = PA_SMS_SMS_SUBMIT,
                .pdu_7bits = {
                        .conversionResult = LE_OK,
                        .length = 33,
                        .data = {
                                0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                                0xF6, 0x00, 0x00, 0xAD, 0x14, 0xD4, 0xF2, 0x9C, 0x0E, 0x9A,
                                0x97, 0xDD, 0xE4, 0xB4, 0xFB, 0x0C, 0x6A, 0x97, 0xE7, 0xF3,
                                0xF0, 0xB9, 0x0C,
                        },
                },
                .pdu_8bits = {
                        .conversionResult = LE_OK,
                        .length = 35,
                        .data = {
                                0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                                0xF6, 0x00, 0x04, 0xAD, 0x14, 0x54, 0x65, 0x73, 0x74, 0x20,
                                0x73, 0x65, 0x6E, 0x64, 0x69, 0x6E, 0x67, 0x20, 0x6D, 0x65,
                                0x73, 0x73, 0x61, 0x67, 0x65,
                        },
                },
        },
        /* 1 */ {
                .dest = "+33617190547",
                .text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi in commodo lectus, quis volutpat erat.",
                .type = PA_SMS_SMS_SUBMIT,
                .pdu_7bits = {
                        .conversionResult = LE_OK,
                        .length = 104,
                        .data = {
                                0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x16, 0x17, 0x09, 0x45,
                                0xF7, 0x00, 0x00, 0xAD, 0x65, 0xCC, 0xB7, 0xBC, 0xDC, 0x06,
                                0xA5, 0xE1, 0xF3, 0x7A, 0x1B, 0x44, 0x7E, 0xB3, 0xDF, 0x72,
                                0xD0, 0x3C, 0x4D, 0x07, 0x85, 0xDB, 0x65, 0x3A, 0x0B, 0x34,
                                0x7E, 0xBB, 0xE7, 0xE5, 0x31, 0xBD, 0x4C, 0xAF, 0xCB, 0x41,
                                0x61, 0x72, 0x1A, 0x9E, 0x9E, 0x8F, 0xD3, 0xEE, 0x33, 0xA8,
                                0xCC, 0x4E, 0xD3, 0x5D, 0xA0, 0xE6, 0x5B, 0x2E, 0x4E, 0x83,
                                0xD2, 0x6E, 0xD0, 0xF8, 0xDD, 0x6E, 0xBF, 0xC9, 0x6F, 0x10,
                                0xBB, 0x3C, 0xA6, 0xD7, 0xE7, 0x2C, 0x50, 0xBC, 0x9E, 0x9E,
                                0x83, 0xEC, 0x6F, 0x76, 0x9D, 0x0E, 0x0F, 0xD3, 0x41, 0x65,
                                0x79, 0x98, 0xEE, 0x02,
                        }
                },
                .pdu_8bits = {
                        .conversionResult = LE_OK,
                        .length = 116,
                        .data = {
                                0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x16, 0x17, 0x09, 0x45,
                                0xF7, 0x00, 0x04, 0xAD, 0x65, 0x4C, 0x6F, 0x72, 0x65, 0x6D,
                                0x20, 0x69, 0x70, 0x73, 0x75, 0x6D, 0x20, 0x64, 0x6F, 0x6C,
                                0x6F, 0x72, 0x20, 0x73, 0x69, 0x74, 0x20, 0x61, 0x6D, 0x65,
                                0x74, 0x2C, 0x20, 0x63, 0x6F, 0x6E, 0x73, 0x65, 0x63, 0x74,
                                0x65, 0x74, 0x75, 0x72, 0x20, 0x61, 0x64, 0x69, 0x70, 0x69,
                                0x73, 0x63, 0x69, 0x6E, 0x67, 0x20, 0x65, 0x6C, 0x69, 0x74,
                                0x2E, 0x20, 0x4D, 0x6F, 0x72, 0x62, 0x69, 0x20, 0x69, 0x6E,
                                0x20, 0x63, 0x6F, 0x6D, 0x6D, 0x6F, 0x64, 0x6F, 0x20, 0x6C,
                                0x65, 0x63, 0x74, 0x75, 0x73, 0x2C, 0x20, 0x71, 0x75, 0x69,
                                0x73, 0x20, 0x76, 0x6F, 0x6C, 0x75, 0x74, 0x70, 0x61, 0x74,
                                0x20, 0x65, 0x72, 0x61, 0x74, 0x2E,
                        },
                },
        },
        /* 2 */ {
                .dest = "0617190547",
                .text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi in commodo lectus, quis volutpat erat.",
                .type = PA_SMS_SMS_SUBMIT,
                .pdu_7bits = {
                        .conversionResult = LE_OK,
                        .length = 103,
                        .data = {
                                0x00, 0x11, 0x00, 0x0A, 0x81, 0x60, 0x71, 0x91, 0x50, 0x74,
                                0x00, 0x00, 0xAD, 0x65, 0xCC, 0xB7, 0xBC, 0xDC, 0x06, 0xA5,
                                0xE1, 0xF3, 0x7A, 0x1B, 0x44, 0x7E, 0xB3, 0xDF, 0x72, 0xD0,
                                0x3C, 0x4D, 0x07, 0x85, 0xDB, 0x65, 0x3A, 0x0B, 0x34, 0x7E,
                                0xBB, 0xE7, 0xE5, 0x31, 0xBD, 0x4C, 0xAF, 0xCB, 0x41, 0x61,
                                0x72, 0x1A, 0x9E, 0x9E, 0x8F, 0xD3, 0xEE, 0x33, 0xA8, 0xCC,
                                0x4E, 0xD3, 0x5D, 0xA0, 0xE6, 0x5B, 0x2E, 0x4E, 0x83, 0xD2,
                                0x6E, 0xD0, 0xF8, 0xDD, 0x6E, 0xBF, 0xC9, 0x6F, 0x10, 0xBB,
                                0x3C, 0xA6, 0xD7, 0xE7, 0x2C, 0x50, 0xBC, 0x9E, 0x9E, 0x83,
                                0xEC, 0x6F, 0x76, 0x9D, 0x0E, 0x0F, 0xD3, 0x41, 0x65, 0x79,
                                0x98, 0xEE, 0x02,
                        }
                },
                .pdu_8bits = {
                        .conversionResult = LE_OK,
                        .length = 115,
                        .data = {
                                0x00, 0x11, 0x00, 0x0A, 0x81, 0x60, 0x71, 0x91, 0x50, 0x74,
                                0x00, 0x04, 0xAD, 0x65, 0x4C, 0x6F, 0x72, 0x65, 0x6D, 0x20,
                                0x69, 0x70, 0x73, 0x75, 0x6D, 0x20, 0x64, 0x6F, 0x6C, 0x6F,
                                0x72, 0x20, 0x73, 0x69, 0x74, 0x20, 0x61, 0x6D, 0x65, 0x74,
                                0x2C, 0x20, 0x63, 0x6F, 0x6E, 0x73, 0x65, 0x63, 0x74, 0x65,
                                0x74, 0x75, 0x72, 0x20, 0x61, 0x64, 0x69, 0x70, 0x69, 0x73,
                                0x63, 0x69, 0x6E, 0x67, 0x20, 0x65, 0x6C, 0x69, 0x74, 0x2E,
                                0x20, 0x4D, 0x6F, 0x72, 0x62, 0x69, 0x20, 0x69, 0x6E, 0x20,
                                0x63, 0x6F, 0x6D, 0x6D, 0x6F, 0x64, 0x6F, 0x20, 0x6C, 0x65,
                                0x63, 0x74, 0x75, 0x73, 0x2C, 0x20, 0x71, 0x75, 0x69, 0x73,
                                0x20, 0x76, 0x6F, 0x6C, 0x75, 0x74, 0x70, 0x61, 0x74, 0x20,
                                0x65, 0x72, 0x61, 0x74, 0x2E,
                        },
                },
        },
        /* 3 */ {
                .dest = "+33661651866",
                .text = "Test with special char [ ...",
                .type = PA_SMS_SMS_SUBMIT,
                .pdu_7bits = {
                        .conversionResult = LE_OK,
                        .length = 41,
                        .data = {
                                0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                                0xF6, 0x00, 0x00, 0xAD, 0x1D, 0xD4, 0xF2, 0x9C, 0x0E, 0xBA,
                                0xA7, 0xE9, 0x68, 0xD0, 0x1C, 0x5E, 0x1E, 0xA7, 0xC3, 0x6C,
                                0xD0, 0x18, 0x1D, 0x96, 0x83, 0x36, 0x3C, 0x90, 0xCB, 0xE5,
                                0x02, 0x00,
                        }
                },
                .pdu_8bits = {
                        .conversionResult = LE_OK,
                        .length = 43,
                        .data = {
                                0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                                0xF6, 0x00, 0x04, 0xAD, 0x1C, 0x54, 0x65, 0x73, 0x74, 0x20,
                                0x77, 0x69, 0x74, 0x68, 0x20, 0x73, 0x70, 0x65, 0x63, 0x69,
                                0x61, 0x6C, 0x20, 0x63, 0x68, 0x61, 0x72, 0x20, 0x5B, 0x20,
                                0x2E, 0x2E, 0x2E, 0x00,
                        },
                },
        },
        /* 4 */ {
                .dest = "+33661651866",
                .text = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890",
                .type = PA_SMS_SMS_SUBMIT,
                .pdu_7bits = {
                        .conversionResult = LE_OK,
                        .length = 155,
                        .data = {
                            0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                            0xF6, 0x00, 0x00, 0xAD, 0xA0, 0x31, 0xD9, 0x8C, 0x56, 0xB3,
                            0xDD, 0x70, 0x39, 0x58, 0x4C, 0x36, 0xA3, 0xD5, 0x6C, 0x37,
                            0x5C, 0x0E, 0x16, 0x93, 0xCD, 0x68, 0x35, 0xDB, 0x0D, 0x97,
                            0x83, 0xC5, 0x64, 0x33, 0x5A, 0xCD, 0x76, 0xC3, 0xE5, 0x60,
                            0x31, 0xD9, 0x8C, 0x56, 0xB3, 0xDD, 0x70, 0x39, 0x58, 0x4C,
                            0x36, 0xA3, 0xD5, 0x6C, 0x37, 0x5C, 0x0E, 0x16, 0x93, 0xCD,
                            0x68, 0x35, 0xDB, 0x0D, 0x97, 0x83, 0xC5, 0x64, 0x33, 0x5A,
                            0xCD, 0x76, 0xC3, 0xE5, 0x60, 0x31, 0xD9, 0x8C, 0x56, 0xB3,
                            0xDD, 0x70, 0x39, 0x58, 0x4C, 0x36, 0xA3, 0xD5, 0x6C, 0x37,
                            0x5C, 0x0E, 0x16, 0x93, 0xCD, 0x68, 0x35, 0xDB, 0x0D, 0x97,
                            0x83, 0xC5, 0x64, 0x33, 0x5A, 0xCD, 0x76, 0xC3, 0xE5, 0x60,
                            0x31, 0xD9, 0x8C, 0x56, 0xB3, 0xDD, 0x70, 0x39, 0x58, 0x4C,
                            0x36, 0xA3, 0xD5, 0x6C, 0x37, 0x5C, 0x0E, 0x16, 0x93, 0xCD,
                            0x68, 0x35, 0xDB, 0x0D, 0x97, 0x83, 0xC5, 0x64, 0x33, 0x5A,
                            0xCD, 0x76, 0xC3, 0xE5, 0x60, 0x00,
                        },
                },
                .pdu_8bits = {
                        .conversionResult = LE_OVERFLOW,
                        .length = 0,
                        .data = {
                            0x00,
                        }
                },
        },
        /* 5 */ {
                .dest = "+33661651866",
                .text = "[123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789",
                .type = PA_SMS_SMS_SUBMIT,
                .pdu_7bits = {
                        .conversionResult = LE_OVERFLOW,
                        .length = 0,
                        .data = {
                            0x00,
                        },
                },
                .pdu_8bits = {
                        .conversionResult = LE_OVERFLOW,
                        .length = 0,
                        .data = {
                            0x00,
                        }
                },
        },
        /* 6 */ {
                .dest = "+33661651866",
                .text = "Test with special char [ ] ^ { } \\ ~ | ...!",
                .type = PA_SMS_SMS_SUBMIT,
                .pdu_7bits = {
                        .conversionResult = LE_OK,
                        .length = 60,
                        .data = {
                            0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                            0xF6, 0x00, 0x00, 0xAD, 0x33, 0xD4, 0xF2, 0x9C, 0x0E, 0xBA,
                            0xA7, 0xE9, 0x68, 0xD0, 0x1C, 0x5E, 0x1E, 0xA7, 0xC3, 0x6C,
                            0xD0, 0x18, 0x1D, 0x96, 0x83, 0x36, 0x3C, 0xD0, 0xC6, 0x07,
                            0xDA, 0x50, 0x40, 0x1B, 0x14, 0x68, 0x93, 0x02, 0x6D, 0x5E,
                            0xA0, 0x4D, 0x0F, 0xB4, 0x01, 0x82, 0x5C, 0x2E, 0x57, 0x08,
                            0x00
                        },
                },
                .pdu_8bits = {
                        .conversionResult = LE_OK,
                        .length = 58,
                        .data = {
                            0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                            0xF6, 0x00, 0x04, 0xAD, 0x2B, 0x54, 0x65, 0x73, 0x74, 0x20,
                            0x77, 0x69, 0x74, 0x68, 0x20, 0x73, 0x70, 0x65, 0x63, 0x69,
                            0x61, 0x6C, 0x20, 0x63, 0x68, 0x61, 0x72, 0x20, 0x5B, 0x20,
                            0x5D, 0x20, 0x5E, 0x20, 0x7B, 0x20, 0x7D, 0x20, 0x5C, 0x20,
                            0x7E, 0x20, 0x7C, 0x20, 0x2E, 0x2E, 0x2E, 0x21, 0x00,
                        }
                },
        },
};


typedef struct {
    const size_t length;
    const uint8_t data[256];
    struct {
        le_result_t result;
        const char * source;
        smsPdu_Encoding_t encoding;
        pa_sms_Message_t message;
    } expected;
} PduReceived_t;

/**
 * Array to store samples of received messages and their expected decoding.
 */
const PduReceived_t PduReceivedDb[] = {
        /* 0 */ {
                .length = 116,
                .data = { 0x07, 0x91, 0x33, 0x86, 0x09, 0x40, 0x00, 0xF0, 0x04, 0x0B,
                        0x91, 0x33, 0x46, 0x53, 0x73, 0x19, 0xF9, 0x00, 0x00, 0x41,
                        0x70, 0x13, 0x02, 0x55, 0x71, 0x80, 0x65, 0xCC, 0xB7, 0xBC,
                        0xDC, 0x06, 0xA5, 0xE1, 0xF3, 0x7A, 0x1B, 0x44, 0x7E, 0xB3,
                        0xDF, 0x72, 0xD0, 0x3C, 0x4D, 0x07, 0x85, 0xDB, 0x65, 0x3A,
                        0x0B, 0x34, 0x7E, 0xBB, 0xE7, 0xE5, 0x31, 0xBD, 0x4C, 0xAF,
                        0xCB, 0x41, 0x61, 0x72, 0x1A, 0x9E, 0x9E, 0x8F, 0xD3, 0xEE,
                        0x33, 0xA8, 0xCC, 0x4E, 0xD3, 0x5D, 0xA0, 0xE6, 0x5B, 0x2E,
                        0x4E, 0x83, 0xD2, 0x6E, 0xD0, 0xF8, 0xDD, 0x6E, 0xBF, 0xC9,
                        0x6F, 0x10, 0xBB, 0x3C, 0xA6, 0xD7, 0xE7, 0x2C, 0x50, 0xBC,
                        0x9E, 0x9E, 0x83, 0xEC, 0x6F, 0x76, 0x9D, 0x0E, 0x0F, 0xD3,
                        0x41, 0x65, 0x79, 0x98, 0xEE, 0x02,
                },
                .expected = {
                        .result = LE_OK,
                        .encoding = SMSPDU_GSM_7_BITS,
                        .message = {
                                .type = PA_SMS_SMS_DELIVER,
                                .smsDeliver = {
                                        .oa = "+33643537919",
                                        .format = LE_SMS_FORMAT_TEXT,
                                        .scts = "14/07/31,20:55:17-00",
                                        .data = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi in commodo lectus, quis volutpat erat.",
                                        .dataLen = 101,
                                },
                        },
                },
        },
        /* 1 */ {
                .length = 33,
                .data = {
                        0x07, 0x91, 0x33, 0x86, 0x09, 0x40, 0x00, 0xF0, 0x04, 0x0B,
                        0x91, 0x33, 0x76, 0x63, 0x47, 0x53, 0xF9, 0x00, 0x00, 0x41,
                        0x70, 0x13, 0x22, 0x30, 0x61, 0x80, 0x06, 0x53, 0x7A, 0x98,
                        0x5E, 0x9F, 0x03,
                },
                .expected = {
                        .result = LE_OK,
                        .message = {
                                .type = PA_SMS_SMS_DELIVER,
                                .smsDeliver = {
                                        .oa = "+33673674359",
                                        .format = LE_SMS_FORMAT_TEXT,
                                        .scts = "14/07/31,22:03:16-00",
                                        .data = "Status",
                                        .dataLen = 6,
                                },
                        },
                },
        },
};

/* The suite initialization function.
* Opens the temporary file used by the tests.
* Returns zero on success, non-zero otherwise.
*/
int init_suite(void)
{
    return 0;
}

/* The suite cleanup function.
* Closes the temporary file used by the tests.
* Returns zero on success, non-zero otherwise.
*/
int clean_suite(void)
{
    return 0;
}

void DumpPDU(const uint8_t * dataPtr, size_t length) {
    int i;
    const int columns = 32;

    for( i = 0; i < length+1; i++) {
        fprintf(stderr, "%02X", dataPtr[i]);
        if(i % columns == (columns-1) )
            fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}

void testDecodePdu()
{
    pa_sms_Message_t message;
    le_result_t res;
    int i;

    le_log_SetFilterLevel(LE_LOG_DEBUG);

    for ( i = 0; i < sizeof(PduReceivedDb)/sizeof(PduReceived_t); i++)
    {
        const PduReceived_t * receivedPtr = &PduReceivedDb[i];

        fprintf(stderr, "\n=> Index %d\n", i);

        res = smsPdu_Decode(receivedPtr->data, &message);
        CU_ASSERT_EQUAL(res, receivedPtr->expected.result);

        if(res != LE_OK)
            continue;

        CU_ASSERT_EQUAL(message.type, receivedPtr->expected.message.type);

        switch(message.type)
        {
            case PA_SMS_SMS_DELIVER:
                fprintf(stderr, "Format: %u\n", message.smsDeliver.format);
                fprintf(stderr, "Data (%u): %s\n", message.smsDeliver.dataLen, message.smsDeliver.data);
                CU_ASSERT_EQUAL( message.smsDeliver.format, receivedPtr->expected.message.smsDeliver.format );
                CU_ASSERT_STRING_EQUAL( message.smsDeliver.oa, receivedPtr->expected.message.smsDeliver.oa );
                CU_ASSERT_STRING_EQUAL( message.smsDeliver.scts, receivedPtr->expected.message.smsDeliver.scts );
                CU_ASSERT_EQUAL( message.smsDeliver.dataLen, receivedPtr->expected.message.smsDeliver.dataLen );
                CU_ASSERT_EQUAL(0, memcmp(message.smsDeliver.data, receivedPtr->expected.message.smsDeliver.data, message.smsDeliver.dataLen) );
                break;

            case PA_SMS_SMS_SUBMIT:
                CU_FAIL("Unexpected submit");
                break;

            default:
                CU_FAIL("Unexpected type");
                break;
        }
    }
}

void testEncodePdu()
{
    pa_sms_Pdu_t pdu;
    pa_sms_Message_t message;
    le_result_t res;
    int i;

    for( i = 0; i < sizeof(PduAssocDb)/sizeof(PduAssoc_t); i++)
    {
        const PduAssoc_t * assoc = &PduAssocDb[i];
        size_t messageLength = strlen(assoc->text);

        fprintf(stderr, "\n=> Index %d\n", i);

        fprintf(stderr, "Text (%zd): (%s)\n",messageLength, assoc->text);

        /* Encode 8 bits */
        fprintf(stderr, "Encoding in 8 bits\n");
        res = smsPdu_Encode((const uint8_t*)assoc->text, strlen(assoc->text), assoc->dest, SMSPDU_8_BITS, assoc->type, &pdu);
        CU_ASSERT_EQUAL( res, assoc->pdu_8bits.conversionResult );

        if ( res == LE_OK )
        {
            fprintf(stderr, "Source: (%zu)\n", assoc->pdu_8bits.length);
            DumpPDU(assoc->pdu_8bits.data, assoc->pdu_8bits.length);

            fprintf(stderr, "Encoded: (%u)\n", pdu.dataLen);
            DumpPDU(pdu.data, pdu.dataLen);

            /* Check */
            CU_ASSERT_EQUAL( pdu.dataLen, assoc->pdu_8bits.length );
            CU_ASSERT_EQUAL( memcmp(pdu.data, assoc->pdu_8bits.data, pdu.dataLen), 0);

            /* Decode */
            res = smsPdu_Decode(pdu.data, &message);
            CU_ASSERT_EQUAL( res, LE_OK );
            CU_ASSERT_EQUAL( message.type, assoc->type );

            switch(message.type)
            {
                case PA_SMS_SMS_DELIVER:
                    CU_ASSERT_EQUAL( message.smsDeliver.format, LE_SMS_FORMAT_BINARY );
                    CU_ASSERT_EQUAL( memcmp(message.smsDeliver.data,assoc->text,strlen(assoc->text)),0);
                    break;

                case PA_SMS_SMS_SUBMIT:
                    fprintf(stderr, "Data (%u): %s\n", message.smsSubmit.dataLen, message.smsSubmit.data);
                    CU_ASSERT_EQUAL( message.smsSubmit.format, LE_SMS_FORMAT_BINARY );
                    CU_ASSERT_EQUAL( memcmp(message.smsSubmit.data,assoc->text,strlen(assoc->text)),0);
                    break;

                default:
                    CU_FAIL("Unexpected type");
                    break;
            }
        }
        fprintf(stderr, "------------------\n");

        /* Encode 7 bits */
        fprintf(stderr, "Encoding in 7 bits\n");
        res = smsPdu_Encode((const uint8_t*)assoc->text, strlen(assoc->text), assoc->dest, SMSPDU_GSM_7_BITS, assoc->type, &pdu);
        CU_ASSERT_EQUAL( res, assoc->pdu_7bits.conversionResult );

        if ( res == LE_OK )
        {
            fprintf(stderr, "Source: (%zu)\n", assoc->pdu_7bits.length);
            DumpPDU(assoc->pdu_7bits.data, assoc->pdu_7bits.length);

            fprintf(stderr, "Encoded: (%u)\n", pdu.dataLen);
            DumpPDU(pdu.data, pdu.dataLen);

            /* Check */
            CU_ASSERT_EQUAL( pdu.dataLen, assoc->pdu_7bits.length );
            CU_ASSERT_EQUAL( memcmp(pdu.data, assoc->pdu_7bits.data, pdu.dataLen), 0);

            /* Decode */
            res = smsPdu_Decode(pdu.data, &message);
            CU_ASSERT_EQUAL( res, LE_OK );
            CU_ASSERT_EQUAL( message.type, assoc->type );
            fprintf(stderr, "Type: %u\n", message.type);

            switch(message.type)
            {
                case PA_SMS_SMS_DELIVER:
                    fprintf(stderr, "Format: %u\n", message.smsDeliver.format);
                    fprintf(stderr, "Data (%u): %s\n", message.smsDeliver.dataLen, message.smsDeliver.data);
                    CU_ASSERT_EQUAL( message.smsDeliver.format, LE_SMS_FORMAT_TEXT );
                    CU_ASSERT_EQUAL( memcmp(message.smsDeliver.data,assoc->text,strlen(assoc->text)),0);
                    break;

                case PA_SMS_SMS_SUBMIT:
                    fprintf(stderr, "Format: %u\n", message.smsSubmit.format);
                    fprintf(stderr, "Data (%u): %s\n", message.smsSubmit.dataLen, message.smsSubmit.data);
                    CU_ASSERT_EQUAL( message.smsSubmit.format, LE_SMS_FORMAT_TEXT );
                    CU_ASSERT_EQUAL( memcmp(message.smsSubmit.data,assoc->text,strlen(assoc->text)),0);
                    break;

                default:
                    CU_FAIL("Unexpected type");
                    break;
            }
        }
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
            { "PDU Convert tests", init_suite, clean_suite, testCases },
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
