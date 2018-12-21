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
#include "watchdogChain.h"

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
 * User-defined error strings pool size
 */
//--------------------------------------------------------------------------------------------------
#define USER_ERROR_POOL_SIZE       50

//--------------------------------------------------------------------------------------------------
/**
 * Number of standard error strings defined in 3GPP TS 27.007 9.2 and 3GPP TS 27.005 3.2.5
 */
//--------------------------------------------------------------------------------------------------
#define STD_ERROR_CODE_SIZE       512

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
 * ASCII line feed code.
 */
//--------------------------------------------------------------------------------------------------
#define NEWLINE             0x0a

//--------------------------------------------------------------------------------------------------
/**
 * ASCII backspace code.
 */
//--------------------------------------------------------------------------------------------------
#define BACKSPACE           0x08

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 8

//--------------------------------------------------------------------------------------------------
/**
 * Error codes modes enum
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    MODE_DISABLED,
    MODE_EXTENDED,
    MODE_VERBOSE
}
ErrorCodesMode_t;

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
 * Pool for user-defined error codes
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t UserErrorPool;

//--------------------------------------------------------------------------------------------------
/**
 * Map for user-defined error codes
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t UserErrorRefMap;

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
 * Error codes current mode
 */
//--------------------------------------------------------------------------------------------------
static ErrorCodesMode_t ErrorCodesMode = MODE_DISABLED;

//--------------------------------------------------------------------------------------------------
/**
 * Pre-formatted strings corresponding to AT commands +CME error codes
 * (see 3GPP TS 27.007 9.2)
 *
 */
//--------------------------------------------------------------------------------------------------
const char* const CmeErrorCodes[STD_ERROR_CODE_SIZE] =
{
    ///< 3GPP TS 27.007 §9.2.1: General errors
    [0]   =   "Phone failure",
    [1]   =   "No connection to phone",
    [2]   =   "Phone-adaptor link reserved",
    [3]   =   "Operation not allowed",
    [4]   =   "Operation not supported",
    [5]   =   "PH-SIM PIN required",
    [6]   =   "PH-FSIM PIN required",
    [7]   =   "PH-FSIM PUK required",
    [10]  =   "SIM not inserted",
    [11]  =   "SIM PIN required",
    [12]  =   "SIM PUK required",
    [13]  =   "SIM failure",
    [14]  =   "SIM busy",
    [15]  =   "SIM wrong",
    [16]  =   "Incorrect password",
    [17]  =   "SIM PIN2 required",
    [18]  =   "SIM PUK2 required",
    [20]  =   "Memory full",
    [21]  =   "Invalid index",
    [22]  =   "Not found",
    [23]  =   "Memory failure",
    [24]  =   "Text string too long",
    [25]  =   "Invalid characters in text string",
    [26]  =   "Dial string too long",
    [27]  =   "Invalid characters in dial string",
    [30]  =   "No network service",
    [31]  =   "Network timeout",
    [32]  =   "Network not allowed - emergency calls only",
    [40]  =   "Network personalization PIN required",
    [41]  =   "Network personalization PUK required",
    [42]  =   "Network subset personalization PIN required",
    [43]  =   "Network subset personalization PUK required",
    [44]  =   "Service provider personalization PIN required",
    [45]  =   "Service provider personalization PUK required",
    [46]  =   "Corporate personalization PIN required",
    [47]  =   "Corporate personalization PUK required",
    [48]  =   "Hidden key required",
    [49]  =   "EAP method not supported",
    [50]  =   "Incorrect parameters",
    [51]  =   "Command implemented but currently disabled",
    [52]  =   "Command aborted by user",
    [53]  =   "Not attached to network due to MT functionality restrictions",
    [54]  =   "Modem not allowed - MT restricted to emergency calls only",
    [55]  =   "Operation not allowed because of MT functionality restrictions",
    [56]  =   "Fixed dial number only allowed - called number is not a fixed dial number",
    [57]  =   "Temporarily out of service due to other MT usage",
    [58]  =   "Language/alphabet not supported",
    [59]  =   "Unexpected data value",
    [60]  =   "System failure",
    [61]  =   "Data missing",
    [62]  =   "Call barred",
    [63]  =   "Message waiting indication subscription failure",
    [100] =   "Unknown",

    ///< 3GPP TS 27.007 §9.2.2.1: GPRS and EPS errors related to a failure to perform an attach
    [103] =   "Illegal MS",
    [106] =   "Illegal ME",
    [107] =   "GPRS services not allowed",
    [108] =   "GPRS services and non-GPRS services not allowed",
    [111] =   "PLMN not allowed",
    [112] =   "Location area not allowed",
    [113] =   "Roaming not allowed in this location area",
    [114] =   "GPRS services not allowed in this PLMN",
    [115] =   "No Suitable Cells In Location Area",
    [122] =   "Congestion",
    [125] =   "Not authorized for this CSG",
    [172] =   "Semantically incorrect message",
    [173] =   "Mandatory information element error",
    [174] =   "Information element non-existent or not implemented",
    [175] =   "Conditional IE error",
    [176] =   "Protocol error, unspecified",

    ///< 3GPP TS 27.007 §9.2.2.2: GPRS and EPS errors related to a failure to activate a context
    [177] =   "Operator Determined Barring",
    [126] =   "Insufficient resources",
    [127] =   "Missing or unknown APN",
    [128] =   "Unknown PDP address or PDP type",
    [129] =   "User authentication failed",
    [130] =   "Activation rejected by GGSN, Serving GW or PDN GW",
    [131] =   "Activation rejected, unspecified",
    [132] =   "Service option not supported",
    [133] =   "Requested service option not subscribed",
    [134] =   "Service option temporarily out of order",
    [140] =   "Feature not supported",
    [141] =   "Semantic error in the TFT operation",
    [142] =   "Syntactical error in the TFT operation",
    [143] =   "Unknown PDP context",
    [144] =   "Semantic errors in packet filter(s)",
    [145] =   "Syntactical errors in packet filter(s)",
    [146] =   "PDP context without TFT already activated",
    [149] =   "PDP authentication failure",
    [178] =   "Maximum number of PDP contexts reached",
    [179] =   "Requested APN not supported in current RAT and PLMN combination",
    [180] =   "Request rejected, Bearer Control Mode violation",
    [181] =   "Unsupported QCI value",

    ///< 3GPP TS 27.007 §9.2.2.2: GPRS and EPS errors related to a failure to disconnect a PDN
    [171] =   "Last PDN disconnection not allowed",

    ///< 3GPP TS 27.007 §9.2.2.4: Other GPRS errors
    [148] =   "Unspecified GPRS error",
    [150] =   "Invalid mobile class",
    [182] =   "User data transmission via control plane is congested",

    ///< 3GPP TS 27.007 §9.2.3: VBS, VGCS and eMLPP-related errors
    [151] =   "VBS/VGCS not supported by the network",
    [152] =   "No service subscription on SIM",
    [153] =   "No subscription for group ID",
    [154] =   "Group Id not activated on SIM",
    [155] =   "No matching notification",
    [156] =   "VBS/VGCS call already present",
    [157] =   "Congestion",
    [158] =   "Network failure",
    [159] =   "Uplink busy",
    [160] =   "No access rights for SIM file",
    [161] =   "No subscription for priority",
    [162] =   "Operation not applicable or not possible",
    [163] =   "Group Id prefixes not supported",
    [164] =   "Group Id prefixes not usable for VBS",
    [165] =   "Group Id prefix value invalid",
};

