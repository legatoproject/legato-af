/** @file le_atServer.c
 *
 * Implementation of AT commands server API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_dev.h"
#include "bridge.h"
#include "le_atServer_local.h"

//--------------------------------------------------------------------------------------------------
/**
 * Max length for error string
 */
//--------------------------------------------------------------------------------------------------
#define ERR_MSG_MAX 256

//--------------------------------------------------------------------------------------------------
/**
 * Device pool size
 */
//--------------------------------------------------------------------------------------------------
#define DEVICE_POOL_SIZE    2

//--------------------------------------------------------------------------------------------------
/**
 * AT commands pool size
 */
//--------------------------------------------------------------------------------------------------
#define CMD_POOL_SIZE       100

//--------------------------------------------------------------------------------------------------
/**
 * Command parameters pool size
 */
//--------------------------------------------------------------------------------------------------
#define PARAM_POOL_SIZE       20

//--------------------------------------------------------------------------------------------------
/**
 * Command responses pool size
 */
//--------------------------------------------------------------------------------------------------
#define RSP_POOL_SIZE       10

//--------------------------------------------------------------------------------------------------
/**
 * Command responses types
 */
//--------------------------------------------------------------------------------------------------
#define RSP_TYPE_OK         0
#define RSP_TYPE_ERROR      1
#define RSP_TYPE_RESPONSE   2

//--------------------------------------------------------------------------------------------------
/**
 * AT parser tokens
 */
//--------------------------------------------------------------------------------------------------
#define AT_TOKEN_EQUAL  '='
#define AT_TOKEN_CR     0x0D
#define AT_TOKEN_QUESTIONMARK  '?'
#define AT_TOKEN_SEMICOLON ';'
#define AT_TOKEN_COMMA ','

//--------------------------------------------------------------------------------------------------
/**
 * Is character a number ?
 */
//--------------------------------------------------------------------------------------------------
#define IS_NUMBER(X)            ( ( X >= '0' ) && ( X<= '9' ) ) /* [ 0-9 ] */

//--------------------------------------------------------------------------------------------------
/**
 * Is character a quote ?
 */
//--------------------------------------------------------------------------------------------------
#define IS_QUOTE(X)            ( X == '"' ) /* " */

//--------------------------------------------------------------------------------------------------
/**
 * Is character a star or a hash ?
 */
//--------------------------------------------------------------------------------------------------
#define IS_STAR_OR_HASH_SIGN(X) ( (X == '#' ) || (X == '*') )  /* * or # */

//--------------------------------------------------------------------------------------------------
/**
 * Is character between 'A' and 'F' ?
 */
//--------------------------------------------------------------------------------------------------
#define IS_BETWEEN_A_AND_F(X)   ( (X >= 'A' ) && (X <= 'F') )  /* [ A-F ] */

//--------------------------------------------------------------------------------------------------
/**
 * Is character hexa token ?
 */
//--------------------------------------------------------------------------------------------------
#define IS_HEXA(X)              ( (X == 'h' ) || (X == 'H') )  /* h or H */

//--------------------------------------------------------------------------------------------------
/**
 * Is character plus or minus ?
 */
//--------------------------------------------------------------------------------------------------
#define IS_PLUS_OR_MINUS(X)    ( (X == '+' ) || (X == '-') )  /* + or - */

//--------------------------------------------------------------------------------------------------
/**
 * Is character '&' ?
 */
//--------------------------------------------------------------------------------------------------
#define IS_AND(X)               (X == '&')

//--------------------------------------------------------------------------------------------------
/**
 * Is character '\' ?
 */
//--------------------------------------------------------------------------------------------------
#define IS_SLASH(X)            (X == '\\')

//--------------------------------------------------------------------------------------------------
/**
 * Is basic syntax command ?
 */
//--------------------------------------------------------------------------------------------------
#define IS_BASIC(X)           (IS_CHAR(X) || IS_AND(X) || IS_SLASH(X))

//--------------------------------------------------------------------------------------------------
/**
 * Is character expected as a parameter ?
 */
//--------------------------------------------------------------------------------------------------
#define IS_PARAM_CHAR(X)          ( IS_NUMBER(X) || \
                                    IS_STAR_OR_HASH_SIGN (X) || \
                                    IS_PLUS_OR_MINUS(X) || \
                                    IS_HEXA(X) || \
                                    IS_BETWEEN_A_AND_F(X) )

//--------------------------------------------------------------------------------------------------
/**
 * Text prompt definition.
 */
//--------------------------------------------------------------------------------------------------
#define TEXT_PROMPT         "\r\n> "

//--------------------------------------------------------------------------------------------------
/**
 * Text prompt len.
 */
//--------------------------------------------------------------------------------------------------
#define TEXT_PROMPT_LEN     4

//--------------------------------------------------------------------------------------------------
/**
 * ASCII substitute control code.
 */
//--------------------------------------------------------------------------------------------------
#define SUBSTITUTE          0x1a

//--------------------------------------------------------------------------------------------------
/**
 * ASCII escape code.
 */
//--------------------------------------------------------------------------------------------------
#define ESCAPE              0x1b
//--------------------------------------------------------------------------------------------------
/**
 * Pool for device context
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t    DevicesPool;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for AT command
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  AtCommandsPool;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for parameter string
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  ParamStringPool;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for response
 * w string
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  RspStringPool;

//--------------------------------------------------------------------------------------------------
/**
 * Map for devices
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t DevicesRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Map for AT commands
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t   SubscribedCmdRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Map for AT commands
 */
//--------------------------------------------------------------------------------------------------
static le_hashmap_Ref_t   CmdHashMap;

//--------------------------------------------------------------------------------------------------
/**
 * Extended error codes
 */
//--------------------------------------------------------------------------------------------------
static bool ExtendedErrorCodes = false;

//--------------------------------------------------------------------------------------------------
/**
 * structure used to describe a parameter.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char            param[LE_ATDEFS_PARAMETER_MAX_BYTES];   ///< string value
    le_dls_Link_t   link;                                   ///< link for list
}
ParamString_t;

//--------------------------------------------------------------------------------------------------
/**
 * AT command response structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char            resp[LE_ATDEFS_RESPONSE_MAX_BYTES];     ///< string value
    le_dls_Link_t   link;                                   ///< link for list
}
RspString_t;

//--------------------------------------------------------------------------------------------------
/**
 * RX parser state.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PARSER_SEARCH_A,
    PARSER_SEARCH_T,
    PARSER_SEARCH_CR
}
RxParserState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Command parser state.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PARSE_CMDNAME,
    PARSE_EQUAL,
    PARSE_QUESTIONMARK,
    PARSE_COMMA,
    PARSE_SEMICOLON,
    PARSE_BASIC,
    PARSE_BASIC_PARAM,
    PARSE_BASIC_END,
    PARSE_LAST,
    PARSE_MAX
}
CmdParserState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Response State.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    AT_RSP_INTERMEDIATE,
    AT_RSP_UNSOLICITED,
    AT_RSP_FINAL,
    AT_RSP_MAX
}
RspState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Subscribed AT Command structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_atServer_CmdRef_t    cmdRef;                                 ///< cmd refrence
    char                    cmdName[LE_ATDEFS_COMMAND_MAX_BYTES];   ///< Command to send
    le_atServer_AvailableDevice_t availableDevice;                  ///< device to send unsol rsp
    le_atServer_Type_t      type;                                   ///< cmd type
    le_dls_List_t           paramList;                              ///< parameters list
    bool                    processing;                             ///< is command processing
    le_atServer_DeviceRef_t deviceRef;                              ///< device refrence
    bool                    bridgeCmd;                              ///< is command created by the
                                                                    ///< AT bridge
    le_msg_SessionRef_t     sessionRef;                             ///< session reference
    bool                    isDialCommand;                          ///< specific dial command
    le_atServer_CommandHandlerFunc_t handlerFunc;                   ///< Handler associated with the
                                                                    ///< AT command
    void*                   handlerContextPtr;                      ///< client handler context
}
ATCmdSubscribed_t;

//--------------------------------------------------------------------------------------------------
/**
 * AT Command parser structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char                    foundCmd[LE_ATDEFS_COMMAND_MAX_LEN];    ///< cmd found in input string
    RxParserState_t         rxState;                                ///< input string parser state
    CmdParserState_t        cmdParser;                              ///< cmd parser state
    CmdParserState_t        lastCmdParserState;                     ///< previous cmd parser state
    char*                   currentAtCmdPtr;                        ///< current AT cmd position
                                                                    ///< in foundCmd buffer
    char*                   currentCharPtr;                         ///< current parsing position
                                                                    ///< in foundCmd buffer
    char*                   lastCharPtr;                            ///< last received character
                                                                    ///< position in foundCmd buffer
    ATCmdSubscribed_t*      currentCmdPtr;                          ///< current command context
}
CmdParser_t;

//--------------------------------------------------------------------------------------------------
/**
 * Final response structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_atServer_FinalRsp_t  final;                               ///< final result code
    bool                    customStringAvailable;               /// custom string available
    char                    resp[LE_ATDEFS_RESPONSE_MAX_BYTES];  ///< string value
}
FinalRsp_t;

//--------------------------------------------------------------------------------------------------
/**
 * Text structure definition.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    bool                                mode;                           ///< Is text mode
    size_t                              offset;                         ///< Buffer offset
    char                                buf[LE_ATSERVER_TEXT_MAX_LEN];  ///< Text buffer
    le_atServer_GetTextCallbackFunc_t   callback;                       ///< Callback function
    void*                               ctxPtr;                         ///< Context
    le_atServer_CmdRef_t                cmdRef;                         ///< Received AT command
}
Text_t;

//--------------------------------------------------------------------------------------------------
/**
 * Device context structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    Device_t                device;                               ///< data of the connected device
    le_atServer_DeviceRef_t ref;                                  ///< reference of the device
    char                    currentCmd[LE_ATDEFS_COMMAND_MAX_LEN];///< input buffer
    uint32_t                indexRead;                            ///< last read character position
                                                                  ///< in currentCmd
    uint32_t                parseIndex;                           ///< current index in currentCmd
    CmdParser_t             cmdParser;                            ///< parsing context
    FinalRsp_t              finalRsp;                             ///< final response to be sent
    bool                    processing;                           ///< is an AT command in progress
                                                                  ///< on the device
    le_dls_List_t           unsolicitedList;                      ///< unsolicited list to be sent
                                                                  ///< when the AT command will be
                                                                  ///< over
    bool                    isFirstIntermediate;                  ///< is first intermediate sent
    RspState_t              rspState;                             ///< sending response state
    le_atServer_BridgeRef_t bridgeRef;                            ///< bridge reference
    le_msg_SessionRef_t     sessionRef;                           ///< session reference
    bool                    suspended;                            ///< is device in data mode
    bool                    echo;                                 ///< is echo enabled
    Text_t                  text;                                 ///< text data
}
DeviceContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * AT commands parser automaton definition and prototypes.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*CmdParserFunc_t)(CmdParser_t* cmdParserPtr);
static le_result_t ParseTypeTest(CmdParser_t* cmdParserPtr);
static le_result_t ParseError(CmdParser_t* cmdParserPtr);
static le_result_t ParseContinue(CmdParser_t* cmdParserPtr);
static le_result_t ParseParam(CmdParser_t* cmdParserPtr);
static le_result_t ParseEqual(CmdParser_t* cmdParserPtr);
static le_result_t ParseTypeRead(CmdParser_t* cmdParserPtr);
static le_result_t ParseParam(CmdParser_t* cmdParserPtr);
static le_result_t ParseSemicolon(CmdParser_t* cmdParserPtr);
static le_result_t ParseLastChar(CmdParser_t* cmdParserPtr);
static le_result_t ParseBasic(CmdParser_t* cmdParserPtr);
static le_result_t ParseBasicEnd(CmdParser_t* cmdParserPtr);
static le_result_t ParseNone(CmdParser_t* cmdParserPtr);
static le_result_t ParseBasicParam(CmdParser_t* cmdParserPtr);
static void ParseAtCmd(DeviceContext_t* devPtr);

CmdParserFunc_t CmdParserTab[PARSE_MAX][PARSE_MAX] =
{
/*PARSE_CMDNAME*/        {   ParseContinue,                   /*PARSE_CMDNAME*/
                             ParseEqual,                      /*PARSE_EQUAL*/
                             ParseTypeRead,                   /*PARSE_QUESTIONMARK*/
                             ParseError,                      /*PARSE_COMMA*/
                             ParseSemicolon,                  /*PARSE_SEMICOLON*/
                             ParseBasic,                      /*PARSE_BASIC*/
                             ParseError,                      /*PARSE_BASIC_PARAM*/
                             ParseError,                      /*PARSE_BASIC_END*/
                             ParseLastChar                    /*PARSE_LAST*/
                         },
