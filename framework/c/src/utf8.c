//--------------------------------------------------------------------------------------------------
/** @file utf8.c
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"


//--------------------------------------------------------------------------------------------------
// Local definitions.
//--------------------------------------------------------------------------------------------------
// Checks that there is a high-order '10'.
#define IS_CONTINUATION_BYTE(cByte)             ( (cByte & 0xC0) == 0x80 )

// Checks the size of the char by looking at the lead byte.
#define IS_SINGLE_BYTE_CHAR(leadByte)           ( (leadByte & 0x80) == 0x00 )
#define IS_TWO_BYTE_CHAR(leadByte)              ( (leadByte & 0xE0) == 0xC0 )
#define IS_THREE_BYTE_CHAR(leadByte)            ( (leadByte & 0xF0) == 0xE0 )
#define IS_FOUR_BYTE_CHAR(leadByte)             ( (leadByte & 0xF8) == 0xF0 )


//--------------------------------------------------------------------------------------------------
/**
 * Looks at the lead byte of a UTF-8 character and determines the number of bytes in
 * the character including the lead character.
 *
 * @return
 *      The number of bytes if succesful.
 *      LE_FORMAT_ERROR if the formatting of the first byte is incorrect.
 */
//--------------------------------------------------------------------------------------------------
static inline int_fast8_t NumBytesInChar(char leadByte)
{
    if ( IS_SINGLE_BYTE_CHAR(leadByte) )
    {
        return 1;
    }
    else if ( IS_TWO_BYTE_CHAR(leadByte) )
    {
        return 2;
    }
    else if ( IS_THREE_BYTE_CHAR(leadByte) )
    {
        return 3;
    }
    else if ( IS_FOUR_BYTE_CHAR(leadByte) )
    {
        return 4;
    }
    else
    {
        return LE_FORMAT_ERROR;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function returns the number of characters in string.
 *
 * UTF-8 encoded characters may be larger than 1 byte so the number of characters is not necessarily
 * equal to the the number of bytes in the string.
 *
 * @return
 *      The number of characters in string if successful.
 *      LE_FORMAT_ERROR if the string is not UTF-8.
 */
//--------------------------------------------------------------------------------------------------
ssize_t le_utf8_NumChars
(
    const char* string      ///< [IN] Pointer to the string.
)
{
    uint_fast8_t i;
    int_fast8_t numBytes = 0;
    size_t strIndex = 0;
    size_t numChars = 0;

    // Check parameters.
    if (string == NULL)
    {
        return 0;
    }

    while (string[strIndex] != '\0')
    {
        numBytes = NumBytesInChar(string[strIndex]);

        if (numBytes < 0)
        {
            return LE_FORMAT_ERROR;
        }

        // Go through the bytes in this character to make sure all bytes are formatted correctly.
        for (i = 1; i < numBytes; i++)
        {
            if ( !IS_CONTINUATION_BYTE(string[++strIndex]) )
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
 *      LE_OK if srcStr was completely copied to the destStr.
 *      LE_OVERFLOW if srcStr was truncated when it was copied to destStr.
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
    LE_ASSERT( (destStr != NULL) && (srcStr != NULL) && (destSize > 0) );

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
            int_fast8_t charLength = NumBytesInChar(srcStr[i]);

            if (charLength < 0)
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
 * This function appends srcStr to destStr by copying characters from srcStr to the end of destStr.
 * The srcStr must be in UTF-8 format.  The number of bytes in the resultant destStr (not including
 * the NULL-terminator) is returned in destStrLenPtr.
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
 *      LE_OK if srcStr was completely copied to the destStr.
 *      LE_OVERFLOW if srcStr was truncated when it was copied to destStr.
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
 * This function copies all characters from the srcStr to destStr up to the first occurrence of
 * subStr.  The subStr is not copied and instead a null-terminator is added to the destStr.
 * The number of bytes copied (not including the null-terminator) is returned in numBytesPtr.
 *
 * The srcStr and subStr must be in null-terminated UTF-8 strings.
 *
 * The destination string will always be null-terminated.
 *
 * If subStr is not found in the srcStr then this function behaves just like le_utf8_Copy().
 *
 * @return
 *      LE_OK if srcStr was completely copied to the destStr.
 *      LE_OVERFLOW if srcStr was truncated when it was copied to destStr.
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
            int_fast8_t charLength = NumBytesInChar(srcStr[i]);

            if (charLength < 0)
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
 * This function checks to see if the string is indeed a UTF-8 encoded, null-terminated string.
 *
 * @return
 *      true if the format is correct.
 *      false if the format is incorrect.
 */
//--------------------------------------------------------------------------------------------------
bool le_utf8_IsFormatCorrect
(
    const char* string      ///< [IN] The string.
)
{
    uint8_t i;
    int8_t numBytes = 0;
    size_t strIndex = 0;

    // Check parameters.
    if (string == NULL)
    {
        return false;
    }

    while (string[strIndex] != '\0')
    {
        numBytes = NumBytesInChar(string[strIndex]);

        if (numBytes < 0)
        {
            return false;
        }

        // Go through the bytes in this character to make sure all bytes are formatted correctly.
        for (i = 1; i < numBytes; i++)
        {
            if ( !IS_CONTINUATION_BYTE(string[++strIndex]) )
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
 *  - LE_OK = Success.
 *  - LE_FORMAT_ERROR = The argument string was not an integer value.
 *  - LE_OUT_OF_RANGE = Value is too large to be stored in an int variable.
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
    // Otherwise, it sets endPtr to point to the first character than is invalid, which should
    // be the null terminator if the whole string contained valid characters.
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
