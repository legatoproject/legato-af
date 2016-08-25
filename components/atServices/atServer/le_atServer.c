/** @file le_atServer.c
 *
 * Implementation of AT commands server API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "interfaces.h"
#include "le_dev.h"
#include <ctype.h>

//--------------------------------------------------------------------------------------------------
/**
 * Max length of thread name
 */
//--------------------------------------------------------------------------------------------------
#define THREAD_NAME_MAX_LENGTH  30

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
 * Is character a letter ?
 */
//--------------------------------------------------------------------------------------------------
#define IS_CHAR(X)              ((X>='A')&&(X<='Z'))||((X>='a')&&(X<='z')) /*[A-Z]||[a-z]*/

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
 * The memory pool for EventIdList objects
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t EventIdPool;

//--------------------------------------------------------------------------------------------------
/**
 * List for all eventId objects
 */
//--------------------------------------------------------------------------------------------------
static le_dls_List_t    EventIdList;

//--------------------------------------------------------------------------------------------------
/**
 * Main thread reference
 */
//--------------------------------------------------------------------------------------------------
static le_thread_Ref_t MainThreadRef;

//--------------------------------------------------------------------------------------------------
/**
 * EventIdList structure.
 * Objects use to manage a pool of eventId.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_event_Id_t   eventId; ///< the eventId
    bool            isUsed;  ///< is it used?
    le_dls_Link_t   link;    ///< link for eventIdList
}
EventIdList_t;

//--------------------------------------------------------------------------------------------------
/**
 * structure used to describe a parameter.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char            param[LE_ATSERVER_PARAMETER_MAX_BYTES];  ///< string value
    le_dls_Link_t   link;                                 ///< link for list
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
    char            resp[LE_ATSERVER_RESPONSE_MAX_BYTES];  ///< string value
    le_dls_Link_t   link;                                 ///< link for list
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
    AT_PARSE_CMDNAME,
    AT_PARSE_EQUAL,
    AT_PARSE_QUESTIONMARK,
    AT_PARSE_COMMA,
    AT_PARSE_SEMICOLON,
    AT_PARSE_BASIC,
    AT_PARSE_BASIC_PARAM,
    AT_PARSE_BASIC_END,
    AT_PARSE_LAST,
    AT_PARSE_MAX
}
CmdParserState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Subscribed AT Command structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_atServer_CmdRef_t    cmdRef;
    char                    cmdName[LE_ATSERVER_COMMAND_MAX_BYTES];     ///< Command to send
    le_event_Id_t           eventId;
    le_atServer_AvailableDevice_t availableDevice;
    le_atServer_Type_t      type;
    le_dls_List_t           paramList;
    bool                    processing;
    le_atServer_DeviceRef_t deviceRef;
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
    char                    foundCmd[LE_ATSERVER_COMMAND_MAX_LEN];
    RxParserState_t         rxState;
    CmdParserState_t        cmdParser;
    CmdParserState_t        lastCmdParserState;
    char*                   currentAtCmdPtr;
    char*                   currentCharPtr;
    char*                   lastCharPtr;
    ATCmdSubscribed_t*      currentCmdPtr;
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
    le_atServer_FinalRsp_t  final;
    bool                    customStringAvailable;
    char                    resp[LE_ATSERVER_RESPONSE_MAX_BYTES];  ///< string value
}
FinalRsp_t;

//--------------------------------------------------------------------------------------------------
/**
 * Device context structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    Device_t                device;             ///< data of the connected device
    le_atServer_DeviceRef_t ref;                ///< reference of the device context
    le_thread_Ref_t         threadRef;          ///< Thread reference
    le_sem_Ref_t            semaphore;          ///< semaphore used for synchronization
    char                    currentCmd[LE_ATSERVER_COMMAND_MAX_LEN];
    uint32_t                indexRead;
    uint32_t                parseIndex;
    CmdParser_t             cmdParser;
    FinalRsp_t              finalRsp;
    bool                    processing;
    le_dls_List_t           unsolicitedList;
}
DeviceContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * AT commands parser automaton definition and prototypes.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef le_result_t (*CmdParserFunc_t)(CmdParser_t* cmdParserPtr);
static le_result_t AtPArserTypeTest(CmdParser_t* cmdParserPtr);
static le_result_t AtParserError(CmdParser_t* cmdParserPtr);
static le_result_t AtParserContinue(CmdParser_t* cmdParserPtr);
static le_result_t AtParserParam(CmdParser_t* cmdParserPtr);
static le_result_t AtParserEqual(CmdParser_t* cmdParserPtr);
static le_result_t AtPArserTypeRead(CmdParser_t* cmdParserPtr);
static le_result_t AtParserParam(CmdParser_t* cmdParserPtr);
static le_result_t AtParserSemicolon(CmdParser_t* cmdParserPtr);
static le_result_t AtParserLastChar(CmdParser_t* cmdParserPtr);
static le_result_t AtParserBasic(CmdParser_t* cmdParserPtr);
static le_result_t AtParserBasicEnd(CmdParser_t* cmdParserPtr);
static le_result_t AtParserNone(CmdParser_t* cmdParserPtr);
static le_result_t AtParserBasicParam(CmdParser_t* cmdParserPtr);
static void ParseATCmd(void* param1Ptr,void* param2Ptr);

CmdParserFunc_t CmdParserTab[AT_PARSE_MAX][AT_PARSE_MAX] =
{
/*AT_PARSE_CMDNAME*/        {   AtParserContinue,                   /*AT_PARSE_CMDNAME*/
                                AtParserEqual,                      /*AT_PARSE_EQUAL*/
                                AtPArserTypeRead,                   /*AT_PARSE_QUESTIONMARK*/
                                AtParserError,                      /*AT_PARSE_COMMA*/
                                AtParserSemicolon,                  /*AT_PARSE_SEMICOLON*/
                                AtParserBasic,                      /*AT_PARSE_BASIC*/
                                AtParserError,                      /*AT_PARSE_BASIC_PARAM*/
                                AtParserError,                      /*AT_PARSE_BASIC_END*/
                                AtParserLastChar                    /*AT_PARSE_LAST*/
                            },
