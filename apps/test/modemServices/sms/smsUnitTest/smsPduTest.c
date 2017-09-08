/**
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include <legato.h>
#include "interfaces.h"

#include "pa_sms.h"
#include "cdmaPdu.h"
#include "smsPdu.h"
#include "main.h"


#define PDU_MAX     256


typedef struct
{
        const char * dest;
        const char * text;
        pa_sms_MsgType_t type;
        bool statusReportEnabled;
        struct
        {
                const size_t length;
                const uint8_t data[256];
                le_result_t conversionResult;
        } gsm_7bits, gsm_8bits, gsm_ucs2;
        struct
        {
                const size_t length;
                const uint8_t data[256];
                le_result_t conversionResult;
                uint8_t timestampIndex; // Use to not check timestamp value
        } cdma_7bits, cdma_8bits ;
} PduAssoc_t;

static const PduAssoc_t PduAssocDb[] =
{
    /* 0 */
    {
        .dest = "+33661651866",
        .text = "Test sending message",
        .type = PA_SMS_SUBMIT,
        .statusReportEnabled = false,
        .gsm_7bits =
        {
            .conversionResult = LE_OK,
            .length = 33,
            .data = {
                0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                0xF6, 0x00, 0x00, 0xAD, 0x14, 0xD4, 0xF2, 0x9C, 0x0E, 0x9A,
                0x97, 0xDD, 0xE4, 0xB4, 0xFB, 0x0C, 0x6A, 0x97, 0xE7, 0xF3,
                0xF0, 0xB9, 0x0C,
            },
        },
        .gsm_8bits =
        {
            .conversionResult = LE_OK,
            .length = 35,
            .data =
            {
                0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                0xF6, 0x00, 0x04, 0xAD, 0x14, 0x54, 0x65, 0x73, 0x74, 0x20,
                0x73, 0x65, 0x6E, 0x64, 0x69, 0x6E, 0x67, 0x20, 0x6D, 0x65,
                0x73, 0x73, 0x61, 0x67, 0x65,
            },
        },
        .gsm_ucs2 =
        {
            .conversionResult = LE_OK,
            .length = 55,
            .data =
            {
                0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61,
                0x15, 0x68, 0xF6, 0x00, 0x08, 0xAD, 0x28, 0x00,
                0x54, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00,
                0x20, 0x00, 0x73, 0x00, 0x65, 0x00, 0x6E, 0x00,
                0x64, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x67, 0x00,
                0x20, 0x00, 0x6D, 0x00, 0x65, 0x00, 0x73, 0x00,
                0x73, 0x00, 0x61, 0x00, 0x67, 0x00, 0x65,
            },
        },
        .cdma_7bits =
        {
            .conversionResult = LE_OK,
            .length = 54,
            .data = {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xCC, 0xD9,
                0x85, 0x94, 0x61, 0x98, 0x08, 0x26, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x14, 0x10, 0xA5, 0x4C, 0xBC, 0xFA, 0x20, 0xE7,
                0x97, 0x76, 0x4D, 0x3B, 0xB3, 0xA0, 0xDB, 0x97, 0x9F, 0x3C,
                0x39, 0xF2, 0x80, 0x03, 0x06, 0x14, 0x07, 0x11, 0x16, 0x53,
                0x27, 0x08, 0x01, 0x00,
            },
            .timestampIndex = 43,
        },
        .cdma_8bits =
        {
            .conversionResult = LE_OK,
            .length = 56,
            .data = {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xCC, 0xD9,
                0x85, 0x94, 0x61, 0x98, 0x08, 0x28, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x16, 0x00, 0xA2, 0xA3, 0x2B, 0x9B, 0xA1, 0x03,
                0x9B, 0x2B, 0x73, 0x23, 0x4B, 0x73, 0x39, 0x03, 0x6B, 0x2B,
                0x9B, 0x9B, 0x0B, 0x3B, 0x28, 0x03, 0x06, 0x14, 0x07, 0x06,
                0x14, 0x18, 0x50, 0x08, 0x01, 0x00,
            },
            .timestampIndex = 45,
        },
    },
    /* 1 */
    {
        .dest = "+33617190547",
        .text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi in commodo lectus,"
                        " quis volutpat erat.",
        .type = PA_SMS_SUBMIT,
        .statusReportEnabled = false,
        .gsm_7bits =
        {
            .conversionResult = LE_OK,
            .length = 104,
            .data =
            {
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
        .gsm_8bits =
        {
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
        .cdma_7bits =
        {
            .conversionResult = LE_OK,
            .length = 124,
            .data = {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xCC, 0xD8,
                0x5C, 0x66, 0x95, 0x1C, 0x08, 0x6C, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x5A, 0x13, 0x2C, 0xCD, 0xFC, 0xB2, 0xED, 0x41,
                0xA7, 0x87, 0x3E, 0xBB, 0x50, 0x64, 0xDF, 0xB3, 0x7F, 0x24,
                0x1C, 0xF4, 0xF4, 0x41, 0x87, 0x6E, 0x5E, 0x8B, 0x10, 0x63,
                0xDF, 0xBB, 0x9E, 0x5C, 0x7D, 0x32, 0xF4, 0xEB, 0xC9, 0x06,
                0x1C, 0x9A, 0x78, 0x69, 0xE7, 0x8F, 0x4E, 0xEC, 0xE8, 0x32,
                0xEC, 0xD3, 0xD1, 0x72, 0x09, 0xBB, 0xF9, 0x62, 0xD2, 0x83,
                0x4E, 0xE4, 0x18, 0xF7, 0xED, 0xDB, 0xBF, 0x26, 0xF4, 0x1B,
                0x32, 0xE3, 0xE9, 0xD7, 0x9A, 0xC4, 0x1C, 0x7A, 0xE9, 0xE6,
                0x83, 0xB6, 0xFD, 0x9D, 0x7A, 0x70, 0xC3, 0xD1, 0x06, 0x5E,
                0x58, 0x7A, 0x2E, 0x03, 0x06, 0x14, 0x07, 0x11, 0x16, 0x53,
                0x27, 0x08, 0x01, 0x00,
            },
            .timestampIndex = 113,
        },
        .cdma_8bits =
        {
            .conversionResult = LE_OK,
            .length = 137,
            .data = {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xCC, 0xD8,
                0x5C, 0x66, 0x95, 0x1C, 0x08, 0x79, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x67, 0x03, 0x2A, 0x63, 0x7B, 0x93, 0x2B, 0x69,
                0x03, 0x4B, 0x83, 0x9B, 0xAB, 0x69, 0x03, 0x23, 0x7B, 0x63,
                0x7B, 0x91, 0x03, 0x9B, 0x4B, 0xA1, 0x03, 0x0B, 0x6B, 0x2B,
                0xA1, 0x61, 0x03, 0x1B, 0x7B, 0x73, 0x9B, 0x2B, 0x1B, 0xA3,
                0x2B, 0xA3, 0xAB, 0x91, 0x03, 0x0B, 0x23, 0x4B, 0x83, 0x4B,
                0x9B, 0x1B, 0x4B, 0x73, 0x39, 0x03, 0x2B, 0x63, 0x4B, 0xA1,
                0x71, 0x02, 0x6B, 0x7B, 0x93, 0x13, 0x49, 0x03, 0x4B, 0x71,
                0x03, 0x1B, 0x7B, 0x6B, 0x6B, 0x7B, 0x23, 0x79, 0x03, 0x63,
                0x2B, 0x1B, 0xA3, 0xAB, 0x99, 0x61, 0x03, 0x8B, 0xAB, 0x4B,
                0x99, 0x03, 0xB3, 0x7B, 0x63, 0xAB, 0xA3, 0x83, 0x0B, 0xA1,
                0x03, 0x2B, 0x93, 0x0B, 0xA1, 0x70, 0x03, 0x06, 0x14, 0x07,
                0x07, 0x10, 0x27, 0x08, 0x08, 0x01, 0x00,
            },
            .timestampIndex = 126,
        },
    },
    /* 2 */
    {
        .dest = "0617190547",
        .text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi in commodo lectus"
                        ", quis volutpat erat.",
        .type = PA_SMS_SUBMIT,
        .statusReportEnabled = false,
        .gsm_7bits =
        {
            .conversionResult = LE_OK,
            .length = 103,
            .data =
            {
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
        .gsm_8bits =
        {
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
        .cdma_7bits =
        {
            .conversionResult = LE_OK,
            .length = 124,
            .data =
            {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xA9, 0x85,
                0xC6, 0x69, 0x51, 0xC0, 0x08, 0x6C, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x5A, 0x13, 0x2C, 0xCD, 0xFC, 0xB2, 0xED, 0x41,
                0xA7, 0x87, 0x3E, 0xBB, 0x50, 0x64, 0xDF, 0xB3, 0x7F, 0x24,
                0x1C, 0xF4, 0xF4, 0x41, 0x87, 0x6E, 0x5E, 0x8B, 0x10, 0x63,
                0xDF, 0xBB, 0x9E, 0x5C, 0x7D, 0x32, 0xF4, 0xEB, 0xC9, 0x06,
                0x1C, 0x9A, 0x78, 0x69, 0xE7, 0x8F, 0x4E, 0xEC, 0xE8, 0x32,
                0xEC, 0xD3, 0xD1, 0x72, 0x09, 0xBB, 0xF9, 0x62, 0xD2, 0x83,
                0x4E, 0xE4, 0x18, 0xF7, 0xED, 0xDB, 0xBF, 0x26, 0xF4, 0x1B,
                0x32, 0xE3, 0xE9, 0xD7, 0x9A, 0xC4, 0x1C, 0x7A, 0xE9, 0xE6,
                0x83, 0xB6, 0xFD, 0x9D, 0x7A, 0x70, 0xC3, 0xD1, 0x06, 0x5E,
                0x58, 0x7A, 0x2E, 0x03, 0x06, 0x14, 0x07, 0x11, 0x16, 0x53,
                0x27, 0x08, 0x01, 0x00,
            },
            .timestampIndex = 113,
        },
        .cdma_8bits =
        {
            .conversionResult = LE_OK,
            .length = 137,
            .data =
            {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xA9, 0x85,
                0xC6, 0x69, 0x51, 0xC0, 0x08, 0x79, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x67, 0x03, 0x2A, 0x63, 0x7B, 0x93, 0x2B, 0x69,
                0x03, 0x4B, 0x83, 0x9B, 0xAB, 0x69, 0x03, 0x23, 0x7B, 0x63,
                0x7B, 0x91, 0x03, 0x9B, 0x4B, 0xA1, 0x03, 0x0B, 0x6B, 0x2B,
                0xA1, 0x61, 0x03, 0x1B, 0x7B, 0x73, 0x9B, 0x2B, 0x1B, 0xA3,
                0x2B, 0xA3, 0xAB, 0x91, 0x03, 0x0B, 0x23, 0x4B, 0x83, 0x4B,
                0x9B, 0x1B, 0x4B, 0x73, 0x39, 0x03, 0x2B, 0x63, 0x4B, 0xA1,
                0x71, 0x02, 0x6B, 0x7B, 0x93, 0x13, 0x49, 0x03, 0x4B, 0x71,
                0x03, 0x1B, 0x7B, 0x6B, 0x6B, 0x7B, 0x23, 0x79, 0x03, 0x63,
                0x2B, 0x1B, 0xA3, 0xAB, 0x99, 0x61, 0x03, 0x8B, 0xAB, 0x4B,
                0x99, 0x03, 0xB3, 0x7B, 0x63, 0xAB, 0xA3, 0x83, 0x0B, 0xA1,
                0x03, 0x2B, 0x93, 0x0B, 0xA1, 0x70, 0x03, 0x06, 0x14, 0x07,
                0x07, 0x10, 0x38, 0x26, 0x08, 0x01, 0x00,
            },
            .timestampIndex = 126,
        },
    },
    /* 3 */
    {
        .dest = "+33661651866",
        .text = "Test with special char [ ...",
        .type = PA_SMS_SUBMIT,
        .statusReportEnabled = false,
        .gsm_7bits =
        {
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
        .gsm_8bits =
        {
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
        .cdma_7bits =
        {
            .conversionResult = LE_OK,
            .length = 61,
            .data = {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xCC, 0xD9,
                0x85, 0x94, 0x61, 0x98, 0x08, 0x2D, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x1B, 0x10, 0xE5, 0x4C, 0xBC, 0xFA, 0x20, 0xEF,
                0xA7, 0xA6, 0x84, 0x1C, 0xF8, 0x65, 0xC7, 0xA7, 0x0E, 0xC4,
                0x18, 0xF4, 0x61, 0xE4, 0x82, 0xDA, 0x05, 0xCB, 0x97, 0x00,
                0x03, 0x06, 0x14, 0x07, 0x20, 0x14, 0x43, 0x32, 0x08, 0x01,
                0x00,
            },
            .timestampIndex = 50,
        },
        .cdma_8bits =
        {
            .conversionResult = LE_OK,
            .length = 64,
            .data =
            {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xCC, 0xD9,
                0x85, 0x94, 0x61, 0x98, 0x08, 0x30, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x1E, 0x00, 0xE2, 0xA3, 0x2B, 0x9B, 0xA1, 0x03,
                0xBB, 0x4B, 0xA3, 0x41, 0x03, 0x9B, 0x83, 0x2B, 0x1B, 0x4B,
                0x0B, 0x61, 0x03, 0x1B, 0x43, 0x0B, 0x91, 0x02, 0xD9, 0x01,
                0x71, 0x71, 0x70, 0x03, 0x06, 0x14, 0x07, 0x07, 0x10, 0x42,
                0x39, 0x08, 0x01, 0x00,
            },
            .timestampIndex = 53,
        },
    },
    /* 4 */
    {
        .dest = "+33661651866",
        .text = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890",
        .type = PA_SMS_SUBMIT,
        .statusReportEnabled = false,
        .gsm_7bits =
        {
            .conversionResult = LE_OK,
            .length = 155,
            .data =
            {
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
        .gsm_8bits =
        {
            .conversionResult = LE_OVERFLOW,
            .length = 0,
            .data =
            {
                0x00,
            }
        },
        .cdma_7bits =
        {
            .conversionResult = LE_OK,
            .length = 176,
            .data =
            {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xCC, 0xD9,
                0x85, 0x94, 0x61, 0x98, 0x08, 0xA0, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x8E, 0x15, 0x03, 0x16, 0x4C, 0xDA, 0x35, 0x6C,
                0xDD, 0xC3, 0x96, 0x0C, 0x59, 0x33, 0x68, 0xD5, 0xB3, 0x77,
                0x0E, 0x58, 0x31, 0x64, 0xCD, 0xA3, 0x56, 0xCD, 0xDC, 0x39,
                0x60, 0xC5, 0x93, 0x36, 0x8D, 0x5B, 0x37, 0x70, 0xE5, 0x83,
                0x16, 0x4C, 0xDA, 0x35, 0x6C, 0xDD, 0xC3, 0x96, 0x0C, 0x59,
                0x33, 0x68, 0xD5, 0xB3, 0x77, 0x0E, 0x58, 0x31, 0x64, 0xCD,
                0xA3, 0x56, 0xCD, 0xDC, 0x39, 0x60, 0xC5, 0x93, 0x36, 0x8D,
                0x5B, 0x37, 0x70, 0xE5, 0x83, 0x16, 0x4C, 0xDA, 0x35, 0x6C,
                0xDD, 0xC3, 0x96, 0x0C, 0x59, 0x33, 0x68, 0xD5, 0xB3, 0x77,
                0x0E, 0x58, 0x31, 0x64, 0xCD, 0xA3, 0x56, 0xCD, 0xDC, 0x39,
                0x60, 0xC5, 0x93, 0x36, 0x8D, 0x5B, 0x37, 0x70, 0xE5, 0x83,
                0x16, 0x4C, 0xDA, 0x35, 0x6C, 0xDD, 0xC3, 0x96, 0x0C, 0x59,
                0x33, 0x68, 0xD5, 0xB3, 0x77, 0x0E, 0x58, 0x31, 0x64, 0xCD,
                0xA3, 0x56, 0xCD, 0xDC, 0x39, 0x60, 0xC5, 0x93, 0x36, 0x8D,
                0x5B, 0x37, 0x70, 0xE5, 0x80, 0x03, 0x06, 0x14, 0x07, 0x11,
                0x17, 0x47, 0x42, 0x08, 0x01, 0x00, 0xB0,

            },
            .timestampIndex = 165,
        },
        .cdma_8bits =
        {
            .conversionResult = LE_OVERFLOW,
            .length = 0,
            .data =
            {
                0x00,
            },
            .timestampIndex = 0,
        },
    },
    /* 5 */
    {
        .dest = "+33661651866",
        .text = "[123456789012345678901234567890123456789012345678901234567890123456789012"
                "3456789012345678901234567890123456789012345678901234567890123456789012345"
                "67890123456789",
        .type = PA_SMS_SUBMIT,
        .statusReportEnabled = false,
        .gsm_7bits =
        {
            .conversionResult = LE_OVERFLOW,
            .length = 0,
            .data =
            {
                0x00,
            },
        },
        .gsm_8bits =
        {
            .conversionResult = LE_OVERFLOW,
            .length = 0,
            .data =
            {
                0x00,
            }
        },
        .cdma_7bits =
        {
            .conversionResult = LE_OK,
            .length = 176,
            .data =
            {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xCC, 0xD9,
                0x85, 0x94, 0x61, 0x98, 0x08, 0xA0, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x8E, 0x15, 0x05, 0xB6, 0x2C, 0x99, 0xB4, 0x6A,
                0xD9, 0xBB, 0x87, 0x2C, 0x18, 0xB2, 0x66, 0xD1, 0xAB, 0x66,
                0xEE, 0x1C, 0xB0, 0x62, 0xC9, 0x9B, 0x46, 0xAD, 0x9B, 0xB8,
                0x72, 0xC1, 0x8B, 0x26, 0x6D, 0x1A, 0xB6, 0x6E, 0xE1, 0xCB,
                0x06, 0x2C, 0x99, 0xB4, 0x6A, 0xD9, 0xBB, 0x87, 0x2C, 0x18,
                0xB2, 0x66, 0xD1, 0xAB, 0x66, 0xEE, 0x1C, 0xB0, 0x62, 0xC9,
                0x9B, 0x46, 0xAD, 0x9B, 0xB8, 0x72, 0xC1, 0x8B, 0x26, 0x6D,
                0x1A, 0xB6, 0x6E, 0xE1, 0xCB, 0x06, 0x2C, 0x99, 0xB4, 0x6A,
                0xD9, 0xBB, 0x87, 0x2C, 0x18, 0xB2, 0x66, 0xD1, 0xAB, 0x66,
                0xEE, 0x1C, 0xB0, 0x62, 0xC9, 0x9B, 0x46, 0xAD, 0x9B, 0xB8,
                0x72, 0xC1, 0x8B, 0x26, 0x6D, 0x1A, 0xB6, 0x6E, 0xE1, 0xCB,
                0x06, 0x2C, 0x99, 0xB4, 0x6A, 0xD9, 0xBB, 0x87, 0x2C, 0x18,
                0xB2, 0x66, 0xD1, 0xAB, 0x66, 0xEE, 0x1C, 0xB0, 0x62, 0xC9,
                0x9B, 0x46, 0xAD, 0x9B, 0xB8, 0x72, 0xC1, 0x8B, 0x26, 0x6D,
                0x1A, 0xB6, 0x6E, 0xE1, 0xC8, 0x03, 0x06, 0x14, 0x07, 0x20,
                0x16, 0x02, 0x22, 0x08, 0x01, 0x00, 0xB0,
            },
            .timestampIndex = 165,
        },
        .cdma_8bits =
        {
            .conversionResult = LE_OVERFLOW,
            .length = 0,
            .data =
            {
                0x00,
            },
            .timestampIndex = 0,
        },
    },
    /* 6 */
    {
        .dest = "+33661651866",
        .text = "Test with special char [ ] ^ { } \\ ~ | ...!",
        .type = PA_SMS_SUBMIT,
        .statusReportEnabled = false,
        .gsm_7bits =
        {
            .conversionResult = LE_OK,
            .length = 60,
            .data =
            {
                0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                0xF6, 0x00, 0x00, 0xAD, 0x33, 0xD4, 0xF2, 0x9C, 0x0E, 0xBA,
                0xA7, 0xE9, 0x68, 0xD0, 0x1C, 0x5E, 0x1E, 0xA7, 0xC3, 0x6C,
                0xD0, 0x18, 0x1D, 0x96, 0x83, 0x36, 0x3C, 0xD0, 0xC6, 0x07,
                0xDA, 0x50, 0x40, 0x1B, 0x14, 0x68, 0x93, 0x02, 0x6D, 0x5E,
                0xA0, 0x4D, 0x0F, 0xB4, 0x01, 0x82, 0x5C, 0x2E, 0x57, 0x08,
                0x00
            },
        },
        .gsm_8bits =
        {
            .conversionResult = LE_OK,
            .length = 58,
            .data =
            {
                0x00, 0x11, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                0xF6, 0x00, 0x04, 0xAD, 0x2B, 0x54, 0x65, 0x73, 0x74, 0x20,
                0x77, 0x69, 0x74, 0x68, 0x20, 0x73, 0x70, 0x65, 0x63, 0x69,
                0x61, 0x6C, 0x20, 0x63, 0x68, 0x61, 0x72, 0x20, 0x5B, 0x20,
                0x5D, 0x20, 0x5E, 0x20, 0x7B, 0x20, 0x7D, 0x20, 0x5C, 0x20,
                0x7E, 0x20, 0x7C, 0x20, 0x2E, 0x2E, 0x2E, 0x21, 0x00,
            }
        },
        .cdma_7bits =
        {
            .conversionResult = LE_OK,
            .length = 74,
            .data =
            {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xCC, 0xD9,
                0x85, 0x94, 0x61, 0x98, 0x08, 0x3A, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x28, 0x11, 0x5D, 0x4C, 0xBC, 0xFA, 0x20, 0xEF,
                0xA7, 0xA6, 0x84, 0x1C, 0xF8, 0x65, 0xC7, 0xA7, 0x0E, 0xC4,
                0x18, 0xF4, 0x61, 0xE4, 0x82, 0xDA, 0x0B, 0xA8, 0x2F, 0x20,
                0xF6, 0x83, 0xEA, 0x0B, 0x88, 0x3F, 0x20, 0xF8, 0x81, 0x72,
                0xE5, 0xC8, 0x40, 0x03, 0x06, 0x14, 0x07, 0x20, 0x14, 0x43,
                0x32, 0x08, 0x01, 0x00, 0x00,
            },
            .timestampIndex = 63,
        },
        .cdma_8bits =
        {
            .conversionResult = LE_OK,
            .length = 79,
            .data =
            {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xCC, 0xD9,
                0x85, 0x94, 0x61, 0x98, 0x08, 0x3F, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x2D, 0x01, 0x5A, 0xA3, 0x2B, 0x9B, 0xA1, 0x03,
                0xBB, 0x4B, 0xA3, 0x41, 0x03, 0x9B, 0x83, 0x2B, 0x1B, 0x4B,
                0x0B, 0x61, 0x03, 0x1B, 0x43, 0x0B, 0x91, 0x02, 0xD9, 0x02,
                0xE9, 0x02, 0xF1, 0x03, 0xD9, 0x03, 0xE9, 0x02, 0xE1, 0x03,
                0xF1, 0x03, 0xE1, 0x01, 0x71, 0x71, 0x71, 0x08, 0x03, 0x06,
                0x14, 0x07, 0x07, 0x10, 0x50, 0x49, 0x08, 0x01, 0x00,
            },
            .timestampIndex = 68,
        },
    },
    /* 7 */
    {
        .dest = "+33661651866",
        .text = "Test sending message with Status Report",
        .type = PA_SMS_SUBMIT,
        .statusReportEnabled = true,
        .gsm_7bits =
        {
            .conversionResult = LE_OK,
            .length = 50,
            .data = {
                0x00, 0x31, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                0xF6, 0x00, 0x00, 0xAD, 0x27, 0xD4, 0xF2, 0x9C, 0x0E, 0x9A,
                0x97, 0xDD, 0xE4, 0xB4, 0xFB, 0x0C, 0x6A, 0x97, 0xE7, 0xF3,
                0xF0, 0xB9, 0x0C, 0xBA, 0xA7, 0xE9, 0x68, 0xD0, 0x94, 0x1E,
                0xA6, 0xD7, 0xE7, 0x20, 0x69, 0x19, 0xFE, 0x96, 0xD3, 0x01
            },
        },
        .gsm_8bits =
        {
            .conversionResult = LE_OK,
            .length = 54,
            .data =
            {
                0x00, 0x31, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                0xF6, 0x00, 0x04, 0xAD, 0x27, 0x54, 0x65, 0x73, 0x74, 0x20,
                0x73, 0x65, 0x6E, 0x64, 0x69, 0x6E, 0x67, 0x20, 0x6D, 0x65,
                0x73, 0x73, 0x61, 0x67, 0x65, 0x20, 0x77, 0x69, 0x74, 0x68,
                0x20, 0x53, 0x74, 0x61, 0x74, 0x75, 0x73, 0x20, 0x52, 0x65,
                0x70, 0x6F, 0x72, 0x74
            },
        },
        .gsm_ucs2 =
        {
            .conversionResult = LE_OK,
            .length = 93,
            .data =
            {
                0x00, 0x31, 0x00, 0x0B, 0x91, 0x33, 0x66, 0x61, 0x15, 0x68,
                0xF6, 0x00, 0x08, 0xAD, 0x4E, 0x00, 0x54, 0x00, 0x65, 0x00,
                0x73, 0x00, 0x74, 0x00, 0x20, 0x00, 0x73, 0x00, 0x65, 0x00,
                0x6E, 0x00, 0x64, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x67, 0x00,
                0x20, 0x00, 0x6D, 0x00, 0x65, 0x00, 0x73, 0x00, 0x73, 0x00,
                0x61, 0x00, 0x67, 0x00, 0x65, 0x00, 0x20, 0x00, 0x77, 0x00,
                0x69, 0x00, 0x74, 0x00, 0x68, 0x00, 0x20, 0x00, 0x53, 0x00,
                0x74, 0x00, 0x61, 0x00, 0x74, 0x00, 0x75, 0x00, 0x73, 0x00,
                0x20, 0x00, 0x52, 0x00, 0x65, 0x00, 0x70, 0x00, 0x6F, 0x00,
                0x72, 0x00, 0x74
            },
        },
        .cdma_7bits =
        {
            .conversionResult = LE_OK,
            .length = 70,
            .data = {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xCC, 0xD9,
                0x85, 0x94, 0x61, 0x98, 0x08, 0x36, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x24, 0x11, 0x3D, 0x4C, 0xBC, 0xFA, 0x20, 0xE7,
                0x97, 0x76, 0x4D, 0x3B, 0xB3, 0xA0, 0xDB, 0x97, 0x9F, 0x3C,
                0x39, 0xF2, 0xA0, 0xEF, 0xA7, 0xA6, 0x84, 0x14, 0xFA, 0x61,
                0xE9, 0xD7, 0x9A, 0x0A, 0x59, 0x78, 0x6F, 0xE5, 0xD0, 0x03,
                0x06, 0x17, 0x08, 0x07, 0x18, 0x52, 0x09, 0x08, 0x01, 0x00
            },
            .timestampIndex = 59,
        },
        .cdma_8bits =
        {
            .conversionResult = LE_OK,
            .length = 75,
            .data = {
                0x00, 0x00, 0x02, 0x10, 0x02, 0x04, 0x07, 0x02, 0xCC, 0xD9,
                0x85, 0x94, 0x61, 0x98, 0x08, 0x3B, 0x00, 0x03, 0x20, 0x00,
                0x10, 0x01, 0x29, 0x01, 0x3A, 0xA3, 0x2B, 0x9B, 0xA1, 0x03,
                0x9B, 0x2B, 0x73, 0x23, 0x4B, 0x73, 0x39, 0x03, 0x6B, 0x2B,
                0x9B, 0x9B, 0x0B, 0x3B, 0x29, 0x03, 0xBB, 0x4B, 0xA3, 0x41,
                0x02, 0x9B, 0xA3, 0x0B, 0xA3, 0xAB, 0x99, 0x02, 0x93, 0x2B,
                0x83, 0x7B, 0x93, 0xA0, 0x03, 0x06, 0x17, 0x08, 0x07, 0x18,
                0x46, 0x35, 0x08, 0x01, 0x00
            },
            .timestampIndex = 64,
        },
    }
};


typedef struct
{
        const bool checkLength;
        const bool checkData;
        const size_t length;
        const uint8_t data[256];
        pa_sms_Protocol_t proto;
        struct
        {
                le_result_t result;
                const char * source;
                smsPdu_Encoding_t encoding;
                pa_sms_Message_t message;
        } expected;
} PduReceived_t;

/**
 * Array to store samples of received messages and their expected decoding.
 */
static const PduReceived_t PduReceivedDb[] =
{
    {
        /* 0 */
        /*
         * 07913386094000F0040B913346537319F900004170130255718065CCB7BCDC06A5E1F37A1B
         * 447EB3DF72D03C4D0785DB653A0B347EBBE7E531BD4CAFCB4161721A9E9E8FD3EE33A8CC4E
         * D35DA0E65B2E4E83D26ED0F8DD6EBFC96F10BB3CA6D7E72C50BC9E9E83EC6F769D0E0FD341
         * 657998EE02
         * SMS DELIVER (receive)
         * Receipt requested: no
         * SMSC: 33689004000
         * Sender: 33643537919
         * TOA: 91 international, Numbering Plan: unknown
         * TimeStamp: 31/07/14 20:55:17 GMT +02:00
         * TP-PID: 00
         * TP-DCS: 00
         * TP-DCS-desc: Uncompressed Text, No class
         * Alphabet: Default (7bit)
         *
         * Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi in commodo lectus"
         * , quis volutpat erat.
         * Length: 101
         */
        .checkLength = true,
        .checkData = true,
        .proto = PA_SMS_PROTOCOL_GSM,
        .length = 116,
        .data = {
            0x07, 0x91, 0x33, 0x86, 0x09, 0x40, 0x00, 0xF0, 0x04, 0x0B,
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
        .expected =
        {
            .result = LE_OK,
            .encoding = SMSPDU_7_BITS,
            .message =
            {
    .type = PA_SMS_DELIVER,
    .smsDeliver =
    {
        .oa = "+33643537919",
        .format = LE_SMS_FORMAT_TEXT,
        .scts = "14/07/31,20:55:17+08",
                    .data = "Lorem ipsum dolor sit amet, consectetur adipiscing"
                        " elit. Morbi in commodo lectus, quis volutpat erat.",
                        .dataLen = 101,
                },
            },
        },
    },
    /* 1 */
    /*
     * 07913386094000F0040B913376634753F900004170132230618006537A985E9F03
     *
     * SMS DELIVER (receive)
     * Receipt requested: no
     * SMSC: 33689004000
     * Sender: 33673674359
     * TOA: 91 international, Numbering Plan: unknown
     * TimeStamp: 31/07/14 22:03:16 GMT +02:00
     * TP-PID: 00
     * TP-DCS: 00
     * TP-DCS-desc: Uncompressed Text, No class
     * Alphabet: Default (7bit)
     *
     * Status
     * Length: 6
     */
    {
        .checkLength = true,
        .checkData = true,
        .proto = PA_SMS_PROTOCOL_GSM,
        .length = 33,
        .data =
        {
            0x07, 0x91, 0x33, 0x86, 0x09, 0x40, 0x00, 0xF0, 0x04, 0x0B,
            0x91, 0x33, 0x76, 0x63, 0x47, 0x53, 0xF9, 0x00, 0x00, 0x41,
            0x70, 0x13, 0x22, 0x30, 0x61, 0x80, 0x06, 0x53, 0x7A, 0x98,
            0x5E, 0x9F, 0x03,
        },
        .expected =
        {
            .result = LE_OK,
            .message =
            {
                .type = PA_SMS_DELIVER,
                .smsDeliver =
                {
                    .oa = "+33673674359",
                    .format = LE_SMS_FORMAT_TEXT,
                    .scts = "14/07/31,22:03:16+08",
                    .data = "Status",
                    .dataLen = 6,
                },
            },
        },
    },
    /* 2 */
    /*
     * 07913386094000F00414D04F79D87D2E83926EF31B00F1511050812563409CD4779D5E06B14
     * F85783D0D2F839EF2B0FB5C0609EBF3B4BB3C9F83A665B93D3D2ECF41F6777D0E82CB0BF3B2
     * 9B5E06CDCB7350BB9C66B3CB75F91C647F97EB7810FC5D978364B0580D140241D9F539887C4
     * ABBCDEF39685E9783D0743A5CF77A89EBF3B4BB3C9FB305
     *
     * SMS DELIVER (receive)
     * Receipt requested: no
     * SMSC: 33689004000
     * Sender: Orange Info
     * TOA: D0 alphanumeric, Numbering Plan: unknown
     * TimeStamp: 05/01/15 18:52:36 GMT +01:00
     * TP-PID: 00
     * TP-DCS: F1
     * TP-DCS-desc: Class: 1 ME specific
     * Alphabet: Default (7bit)
     *
     * Toute l'équipe Orange Business Services vous présente ses meilleurs voeux pour 2015 !
     *  Plus d'infos sur http://businessl$
     * Length: 156
     */
    {
        .checkLength = true,
        .checkData = false, /* Due to special char in the string */
        .proto = PA_SMS_PROTOCOL_GSM,
        .length = 136,
        .data =
        {
            0x07, 0x91, 0x33, 0x86, 0x09, 0x40, 0x00, 0xF0, 0x04, 0x14,
            0xD0, 0x4F, 0x79, 0xD8, 0x7D, 0x2E, 0x83, 0x92, 0x6E, 0xF3,
            0x1B, 0x00, 0xF1, 0x51, 0x10, 0x50, 0x81, 0x25, 0x63, 0x40,
            0x9C, 0xD4, 0x77, 0x9D, 0x5E, 0x06, 0xB1, 0x4F, 0x85, 0x78,
            0x3D, 0x0D, 0x2F, 0x83, 0x9E, 0xF2, 0xB0, 0xFB, 0x5C, 0x06,

            0x09, 0xEB, 0xF3, 0xB4, 0xBB, 0x3C, 0x9F, 0x83, 0xA6, 0x65,
            0xB9, 0x3D, 0x3D, 0x2E, 0xCF, 0x41, 0xF6, 0x77, 0x7D, 0x0E,
            0x82, 0xCB, 0x0B, 0xF3, 0xB2, 0x9B, 0x5E, 0x06, 0xCD, 0xCB,
            0x73, 0x50, 0xBB, 0x9C, 0x66, 0xB3, 0xCB, 0x75, 0xF9, 0x1C,
            0x64, 0x7F, 0x97, 0xEB, 0x78, 0x10, 0xFC, 0x5D, 0x97, 0x83,

            0x64, 0xB0, 0x58, 0x0D, 0x14, 0x02, 0x41, 0xD9, 0xF5, 0x39,
            0x88, 0x7C, 0x4A, 0xBB, 0xCD, 0xEF, 0x39, 0x68, 0x5E, 0x97,
            0x83, 0xD0, 0x74, 0x3A, 0x5C, 0xF7, 0x7A, 0x89, 0xEB, 0xF3,
            0xB4, 0xBB, 0x3C, 0x9F, 0xB3, 0x05,
        },
        .expected =
        {
            .result = LE_OK,
            .encoding = SMSPDU_7_BITS,
            .message =
            {
                .type = PA_SMS_DELIVER,
                .smsDeliver =
                {
                    .oa = "Orange Info",
                    .format = LE_SMS_FORMAT_TEXT,
                    .scts = "15/01/05,18:52:36+04",
                    .data = "Toute l'équipe Orange Business Services vous présente ses"
                        " meilleurs voeux pour 2015 ! Plus d'infos sur http://business,(",
                        .dataLen = 156,
                },
            },
        },
    },
    /* 3 */
    /*
     * 07913386094000F00412D04276BD0C3ACACB653700F1513013017562807C4FB3595E064DE1857
     * 13ACC2E2B8661B9BB4C0791CBA0D80C749497CBEE96B95C9E2B822078584E4FCB41E4324886C3
     * 813665C5F0ED26A7E9E9B77B0E3A17DC0579985D9E83C86590BDECA697E7A0701D747CB3CD20E
     * 111340DA7DD7450B45E76D3D36EC594FA848266F0
     *
     * SMS DELIVER (receive)
     * Receipt requested: no
     * SMSC: 33689004000
     * Sender: Blue Green
     * TOA: D0 alphanumeric, Numbering Plan: unknown
     * TimeStamp: 31/03/15 10:57:26 GMT +02:00
     * TP-PID: 00
     * TP-DCS: F1
     * TP-DCS-desc: Class: 1 ME specific
     * Alphabet: Default (7bit)
     *
     * Offre Spéciale
     * Carnet de 13 Green-fees
     * A partir de 288 €
     * Conditions générales de ventes au Golf BG Saint Quentin
     * STOP 3
     * Length: 124
     * */
    {
        .checkLength = true,
        .checkData = false, /* Due to special char in the string */
        .proto = PA_SMS_PROTOCOL_GSM,
        .length = 136,
        .data =
        {
            0x07, 0x91, 0x33, 0x86, 0x09, 0x40, 0x00, 0xF0, 0x04, 0x12,
            0xD0, 0x42, 0x76, 0xBD, 0x0C, 0x3A, 0xCA, 0xCB, 0x65, 0x37,
            0x00, 0xF1, 0x51, 0x30, 0x13, 0x01, 0x75, 0x62, 0x80, 0x7C,
            0x4F, 0xB3, 0x59, 0x5E, 0x06, 0x4D, 0xE1, 0x85, 0x71, 0x3A,
            0xCC, 0x2E, 0x2B, 0x86, 0x61, 0xB9, 0xBB, 0x4C, 0x07, 0x91,
            0xCB, 0xA0, 0xD8, 0x0C, 0x74, 0x94, 0x97, 0xCB, 0xEE, 0x96,
            0xB9, 0x5C, 0x9E, 0x2B, 0x82, 0x20, 0x78, 0x58, 0x4E, 0x4F,
            0xCB, 0x41, 0xE4, 0x32, 0x48, 0x86, 0xC3, 0x81, 0x36, 0x65,
            0xC5, 0xF0, 0xED, 0x26, 0xA7, 0xE9, 0xE9, 0xB7, 0x7B, 0x0E,
            0x3A, 0x17, 0xDC, 0x05, 0x79, 0x98, 0x5D, 0x9E, 0x83, 0xC8,
            0x65, 0x90, 0xBD, 0xEC, 0xA6, 0x97, 0xE7, 0xA0, 0x70, 0x1D,
            0x74, 0x7C, 0xB3, 0xCD, 0x20, 0xE1, 0x11, 0x34, 0x0D, 0xA7,
            0xDD, 0x74, 0x50, 0xB4, 0x5E, 0x76, 0xD3, 0xD3, 0x6E, 0xC5,
            0x94, 0xFA, 0x84, 0x82, 0x66, 0xF0,
        },
        .expected =
        {
            .result = LE_OK,
            .encoding = SMSPDU_7_BITS,
            .message =
            {
                .type = PA_SMS_DELIVER,
                .smsDeliver =
                {
                    .oa = "Blue Green",
                    .format = LE_SMS_FORMAT_TEXT,
                    .scts = "15/03/31,10:57:26+08",
                    .data = "Conditions générales de ventes au Golf BG Saint QuentinSTOP 3¿@",
                    .dataLen = 124,
                },
            },
        },
    },
    /* 4 */
    /*
     * 07913386094000F0400DD04F79D87D2E830039F131705090139180A00500030D0201
     * 9EF2B0FB5CD641E56F739A5ED683C88439285C57BFEB72F2095D4F83C865103B0CA2
     * 1D41E4B07B0E8AC166207B9ACD2ECF5DC4C2F85DB7CBCB7A90FB3D07BDCD6679790E
     * 2AD341EDB738CD2ECF41B423685E97EB40683A1DAE7BBDDEF2B0DB752EBF62B072F
     *
     * SMS DELIVER (receive)
     * Receipt requested: no
     * SMSC: 33689004000
     * Sender: Orange
     * TOA: D0 alphanumeric, Numbering Plan: unknown
     * TimeStamp: 05/07/13 09:31:19 GMT +02:00
     * TP-PID: 39
     * TP-DCS: F1
     * TP-DCS-desc: Class: 1 ME specific
     * Alphabet: Default (7bit)
     * User Data Header: 05 00 03 0D 02 01
     *
     * Orange:Profitez dès aujourd'hui de la 4G dans 103 villes.Découvrez nos offres et
     *  mobiles 4G sur: http://oran.ge/10e£
     * Length: 160
     */
    {
        .checkLength = true,
        .checkData = false, /* Due to special char in the string */
        .proto = PA_SMS_PROTOCOL_GSM,
        .length = 136,
        .data =
        {
            0x07, 0x91, 0x33, 0x86, 0x09, 0x40, 0x00, 0xF0, 0x40, 0x0D,
            0xD0, 0x4F, 0x79, 0xD8, 0x7D, 0x2E, 0x83, 0x00, 0x39, 0xF1,
            0x31, 0x70, 0x50, 0x90, 0x13, 0x91, 0x80, 0xA0, 0x05, 0x00,
            0x03, 0x0D, 0x02, 0x01, 0x9E, 0xF2, 0xB0, 0xFB, 0x5C, 0xD6,
            0x41, 0xE5, 0x6F, 0x73, 0x9A, 0x5E, 0xD6, 0x83, 0xC8, 0x84,
            0x39, 0x28, 0x5C, 0x57, 0xBF, 0xEB, 0x72, 0xF2, 0x09, 0x5D,
            0x4F, 0x83, 0xC8, 0x65, 0x10, 0x3B, 0x0C, 0xA2, 0x1D, 0x41,
            0xE4, 0xB0, 0x7B, 0x0E, 0x8A, 0xC1, 0x66, 0x20, 0x7B, 0x9A,
            0xCD, 0x2E, 0xCF, 0x5D, 0xC4, 0xC2, 0xF8, 0x5D, 0xB7, 0xCB,
            0xCB, 0x7A, 0x90, 0xFB, 0x3D, 0x07, 0xBD, 0xCD, 0x66, 0x79,
            0x79, 0x0E, 0x2A, 0xD3, 0x41, 0xED, 0xB7, 0x38, 0xCD, 0x2E,
            0xCF, 0x41, 0xB4, 0x23, 0x68, 0x5E, 0x97, 0xEB, 0x40, 0x68,
            0x3A, 0x1D, 0xAE, 0x7B, 0xBD, 0xDE, 0xF2, 0xB0, 0xDB, 0x75,
            0x2E, 0xBF, 0x62, 0xB0, 0x72, 0xF0,
        },
        .expected =
        {
            .result = LE_UNSUPPORTED,
            .encoding = SMSPDU_7_BITS,
            .message =
            {
                .type = PA_SMS_DELIVER,
                .smsDeliver =
                {
                    .oa = "Orange",
                    .format = LE_SMS_FORMAT_TEXT,
                    .scts = "13/07/05,09:31:19+08",
                    .data = "Toute l'équipe Orange Business Services vous présente ses"
                        " meilleurs voeux pour 2015 ! Plus d'infos sur http://business,(",
                        .dataLen = 160,
                },
            },
        },
    },
    /* 5 */
    /*
     * 07913386094000F0440DD04F79D87D2E830039F1317050901302800F0500030D0202EAF3B13C4D2F8300
     *
     * SMS DELIVER (receive)
     * Receipt requested: no
     * SMSC: 33689004000
     * Sender: Orange
     * TOA: D0 alphanumeric, Numbering Plan: unknown
     * TimeStamp: 05/07/13 09:31:20 GMT +02:00
     * TP-PID: 39
     * TP-DCS: F1
     * TP-DCS-desc: Class: 1 ME specific
     * Alphabet: Default (7bit)
     * User Data Header: 05 00 03 0D 02 02
     *
     * uscrite
     * Length: 15
     */
    {
        .checkLength = false, /* SMS not supported */
        .checkData = false, /* SMS not supported */
        .proto = PA_SMS_PROTOCOL_GSM,
        .length = 42,
        .data =
        {
            0x07, 0x91, 0x33, 0x86, 0x09, 0x40, 0x00, 0xF0, 0x44, 0x0D,
            0xD0, 0x4F, 0x79, 0xD8, 0x7D, 0x2E, 0x83, 0x00, 0x39, 0xF1,
            0x31, 0x70, 0x50, 0x90, 0x13, 0x02, 0x80, 0x0F, 0x05, 0x00,
            0x03, 0x0D, 0x02, 0x02, 0xEA, 0xF3, 0xB1, 0x3C, 0x4D, 0x2F,
            0x83, 0x00,
        },
        .expected =
        {
            .result = LE_UNSUPPORTED,
            .encoding = SMSPDU_7_BITS,
            .message =
            {
                .type = PA_SMS_DELIVER,
                .smsDeliver =
                {
                    .oa = "Orange Info",
                    .format = LE_SMS_FORMAT_TEXT,
                    .scts = "13/07/05,09:31:20+08",
                    .data = "uscrite",
                    .dataLen = 15,
                },
            },
        },
    },
    /* 6 */
    /*
     * 07913306091093F0440B913386286620F30008515070018213807205000
     * 31304040032006F0066006A00720067007000350035006C00390065002E
     * 00200031002000770062003100360036003700310064006800310037003
     * 1003100680066003200660038003200200069006C002E00200032002000
     * 32006600660040D83DDE04D83DDE04D83D8
     *
     * SMS DELIVER (receive)
     * Receipt requested: no
     * SMSC: 33609001390
     * Sender: 33688266023
     * TOA: 91 international, Numbering Plan: unknown
     * TimeStamp: 07/05/15 10:28:31 GMT +02:00
     * TP-PID: 00
     * TP-DCS: 08
     * TP-DCS-desc: Uncompressed Text, No class
     * Alphabet: UCS2 (16bit)
     * User Data Header: 05 00 03 13 04 04
     *
     * <not decoded>
     * Length: 57
     */
    {
        .checkLength = false, /* SMS not supported */
        .checkData = false, /* SMS not supported */
        .proto = PA_SMS_PROTOCOL_GSM,
        .length = 136,
        .data =
        {
            0x07, 0x91, 0x33, 0x06, 0x09, 0x10, 0x93, 0xF0, 0x44, 0x0B,
            0x91, 0x33, 0x86, 0x28, 0x66, 0x20, 0xF3, 0x00, 0x08, 0x51,
            0x50, 0x70, 0x01, 0x82, 0x13, 0x80, 0x72, 0x05, 0x00, 0x03,
            0x13, 0x04, 0x04, 0x00, 0x32, 0x00, 0x6F, 0x00, 0x66, 0x00,
            0x6A, 0x00, 0x72, 0x00, 0x67, 0x00, 0x70, 0x00, 0x35, 0x00,
            0x35, 0x00, 0x6C, 0x00, 0x39, 0x00, 0x65, 0x00, 0x2E, 0x00,
            0x20, 0x00, 0x31, 0x00, 0x20, 0x00, 0x77, 0x00, 0x62, 0x00,
            0x31, 0x00, 0x36, 0x00, 0x36, 0x00, 0x37, 0x00, 0x31, 0x00,
            0x64, 0x00, 0x68, 0x00, 0x31, 0x00, 0x37, 0x00, 0x31, 0x00,
            0x31, 0x00, 0x68, 0x00, 0x66, 0x00, 0x32, 0x00, 0x66, 0x00,
            0x38, 0x00, 0x32, 0x00, 0x20, 0x00, 0x69, 0x00, 0x6C, 0x00,
            0x2E, 0x00, 0x20, 0x00, 0x32, 0x00, 0x20, 0x00, 0x32, 0x00,
            0x66, 0x00, 0x66, 0x00, 0x40, 0xD8, 0x3D, 0xDE, 0x04, 0xD8,
            0x3D, 0xDE, 0x04, 0xD8, 0x3D, 0x80,
        },
        .expected =
        {
            .result = LE_UNSUPPORTED,
            .encoding = SMSPDU_7_BITS,
            .message =
            {
                .type = PA_SMS_DELIVER,
                .smsDeliver =
                {
                    .oa = "33688266023",
                    .format = LE_SMS_FORMAT_TEXT,
                    .scts = "15/05/07,10:28:31+08",
                    .data = "",
                    .dataLen = 57,
                },
            },
        },
    },
    /* 7 */
    /*
     * 07913396050046F2040B913356417922F600085160802150618004004D0079
     *
     * SMS DELIVER (receive)
     * Receipt requested: no
     * SMSC: 33695000642
     * Sender: 33651497226
     * TOA: 91 international, Numbering Plan: unknown
     * TimeStamp: 08/06/15 12:05:16 GMT +02:00
     * TP-PID: 00
     * TP-DCS: 08
     * TP-DCS-desc: Uncompressed Text, No class
     * Alphabet: UCS2 (16bit)
     *
     * My
     * Length: 2
     */
    {
        .checkLength = true, /* SMS not supported */
        .checkData = true, /* SMS not supported */
        .proto = PA_SMS_PROTOCOL_GSM,
        .length = 31,
        .data =
        {
            0x07, 0x91, 0x33, 0x96, 0x05, 0x00, 0x46, 0xF2, 0x04, 0x0B,
            0x91, 0x33, 0x56, 0x41, 0x79, 0x22, 0xF6, 0x00, 0x08, 0x51,
            0x60, 0x80, 0x21, 0x50, 0x61, 0x80, 0x04, 0x00, 0x4D, 0x00,
            0x79,
        },
        .expected =
        {
            .result = LE_OK,
            .encoding = SMSPDU_UCS2_16_BITS,
            .message =
            {
                .type = PA_SMS_DELIVER,
                .smsDeliver =
                {
                    .oa = "+33651497226",
                    .format = LE_SMS_FORMAT_UCS2,
                    .scts = "15/06/08,12:05:16+08",
                    .data = { 0x00, 0x4d, 0x00, 0x79 },
                    .dataLen = 4,
                },
            },
        },
    },
    /* 8 */
    /*
     * 000100900111C576597E2EBBC7F95008442DCFE920D0B0199C8272B0
     *
     * USSD/User Data without length information
     * Alphabet: GSM 7bit
     * Serial number: 0001
     * Message identifier: 00
     * Data Coding Scheme: 01 Default Alphabet (7bit)
     * Page Parameter: 11
     *
     * Emergency!! Test CMAS 90
     * Length: 25
     */
    {
        .checkLength = true, /* SMS not supported */
        .checkData = true, /* SMS not supported */
        .proto = PA_SMS_PROTOCOL_GW_CB,
        .length = 28,
        .data =
        {
            0x00, 0x01, 0x00, 0x90, 0x01, 0x11, 0xC5, 0x76, 0x59, 0x7E,
            0x2E, 0xBB, 0xC7, 0xF9, 0x50, 0x08, 0x44, 0x2D, 0xCF, 0xE9,
            0x20, 0xD0, 0xB0, 0x19, 0x9C, 0x82, 0x72, 0xB0,
        },
        .expected =
        {
            .result = LE_OK,
            .encoding = SMSPDU_7_BITS,
            .message =
            {
                .type = PA_SMS_CELL_BROADCAST,
                .smsDeliver =
                {
                    .oa = "",
                    .format = LE_SMS_FORMAT_TEXT,
                    .scts = "",
                    .data = "Emergency!! Test  CMAS 90",
                    .dataLen = 25,
                },
            },
        },
    },
    {
        /* 9 */
        /*
         * 07910386094000F0040BA13376634753F900004170132230618006537A985E9F03
         * SMS DELIVER (receive)
         * Receipt requested: no
         * SMSC: 30689004000
         * Sender: 33673674359
         * TOA: A1 national, Numbering Plan: unknown
         * TimeStamp: 31/07/14 22:03:16 GMT +02:00
         * TP-PID: 00
         * TP-DCS: 00
         * TP-DCS-desc: Uncompressed Text, No class
         * Alphabet: Default (7bit)
         *
         * Status
         * Length: 6
         */
        .checkLength = true,
        .checkData = true,
        .proto = PA_SMS_PROTOCOL_GSM,
        .data =
        {
            0x07, 0x91, 0x03, 0x86, 0x09, 0x40, 0x00, 0xF0, 0x04, 0x0B,
            0xA1, 0x21, 0x76, 0x63, 0x47, 0x53, 0xF9, 0x00, 0x00, 0x41,
            0x70, 0x13, 0x22, 0x30, 0x61, 0x80, 0x06, 0x53, 0x7A, 0x98,
            0x5E, 0x9F, 0x03,
        },
        .expected =
        {
            .result = LE_OK,
            .encoding = SMSPDU_7_BITS,
            .message =
            {
                .type = PA_SMS_DELIVER,
                .smsDeliver =
                {
                    .oa = "12673674359",
                    .format = LE_SMS_FORMAT_TEXT,
                    .scts = "14/07/31,22:03:16+08",
                    .data = "Status",
                    .dataLen = 6,
                },
            },
        },
    },
    /* 10 */
    /*
     * 07913366003000F0040B913366921237F000086110125110
     * 934014004D00790020006D006500730073006100670065
     *
     * SMS DELIVER (receive)
     * Receipt requested: no
     * SMSC: 33660003000
     * Sender: 33662921730
     * TOA: 91 international, Numbering Plan: unknown
     * TimeStamp: 21/01/16 15:01:39 GMT +01:00
     * TP-PID: 00
     * TP-DCS: 08
     * TP-DCS-desc: Uncompressed Text, No class
     * Alphabet: UCS2 (16bit)
     *
     * My message
     * Length: 10
     */
    {
        .checkLength = true,
        .checkData = true,
        .proto = PA_SMS_PROTOCOL_GSM,
        .length = 47,
        .data =
        {
            0x07, 0x91, 0x33, 0x66, 0x00, 0x30, 0x00, 0xF0, 0x04, 0x0B,
            0x91, 0x33, 0x66, 0x92, 0x12, 0x37, 0xF0, 0x00, 0x08, 0x61,
            0x10, 0x12, 0x51, 0x10, 0x93, 0x40, 0x14, 0x00, 0x4D, 0x00,
            0x79, 0x00, 0x20, 0x00, 0x6D, 0x00, 0x65, 0x00, 0x73, 0x00,
            0x73, 0x00, 0x61, 0x00, 0x67, 0x00, 0x65,
        },
        .expected =
        {
            .result = LE_OK,
            .encoding = SMSPDU_UCS2_16_BITS,
            .message =
            {
                .type = PA_SMS_DELIVER,
                .smsDeliver =
                {
                    .oa = "+33662921730",
                    .format = LE_SMS_FORMAT_UCS2,
                    .scts = "16/01/21,15:01:39+04",
                    //"My message",
                    //004D00790020006D006500730073006100670065
                    .data =
                    {
                        0x00, 0x4D, 0x00, 0x79, 0x00, 0x20, 0x00, 0x6D, 0x00, 0x65,
                        0x00, 0x73, 0x00, 0x73, 0x00, 0x61, 0x00, 0x67, 0x00, 0x65,
                    },
                    .dataLen = 20,
                },
            },
        },
    },

    /* 11 */
    /*
     * 04 0B 91 33 67 50 92 46  F0 00 10 71 90 10 61 14 54 80 05 E8 32 9B FD 06
     *
     * PDU LENGTH IS 24 BYTES
     * NO SMSC ADDRESS PRESENT
     * MESSAGE HEADER FLAGS
     * MESSAGE TYPE :  SMS DELIVER
     * MSGS WAITING IN SC :    NO
     * LOOP PREVENTION :   NO
     * SEND STATUS REPORT :    NO
     * USER DATA HEADER :  NO UDH
     * REPLY PATH :    NO
     *
     * ORIGINATING ADDRESS
     * NUMBER IS : +33760529640
     * TYPE OF NR. :   International
     * NPI :   ISDN/Telephone (E.164/163)
     *
     * PROTOCOL IDENTIFIER (0x00)
     * MESSAGE ENTITIES :  SME-to-SME
     * PROTOCOL USED : Implicit / SC-specific
     *
     * DATA CODING SCHEME  (0x10)
     * AUTO-DELETION : OFF
     * COMPRESSION :   OFF
     * MESSAGE CLASS : 0 (imm. display)
     * ALPHABET USED : 7bit default
     *
     * SMSC TIMESTAMP :    01/09/17 16:41 GMT+2,00
     * USER DATA PART OF SM
     * USER DATA LENGTH :  5 septets
     * USER DATA (TEXT) :  hello
     */
    {
        .checkLength = true,
        .checkData = true,
        .proto = PA_SMS_PROTOCOL_GSM,
        .length = 24,
        .data =
        {
            0x04, 0x0B, 0x91, 0x33, 0x67, 0x50, 0x92, 0x46, 0xF0, 0x00,
            0x10, 0x71, 0x90, 0x10, 0x61, 0x14, 0x54, 0x80, 0x05, 0xE8,
            0x32, 0x9B, 0xFD, 0x06,
        },
        .expected =
        {
            .result = LE_UNSUPPORTED,
            .encoding = SMSPDU_7_BITS,
            .message =
            {
                .type = PA_SMS_DELIVER,
                .smsDeliver =
                {
                    .oa = "+33760529640",
                    .format = LE_SMS_FORMAT_TEXT,
                    .scts = "01/09/17,16:41:+04",
                    .data = "hello",
                    .dataLen = 5,
                },
            },
        },
    },

    /* 12 */
    /*
     * 04 0B 91 33 67 50 92 46 F0 00 C0 71 90 40 11 93 63 80 00
     *
     * PDU LENGTH IS 16 BYTES
     * NO SMSC ADDRESS PRESENT
     * MESSAGE HEADER FLAGS
     * MESSAGE TYPE :  SMS DELIVER
     * MSGS WAITING IN SC :    NO
     * LOOP PREVENTION :   NO
     * SEND STATUS REPORT :    NO
     * USER DATA HEADER :  NO UDH
     * REPLY PATH :    NO
     *
     * ORIGINATING ADDRESS
     * NUMBER IS : +33760529640
     * TYPE OF NR. :   International
     * NPI :   ISDN/Telephone (E.164/163)
     *
     * PROTOCOL IDENTIFIER (0x00)
     * MESSAGE ENTITIES :  SME-to-SME
     * PROTOCOL USED : Implicit / SC-specific
     *
     * DATA CODING SCHEME  (0xC0)
     * STORE MESSAGE : MAY
     * MESSAGE INDICATOR : DEACTIVATE
     * INDICATOR TYPE :    VOICEMAIL waiting
     * ALPHABET USED : 7bit default
     *
     * SMSC TIMESTAMP :    04/09/17 11:39 GMT
     * USER DATA PART OF SM
     * USER DATA LENGTH :  0 septets
     * USER DATA (TEXT) :  <no data>
     */
    {
        .checkLength = true,
        .checkData = true,
        .proto = PA_SMS_PROTOCOL_UNKNOWN,
        .length = 16,
        .data =
        {
            0x04, 0x0B, 0x91, 0x33, 0x67, 0x50, 0x92, 0x46, 0xF0, 0x00,
            0xC0, 0x71, 0x90, 0x40, 0x11, 0x93, 0x63, 0x80, 0x00,
        },
        .expected =
        {
            .result = LE_UNSUPPORTED,
            .encoding = SMSPDU_7_BITS,
            .message =
            {
                .type = PA_SMS_DELIVER,
                .smsDeliver =
                {
                    .oa = "+33760529640",
                    .format = LE_SMS_FORMAT_PDU,
                    .scts = "04/09/17,11:39:+04",
                    .data = "",
                    .dataLen = 0,
                },
            },
        },
    },

    /* 13 */
    /*
     * 04 0B 91 33 67 50 92 46 F0 00 C8 71 90 40 11 64
     *
     * PDU LENGTH IS 16 BYTES
     * NO SMSC ADDRESS PRESENT
     * MESSAGE HEADER FLAGS
     * MESSAGE TYPE :  SMS DELIVER
     * MSGS WAITING IN SC :    NO
     * LOOP PREVENTION :   NO
     * SEND STATUS REPORT :    NO
     * USER DATA HEADER :  NO UDH
     * REPLY PATH :    NO
     *
     * ORIGINATING ADDRESS
     * NUMBER IS : +33760529640
     * TYPE OF NR. :   International
     * NPI :   ISDN/Telephone (E.164/163)
     *
     * PROTOCOL IDENTIFIER (0x00)
     * MESSAGE ENTITIES :  SME-to-SME
     * PROTOCOL USED : Implicit / SC-specific
     *
     * DATA CODING SCHEME  (0xC8)
     * STORE MESSAGE : MAY
     * MESSAGE INDICATOR : ACTIVATE
     * INDICATOR TYPE :    VOICEMAIL waiting
     * ALPHABET USED : 7bit default
     *
     * SMSC TIMESTAMP :    04/09/17 11:46 GMT
     * USER DATA PART OF SM
     * USER DATA LENGTH :  0 septets
     * USER DATA (TEXT) :  <no data>
     */
    {
        .checkLength = true,
        .checkData = true,
        .proto = PA_SMS_PROTOCOL_UNKNOWN,
        .length = 16,
        .data =
        {
            0x04, 0x0B, 0x91, 0x33, 0x67, 0x50, 0x92, 0x46, 0xF0, 0x00,
            0xC8, 0x71, 0x90, 0x40, 0x11, 0x64,
        },
        .expected =
        {
            .result = LE_UNSUPPORTED,
            .encoding = SMSPDU_7_BITS,
            .message =
            {
                .type = PA_SMS_DELIVER,
                .smsDeliver =
                {
                    .oa = "+33760529640",
                    .format = LE_SMS_FORMAT_PDU,
                    .scts = "04/09/17,11:46",
                    .data = "",
                    .dataLen = 0,
                },
            },
        },
    },
};

static le_result_t TestDecodePdu
(
    void
)
{
    pa_sms_Message_t message;
    le_result_t res;
    int i;

    for ( i = 0; i < sizeof(PduReceivedDb)/sizeof(PduReceived_t); i++)
    {
        const PduReceived_t * receivedPtr = &PduReceivedDb[i];

        LE_INFO("=> Index %d", i);

        res = smsPdu_Decode(receivedPtr->proto,
                            receivedPtr->data,
                            receivedPtr->length,
                            true,
                            &message);
        if (res != receivedPtr->expected.result)
        {
            LE_ERROR("smsPdu_Decode() returns %d", res);
            return LE_FAULT;
        }

        if (res == LE_OK)
        {
            if (message.type != receivedPtr->expected.message.type)
            {
                LE_ERROR("type %d , expected %d", message.type, receivedPtr->expected.message.type);
                return LE_FAULT;
            }

            switch(message.type)
            {
                case PA_SMS_DELIVER:
                {
                    LE_INFO("Format: %u", message.smsDeliver.format);
                    LE_INFO("Data (%u): %s", message.smsDeliver.dataLen, message.smsDeliver.data);
                    if(message.smsDeliver.format != receivedPtr->expected.message.smsDeliver.format)
                    {
                        LE_ERROR("format %d != %d",
                                        message.smsDeliver.format,
                                        receivedPtr->expected.message.smsDeliver.format);
                        return LE_FAULT;
                    }

                    if(strcmp(message.smsDeliver.oa,
                                    receivedPtr->expected.message.smsDeliver.oa) != 0)
                    {
                        LE_ERROR(" oa %s != %s",
                                        message.smsDeliver.oa,
                                        receivedPtr->expected.message.smsDeliver.oa);
                        return LE_FAULT;
                    }

                    if(strcmp(message.smsDeliver.scts,
                                    receivedPtr->expected.message.smsDeliver.scts) != 0)
                    {
                        LE_ERROR("scts %s != %s", message.smsDeliver.scts,
                                        receivedPtr->expected.message.smsDeliver.scts);
                        return LE_FAULT;
                    }

                    if(message.smsDeliver.dataLen !=
                                    receivedPtr->expected.message.smsDeliver.dataLen)
                    {
                        LE_ERROR("dataLen %d != %d",
                                        message.smsDeliver.dataLen,
                                        receivedPtr->expected.message.smsDeliver.dataLen);
                        if (receivedPtr->checkData)
                        {
                            return LE_FAULT;
                        }
                    }

                    if(memcmp(message.smsDeliver.data,
                                    receivedPtr->expected.message.smsDeliver.data,
                                    message.smsDeliver.dataLen) != 0)
                    {
                        LE_ERROR("Data doesn't match (%d)", message.smsDeliver.dataLen);
                        if(receivedPtr->checkData)
                        {
                            DumpPdu("Pdu decoded:",message.smsDeliver.data, message.smsDeliver.dataLen);
                            DumpPdu("Pdu ref:",receivedPtr->expected.message.smsDeliver.data,
                                message.smsDeliver.dataLen);
                            return LE_FAULT;
                        }
                    }
                }
                break;

                case PA_SMS_SUBMIT:
                {
                    LE_ERROR("Unexpected submit");
                    return LE_FAULT;
                }

                case PA_SMS_CELL_BROADCAST:
                {
                    LE_INFO("Format: %u", message.cellBroadcast.format);
                    if(message.cellBroadcast.dataLen !=
                                    receivedPtr->expected.message.smsDeliver.dataLen)
                    {
                        LE_ERROR("dataLen %d != %d",
                                        message.cellBroadcast.dataLen,
                                        receivedPtr->expected.message.smsDeliver.dataLen);
                        if (receivedPtr->checkData)
                        {
                            return LE_FAULT;
                        }
                    }

                    if(memcmp(message.cellBroadcast.data,
                                    receivedPtr->expected.message.smsDeliver.data,
                                    message.cellBroadcast.dataLen) != 0)
                    {
                        LE_ERROR("Data doesn't match (%d)", message.cellBroadcast.dataLen);
                        if(receivedPtr->checkData)
                        {
                            DumpPdu("Pdu decoded:",message.cellBroadcast.data, message.cellBroadcast.dataLen);
                            DumpPdu("Pdu ref:",receivedPtr->expected.message.smsDeliver.data,
                                receivedPtr->expected.message.smsDeliver.dataLen);
                            return LE_FAULT;
                        }
                    }
                }
                break;

                default:
                {
                    LE_ERROR("Unexpected type");
                    return LE_FAULT;
                }
            }
        }
    }
    return LE_OK;
}

static le_result_t TestEncodePdu
(
    void
)
{
    pa_sms_Pdu_t pdu;
    pa_sms_Message_t message;
    le_result_t res;
    smsPdu_DataToEncode_t data;
    int i;

    for (i = 0; i < sizeof(PduAssocDb)/sizeof(PduAssoc_t); i++)
    {
        const PduAssoc_t * assoc = &PduAssocDb[i];
        size_t messageLength = strlen(assoc->text);

        LE_INFO("=> Index %d", i);
        LE_INFO("Text (%zd): (%s)", messageLength, assoc->text);

        // Enable or disable SMS Status Report
        if (assoc->statusReportEnabled)
        {
            le_sms_EnableStatusReport();
        }
        else
        {
            le_sms_DisableStatusReport();
        }

        /* GSM Encode 8 bits */
        LE_INFO("Encoding in 8 bits GSM");

        memset(&data, 0, sizeof(data));
        data.protocol = PA_SMS_PROTOCOL_GSM;
        data.messagePtr = (const uint8_t*)assoc->text;
        data.length = strlen(assoc->text);
        data.addressPtr = assoc->dest;
        data.encoding = SMSPDU_8_BITS;
        data.messageType = assoc->type;
        le_sms_IsStatusReportEnabled(&data.statusReport);

        res = smsPdu_Encode(&data, &pdu);
        if (res != assoc->gsm_8bits.conversionResult)
        {
            return LE_FAULT;
        }

        if ( res == LE_OK )
        {
            LE_INFO("Source: (%d)", (int) assoc->gsm_8bits.length);
            LE_INFO("Encoded: (%d)", pdu.dataLen);

            /* Check */
            if (pdu.dataLen != assoc->gsm_8bits.length)
            {
                return LE_FAULT;
            }

            if (memcmp(pdu.data, assoc->gsm_8bits.data, pdu.dataLen) != 0)
            {
                DumpPdu("Pdu ref:",assoc->gsm_8bits.data, assoc->gsm_8bits.length);
                DumpPdu("Pdu encoded:",pdu.data, pdu.dataLen);
                return LE_FAULT;
            }

            /* Decode */
            res = smsPdu_Decode(PA_SMS_PROTOCOL_GSM, pdu.data, pdu.dataLen, true, &message);
            if (res != LE_OK)
            {
                return LE_FAULT;
            }
            if (message.type != assoc->type)
            {
                return LE_FAULT;
            }

            switch(message.type)
            {
                case PA_SMS_DELIVER:
                {
                    if (message.smsDeliver.format != LE_SMS_FORMAT_BINARY)
                    {
                        return LE_FAULT;
                    }

                    if (memcmp(message.smsDeliver.data,assoc->text,strlen(assoc->text)) != 0)
                    {
                        return LE_FAULT;
                    }
                }
                break;

                case PA_SMS_SUBMIT:
                {
                    LE_INFO("Data (%d): '%s'", message.smsSubmit.dataLen, message.smsSubmit.data);
                    if (message.smsSubmit.format != LE_SMS_FORMAT_BINARY)
                    {
                        return LE_FAULT;
                    }
                    if (memcmp(message.smsSubmit.data,assoc->text,strlen(assoc->text)) != 0)
                    {
                        return LE_FAULT;
                    }
                }
                break;

                default:
                {
                    LE_ERROR("Unexpected type");
                    return LE_FAULT;
                }
            }
        }
        LE_INFO("------------------");

        /* GSM Encode 7 bits */
        LE_INFO("Encoding in 7 bits GSM ");
        data.encoding = SMSPDU_7_BITS;

        res = smsPdu_Encode(&data, &pdu);
        if (res != assoc->gsm_7bits.conversionResult)
        {
            return LE_FAULT;
        }

        if ( res == LE_OK )
        {
            LE_INFO("Source: (%zu)", assoc->gsm_7bits.length);
            LE_INFO("Encoded: (%u)", pdu.dataLen);

            /* Check */
            if (pdu.dataLen != assoc->gsm_7bits.length )
            {
                return LE_FAULT;
            }

            if (memcmp(pdu.data, assoc->gsm_7bits.data, pdu.dataLen) != 0 )
            {
                DumpPdu("Pdu ref:",assoc->gsm_7bits.data, assoc->gsm_7bits.length);
                DumpPdu("Pdu encoded:",pdu.data, pdu.dataLen);
                return LE_FAULT;
            }

            /* Decode */
            res = smsPdu_Decode(PA_SMS_PROTOCOL_GSM, pdu.data, pdu.dataLen, true, &message);
            if (res != LE_OK)
            {
                return LE_FAULT;
            }

            if (message.type != assoc->type)
            {
                return LE_FAULT;
            }
            LE_INFO("Type: %u", message.type);

            switch(message.type)
            {
                case PA_SMS_DELIVER:
                {
                    LE_INFO("Format: %u", message.smsDeliver.format);
                    LE_INFO("Data (%u): '%s'", message.smsDeliver.dataLen, message.smsDeliver.data);
                    if (message.smsDeliver.format != LE_SMS_FORMAT_TEXT)
                    {
                        return LE_FAULT;
                    }
                    if (memcmp(message.smsDeliver.data,assoc->text,strlen(assoc->text)) != 0)
                    {
                        return LE_FAULT;
                    }
                }
                break;

                case PA_SMS_SUBMIT:
                {
                    LE_INFO("Format: %u", message.smsSubmit.format);
                    LE_INFO("Data (%u): '%s'", message.smsSubmit.dataLen, message.smsSubmit.data);
                    if (message.smsSubmit.format != LE_SMS_FORMAT_TEXT)
                    {
                        return LE_FAULT;
                    }
                    if (memcmp(message.smsSubmit.data,assoc->text,strlen(assoc->text)) != 0)
                    {
                        return LE_FAULT;
                    }
                }
                break;

                default:
                {
                    LE_ERROR("Unexpected type");
                    return LE_FAULT;
                }
            }
        }

        if (assoc->gsm_ucs2.length)
        {
            LE_INFO("------------------");

            /* GSM Encode UCS2 bits */
            uint8_t ucs2stringconverted[LE_SMS_UCS2_MAX_BYTES] = { 0 };
            int len = strlen(assoc->text);
            int j = 0;

            if (len > LE_SMS_UCS2_MAX_CHARS)
            {
                return LE_FAULT;
            }
            for (j = 0; j < len; j++)
            {
                ucs2stringconverted[j*2] = 0x00;
                ucs2stringconverted[j*2+1] = assoc->text[j];
            }
            len = len *2;

            LE_INFO("Encoding in UCS2 GSM");
            data.messagePtr = (const uint8_t*) ucs2stringconverted;
            data.length = len;
            data.encoding = SMSPDU_UCS2_16_BITS;

            res = smsPdu_Encode(&data, &pdu);
            if (res != assoc->gsm_ucs2.conversionResult)
            {
                return LE_FAULT;
            }

            if ( res == LE_OK )
            {
                LE_INFO("Source: (%zu)", assoc->gsm_ucs2.length);
                LE_INFO("Encoded: (%u)", pdu.dataLen);

                /* Check */
                if (pdu.dataLen != assoc->gsm_ucs2.length )
                {
                    return LE_FAULT;
                }

                if (memcmp(pdu.data, assoc->gsm_ucs2.data, pdu.dataLen) != 0 )
                {
                    DumpPdu("Pdu ref:",assoc->gsm_ucs2.data, assoc->gsm_ucs2.length);
                    DumpPdu("Pdu encoded:",pdu.data, pdu.dataLen);
                    return LE_FAULT;
                }

                /* Decode */
                res = smsPdu_Decode(PA_SMS_PROTOCOL_GSM, pdu.data, pdu.dataLen, true, &message);
                if (res != LE_OK)
                {
                    return LE_FAULT;
                }

                if (message.type != assoc->type)
                {
                    return LE_FAULT;
                }
                LE_INFO("Type: %u", message.type);

                switch(message.type)
                {
                    case PA_SMS_DELIVER:
                    {
                        LE_INFO("Format: %u", message.smsDeliver.format);
                        LE_INFO("Data (%u): '%s'", message.smsDeliver.dataLen, message.smsDeliver.data);
                        if (message.smsDeliver.format != LE_SMS_FORMAT_UCS2)
                        {
                            return LE_FAULT;
                        }
                        if (memcmp(message.smsDeliver.data,assoc->text,strlen(assoc->text)) != 0)
                        {
                            return LE_FAULT;
                        }
                    }
                    break;

                    case PA_SMS_SUBMIT:
                    {
                        LE_INFO("Format: %u", message.smsSubmit.format);
                        LE_INFO("Data (%u): '%s'", message.smsSubmit.dataLen, message.smsSubmit.data);
                        if (message.smsSubmit.format != LE_SMS_FORMAT_UCS2)
                        {
                            return LE_FAULT;
                        }
                        if (memcmp(message.smsSubmit.data,ucs2stringconverted,len) != 0)
                        {
                            return LE_FAULT;
                        }
                    }
                    break;

                    default:
                    {
                        LE_ERROR("Unexpected type");
                        return LE_FAULT;
                    }
                }
            }
        }

        LE_INFO("------------------");

        /* CDMA Encode 8 bits */
        LE_INFO("Encoding in 8 bits CDMA");
        data.protocol = PA_SMS_PROTOCOL_CDMA;
        data.messagePtr = (const uint8_t*)assoc->text;
        data.length = strlen(assoc->text);
        data.encoding = SMSPDU_8_BITS;

        res = smsPdu_Encode(&data, &pdu);
        if (res != assoc->cdma_8bits.conversionResult)
        {
            return LE_FAULT;
        }

        if ( res == LE_OK )
        {
            uint32_t timestampSize = 8;
            uint32_t indexAfterTimestamp = assoc->cdma_8bits.timestampIndex+timestampSize;

            LE_INFO("Source: (%zu)", assoc->cdma_8bits.length);
            LE_INFO("Encoded: (%u)", pdu.dataLen);

            /* Check, exclude timestamp*/
            if (pdu.dataLen != assoc->cdma_8bits.length)
            {
                return LE_FAULT;
            }

            if ( memcmp(pdu.data, assoc->cdma_8bits.data, assoc->cdma_8bits.timestampIndex+1) != 0)
            {
                DumpPdu("Pdu ref:",assoc->cdma_8bits.data, assoc->cdma_8bits.length);
                DumpPdu("Pdu Encoded:",pdu.data, pdu.dataLen);
                return LE_FAULT;
            }

            if (memcmp(&pdu.data[indexAfterTimestamp],
                            &assoc->cdma_8bits.data[indexAfterTimestamp],
                            assoc->cdma_8bits.length-indexAfterTimestamp) != 0)
            {
                return LE_FAULT;
            }

            /* Decode */
            res = smsPdu_Decode(PA_SMS_PROTOCOL_CDMA, pdu.data, pdu.dataLen, true, &message);
            if (res != LE_OK )
            {
                return LE_FAULT;
            }
            if (message.type != assoc->type)
            {
                return LE_FAULT;
            }

            switch(message.type)
            {
                case PA_SMS_DELIVER:
                {
                    if (message.smsDeliver.format != LE_SMS_FORMAT_BINARY  )
                    {
                        return LE_FAULT;
                    }
                    if (memcmp(message.smsDeliver.data,assoc->text,strlen(assoc->text)) != 0)
                    {
                        return LE_FAULT;
                    }
                }
                break;

                case PA_SMS_SUBMIT:
                {
                    LE_INFO("Data (%u): '%s'", message.smsSubmit.dataLen, message.smsSubmit.data);
                    if (message.smsSubmit.format != LE_SMS_FORMAT_BINARY)
                    {
                        return LE_FAULT;
                    }
                    if (memcmp(message.smsSubmit.data,assoc->text,strlen(assoc->text)) != 0)
                    {
                        return LE_FAULT;
                    }
                }
                break;

                default:
                {
                    LE_ERROR("Unexpected type");
                    return LE_FAULT;
                }
            }
        }

        LE_INFO("------------------");

        /* CDMA Encode 7 bits */
        LE_INFO("Encoding in 7 bits CDMA ");
        data.encoding = SMSPDU_7_BITS;

        res = smsPdu_Encode(&data, &pdu);
        if (res != assoc->cdma_7bits.conversionResult)
        {
            return LE_FAULT;
        }

        if ( res == LE_OK )
        {
            uint32_t timestampSize = 8;
            uint32_t indexAfterTimestamp = assoc->cdma_7bits.timestampIndex+timestampSize;

            LE_INFO("Source: (%zu)", assoc->cdma_7bits.length);
            LE_INFO("Encoded: (%u)", pdu.dataLen);

            /* Check, exclude timestamp*/
            if (pdu.dataLen != assoc->cdma_7bits.length)
            {
                return LE_FAULT;
            }

            if (memcmp(pdu.data, assoc->cdma_7bits.data,assoc->cdma_7bits.timestampIndex+1) != 0)
            {
                DumpPdu("Pdu Source:",assoc->cdma_7bits.data, assoc->cdma_7bits.length);
                DumpPdu("Pdu Encoded:",pdu.data, pdu.dataLen);
                return LE_FAULT;
            }

            if (memcmp(&pdu.data[indexAfterTimestamp],
                            &assoc->cdma_7bits.data[indexAfterTimestamp],
                            assoc->cdma_7bits.length-indexAfterTimestamp) != 0)
            {
                return LE_FAULT;
            }

            /* Decode */
            res = smsPdu_Decode(PA_SMS_PROTOCOL_CDMA, pdu.data, pdu.dataLen, true, &message);
            if (res != LE_OK)
            {
                return LE_FAULT;
            }
            if (message.type != assoc->type)
            {
                return LE_FAULT;
            }

            switch(message.type)
            {
                case PA_SMS_DELIVER:
                {
                    LE_INFO("Format: %u", message.smsDeliver.format);
                    LE_INFO("Data (%u): '%s'", message.smsDeliver.dataLen, message.smsDeliver.data);
                    if (message.smsDeliver.format != LE_SMS_FORMAT_TEXT)
                    {
                        return LE_FAULT;
                    }
                    if (memcmp(message.smsDeliver.data,assoc->text,strlen(assoc->text)) != 0)
                    {
                        return LE_FAULT;
                    }
                }
                break;

                case PA_SMS_SUBMIT:
                {
                    LE_INFO("Format: %u", message.smsSubmit.format);
                    LE_INFO("Data (%u): '%s'", message.smsSubmit.dataLen, message.smsSubmit.data);
                    if (message.smsSubmit.format != LE_SMS_FORMAT_TEXT)
                    {
                        return LE_FAULT;
                    }
                    if (memcmp(message.smsSubmit.data,assoc->text,strlen(assoc->text)) != 0)
                    {
                        return LE_FAULT;
                    }
                }
                break;

                default:
                {
                    LE_ERROR("Unexpected type");
                    return LE_FAULT;
                }
            }
        }
        LE_INFO("------------------");
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/*
 * SMS PDU encoding and decoding test
 *
 */
//--------------------------------------------------------------------------------------------------
void testle_sms_SmsPduTest
(
    void
)
{
    LE_INFO("Test EncodePdu started");
    LE_ASSERT_OK(TestEncodePdu());

    LE_INFO("Test DecodePdu started");
    LE_ASSERT_OK(TestDecodePdu());

    LE_INFO("smsPduTest SUCCESS");
}
