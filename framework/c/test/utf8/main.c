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
  *
  * Copyright (C) Sierra Wireless, Inc. 2012.  All rights reserved. Use of this work is subject to license.
  */

#include "legato.h"
#include <stdio.h>

#define TWO_CHAR_BYTE   0xC0
#define THREE_CHAR_BYTE 0xE0
#define FOUR_CHAR_BYTE  0xF0
#define CONT_BYTE       0x80
#define INVALID_BYTE    0xA0


static bool compareConCat(char* finalStr, char* firstStr, char* secondStr, size_t numBytesToCheck);


int main(int argc, char *argv[])
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
        return LE_FAULT;
    }

    if (le_utf8_NumBytes(multiByteStr) != sizeof(multiByteStr) - 1)
    {
        printf("Num bytes incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    if (le_utf8_NumBytes(missLeadStr) != sizeof(missLeadStr) - 1)
    {
        printf("Num bytes incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    if (le_utf8_NumBytes(missContStr) != sizeof(missContStr) - 1)
    {
        printf("Num bytes incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    if (le_utf8_NumBytes(invalidStr) != sizeof(invalidStr) - 1)
    {
        printf("Num bytes incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    printf("Got number of bytes correctly.\n");


    // Get the number of chars in the strings.
    if (le_utf8_NumChars(asciiStr) != sizeof(asciiStr) - 1)
    {
        printf("Num chars incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    if (le_utf8_NumChars(multiByteStr) != 4)
    {
        printf("Num chars incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    if (le_utf8_NumChars(missLeadStr) != LE_FORMAT_ERROR)
    {
        printf("Num chars incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    if (le_utf8_NumChars(missContStr) != LE_FORMAT_ERROR)
    {
        printf("Num chars incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    if (le_utf8_NumChars(invalidStr) != LE_FORMAT_ERROR)
    {
        printf("Num chars incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    printf("Got number of chars correctly.\n");


    // Copy strings.
    if ( (le_utf8_Copy(destBuffer, asciiStr, 100, &numBytesCopied) != LE_OK) ||
         (numBytesCopied != sizeof(asciiStr) - 1) ||
         (strncmp(destBuffer, asciiStr, sizeof(asciiStr)) != 0) )
    {
        printf("Copy incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    if ( (le_utf8_Copy(destBuffer, multiByteStr, 100, &numBytesCopied) != LE_OK) ||
         (numBytesCopied != sizeof(multiByteStr)-1) ||
         (strncmp(destBuffer, multiByteStr, sizeof(multiByteStr)) != 0) )
    {
        printf("Copy incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }


    // Copy ascii truncate
    if ( (le_utf8_Copy(destBuffer, asciiStr, 8, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 7) ||
         (strncmp(destBuffer, asciiStr, 7) != 0) ) // Don't compare the null char.
    {
        printf("Copy incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    // Copy multi-byte truncate with alignment
    if ( (le_utf8_Copy(destBuffer, multiByteStr, 7, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 6) ||
         (strncmp(destBuffer, multiByteStr, 6) != 0) ) // Don't compare the null char.
    {
        printf("Copy incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    // Copy multi-byte truncate where the lead character is at the end of the buffer -1.
    if ( (le_utf8_Copy(destBuffer, multiByteStr, 8, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 6) ||
         (strncmp(destBuffer, multiByteStr, 6) != 0) ) // Don't compare the null char.
    {
        printf("Copy incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    // Copy multi-byte truncate where the last character straddles the end of the buffer.
    if ( (le_utf8_Copy(destBuffer, multiByteStr, 9, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 6) ||
         (strncmp(destBuffer, multiByteStr, 6) != 0) ) // Don't compare the null char.
    {
        printf("Copy incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    printf("Copies correct.\n");


    // Append strings.
    le_utf8_Copy(destBuffer, asciiStr, 100, NULL);
    if ( (le_utf8_Append(destBuffer, asciiStr2, 100, &numBytesCopied) != LE_OK) ||
         (numBytesCopied != sizeof(asciiStr) + sizeof(asciiStr2) - 2) ||
         (!compareConCat(destBuffer, asciiStr, asciiStr2, 100)) )
    {
        printf("Append incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    le_utf8_Copy(destBuffer, multiByteStr, 100, NULL);
    if ( (le_utf8_Append(destBuffer, multiByteStr2, 100, &numBytesCopied) != LE_OK) ||
         (numBytesCopied != sizeof(multiByteStr) + sizeof(multiByteStr2) - 2) ||
         (!compareConCat(destBuffer, multiByteStr, multiByteStr2, 100)) )
    {
        printf("Append incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    // Append ascii truncate
    le_utf8_Copy(destBuffer, asciiStr, 100, NULL);
    if ( (le_utf8_Append(destBuffer, asciiStr2, 15, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 14) ||
         (!compareConCat(destBuffer, asciiStr, asciiStr2, 15)) )
    {
        printf("Append incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    // Append multi-byte truncate with alignment
    le_utf8_Copy(destBuffer, multiByteStr, 100, NULL);
    if ( (le_utf8_Append(destBuffer, multiByteStr2, 16, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 15) ||
         (!compareConCat(destBuffer, multiByteStr, multiByteStr2, 16)) )
    {
        printf("Append incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    // Append multi-byte truncate where the lead character is at the end of the buffer -1.
    le_utf8_Copy(destBuffer, multiByteStr, 100, NULL);
    if ( (le_utf8_Append(destBuffer, multiByteStr2, 17, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 15) ||
         (!compareConCat(destBuffer, multiByteStr, multiByteStr2, 16)) )
    {
        printf("Append incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    // Append multi-byte truncate where the last character straddles the end of the buffer.
    le_utf8_Copy(destBuffer, multiByteStr, 100, NULL);
    if ( (le_utf8_Append(destBuffer, multiByteStr2, 18, &numBytesCopied) != LE_OVERFLOW) ||
         (numBytesCopied != 15) ||
         (!compareConCat(destBuffer, multiByteStr, multiByteStr2, 16)) )
    {
        printf("Append incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    printf("Appends correct.\n");


    // Check the formatting of strings.
    if (le_utf8_IsFormatCorrect(asciiStr) == false)
    {
        printf("Format check incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    if (le_utf8_IsFormatCorrect(multiByteStr) == false)
    {
        printf("Format check incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    if (le_utf8_IsFormatCorrect(missLeadStr) == true)
    {
        printf("Format check incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    if (le_utf8_IsFormatCorrect(missContStr) == true)
    {
        printf("Format check incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    if (le_utf8_IsFormatCorrect(invalidStr) == true)
    {
        printf("Format check incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    printf("Format checks correct.\n");



    // Copy up to a substring.

    // Copy up to an ascii character.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, asciiStr, "t", sizeof(destBuffer), &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != 7) ||
         (strncmp(destBuffer, asciiStr, 7) != 0) )
    {
        printf("Copy up to incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    // Copy up to a multibyte substring in an ascii string.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, asciiStr, multiByteSubStr, sizeof(destBuffer), &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != strlen(asciiStr)) ||
         (strcmp(destBuffer, asciiStr) != 0) )
    {
        printf("Copy up to incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    // Copy up to a multibyte substring in a multibyte string.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, multiByteStr2, multiByteSubStr, sizeof(destBuffer), &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != 5) ||
         (strncmp(destBuffer, multiByteStr2, 5) != 0) )
    {
        printf("Copy up to incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    // Copy up to a multibyte substring in a multibyte string.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, longMultiByteStr, multiByteSubStr, sizeof(destBuffer), &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != 4) ||
         (strncmp(destBuffer, longMultiByteStr, 4) != 0) )
    {
        printf("Copy up to incorrect: %d, %zd\n", __LINE__, numBytesCopied);
        return LE_FAULT;
    }

    // Copy up to an ascii character in a multibyte string.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, multiByteStr2, "a", sizeof(destBuffer), &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != 4) ||
         (strncmp(destBuffer, multiByteStr2, 4) != 0) )
    {
        printf("Copy up to incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    // Copy up to a character in a multibyte string but not there.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, multiByteStr2, "X", sizeof(destBuffer), &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != strlen(multiByteStr2)) ||
         (strcmp(destBuffer, multiByteStr2) != 0) )
    {
        printf("Copy up to incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    // Copy up to an ascii character so that the buffer is completely filled.
    if ( (le_utf8_CopyUpToSubStr(destBuffer, asciiStr, " ", 6, &numBytesCopied) == LE_OVERFLOW) ||
         (numBytesCopied != 5) ||
         (strncmp(destBuffer, asciiStr, 5) != 0) )
    {
        printf("Copy up to incorrect: %d\n", __LINE__);
        return LE_FAULT;
    }

    printf("Copy Up To Substring correct.\n");

    printf("*** Unit Test for le_utf8 module passed. ***\n");
    printf("\n");

    return LE_OK;
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

