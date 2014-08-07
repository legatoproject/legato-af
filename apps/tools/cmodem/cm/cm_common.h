
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

#ifndef CMODEM_COMMON_INCLUDE_GUARD
#define CMODEM_COMMON_INCLUDE_GUARD


// -------------------------------------------------------------------------------------------------
/**
*  Prints a data item and its description to stdout according to the tool's standard output format.
*/
// -------------------------------------------------------------------------------------------------
void cm_cmn_FormatPrint
(
    const char * data,    ///< [IN] Pointer to the data string to be printed
    const char * desc     ///< [IN] Pointer to the description string to be printed
);


// -------------------------------------------------------------------------------------------------
/**
*  Convert characters to all lower cases.
*/
// -------------------------------------------------------------------------------------------------
void cm_cmn_toLower
(
    const char * data, ///< [IN] Pointer to the data string that we want converted to lower case
    char * dataToLower,
    size_t len
);


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
);

#endif