/*PARSE_EQUAL*/          {   ParseError,                      /*PARSE_CMDNAME*/
                             ParseError,                      /*PARSE_EQUAL*/
                             ParseTypeTest,                   /*PARSE_QUESTIONMARK*/
                             ParseParam,                      /*PARSE_COMMA*/
                             ParseError,                      /*PARSE_SEMICOLON*/
                             ParseError,                      /*PARSE_BASIC*/
                             ParseError,                      /*PARSE_BASIC_PARAM*/
                             ParseError,                      /*PARSE_BASIC_END*/
                             ParseNone                        /*PARSE_LAST*/
                         },
/*PARSE_QUESTIONMARK*/   {   ParseBasicEnd,                   /*PARSE_CMDNAME*/
                             ParseError,                      /*PARSE_EQUAL*/
                             ParseError,                      /*PARSE_QUESTIONMARK*/
                             ParseError,                      /*PARSE_COMMA*/
                             ParseSemicolon,                  /*PARSE_SEMICOLON*/
                             ParseError,                      /*PARSE_BASIC*/
                             ParseError,                      /*PARSE_BASIC_PARAM*/
                             ParseError,                      /*PARSE_BASIC_END*/
                             ParseNone                        /*PARSE_LAST*/
                         },
/*PARSE_COMMA*/          {   ParseError,                      /*PARSE_CMDNAME*/
                             ParseError,                      /*PARSE_EQUAL*/
                             ParseError,                      /*PARSE_QUESTIONMARK*/
                             ParseParam,                      /*PARSE_COMMA*/
                             ParseSemicolon,                  /*PARSE_SEMICOLON*/
                             ParseError,                      /*PARSE_BASIC*/
                             ParseError,                      /*PARSE_BASIC_PARAM*/
                             ParseError,                      /*PARSE_BASIC_END*/
                             ParseNone                        /*PARSE_LAST*/
                         },
/*PARSE_SEMICOLON*/      {   ParseContinue,                   /*PARSE_CMDNAME*/
                             ParseSemicolon,                  /*PARSE_EQUAL*/
                             ParseError,                      /*PARSE_QUESTIONMARK*/
                             ParseError,                      /*PARSE_COMMA*/
                             ParseError,                      /*PARSE_SEMICOLON*/
                             ParseError,                      /*PARSE_BASIC*/
                             ParseError,                      /*PARSE_BASIC_PARAM*/
                             ParseError,                      /*PARSE_BASIC_END*/
                             ParseNone                        /*PARSE_LAST*/
                         },
/* PARSE_BASIC */        {   ParseContinue,                   /*PARSE_CMDNAME*/
                             ParseError,                      /*PARSE_EQUAL*/
                             ParseTypeRead,                   /*PARSE_QUESTIONMARK*/
                             ParseError,                      /*PARSE_COMMA*/
                             ParseSemicolon,                  /*PARSE_SEMICOLON*/
                             ParseError,                      /*PARSE_BASIC*/
                             ParseBasicParam,                 /*PARSE_BASIC_PARAM*/
                             ParseBasicEnd,                   /*PARSE_BASIC_END*/
                             ParseNone                        /*PARSE_LAST*/
                         },
/* PARSE_BASIC_PARAM */  {   ParseError,                      /*PARSE_CMDNAME*/
                             ParseError,                      /*PARSE_EQUAL*/
                             ParseTypeRead,                   /*PARSE_QUESTIONMARK*/
                             ParseError,                      /*PARSE_COMMA*/
                             ParseSemicolon,                  /*PARSE_SEMICOLON*/
                             ParseError,                      /*PARSE_BASIC*/
                             ParseBasicParam,                 /*PARSE_BASIC_PARAM*/
                             ParseBasicEnd,                   /*PARSE_BASIC_END*/
                             ParseError                       /*PARSE_LAST*/
                         },
/* PARSE_BASIC_END */    {   ParseError,                      /*PARSE_CMDNAME*/
                             ParseError,                      /*PARSE_EQUAL*/
                             ParseError,                      /*PARSE_QUESTIONMARK*/
                             ParseError,                      /*PARSE_COMMA*/
                             ParseError,                      /*PARSE_SEMICOLON*/
                             ParseError,                      /*PARSE_BASIC*/
                             ParseError,                      /*PARSE_BASIC_PARAM*/
                             ParseError,                      /*PARSE_BASIC_END*/
                             ParseNone                        /*PARSE_LAST*/
                         },
/* PARSE_LAST  */        {   ParseContinue,                   /*PARSE_CMDNAME*/
                             ParseError,                      /*PARSE_EQUAL*/
                             ParseError,                      /*PARSE_QUESTIONMARK*/
                             ParseError,                      /*PARSE_COMMA*/
                             ParseError,                      /*PARSE_SEMICOLON*/
                             ParseError,                      /*PARSE_BASIC*/
                             ParseError,                      /*PARSE_BASIC_PARAM*/
                             ParseError,                      /*PARSE_BASIC_END*/
                             ParseNone                        /*PARSE_LAST*/
                         }
};

//--------------------------------------------------------------------------------------------------
/**
 * This function is the destructor for ATCmdSubscribed_t struct
 *
 */
