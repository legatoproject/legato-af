 /**
  * This module is for unit testing the le_utf8 module in the legato
  * runtime library (liblegato.so).
  *
  * The following is a list of the test cases:
  *
  *  - Getting the number of bytes and characters for:
  *         - an ASCII string
  *         - multi-byte UTF-8 String
  *         - an improper string with a missing lead byte,
  *         - an improper string with a missing continuation byte,
  *         - an improper string with an invalid byte.
  *
  *  - Copy and appending:
  *         - ASCII strings
  *         - multi-byte strings,
  *         - ASCII strings that truncates,
  *         - multi-byte strings that truncates and aligns to the end of destination buffer,
  *         - multi-byte strings that truncates and aligns with the end of the destination buffer -1,
  *         - multi-byte strings that truncates and the last character straddles the end of the destination buffer.
  *         - an improper string with a missing lead byte,
  *         - an improper string with a missing continuation byte,
  *         - an improper string with an invalid byte.
  *
  *  - Check the formatting of:
  *         - an ASCII string,
  *         - a multi-byte string,
  *         - an improper string with a missing lead byte,
  *         - an improper string with a missing continuation byte,
  *         - an improper string with an invalid byte.
  *
  *  - Parsing of integers from strings.
  *
  *  - Encoding/decoding of unicode code points into/from utf-8 data
  *
  * Copyright (C) Sierra Wireless Inc.
  */

#include "legato.h"
#include <stdio.h>

#define TWO_CHAR_BYTE   0xC0
#define THREE_CHAR_BYTE 0xE0
#define FOUR_CHAR_BYTE  0xF0
#define CONT_BYTE       0x80
#define INVALID_BYTE    0xA0


static bool compareConCat(char* finalStr, char* firstStr, char* secondStr, size_t numBytesToCheck);


static void TestIntParsing(void)
{
    char s[100];
    int value = 12; // arbitrary non-zero value.

    LE_ASSERT(LE_FORMAT_ERROR == le_utf8_ParseInt(&value, "foo"));
    LE_ASSERT(LE_FORMAT_ERROR == le_utf8_ParseInt(&value, "4foo"));
    LE_ASSERT(LE_FORMAT_ERROR == le_utf8_ParseInt(&value, "1237^78"));

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "0"));
    LE_ASSERT(value == 0);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "00"));
    LE_ASSERT(value == 0);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "0x0"));
    LE_ASSERT(value == 0);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "0x0000"));
    LE_ASSERT(value == 0);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "-0x0000"));
    LE_ASSERT(value == 0);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, " 0"));
    LE_ASSERT(value == 0);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "                 0"));
    LE_ASSERT(value == 0);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "1"));
    LE_ASSERT(value == 1);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "-1"));
    LE_ASSERT(value == -1);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "0x1"));
    LE_ASSERT(value == 1);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "-0x1"));
    LE_ASSERT(value == -1);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "01"));
    LE_ASSERT(value == 1);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "-01"));
    LE_ASSERT(value == -1);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "0x7FFFFFFF"));
    LE_ASSERT(value == 0x7FFFFFFF);

    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, "-0x80000000"));
    LE_ASSERT(value == -0x80000000);

    sprintf(s, "%d", INT_MAX);
    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, s));
    LE_ASSERT(value == INT_MAX);

    sprintf(s, "%d", INT_MIN);
    LE_ASSERT(LE_OK == le_utf8_ParseInt(&value, s));
    LE_ASSERT(value == INT_MIN);

    sprintf(s, "%lld", ((long long int)(INT_MAX)) + 1);
    LE_ASSERT(LE_OUT_OF_RANGE == le_utf8_ParseInt(&value, s));

    sprintf(s, "%lld", ((long long int)(INT_MIN)) - 1);
    LE_ASSERT(LE_OUT_OF_RANGE == le_utf8_ParseInt(&value, s));
}