/*AT_PARSE_EQUAL*/          {   AtParserError,                      /*AT_PARSE_CMDNAME*/
                                AtParserError,                      /*AT_PARSE_EQUAL*/
                                AtPArserTypeTest,                   /*AT_PARSE_QUESTIONMARK*/
                                AtParserParam,                      /*AT_PARSE_COMMA*/
                                AtParserError,                      /*AT_PARSE_SEMICOLON*/
                                AtParserError,                      /*AT_PARSE_BASIC*/
                                AtParserError,                      /*AT_PARSE_BASIC_PARAM*/
                                AtParserError,                      /*AT_PARSE_BASIC_END*/
                                AtParserNone                        /*AT_PARSE_LAST*/
                            },
/*AT_PARSE_QUESTIONMARK*/   {   AtParserBasicEnd,                   /*AT_PARSE_CMDNAME*/
                                AtParserError,                      /*AT_PARSE_EQUAL*/
                                AtParserError,                      /*AT_PARSE_QUESTIONMARK*/
                                AtParserError,                      /*AT_PARSE_COMMA*/
                                AtParserSemicolon,                  /*AT_PARSE_SEMICOLON*/
                                AtParserError,                      /*AT_PARSE_BASIC*/
                                AtParserError,                      /*AT_PARSE_BASIC_PARAM*/
                                AtParserError,                      /*AT_PARSE_BASIC_END*/
                                AtParserNone                        /*AT_PARSE_LAST*/
                            },
/*AT_PARSE_COMMA*/          {   AtParserError,                      /*AT_PARSE_CMDNAME*/
                                AtParserError,                      /*AT_PARSE_EQUAL*/
                                AtParserError,                      /*AT_PARSE_QUESTIONMARK*/
                                AtParserParam,                      /*AT_PARSE_COMMA*/
                                AtParserSemicolon,                  /*AT_PARSE_SEMICOLON*/
                                AtParserError,                      /*AT_PARSE_BASIC*/
                                AtParserError,                      /*AT_PARSE_BASIC_PARAM*/
                                AtParserError,                      /*AT_PARSE_BASIC_END*/
                                AtParserNone                        /*AT_PARSE_LAST*/
                            },
/*AT_PARSE_SEMICOLON*/      {   AtParserContinue,                   /*AT_PARSE_CMDNAME*/
                                AtParserSemicolon,                  /*AT_PARSE_EQUAL*/
                                AtParserError,                      /*AT_PARSE_QUESTIONMARK*/
                                AtParserError,                      /*AT_PARSE_COMMA*/
                                AtParserError,                      /*AT_PARSE_SEMICOLON*/
                                AtParserError,                      /*AT_PARSE_BASIC*/
                                AtParserError,                      /*AT_PARSE_BASIC_PARAM*/
                                AtParserError,                      /*AT_PARSE_BASIC_END*/
                                AtParserNone                        /*AT_PARSE_LAST*/
                            },
/* AT_PARSE_BASIC */        {   AtParserContinue,                   /*AT_PARSE_CMDNAME*/
                                AtParserError,                      /*AT_PARSE_EQUAL*/
                                AtPArserTypeRead,                   /*AT_PARSE_QUESTIONMARK*/
                                AtParserError,                      /*AT_PARSE_COMMA*/
                                AtParserSemicolon,                  /*AT_PARSE_SEMICOLON*/
                                AtParserError,                      /*AT_PARSE_BASIC*/
                                AtParserBasicParam,                 /*AT_PARSE_BASIC_PARAM*/
                                AtParserBasicEnd,                   /*AT_PARSE_BASIC_END*/
                                AtParserNone                       /*AT_PARSE_LAST*/
                            },