//--------------------------------------------------------------------------------------------------
/**
 * Pre-formatted strings corresponding to AT commands +CMS error codes
 * (see 3GPP TS 27.005 3.2.5, 3GPP TS 24.011 E-2 and 3GPP TS 23.040 9.2.3.22)
 *
 */
const char* const CmsErrorCodes[STD_ERROR_CODE_SIZE] =
{
    ///< 3GPP TS 24.011 §E-2:  RP-cause definition mobile originating SM-transfer
    [1]    = "Unassigned (unallocated) number",
    [8]    =  "Operator determined barring",
    [10]   = "Call barred",
    [21]   = "Short message transfer rejected",
    [27]   = "Destination out of service",
    [28]   = "Unidentified subscriber",
    [29]   = "Facility rejected",
    [30]   = "Unknown subscriber",
    [38]   = "Network out of order",
    [41]   = "Temporary failure",
    [42]   = "Congestion",
    [47]   = "Resources unavailable, unspecified",
    [50]   = "Requested facility not subscribed",
    [69]   = "Requested facility not implemented",
    [81]   = "Invalid short message transfer reference value",
    [95]   = "Invalid message, unspecified",
    [96]   = "Invalid mandatory information",
    [97]   = "Message type non-existent or not implemented",
    [98]   = "Message not compatible with short message protocol state",
    [99]   = "Information element non-existent or not implemented",
    [111]  = "Protocol error, unspecified",
    [17]   = "Network failure",
    [22]   = "Congestion",
    [127]  = "Interworking, unspecified",

    ///< 3GPP TS 23.040 §9.2.3.22: TP-Failure-Cause
    [128]   = "Telematic interworking not supported",
    [129]   = "Short message Type 0 not supported",
    [130]   = "Cannot replace short message",
    [143]   = "Unspecified TP-PID error",
    [144]   = "Data coding scheme (alphabet) not supported",
    [145]   = "Message class not supported",
    [159]   = "Unspecified TP-DCS error",
    [160]   = "Command cannot be actioned",
    [161]   = "Command unsupported",
    [175]   = "Unspecified TP-Command error",
    [176]   = "TPDU not supported",
    [192]   = "SC busy",
    [193]   = "No SC subscription",
    [194]   = "SC system failure ",
    [195]   = "Invalid SME address",
    [196]   = "Destination SME barred",
    [197]   = "SM Rejected-Duplicate SM",
    [198]   = "TP-VPF not supported",
    [199]   = "TP-VP not supported",
    [208]   = "(U)SIM SMS storage full",
    [209]   = "No SMS storage capability in (U)SIM",
    [210]   = "Error in MS",
    [211]   = "Memory Capacity Exceeded",
    [212]   = "(U)SIM Application Toolkit Busy",
    [213]   = "(U)SIM data download error",
    [255]   = "Unspecified error cause",

    ///< 3GPP TS 27.005 §3.2.5: Message service failure errors
    [300] =   "ME failure",
    [301] =   "SMS service of ME reserved",
    [302] =   "Operation not allowed",
    [303] =   "Operation not supported",
    [304] =   "Invalid PDU mode parameter",
    [305] =   "Invalid text mode parameter",
    [310] =   "(U)SIM not inserted",
    [311] =   "(U)SIM PIN required",
    [312] =   "PH-(U)SIM PIN required",
    [313] =   "(U)SIM failure",
    [314] =   "(U)SIM busy",
    [315] =   "(U)SIM wrong",
    [316] =   "(U)SIM PUK required",
    [317] =   "(U)SIM PIN2 required",
    [318] =   "(U)SIM PUK2 required",
    [320] =   "Memory failure",
    [321] =   "Invalid memory index",
    [322] =   "Memory full",
    [330] =   "SMSC address unknown",
    [331] =   "No network service",
    [332] =   "Network timeout",
    [340] =   "No +CNMA acknowledgement expected",
    [500] =   "Unknown error",
};