static void TestEncodeDecodeCodePoint(void)
{
    uint32_t codePoint;
    uint32_t decodedCodePoint;
    char out[4];
    size_t outSize;

    // The utf-8 encoding of the lowest and highest values that fit in 1, 2, 3 and 4 bytes
    const char lowOneEncoding[]       = {0x00};
    const char highOneEncoding[]      = {0x7F};
    const char lowTwoEncoding[]       = {0xC2, 0x80};
    const char highTwoEncoding[]      = {0xDF, 0xBF};
    const char lowThreeEncoding[]     = {0xE0, 0xA0, 0x80};
    const char highThreeEncoding[]    = {0xEF, 0xBF, 0xBF};
    const char lowFourEncoding[]      = {0xF0, 0x90, 0x80, 0x80};
    const char highFourEncoding[]     = {0xF4, 0x8F, 0xBF, 0xBF};

    // invalid continuation in byte 1
    const char invalidTwoEncoding[]   = {0xC2, 0xFF};
    // invalid continuation in byte 1
    const char invalidThreeEncoding[] = {0xE1, 0xC0, 0x80};
    // invalid continuation in byte 2
    const char invalidFourEncoding1[] = {0xF0, 0x89, 0xC0, 0x80};
    // invalid byte 0 (5 leading 1 bits)
    const char invalidFourEncoding2[] = {0xF8, 0x80, 0x80, 0x80};

    const char overlongTwoEncoding[] = {0xC0, 0x80};
    const char overlongThreeEncoding[] = {0xE0, 0x80, 0x80};
    const char overlongFourEncoding[] = {0xF0, 0x80, 0x80, 0x80};

    const char tooBigEncoding[] = {0xF7, 0xBF, 0xBF, 0xBF};

    // Encode and then decode all of the valid boundary conditions for encoding length
    codePoint = 0x000000;
    outSize = 4;
    LE_ASSERT(le_utf8_EncodeUnicodeCodePoint(codePoint, out, &outSize) == LE_OK);
    LE_ASSERT(outSize == sizeof(lowOneEncoding));
    LE_ASSERT(memcmp(out, lowOneEncoding, sizeof(lowOneEncoding)) == 0);
    LE_ASSERT(le_utf8_DecodeUnicodeCodePoint(out, &outSize, &decodedCodePoint) == LE_OK);
    LE_ASSERT(decodedCodePoint == codePoint);
    LE_ASSERT(outSize == 1);

    codePoint = 0x00007F;
    outSize = 4;
    LE_ASSERT(le_utf8_EncodeUnicodeCodePoint(codePoint, out, &outSize) == LE_OK);
    LE_ASSERT(outSize == sizeof(highOneEncoding));
    LE_ASSERT(memcmp(out, highOneEncoding, sizeof(highOneEncoding)) == 0);
    LE_ASSERT(le_utf8_DecodeUnicodeCodePoint(out, &outSize, &decodedCodePoint) == LE_OK);
    LE_ASSERT(decodedCodePoint == codePoint);
    LE_ASSERT(outSize == 1);

    codePoint = 0x000080;
    outSize = 4;
    LE_ASSERT(le_utf8_EncodeUnicodeCodePoint(codePoint, out, &outSize) == LE_OK);
    LE_ASSERT(outSize == sizeof(lowTwoEncoding));
    LE_ASSERT(memcmp(out, lowTwoEncoding, sizeof(lowTwoEncoding)) == 0);
    LE_ASSERT(le_utf8_DecodeUnicodeCodePoint(out, &outSize, &decodedCodePoint) == LE_OK);
    LE_ASSERT(decodedCodePoint == codePoint);
    LE_ASSERT(outSize == 2);

    codePoint = 0x0007FF;
    outSize = 4;
    LE_ASSERT(le_utf8_EncodeUnicodeCodePoint(codePoint, out, &outSize) == LE_OK);
    LE_ASSERT(outSize == sizeof(highTwoEncoding));
    LE_ASSERT(memcmp(out, highTwoEncoding, sizeof(highTwoEncoding)) == 0);
    LE_ASSERT(le_utf8_DecodeUnicodeCodePoint(out, &outSize, &decodedCodePoint) == LE_OK);
    LE_ASSERT(decodedCodePoint == codePoint);
    LE_ASSERT(outSize == 2);

    codePoint = 0x000800;
    outSize = 4;
    LE_ASSERT(le_utf8_EncodeUnicodeCodePoint(codePoint, out, &outSize) == LE_OK);
    LE_ASSERT(outSize == sizeof(lowThreeEncoding));
    LE_ASSERT(memcmp(out, lowThreeEncoding, sizeof(lowThreeEncoding)) == 0);
    LE_ASSERT(le_utf8_DecodeUnicodeCodePoint(out, &outSize, &decodedCodePoint) == LE_OK);
    LE_ASSERT(decodedCodePoint == codePoint);
    LE_ASSERT(outSize == 3);

    codePoint = 0x00FFFF;
    outSize = 4;
    LE_ASSERT(le_utf8_EncodeUnicodeCodePoint(codePoint, out, &outSize) == LE_OK);
    LE_ASSERT(outSize == sizeof(highThreeEncoding));
    LE_ASSERT(memcmp(out, highThreeEncoding, sizeof(highThreeEncoding)) == 0);
    LE_ASSERT(le_utf8_DecodeUnicodeCodePoint(out, &outSize, &decodedCodePoint) == LE_OK);
    LE_ASSERT(decodedCodePoint == codePoint);
    LE_ASSERT(outSize == 3);

    codePoint = 0x010000;
    outSize = 4;
    LE_ASSERT(le_utf8_EncodeUnicodeCodePoint(codePoint, out, &outSize) == LE_OK);
    LE_ASSERT(outSize == sizeof(lowFourEncoding));
    LE_ASSERT(memcmp(out, lowFourEncoding, sizeof(lowFourEncoding)) == 0);
    LE_ASSERT(le_utf8_DecodeUnicodeCodePoint(out, &outSize, &decodedCodePoint) == LE_OK);
    LE_ASSERT(decodedCodePoint == codePoint);
    LE_ASSERT(outSize == 4);

    codePoint = 0x10FFFF;
    outSize = 4;
    LE_ASSERT(le_utf8_EncodeUnicodeCodePoint(codePoint, out, &outSize) == LE_OK);
    LE_ASSERT(outSize == sizeof(highFourEncoding));
    LE_ASSERT(memcmp(out, highFourEncoding, sizeof(highFourEncoding)) == 0);
    LE_ASSERT(le_utf8_DecodeUnicodeCodePoint(out, &outSize, &decodedCodePoint) == LE_OK);
    LE_ASSERT(decodedCodePoint == codePoint);
    LE_ASSERT(outSize == 4);

    // Encoding a code point value that is too large should fail
    codePoint = UINT32_MAX;
    outSize = 4;
    LE_ASSERT(le_utf8_EncodeUnicodeCodePoint(codePoint, out, &outSize) == LE_OUT_OF_RANGE);

    // Attempt to encode code points that would be too large to fit in the buffer
    codePoint = 0x80;
    outSize = 1;
    LE_ASSERT(le_utf8_EncodeUnicodeCodePoint(codePoint, out, &outSize) == LE_OVERFLOW);
    LE_ASSERT(outSize == 2);

    codePoint = 0x800;
    outSize = 2;
    LE_ASSERT(le_utf8_EncodeUnicodeCodePoint(codePoint, out, &outSize) == LE_OVERFLOW);
    LE_ASSERT(outSize == 3);

    codePoint = 0x10000;
    outSize = 3;
    LE_ASSERT(le_utf8_EncodeUnicodeCodePoint(codePoint, out, &outSize) == LE_OVERFLOW);
    LE_ASSERT(outSize == 4);

    // Attempt to decode from a zero length input
    outSize = 0;
    LE_ASSERT(
        le_utf8_DecodeUnicodeCodePoint(
            lowOneEncoding, &outSize, &decodedCodePoint) == LE_BAD_PARAMETER);

    // Attempt to decode from input that is too short
    outSize = sizeof(lowTwoEncoding) - 1;
    LE_ASSERT(
        le_utf8_DecodeUnicodeCodePoint(
            lowTwoEncoding, &outSize, &decodedCodePoint) == LE_UNDERFLOW);

    outSize = sizeof(lowThreeEncoding) - 1;
    LE_ASSERT(
        le_utf8_DecodeUnicodeCodePoint(
            lowThreeEncoding, &outSize, &decodedCodePoint) == LE_UNDERFLOW);

    outSize = sizeof(lowFourEncoding) - 1;
    LE_ASSERT(
        le_utf8_DecodeUnicodeCodePoint(
            lowFourEncoding, &outSize, &decodedCodePoint) == LE_UNDERFLOW);

    // Attempt to decode from buffers which are not valid utf-8
    outSize = sizeof(invalidTwoEncoding);
    LE_ASSERT(
        le_utf8_DecodeUnicodeCodePoint(
            invalidTwoEncoding, &outSize, &decodedCodePoint) == LE_FORMAT_ERROR);

    outSize = sizeof(invalidThreeEncoding);
    LE_ASSERT(
        le_utf8_DecodeUnicodeCodePoint(
            invalidThreeEncoding, &outSize, &decodedCodePoint) == LE_FORMAT_ERROR);

    outSize = sizeof(invalidFourEncoding1);
    LE_ASSERT(
        le_utf8_DecodeUnicodeCodePoint(
            invalidFourEncoding1, &outSize, &decodedCodePoint) == LE_FORMAT_ERROR);

    outSize = sizeof(invalidFourEncoding2);
    LE_ASSERT(
        le_utf8_DecodeUnicodeCodePoint(
            invalidFourEncoding2, &outSize, &decodedCodePoint) == LE_FORMAT_ERROR);

    // Attempt to decode overlong encoded data which is invalid utf-8
    outSize = sizeof(overlongTwoEncoding);
    LE_ASSERT(
        le_utf8_DecodeUnicodeCodePoint(
            overlongTwoEncoding, &outSize, &decodedCodePoint) == LE_FORMAT_ERROR);

    outSize = sizeof(overlongThreeEncoding);
    LE_ASSERT(
        le_utf8_DecodeUnicodeCodePoint(
            overlongThreeEncoding, &outSize, &decodedCodePoint) == LE_FORMAT_ERROR);

    outSize = sizeof(overlongFourEncoding);
    LE_ASSERT(
        le_utf8_DecodeUnicodeCodePoint(
            overlongFourEncoding, &outSize, &decodedCodePoint) == LE_FORMAT_ERROR);

    // decode codepoint > 0x10ffff which is the maximum allowed value in utf-8
    outSize = sizeof(tooBigEncoding);
    LE_ASSERT(
        le_utf8_DecodeUnicodeCodePoint(
            tooBigEncoding, &outSize, &decodedCodePoint) == LE_OUT_OF_RANGE);
}


