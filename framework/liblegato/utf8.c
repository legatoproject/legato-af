//--------------------------------------------------------------------------------------------------
/** @file utf8.c
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"


//--------------------------------------------------------------------------------------------------
// Local definitions.
//--------------------------------------------------------------------------------------------------
// Checks the size of the char by looking at the lead byte.
#define IS_SINGLE_BYTE_CHAR(leadByte)           ( (leadByte & 0x80) == 0x00 )
#define IS_TWO_BYTE_CHAR(leadByte)              ( (leadByte & 0xE0) == 0xC0 )
#define IS_THREE_BYTE_CHAR(leadByte)            ( (leadByte & 0xF0) == 0xE0 )
#define IS_FOUR_BYTE_CHAR(leadByte)             ( (leadByte & 0xF8) == 0xF0 )


//--------------------------------------------------------------------------------------------------
/**
 * Parse Hex string to uint16_t value
 *
 * @return
 *      - Parsed Hex number in uint16_t type
 */
//--------------------------------------------------------------------------------------------------
static uint16_t ParseHex4
(
    const char* input               ///< [IN] String from which to parse hexadecimal value
)
{
    uint16_t hexVal = 0;
    uint8_t i = 0;

    for (i = 0; i < 4; i++)
    {
        if ((input[i] >= '0') && (input[i] <= '9'))
        {
            hexVal += (uint16_t) input[i] - '0';
        }
        else if ((input[i] >= 'A') && (input[i] <= 'F'))
        {
            hexVal += (uint16_t) 10 + input[i] - 'A';
        }
        else if ((input[i] >= 'a') && (input[i] <= 'f'))
        {
            hexVal += (uint16_t) 10 + input[i] - 'a';
        }
        else
        {
            return 0;
        }

        if (i < 3)
        {
            // shift left to make place for the next nibble
            hexVal = hexVal << 4;
        }
    }

    return hexVal;
}


