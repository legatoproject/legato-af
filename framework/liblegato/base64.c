//--------------------------------------------------------------------------------------------------
/** @file base64.c
 *
 * This module contains functions perform base64 encoding/decoding.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Perform base64 data encoding.
 *
 * @return
 *      - LE_OK if succeeds
 *      - LE_BAD_PARAMETER if NULL pointer provided
 *      - LE_OVERFLOW if provided buffer is not large enough
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_base64_Encode
(

    const uint8_t *srcPtr,  ///< [IN] Data to be encoded
    size_t srcLen,          ///< [IN] Data length
    char *dstPtr,           ///< [OUT] Base64-encoded string buffer
    size_t *dstLenPtr       ///< [INOUT] Length of the base64-encoded string buffer
)
{
    const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const uint8_t *data = (const uint8_t *) srcPtr;
    size_t resultIndex = 0;
    size_t x;
    uint32_t n = 0;
    int padCount = srcLen % 3;
    uint8_t n0, n1, n2, n3;
    size_t resultSize;

    if ((NULL == dstLenPtr) || (NULL == dstPtr) || (NULL == srcPtr))
    {
        return LE_BAD_PARAMETER;
    }
    resultSize = *dstLenPtr;

    /* increment over the length of the string, three characters at a time */
    for (x = 0; x < srcLen; x += 3)
    {
        /* these three 8-bit (ASCII) characters become one 24-bit number */
        n = ((uint32_t) data[x]) << 16;
        if ((x + 1) < srcLen)
        {
            n += ((uint32_t)data[x + 1]) << 8;
        }

        if ((x + 2) < srcLen)
        {
            n += data[x + 2];
        }

        /* this 24-bit number gets separated into four 6-bit numbers */
        n0 = (uint8_t)(n >> 18) & 63;
        n1 = (uint8_t)(n >> 12) & 63;
        n2 = (uint8_t)(n >> 6) & 63;
        n3 = (uint8_t)n & 63;

        /*
         * if we have one byte available, then its encoding is spread
         * out over two characters
         */
        if(resultIndex >= resultSize)
        {
            return LE_OVERFLOW;
        }
        dstPtr[resultIndex++] = base64Chars[n0];
        if(resultIndex >= resultSize)
        {
            return LE_OVERFLOW;
        }
        dstPtr[resultIndex++] = base64Chars[n1];

        /*
         * if we have only two bytes available, then their encoding is
         * spread out over three chars
         */
        if ((x + 1) < srcLen)
        {
            if(resultIndex >= resultSize)
            {
                return LE_OVERFLOW;
            }
            dstPtr[resultIndex++] = base64Chars[n2];
        }

        /*
         * if we have all three bytes available, then their encoding is spread
         * out over four characters
         */
        if ((x + 2) < srcLen)
        {
            if(resultIndex >= resultSize)
            {
                return LE_OVERFLOW;
            }
            dstPtr[resultIndex++] = base64Chars[n3];
        }
    }

    /*
     * create and add padding that is required if we did not have a multiple of 3
     * number of characters available
     */
    if (padCount > 0)
    {
        for (; padCount < 3; padCount++)
        {
            if(resultIndex >= resultSize)
            {
                return LE_OVERFLOW;
            }
            dstPtr[resultIndex++] = '=';
        }
    }
    if (resultIndex >= resultSize)
    {
        return LE_OVERFLOW;   /* indicate failure: buffer too small */
    }
    dstPtr[resultIndex] = 0;

    // also counting the terminating 0
    *dstLenPtr = resultIndex + 1;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Base64 decode table
 */
//--------------------------------------------------------------------------------------------------
static const unsigned char DecodeTable[] = {
    66,66,66,66,66,66,66,66,66,66,64,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,62,66,66,66,63,52,53,
    54,55,56,57,58,59,60,61,66,66,66,65,66,66,66, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,66,66,66,66,66,66,26,27,28,
    29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,
    66,66,66,66,66,66
};

//--------------------------------------------------------------------------------------------------
/**
 * Decode table definitions for characters requiring special treatment.
 */
//--------------------------------------------------------------------------------------------------
#define WHITESPACE 64
#define EQUALS     65
#define INVALID    66

//--------------------------------------------------------------------------------------------------
/**
 * Decode base64-encoded data.
 *
 * @return
 *      - LE_OK if succeeds
 *      - LE_BAD_PARAMETER if NULL pointer provided
 *      - LE_FORMAT_ERROR if data contains invalid (non-base64) characters
 *      - LE_OVERFLOW if provided buffer is not large enough
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_base64_Decode
(
    const char *srcPtr, ///< [IN] Encoded string
    size_t srcLen,      ///< [IN] Encoded string length
    uint8_t *dstPtr,    ///< [OUT] Binary data buffer
    size_t *dstLenPtr   ///< [INOUT] Binary data buffer size / decoded data size
)
{
    const char *in = srcPtr;
    const char *end = in + srcLen;
    char iter = 0;
    uint32_t buf = 0;
    size_t len = 0;
    uint8_t *out = dstPtr;
    size_t outLen;

    if ((NULL == srcPtr) || (NULL == dstPtr) || (NULL == dstLenPtr))
    {
        return LE_BAD_PARAMETER;
    }

    outLen = *dstLenPtr;

    while (in < end)
    {
        unsigned char c = DecodeTable[(int)(*in++)];

        switch (c)
        {
            case WHITESPACE:
                continue;   /* skip whitespace */
            case INVALID:
                return LE_FORMAT_ERROR;  /* invalid input, return error */
            case EQUALS:                 /* pad character, end of data */
                in = end;
                continue;
            default:
                buf = buf << 6 | c;
                iter++; // increment the number of iteration
                /* If the buffer is full, split it into bytes */
                if (iter == 4)
                {
                    if ((len += 3) > outLen)
                    {
                        return LE_OVERFLOW; /* buffer overflow */
                    }
                    *(out++) = (buf >> 16) & 255;
                    *(out++) = (buf >> 8) & 255;
                    *(out++) = buf & 255;
                    buf = 0;
                    iter = 0;
                }
        }
    }

    if (iter == 3)
    {
        if ((len += 2) > outLen)
        {
            return LE_OVERFLOW;
        }
        *(out++) = (buf >> 10) & 255;
        *(out++) = (buf >> 2) & 255;
    }
    else if (iter == 2)
    {
        if (++len > outLen)
        {
            return LE_OVERFLOW;
        }
        *(out++) = (buf >> 4) & 255;
    }

    *dstLenPtr = len; /* modify to reflect the actual output size */

    return LE_OK;
}