COMPONENT_INIT
{
    size_t numBytesCopied;
    char asciiStr[] = "ASCII String";
    char asciiStr2[] = "Second ASCII String";
    char multiByteStr[] = {TWO_CHAR_BYTE, CONT_BYTE, 'a', THREE_CHAR_BYTE, CONT_BYTE, CONT_BYTE,
                           FOUR_CHAR_BYTE, CONT_BYTE, CONT_BYTE, CONT_BYTE, '\0'};
    char multiByteStr2[] = {FOUR_CHAR_BYTE, CONT_BYTE, CONT_BYTE, CONT_BYTE, 'a',
                            THREE_CHAR_BYTE, CONT_BYTE, CONT_BYTE,
                            TWO_CHAR_BYTE, CONT_BYTE, '\0'};
    char missLeadStr[] = {TWO_CHAR_BYTE, CONT_BYTE, 'a', CONT_BYTE, CONT_BYTE,
                          FOUR_CHAR_BYTE, CONT_BYTE, CONT_BYTE, CONT_BYTE, '\0'};
    char missContStr[] = {TWO_CHAR_BYTE, CONT_BYTE, 'a', THREE_CHAR_BYTE, CONT_BYTE, CONT_BYTE,
                           FOUR_CHAR_BYTE, CONT_BYTE, CONT_BYTE, '\0'};
    char invalidStr[] = {TWO_CHAR_BYTE, CONT_BYTE, INVALID_BYTE, FOUR_CHAR_BYTE, CONT_BYTE, CONT_BYTE, CONT_BYTE, '\0'};
    char destBuffer[100];
    char multiByteSubStr[] = {THREE_CHAR_BYTE, CONT_BYTE, CONT_BYTE, TWO_CHAR_BYTE, CONT_BYTE, '\0'};
    char longMultiByteStr[] = {TWO_CHAR_BYTE, CONT_BYTE, 'a', 'b',
                               THREE_CHAR_BYTE, CONT_BYTE, CONT_BYTE,
                               TWO_CHAR_BYTE, CONT_BYTE,
                               'c',
                               FOUR_CHAR_BYTE, CONT_BYTE, CONT_BYTE, CONT_BYTE,
                               THREE_CHAR_BYTE, CONT_BYTE, CONT_BYTE,
                               TWO_CHAR_BYTE, CONT_BYTE,
                               THREE_CHAR_BYTE, CONT_BYTE, CONT_BYTE,
                               'c',
                               THREE_CHAR_BYTE, CONT_BYTE, CONT_BYTE, '\0'};

    printf("\n");
    printf("*** Unit Test for le_utf8 module in liblegato.so library. ***\n");


    // Get the number of bytes in the strings.
    if (le_utf8_NumBytes(asciiStr) != sizeof(asciiStr) - 1)
    {
        printf("Num bytes incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    if (le_utf8_NumBytes(multiByteStr) != sizeof(multiByteStr) - 1)
    {
        printf("Num bytes incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    if (le_utf8_NumBytes(missLeadStr) != sizeof(missLeadStr) - 1)
    {
        printf("Num bytes incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    if (le_utf8_NumBytes(missContStr) != sizeof(missContStr) - 1)
    {
        printf("Num bytes incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    if (le_utf8_NumBytes(invalidStr) != sizeof(invalidStr) - 1)
    {
        printf("Num bytes incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    printf("Got number of bytes correctly.\n");


    // Get the number of chars in the strings.
    if (le_utf8_NumChars(asciiStr) != sizeof(asciiStr) - 1)
    {
        printf("Num chars incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    if (le_utf8_NumChars(multiByteStr) != 4)
    {
        printf("Num chars incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    if (le_utf8_NumChars(missLeadStr) != LE_FORMAT_ERROR)
    {
        printf("Num chars incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    if (le_utf8_NumChars(missContStr) != LE_FORMAT_ERROR)
    {
        printf("Num chars incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    if (le_utf8_NumChars(invalidStr) != LE_FORMAT_ERROR)
    {
        printf("Num chars incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    printf("Got number of chars correctly.\n");


    // Copy strings.
    if ( (le_utf8_Copy(destBuffer, asciiStr, 100, &numBytesCopied) != LE_OK) ||
         (numBytesCopied != sizeof(asciiStr) - 1) ||
         (strncmp(destBuffer, asciiStr, sizeof(asciiStr)) != 0) )
    {
        printf("Copy incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    if ( (le_utf8_Copy(destBuffer, multiByteStr, 100, &numBytesCopied) != LE_OK) ||
         (numBytesCopied != sizeof(multiByteStr)-1) ||
         (strncmp(destBuffer, multiByteStr, sizeof(multiByteStr)) != 0) )
    {
        printf("Copy incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }


    // Copy ascii truncate
    if ( (le_utf8_Copy(destBuffer, asciiStr, 8, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 7) ||
         (strncmp(destBuffer, asciiStr, 7) != 0) ) // Don't compare the null char.
    {
        printf("Copy incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    // Copy multi-byte truncate with alignment
    if ( (le_utf8_Copy(destBuffer, multiByteStr, 7, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 6) ||
         (strncmp(destBuffer, multiByteStr, 6) != 0) ) // Don't compare the null char.
    {
        printf("Copy incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    // Copy multi-byte truncate where the lead character is at the end of the buffer -1.
    if ( (le_utf8_Copy(destBuffer, multiByteStr, 8, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 6) ||
         (strncmp(destBuffer, multiByteStr, 6) != 0) ) // Don't compare the null char.
    {
        printf("Copy incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    // Copy multi-byte truncate where the last character straddles the end of the buffer.
    if ( (le_utf8_Copy(destBuffer, multiByteStr, 9, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 6) ||
         (strncmp(destBuffer, multiByteStr, 6) != 0) ) // Don't compare the null char.
    {
        printf("Copy incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    printf("Copies correct.\n");


    // Append strings.
    le_utf8_Copy(destBuffer, asciiStr, 100, NULL);
    if ( (le_utf8_Append(destBuffer, asciiStr2, 100, &numBytesCopied) != LE_OK) ||
         (numBytesCopied != sizeof(asciiStr) + sizeof(asciiStr2) - 2) ||
         (!compareConCat(destBuffer, asciiStr, asciiStr2, 100)) )
    {
        printf("Append incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    le_utf8_Copy(destBuffer, multiByteStr, 100, NULL);
    if ( (le_utf8_Append(destBuffer, multiByteStr2, 100, &numBytesCopied) != LE_OK) ||
         (numBytesCopied != sizeof(multiByteStr) + sizeof(multiByteStr2) - 2) ||
         (!compareConCat(destBuffer, multiByteStr, multiByteStr2, 100)) )
    {
        printf("Append incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    // Append ascii truncate
    le_utf8_Copy(destBuffer, asciiStr, 100, NULL);
    if ( (le_utf8_Append(destBuffer, asciiStr2, 15, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 14) ||
         (!compareConCat(destBuffer, asciiStr, asciiStr2, 15)) )
    {
        printf("Append incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    // Append multi-byte truncate with alignment
    le_utf8_Copy(destBuffer, multiByteStr, 100, NULL);
    if ( (le_utf8_Append(destBuffer, multiByteStr2, 16, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 15) ||
         (!compareConCat(destBuffer, multiByteStr, multiByteStr2, 16)) )
    {
        printf("Append incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    // Append multi-byte truncate where the lead character is at the end of the buffer -1.
    le_utf8_Copy(destBuffer, multiByteStr, 100, NULL);
    if ( (le_utf8_Append(destBuffer, multiByteStr2, 17, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 15) ||
         (!compareConCat(destBuffer, multiByteStr, multiByteStr2, 16)) )
    {
        printf("Append incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    // Append multi-byte truncate where the last character straddles the end of the buffer.
    le_utf8_Copy(destBuffer, multiByteStr, 100, NULL);
    if ( (le_utf8_Append(destBuffer, multiByteStr2, 18, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 15) ||
         (!compareConCat(destBuffer, multiByteStr, multiByteStr2, 16)) )
    {
        printf("Append incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    printf("Appends correct.\n");


    // Check the formatting of strings.
    if (le_utf8_IsFormatCorrect(asciiStr) == false)
    {
        printf("Format check incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    if (le_utf8_IsFormatCorrect(multiByteStr) == false)
    {
        printf("Format check incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    if (le_utf8_IsFormatCorrect(missLeadStr) == true)
    {
        printf("Format check incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    if (le_utf8_IsFormatCorrect(missContStr) == true)
    {
        printf("Format check incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    if (le_utf8_IsFormatCorrect(invalidStr) == true)
    {
        printf("Format check incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    printf("Format checks correct.\n");



    // Copy up to a substring.

    // Copy up to an ascii character.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, asciiStr, "t", sizeof(destBuffer), &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != 7) ||
         (strncmp(destBuffer, asciiStr, 7) != 0) )
    {
        printf("Copy up to incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    // Copy up to a multibyte substring in an ascii string.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, asciiStr, multiByteSubStr, sizeof(destBuffer), &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != strlen(asciiStr)) ||
         (strcmp(destBuffer, asciiStr) != 0) )
    {
        printf("Copy up to incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    // Copy up to a multibyte substring in a multibyte string.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, multiByteStr2, multiByteSubStr, sizeof(destBuffer), &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != 5) ||
         (strncmp(destBuffer, multiByteStr2, 5) != 0) )
    {
        printf("Copy up to incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    // Copy up to a multibyte substring in a multibyte string.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, longMultiByteStr, multiByteSubStr, sizeof(destBuffer), &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != 4) ||
         (strncmp(destBuffer, longMultiByteStr, 4) != 0) )
    {
        printf("Copy up to incorrect: %d, %zd\n", __LINE__, numBytesCopied);
        exit(EXIT_FAILURE);
    }

    // Copy up to an ascii character in a multibyte string.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, multiByteStr2, "a", sizeof(destBuffer), &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != 4) ||
         (strncmp(destBuffer, multiByteStr2, 4) != 0) )
    {
        printf("Copy up to incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    // Copy up to a character in a multibyte string but not there.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, multiByteStr2, "X", sizeof(destBuffer), &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != strlen(multiByteStr2)) ||
         (strcmp(destBuffer, multiByteStr2) != 0) )
    {
        printf("Copy up to incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    // Copy up to an ascii character so that the buffer is completely filled.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, asciiStr, " ", 6, &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != 5) ||
         (strncmp(destBuffer, asciiStr, 5) != 0) )
    {
        printf("Copy up to incorrect: %d\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    printf("Copy Up To Substring correct.\n");

    TestIntParsing();

    printf("Int parsing correct.\n");

    printf("Testing encode/decode\n");
    TestEncodeDecodeCodePoint();
    printf("Completed testing encode/decode\n");

    printf("*** Unit Test for le_utf8 module passed. ***\n");
    printf("\n");

    exit(EXIT_SUCCESS);
}


static bool compareConCat(char* finalStr, char* firstStr, char* secondStr, size_t numBytesToCheck)
{
    size_t i;
    char* catStr = firstStr;
    bool usingSecondStr = false;

    // Compares the finalStr with the concatenation of firstStr and secondStr up to numBytesToCheck.
    for (i = 0; i < numBytesToCheck; i++)
    {
        if (finalStr[i] == '\0')
        {
            // End of final string just stop because the null-terminator may not match if there was a truncation.
            break;
        }

        if (*catStr == '\0')
        {
            if (usingSecondStr)
            {
                // End of the second string but not the final string so they must not be equal.
                return false;
            }
            else
            {
                // Switch to the second string.
                catStr = secondStr;
                usingSecondStr = true;
            }
        }

        if (finalStr[i] != *catStr)
        {
            return false;
        }

        catStr++;
    }

    return true;
}