//--------------------------------------------------------------------------------------------------
/**
 * Structure used to hold user-defined error codes
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_atServer_ErrorCodeRef_t ref;                                      ///< Ref of the error code
    uint32_t                   errorCode;                                ///< Error code identifier
    char                       pattern[LE_ATDEFS_RESPONSE_MAX_BYTES];    ///< Response prefix
    char                       verboseMsg[LE_ATDEFS_RESPONSE_MAX_BYTES]; ///< Verbose message
}
UserErrorCode_t;

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
    PARSE_READ_PARAM,
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
 * Text processing state.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    CONTINUE,
    END_OF_LINE,
    CANCEL,
    INVALID_CHARACTER,
    INVALID_SEQUENCE,
}
TextProcessingState_t;

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
    void*                   modemCmdDescRefPtr;                     ///< modem desc reference
    le_msg_SessionRef_t     sessionRef;                             ///< session reference
    bool                    isDialCommand;                          ///< specific dial command
    bool                    isBasicCommand;                         ///< is a basic format command
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
    le_atServer_FinalRsp_t final;                                 ///< Final result code
    uint32_t               errorCode;                             ///< Final error code
    char                   pattern[LE_ATDEFS_RESPONSE_MAX_BYTES]; ///< Prefix to the return string
    bool                   customStringAvailable;                 ///< Custom string available(kept
                                                                  ///  for legacy purpose)
    char                   resp[LE_ATDEFS_RESPONSE_MAX_BYTES];    ///< Response string
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
    ssize_t                             offset;                         ///< Buffer offset
    char                                buf[LE_ATDEFS_TEXT_MAX_BYTES];  ///< Text buffer
    le_atServer_GetTextCallbackFunc_t   callback;                       ///< Callback function
    void*                               ctxPtr;                         ///< Context
    le_atServer_CmdRef_t                cmdRef;                         ///< Received AT command
    le_result_t                         result;                         ///< Text processing result
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
static le_result_t ParseReadParam(CmdParser_t* cmdParserPtr);
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
                             ParseError,                      /*PARSE_READ_PARAM*/
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
                             ParseError,                      /*PARSE_READ_PARAM*/
                             ParseParam,                      /*PARSE_COMMA*/
                             ParseError,                      /*PARSE_SEMICOLON*/
                             ParseError,                      /*PARSE_BASIC*/
                             ParseError,                      /*PARSE_BASIC_PARAM*/
                             ParseError,                      /*PARSE_BASIC_END*/
                             ParseNone                        /*PARSE_LAST*/
                         },
/*PARSE_QUESTIONMARK*/   {   ParseError,                      /*PARSE_CMDNAME*/
                             ParseError,                      /*PARSE_EQUAL*/
                             ParseError,                      /*PARSE_QUESTIONMARK*/
                             ParseReadParam,                  /*PARSE_READ_PARAM*/
                             ParseError,                      /*PARSE_COMMA*/
                             ParseSemicolon,                  /*PARSE_SEMICOLON*/
                             ParseError,                      /*PARSE_BASIC*/
                             ParseError,                      /*PARSE_BASIC_PARAM*/
                             ParseError,                      /*PARSE_BASIC_END*/
                             ParseError                       /*PARSE_LAST*/
                         },
/*PARSE_READ_PARAM*/     {   ParseError,                      /*PARSE_CMDNAME*/
                             ParseError,                      /*PARSE_EQUAL*/
                             ParseError,                      /*PARSE_QUESTIONMARK*/
                             ParseError,                      /*PARSE_READ_PARAM*/
                             ParseError,                      /*PARSE_COMMA*/
                             ParseSemicolon,                  /*PARSE_SEMICOLON*/
                             ParseError,                      /*PARSE_BASIC*/
                             ParseError,                      /*PARSE_BASIC_PARAM*/
                             ParseError,                      /*PARSE_BASIC_END*/
                             ParseError                       /*PARSE_LAST*/
                         },
/*PARSE_COMMA*/          {   ParseError,                      /*PARSE_CMDNAME*/
                             ParseError,                      /*PARSE_EQUAL*/
                             ParseError,                      /*PARSE_QUESTIONMARK*/
                             ParseError,                      /*PARSE_READ_PARAM*/
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
                             ParseError,                      /*PARSE_READ_PARAM*/
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
                             ParseError,                      /*PARSE_READ_PARAM*/
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
                             ParseError,                      /*PARSE_READ_PARAM*/
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
                             ParseError,                      /*PARSE_READ_PARAM*/
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
                             ParseError,                      /*PARSE_READ_PARAM*/
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

    LE_DEBUG("AT command destructor for '%s'", cmdPtr->cmdName);

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
    const char* rspPtr
)
{
    char string[LE_ATDEFS_RESPONSE_MAX_BYTES+4] = {0};

    if ((devPtr->rspState == AT_RSP_FINAL) || (devPtr->rspState == AT_RSP_UNSOLICITED) ||
        ((devPtr->rspState == AT_RSP_INTERMEDIATE) && devPtr->isFirstIntermediate))
    {
        snprintf(string, LE_ATDEFS_RESPONSE_MAX_BYTES+4, "\r\n%s\r\n", rspPtr);
        devPtr->isFirstIntermediate = false;
    }
    else
    {
        snprintf(string, LE_ATDEFS_RESPONSE_MAX_BYTES+2, "%s\r\n", rspPtr);
    }

    le_dev_Write(&devPtr->device, (uint8_t*)string, strlen(string));
}