/* AT_PARSE_BASIC_PARAM */  {   AtParserError,                      /*AT_PARSE_CMDNAME*/
                                AtParserError,                      /*AT_PARSE_EQUAL*/
                                AtPArserTypeRead,                   /*AT_PARSE_QUESTIONMARK*/
                                AtParserError,                      /*AT_PARSE_COMMA*/
                                AtParserSemicolon,                  /*AT_PARSE_SEMICOLON*/
                                AtParserError,                      /*AT_PARSE_BASIC*/
                                AtParserBasicParam,                 /*AT_PARSE_BASIC_PARAM*/
                                AtParserBasicEnd,                   /*AT_PARSE_BASIC_END*/
                                AtParserError                       /*AT_PARSE_LAST*/
                            },
/* AT_PARSE_BASIC_END */    {   AtParserError,                      /*AT_PARSE_CMDNAME*/
                                AtParserError,                      /*AT_PARSE_EQUAL*/
                                AtParserError,                      /*AT_PARSE_QUESTIONMARK*/
                                AtParserError,                      /*AT_PARSE_COMMA*/
                                AtParserError,                      /*AT_PARSE_SEMICOLON*/
                                AtParserError,                      /*AT_PARSE_BASIC*/
                                AtParserError,                      /*AT_PARSE_BASIC_PARAM*/
                                AtParserError,                      /*AT_PARSE_BASIC_END*/
                                AtParserNone                        /*AT_PARSE_LAST*/
                            },
