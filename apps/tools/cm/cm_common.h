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

#ifndef CMODEM_COMMON_INCLUDE_GUARD
#define CMODEM_COMMON_INCLUDE_GUARD

#define CMODEM_COMMON_SERVICE_STR_LEN       10
#define CMODEM_COMMON_COMMAND_STR_LEN       25

#define CMODEM_COMMON_RAT_STR_LEN           10
#define CMODEM_COMMON_NETWORK_STR_LEN       25

#define CMODEM_COMMON_PDP_STR_LEN           10
#define CMODEM_COMMON_AUTH_STR_LEN          10
#define CMODEM_COMMON_PROFILE_IDX_STR_LEN   5
#define CMODEM_COMMON_TIMEOUT_STR_LEN       5

#define CMODEM_COMMON_COLUMN_LEN            30


//-------------------------------------------------------------------------------------------------
/**
 * Prints a data item and its description to stdout according to the tool's standard output format.
 */
//-------------------------------------------------------------------------------------------------
void cm_cmn_FormatPrint
(
    const char * data,    ///< [IN] Pointer to the data string to be printed
    const char * desc     ///< [IN] Pointer to the description string to be printed
);


//-------------------------------------------------------------------------------------------------
/**
 * Convert characters to all lower cases.
 */
//-------------------------------------------------------------------------------------------------
void cm_cmn_ToLower
(
    const char * data, ///< [IN] Pointer to the data string that we want converted to lower case
    char * dataToLower,
    size_t len
);


//-------------------------------------------------------------------------------------------------
/**
 * Convert characters to all upper cases.
 */
//-------------------------------------------------------------------------------------------------
void cm_cmn_ToUpper
(
    const char * data,  ///< [IN] Pointer to the data string that we want converted to upper case
    char * dataToUpper, ///< [OUT] Data string converted to upper case
    size_t len          ///< [IN] Length of buffer
);

//--------------------------------------------------------------------------------------------------
/**
 * Verify if enough parameter passed into command. If not, output error message to stdout.
 */
//--------------------------------------------------------------------------------------------------
bool cm_cmn_CheckEnoughParams
(
    size_t requiredParams,  ///< [IN] Required parameters for the command
    size_t numArgs,         ///< [IN] Number of arguments passed into the command line
    const char * errorMsg   ///< [IN] Error message to output if not enough parameters
);

//--------------------------------------------------------------------------------------------------
/**
 * Verify if enough parameter passed into command. If not, output error message to stderr and exit.
 */
//--------------------------------------------------------------------------------------------------
void cm_cmn_CheckNumberParams
(
    size_t requiredParams,      ///< [IN] Required parameters for the command
    size_t maxParams,           ///< [IN] Max number of parameters for the command
                                ///       Optional included, use -1 to disable check.
    size_t numArgs,             ///< [IN] Number of arguments passed into the command line
    const char * errorMsg       ///< [IN] Error message to output if not enough parameters
);

//-------------------------------------------------------------------------------------------------
/**
 * Function prototype to provide help usage about a service.
 */
//-------------------------------------------------------------------------------------------------
typedef void (*cm_ServiceHelpHandler_t)();

//-------------------------------------------------------------------------------------------------
/**
 * Function prototype to execute a command for a specific service.
 */
//-------------------------------------------------------------------------------------------------
typedef void (*cm_ServiceCommandHandler_t)
(
    const char * commandPtr, ///< [IN] Command
    size_t numArgs           ///< [IN] Number of arguments
);

//-------------------------------------------------------------------------------------------------
/**
 * Structure that contains information about a service instance.
 */
//-------------------------------------------------------------------------------------------------
typedef struct {
    const char * serviceNamePtr;
    const char * defaultCommandPtr;

    cm_ServiceHelpHandler_t    helpHandler;
    cm_ServiceCommandHandler_t commandHandler;
}
cm_Service_t;

#endif