//--------------------------------------------------------------------------------------------------
/**
 * Get the pointer of a custom error code using its error code and pattern
 *
 * @return
  *      - UserErrorCode_t Pointer to the error code, NULL if it doesn't exist
 */
//--------------------------------------------------------------------------------------------------
static UserErrorCode_t* GetCustomErrorCode
(
    uint32_t errorCode,
         ///< [IN] Numerical error code

    const char* patternPtr
        ///< [IN] Prefix of the final response string
)
{
    le_ref_IterRef_t iter;
    UserErrorCode_t* errorCodePtr = NULL;

    if (NULL == patternPtr)
    {
        return NULL;
    }

    iter = le_ref_GetIterator(UserErrorRefMap);
    while (LE_OK == le_ref_NextNode(iter))
    {
        errorCodePtr = (UserErrorCode_t*)le_ref_GetValue(iter);
        if (errorCodePtr)
        {
            if ((errorCode == errorCodePtr->errorCode) &&
                (0 == strncmp(patternPtr, errorCodePtr->pattern, sizeof(errorCodePtr->pattern))))
            {
                return errorCodePtr;
            }
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * Get standard verbose message code
 *
 * @return
 *      - char* pointer to the verbose message
 *      - NULL if unable to retreive a verbose message
 */
//--------------------------------------------------------------------------------------------------
static const char* GetStdVerboseMsg
(
    uint32_t errorCode,
         ///< [IN] Numerical error code

    const char* patternPtr
        ///< [IN] Prefix of the final response string
)
{
    if (errorCode >= STD_ERROR_CODE_SIZE)
    {
        return NULL;
    }

    if (0 == strcmp(patternPtr, LE_ATDEFS_CME_ERROR))
    {
        return CmeErrorCodes[errorCode];
    }

    if (0 == strcmp(patternPtr, LE_ATDEFS_CMS_ERROR))
    {
        return CmsErrorCodes[errorCode];
    }

    LE_DEBUG("Not a standard pattern");
    return NULL;
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
    UserErrorCode_t* errorCodePtr;
    size_t patternLength = 0;

    devPtr->rspState = AT_RSP_FINAL;

    // This check is kept for legacy purposes since le_atServer_SendFinalResponse() is deprecated
    // but still in use
    if (devPtr->finalRsp.customStringAvailable && (MODE_DISABLED != ErrorCodesMode))
    {
        LE_DEBUG("Custom string mode");
        SendRspString(devPtr, devPtr->finalRsp.resp);
        goto end_processing;
    }

    patternLength = strnlen(devPtr->finalRsp.pattern, sizeof(devPtr->finalRsp.pattern));

    // This check is kept for legacy compatibility with old API. When a pattern is introducted
    // and the final response is not an error, we use it as a custom string
    if ((LE_ATSERVER_ERROR != devPtr->finalRsp.final) && patternLength)
    {
        SendRspString(devPtr, devPtr->finalRsp.pattern);
        goto end_processing;
    }

    switch (devPtr->finalRsp.final)
    {
        case LE_ATSERVER_OK:
            snprintf(devPtr->finalRsp.resp, LE_ATDEFS_RESPONSE_MAX_BYTES,"OK");
            break;

        case LE_ATSERVER_NO_CARRIER:
            snprintf(devPtr->finalRsp.resp, LE_ATDEFS_RESPONSE_MAX_BYTES,"NO CARRIER");
            break;

        case LE_ATSERVER_NO_DIALTONE:
            snprintf(devPtr->finalRsp.resp, LE_ATDEFS_RESPONSE_MAX_BYTES,"NO DIALTONE");
            break;

        case LE_ATSERVER_BUSY:
            snprintf(devPtr->finalRsp.resp, LE_ATDEFS_RESPONSE_MAX_BYTES,"BUSY");
            break;

        case LE_ATSERVER_ERROR:
            if ((MODE_DISABLED == ErrorCodesMode) || (0 == patternLength))
            {
                snprintf(devPtr->finalRsp.resp, LE_ATDEFS_RESPONSE_MAX_BYTES,"ERROR");
                break;
            }

            // Build the response string [pattern + error code] or [pattern + verbose msg]
            {
                int sizeMax = sizeof(devPtr->finalRsp.resp);
                if (sizeMax > 0)
                {
                    strncpy(devPtr->finalRsp.resp, devPtr->finalRsp.pattern, sizeMax - 1);
                    // Make devPtr->finalRsp.resp string null terminated if devPtr->finalRsp.pattern
                    // string size is bigger than sizeMax - 1.
                    devPtr->finalRsp.resp[sizeMax-1] = '\0';
                }
            }

            if (MODE_EXTENDED == ErrorCodesMode)
            {
                LE_DEBUG("Extended mode");
                snprintf(devPtr->finalRsp.resp + patternLength,
                         LE_ATDEFS_RESPONSE_MAX_BYTES - patternLength,
                         "%d",
                         devPtr->finalRsp.errorCode);
            }
            else if (MODE_VERBOSE == ErrorCodesMode)
            {
                LE_DEBUG("Verbose mode");
                if (devPtr->finalRsp.errorCode < STD_ERROR_CODE_SIZE)
                {
                    const char* msgPtr = GetStdVerboseMsg(devPtr->finalRsp.errorCode,
                                                          devPtr->finalRsp.pattern);
                    if (NULL != msgPtr)
                    {
                        strncpy(devPtr->finalRsp.resp + patternLength,
                                msgPtr,
                                LE_ATDEFS_RESPONSE_MAX_BYTES - patternLength);
                    }
                    else
                    {
                        snprintf(devPtr->finalRsp.resp + patternLength,
                                 LE_ATDEFS_RESPONSE_MAX_BYTES - patternLength,
                                 "%d",
                                 devPtr->finalRsp.errorCode);
                    }
                }
                else
                {
                    errorCodePtr = GetCustomErrorCode(devPtr->finalRsp.errorCode,
                                                      devPtr->finalRsp.pattern);
                    if (errorCodePtr)
                    {
                        strncpy(devPtr->finalRsp.resp + patternLength,
                                errorCodePtr->verboseMsg,
                                LE_ATDEFS_RESPONSE_MAX_BYTES - patternLength);
                    }
                    else
                    {
                        snprintf(devPtr->finalRsp.resp + patternLength,
                                 LE_ATDEFS_RESPONSE_MAX_BYTES - patternLength,
                                 "%d",
                                 devPtr->finalRsp.errorCode);
                    }
                }
            }
            break;

        default:
            break;
    }
    SendRspString(devPtr, devPtr->finalRsp.resp);

end_processing:
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

        SendRspString(devPtr, rspStringPtr->resp);
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

    SendRspString(devPtr, rspStringPtr->resp);
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
        SendRspString(devPtr, rspStringPtr->resp);
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
    char*        atCmdPtr,
    le_atServer_BridgeRef_t bridgeRef
)
{
    void* cmdDescRef = NULL;

    if ((bridge_Create(atCmdPtr, &cmdDescRef) != LE_OK )||(NULL == cmdDescRef))
    {
        LE_ERROR("Error in AT command creation");
        return LE_FAULT;
    }

    cmdParserPtr->currentCmdPtr = le_hashmap_Get(CmdHashMap,
                                                 atCmdPtr);

    if ( cmdParserPtr->currentCmdPtr == NULL )
    {
        LE_ERROR("At command still not exists");
        bridge_ReleaseModemCmd(cmdDescRef);
        return LE_FAULT;
    }

    // AT command is created by bridge device, save the session reference
    // for command removal tracking.
    le_msg_SessionRef_t  sessionRef = NULL;
    if (bridge_GetSessionRef(bridgeRef, &sessionRef) == LE_OK)
    {
        (cmdParserPtr->currentCmdPtr)->sessionRef = sessionRef;
    }
    else
    {
        LE_ERROR("Failed to get the session reference of the bridge device");
        bridge_ReleaseModemCmd(cmdDescRef);
        return LE_FAULT;
    }

    (cmdParserPtr->currentCmdPtr)->bridgeCmd = true;
    (cmdParserPtr->currentCmdPtr)->modemCmdDescRefPtr = cmdDescRef;

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
                if (( CreateModemCommand(cmdParserPtr,
                                         cmdParserPtr->currentAtCmdPtr,
                                         devPtr->bridgeRef) != LE_OK ) ||
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

        cmdParserPtr->currentCharPtr++;
        if ((cmdParserPtr->currentCmdPtr->isBasicCommand) &&
            (cmdParserPtr->currentCharPtr <=  cmdParserPtr->lastCharPtr) &&
            (*cmdParserPtr->currentCharPtr != AT_TOKEN_SEMICOLON))
        {
            ParseBasicEnd(cmdParserPtr);
            return res;
        }
        cmdParserPtr->currentCharPtr--;
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
                if (index < sizeof(paramPtr->param) -1)
                {
                    paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
                }
                else
                {
                    return LE_OVERFLOW;
                }
            }
        }
        else
        {
            if ((tokenQuote) || ( IS_NUMBER(*cmdParserPtr->currentCharPtr) ))
            {
                if (index < sizeof(paramPtr->param) -1)
                {
                    paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
                }
                else
                {
                    return LE_OVERFLOW;
                }
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

                if (index < sizeof(paramPtr->param) -1)
                {
                    paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
                }
                else
                {
                    return LE_OVERFLOW;
                }
            }
            else
            {
                if (tokenQuote)
                {
                    if (index < sizeof(paramPtr->param) -1)
                    {
                        paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
                    }
                    else
                    {
                        return LE_OVERFLOW;
                    }
                }
                else
                {
                    if ( (*cmdParserPtr->currentCharPtr == 'i') ||
                         ( *cmdParserPtr->currentCharPtr == 'g') )
                    {
                        if (index < sizeof(paramPtr->param) -1)
                        {
                            paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
                        }
                        else
                        {
                            return LE_OVERFLOW;
                        }
                    }
                    else
                    {
                        if (index < sizeof(paramPtr->param) -1)
                        {
                            paramPtr->param[index++] = toupper(*cmdParserPtr->currentCharPtr);
                        }
                        else
                        {
                            return LE_OVERFLOW;
                        }
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
                        if (index < sizeof(paramPtr->param) -1)
                        {
                            paramPtr->param[index++] = *testCharPtr;
                        }
                        else
                        {
                            return LE_OVERFLOW;
                        }
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
    cmdParserPtr->currentCmdPtr->isBasicCommand = true;
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
            ( !IS_QUOTE(*cmdParserPtr->currentCharPtr) ) &&
            (*cmdParserPtr->currentCharPtr != AT_TOKEN_SEMICOLON))
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

        if (( CreateModemCommand(cmdParserPtr,
                                 atCmd,
                                 devPtr->bridgeRef) != LE_OK ) ||
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
 * AT parser transition (treat a parameter for extended format parameter commands)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseParam
(
    CmdParser_t* cmdParserPtr
)
{
    uint32_t index = 0;
    bool tokenQuote = false;
    bool loop = true;
    ParamString_t* paramPtr = le_mem_ForceAlloc(ParamStringPool);
    memset(paramPtr, 0, sizeof(ParamString_t));

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
                if (index < sizeof(paramPtr->param) -1)
                {
                    paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
                }
                else
                {
                    return LE_OVERFLOW;
                }
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
                if (index < sizeof(paramPtr->param) -1)
                {
                    paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
                }
                else
                {
                    return LE_OVERFLOW;
                }
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
 * AT parser transition (treat a parameter for extended format read command)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t ParseReadParam
(
    CmdParser_t* cmdParserPtr
)
{
    if (cmdParserPtr->currentCmdPtr != NULL)
    {
        // For AT extended format read command like "AT+<command>?[<value>]", we put <value>
        // into the parameter 0 if exists.
        uint32_t index = 0;
        bool loop = true;
        ParamString_t* paramPtr = le_mem_ForceAlloc(ParamStringPool);
        memset(paramPtr, 0, sizeof(ParamString_t));

        // Go through paramater buffers until ";" or last char.
        while (loop)
        {
            if (index < sizeof(paramPtr->param) -1)
            {
                paramPtr->param[index++] = *cmdParserPtr->currentCharPtr;
            }
            else
            {
                return LE_OVERFLOW;
            }

            cmdParserPtr->currentCharPtr++;

            if (( cmdParserPtr->currentCharPtr > cmdParserPtr->lastCharPtr ) ||
            ( *cmdParserPtr->currentCharPtr == AT_TOKEN_SEMICOLON ))
            {
                loop = false;
                cmdParserPtr->currentCharPtr--;
            }
        }
        le_dls_Queue(&(cmdParserPtr->currentCmdPtr->paramList),&(paramPtr->link));
        return LE_OK;
    }
    return LE_FAULT;
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
                    (cmdParserPtr->currentCmdPtr->type == LE_ATSERVER_TYPE_READ))
                {
                    // For AT extended read command, we need check its parameter.
                    // Here we didn't follow state PARSE_COMMA because the parameter
                    // format is incompatible.
                    cmdParserPtr->cmdParser = PARSE_READ_PARAM;
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

            // Incurred error in parsing AT command. Clear all parsed parameters.
            if (cmdParserPtr->currentCmdPtr)
            {
                ATCmdSubscribed_t* cmdPtr = cmdParserPtr->currentCmdPtr;
                le_dls_Link_t* linkPtr;
                while ((linkPtr = le_dls_Pop(&cmdPtr->paramList)) != NULL)
                {
                    ParamString_t *paraPtr = CONTAINER_OF(linkPtr, ParamString_t, link);
                    le_mem_Release(paraPtr);
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

            // Clean AT command context, not in use now
            le_dls_Link_t* linkPtr;
            while ((linkPtr=le_dls_Pop(&cmdPtr->paramList)) != NULL)
            {
                ParamString_t *paraPtr = CONTAINER_OF(linkPtr, ParamString_t, link);
                le_mem_Release(paraPtr);
            }

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

                        ssize_t offset = strnlen(devPtr->cmdParser.foundCmd,
                                                 sizeof(devPtr->cmdParser.foundCmd)) - 1;
                        if((offset >= 0) && (offset < sizeof(devPtr->cmdParser.foundCmd)))
                        {
                            devPtr->cmdParser.lastCharPtr = devPtr->cmdParser.foundCmd + offset;

                            devPtr->cmdParser.currentCharPtr = devPtr->cmdParser.foundCmd;
                            devPtr->cmdParser.currentAtCmdPtr = devPtr->cmdParser.foundCmd;

                            ParseAtCmd(devPtr);
                        }
                        // It's possible that non-ASCII char is detected because of line error
                        // which casues string length to zero. In this case we assume it's an
                        // illegal command.
                        else
                        {
                            LE_WARN("Illegal command detected!");
                            SendRspString(devPtr, "ERROR");
                            devPtr->processing = false;
                        }
                    }
                    else
                    {
                        LE_WARN("Command in progress");
                        SendRspString(devPtr, "ERROR");
                    }

                    devPtr->parseIndex=0;
                    devPtr->cmdParser.rxState = PARSER_SEARCH_A;
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
        SendRspString(devPtr, "ERROR");
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

    // Value of size is negative.
    if (0 > size)
    {
        LE_ERROR("le_dev_Read failed!");
        return;
    }
    // Value of size is 0.
    else if (0 == size)
    {
        LE_DEBUG("Read data size 0.");
        return;
    }

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
 * This function removes a backspace and the character before it
 *
 * @return
 *      modified string
 */
//--------------------------------------------------------------------------------------------------
static char* RemoveBackspace
(
    char* strPtr
)
{
    int i = 0, ch = 0;

    if (!strPtr)
    {
        LE_ERROR("null string");
        return NULL;
    }

    while (*strPtr)
    {
        if (*strPtr == '\b')
        {
            while (*strPtr)
            {
                if (ch)
                {
                    *(strPtr - 1) = *(strPtr + 1);
                }
                else
                {
                    *strPtr = *(strPtr + 1);
                }
                strPtr++;
                i++;
            }
            break;
        }
        strPtr++;
        i++;
        ch++;
    }

    return (strPtr - i);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function processes received text buffer
 *
 * @return
 *      processing state.
 */
//--------------------------------------------------------------------------------------------------
static TextProcessingState_t ProcessText
(
    Text_t*     textPtr,
    ssize_t     count,
    Device_t*   dev
)
{
    char* copyPtr = textPtr->buf + (textPtr->offset - count);
    TextProcessingState_t state = CONTINUE;

    textPtr->result = LE_OK;

    while (*copyPtr)
    {
        if (state)
        {
            LE_ERROR("Invalid sequence");
            state = INVALID_SEQUENCE;
            break;
        }
        switch (*copyPtr)
        {
            case NEWLINE:
                LE_DEBUG("Linefeed");
                le_dev_Write(dev, (uint8_t *)TEXT_PROMPT, TEXT_PROMPT_LEN);
                break;
            case ESCAPE:
                LE_DEBUG("Cancel request");
                state = CANCEL;
                break;
            case SUBSTITUTE:
                LE_DEBUG("End of text");
                state = END_OF_LINE;
                break;
            default:
                if (!isprint(*copyPtr))
                {
                    LE_ERROR("Invalid character");
                    state = INVALID_CHARACTER;
                }
                break;
        }
        copyPtr++;
    }

    switch (state)
    {
        case CANCEL:
            memset(textPtr->buf, 0, textPtr->offset);
            textPtr->offset = 0;
            break;
        case INVALID_CHARACTER:
        case INVALID_SEQUENCE:
            memset(textPtr->buf, 0, textPtr->offset);
            textPtr->offset = 0;
            textPtr->result = LE_FORMAT_ERROR;
            break;
        case END_OF_LINE:
            textPtr->buf[--textPtr->offset] = 0;
            break;
        default:
            break;
    }

    return state;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function handles text receiving
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
    Text_t* textPtr;
    char* bufPtr;
    void* ctxPtr;

    textPtr = &devPtr->text;
    bufPtr = textPtr->buf + textPtr->offset;
    size = LE_ATDEFS_TEXT_MAX_LEN - textPtr->offset;
    device = devPtr->device;
    ctxPtr = NULL;

    if (textPtr->ctxPtr)
    {
        ctxPtr = textPtr->ctxPtr;
    }

    count = le_dev_Read(&device, (uint8_t *)bufPtr, size);
    if (count <= 0)
    {
        LE_ERROR("connection closed");
        if (textPtr->callback)
        {
            textPtr->callback(textPtr->cmdRef, LE_IO_ERROR, "", 0, ctxPtr);
        }
        textPtr->mode = false;
        return;
    }

    // Ensure the string point to by bufPtr is null-terminated
    bufPtr[size] = '\0';

    while (strchr((const char *)bufPtr, '\b'))
    {
        bufPtr = RemoveBackspace(bufPtr);
        if (!bufPtr)
        {
            LE_ERROR("Failed to remove backspaces");
            textPtr->callback(textPtr->cmdRef, LE_FAULT, "", 0, ctxPtr);
            textPtr->mode = false;
            return;
        }
    }

    count = strlen(bufPtr);

    textPtr->offset += count;

    if (devPtr->echo)
    {
        le_dev_Write(&device, (uint8_t *)bufPtr, count);
    }

    if (ProcessText(textPtr, count, &device))
    {
        if (textPtr->callback)
        {
            textPtr->callback(textPtr->cmdRef, textPtr->result, textPtr->buf,
                textPtr->offset, ctxPtr);
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
                if (cmdPtr->bridgeCmd)
                {
                    LE_DEBUG("deleting '%s' (created by bridge device)", cmdPtr->cmdName);
                    bridge_ReleaseModemCmd(cmdPtr->modemCmdDescRefPtr);
                }
                else
                {
                    LE_DEBUG("deleting '%s' (created by app)", cmdPtr->cmdName);
                    le_mem_Release(cmdPtr);
                }
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
                LE_DEBUG("deleting device fd %d", devPtr->device.fd);
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
    if (!devPtr)
    {
        LE_ERROR("Invalid device");
        return LE_BAD_PARAMETER;
    }

    devicePtr = &devPtr->device;

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

    // NOTE: The 'sessionRef' is NULL if the command is created by bridge device because
    // we are not in IPC command environment. In this case, "sessionRef" is set when the
    // bridge command is created in CreateModemCommand().
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
 * @note If the parameter is parsed with quotes, the quotes are removed when retrieving the
 * parameter value using this API. If a parmeter is not parsed with quotes, that parameter is
 * converted to uppercase equivalent.
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
 * @deprecated le_atServer_SendFinalResponse() should not be used anymore and will be removed soon.
 * It has been replaced by le_atServer_SendFinalResultCode()
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
    while ((linkPtr = le_dls_Pop(&cmdPtr->paramList)) != NULL)
    {
        ParamString_t *paraPtr = CONTAINER_OF(linkPtr, ParamString_t, link);
        le_mem_Release(paraPtr);
    }

    cmdPtr->deviceRef = NULL;
    cmdPtr->processing = false;

    if (final != LE_ATSERVER_ERROR)
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
 * This function can be used to send the final result code
 *
 * @return
 *      - LE_OK            The function succeeded
 *      - LE_FAULT         The function failed to send the final response
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_SendFinalResultCode
(
    le_atServer_CmdRef_t commandRef,
        ///< [IN] AT command reference

    le_atServer_FinalRsp_t final,
        ///< [IN] Final result code to be sent

    const char* patternPtr,
        ///< [IN] Prefix string of the return message

    uint32_t errorCode
        ///< [IN] Numeric error code
)
{
    ATCmdSubscribed_t* cmdPtr = le_ref_Lookup(SubscribedCmdRefMap, commandRef);

    if (NULL == cmdPtr)
    {
        LE_ERROR("Bad command reference");
        return LE_FAULT;
    }

    DeviceContext_t* devPtr = le_ref_Lookup(DevicesRefMap, cmdPtr->deviceRef);

    if (NULL == devPtr)
    {
        LE_ERROR("Bad device reference");
        return LE_FAULT;
    }

    devPtr->finalRsp.final = final;
    devPtr->finalRsp.errorCode = errorCode;

    if (NULL != patternPtr)
    {
        int sizeMax = sizeof(devPtr->finalRsp.pattern);
        if (sizeMax > 0)
        {
            strncpy(devPtr->finalRsp.pattern, patternPtr, sizeMax - 1);
            // Make devPtr->finalRsp.pattern string null terminated if patternPtr
            // string size is bigger than sizeMax - 1.
            devPtr->finalRsp.pattern[sizeMax-1] = '\0';
        }
    }

    // Clean AT command context, not in use now
    le_dls_Link_t* linkPtr;
    while ((linkPtr=le_dls_Pop(&cmdPtr->paramList)) != NULL)
    {
        ParamString_t *paraPtr = CONTAINER_OF(linkPtr, ParamString_t, link);
        le_mem_Release(paraPtr);
    }

    cmdPtr->deviceRef = NULL;
    cmdPtr->processing = false;

    if (final != LE_ATSERVER_ERROR)
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
 * This function enables verbose error codes on the selected device
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_EnableVerboseErrorCodes
(
    void
)
{
    ErrorCodesMode = MODE_VERBOSE;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function enables extended error codes on the selected device
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_EnableExtendedErrorCodes
(
    void
)
{
    ErrorCodesMode = MODE_EXTENDED;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function disables the current error codes mode on the selected device
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_DisableExtendedErrorCodes
(
    void
)
{
    ErrorCodesMode = MODE_DISABLED;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function creates a custom error code.
 * @return
 *      - ErrorCode    Reference to the created error code
 *      - NULL         Function failed to create the error code
 *
 * @note This function fails to create the error code if the combinaison (errorCode, pattern)
 * already exists or if the errorCode number is lower than 512.
 *
 */
//--------------------------------------------------------------------------------------------------
le_atServer_ErrorCodeRef_t le_atServer_CreateErrorCode
(
    uint32_t errorCode,
         ///< [IN] Numerical error code

    const char* patternPtr
        ///< [IN] Prefix of the final response string
)
{
    int bufLength = 0;

    if ((errorCode < STD_ERROR_CODE_SIZE) || (NULL == patternPtr))
    {
        // Invalid input parameters
        return NULL;
    }

    if (NULL != GetCustomErrorCode(errorCode, patternPtr))
    {
        // The error code already exists, return a NULL reference
        return NULL;
    }

    UserErrorCode_t* newErrorCode = le_mem_ForceAlloc(UserErrorPool);
    newErrorCode->errorCode = errorCode;

    bufLength = sizeof(newErrorCode->pattern);
    strncpy(newErrorCode->pattern, patternPtr, bufLength - 1);
    newErrorCode->pattern[bufLength - 1] = '\0';

    newErrorCode->ref = le_ref_CreateRef(UserErrorRefMap, newErrorCode);

    return newErrorCode->ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function deletes a custom error code
 *
 * @return
 *      - LE_OK            Error code deleted sucessfully
        - LE_NOT_FOUND     Error code reference not found
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_DeleteErrorCode
(
    le_atServer_ErrorCodeRef_t errorCodeRef
        ///< [IN] Reference to a custom error code
)
{
    UserErrorCode_t* errorCodePtr = le_ref_Lookup(UserErrorRefMap, errorCodeRef);
    if (NULL == errorCodePtr)
    {
        return LE_NOT_FOUND;
    }

    le_ref_DeleteRef(UserErrorRefMap, errorCodeRef);
    le_mem_Release(errorCodePtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function adds a verbose message to a specified error code
 *
 * @return
 *      - LE_OK            The function succeeded
 *      - LE_FAULT         The function failed to set the verbose message
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_SetVerboseErrorCode
(
    le_atServer_ErrorCodeRef_t  errorCodeRef,
        ///< [IN] Reference to a custom error code

    const char*  messagePtr
        ///< [IN] Verbose string
)
{
    int bufLength = 0;

    if (NULL == messagePtr)
    {
        // Invalid input parameter
        return LE_FAULT;
    }

    UserErrorCode_t* errorCodePtr = le_ref_Lookup(UserErrorRefMap, errorCodeRef);
    if (NULL == errorCodePtr)
    {
        // Error code not found
        return LE_FAULT;
    }

    bufLength = sizeof(errorCodePtr->verboseMsg);
    strncpy(errorCodePtr->verboseMsg, messagePtr, bufLength - 1);
    errorCodePtr->verboseMsg[bufLength - 1] = '\0';

    return LE_OK;
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
    memset(devPtr->text.buf, 0, LE_ATDEFS_TEXT_MAX_BYTES);
    devPtr->text.callback = callback;
    devPtr->text.ctxPtr = ctxPtr;
    devPtr->text.cmdRef = cmdRef;

    le_dev_Write(&devPtr->device, (uint8_t *)TEXT_PROMPT, TEXT_PROMPT_LEN);

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

    // User-defined errors pool allocation
    UserErrorPool = le_mem_CreatePool("UserErrorPool",sizeof(UserErrorCode_t));
    le_mem_ExpandPool(UserErrorPool,USER_ERROR_POOL_SIZE);
    UserErrorRefMap = le_ref_CreateMap("UserErrorCmdRefMap", USER_ERROR_POOL_SIZE);

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler(
        le_atServer_GetServiceRef(), CloseSessionEventHandler, NULL);

    bridge_Init();

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);
}