//--------------------------------------------------------------------------------------------------
static void AtCmdPoolDestructor
(
    void* commandPtr
)
{
    ATCmdSubscribed_t* cmdPtr = commandPtr;
    le_dls_Link_t* linkPtr;

    LE_DEBUG("AT command pool destructor");

    // cleanup the hashmap
    le_hashmap_Remove(CmdHashMap, cmdPtr->cmdName);

    // cleanup ParamList dls pool
    while((linkPtr = le_dls_Pop(&cmdPtr->paramList)) != NULL)
    {
        ParamString_t *paramPtr = CONTAINER_OF(linkPtr,
                                    ParamString_t,
                                    link);
        le_mem_Release(paramPtr);
    }

    le_ref_DeleteRef(SubscribedCmdRefMap, cmdPtr->cmdRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a response on the opened device.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendRspString
(
    DeviceContext_t* devPtr,
    uint8_t rspType,
    const char* rspPtr
)
{
    char string[LE_ATDEFS_RESPONSE_MAX_BYTES+4];

    memset(string,0,LE_ATDEFS_RESPONSE_MAX_BYTES+4);

    switch (rspType)
    {
        case RSP_TYPE_OK:
            snprintf(string,LE_ATDEFS_RESPONSE_MAX_BYTES,"\r\nOK\r\n");
            break;
        case RSP_TYPE_ERROR:
            snprintf(string,LE_ATDEFS_RESPONSE_MAX_BYTES,"\r\nERROR\r\n");
            break;
        case RSP_TYPE_RESPONSE:
            if ( (devPtr->rspState == AT_RSP_FINAL) ||
                (devPtr->rspState == AT_RSP_UNSOLICITED) ||
                ((devPtr->rspState == AT_RSP_INTERMEDIATE) &&
                    devPtr->isFirstIntermediate) )
            {
                snprintf(string, LE_ATDEFS_RESPONSE_MAX_BYTES+4, \
                    "\r\n%s\r\n", rspPtr);
                devPtr->isFirstIntermediate = false;
            }
            else
            {
                snprintf(string, LE_ATDEFS_RESPONSE_MAX_BYTES+2, \
                    "%s\r\n", rspPtr);
            }
            break;
    }

    le_dev_Write(&devPtr->device, (uint8_t*)string, strlen(string));
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a final response on the opened device.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendFinalRsp
(
    DeviceContext_t* devPtr
)
{
    devPtr->rspState = AT_RSP_FINAL;

    if (devPtr->finalRsp.customStringAvailable && ExtendedErrorCodes)
    {
        SendRspString(devPtr, RSP_TYPE_RESPONSE, devPtr->finalRsp.resp);
    }
    else
    {
        switch (devPtr->finalRsp.final)
        {
            case LE_ATSERVER_OK:
                SendRspString(devPtr, RSP_TYPE_OK, "");
            break;
            default:
                SendRspString(devPtr, RSP_TYPE_ERROR, "");
            break;
        }
    }

    devPtr->processing = false;

    memset( &devPtr->cmdParser, 0, sizeof(CmdParser_t) );
    memset( &devPtr->finalRsp, 0, sizeof(FinalRsp_t) );

    // Send backup unsolicited responses
    le_dls_Link_t* linkPtr;

    while ( (linkPtr = le_dls_Pop(&devPtr->unsolicitedList)) != NULL )
    {
        RspString_t* rspStringPtr = CONTAINER_OF(linkPtr,
                                    RspString_t,
                                    link);

        SendRspString(devPtr, RSP_TYPE_RESPONSE, rspStringPtr->resp);
        le_mem_Release(rspStringPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a intermediate response on the opened device.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendIntermediateRsp
(
    DeviceContext_t* devPtr,
    RspString_t* rspStringPtr
)
{
    if (rspStringPtr == NULL)
    {
        LE_ERROR("Bad rspStringPtr");
        return;
    }

    if (devPtr == NULL)
    {
        LE_ERROR("Bad devPtr");
        le_mem_Release(rspStringPtr);
        return;
    }

    devPtr->rspState = AT_RSP_INTERMEDIATE;

    // Check if command is currently in processing
    if ((!devPtr->processing) ||
        (devPtr->cmdParser.currentCmdPtr && !((devPtr->cmdParser.currentCmdPtr)->processing)))
    {
        LE_ERROR("Command not processing anymore");
        le_mem_Release(rspStringPtr);
        return;
    }

    SendRspString(devPtr, RSP_TYPE_RESPONSE, rspStringPtr->resp);
    le_mem_Release(rspStringPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Send an unsolicited response on the opened device.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendUnsolRsp
(
    DeviceContext_t* devPtr,
    RspString_t* rspStringPtr
)
{
    if (rspStringPtr == NULL)
    {
        LE_ERROR("Bad rspStringPtr");
        return;
    }

    if (devPtr == NULL)
    {
        LE_ERROR("Bad devPtr");
        le_mem_Release(rspStringPtr);
        return;
    }

    devPtr->rspState = AT_RSP_UNSOLICITED;

    if (!devPtr->processing && !devPtr->suspended)
    {
        SendRspString(devPtr, RSP_TYPE_RESPONSE, rspStringPtr->resp);
        le_mem_Release(rspStringPtr);
    }
    else
    {
        le_dls_Queue(&devPtr->unsolicitedList, &(rspStringPtr->link));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a modem AT command using the ATBridge.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CreateModemCommand
(
    CmdParser_t* cmdParserPtr,
    char*        atCmdPtr
)
{
    if ( bridge_Create(atCmdPtr) != LE_OK )
    {
        LE_ERROR("Error in AT command creation");
        return LE_FAULT;
    }

    cmdParserPtr->currentCmdPtr = le_hashmap_Get(CmdHashMap,
                                                 atCmdPtr);

    if ( cmdParserPtr->currentCmdPtr == NULL )
    {
        LE_ERROR("At command still not exists");
        return LE_FAULT;
    }

    (cmdParserPtr->currentCmdPtr)->bridgeCmd = true;

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * Get the AT command context
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t GetAtCmdContext
(
    CmdParser_t* cmdParserPtr
)
{
    DeviceContext_t* devPtr = CONTAINER_OF(cmdParserPtr, DeviceContext_t, cmdParser);

    if (cmdParserPtr->currentCmdPtr == NULL)
    {
        cmdParserPtr->currentCmdPtr = le_hashmap_Get(CmdHashMap, cmdParserPtr->currentAtCmdPtr);

        if ( cmdParserPtr->currentCmdPtr == NULL )
        {
            LE_DEBUG("AT command not found");
            if ( devPtr->bridgeRef )
            {
                if (( CreateModemCommand(cmdParserPtr, cmdParserPtr->currentAtCmdPtr) != LE_OK ) ||
                    ( cmdParserPtr->currentCmdPtr == NULL ))
                {
                    LE_ERROR("At command still not exists");
                    return LE_FAULT;
                }
            }
            else
            {
                return LE_FAULT;
            }
        }
        else if ( cmdParserPtr->currentCmdPtr->processing )
        {
            LE_DEBUG("AT command currently in processing");
            return LE_BUSY;
        }

        cmdParserPtr->currentCmdPtr->processing = true;

        cmdParserPtr->currentCmdPtr->deviceRef = devPtr->ref;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (detection of =?)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseTypeTest
(
    CmdParser_t* cmdParserPtr
)
{
    if ( cmdParserPtr->currentCmdPtr == NULL )
    {
        return LE_FAULT;
    }

    cmdParserPtr->currentCmdPtr->type = LE_ATSERVER_TYPE_TEST;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (detection of ?)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseTypeRead
(
    CmdParser_t* cmdParserPtr
)
{
    *cmdParserPtr->currentCharPtr='\0';
    le_result_t res = GetAtCmdContext(cmdParserPtr);

    if (res == LE_OK)
    {
        cmdParserPtr->currentCmdPtr->type = LE_ATSERVER_TYPE_READ;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (unspecified)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseError
(
    CmdParser_t* cmdParserPtr
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (nothing to do)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseNone
(
    CmdParser_t* cmdParserPtr
)
{
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (put all characters in uppercase)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseContinue
(
    CmdParser_t* cmdParserPtr
)
{
    // Put character in upper case
    *cmdParserPtr->currentCharPtr = toupper(*cmdParserPtr->currentCharPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (Get a parameter from basic format commands)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseBasicCmdParam
(
    CmdParser_t* cmdParserPtr
)
{
    ParamString_t* paramPtr = le_mem_ForceAlloc(ParamStringPool);
    memset(paramPtr,0,sizeof(ParamString_t));
    uint32_t index = 0;
    bool tokenQuote = false;

    while ( cmdParserPtr->currentCharPtr <= cmdParserPtr->lastCharPtr )
    {
        if ( IS_QUOTE(*cmdParserPtr->currentCharPtr) )
        {
            if (tokenQuote)
            {
                tokenQuote = false;
            }
            else
            {
                tokenQuote = true;
            }

            // If "bridge command", keep the quote
            if ((cmdParserPtr->currentCmdPtr)->bridgeCmd)
            {
                paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
            }
        }
        else
        {
            if ((tokenQuote) || ( IS_NUMBER(*cmdParserPtr->currentCharPtr) ))
            {
                paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
            }
            else if (*cmdParserPtr->currentCharPtr == AT_TOKEN_EQUAL)
            {
                break;
            }
            else if ( !IS_NUMBER(*cmdParserPtr->currentCharPtr) )
            {
                cmdParserPtr->currentCharPtr--;
                break;
            }
        }

        cmdParserPtr->currentCharPtr++;
    }

    cmdParserPtr->currentCmdPtr->type = LE_ATSERVER_TYPE_PARA;
    le_dls_Queue(&(cmdParserPtr->currentCmdPtr->paramList),&(paramPtr->link));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get Dial command parameter
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseBasicDCmdParam
(
    CmdParser_t* cmdParserPtr
)
{
    const char* possibleCharUpper[]={"A","B","C","D","T","P","W"};
    const char* possibleChar[]={"0","1","2","3","4","5","6","7","8","9","*","#","+",",","!","@",";",
                                "I","i","G","g"};

    int i;
    int index = 0;
    ParamString_t* paramPtr = le_mem_ForceAlloc(ParamStringPool);
    bool dialingFromPhonebook = false;
    memset(paramPtr,0,sizeof(ParamString_t));
    bool tokenQuote = false;

    LE_DEBUG("%s", cmdParserPtr->currentCharPtr);

    if ( *cmdParserPtr->currentCharPtr == '>' )
    {
        dialingFromPhonebook = true;
    }

    while ( cmdParserPtr->currentCharPtr <= cmdParserPtr->lastCharPtr )
    {
        if (dialingFromPhonebook)
        {
            if ( IS_QUOTE(*cmdParserPtr->currentCharPtr) )
            {
                if (tokenQuote)
                {
                    tokenQuote = false;
                }
                else
                {
                    tokenQuote = true;
                }

                paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
            }
            else
            {
                if (tokenQuote)
                {
                    paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
                }
                else
                {
                    if ( (*cmdParserPtr->currentCharPtr == 'i') ||
                         ( *cmdParserPtr->currentCharPtr == 'g') )
                    {
                        paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
                    }
                    else
                    {
                        paramPtr->param[index++] = toupper(*cmdParserPtr->currentCharPtr);
                    }
                }
            }
        }
        else
        {

            bool charFound = false;
            int nbPass = 0;
            const char** charTabPtr = possibleChar;
            int nbValue = NUM_ARRAY_MEMBERS(possibleChar);
            char* testCharPtr = cmdParserPtr->currentCharPtr;

            // Only the valid characters are kept, other are ignored
            while (nbPass < 2)
            {
                for (i=0; i<nbValue; i++)
                {
                    if (*testCharPtr == *charTabPtr[i])
                    {
                        paramPtr->param[index++] = *testCharPtr;
                        charFound = true;
                        break;
                    }
                }

                if (charFound)
                {
                    break;
                }
                else
                {
                    *testCharPtr = toupper(*testCharPtr);
                    charTabPtr = possibleCharUpper;
                    nbValue = NUM_ARRAY_MEMBERS(possibleCharUpper);
                    nbPass++;
                }
            }
        }

        // V25ter mentions that the end of the dial command is:
        // - terminated by a semicolon character
        // - the end of the command line
        if (*cmdParserPtr->currentCharPtr == AT_TOKEN_SEMICOLON)
        {
            // Stop the parsing, it looks that we reach the end of ATD command parameter
            goto end;
        }

        cmdParserPtr->currentCharPtr++;
    }

    if (index == 0)
    {
        LE_ERROR("empty phone number");
        le_mem_Release(paramPtr);
        return LE_FAULT;
    }

end:
    cmdParserPtr->currentCmdPtr->type = LE_ATSERVER_TYPE_PARA;
    le_dls_Queue(&(cmdParserPtr->currentCmdPtr->paramList),&(paramPtr->link));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (Get a parameter from basic command)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseBasicParam
(
    CmdParser_t* cmdParserPtr
)
{
    if (cmdParserPtr->currentCmdPtr)
    {
        if (cmdParserPtr->currentCmdPtr->isDialCommand)
        {
            return ParseBasicDCmdParam(cmdParserPtr);
        }
        else
        {
            return ParseBasicCmdParam(cmdParserPtr);
        }
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (end of treatment for a basic command)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseBasicEnd
(
    CmdParser_t* cmdParserPtr
)
{
    cmdParserPtr->currentAtCmdPtr = cmdParserPtr->currentCharPtr-2;
    memcpy(cmdParserPtr->currentAtCmdPtr, "AT", 2);

    // Put the index at the correct place for next parsing
    cmdParserPtr->currentCharPtr = cmdParserPtr->currentAtCmdPtr;

    cmdParserPtr->cmdParser = PARSE_LAST;

    cmdParserPtr->currentCharPtr--;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Basic format command found, update command context
 *
 */
//--------------------------------------------------------------------------------------------------
static void BasicCmdFound
(
    CmdParser_t* cmdParserPtr
)
{
    DeviceContext_t* devPtr = CONTAINER_OF(cmdParserPtr, DeviceContext_t, cmdParser);

    cmdParserPtr->currentCmdPtr->deviceRef = devPtr->ref;
    cmdParserPtr->currentCmdPtr->type = LE_ATSERVER_TYPE_ACT;
    cmdParserPtr->currentCmdPtr->processing = true;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (treatment of a basic format commands)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseBasic
(
    CmdParser_t* cmdParserPtr
)
{
    while ( ( cmdParserPtr->currentCharPtr <= cmdParserPtr->lastCharPtr ) &&
            ( !IS_NUMBER(*cmdParserPtr->currentCharPtr) ) &&
            ( !IS_QUOTE(*cmdParserPtr->currentCharPtr) ) )
    {
        *cmdParserPtr->currentCharPtr = toupper(*cmdParserPtr->currentCharPtr);
        cmdParserPtr->currentCharPtr++;
    }

    uint32_t len = cmdParserPtr->currentCharPtr-cmdParserPtr->currentAtCmdPtr+1;
    char* initialPosPtr = cmdParserPtr->currentCharPtr;
    char atCmd[len];
    memset(atCmd,0,len);
    strncpy(atCmd, cmdParserPtr->currentAtCmdPtr, len-1);

    while (strlen(atCmd) > 2)
    {
        cmdParserPtr->currentCmdPtr = le_hashmap_Get(CmdHashMap, atCmd);

        if ( cmdParserPtr->currentCmdPtr == NULL )
        {
            atCmd[strlen(atCmd)-1] = '\0';
            cmdParserPtr->currentCharPtr--;
        }
        else
        {
            BasicCmdFound(cmdParserPtr);

            cmdParserPtr->currentCharPtr--;

            return LE_OK;
        }
    }

    DeviceContext_t* devPtr = CONTAINER_OF(cmdParserPtr, DeviceContext_t, cmdParser);

    if ( devPtr->bridgeRef )
    {
        //Reset the pointer to its initial value
        cmdParserPtr->currentCharPtr = initialPosPtr;

        strncpy(atCmd, cmdParserPtr->currentAtCmdPtr, len-1);

        if (( CreateModemCommand(cmdParserPtr, atCmd) != LE_OK ) ||
            ( cmdParserPtr->currentCmdPtr == NULL ))
        {
            LE_ERROR("At command still not exists");
            return LE_FAULT;
        }

        BasicCmdFound(cmdParserPtr);

        cmdParserPtr->currentCharPtr--;

        return LE_OK;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (treat '=')
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseEqual
(
    CmdParser_t* cmdParserPtr
)
{
    *cmdParserPtr->currentCharPtr='\0';
    le_result_t res = GetAtCmdContext(cmdParserPtr);

    if (res == LE_OK)
    {
        cmdParserPtr->currentCmdPtr->type = LE_ATSERVER_TYPE_PARA;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (treat a parameter for extended format commands)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseParam
(
    CmdParser_t* cmdParserPtr
)
{
    ParamString_t* paramPtr = le_mem_ForceAlloc(ParamStringPool);
    memset(paramPtr,0,sizeof(ParamString_t));
    uint32_t index = 0;
    bool tokenQuote = false;
    bool loop = true;

    if (le_dls_NumLinks(&(cmdParserPtr->currentCmdPtr->paramList)) != 0)
    {
        // bypass comma (not done for the first param)
        cmdParserPtr->currentCharPtr++;
    }

    if (( cmdParserPtr->currentCharPtr > cmdParserPtr->lastCharPtr ) ||
        ( *cmdParserPtr->currentCharPtr == AT_TOKEN_COMMA ) ||
        ( *cmdParserPtr->currentCharPtr == AT_TOKEN_SEMICOLON ))
    {
        loop = false;
        cmdParserPtr->currentCharPtr--;
    }

    while (loop)
    {
        if ( IS_QUOTE(*cmdParserPtr->currentCharPtr) )
        {
            if (tokenQuote)
            {
                tokenQuote = false;
            }
            else
            {
                tokenQuote = true;
            }

            // If "bridge command", keep the quote
            if ((cmdParserPtr->currentCmdPtr)->bridgeCmd)
            {
                paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
            }
        }
        else
        {
            if (!tokenQuote)
            {
                // Put character in upper case
                *cmdParserPtr->currentCharPtr = toupper(*cmdParserPtr->currentCharPtr);
            }

            if ((tokenQuote) || ( IS_PARAM_CHAR(*cmdParserPtr->currentCharPtr) ))
            {
                paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
            }
            else
            {
                le_mem_Release(paramPtr);
                return LE_FAULT;
            }
        }

        cmdParserPtr->currentCharPtr++;

        if ( cmdParserPtr->currentCharPtr > cmdParserPtr->lastCharPtr )
        {
            loop = false;
        }

        if ((tokenQuote == false) &&
            (( *cmdParserPtr->currentCharPtr == AT_TOKEN_COMMA ) ||
             ( *cmdParserPtr->currentCharPtr == AT_TOKEN_SEMICOLON )))
        {
            loop = false;
            cmdParserPtr->currentCharPtr--;
        }
    }

    le_dls_Queue(&(cmdParserPtr->currentCmdPtr->paramList),&(paramPtr->link));

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (treat last character)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseLastChar
(
    CmdParser_t* cmdParserPtr
)
{
    if ( cmdParserPtr->currentCmdPtr == NULL )
    {
        le_result_t res;

        // Put character in upper case
        *cmdParserPtr->currentCharPtr = toupper(*cmdParserPtr->currentCharPtr);

        res = GetAtCmdContext(cmdParserPtr);

        if (res == LE_OK)
        {
            cmdParserPtr->currentCmdPtr->type = LE_ATSERVER_TYPE_ACT;
        }

        return res;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (treat ';')
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseSemicolon
(
    CmdParser_t* cmdParserPtr
)
{
    *cmdParserPtr->currentCharPtr='\0';

    // if AT command not resolved yet, try to get it
    if ( ParseLastChar(cmdParserPtr) != LE_OK )
    {
        return LE_FAULT;
    }

    // Concatenate command: prepare the buffer for the next parsing
    // Be sure to not write outside the buffer
    cmdParserPtr->currentCharPtr--;
    if (cmdParserPtr->currentCharPtr >= cmdParserPtr->foundCmd)
    {
        memcpy(cmdParserPtr->currentCharPtr, "AT", 2);

        // Put the index at the correct place for next parsing
        cmdParserPtr->currentAtCmdPtr = cmdParserPtr->currentCharPtr;
        cmdParserPtr->currentCharPtr--;
    }
    else
    {
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser main function
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseAtCmd
(
    DeviceContext_t* devPtr
)
{
    if (devPtr == NULL)
    {
        LE_ERROR("Bad device");
        return;
    }

    CmdParser_t* cmdParserPtr = &devPtr->cmdParser;

    cmdParserPtr->cmdParser = PARSE_CMDNAME;

    devPtr->cmdParser.currentCmdPtr = NULL;

    devPtr->isFirstIntermediate = true;

    // if parsing over, send the final response
    if (cmdParserPtr->currentCharPtr > cmdParserPtr->lastCharPtr)
    {
        SendFinalRsp(devPtr);
        return;
    }

    while (( cmdParserPtr->cmdParser != PARSE_SEMICOLON ) &&
           ( cmdParserPtr->cmdParser != PARSE_LAST ))
    {
        switch (*cmdParserPtr->currentCharPtr)
        {
            case AT_TOKEN_EQUAL:
                cmdParserPtr->cmdParser = PARSE_EQUAL;
            break;
            case AT_TOKEN_QUESTIONMARK:
                cmdParserPtr->cmdParser = PARSE_QUESTIONMARK;
            break;
            case AT_TOKEN_COMMA:
                cmdParserPtr->cmdParser = PARSE_COMMA;
            break;
            case AT_TOKEN_SEMICOLON:
                cmdParserPtr->cmdParser = PARSE_SEMICOLON;
            break;
            default:
                if (cmdParserPtr->cmdParser >= PARSE_BASIC)
                {
                    if ( cmdParserPtr->currentCmdPtr &&
                         cmdParserPtr->currentCmdPtr->isDialCommand )
                    {
                        if (cmdParserPtr->cmdParser == PARSE_BASIC)
                        {
                            cmdParserPtr->cmdParser = PARSE_BASIC_PARAM;
                        }
                        else if (cmdParserPtr->cmdParser == PARSE_BASIC_PARAM)
                        {
                            cmdParserPtr->cmdParser = PARSE_BASIC_END;
                        }
                        break;
                    }

                    if ( IS_NUMBER(*cmdParserPtr->currentCharPtr) ||
                         IS_QUOTE(*cmdParserPtr->currentCharPtr) )
                    {
                         cmdParserPtr->cmdParser = PARSE_BASIC_PARAM;
                    }
                    else
                    {
                         cmdParserPtr->cmdParser = PARSE_BASIC_END;
                    }
                    break;
                }

                if ((cmdParserPtr->currentCharPtr - cmdParserPtr->currentAtCmdPtr == 2) &&
                    (IS_BASIC(*cmdParserPtr->currentCharPtr)))
                {
                    // 3rd char of the command is into [A-Z] => basic command
                    cmdParserPtr->cmdParser = PARSE_BASIC;
                }

                else if ((cmdParserPtr->currentCmdPtr) &&
                    (cmdParserPtr->currentCmdPtr->type == LE_ATSERVER_TYPE_PARA))
                {
                    cmdParserPtr->cmdParser = PARSE_COMMA;
                }
                else if (cmdParserPtr->currentCharPtr == cmdParserPtr->lastCharPtr)
                {
                    cmdParserPtr->cmdParser = PARSE_LAST;
                }
                else
                {
                    cmdParserPtr->cmdParser = PARSE_CMDNAME;
                }
            break;
        }

        if ((cmdParserPtr->lastCmdParserState >= PARSE_MAX) ||
            (cmdParserPtr->cmdParser >= PARSE_MAX))
        {
            LE_ERROR("Wrong parser state");
            return;
        }

        le_result_t res;
        res = CmdParserTab[cmdParserPtr->lastCmdParserState][cmdParserPtr->cmdParser](cmdParserPtr);

        if (res == LE_OK)
        {
            cmdParserPtr->lastCmdParserState = cmdParserPtr->cmdParser;
            cmdParserPtr->currentCharPtr++;

            if (cmdParserPtr->currentCharPtr > cmdParserPtr->lastCharPtr)
            {
                cmdParserPtr->cmdParser = PARSE_LAST;
            }
        }
        else
        {
            LE_ERROR("Error in parsing AT command, lastState %d, current state %d",
                                                        cmdParserPtr->lastCmdParserState,
                                                        cmdParserPtr->cmdParser);

            if (res == LE_BUSY)
            {
                LE_INFO("AT command busy");
            }
            else
            {
                if (cmdParserPtr->currentCmdPtr)
                {
                    cmdParserPtr->currentCmdPtr->processing = false;
                }
            }

            goto sendErrorRsp;
        }
    }

    if (cmdParserPtr->currentCmdPtr)
    {
        ATCmdSubscribed_t* cmdPtr = cmdParserPtr->currentCmdPtr;

        if (cmdPtr->handlerFunc)
        {
            (cmdPtr->handlerFunc)( cmdPtr->cmdRef,
                                   cmdPtr->type,
                                   le_dls_NumLinks(&(cmdPtr->paramList)),
                                   cmdPtr->handlerContextPtr );
        }
        else
        {
            // Command exists, but no handler associate to it
            cmdParserPtr->currentCmdPtr->processing = false;
            goto sendErrorRsp;
        }
    }

    return;

sendErrorRsp:
    devPtr->finalRsp.final = LE_ATSERVER_ERROR;
    devPtr->finalRsp.customStringAvailable = false;

    SendFinalRsp(devPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Parser incoming characters
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseBuffer
(
    DeviceContext_t* devPtr
)
{
    uint32_t i;

    for (i = devPtr->parseIndex; i < devPtr->indexRead; i++)
    {
        char input = devPtr->currentCmd[i];

        switch (devPtr->cmdParser.rxState)
        {
            case PARSER_SEARCH_A:
                if (( input == 'A' ) || ( input == 'a' ))
                {
                    devPtr->currentCmd[0] = input;
                    devPtr->cmdParser.rxState = PARSER_SEARCH_T;
                    devPtr->parseIndex = 1;
                }
            break;
            case PARSER_SEARCH_T:
                switch (input)
                {
                    case 'T':
                    case 't':
                    {
                        devPtr->currentCmd[1] = input;
                        devPtr->cmdParser.rxState = PARSER_SEARCH_CR;
                        devPtr->parseIndex = 2;
                    }
                    break;
                    case 'A':
                    case 'a':
                        // do nothing in this case
                    break;
                    default:
                    {
                        devPtr->cmdParser.rxState = PARSER_SEARCH_A;
                        devPtr->parseIndex = 0;
                    }
                    break;
                }
            break;
            case PARSER_SEARCH_CR:
            {
                if ( input == AT_TOKEN_CR )
                {
                    if (!devPtr->processing)
                    {
                        devPtr->processing = true;

                        devPtr->currentCmd[devPtr->parseIndex] = '\0';
                        LE_DEBUG("Command found %s", devPtr->currentCmd);
                        le_utf8_Copy(devPtr->cmdParser.foundCmd,
                                     devPtr->currentCmd,
                                     LE_ATDEFS_COMMAND_MAX_LEN,
                                     NULL);

                        devPtr->cmdParser.currentCharPtr = devPtr->cmdParser.foundCmd;
                        devPtr->cmdParser.lastCharPtr = devPtr->cmdParser.foundCmd +
                                                         strlen(devPtr->cmdParser.foundCmd) - 1;
                        devPtr->cmdParser.currentAtCmdPtr = devPtr->cmdParser.foundCmd;

                        ParseAtCmd(devPtr);
                    }
                    else
                    {
                        LE_ERROR("Command in progress");
                        SendRspString(devPtr, RSP_TYPE_ERROR, "");
                    }

                    devPtr->parseIndex=0;
                }
                // backspace character
                else if ( input == 0x7F )
                {
                    devPtr->parseIndex--;
                }
                else
                {
                    devPtr->currentCmd[devPtr->parseIndex] = input;
                    devPtr->parseIndex++;
                }
            }
            break;
            default:
                LE_ERROR("bad state !!");
            break;
        }
    }

    devPtr->indexRead = devPtr->parseIndex;

    if (devPtr->indexRead >= LE_ATDEFS_COMMAND_MAX_LEN)
    {
        devPtr->indexRead = devPtr->parseIndex = 0;
        devPtr->cmdParser.rxState = PARSER_SEARCH_A;
        SendRspString(devPtr, RSP_TYPE_ERROR, "");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function handles receiving AT commands
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReceiveCmd
(
    DeviceContext_t *devPtr
)
{
    ssize_t size;
    // Read RX data on uart
    size = le_dev_Read(&devPtr->device,
                (uint8_t *)(devPtr->currentCmd + devPtr->indexRead),
                (LE_ATDEFS_COMMAND_MAX_LEN - devPtr->indexRead));

    // Echo is activated
    if (devPtr->echo)
    {
        le_dev_Write(&devPtr->device,
                    (uint8_t *)(devPtr->currentCmd + devPtr->indexRead),
                    size);
    }

    devPtr->indexRead += size;
    ParseBuffer(devPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function handles receiving text
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReceiveText
(
    DeviceContext_t *devPtr
)
{
    size_t size;
    ssize_t count;
    Device_t device;
    Text_t *textPtr;
    void* ctxPtr;

    size = LE_ATSERVER_TEXT_MAX_LEN;
    textPtr = &devPtr->text;
    device = devPtr->device;
    ctxPtr = NULL;

    if (textPtr->ctxPtr)
    {
        ctxPtr = textPtr->ctxPtr;
    }

    count = le_dev_Read(&device, (uint8_t *)textPtr->buf+textPtr->offset, size-textPtr->offset);
    if (!count)
    {
        LE_ERROR("connection closed");
        textPtr->buf[0] = '\0';
        if (textPtr->callback)
        {
            textPtr->callback(textPtr->cmdRef, LE_IO_ERROR, "", 0, ctxPtr);
        }
        textPtr->mode = false;
        return;
    }

    if (devPtr->echo)
    {
        le_dev_Write(&devPtr->device, (uint8_t *)textPtr->buf+textPtr->offset, count);
    }

    textPtr->offset += count;

    if (ESCAPE == textPtr->buf[textPtr->offset-1])
    {
        LE_DEBUG("Received a cancel request");
        textPtr->buf[0] = '\0';
        if (textPtr->callback)
        {
            textPtr->callback(textPtr->cmdRef, LE_OK, "", 0, ctxPtr);
        }
        textPtr->mode = false;
    }

    if (SUBSTITUTE == textPtr->buf[textPtr->offset-1])
    {
        LE_DEBUG("Received text %s", textPtr->buf);
        textPtr->buf[textPtr->offset-1] = '\0';
        if (textPtr->callback)
        {
            textPtr->callback(textPtr->cmdRef, LE_OK, textPtr->buf, textPtr->offset-1, ctxPtr);
        }
        textPtr->mode = false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called when data are available to be read on fd
 *
 */
//--------------------------------------------------------------------------------------------------
static void RxNewData
(
    int fd,      ///< File descriptor to read on
    short events ///< Event reported on fd (expect only POLLIN)
)
{
    DeviceContext_t *devPtr;

    devPtr = le_fdMonitor_GetContextPtr();

    if (events & POLLRDHUP)
    {
        LE_INFO("fd %d: Connection reset by peer", fd);
        le_dev_RemoveFdMonitoring(&devPtr->device);
        return;
    }

    if (events & (POLLIN | POLLPRI))
    {
        if (devPtr->text.mode)
        {
            LE_DEBUG("Receiving text");
            ReceiveText(devPtr);
        }
        else
        {
            LE_DEBUG("Receiving AT command");
            ReceiveCmd(devPtr);
        }
    }
    else
    {
        LE_CRIT("Unexpected event(s) on fd %d (0x%hX).", fd, events);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Send unsolicited response.
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t SendUnsolicitedResponse
(
    DeviceContext_t* devPtr,
    const char* unsolRsp
)
{
    if (!devPtr || !unsolRsp)
    {
        LE_ERROR("Bad entries");
        return LE_FAULT;
    }

    RspString_t* rspStringPtr = le_mem_ForceAlloc(RspStringPool);
    le_utf8_Copy(rspStringPtr->resp, unsolRsp, LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);

    SendUnsolRsp(devPtr, rspStringPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function closes the AT server session on the requested device.
 *
 * @return
 *      - LE_OK             The function succeeded.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 *      - LE_BUSY           The requested device is busy.
 *      - LE_FAULT          Failed to stop the server, check logs
 *                              for more information.
 */
//--------------------------------------------------------------------------------------------------
static le_result_t CloseServer
(
    le_atServer_DeviceRef_t devRef
        ///< [IN] device to be unbinded
)
{
    ATCmdSubscribed_t* cmdPtr = NULL;
    le_dls_Link_t* linkPtr = NULL;
    char errMsg[ERR_MSG_MAX];

    DeviceContext_t* devPtr = le_ref_Lookup(DevicesRefMap, devRef);

    if (!devPtr)
    {
        LE_ERROR("Invalid device");
        return LE_BAD_PARAMETER;
    }

    LE_DEBUG("Stopping device %d", devPtr->device.fd);

    le_dev_RemoveFdMonitoring(&devPtr->device);

    if (close(devPtr->device.fd))
    {
        // using thread safe strerror
        memset(errMsg, 0, ERR_MSG_MAX);
        LE_ERROR("%s", strerror_r(errno, errMsg, ERR_MSG_MAX));
        return LE_FAULT;
    }

    cmdPtr = devPtr->cmdParser.currentCmdPtr;
    if (cmdPtr)
    {
        cmdPtr->processing = false;
    }

    // cleanup the dls pool
    while((linkPtr = le_dls_Pop(&devPtr->unsolicitedList)) != NULL)
    {
        RspString_t *unsolicitedPtr = CONTAINER_OF(linkPtr,
                                    RspString_t,
                                    link);
        le_mem_Release(unsolicitedPtr);
    }

    // Remove from bridge
    if (devPtr->bridgeRef)
    {
        le_atServer_RemoveDeviceFromBridge(devRef, devPtr->bridgeRef);
    }

    le_ref_DeleteRef(DevicesRefMap, devPtr->ref);

    le_mem_Release(devPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * handler function to the close session service
 *
 */
//--------------------------------------------------------------------------------------------------
static void CloseSessionEventHandler
(
    le_msg_SessionRef_t sessionRef,
    void*               contextPtr
)
{
    le_ref_IterRef_t iter;
    ATCmdSubscribed_t *cmdPtr = NULL;
    DeviceContext_t *devPtr = NULL;

    iter = le_ref_GetIterator(SubscribedCmdRefMap);
    while (LE_OK == le_ref_NextNode(iter))
    {
        cmdPtr = (ATCmdSubscribed_t *) le_ref_GetValue(iter);
        if (cmdPtr)
        {
            if (sessionRef == cmdPtr->sessionRef)
            {
                LE_DEBUG("deleting %s", cmdPtr->cmdName);
                le_mem_Release(cmdPtr);
            }
        }
    }

    // close associated bridge
    bridge_CleanContext(sessionRef);

    iter = le_ref_GetIterator(DevicesRefMap);
    while (LE_OK == le_ref_NextNode(iter))
    {
        devPtr = (DeviceContext_t *) le_ref_GetValue(iter);
        if (devPtr)
        {
            if (sessionRef == devPtr->sessionRef)
            {
                LE_DEBUG("deleting fd %d", devPtr->device.fd);
                CloseServer(devPtr->ref);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Suspend server / enter data mode
 *
 * When this function is called the server stops monitoring the fd for events
 * hence no more I/O operations are done on the fd by the server.
 *
 * @return
 *      - LE_OK             Success.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 *      - LE_FAULT          Device not monitored
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_Suspend
(
    le_atServer_DeviceRef_t devRef ///< [IN] device to be unbinded
)
{
    DeviceContext_t* devPtr;
    Device_t *devicePtr;

    devPtr = le_ref_Lookup(DevicesRefMap, devRef);
    devicePtr = &devPtr->device;

    if (!devPtr)
    {
        LE_ERROR("Invalid device");
        return LE_BAD_PARAMETER;
    }

    if (!devicePtr || !devicePtr->fdMonitor)
    {
        LE_ERROR("Device is not monitored");
        return LE_FAULT;
    }

    le_dev_RemoveFdMonitoring(devicePtr);

    devPtr->suspended = true;

    LE_INFO("server suspended");

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Resume server / enter command mode
 *
 * When this function is called the server resumes monitoring the fd for events
 * and is able to interpret AT commands again.
 *
 * @return
 *      - LE_OK             Success.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 *      - LE_FAULT          Device not monitored
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_Resume
(
    le_atServer_DeviceRef_t devRef ///< [IN] device to be unbinded
)
{
    DeviceContext_t* devPtr;
    Device_t *devicePtr;

    devPtr = le_ref_Lookup(DevicesRefMap, devRef);
    devicePtr = &devPtr->device;

    if (!devPtr || !devicePtr)
    {
        LE_ERROR("Invalid device");
        return LE_BAD_PARAMETER;
    }

    if (le_dev_AddFdMonitoring(devicePtr, RxNewData, devPtr) != LE_OK)
    {
        LE_ERROR("Error during adding the fd monitoring");
        return LE_FAULT;
    }

    devPtr->suspended = false;

    LE_INFO("server resumed");

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function gets the bridge reference on a AT command in progress.
 *
 * @return
 *      - Reference to the requested device.
 *      - NULL if the device is not available.
 *
 * @note
 *  This function internal, not exposed as API
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_GetBridgeRef
(
    le_atServer_CmdRef_t     commandRef,
    le_atServer_BridgeRef_t* bridgeRefPtr
)
{
    ATCmdSubscribed_t* cmdPtr = le_ref_Lookup(SubscribedCmdRefMap, commandRef);

    if (cmdPtr == NULL)
    {
        LE_ERROR("Bad reference");
        return LE_FAULT;
    }

    if (cmdPtr->bridgeCmd && cmdPtr->processing)
    {
        DeviceContext_t* devPtr = le_ref_Lookup(DevicesRefMap, cmdPtr->deviceRef);

        if (devPtr == NULL)
        {
            LE_ERROR("Bad device reference");
            return LE_FAULT;
        }

        *bridgeRefPtr = devPtr->bridgeRef;

        return LE_OK;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function unlinks the device from the bridge.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to unlink the device from the bridge.
 *
 * @note
 *  This function internal, not exposed as API
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_UnlinkDeviceFromBridge
(
    le_atServer_DeviceRef_t deviceRef,
    le_atServer_BridgeRef_t bridgeRef
)
{
    DeviceContext_t* devPtr = le_ref_Lookup(DevicesRefMap, deviceRef);

    if (devPtr == NULL)
    {
        LE_ERROR("Bad reference");
        return LE_FAULT;
    }

    if (devPtr->bridgeRef == bridgeRef)
    {
        devPtr->bridgeRef = NULL;
        return LE_OK;
    }

    LE_ERROR("Unable to unlink device %p from bridge %p, current association: %p",
                                                    deviceRef, bridgeRef, devPtr->bridgeRef);

    return LE_FAULT;

}


//--------------------------------------------------------------------------------------------------
/**
 * This function opens an AT server session on the requested device.
 *
 * @return
 *      - Reference to the requested device.
 *      - NULL if the device is not available or fd is a BAD FILE DESCRIPTOR.
 */
//--------------------------------------------------------------------------------------------------
le_atServer_DeviceRef_t le_atServer_Open
(
    int32_t              fd          ///< The file descriptor
)
{
    char errMsg[ERR_MSG_MAX];

    // check if the file descriptor is valid
    if (fcntl(fd, F_GETFD) == -1)
    {
        memset(errMsg, 0, ERR_MSG_MAX);
        LE_ERROR("%s", strerror_r(errno, errMsg, ERR_MSG_MAX));
        return NULL;
    }


    DeviceContext_t* devPtr = le_mem_ForceAlloc(DevicesPool);
    if (!devPtr)
    {
        return NULL;
    }

    memset(devPtr,0,sizeof(DeviceContext_t));

    LE_DEBUG("Create a new interface for %d", fd);
    devPtr->device.fd = fd;

    if (devPtr->device.fdMonitor)
    {
        LE_ERROR("Interface already monitored %d", devPtr->device.fd);
        return NULL;
    }

    if (le_dev_AddFdMonitoring(&devPtr->device, RxNewData, devPtr) != LE_OK)
    {
        LE_ERROR("Error during adding the fd monitoring");
        return NULL;
    }

    devPtr->cmdParser.rxState = PARSER_SEARCH_A;
    devPtr->parseIndex = 0;
    devPtr->unsolicitedList = LE_DLS_LIST_INIT;
    devPtr->isFirstIntermediate = true;
    devPtr->sessionRef = le_atServer_GetClientSessionRef();
    devPtr->ref = le_ref_CreateRef(DevicesRefMap, devPtr);
    devPtr->suspended = false;

    LE_INFO("created device");

    return devPtr->ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function closes the AT server session on the requested device.
 *
 * @return
 *      - LE_OK             The function succeeded.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 *      - LE_BUSY           The requested device is busy.
 *      - LE_FAULT          Failed to stop the server, check logs for more information.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_Close
(
    le_atServer_DeviceRef_t devRef
        ///< [IN] device to be unbinded
)
{
    DeviceContext_t* devPtr = le_ref_Lookup(DevicesRefMap, devRef);

    if (devPtr == NULL)
    {
        LE_ERROR("Bad reference");
        return LE_BAD_PARAMETER;
    }

    if (devPtr->processing)
    {
        LE_ERROR("Device busy");
        return LE_BUSY;
    }

    return CloseServer(devRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function creates an AT command and registers it into the AT parser.
 *
 * @return
 *      - Reference to the AT command.
 *      - NULL if an error occurs.
 */
//--------------------------------------------------------------------------------------------------
le_atServer_CmdRef_t le_atServer_Create
(
    const char* namePtr
        ///< [IN] AT command name string
)
{
    // Search if the command already exists
    ATCmdSubscribed_t* cmdPtr = le_hashmap_Get(CmdHashMap, namePtr);

    // if the command exists return its reference
    if (cmdPtr)
    {
        LE_INFO("command %s exists", cmdPtr->cmdName);
        return cmdPtr->cmdRef;
    }

    cmdPtr = le_mem_ForceAlloc(AtCommandsPool);
    if (!cmdPtr)
    {
        return NULL;
    }

    LE_DEBUG("Create: %s", namePtr);

    memset(cmdPtr,0,sizeof(ATCmdSubscribed_t));

    le_utf8_Copy(cmdPtr->cmdName, namePtr, LE_ATDEFS_COMMAND_MAX_BYTES,0);

    cmdPtr->cmdRef = le_ref_CreateRef(SubscribedCmdRefMap, cmdPtr);

    le_hashmap_Put(CmdHashMap, cmdPtr->cmdName, cmdPtr);

    cmdPtr->availableDevice = LE_ATSERVER_ALL_DEVICES;
    cmdPtr->paramList = LE_DLS_LIST_INIT;
    cmdPtr->sessionRef = le_atServer_GetClientSessionRef();

    // Check for specific DIAL command
    if (strncmp(namePtr, "ATD", 3) == 0)
    {
        cmdPtr->isDialCommand = true;
    }
    else
    {
        cmdPtr->isDialCommand = false;
    }

    return cmdPtr->cmdRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes an AT command (i.e. unregister from the AT parser).
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to delete the command.
 *      - LE_BUSY          Command is in progress.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_Delete
(
    le_atServer_CmdRef_t commandRef
        ///< [IN] AT command reference
)
{
    ATCmdSubscribed_t* cmdPtr = le_ref_Lookup(SubscribedCmdRefMap, commandRef);

    if (!cmdPtr)
    {
        LE_ERROR("Invalid reference (%p) provided!", commandRef);
        return LE_FAULT;
    }

    if (cmdPtr->processing)
    {
        LE_ERROR("Command in progess");
        return LE_BUSY;
    }

    le_mem_Release(cmdPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Add handler function for EVENT 'le_atServer_Command'
 *
 * This event provides information when the AT command is detected.
 */
//--------------------------------------------------------------------------------------------------
le_atServer_CommandHandlerRef_t le_atServer_AddCommandHandler
(
    le_atServer_CmdRef_t commandRef,
        ///< [IN] AT command reference

    le_atServer_CommandHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr
        ///< [IN]
)
{
    ATCmdSubscribed_t* cmdPtr = le_ref_Lookup(SubscribedCmdRefMap, commandRef);

    if (!cmdPtr)
    {
        LE_ERROR("Bad command reference");
        return NULL;
    }

    if (cmdPtr->handlerFunc)
    {
        LE_INFO("Handler already exists");
        return NULL;
    }

    cmdPtr->handlerFunc = handlerPtr;
    cmdPtr->handlerContextPtr = contextPtr;

    return (le_atServer_CommandHandlerRef_t)(cmdPtr->cmdRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_atServer_Command'
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_RemoveCommandHandler
(
    le_atServer_CommandHandlerRef_t handlerRef
        ///< [IN]
)
{
    if (handlerRef)
    {
        ATCmdSubscribed_t* cmdPtr = le_ref_Lookup(SubscribedCmdRefMap, handlerRef);

        if (cmdPtr)
        {
            cmdPtr->handlerFunc = NULL;
            cmdPtr->handlerContextPtr = NULL;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to get the parameters of a received AT command.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the requested parameter.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_GetParameter
(
    le_atServer_CmdRef_t commandRef,
        ///< [IN] AT command reference

    uint32_t index,
        ///< [IN] agument index

    char* parameter,
        ///< [OUT] parameter value

    size_t parameterNumElements
        ///< [IN]
)
{
    ATCmdSubscribed_t* cmdPtr = le_ref_Lookup(SubscribedCmdRefMap, commandRef);

    if (cmdPtr == NULL)
    {
        LE_ERROR("Bad command reference");
        return LE_FAULT;
    }

    uint32_t numParam = le_dls_NumLinks(&cmdPtr->paramList);
    uint32_t i;
    le_dls_Link_t* linkPtr;

    if (index >= numParam)
    {
        return LE_BAD_PARAMETER;
    }

    linkPtr = le_dls_Peek(&cmdPtr->paramList);

    for(i=0;i<index;i++)
    {
        linkPtr = le_dls_PeekNext(&cmdPtr->paramList, linkPtr);
    }

    if (linkPtr)
    {
        ParamString_t* paramPtr;

        paramPtr = CONTAINER_OF(linkPtr, ParamString_t, link);

        snprintf(parameter, parameterNumElements, "%s", paramPtr->param);

        return LE_OK;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to get the AT command string.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the AT command string.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_GetCommandName
(
    le_atServer_CmdRef_t commandRef,
        ///< [IN] AT command reference

    char* namePtr,
        ///< [OUT] AT command string

    size_t nameNumElements
        ///< [IN]
)
{
    ATCmdSubscribed_t* cmdPtr = le_ref_Lookup(SubscribedCmdRefMap, commandRef);

    if (cmdPtr == NULL)
    {
        LE_ERROR("Bad command reference");
        return LE_FAULT;
    }

    snprintf(namePtr, nameNumElements, "%s", cmdPtr->cmdName);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to get the device reference in use for an AT command specified with
 * its reference.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to get the AT command string.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_GetDevice
(
    le_atServer_CmdRef_t commandRef,
        ///< [IN] AT command reference

    le_atServer_DeviceRef_t* deviceRefPtr
        ///< [OUT] Device reference
)
{
    ATCmdSubscribed_t* cmdPtr = le_ref_Lookup(SubscribedCmdRefMap, commandRef);

    if ((cmdPtr == NULL) || (deviceRefPtr == NULL))
    {
        LE_ERROR("Bad command reference");
        return LE_FAULT;
    }

    if (!cmdPtr->processing)
    {
        LE_ERROR("Command not processing");
        return LE_FAULT;
    }

    *deviceRefPtr = cmdPtr->deviceRef;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to send an intermediate response.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to send the intermediate response.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_SendIntermediateResponse
(
    le_atServer_CmdRef_t commandRef,
        ///< [IN] AT command reference

    const char* intermediateRspPtr
        ///< [IN] Intermediate response to be sent
)
{
    ATCmdSubscribed_t* cmdPtr = le_ref_Lookup(SubscribedCmdRefMap, commandRef);

    if (cmdPtr == NULL)
    {
        LE_ERROR("Bad command reference");
        return LE_FAULT;
    }

    if (!cmdPtr->processing)
    {
        LE_ERROR("Command not processing");
        return LE_FAULT;
    }

    DeviceContext_t* devPtr = le_ref_Lookup(DevicesRefMap, cmdPtr->deviceRef);

    if (devPtr == NULL)
    {
        LE_ERROR("Bad device reference");
        return LE_FAULT;
    }

    RspString_t* rspStringPtr = le_mem_ForceAlloc(RspStringPool);
    le_utf8_Copy(rspStringPtr->resp, intermediateRspPtr, LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);

    SendIntermediateRsp(devPtr, rspStringPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to send the final response.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to send the final response.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_SendFinalResponse
(
    le_atServer_CmdRef_t commandRef,
        ///< [IN] AT command reference

    le_atServer_FinalRsp_t final,
        ///< [IN] Final response to be sent

    bool customStringAvailable,
        ///< [IN]  Custom final response has to be sent
        ///<      instead of the default one.

    const char* finalRspPtr
        ///< [IN] custom final response string
)
{
    ATCmdSubscribed_t* cmdPtr = le_ref_Lookup(SubscribedCmdRefMap, commandRef);

    if (cmdPtr == NULL)
    {
        LE_ERROR("Bad command reference");
        return LE_FAULT;
    }

    DeviceContext_t* devPtr = le_ref_Lookup(DevicesRefMap, cmdPtr->deviceRef);

    if (devPtr == NULL)
    {
        LE_ERROR("Bad device reference");
        return LE_FAULT;
    }

    devPtr->finalRsp.final = final;
    devPtr->finalRsp.customStringAvailable = customStringAvailable;

    if (customStringAvailable)
    {
        le_utf8_Copy( devPtr->finalRsp.resp, finalRspPtr, LE_ATDEFS_RESPONSE_MAX_BYTES, NULL );
    }

    // clean AT command context, not in use now
    le_dls_Link_t* linkPtr;
    while ((linkPtr=le_dls_Pop(&cmdPtr->paramList)) != NULL)
    {
        ParamString_t *paraPtr = CONTAINER_OF(linkPtr, ParamString_t, link);
        le_mem_Release(paraPtr);
    }

    cmdPtr->deviceRef = NULL;
    cmdPtr->processing = false;

    if (final == LE_ATSERVER_OK)
    {
        // Parse next AT commands, if any
        ParseAtCmd(devPtr);
    }
    else
    {
        SendFinalRsp(devPtr);
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function can be used to send the unsolicited response.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to send the unsolicited response.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_SendUnsolicitedResponse
(
    const char* unsolRspPtr,
        ///< [IN] Unsolicited response to be sent

    le_atServer_AvailableDevice_t availableDevice,
        ///< [IN] device to send the unsolicited response

    le_atServer_DeviceRef_t device
        ///< [IN] device reference where the unsolicited
        ///<      response has to be sent
)
{
    if (availableDevice == LE_ATSERVER_SPECIFIC_DEVICE)
    {
        DeviceContext_t* devPtr = le_ref_Lookup(DevicesRefMap, device);

        if (devPtr == NULL)
        {
            LE_ERROR("Bad device reference");
            return LE_FAULT;
        }

        if (SendUnsolicitedResponse(devPtr, unsolRspPtr) != LE_OK)
        {
            return LE_FAULT;
        }
    }
    else
    {
        le_ref_IterRef_t iterRef = le_ref_GetIterator(DevicesRefMap);
        while (le_ref_NextNode(iterRef) == LE_OK)
        {
            DeviceContext_t* devPtr = (DeviceContext_t*) le_ref_GetValue(iterRef);

            if (SendUnsolicitedResponse(devPtr, unsolRspPtr) != LE_OK)
            {
                return LE_FAULT;
            }
        }
    }


    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function enables echo on the selected device.
 *
 * @return
 *      - LE_OK             The function succeeded.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_EnableEcho
(
    le_atServer_DeviceRef_t device
        ///< [IN] device reference

)
{
    DeviceContext_t* devPtr = le_ref_Lookup(DevicesRefMap, device);

    if (devPtr == NULL)
    {
        LE_ERROR("Bad device reference");
        return LE_BAD_PARAMETER;
    }
    else
    {
        devPtr->echo = true;
        return LE_OK;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function disables echo on the selected device.
 *
 * @return
 *      - LE_OK             The function succeeded.
 *      - LE_BAD_PARAMETER  Invalid device reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_DisableEcho
(
    le_atServer_DeviceRef_t device
        ///< [IN] device reference

)
{
    DeviceContext_t* devPtr = le_ref_Lookup(DevicesRefMap, device);

    if (devPtr == NULL)
    {
        LE_ERROR("Bad device reference");
        return LE_BAD_PARAMETER;
    }
    else
    {
        devPtr->echo = false;
        return LE_OK;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function opens a AT commands server bridge.
 * All unknown AT commands will be sent on this alternative file descriptor thanks to the AT client
 * Service.
 *
 * @return
 *      - Reference to the requested bridge.
 *      - NULL if the device can't be bridged
 */
//--------------------------------------------------------------------------------------------------
le_atServer_BridgeRef_t le_atServer_OpenBridge
(
    int fd
        ///< [IN] File descriptor.
)
{
    le_atServer_BridgeRef_t bridgeRef = bridge_Open(fd);

    if (bridgeRef == NULL)
    {
        LE_ERROR("Error during bridge creation");
    }

    return bridgeRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function closes an opened bridge.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to close the bridge.
 *      - LE_BUSY          The bridge is in use (devices references have to be removed first).
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_CloseBridge
(
    le_atServer_BridgeRef_t bridgeRef
        ///< [IN] Bridge refence
)
{
    return bridge_Close(bridgeRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a device to an opened bridge.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_BUSY          The device is already used by the bridge.
 *      - LE_FAULT         The function failed to add the device to the bridge.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_AddDeviceToBridge
(
    le_atServer_DeviceRef_t deviceRef,
        ///< [IN] Device reference to add to the bridge

    le_atServer_BridgeRef_t bridgeRef
        ///< [IN] Bridge refence
)
{
    DeviceContext_t* devPtr = le_ref_Lookup(DevicesRefMap, deviceRef);
    le_result_t res;

    if (devPtr == NULL)
    {
        LE_ERROR("Bad device reference");
        return LE_FAULT;
    }

    if (devPtr->bridgeRef != NULL)
    {
        return LE_BUSY;
    }

    if ( (res = bridge_AddDevice(deviceRef, bridgeRef)) != LE_OK )
    {
        return res;
    }

    devPtr->bridgeRef = bridgeRef;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function removes a device from a bridge
 *
 * @return
 *      - LE_OK            The function succeeded.
        - LE_NOT_FOUND     The device is not isued by the specified bridge
 *      - LE_FAULT         The function failed to add the device to the bridge.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_RemoveDeviceFromBridge
(
    le_atServer_DeviceRef_t deviceRef,
        ///< [IN] Device reference to add to the bridge

    le_atServer_BridgeRef_t bridgeRef
        ///< [IN] Bridge refence
)
{
    DeviceContext_t* devPtr = le_ref_Lookup(DevicesRefMap, deviceRef);

    if (devPtr == NULL)
    {
        LE_ERROR("Bad device reference");
        return LE_FAULT;
    }

    if (devPtr->bridgeRef == NULL)
    {
        // Device not bridge
        LE_ERROR("Device not bridged");
        return LE_FAULT;
    }

    if (devPtr->cmdParser.currentCmdPtr
        && devPtr->cmdParser.currentCmdPtr->processing
        && devPtr->cmdParser.currentCmdPtr->bridgeCmd)
    {
        return LE_BUSY;
    }

    if (bridge_RemoveDevice(deviceRef, bridgeRef) != LE_OK)
    {
        return LE_FAULT;
    }

    devPtr->bridgeRef = NULL;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function enables extended error codes on the selected device.
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_EnableExtendedErrorCodes
(
    void
)
{
    ExtendedErrorCodes = true;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function disables extended error codes on the selected device.
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_DisableExtendedErrorCodes
(
    void
)
{
    ExtendedErrorCodes = false;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function allows the user to register a le_atServer_GetTextCallback_t callback
 * to retrieve text and sends a prompt <CR><LF>><SPACE> on the current command's device.
 *
 * @return
 *      - LE_OK             The function succeeded.
 *      - LE_BAD_PARAMETER  Invalid device or command reference.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_GetTextAsync
(
    le_atServer_CmdRef_t cmdRef,
    le_atServer_GetTextCallbackFunc_t callback,
    void *ctxPtr
)
{
    ATCmdSubscribed_t* cmdPtr = le_ref_Lookup(SubscribedCmdRefMap, cmdRef);

    if (!cmdPtr)
    {
        LE_ERROR("Bad command reference");
        return LE_BAD_PARAMETER;
    }

    DeviceContext_t* devPtr = le_ref_Lookup(DevicesRefMap, cmdPtr->deviceRef);

    if (!devPtr)
    {
        LE_ERROR("Bad device reference");
        return LE_BAD_PARAMETER;
    }

    devPtr->text.mode = true;
    devPtr->text.offset = 0;
    memset(devPtr->text.buf, 0, LE_ATSERVER_TEXT_MAX_LEN);
    devPtr->text.callback = callback;
    devPtr->text.ctxPtr = ctxPtr;
    devPtr->text.cmdRef = cmdRef;

    LE_INFO("%s", devPtr->text.buf);

    le_dev_Write(&devPtr->device, (uint8_t *)TEXT_PROMPT, TEXT_PROMPT_LEN);

    cmdPtr->processing = false;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The COMPONENT_INIT intialize the AT Server Component when Legato start
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Device pool allocation
    DevicesPool = le_mem_CreatePool("ATServerDevicesPool",sizeof(DeviceContext_t));
    le_mem_ExpandPool(DevicesPool,DEVICE_POOL_SIZE);
    DevicesRefMap = le_ref_CreateMap("DevicesRefMap", DEVICE_POOL_SIZE);

    // AT commands pool allocation
    AtCommandsPool = le_mem_CreatePool("AtServerCommandsPool",sizeof(ATCmdSubscribed_t));
    le_mem_ExpandPool(AtCommandsPool, CMD_POOL_SIZE);
    le_mem_SetDestructor(AtCommandsPool,AtCmdPoolDestructor);
    SubscribedCmdRefMap = le_ref_CreateMap("SubscribedCmdRefMap", CMD_POOL_SIZE);
    CmdHashMap = le_hashmap_Create("CmdHashMap",
                                    CMD_POOL_SIZE,
                                    le_hashmap_HashString,
                                    le_hashmap_EqualsString
                                   );

    // Parameters pool allocation
    ParamStringPool = le_mem_CreatePool("ParamStringPool",sizeof(ParamString_t));
    le_mem_ExpandPool(ParamStringPool,PARAM_POOL_SIZE);

    // Parameters pool allocation
    RspStringPool = le_mem_CreatePool("RspStringPool",sizeof(RspString_t));
    le_mem_ExpandPool(RspStringPool,RSP_POOL_SIZE);

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler(
        le_atServer_GetServiceRef(), CloseSessionEventHandler, NULL);

    bridge_Init();
}
