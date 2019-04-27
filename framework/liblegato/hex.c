/**
 *
 * Copyright (C) Sierra Wireless Inc.
 */


/** @file hex.c
 *
 * Copyright (C) Sierra Wireless Inc.
 */
#include "legato.h"

//--------------------------------------------------------------------------------------------------
/**
 * Number of HexDump Columns
 */
//--------------------------------------------------------------------------------------------------
#define DUMP_COLS                                   16

//--------------------------------------------------------------------------------------------------
/**
 * HexDump line length
 */
//--------------------------------------------------------------------------------------------------
#define DUMP_LINE_LEN                               80

//--------------------------------------------------------------------------------------------------
/**
 * HexDump insufficient buffer error message
 */
//--------------------------------------------------------------------------------------------------
#define DUMP_INSUFFICIENT_BUFFER_LEN_MSG           "Buffer too small!\n"


//--------------------------------------------------------------------------------------------------
/**
 * Safely appends a new string to existing string taking into account
 * destination buffer size and current string length
 */
//--------------------------------------------------------------------------------------------------
static inline void SafeStrAppend
(
    char        *destBufferPtr,         ///< [IN]  Buffer to append to.
    size_t       destBufferSize,        ///< [IN]  Buffer size.
    size_t      *currentDestStrLenPtr,  ///< [OUT] Length of resulting string.
    const char  *appendStrPtr           ///< [IN]  String to append to buffer.
)
{
    le_result_t result = le_utf8_Append(destBufferPtr, appendStrPtr, destBufferSize,
                            currentDestStrLenPtr);
    if (result != LE_OK)
    {
        LE_WARN("Error appending to string: %s", LE_RESULT_TXT(result));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert a numeric value into a uppercase character representing the hexidecimal value of the
 * input.
 *
 * @return Hexidecimal character in the range [0-9A-F] or 0 if the input value was too large
 */
//--------------------------------------------------------------------------------------------------
static char DecToHex(uint8_t hex)
{
    if (hex < 10) {
        return (char)('0'+hex);  // for number
    }
    else if (hex < 16) {
        return (char)('A'+hex-10);  // for A,B,C,D,E,F
    }
    else {
        LE_DEBUG("value %u cannot be converted in HEX string", hex);
        return 0;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert a string of valid hexadecimal characters [0-9a-fA-F] into a byte array where each
 * element of the byte array holds the value corresponding to a pair of hexadecimal characters.
 *
 * @return
 *      - number of bytes written into binaryPtr
 *      - -1 if the binarySize is too small or stringLength is odd or stringPtr contains an invalid
 *        character
 *
 * @note The input string is not required to be NULL terminated.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_hex_StringToBinary
(
    const char *stringPtr,     ///< [IN] string to convert
    uint32_t    stringLength,  ///< [IN] string length
    uint8_t    *binaryPtr,     ///< [OUT] binary result
    uint32_t    binarySize     ///< [IN] size of the binary table.  Must be >= stringLength / 2
)
{
    uint32_t idxString;
    uint32_t idxBinary;
    char*    refStrPtr = "0123456789ABCDEF";

    if (stringLength > strlen(stringPtr))
    {
        LE_DEBUG("The stringLength (%" PRIu32 ") is more than size of stringPtr (%s)",
                 stringLength, stringPtr);
        return -1;
    }

    if (stringLength % 2 != 0)
    {
        LE_DEBUG("The input stringLength=%" PRIu32 " is not a multiple of 2", stringLength);
        return -1;
    }

    if (stringLength / 2 > binarySize)
    {
        LE_DEBUG(
            "The stringLength (%" PRIu32 ") is too long to convert"
            " into a byte array of length (%" PRIu32 ")",
            stringLength,
            binarySize);
        return -1;
    }

    for (idxString=0,idxBinary=0 ; idxString<stringLength ; idxString+=2,idxBinary++)
    {
        char* ch1Ptr;
        char* ch2Ptr;

        if ( ((ch1Ptr = strchr(refStrPtr, toupper((int)stringPtr[idxString]))) && *ch1Ptr) &&
             ((ch2Ptr = strchr(refStrPtr, toupper((int)stringPtr[idxString+1]))) && *ch2Ptr) )
        {
            binaryPtr[idxBinary] = ((ch2Ptr - refStrPtr) & 0x0F) |
                                   (((ch1Ptr - refStrPtr)<<4) & 0xF0);
        }
        else
        {
            LE_DEBUG("Invalid string to convert (%s)", stringPtr);
            return -1;
        }
    }

    return idxBinary;
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert a byte array into a string of uppercase hexadecimal characters.
 *
 * @return number of characters written to stringPtr or -1 if stringSize is too small for
 *         binarySize
 *
 * @note the string written to stringPtr will be NULL terminated.
 */
//--------------------------------------------------------------------------------------------------
int32_t le_hex_BinaryToString
(
    const uint8_t *binaryPtr,  ///< [IN] binary array to convert
    uint32_t       binarySize, ///< [IN] size of binary array
    char          *stringPtr,  ///< [OUT] hex string array, terminated with '\0'.
    uint32_t       stringSize  ///< [IN] size of string array.  Must be >= (2 * binarySize) + 1
)
{
    uint32_t idxString,idxBinary;

    if (stringSize < (2 * binarySize) + 1)
    {
        LE_DEBUG(
            "Hex string array (%" PRIu32 ") is too small to convert (%" PRIu32 ") bytes",
            stringSize,
            binarySize);
        return -1;
    }

    for(idxBinary=0,idxString=0;
        idxBinary<binarySize;
        idxBinary++,idxString=idxString+2)
    {
        stringPtr[idxString]   = DecToHex( (binaryPtr[idxBinary]>>4) & 0x0F);
        stringPtr[idxString+1] = DecToHex(  binaryPtr[idxBinary]     & 0x0F);
    }
    stringPtr[idxString] = '\0';

    return idxString;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function that takes binary data and creates 'hex dump' that is
 * stored into user provided ASCII buffer and is null terminated
 * Total length of one line will be 74 characters:
 * 9 + (16 * 3) + 17. Hence, you should choose ascii buffer
 * size to be about 5 times the size of binary data you want
 * to hexdump.
 * 0x000000: 2e 2f 68 65 78 64 75 6d 0 00 53 53 48 5f 41 47 ./hexdump.SSH_AG
 */
//--------------------------------------------------------------------------------------------------
void le_hex_Dump
(
    char     *asciiBufferPtr,
    size_t    asciiBufferSize,
    char     *binaryDataPtr,
    size_t    binaryDataLen
)
{
    size_t  i, j;
    char    tempBuffer[DUMP_LINE_LEN];
    size_t  currentDumpLen;

    /**
     * First calculate if converted binary data would fit
     * into provided buffer
     */
    if ((binaryDataLen * (DUMP_LINE_LEN/DUMP_COLS)) > asciiBufferSize)
    {
        snprintf(asciiBufferPtr, asciiBufferSize, "%s", DUMP_INSUFFICIENT_BUFFER_LEN_MSG);
        return;
    }

    asciiBufferPtr[0] = '\0';

    /**
     * Used to avoid constant strlen() which would degrade efficiency
     */
    currentDumpLen = 0;

    for (i = 0;
         i < binaryDataLen + ((binaryDataLen % DUMP_COLS) ? (DUMP_COLS - (binaryDataLen % DUMP_COLS)) : 0);
         i++)
    {
        /* print offset */
        if (i % DUMP_COLS == 0)
        {
            snprintf(tempBuffer, sizeof(tempBuffer), "0x%06" PRIxS ": ", i);
            SafeStrAppend(asciiBufferPtr, asciiBufferSize, &currentDumpLen, tempBuffer);
        }

        /* print hex data */
        if (i < binaryDataLen)
        {
            snprintf(tempBuffer,
                     sizeof(tempBuffer),
                     "%02x ",
                     0xFF & ((char*)binaryDataPtr)[i]);
            SafeStrAppend(asciiBufferPtr, asciiBufferSize, &currentDumpLen, tempBuffer);
        }
        else /* end of block, just aligning for ASCII dump */
        {
            SafeStrAppend(asciiBufferPtr, asciiBufferSize, &currentDumpLen, "   ");
        }

        /* print ASCII dump */
        if (i % DUMP_COLS == (DUMP_COLS - 1))
        {
            for (j = i - (DUMP_COLS - 1); j <= i; j++)
            {
                if (j >= binaryDataLen) /* end of block, not really printing */
                {
                    SafeStrAppend(asciiBufferPtr, asciiBufferSize, &currentDumpLen, " ");
                }
                else if(isprint((int)binaryDataPtr[j])) /* printable char */
                {
                    snprintf(tempBuffer,
                             sizeof(tempBuffer),
                             "%c",
                             0xFF & ((char)binaryDataPtr[j]));
                    SafeStrAppend(asciiBufferPtr, asciiBufferSize, &currentDumpLen, tempBuffer);
                }
                else /* other char */
                {
                    SafeStrAppend(asciiBufferPtr, asciiBufferSize, &currentDumpLen, ".");
                }
            }
            SafeStrAppend(asciiBufferPtr, asciiBufferSize, &currentDumpLen, "\n");
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Convert a NULL terminated string of valid hexadecimal characters [0-9a-fA-F] into an integer.
 *
 * @return
 *      - Positive integer corresponding to the hexadecimal input string
 *      - -1 if the input contains an invalid character or the value will not fit in an integer
 */
//--------------------------------------------------------------------------------------------------
int le_hex_HexaToInteger
(
    const char *stringPtr ///< [IN] string of hex chars to convert into an int
)
{
    int result = 0;
    while (*stringPtr != '\0')
    {
        if (result > (INT_MAX / 16))
        {
            // Consuming more input data may overflow the integer value
            return -1;
        }

        char base;
        if (*stringPtr >= '0' && *stringPtr <='9')
        {
            base = '0';
        }
        else if (*stringPtr >= 'a' && *stringPtr <= 'f')
        {
            base = 'a' - 10;
        }
        else if (*stringPtr >= 'A' && *stringPtr <= 'F')
        {
            base = 'A' - 10;
        }
        else
        {
            // Invalid input character
            return -1;
        }

        result = (result << 4) | (*stringPtr - base);
        stringPtr++;
    }

    return result;
}