/* AT_PARSE_LAST  */        {   AtParserContinue,                   /*AT_PARSE_CMDNAME*/
                                AtParserError,                      /*AT_PARSE_EQUAL*/
                                AtParserError,                      /*AT_PARSE_QUESTIONMARK*/
                                AtParserError,                      /*AT_PARSE_COMMA*/
                                AtParserError,                      /*AT_PARSE_SEMICOLON*/
                                AtParserError,                      /*AT_PARSE_BASIC*/
                                AtParserError,                      /*AT_PARSE_BASIC_PARAM*/
                                AtParserError,                      /*AT_PARSE_BASIC_END*/
                                AtParserNone                        /*AT_PARSE_LAST*/
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
    void *ptr
)
{
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is the destructor for DeviceContext_t struct
 *
 */
//--------------------------------------------------------------------------------------------------
static void DevicesPoolDestructor
(
    void *ptr
)
{

}

//--------------------------------------------------------------------------------------------------
/**
 * Device thread destructor.
 *
 */
//--------------------------------------------------------------------------------------------------
static void DestroyDeviceThread
(
    void *contextPtr
)
{

}

//--------------------------------------------------------------------------------------------------
/**
 * Send a response on the opened device.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendRspString
(
    DeviceContext_t *devPtr,
    RspString_t* rspStringPtr
)
{
    char string[LE_ATSERVER_RESPONSE_MAX_BYTES+4];
    memset(string,0,LE_ATSERVER_RESPONSE_MAX_BYTES+4);
    snprintf(string, LE_ATSERVER_RESPONSE_MAX_BYTES+4, "\r\n%s\r\n", rspStringPtr->resp);

    le_dev_Write( &devPtr->device, (uint8_t*)string, strlen(string) );

    le_mem_Release(rspStringPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a final response on the opened device.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendFinalRsp
(
    void* param1Ptr,
    void* param2Ptr
)
{
    DeviceContext_t *devPtr = param1Ptr;
    char string[LE_ATSERVER_RESPONSE_MAX_BYTES+4];
    memset(string, 0, LE_ATSERVER_RESPONSE_MAX_BYTES+4);

    if (devPtr->finalRsp.customStringAvailable)
    {
        snprintf(string, LE_ATSERVER_RESPONSE_MAX_BYTES+4, "\r\n%s\r\n", devPtr->finalRsp.resp);
    }
    else
    {
        switch (devPtr->finalRsp.final)
        {
            case LE_ATSERVER_OK:
                snprintf(string,LE_ATSERVER_RESPONSE_MAX_BYTES,"\r\nOK\r\n");
            break;
            default:
                snprintf(string,LE_ATSERVER_RESPONSE_MAX_BYTES,"\r\nERROR\r\n");
            break;
        }
    }

    le_dev_Write( &devPtr->device, (uint8_t*)string, strlen(string) );
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

        SendRspString(devPtr, rspStringPtr);
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
    void* param1Ptr,
    void* param2Ptr
)
{
    DeviceContext_t *devPtr = param1Ptr;
    RspString_t* rspStringPtr = param2Ptr;

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

    // Check if command is currently in processing
    if ((!devPtr->processing) ||
        (devPtr->cmdParser.currentCmdPtr && !((devPtr->cmdParser.currentCmdPtr)->processing)))
    {
        LE_ERROR("Command not processing anymore");
        le_mem_Release(rspStringPtr);
        return;
    }

    SendRspString(devPtr, rspStringPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Send an intermediate response on the opened device.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendUnsolRsp
(
    void* param1Ptr,
    void* param2Ptr
)
{
    DeviceContext_t *devPtr = param1Ptr;
    RspString_t* rspStringPtr = param2Ptr;

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

    if (!devPtr->processing)
    {
        SendRspString(devPtr, rspStringPtr);
    }
    else
    {
        le_dls_Queue(&devPtr->unsolicitedList, &(rspStringPtr->link));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function finds or creates an eventId into the EventIdList
 *
 */
//--------------------------------------------------------------------------------------------------
static le_event_Id_t GetEventId
(
    void
)
{
    EventIdList_t*      currentPtr=NULL;
    le_dls_Link_t*      linkPtr = le_dls_Peek(&EventIdList);
    char                eventIdName[24];
    int32_t             eventIdIdx = 1;

    while (linkPtr!=NULL)
    {
        currentPtr = CONTAINER_OF(linkPtr,
                                  EventIdList_t,
                                  link);

        if (!currentPtr->isUsed)
        {
            LE_DEBUG("Found one unused eventId (%p)", currentPtr->eventId);
            currentPtr->isUsed = true;
            return currentPtr->eventId;
        }
        linkPtr = le_dls_PeekNext(&EventIdList,linkPtr);

        eventIdIdx++;
    }

    snprintf(eventIdName, sizeof(eventIdName), "atCmd-%d", eventIdIdx);

    currentPtr = le_mem_ForceAlloc(EventIdPool);
    currentPtr->eventId = le_event_CreateId(eventIdName, sizeof(ATCmdSubscribed_t*));
    currentPtr->isUsed = true;
    currentPtr->link = LE_DLS_LINK_INIT;

    le_dls_Queue(&EventIdList, &(currentPtr->link));

    LE_DEBUG("Create a new eventId (%p)", currentPtr->eventId);

    return currentPtr->eventId;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function releases an eventId from the EventIdList
 *
 */
//--------------------------------------------------------------------------------------------------
#if 0
static void ReleaseEventId
(
    le_event_Id_t eventId
)
{
    le_dls_Link_t* linkPtr = le_dls_Peek(&EventIdList);

    while (linkPtr!=NULL)
    {
        struct eventIdList* currentPtr = CONTAINER_OF(linkPtr,
                                                      struct eventIdList,
                                                      link);

        if (currentPtr->eventId == eventId)
        {
            LE_DEBUG("Found eventId to release (%p)", currentPtr->eventId);
            currentPtr->isUsed = false;
            return;
        }
        linkPtr = le_dls_PeekNext(&EventIdList,linkPtr);
    }

    LE_DEBUG("could not found eventId to release");
    return;
}
#endif

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
    if (cmdParserPtr->currentCmdPtr == NULL)
    {
        cmdParserPtr->currentCmdPtr = le_hashmap_Get(CmdHashMap, cmdParserPtr->currentAtCmdPtr);

        if (( cmdParserPtr->currentCmdPtr == NULL ) ||
            ( cmdParserPtr->currentCmdPtr && cmdParserPtr->currentCmdPtr->processing ))
        {
            LE_DEBUG("AT command not found");
            return LE_FAULT;
        }

        cmdParserPtr->currentCmdPtr->processing = true;

        DeviceContext_t* devPtr = CONTAINER_OF(cmdParserPtr, DeviceContext_t, cmdParser);
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
static le_result_t AtPArserTypeTest
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
static le_result_t AtPArserTypeRead
(
    CmdParser_t* cmdParserPtr
)
{
    *cmdParserPtr->currentCharPtr='\0';

    if (GetAtCmdContext(cmdParserPtr) == LE_OK)
    {
        cmdParserPtr->currentCmdPtr->type = LE_ATSERVER_TYPE_READ;
        return LE_OK;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (unspecified)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AtParserError
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
static le_result_t AtParserNone
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
static le_result_t AtParserContinue
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
 * AT parser transition (Get a parameter from basic command)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AtParserBasicParam
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
 * AT parser transition (end of treatment for a basic command)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AtParserBasicEnd
(
    CmdParser_t* cmdParserPtr
)
{
    cmdParserPtr->currentAtCmdPtr = cmdParserPtr->currentCharPtr-2;
    memcpy(cmdParserPtr->currentAtCmdPtr, "AT", 2);

    // Put the index at the correct place for next parsing
    cmdParserPtr->currentCharPtr = cmdParserPtr->currentAtCmdPtr;

    cmdParserPtr->cmdParser = AT_PARSE_LAST;

    cmdParserPtr->currentCharPtr--;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (treatment of a basic command)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AtParserBasic
(
    CmdParser_t* cmdParserPtr
)
{
    while ( ( cmdParserPtr->currentCharPtr <= cmdParserPtr->lastCharPtr ) &&
           ( !IS_NUMBER(*cmdParserPtr->currentCharPtr) ) )
    {
        AtParserContinue(cmdParserPtr);
        cmdParserPtr->currentCharPtr++;
    }

    uint32_t len = cmdParserPtr->currentCharPtr-cmdParserPtr->currentAtCmdPtr+1;

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
            DeviceContext_t* devPtr = CONTAINER_OF(cmdParserPtr, DeviceContext_t, cmdParser);
            cmdParserPtr->currentCmdPtr->deviceRef = devPtr->ref;

            cmdParserPtr->currentCmdPtr->type = LE_ATSERVER_TYPE_ACT;
            cmdParserPtr->currentCmdPtr->processing = true;

            cmdParserPtr->currentCharPtr--;

            return LE_OK;
        }
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (treat '=')
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AtParserEqual
(
    CmdParser_t* cmdParserPtr
)
{
    *cmdParserPtr->currentCharPtr='\0';

    if (GetAtCmdContext(cmdParserPtr) == LE_OK)
    {
        cmdParserPtr->currentCmdPtr->type = LE_ATSERVER_TYPE_PARA;
        return LE_OK;
    }

    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (treat a parameter for extended commands)
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AtParserParam
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
        }
        else
        {
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

        if (( *cmdParserPtr->currentCharPtr == AT_TOKEN_COMMA ) ||
            ( *cmdParserPtr->currentCharPtr == AT_TOKEN_SEMICOLON ))
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
static le_result_t AtParserLastChar
(
    CmdParser_t* cmdParserPtr
)
{
    LE_DEBUG("AtParserLastChar");

    if ( cmdParserPtr->currentCmdPtr == NULL )
    {
        // Put character in upper case
        *cmdParserPtr->currentCharPtr = toupper(*cmdParserPtr->currentCharPtr);

        if (GetAtCmdContext(cmdParserPtr) == LE_OK)
        {
            cmdParserPtr->currentCmdPtr->type = LE_ATSERVER_TYPE_ACT;
        }
        else
        {
            return LE_FAULT;
        }
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * AT parser transition (treat ';')
 *
 */
//--------------------------------------------------------------------------------------------------
static le_result_t AtParserSemicolon
(
    CmdParser_t* cmdParserPtr
)
{
    *cmdParserPtr->currentCharPtr='\0';

    /* if AT command not resolved yet, try to get it */
    if ( AtParserLastChar(cmdParserPtr) != LE_OK )
    {
        return LE_FAULT;
    }

    /* Concatenate command: prepare the buffer for the next parsing */
    /* Be sure to not write outside the buffer */
    cmdParserPtr->currentCharPtr--;
    if (cmdParserPtr->currentCharPtr >= cmdParserPtr->foundCmd)
    {
        memcpy(cmdParserPtr->currentCharPtr, "AT", 2);

        /* Put the index at the correct place for next parsing */
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
static void ParseATCmd
(
    void* param1Ptr,
    void* param2Ptr
)
{
    DeviceContext_t *devPtr = param1Ptr;
    CmdParser_t* cmdParserPtr = &devPtr->cmdParser;

    cmdParserPtr->cmdParser = AT_PARSE_CMDNAME;

    devPtr->cmdParser.currentCmdPtr = NULL;

    /* if parsing over, send the final response */
    if (cmdParserPtr->currentCharPtr > cmdParserPtr->lastCharPtr)
    {
        le_event_QueueFunctionToThread( devPtr->threadRef,
                                        SendFinalRsp,
                                        devPtr,
                                        NULL );
        return;
    }

    while (( cmdParserPtr->cmdParser != AT_PARSE_SEMICOLON ) &&
           ( cmdParserPtr->cmdParser != AT_PARSE_LAST ))
    {
        switch (*cmdParserPtr->currentCharPtr)
        {
            case AT_TOKEN_EQUAL:
                cmdParserPtr->cmdParser = AT_PARSE_EQUAL;
            break;
            case AT_TOKEN_QUESTIONMARK:
                cmdParserPtr->cmdParser = AT_PARSE_QUESTIONMARK;
            break;
            case AT_TOKEN_COMMA:
                cmdParserPtr->cmdParser = AT_PARSE_COMMA;
            break;
            case AT_TOKEN_SEMICOLON:
                cmdParserPtr->cmdParser = AT_PARSE_SEMICOLON;
            break;
            default:
                if (cmdParserPtr->cmdParser >= AT_PARSE_BASIC)
                {
                    if ( IS_NUMBER(*cmdParserPtr->currentCharPtr) ||
                       ( IS_QUOTE(*cmdParserPtr->currentCharPtr) ) )
                    {
                        cmdParserPtr->cmdParser = AT_PARSE_BASIC_PARAM;
                    }
                    else
                    {
                        cmdParserPtr->cmdParser = AT_PARSE_BASIC_END;
                    }
                    break;
                }

                if ((cmdParserPtr->currentCharPtr - cmdParserPtr->currentAtCmdPtr == 2) &&
                    (IS_CHAR(*cmdParserPtr->currentCharPtr) ||
                     IS_AND(*cmdParserPtr->currentCharPtr)  ||
                     IS_SLASH(*cmdParserPtr->currentCharPtr)))
                {
                    /* 3rd char of the command is into [A-Z] => basic command */
                    cmdParserPtr->cmdParser = AT_PARSE_BASIC;
                }

                else if ((cmdParserPtr->currentCmdPtr) &&
                    (cmdParserPtr->currentCmdPtr->type == LE_ATSERVER_TYPE_PARA))
                {
                    cmdParserPtr->cmdParser = AT_PARSE_COMMA;
                }
                else if (cmdParserPtr->currentCharPtr == cmdParserPtr->lastCharPtr)
                {
                    cmdParserPtr->cmdParser = AT_PARSE_LAST;
                }
                else
                {
                    cmdParserPtr->cmdParser = AT_PARSE_CMDNAME;
                }
            break;
        }

        if ((cmdParserPtr->lastCmdParserState >= AT_PARSE_MAX) ||
            (cmdParserPtr->cmdParser >= AT_PARSE_MAX))
        {
            LE_ERROR("Wrong parser state");
            return;
        }

        if (CmdParserTab[cmdParserPtr->lastCmdParserState][cmdParserPtr->cmdParser](cmdParserPtr)
                                                                                           != LE_OK)
        {
            LE_ERROR("Error in parsing AT command, lastState %d, current state %d",
                                                        cmdParserPtr->lastCmdParserState,
                                                        cmdParserPtr->cmdParser);
            devPtr->finalRsp.final = LE_ATSERVER_ERROR;
            devPtr->finalRsp.customStringAvailable = false;
            le_event_QueueFunctionToThread( devPtr->threadRef,
                                            SendFinalRsp,
                                            devPtr,
                                            NULL );

            return;
        }

        cmdParserPtr->lastCmdParserState = cmdParserPtr->cmdParser;
        cmdParserPtr->currentCharPtr++;

        if (cmdParserPtr->currentCharPtr > cmdParserPtr->lastCharPtr)
        {
            cmdParserPtr->cmdParser = AT_PARSE_LAST;
        }
    }

    if (cmdParserPtr->currentCmdPtr)
    {
        le_event_Report(cmdParserPtr->currentCmdPtr->eventId,
                        &cmdParserPtr->currentCmdPtr, sizeof(ATCmdSubscribed_t*));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Parser incoming characters
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseBuffer
(
    DeviceContext_t *devPtr
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
                if (( input == 'T' ) || ( input == 't' ))
                {
                    devPtr->currentCmd[1] = input;
                    devPtr->cmdParser.rxState = PARSER_SEARCH_CR;
                    devPtr->parseIndex = 2;
                }
                else
                {
                    devPtr->cmdParser.rxState = PARSER_SEARCH_A;
                    devPtr->parseIndex = 0;
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
                        strncpy(devPtr->cmdParser.foundCmd,
                                devPtr->currentCmd,
                                LE_ATSERVER_COMMAND_MAX_LEN);

                        devPtr->cmdParser.currentCharPtr = devPtr->cmdParser.foundCmd;
                        devPtr->cmdParser.lastCharPtr = devPtr->cmdParser.foundCmd +
                                                         strlen(devPtr->cmdParser.foundCmd) - 1;
                        devPtr->cmdParser.currentAtCmdPtr = devPtr->cmdParser.foundCmd;

                        le_event_QueueFunctionToThread( MainThreadRef,
                                                        ParseATCmd,
                                                        devPtr,
                                                        NULL );
                    }
                    else
                    {
                        LE_ERROR("Command in progress");
                        le_dev_Write(&devPtr->device, (uint8_t*)"\r\nERROR\r\n", 9 );
                    }

                    devPtr->parseIndex=0;
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
    if (events & ~POLLIN)
    {
        LE_CRIT("Unexpected event(s) on fd %d (0x%hX).", fd, events);
    }

    DeviceContext_t *devPtr = le_fdMonitor_GetContextPtr();

    /* Read RX data on uart */
    int32_t size = le_dev_Read( &devPtr->device,
                                (uint8_t *)(devPtr->currentCmd + devPtr->indexRead),
                                (LE_ATSERVER_COMMAND_MAX_LEN - devPtr->indexRead) );

    devPtr->indexRead += size;

    ParseBuffer(devPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * Thread for device Rx parsing.
 *
 */
//--------------------------------------------------------------------------------------------------
static void *DeviceThread
(
    void* context
)
{
    DeviceContext_t* devPtr = context;

    if (devPtr->device.fdMonitor)
    {
        LE_ERROR("Interface %s already started",devPtr->device.path);
        return NULL;
    }

    le_dev_Open(&devPtr->device, RxNewData, devPtr);

    le_sem_Post(devPtr->semaphore);

    le_event_RunLoop();

    return NULL;
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
    strncpy(rspStringPtr->resp, unsolRsp, LE_ATSERVER_RESPONSE_MAX_BYTES);

    le_event_QueueFunctionToThread( devPtr->threadRef,
                                    SendUnsolRsp,
                                    devPtr,
                                    rspStringPtr );

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The first-layer AT command events Handler.
 *
 */
//--------------------------------------------------------------------------------------------------
static void FirstLayerAtCmdHandler
(
    void* reportPtr,
    void* secondLayerHandlerFunc
)
{
    ATCmdSubscribed_t** cmdPtr = reportPtr;
    le_atServer_CommandHandlerFunc_t clientHandlerFunc = secondLayerHandlerFunc;

    clientHandlerFunc((*cmdPtr)->cmdRef,
                      (*cmdPtr)->type,
                      le_dls_NumLinks(&((*cmdPtr)->paramList)),
                      le_event_GetContextPtr());
}

//--------------------------------------------------------------------------------------------------
/**
 * This function starts an AT server session on the requested device.
 *
 * @return
 *      - Reference to the requested device.
 *      - NULL if the device is not available.
 */
//--------------------------------------------------------------------------------------------------
le_atServer_DeviceRef_t le_atServer_Start
(
    const char* devicePathPtr
        ///< [IN] device path which has to be used for the AT parser
)
{
    char name[THREAD_NAME_MAX_LENGTH];
    static uint32_t threatCounter = 1;

    // Search if the device is already opened
    le_ref_IterRef_t iterRef = le_ref_GetIterator(DevicesRefMap);

     while (le_ref_NextNode(iterRef) == LE_OK)
     {
        DeviceContext_t* devPtr = (DeviceContext_t*) le_ref_GetValue(iterRef);

        if (le_path_IsEquivalent(devicePathPtr, devPtr->device.path, "/") == true)
        {
            le_mem_AddRef(devPtr);
            return devPtr->ref;
        }
    }

    DeviceContext_t* devPtr = le_mem_ForceAlloc(DevicesPool);

    memset(devPtr,0,sizeof(DeviceContext_t));

    le_utf8_Copy(devPtr->device.path,devicePathPtr,LE_ATSERVER_PATH_MAX_LEN,0);

    LE_DEBUG("Create a new interface for '%s'", devicePathPtr);

    snprintf(name,THREAD_NAME_MAX_LENGTH,"atCommandServer-%d",threatCounter);
    devPtr->threadRef = le_thread_Create(name,DeviceThread,devPtr);

    memset(name,0,THREAD_NAME_MAX_LENGTH);
    snprintf(name,THREAD_NAME_MAX_LENGTH,"AtServerSem-%d",threatCounter);
    devPtr->semaphore = le_sem_Create(name,0);

    devPtr->cmdParser.rxState = PARSER_SEARCH_A;
    devPtr->parseIndex = 0;
    devPtr->unsolicitedList = LE_DLS_LIST_INIT;

    threatCounter++;

    le_thread_AddChildDestructor(devPtr->threadRef,
                                 DestroyDeviceThread,
                                 devPtr);

    le_thread_SetJoinable(devPtr->threadRef);

    le_thread_Start(devPtr->threadRef);
    le_sem_Wait(devPtr->semaphore);


    if (devPtr != NULL)
    {
        devPtr->ref = le_ref_CreateRef(DevicesRefMap, devPtr);
        return devPtr->ref;
    }
    else
    {
        return NULL;
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function stops the AT server session on the requested device.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to stop the AT server session.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_Stop
(
    le_atServer_DeviceRef_t device
        ///< [IN] device to be unbinded
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function created an AT command and register it into the AT parser.
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

    if (cmdPtr != NULL)
    {
        le_mem_AddRef(cmdPtr);
    }
    else
    {
        cmdPtr = le_mem_ForceAlloc(AtCommandsPool);

        LE_DEBUG("Create: %s", namePtr);

        memset(cmdPtr,0,sizeof(ATCmdSubscribed_t));

        le_utf8_Copy(cmdPtr->cmdName, namePtr, LE_ATSERVER_COMMAND_MAX_BYTES,0);

        cmdPtr->cmdRef = le_ref_CreateRef(SubscribedCmdRefMap, cmdPtr);

        le_hashmap_Put(CmdHashMap, cmdPtr->cmdName, cmdPtr);

        cmdPtr->availableDevice = LE_ATSERVER_ALL_DEVICES;

        cmdPtr->paramList = LE_DLS_LIST_INIT;

        cmdPtr->eventId = GetEventId();
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
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_Delete
(
    le_atServer_CmdRef_t commandRef
        ///< [IN] AT command reference
)
{
    return LE_FAULT;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function sets the device(s) where the specified AT command is available.
 *
 * @return
 *      - LE_OK            The function succeeded.
 *      - LE_FAULT         The function failed to set the device.
 *
 * @note If the AT command is available for all devices (i.e. availableDevice argument is set to
 * LE_ATSERVER_ALL_DEVICES), the "device" argument is unused.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atServer_SetDevice
(
    le_atServer_CmdRef_t commandRef,
        ///< [IN] AT command reference

    le_atServer_AvailableDevice_t availableDevice,
        ///< [IN] device available for the AT command

    le_atServer_DeviceRef_t device
        ///< [IN] device reference where the AT command is available
)
{
    return LE_FAULT;
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
    le_event_HandlerRef_t handlerRef = NULL;
    ATCmdSubscribed_t* cmdPtr = le_ref_Lookup(SubscribedCmdRefMap, commandRef);

    if (cmdPtr == NULL)
    {
        LE_ERROR("Bad command reference");
        return NULL;
    }

    char name[30];
    memset(name,0,30);
    snprintf(name, 30, "%s-handler", cmdPtr->cmdName);

    LE_DEBUG("Add handler to: %s", cmdPtr->cmdName);

    handlerRef = le_event_AddLayeredHandler(name,
                                            cmdPtr->eventId,
                                            FirstLayerAtCmdHandler,
                                            (le_event_HandlerFunc_t)handlerPtr);

    le_event_SetContextPtr(handlerRef, contextPtr);

    return (le_atServer_CommandHandlerRef_t)(handlerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_atServer_Command'
 */
//--------------------------------------------------------------------------------------------------
void le_atServer_RemoveCommandHandler
(
    le_atServer_CommandHandlerRef_t addHandlerRef
        ///< [IN]
)
{
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
    strncpy(rspStringPtr->resp, intermediateRspPtr, LE_ATSERVER_RESPONSE_MAX_BYTES);

    le_event_QueueFunctionToThread( devPtr->threadRef,
                                    SendIntermediateRsp,
                                    devPtr,
                                    rspStringPtr );

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
        ///< [IN] Custom finalRsp string has to be sent
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
    strncpy( devPtr->finalRsp.resp, finalRspPtr, LE_ATSERVER_RESPONSE_MAX_BYTES );

    /* clean AT command context, not in use now */
    le_dls_Link_t* linkPtr;
    while ((linkPtr=le_dls_Pop(&cmdPtr->paramList)) != NULL)
    {
        ParamString_t *paraPtr = CONTAINER_OF(linkPtr, ParamString_t, link);
        le_mem_Release(paraPtr);
    }

    cmdPtr->processing = false;
    cmdPtr->deviceRef = NULL;

    if (final == LE_ATSERVER_OK)
    {
        /* Parse next AT commands, if any */
        ParseATCmd(devPtr, NULL);
    }
    else
    {
        le_event_QueueFunctionToThread( devPtr->threadRef,
                                        SendFinalRsp,
                                        devPtr,
                                        NULL );
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
    le_mem_SetDestructor(DevicesPool,DevicesPoolDestructor);

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
    EventIdPool = le_mem_CreatePool("ATServerEventIdPool", sizeof(EventIdList_t));
    le_mem_ExpandPool(EventIdPool, CMD_POOL_SIZE);

    // Parameters pool allocation
    ParamStringPool = le_mem_CreatePool("ParamStringPool",sizeof(ParamString_t));
    le_mem_ExpandPool(ParamStringPool,PARAM_POOL_SIZE);

    // Parameters pool allocation
    RspStringPool = le_mem_CreatePool("RspStringPool",sizeof(RspString_t));
    le_mem_ExpandPool(RspStringPool,RSP_POOL_SIZE);

    // init EventIdList
    EventIdList = LE_DLS_LIST_INIT;

    MainThreadRef = le_thread_GetCurrent();
}