//--------------------------------------------------------------------------------------------------
/**
* Converts a UTF-16 literal to UTF-8
* A literal can be one or two sequences of the form \uXXXX
*
* @return
*      - 0 on failure
*      - Converted literal length in bytes on success
*/
//--------------------------------------------------------------------------------------------------
static uint8_t Utf16LiteralToUtf8
(
    const char* inputPtr,           ///< [IN] Pointer to the UTF16 literal
    const char* inputEnd,           ///< [IN] Pointer to the end of the string to parse
    char** outputPtr,               ///< [IN/OUT] Buffer to which converted literal will
                                    ///<          be written to
    size_t* outputLen               ///< [IN/OUT] Output buffer size
)
{
    uint64_t codepoint = 0;
    uint16_t firstCode = 0;
    const char *firstSequence = inputPtr;
    uint8_t utf8Len = 0;
    uint8_t utf8Pos = 0;
    uint8_t sequenceLen = 0;
    uint8_t firstByteMark = 0;

    if ((inputEnd - firstSequence) < 6)
    {
        LE_ERROR("UTF16 input not long enough");
        goto fail;
    }

    firstCode = ParseHex4(firstSequence + 2);
    if (((firstCode >= 0xDC00) && (firstCode <= 0xDFFF)))
    {
        LE_ERROR("UTF16 input: invalid code");
        goto fail;
    }

    // UTF16 surrogate pair
    if ((firstCode >= 0xD800) && (firstCode <= 0xDBFF))
    {
        const char *second_sequence = firstSequence + 6;
        uint16_t secondCode = 0;
        sequenceLen = 12; // \uXXXX\uXXXX

        if ((inputEnd - second_sequence) < 6)
        {
            LE_ERROR("UTF16 input surrogate pair not long enough");
            goto fail;
        }

        if ((second_sequence[0] != '\\') || (second_sequence[1] != 'u'))
        {
            LE_ERROR("UTF16 input missing surrogate pair");
            goto fail;
        }

        secondCode = ParseHex4(second_sequence + 2);
        if ((secondCode < 0xDC00) || (secondCode > 0xDFFF))
        {
            LE_ERROR("UTF16 input: invalid code for surrogate pair");
            goto fail;
        }

        codepoint = 0x10000 + (((firstCode & 0x3FF) << 10) | (secondCode & 0x3FF));
    }
    else
    {
        sequenceLen = 6; // \uXXXX
        codepoint = firstCode;
    }

    /* encode as UTF-8
     * takes at most 4 bytes to encode:
     * 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    if (codepoint < 0x80)
    {
        // normal ascii, encoding 0xxxxxxx
        utf8Len = 1;
    }
    else if (codepoint < 0x800)
    {
        // two bytes, encoding 110xxxxx 10xxxxxx
        utf8Len = 2;
        firstByteMark = 0xC0; // 11000000
    }
    else if (codepoint < 0x10000)
    {
        // three bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx
        utf8Len = 3;
        firstByteMark = 0xE0; // 11100000
    }
    else if (codepoint <= 0x10FFFF)
    {
        // four bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx 10xxxxxx
        utf8Len = 4;
        firstByteMark = 0xF0; // 11110000
    }
    else
    {
        LE_ERROR("UTF16 invalid codepoint");
        goto fail;
    }

    // ensure output buffer size is enough
    if (*outputLen < utf8Len)
    {
        goto fail;
    }

    // encode as utf8
    for (utf8Pos = (utf8Len - 1); utf8Pos > 0; utf8Pos--)
    {
        // 10xxxxxx
        (*outputPtr)[utf8Pos] = ((codepoint | 0x80) & 0xBF);
        codepoint >>= 6;
    }
    // encode first byte
    if (utf8Len > 1)
    {
        (*outputPtr)[0] = ((codepoint | firstByteMark) & 0xFF);
    }
    else
    {
        (*outputPtr)[0] = (codepoint & 0x7F);
    }

    *outputPtr += utf8Len;
    *outputLen -= utf8Len;

    return sequenceLen;

fail:
    return 0;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function returns the number of characters in string.
 *
 * UTF-8 encoded characters may be larger than 1 byte so the number of characters is not necessarily
 * equal to the the number of bytes in the string.
 *
 * @return
 *      - Number of characters in string if successful.
 *      - LE_FORMAT_ERROR if the string is not UTF-8.
 */
//--------------------------------------------------------------------------------------------------
ssize_t le_utf8_NumChars
(
    const char* string      ///< [IN] Pointer to the string.
)
{
    size_t i;
    size_t numBytes;
    size_t strIndex = 0;
    size_t numChars = 0;

    // Check parameters.
    if (string == NULL)
    {
        return 0;
    }

    while (string[strIndex] != '\0')
    {
        numBytes = le_utf8_NumBytesInChar(string[strIndex]);

        if (numBytes == 0)
        {
            return LE_FORMAT_ERROR;
        }

        // Go through the bytes in this character to make sure all bytes are formatted correctly.
        for (i = 1; i < numBytes; i++)
        {
            if ( !le_utf8_IsContinuationByte(string[++strIndex]) )
            {
                return LE_FORMAT_ERROR;
            }
        }

        // This character is correct.
        numChars++;

        // Move on.
        strIndex++;
    }

    return numChars;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function returns the number of bytes in string (not including the null-terminator).
 *
 * @return
 *      The number of bytes in string (not including the null-terminator).
 */
//--------------------------------------------------------------------------------------------------
size_t le_utf8_NumBytes
(
    const char* string      ///< [IN] The string.
)
{
    // Check parameters.
    if (string == NULL)
    {
        return 0;
    }

    return strlen(string);
}


//--------------------------------------------------------------------------------------------------
/**
 * Returns the number of bytes in the character that starts with a given byte.
 *
 * @return
 *      Number of bytes in the character, or 0 if the byte provided is not a valid starting byte.
 */
//--------------------------------------------------------------------------------------------------
size_t le_utf8_NumBytesInChar
(
    const char firstByte    ///< [IN] The first byte in the character.
)
//--------------------------------------------------------------------------------------------------
{
    if ( IS_SINGLE_BYTE_CHAR(firstByte) )
    {
        return 1;
    }
    else if ( IS_TWO_BYTE_CHAR(firstByte) )
    {
        return 2;
    }
    else if ( IS_THREE_BYTE_CHAR(firstByte) )
    {
        return 3;
    }
    else if ( IS_FOUR_BYTE_CHAR(firstByte) )
    {
        return 4;
    }
    else
    {
        return 0;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function copies the string in srcStr to the start of destStr and returns the number of bytes
 * copied (not including the NULL-terminator) in numBytesPtr.  Null can be passed into numBytesPtr
 * if the number of bytes copied is not needed.  The srcStr must be in UTF-8 format.
 *
 * If the size of srcStr is less than or equal to the destination buffer size then the entire srcStr
 * will be copied including the null-character.  The rest of the destination buffer is not modified.
 *
 * If the size of srcStr is larger than the destination buffer then the maximum number of characters
 * (from srcStr) plus a null-character that will fit in the destination buffer is copied.
 *
 * UTF-8 characters may be more than one byte long and this function will only copy whole characters
 * not partial characters.  Therefore, even if srcStr is larger than the destination buffer the
 * copied characters may not fill the entire destination buffer because the last character copied
 * may not align exactly with the end of the destination buffer.
 *
 * The destination string will always be Null-terminated, unless destSize is zero.
 *
 * If destStr and srcStr overlap the behaviour of this function is undefined.
 *
 * @return
 *      - LE_OK if srcStr was completely copied to the destStr.
 *      - LE_OVERFLOW if srcStr was truncated when it was copied to destStr.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_utf8_Copy
(
    char* destStr,          ///< [IN] The destination where the srcStr is to be copied.
    const char* srcStr,     ///< [IN] The UTF-8 source string.
    const size_t destSize,  ///< [IN] Size of the destination buffer in bytes.
    size_t* numBytesPtr     ///< [OUT] The number of bytes copied not including the NULL-terminator.
                            ///        This parameter can be set to NULL if the number of bytes
                            ///        copied is not needed.
)
{
    // Check parameters.
    LE_ASSERT(destStr != NULL);
    LE_ASSERT(srcStr != NULL);
    LE_ASSERT(destSize > 0);

    // Go through the string copying one character at a time.
    size_t i = 0;
    while (1)
    {
        if (srcStr[i] == '\0')
        {
            // NULL character found.  Complete the copy and return.
            destStr[i] = '\0';

            if (numBytesPtr)
            {
                *numBytesPtr = i;
            }

            return LE_OK;
        }
        else
        {
            size_t charLength = le_utf8_NumBytesInChar(srcStr[i]);

            if (charLength == 0)
            {
                // This is an error in the string format.  Zero out the destStr and return.
                destStr[0] = '\0';

                if (numBytesPtr)
                {
                    *numBytesPtr = 0;
                }

                return LE_OK;
            }
            else if (charLength + i >= destSize)
            {
                // This character will not fit in the available space so stop.
                destStr[i] = '\0';

                if (numBytesPtr)
                {
                    *numBytesPtr = i;
                }

                return LE_OVERFLOW;
            }
            else
            {
                // Copy the character.
                for (; charLength > 0; charLength--)
                {
                    destStr[i] = srcStr[i];
                    i++;
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Appends srcStr to destStr by copying characters from srcStr to the end of destStr.  The srcStr
 * must be in UTF-8 format.  The number of bytes in the resultant destStr (not including the
 * NULL-terminator) is returned in destStrLenPtr.
 *
 * A null-character is always added to the end of destStr after all srcStr characters have been
 * copied.
 *
 * This function will copy as many characters as possible from srcStr to destStr while ensuring that
 * the resultant string (including the null-character) will fit within the destination buffer.
 *
 * UTF-8 characters may be more than one byte long and this function will only copy whole characters
 * not partial characters.
 *
 * The destination string will always be Null-terminated, unless destSize is zero.
 *
 * If destStr and srcStr overlap the behaviour of this function is undefined.
 *
 * @return
 *      - LE_OK if srcStr was completely copied to the destStr.
 *      - LE_OVERFLOW if srcStr was truncated when it was copied to destStr.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_utf8_Append
(
    char* destStr,          ///< [IN] The destination string.
    const char* srcStr,     ///< [IN] The UTF-8 source string.
    const size_t destSize,  ///< [IN] Size of the destination buffer in bytes.
    size_t* destStrLenPtr   ///< [OUT] The number of bytes in the resultant destination string (not
                            ///        including the NULL-terminator).  This parameter can be set to
                            ///        NULL if the destination string size is not needed.
)
{
    // Check parameters.
    LE_ASSERT( (destStr != NULL) && (srcStr != NULL) && (destSize > 0) );

    size_t destStrSize = strlen(destStr);
    le_result_t result = le_utf8_Copy(&(destStr[destStrSize]),
                                      srcStr,
                                      destSize - destStrSize,
                                      destStrLenPtr);

    if (destStrLenPtr)
    {
        *destStrLenPtr += destStrSize;
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Copies all characters from the srcStr to destStr up to the first occurrence of subStr.  The
 * subStr is not copied and instead a null-terminator is added to the destStr.  The number of bytes
 * copied (not including the null-terminator) is returned in numBytesPtr.
 *
 * The srcStr and subStr must be in null-terminated UTF-8 strings.
 *
 * The destination string will always be null-terminated.
 *
 * If subStr is not found in the srcStr then this function behaves just like le_utf8_Copy().
 *
 * @return
 *      - LE_OK if srcStr was completely copied to the destStr.
 *      - LE_OVERFLOW if srcStr was truncated when it was copied to destStr.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_utf8_CopyUpToSubStr
(
    char* destStr,          ///< [IN] The destination where the srcStr is to be copied.
    const char* srcStr,     ///< [IN] The UTF-8 source string.
    const char* subStr,     ///< [IN] The sub-string to copy up to.
    const size_t destSize,  ///< [IN] Size of the destination buffer in bytes.
    size_t* numBytesPtr     ///< [OUT] The number of bytes copied not including the NULL-terminator.
                            ///        This parameter can be set to NULL if the number of bytes
                            ///        copied is not needed.
)
{
    // Check parameters.
    LE_ASSERT( (destStr != NULL) && (srcStr != NULL) && (destSize > 0) && (subStr != NULL) );

    int_fast8_t subStrLen = le_utf8_NumBytes(subStr);

    // Go through the string copying one character at a time.
    size_t i = 0;
    while (1)
    {
        if (srcStr[i] == '\0')
        {
            // NULL character found.  Complete the copy and return.
            destStr[i] = '\0';

            if (numBytesPtr)
            {
                *numBytesPtr = i;
            }

            return LE_OK;
        }
        else
        {
            size_t charLength = le_utf8_NumBytesInChar(srcStr[i]);

            if (charLength == 0)
            {
                // This is an error in the string format.  Zero out the destStr and return.
                destStr[0] = '\0';

                if (numBytesPtr)
                {
                    *numBytesPtr = 0;
                }

                return LE_OK;
            }
            else if (memcmp(srcStr + i, subStr, subStrLen) == 0)
            {
                // Note: Do this check before the overflow check so that we do not get false
                // positives for overflow.

                // Found character.  Complete the copy and return.
                destStr[i] = '\0';

                if (numBytesPtr)
                {
                    *numBytesPtr = i;
                }

                return LE_OK;
            }
            else if (charLength + i >= destSize)
            {
                // This character will not fit in the available space so stop.
                destStr[i] = '\0';

                if (numBytesPtr)
                {
                    *numBytesPtr = i;
                }

                return LE_OVERFLOW;
            }
            else
            {
                // Copy the character.
                for (; charLength > 0; charLength--)
                {
                    destStr[i] = srcStr[i];
                    i++;
                }
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Checks to see if the string is indeed a UTF-8 encoded, null-terminated string.
 *
 * @return
 *      true if the format is correct or false otherwise
 */
//--------------------------------------------------------------------------------------------------
bool le_utf8_IsFormatCorrect
(
    const char* string      ///< [IN] The string.
)
{
    size_t i;
    size_t numBytes = 0;
    size_t strIndex = 0;

    // Check parameters.
    if (string == NULL)
    {
        return false;
    }

    while (string[strIndex] != '\0')
    {
        numBytes = le_utf8_NumBytesInChar(string[strIndex]);

        if (numBytes == 0)
        {
            return false;
        }

        // Go through the bytes in this character to make sure all bytes are formatted correctly.
        for (i = 1; i < numBytes; i++)
        {
            if ( !le_utf8_IsContinuationByte(string[++strIndex]) )
            {
                return false;
            }
        }

        // Move on.
        strIndex++;
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Parse an integer value from a string.
 *
 * @return
 *      - LE_OK = Success.
 *      - LE_FORMAT_ERROR = The argument string was not an integer value.
 *      - LE_OUT_OF_RANGE = Value is too large to be stored in an int variable.
 **/
//--------------------------------------------------------------------------------------------------
le_result_t le_utf8_ParseInt
(
    int* valuePtr,  ///< [OUT] Ptr to where the value will be stored if successful.
    const char* arg ///< [IN] The string to parse.
)
//--------------------------------------------------------------------------------------------------
{
    char* endPtr;
    long int value;

    errno = 0;
    value = strtol(arg, &endPtr, 0);

    // strtol() sets errno to ERANGE if the magnitude is greater than a long int can hold.
    if (errno == ERANGE)
    {
        return LE_OUT_OF_RANGE;
    }

    // strtol() sets the endPtr to the same as its first argument if no characters were valid.
    // Otherwise, it sets endPtr to point to the first character than is invalid, which should be
    // the null terminator if the whole string contained valid characters.
    if ((endPtr == arg) || (*endPtr != '\0'))
    {
        return LE_FORMAT_ERROR;
    }

    // Copy the long int value into the int result variable.
    *valuePtr = (int)value;

    // Check for overflow/underflow.
    if (((long int)(*valuePtr)) != value)
    {
        return LE_OUT_OF_RANGE;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a unicode code point as UTF-8 into a buffer.
 *
 * @return
 *      - LE_OK on success
 *      - LE_OUT_OF_RANGE if the code point supplied is outside the range of unicode code points
 *      - LE_OVERFLOW if the out buffer is not large enough to store the UTF-8 encoding of the code
 *        point
 *
 * @note
 *      Not all code point values are valid unicode. This function does not validate whether the
 *      code point is valid unicode.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_utf8_EncodeUnicodeCodePoint
(
    uint32_t codePoint, ///< [IN] Code point to encode as UTF-8.
    char* out,          ///< [OUT] Buffer to store the UTF-8 encoded value in.
    size_t* outSize     ///< [IN/OUT] As an input, this value is interpreted as the size of the out
                        ///  buffer.  As an output, it is updated to hold the size of the UTF-8
                        ///  encoded value (in the case of an LE_OK return value) or size that would
                        ///  be required to encode the code point (in the case or an LE_OVERFLOW
                        ///  return value).
)
{
    const size_t bufferSize = *outSize;
    if (codePoint <= 0x00007F)
    {
        *outSize = 1;
        if (bufferSize >= 1)
        {
            out[0] = codePoint;
        }
        else
        {
            return LE_OVERFLOW;
        }
    }
    else if (codePoint <= 0x0007FF)
    {
        *outSize = 2;
        if (bufferSize >= 2)
        {
            out[0] = (0xC0 | ((codePoint >>  6) & 0x1F));
            out[1] = (0x80 | ((codePoint >>  0) & 0x3F));
        }
        else
        {
            return LE_OVERFLOW;
        }
    }
    else if (codePoint <= 0x00FFFF)
    {
        *outSize = 3;
        if (bufferSize >= 3)
        {
            out[0] = (0xE0 | ((codePoint >> 12) & 0x0F));
            out[1] = (0x80 | ((codePoint >>  6) & 0x3F));
            out[2] = (0x80 | ((codePoint >>  0) & 0x3F));
        }
        else
        {
            return LE_OVERFLOW;
        }
    }
    else if (codePoint <= 0x10FFFF)
    {
        *outSize = 4;
        if (bufferSize >= 4)
        {
            out[0] = (0xF0 | ((codePoint >> 18) & 0x07));
            out[1] = (0x80 | ((codePoint >> 12) & 0x3F));
            out[2] = (0x80 | ((codePoint >>  6) & 0x3F));
            out[3] = (0x80 | ((codePoint >>  0) & 0x3F));
        }
        else
        {
            return LE_OVERFLOW;
        }
    }
    else
    {
        return LE_OUT_OF_RANGE;
    }

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode the first unicode code point from the UTF-8 string src.
 *
 * @return
 *      - LE_OK on success
 *      - LE_BAD_PARAMETER if byteLength points to 0
 *      - LE_UNDERFLOW if src appears to be the beginning of a UTF-8 character which extends beyond
 *        the end of the string as specified by byteLength.
 *      - LE_FORMAT_ERROR if src is not valid UTF-8 encoded string data.
 *      - LE_OUT_OF_RANGE if src encodes a four byte character whose value is greater than what is
 *        permitted in UTF-8.
 *
 * @note
 *      Not all code point values are valid unicode. This function does not validate whether the
 *      code point is valid unicode.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_utf8_DecodeUnicodeCodePoint
(
    const char* src,     ///< [IN] UTF-8 encoded data to extract a code point from.
    size_t* byteLength,  ///< [IN/OUT] As an input parameter, the value pointed to represents the
                         ///  number of bytes in src. As an output parameter, the value pointed to
                         ///  is the number of bytes from src that were consumed to decode the code
                         ///  point (in the case of an LE_OK return value) or the number of bytes
                         ///  that would have been consumed had src been long enough (in the case of
                         ///  an LE_UNDERFLOW return value).
    uint32_t* codePoint  ///< [OUT] Code point that was decoded from src.  This value is only valid
                         ///  when the function returns LE_OK.
)
{
    const size_t bufferSize = *byteLength;
    // Trying to decode a zero-length buffer is invalid
    if (bufferSize == 0)
    {
        return LE_BAD_PARAMETER;
    }

    if (IS_SINGLE_BYTE_CHAR(src[0]))
    {
        *byteLength = 1;
        *codePoint = src[0];
    }
    else if (IS_TWO_BYTE_CHAR(src[0]))
    {
        *byteLength = 2;
        if (bufferSize < 2)
        {
            return LE_UNDERFLOW;
        }

        if (!le_utf8_IsContinuationByte(src[1]))
        {
            return LE_FORMAT_ERROR;
        }

        *codePoint = (((src[0] & 0x1F) << 6) | ((src[1] & 0x3F) << 0));
        if (*codePoint <= 0x7F)
        {
            // overlong encoding is invalid
            return LE_FORMAT_ERROR;
        }
    }
    else if (IS_THREE_BYTE_CHAR(src[0]))
    {
        *byteLength = 3;
        if (bufferSize < 3)
        {
            return LE_UNDERFLOW;
        }

        if (!le_utf8_IsContinuationByte(src[1]) || !le_utf8_IsContinuationByte(src[2]))
        {
            return LE_FORMAT_ERROR;
        }

        *codePoint = (((src[0] & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | ((src[2] & 0x3F) << 0));
        if (*codePoint <= 0x7FF)
        {
            // overlong encoding is invalid
            return LE_FORMAT_ERROR;
        }
    }
    else if (IS_FOUR_BYTE_CHAR(src[0]))
    {
        *byteLength = 4;
        if (bufferSize < 4)
        {
            return LE_UNDERFLOW;
        }

        if (!le_utf8_IsContinuationByte(src[1]) ||
            !le_utf8_IsContinuationByte(src[2]) ||
            !le_utf8_IsContinuationByte(src[3]))
        {
            return LE_FORMAT_ERROR;
        }

        *codePoint = (
            ((src[0] & 0x07) << 18) |
            ((src[1] & 0x3F) << 12) |
            ((src[2] & 0x3F) << 6) |
            ((src[3] & 0x3F) << 0));
        if (*codePoint <= 0xFFFF)
        {
            // overlong encoding is invalid
            return LE_FORMAT_ERROR;
        }

        if (*codePoint > 0x10FFFF)
        {
            return LE_OUT_OF_RANGE;
        }
    }
    else
    {
        // src is not a valid UTF-8 character
        return LE_FORMAT_ERROR;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Creates an unescaped version of a string into a provided buffer
 * Buffer receiving the unescaped version must be large enough to contain the unescaped
 * output.
 * The same buffer may be used for input and output.
 *
 * @return
 *      - LE_OK                 on success
 *      - LE_BAD_PARAMETER      if input or output is a NULL pointer
 *      - LE_NOT_IMPLEMENTED    if utility does not support required unescaping type
 *      - LE_NO_MEMORY          if not enough memory available in output buffer
 *      - LE_FAULT              if input string is malformed escaping wise
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_utf8_StringUnescape
(
    const char* input,              ///< [IN]  Null terminated string to unescape
    char* output,                   ///< [OUT] Buffer receiving the unescaped string.
                                    ///<       Must be at least the same size as input
    size_t outputLen                ///< [IN]  Length of input and output strings, in bytes
)
{
    if (NULL == input || NULL == output)
    {
        LE_ERROR("Expect valid pointers for input/output (got NULL pointer)");
        return LE_BAD_PARAMETER;
    }

    const size_t inputLen = strlen(input);
    const char * inputPtr = input;
    const char * inputEnd = inputPtr + inputLen;
    char * outputPtr = output;

    while (*inputPtr)
    {
        // Make sure still have space in output buffer
        if (outputLen < 1)
        {
            return LE_NO_MEMORY;
        }

        // No escape to perform: basic copy of the character
        if (*inputPtr != '\\')
        {
            *outputPtr++ = *inputPtr++;
            outputLen--;
        }
        // Escaped sequence: perform adequate escaping
        else
        {
            uint8_t sequenceLen = 2;
            // If input string ends with a '\': it is malformed
            if ((inputEnd - inputPtr) <= 1)
            {
                return LE_FAULT;
            }

            switch (inputPtr[1])
            {
                case 'b':
                    *outputPtr++ = '\b';
                    outputLen--;
                    break;
                case 'f':
                    *outputPtr++ = '\f';
                    outputLen--;
                    break;
                case 'n':
                    *outputPtr++ = '\n';
                    outputLen--;
                    break;
                case 'r':
                    *outputPtr++ = '\r';
                    outputLen--;
                    break;
                case 't':
                    *outputPtr++ = '\t';
                    outputLen--;
                    break;
                case '\"':
                case '\\':
                case '/':
                    *outputPtr++ = inputPtr[1];
                    outputLen--;
                    break;

                case 'u':
                    sequenceLen = Utf16LiteralToUtf8(inputPtr,
                                                     inputEnd,
                                                     &outputPtr,
                                                     &outputLen
                                                     );
                    if (sequenceLen == 0)
                    {
                        // UTF16 to UTF8 conversion failed: malformed input
                        return LE_FAULT;
                    }
                    break;

                case 'x':
                {
                    // Hex escape format is fixed length of 4: "\xXX"
                    if ((inputEnd - inputPtr) < 4)
                    {
                        return LE_FAULT;
                    }
                    // Input must be two hex characters and result must be one binary byte
                    if (1 != le_hex_StringToBinary(inputPtr + 2,
                                                   2,
                                                   (uint8_t *)outputPtr, 1))
                    {
                        LE_ERROR("Failed to convert %s", inputPtr);
                        sequenceLen = 0;
                        return LE_FAULT;
                    }
                    sequenceLen = 4;
                    outputPtr++;
                    outputLen--;
                    break;
                }

                default:
                    // No implemented escpaing matches input: not a malformed error
                    LE_ERROR("Escape \\%c not implemented", inputPtr[1]);
                    return LE_NOT_IMPLEMENTED;
            }
            inputPtr += sequenceLen;
        }
    }

    *outputPtr = '\0';
    return LE_OK;
}
