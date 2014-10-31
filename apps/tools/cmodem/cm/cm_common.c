
// -------------------------------------------------------------------------------------------------
/**
 *  @file cm_common.c
 *
 *  Common functions between components.
 *
 *  Copyright (C) Sierra Wireless, Inc. 2013, 2014. All rights reserved. Use of this work is subject
 *  to license.
 */
// -------------------------------------------------------------------------------------------------

#include "legato.h"
#include "cm_common.h"


// -------------------------------------------------------------------------------------------------
/**
*  Prints a data item and its description to stdout according to the tool's standard output format.
*/
// -------------------------------------------------------------------------------------------------
void cm_cmn_FormatPrint
(
    const char * data,    ///< [IN] Pointer to the data string to be printed
    const char * desc     ///< [IN] Pointer to the description string to be printed
)
{
    char sysInfoColon[30];
    snprintf(sysInfoColon, sizeof(sysInfoColon), "%s:", data);
    printf("%-10s %s\n", sysInfoColon, desc);
}


// -------------------------------------------------------------------------------------------------
/**
*  Convert characters to all lower cases.
*/
// -------------------------------------------------------------------------------------------------
void cm_cmn_toLower
(
    const char * data, ///< [IN] Pointer to the data string that we want converted to lower case
    char * dataToLower, ///< [OUT] Data string converted to lower case
    size_t len ///< [IN] Length of buffer
)
{
    int i = 0;
    le_result_t res;

    res = le_utf8_Copy(dataToLower, data, len, NULL);

    if (res != LE_OK)
    {
        printf("Unable to convert to lowercase.\n");
    }

    while (data[i])
    {
        dataToLower[i] = tolower(data[i]);
        i++;
    }
}


// -------------------------------------------------------------------------------------------------
/**
*  Convert characters to all upper cases.
*/
// -------------------------------------------------------------------------------------------------
void cm_cmn_toUpper
(
    const char * data, ///< [IN] Pointer to the data string that we want converted to upper case
    char * dataToUpper, ///< [OUT] Data string converted to upper case
    size_t len ///< [IN] Length of buffer
)
{
    int i = 0;
    le_result_t res;

    res = le_utf8_Copy(dataToUpper, data, len, NULL);

    if (res != LE_OK)
    {
        printf("Unable to convert to uppercase.\n");
    }

    while (data[i])
    {
        dataToUpper[i] = toupper(data[i]);
        i++;
    }
}
