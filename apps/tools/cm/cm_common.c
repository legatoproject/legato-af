//-------------------------------------------------------------------------------------------------
/**
 * @file cm_common.c
 *
 * Common functions between components.
 *
 * Copyright (C) Sierra Wireless Inc.
 * to license.
 */
//-------------------------------------------------------------------------------------------------

#include "legato.h"
#include "cm_common.h"


//-------------------------------------------------------------------------------------------------
/**
 * Prints a data item and its description to stdout according to the tool's standard output format.
 */
//-------------------------------------------------------------------------------------------------
void cm_cmn_FormatPrint
(
    const char * data,    ///< [IN] Pointer to the data string to be printed
    const char * desc     ///< [IN] Pointer to the description string to be printed
)
{
    char sysInfoColon[CMODEM_COMMON_COLUMN_LEN];
    snprintf(sysInfoColon, sizeof(sysInfoColon), "%s:", data);
    printf("%-"STRINGIZE(CMODEM_COMMON_COLUMN_LEN)"s %s\n", sysInfoColon, desc);
}


//-------------------------------------------------------------------------------------------------
/**
 * Convert characters to all lower cases.
 */
//-------------------------------------------------------------------------------------------------
void cm_cmn_ToLower
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


//-------------------------------------------------------------------------------------------------
/**
 * Convert characters to all upper cases.
 */
//-------------------------------------------------------------------------------------------------
void cm_cmn_ToUpper
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

//--------------------------------------------------------------------------------------------------
/**
 * Verify if enough parameter passed into command.
 * If not, output error message to stdout and terminate the program.
 */
//--------------------------------------------------------------------------------------------------
bool cm_cmn_CheckEnoughParams
(
    size_t requiredParams,  ///< [IN] Required parameters for the command
    size_t numArgs,         ///< [IN] Number of arguments passed into the command line
    const char * errorMsg   ///< [IN] Error message to output if not enough parameters
)
{
    if ( (requiredParams + 1) < numArgs)
    {
        return true;
    }
    else
    {
        printf("%s\n\n", errorMsg);
        exit(EXIT_FAILURE);
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * Verify if enough parameter passed into command. If not, output error message to stderr and exit.
 */
//--------------------------------------------------------------------------------------------------
void cm_cmn_CheckNumberParams
(
    size_t requiredParams,      ///< [IN] Required parameters for the command
    size_t maxParams,           ///< [IN] Max number of parameters allowed for the command
                                ///       Optional included. Use -1 to disable check.
    size_t numArgs,             ///< [IN] Number of arguments passed into the command line
    const char * errorMsg       ///< [IN] Optional, error message to output if not enough parameters
)
{
    int numParams = numArgs;

    // Remove the service & command arguments
    numParams -= 2;
    if (numParams < 0)
    {
        numParams = 0;
    }

    // Check for required parameters
    if (numParams < requiredParams)
    {
        if (errorMsg != NULL)
        {
            fprintf(stderr, "%s\n\n", errorMsg);
        }
        else
        {
            fprintf(stderr, "Not enough parameters.\n\n");
        }
        exit(EXIT_FAILURE);
    }

    // Check for maximum number of parameters allowed
    if ( (maxParams != -1) && (numParams > maxParams))
    {
        fprintf(stderr, "Too many parameters.\n\n");
        exit(EXIT_FAILURE);
    }
}
