/** @file le_atClient.c
 *
 * Implementation of AT commands client API.
 *
 * Copyright (C) Sierra Wireless Inc.
 */

/*
 * AT Command Client state machine
 *
 * @verbatim
 *
 *        EVENT_SENDCMD
 *          & Command NULL                               EVENT_SENDTEXT
 *            -----------                                 ------------
 *           |           |                               |            |
 *           \/          |         EVENT_SENDCMD         |            \/
 *    --------------    -       & Command not NULL        -    ----------------
 *   |              |   ---------------------------------->   |                |
 *   | WaitingState |                                         |  SendingState  |
 *   |              |   <----------------------------------   |                |
 *    --------------    -        EVENT_PROCESSLINE        -    ----------------
 *           /\          |     & Final pattern match     |            /\
 *           |           |                               |            |
 *            -----------                                 ------------
 *      EVENT_PROCESSLINE                                EVENT_PROCESSLINE
 *                                                      & Final pattern not match
 *
 * @endverbatim
 *
 *
 *
 *
 *
 * Rx Parser state machine
 *
 * @verbatim
 *
 *    ---------------                                           ---------------------
 *   |               |                 PARSER_CHAR             |                     |
 *   | StartingState |   ---------------------------------->   |  InitializingState  |
 *   |               |                                         |                     |
 *    ---------------                                           ---------------------
 *          |                                                            |
 *          |                                                            |
 *          |                                                            |
 *          |                     -----------------       PARSER_CRLF    |
 *          |                    |                 | <-------------------
 *           ---------------->   | ProcessingState | -----------------------
 *               PARSER_CRLF     |                 | --------------------   |
 *                                -----------------                      |  |
 *                                    /\       /\            PARSER_CRLF |  |
 *                                    |        |                         |  |
 *                                    |         -------------------------   |
 *                                     -------------------------------------
 *                                                 PARSER_PROMPT
 *
 * @verbatim
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "le_dev.h"
#include "watchdogChain.h"

//--------------------------------------------------------------------------------------------------
/**
 * Max length for error string
 */
//--------------------------------------------------------------------------------------------------
#define ERR_MSG_MAX 256

//--------------------------------------------------------------------------------------------------
/**
 * Max length of thread name
 */
//--------------------------------------------------------------------------------------------------
#define THREAD_NAME_MAX_LENGTH  30

//--------------------------------------------------------------------------------------------------
/**
 * Command responses pool size
 */
//--------------------------------------------------------------------------------------------------
#define RSP_POOL_SIZE       10

//--------------------------------------------------------------------------------------------------
/**
 * AT commands pool size
 */
//--------------------------------------------------------------------------------------------------
#define CMD_POOL_SIZE       5

//--------------------------------------------------------------------------------------------------
/**
 * Device pool size
 */
//--------------------------------------------------------------------------------------------------
#define DEVICE_POOL_SIZE    2

//--------------------------------------------------------------------------------------------------
/**
 * Unsolicited responses pool size
 */
//--------------------------------------------------------------------------------------------------
#define UNSOLICITED_POOL_SIZE 10

//--------------------------------------------------------------------------------------------------
/**
 * Rx Buffer length
 */
//--------------------------------------------------------------------------------------------------
#define PARSER_BUFFER_MAX_BYTES 1024

//--------------------------------------------------------------------------------------------------
/**
 * The timer interval to kick the watchdog chain.
 */
//--------------------------------------------------------------------------------------------------
#define MS_WDOG_INTERVAL 8

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration of AT Commands Client events
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    EVENT_SENDCMD=0,    ///< Send command
    EVENT_SENDTEXT,     ///< Send text
    EVENT_PROCESSLINE,  ///< Process line event
    STATE_MAX           ///< Unused
}
ClientEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * Enumeration for Rx parser events
 *
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    PARSER_CHAR=0,    ///< Any character except CRLF('\r\n') or PROMPT('>')
    PARSER_CRLF,      ///< CRLF ('\r\n')
    PARSER_PROMPT,    ///< PROMPT ('>')
    PARSER_MAX        ///< unused
}
RxEvent_t;

//--------------------------------------------------------------------------------------------------
/**
 * Rx buffer parser opaque structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct RxParser* RxParserPtr_t;

//--------------------------------------------------------------------------------------------------
/**
 * AT Command Client State Machine opaque structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct ClientState* ClientStatePtr_t;

//--------------------------------------------------------------------------------------------------
/**
 * device context opaque structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct DeviceContext* DeviceContextPtr_t;

//--------------------------------------------------------------------------------------------------
/**
 * Client state function prototype
 */
//--------------------------------------------------------------------------------------------------
typedef void (*ClientStateFunc_t)(ClientStatePtr_t parserStatePtr, ClientEvent_t input);

//--------------------------------------------------------------------------------------------------
/**
 * AT command client state machine structure
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct ClientState
{
    ClientStateFunc_t  prevState;      ///< Previous state for debugging purpose
    ClientStateFunc_t  curState;       ///< Current state
    ClientEvent_t      lastEvent;      ///< Last event received for debugging purpose
    DeviceContextPtr_t interfacePtr;
}
ClientState_t;

//--------------------------------------------------------------------------------------------------
/**
 * Response string structure
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char            line[LE_ATDEFS_RESPONSE_MAX_BYTES]; ///< string value
    le_dls_Link_t   link;                               ///< link for list
}
RspString_t;

//--------------------------------------------------------------------------------------------------
/**
 * Rx Data structure.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct RxData
{
    uint8_t  buffer[PARSER_BUFFER_MAX_BYTES];///< buffer read
    int32_t  idx;                            ///< index of parsing the buffer
    size_t   endBuffer;                      ///< index where the read was finished (idx<endbuffer)
    int32_t  idxLastCrLf;                    ///< index where the last CRLF has been found
}
RxData_t;

//--------------------------------------------------------------------------------------------------
/**
 * Rx parser function prototype
 */
//--------------------------------------------------------------------------------------------------
typedef void (*RxParserFunc_t)(RxParserPtr_t charParserPtr, RxEvent_t input);

//--------------------------------------------------------------------------------------------------
/**
 * Rx parser structure
 */
//--------------------------------------------------------------------------------------------------
typedef struct RxParser
{
    RxParserFunc_t      prevState;      ///< Previous state for debuging purpose
    RxParserFunc_t      curState;       ///< Current state
    RxEvent_t           lastEvent;      ///< Last event received for debugging purpose
    RxData_t            rxData;         ///< Read data structure
    DeviceContextPtr_t  interfacePtr;   ///< Device context
}
RxParser_t;

//--------------------------------------------------------------------------------------------------
/**
 * Unsolicited structure
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    le_atClient_UnsolicitedResponseHandlerFunc_t handlerPtr;    ///< Unsolicited handler
    void*         contextPtr;                                   ///< User context
    char          unsolRsp[LE_ATDEFS_UNSOLICITED_MAX_BYTES];    ///< pattern to match
    char          unsolBuffer[LE_ATDEFS_UNSOLICITED_MAX_BYTES]; ///< Unsolicited buffer
    uint32_t      lineCount;                                    ///< Unsolicited lines number
    uint32_t      lineCounter;                                  ///< Received line counter
    bool          inProgress;                                   ///< Reception in progress
    le_atClient_UnsolicitedResponseHandlerRef_t ref;            ///< Unsolicited reference
    DeviceContextPtr_t interfacePtr;                            ///< device context
    le_dls_Link_t link;                                         ///< link in Unsolicited List
    le_msg_SessionRef_t sessionRef;                             ///< client session reference
}
Unsolicited_t;



//--------------------------------------------------------------------------------------------------
/**
 * Interface context structure
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct DeviceContext
{
    le_thread_Ref_t threadRef;          ///< Thread reference
    ClientState_t   clientState;        ///< client state machine
    Device_t        device;             ///< data of the connected device
    RxParser_t      rxParser;           ///< Rx buffer parser context
    le_timer_Ref_t  timerRef;           ///< command timer
    le_dls_List_t   atCommandList;      ///< List of command waiting for execution
    le_dls_List_t   unsolicitedList;    ///< unsolicited command list
    le_sem_Ref_t    waitingSemaphore;   ///< semaphore used for synchronization
    le_atClient_DeviceRef_t ref;        ///< reference of the device context
    le_msg_SessionRef_t sessionRef;     ///< client session reference
}
DeviceContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure of an AT Command.
 */
//--------------------------------------------------------------------------------------------------
typedef struct AtCmd
{
    char                   cmd[LE_ATDEFS_COMMAND_MAX_BYTES];    ///< Command to send
    le_dls_List_t          ExpectintermediateResponseList;      ///< List of string pattern for
                                                                ///< intermediate response
    le_dls_List_t          expectResponseList;                  ///< List of str  pattern for final
                                                                ///< response
    char                   text[LE_ATDEFS_TEXT_MAX_BYTES+1];    ///< text to be sent after >
                                                                ///< +1 for ctrl-z
    size_t                 textSize;                            ///< size of text to send
    DeviceContext_t*       interfacePtr;                        ///< interface to send the command
    uint32_t               timeout;                             ///< command timeout (in ms)
    le_atClient_CmdRef_t   ref;                                 ///< command reference
    le_dls_List_t          responseList;                        ///< Responses list
    uint32_t               intermediateIndex;                   ///< current index for intermediate
                                                                ///< reponses reading
    uint32_t               responsesCount;                      ///< responses count in responseList
    le_sem_Ref_t           endSem;                              ///< end treatment semaphore
    le_result_t            result;                              ///< result operation
    le_dls_Link_t          link;                                ///< link in AT commands list
    le_msg_SessionRef_t    sessionRef;                          ///< client session reference
}
AtCmd_t;


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
static le_mem_PoolRef_t  AtCommandPool;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for responses string
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  RspStringPool;

//--------------------------------------------------------------------------------------------------
/**
 * Pool for unsolicited response
 */
//--------------------------------------------------------------------------------------------------
static le_mem_PoolRef_t  UnsolicitedPool;

//--------------------------------------------------------------------------------------------------
/**
 * Map for AT commands
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t   CmdRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Map for devices
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t DevicesRefMap;

//--------------------------------------------------------------------------------------------------
/**
 * Map for unsolicited
 */
//--------------------------------------------------------------------------------------------------
static le_ref_MapRef_t UnsolRefMap;

static void WaitingState(ClientStatePtr_t parserStatePtr,ClientEvent_t input);
static void SendingState(ClientStatePtr_t  parserStatePtr,ClientEvent_t input);

static void StartingState      (RxParserPtr_t charParserPtr,RxEvent_t input);
static void InitializingState  (RxParserPtr_t charParserPtr,RxEvent_t input);
static void ProcessingState    (RxParserPtr_t charParserPtr,RxEvent_t input);
static void UpdateTransitionManager(ClientStatePtr_t  parserStatePtr,
                                    ClientEvent_t input,
                                    ClientStateFunc_t newState);

static void SendLine(RxParserPtr_t charParserPtr);
static void SendData(RxParserPtr_t charParserPtr);

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to check if the received data matches with a subscribed unsolicited
 * response.
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckUnsolicited
(
    char* unsolRspPtr,
    size_t stringSize,
    le_dls_List_t *unsolListPtr
)
{
    LE_DEBUG("Start checking unsolicited");

    le_dls_Link_t* linkPtr = le_dls_Peek(unsolListPtr);

    /* Browse all the queue while the string is not found */
    while (linkPtr != NULL)
    {
        Unsolicited_t *unsolPtr = CONTAINER_OF(linkPtr,
                                               Unsolicited_t,
                                                link);

        if ((strncmp(unsolPtr->unsolRsp,unsolRspPtr,strlen(unsolPtr->unsolRsp)) == 0) ||
            (unsolPtr->inProgress))
        {
            LE_DEBUG("unsol found");
            uint32_t len =
                (stringSize < LE_ATDEFS_UNSOLICITED_MAX_LEN-strlen(unsolPtr->unsolBuffer)) ?
                stringSize :
                LE_ATDEFS_UNSOLICITED_MAX_LEN-strlen(unsolPtr->unsolBuffer);

            strncpy(unsolPtr->unsolBuffer+strlen(unsolPtr->unsolBuffer), unsolRspPtr, len);

            unsolPtr->inProgress = true;
        }

        if (unsolPtr->inProgress)
        {
            if ( (unsolPtr->lineCount - unsolPtr->lineCounter) == 1 )
            {
                unsolPtr->handlerPtr(unsolPtr->unsolBuffer, unsolPtr->contextPtr );
                memset(unsolPtr->unsolBuffer,0,LE_ATDEFS_UNSOLICITED_MAX_BYTES);
                unsolPtr->lineCounter = 0;
                unsolPtr->inProgress = false;
            }
            else
            {
                if (LE_ATDEFS_UNSOLICITED_MAX_BYTES - strlen(unsolPtr->unsolBuffer) > sizeof("\r\n"))
                {
                    snprintf(unsolPtr->unsolBuffer+strlen(unsolPtr->unsolBuffer),
                             sizeof("\r\n") + 1,    // +1 for Null terminator
                             "\r\n" );
                }

                unsolPtr->lineCounter++;
            }
        }

        linkPtr = le_dls_PeekNext(unsolListPtr, linkPtr);
    }

    LE_DEBUG("Stop checking unsolicited");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the next event to send to the Rx parser
 *
 * @return
 *      - true if a new event is detected and RX parser has to be processed
 *      - false otherwise
 *
 */
//--------------------------------------------------------------------------------------------------
static bool GetNextEvent
(
    RxParserPtr_t  charParserPtr,
    RxEvent_t     *evPtr
)
{
    int32_t idx = charParserPtr->rxData.idx++;
    if (idx < charParserPtr->rxData.endBuffer)
    {
        if (charParserPtr->rxData.buffer[idx] == '\r')
        {
            idx = charParserPtr->rxData.idx++;
            if (idx < charParserPtr->rxData.endBuffer)
            {
                if ( charParserPtr->rxData.buffer[idx] == '\n')
                {
                    *evPtr = PARSER_CRLF;
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                charParserPtr->rxData.idx--;
                return false;
            }
        }
        else if (charParserPtr->rxData.buffer[idx] == '\n')
        {
            if ( idx-1 > 0 )
            {
                if (charParserPtr->rxData.buffer[idx-1] == '\r')
                {
                    *evPtr = PARSER_CRLF;
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        else if (charParserPtr->rxData.buffer[idx] == '>')
        {
            *evPtr = PARSER_PROMPT;
            return true;
        }
        else
        {
            *evPtr = PARSER_CHAR;
            return true;
        }
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to read and send event to the Rx parser
 *
 */
//--------------------------------------------------------------------------------------------------
static void ParseRxBuffer
(
    RxParserPtr_t rxParserPtr
)
{
    RxEvent_t event;

    for (;rxParserPtr->rxData.idx < rxParserPtr->rxData.endBuffer;)
    {
        if (GetNextEvent(rxParserPtr, &event))
        {
            (rxParserPtr->curState)(rxParserPtr,event);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete characters that were already read.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ResetRxBuffer
(
    RxParserPtr_t rxParserPtr
)
{
    if (rxParserPtr->curState == ProcessingState)
    {
        size_t idx,sizeToCopy;
        sizeToCopy = rxParserPtr->rxData.endBuffer-rxParserPtr->rxData.idxLastCrLf+2;

        LE_DEBUG("%d sizeToCopy %zd from %d",
                            rxParserPtr->rxData.idx,sizeToCopy,rxParserPtr->rxData.idxLastCrLf-2);

        for (idx = 0; idx < sizeToCopy; idx++) {
            rxParserPtr->rxData.buffer[idx] =
                                rxParserPtr->rxData.buffer[idx+rxParserPtr->rxData.idxLastCrLf-2];
        }

        rxParserPtr->rxData.idxLastCrLf = 2;
        rxParserPtr->rxData.endBuffer = sizeToCopy;
        rxParserPtr->rxData.idx = rxParserPtr->rxData.endBuffer;
        LE_DEBUG("new idx %d, startLine %d",
                 rxParserPtr->rxData.idx,
                 rxParserPtr->rxData.idxLastCrLf);
    }
    else
    {
        LE_DEBUG("Nothing should be copied in RxData\n");
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the AT Command Client state machine.
 *
 */
//--------------------------------------------------------------------------------------------------
static void InitializeState
(
    DeviceContext_t* interfacePtr
)
{
    ClientStatePtr_t  clientStatePtr = &interfacePtr->clientState;
    clientStatePtr->curState = WaitingState;
    clientStatePtr->interfacePtr = interfacePtr;

    RxParserPtr_t rxParserPtr = &interfacePtr->rxParser;
    rxParserPtr->curState = StartingState;

    interfacePtr->timerRef = le_timer_Create("CommandTimer");
    interfacePtr->rxParser.interfacePtr = interfacePtr;
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

    ssize_t size = 0;
    DeviceContext_t *interfacePtr = le_fdMonitor_GetContextPtr();

    LE_DEBUG("Start read");

    /* Read RX data on uart */
    // PARSER_BUFFER_MAX_BYTES length is including '\0' character.
    size = le_dev_Read(&interfacePtr->device,
                         (uint8_t *)(&interfacePtr->rxParser.rxData.buffer) +
                         interfacePtr->rxParser.rxData.idx,
                         (PARSER_BUFFER_MAX_BYTES - interfacePtr->rxParser.rxData.idx - 1));

    /* Start the parsing only if we have read some bytes */
    if (size > 0)
    {
        interfacePtr->rxParser.rxData.buffer[interfacePtr->rxParser.rxData.idx + size] = '\0';
        interfacePtr->rxParser.rxData.endBuffer += size;

        /* Call the parser */
        LE_DEBUG("Parsing received data: %s", interfacePtr->rxParser.rxData.buffer);
        ParseRxBuffer(&interfacePtr->rxParser);
        ResetRxBuffer(&interfacePtr->rxParser);
    }

    if (interfacePtr->rxParser.rxData.endBuffer > PARSER_BUFFER_MAX_BYTES)
    {
        LE_WARN("Rx Buffer Overflow (FillIndex = %d)!!!", interfacePtr->rxParser.rxData.idx);
    }

    LE_DEBUG("read finished");
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
    DeviceContext_t* interfacePtr = contextPtr;
    le_dls_Link_t* linkPtr;

    LE_DEBUG("Destroy thread for interface %d", interfacePtr->device.fd);

    while ((linkPtr=le_dls_Pop(&interfacePtr->unsolicitedList)) != NULL)
    {
        Unsolicited_t *unsolPtr = CONTAINER_OF(linkPtr, Unsolicited_t, link);
        le_mem_Release(unsolPtr);
    }

    while ((linkPtr=le_dls_Pop(&interfacePtr->atCommandList)) != NULL)
    {
        AtCmd_t* atCmdPtr = CONTAINER_OF(linkPtr, AtCmd_t, link);
        le_mem_Release(atCmdPtr);
    }

    if (interfacePtr->timerRef)
    {
        le_timer_Delete(interfacePtr->timerRef);
    }

    if (interfacePtr->waitingSemaphore)
    {
        le_sem_Delete(interfacePtr->waitingSemaphore);
    }

    if (interfacePtr->device.fd)
    {
        le_dev_RemoveFdMonitoring(&interfacePtr->device);
        close(interfacePtr->device.fd);
    }
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
    DeviceContext_t *interfacePtr = context;
    LE_DEBUG("Start thread for %d", interfacePtr->device.fd);

    if (interfacePtr->device.fdMonitor)
    {
        LE_ERROR("Interface %d already monitored",interfacePtr->device.fd);
        return NULL;
    }

    InitializeState(interfacePtr);

    if (le_dev_AddFdMonitoring(&interfacePtr->device, RxNewData, interfacePtr) != LE_OK)
    {
        LE_ERROR("Error during adding the fd monitoring");
        return NULL;
    }

    le_sem_Post(interfacePtr->waitingSemaphore);

    le_event_RunLoop();

    return NULL; // Should not happen
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is used to do a transition between two state with one event.
 *
 */
//--------------------------------------------------------------------------------------------------
static void UpdateTransitionParser
(
    RxParserPtr_t     rxParserPtr,
    RxEvent_t         input,
    RxParserFunc_t    newState
)
{
    rxParserPtr->prevState   = rxParserPtr->curState;
    rxParserPtr->curState    = newState;
    rxParserPtr->lastEvent   = input;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to do a transition between two state with one event.
 *
 */
//--------------------------------------------------------------------------------------------------
static void UpdateTransitionManager
(
    ClientStatePtr_t  clientStatePtr,
    ClientEvent_t     input,
    ClientStateFunc_t newState
)
{
    clientStatePtr->prevState   = clientStatePtr->curState;
    clientStatePtr->curState    = newState;
    clientStatePtr->lastEvent   = input;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the timer of a command
 *
 */
//--------------------------------------------------------------------------------------------------
static void StopTimer
(
    AtCmd_t* cmdPtr
)
{
    le_timer_Stop(cmdPtr->interfacePtr->timerRef);
}

//--------------------------------------------------------------------------------------------------
/**
 * Timer handler (called when the AT command timeout is reached)
 *
 */
//--------------------------------------------------------------------------------------------------
static void TimerHandler
(
    le_timer_Ref_t timerRef
)
{
    AtCmd_t* atCmdPtr = le_timer_GetContextPtr(timerRef);

    LE_ERROR("Timeout when sending %s, timeout = %d",  atCmdPtr->cmd, atCmdPtr->timeout);
    atCmdPtr->result = LE_TIMEOUT;
    le_dls_Pop(&atCmdPtr->interfacePtr->atCommandList);
    le_sem_Post(atCmdPtr->endSem);
    ClientStatePtr_t clientStatePtr = &atCmdPtr->interfacePtr->clientState;

    UpdateTransitionManager(clientStatePtr,EVENT_SENDCMD,WaitingState);

    // Send the next command
    (clientStatePtr->curState)(clientStatePtr,EVENT_SENDCMD);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start the timer of a command
 *
 */
//--------------------------------------------------------------------------------------------------
static void StartTimer
(
    AtCmd_t* cmdPtr
)
{
    le_timer_SetHandler(cmdPtr->interfacePtr->timerRef,
                        TimerHandler);

    le_timer_SetContextPtr(cmdPtr->interfacePtr->timerRef,
                           cmdPtr);

    le_timer_SetMsInterval(cmdPtr->interfacePtr->timerRef,cmdPtr->timeout);

    le_timer_Start(cmdPtr->interfacePtr->timerRef);
}



//--------------------------------------------------------------------------------------------------
/**
 * This function is used to check if the line matches any of response strings of the command
 *
 * @return
 *      - TRUE if the line matches a response string of the command
 *      - FALSE otherwise
 */
//--------------------------------------------------------------------------------------------------
static bool CheckResponse
(
    char*          receivedRspPtr,   ///< [IN] Received line pointer
    size_t         lineSize,         ///< [IN] Received line size
    le_dls_List_t* responseListPtr,  ///< [IN] List of response strings of the command
    le_dls_List_t* resultListPtr,    ///< [OUT] List of matched strings after comparison
    char*          cmdNamePtr        ///< [IN] Command name pointer
)
{
    LE_DEBUG("Start checking response");

    if (!lineSize)
    {
        return false;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(responseListPtr);

    LE_DEBUG("Command: %s, size: %zu", cmdNamePtr, strlen(cmdNamePtr));
    LE_DEBUG("Received response: %s, size: %zu", receivedRspPtr, lineSize);

    if (strncmp(cmdNamePtr, receivedRspPtr, strlen(cmdNamePtr)) == 0)
    {
        LE_DEBUG("Found command echo in response");
        return false;
    }

    /* Browse all the queue while the string is not found */
    while (linkPtr != NULL)
    {
        RspString_t* currStringPtr = CONTAINER_OF(linkPtr,
                                                  RspString_t,
                                                  link);
        LE_DEBUG("Item: %s, size: %zu", currStringPtr->line, strlen(currStringPtr->line));

        if ((strlen(currStringPtr->line) == 0) ||
           ((lineSize >= strlen(currStringPtr->line)) &&
           (strncmp(currStringPtr->line, receivedRspPtr, strlen(currStringPtr->line)) == 0)))
        {
            LE_DEBUG("Rsp matched, size: %zu", lineSize);

            RspString_t* newStringPtr = le_mem_ForceAlloc(RspStringPool);
            memset(newStringPtr, 0, sizeof(RspString_t));

            if(lineSize>LE_ATDEFS_RESPONSE_MAX_BYTES)
            {
                LE_ERROR("String too long");
                le_mem_Release(newStringPtr);
                return false;
            }

            strncpy(newStringPtr->line, receivedRspPtr, lineSize);
            newStringPtr->link = LE_DLS_LINK_INIT;
            le_dls_Queue(resultListPtr, &(newStringPtr->link));
            return true;
        }

        linkPtr = le_dls_PeekNext(responseListPtr, linkPtr);
    }

    LE_DEBUG("Stop checking response");
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the AT Command Client state machine.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendingState
(
    ClientStatePtr_t clientStatePtr,
    ClientEvent_t    input
)
{
    LE_DEBUG("%d", input);

    DeviceContext_t* interfacePtr = clientStatePtr->interfacePtr;

    le_dls_Link_t* linkPtr = le_dls_Peek(&(interfacePtr->atCommandList));

    if (linkPtr==NULL)
    {
        LE_DEBUG("No more command to execute");
        return;
    }

    AtCmd_t* cmdPtr = CONTAINER_OF(linkPtr, AtCmd_t, link);

    switch (input)
    {
        case EVENT_SENDTEXT:
        {
            // Send data
            le_dev_Write(&(interfacePtr->device),
                           (uint8_t*) cmdPtr->text,
                           cmdPtr->textSize);

            // Send Ctrl-z
            uint8_t ctrlZ = 0x1A;
            le_dev_Write(&(interfacePtr->device),
                         &ctrlZ,
                         1);

            break;
        }
        case EVENT_PROCESSLINE:
        {
            RxData_t* parserPtr = &interfacePtr->rxParser.rxData;

            //~CheckUnsolicited(smRef,
                             //~smRef->curContext.atLine,
                             //~strlen(smRef->curContext.atLine));
            int32_t newCRLF = parserPtr->idx-2;
            size_t lineSize = newCRLF - parserPtr->idxLastCrLf;

            if (CheckResponse((char*)&(parserPtr->buffer[parserPtr->idxLastCrLf]), lineSize,
                              &(cmdPtr->expectResponseList), &(cmdPtr->responseList),
                              cmdPtr->cmd))
            {
                LE_DEBUG("Final command found");

                le_dls_Pop(&interfacePtr->atCommandList);

                cmdPtr->result = LE_OK;
                StopTimer(cmdPtr);
                le_sem_Post(cmdPtr->endSem);

                UpdateTransitionManager(clientStatePtr,input,WaitingState);
                // Send the next command
                (clientStatePtr->curState)(clientStatePtr,EVENT_SENDCMD);
                return;
            }

            CheckResponse((char*)&(parserPtr->buffer[parserPtr->idxLastCrLf]), lineSize,
                          &(cmdPtr->ExpectintermediateResponseList), &(cmdPtr->responseList),
                          cmdPtr->cmd);
            break;
        }
        default:
        {
            LE_WARN("This event(%d) is not usefull in state 'SendingState'",input);
            break;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the AT Command Client state machine.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WaitingState
(
    ClientStatePtr_t clientStatePtr,
    ClientEvent_t    input
)
{
    DeviceContext_t* interfacePtr = clientStatePtr->interfacePtr;

    LE_DEBUG("input %d", input);

    switch (input)
    {
        case EVENT_SENDCMD:
        {
            // Send at command
            le_dls_Link_t* linkPtr = le_dls_Peek(&(interfacePtr->atCommandList));

            if (linkPtr==NULL)
            {
                LE_DEBUG("No more command to execute");
                return;
            }

            AtCmd_t* cmdPtr = CONTAINER_OF(linkPtr, AtCmd_t, link);

            if (cmdPtr->timeout > 0)
            {
                StartTimer(cmdPtr);
            }

            uint32_t len = strlen(cmdPtr->cmd)+2;
            char atCommand[len];
            memset(atCommand, 0, len);
            snprintf(atCommand, len, "%s\r", cmdPtr->cmd);

            le_dev_Write(&(interfacePtr->device),
                           (uint8_t*) atCommand,
                           len-1);

            UpdateTransitionManager(clientStatePtr,input,SendingState);

            break;
        }
        case EVENT_PROCESSLINE:
        {
            RxData_t* parserPtr = &interfacePtr->rxParser.rxData;

            int32_t newCRLF = parserPtr->idx-2;
            size_t lineSize = newCRLF - parserPtr->idxLastCrLf;

            CheckUnsolicited((char*)&(parserPtr->buffer[parserPtr->idxLastCrLf]),
                              lineSize,
                              &interfacePtr->unsolicitedList);
            break;
        }
        default:
        {
            LE_WARN("This event(%d) is not usefull in state 'WaitingState'",input);
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the Rx data parser.
 *
 */
//--------------------------------------------------------------------------------------------------
static void StartingState
(
    RxParserPtr_t rxParserPtr,
    RxEvent_t     input
)
{
    LE_DEBUG("%d", input);

    switch (input)
    {
        case PARSER_CRLF:
            rxParserPtr->rxData.idxLastCrLf = rxParserPtr->rxData.idx;
            UpdateTransitionParser(rxParserPtr,input,ProcessingState);
            break;
        case PARSER_CHAR:
            UpdateTransitionParser(rxParserPtr,input,InitializingState);
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the Rx data parser.
 *
 */
//--------------------------------------------------------------------------------------------------
static void InitializingState
(
    RxParserPtr_t rxParserPtr,
    RxEvent_t     input
)
{
    LE_DEBUG("%d", input);

    switch (input)
    {
        case PARSER_CRLF:
            rxParserPtr->rxData.idxLastCrLf = rxParserPtr->rxData.idx;
            UpdateTransitionParser(rxParserPtr,input,ProcessingState);
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the Rx data parser.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ProcessingState
(
    RxParserPtr_t rxParserPtr,
    RxEvent_t     input
)
{
    LE_DEBUG("%d", input);

    switch (input)
    {
        case PARSER_CRLF:
            SendLine(rxParserPtr);
            UpdateTransitionParser(rxParserPtr,input,ProcessingState);
            break;
        case PARSER_PROMPT:
            SendData(rxParserPtr);
            UpdateTransitionParser(rxParserPtr,input,ProcessingState);
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the Rx data parser.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendData
(
    RxParserPtr_t rxParserPtr
)
{
    LE_DEBUG("Send text");
    ClientStatePtr_t clientStatePtr = &rxParserPtr->interfacePtr->clientState;

    (clientStatePtr->curState)(clientStatePtr,EVENT_SENDTEXT);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to send the Line found between to CRLF (\\r\\n)
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendLine
(
    RxParserPtr_t rxParserPtr
)
{
    ClientStatePtr_t clientStatePtr = &rxParserPtr->interfacePtr->clientState;

    (clientStatePtr->curState)(clientStatePtr,EVENT_PROCESSLINE);

    rxParserPtr->rxData.idxLastCrLf = rxParserPtr->rxData.idx;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release all string list.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ReleaseRspStringList
(
    le_dls_List_t* listPtr
)
{
    le_dls_Link_t* linkPtr;

    while ((linkPtr=le_dls_Pop(listPtr)) != NULL)
    {
        RspString_t * currentPtr = CONTAINER_OF(linkPtr,RspString_t,link);
        le_mem_Release(currentPtr);
    }
    LE_DEBUG("All string has been released");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is to send a new AT command
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendCommand
(
    void *param1Ptr,
    void *param2Ptr
)
{
    DeviceContext_t* interfacePtr = param1Ptr;

    if (interfacePtr)
    {
        ClientState_t* clientState = &interfacePtr->clientState;
        (clientState->curState)(clientState,EVENT_SENDCMD);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is the destructor for AtCmd_t struct
 *
 */
//--------------------------------------------------------------------------------------------------
static void AtCmdPoolDestructor
(
    void *ptr
)
{
    AtCmd_t* oldPtr = ptr;

    LE_DEBUG("Destroy AT command %s", oldPtr->cmd);

    ReleaseRspStringList(&(oldPtr->responseList));
    ReleaseRspStringList(&(oldPtr->expectResponseList));
    ReleaseRspStringList(&(oldPtr->ExpectintermediateResponseList));

    le_ref_DeleteRef(CmdRefMap, oldPtr->ref);
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
    DeviceContext_t* interfacePtr = ptr;

    if (le_thread_Cancel(interfacePtr->threadRef))
    {
        LE_ERROR("failed to Cancel device thread");
        return;
    }

    le_thread_Join(interfacePtr->threadRef,NULL);

    le_ref_DeleteRef(DevicesRefMap, interfacePtr->ref);

}

//--------------------------------------------------------------------------------------------------
/**
 * This function is the destructor for Unsolicited_t struct
 *
 */
//--------------------------------------------------------------------------------------------------
static void UnsolicitedPoolDestructor
(
    void *ptr
)
{
    Unsolicited_t* unsolicitedPtr = ptr;
    le_dls_List_t* listPtr;
    le_dls_Link_t* linkPtr;

    listPtr = &unsolicitedPtr->interfacePtr->unsolicitedList;
    linkPtr = &unsolicitedPtr->link;

    LE_DEBUG("Destroy unsolicited %s", unsolicitedPtr->unsolRsp);

    if ( le_dls_IsInList(listPtr, linkPtr) )
    {
        le_dls_Remove(listPtr, linkPtr);
    }

    // Delete the reference for unsolicited structure pointer.
    le_ref_DeleteRef(UnsolRefMap, unsolicitedPtr->ref);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the intermediate response at specified index
 *
 */
//--------------------------------------------------------------------------------------------------
static char* GetIntermediateResponse
(
    le_dls_List_t* responseListPtr,
    uint32_t       index
)
{
    le_dls_Link_t* linkPtr;

    uint32_t i;

    linkPtr = le_dls_Peek(responseListPtr);

    for(i=0;i<index;i++)
    {
        linkPtr = le_dls_PeekNext(responseListPtr, linkPtr);
    }
    if (linkPtr)
    {
        RspString_t* rspPtr;

        rspPtr = CONTAINER_OF(linkPtr, RspString_t, link);

        return rspPtr->line;
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function removes an unsolicited response subscription.
 */
//--------------------------------------------------------------------------------------------------
static void RemoveUnsolicited
(
    void* param1Ptr,
    void* param2Ptr
)
{
    Unsolicited_t* unsolicitedPtr = param1Ptr;

    le_mem_Release(unsolicitedPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a new AT command.
 *
 * @return pointer to the new AT Command reference
 */
//--------------------------------------------------------------------------------------------------
le_atClient_CmdRef_t le_atClient_Create
(
    void
)
{
    AtCmd_t* cmdPtr = le_mem_ForceAlloc(AtCommandPool);

    // Initialize the command object
    memset(cmdPtr,0,sizeof(AtCmd_t));

    cmdPtr->ExpectintermediateResponseList  = LE_DLS_LIST_INIT;
    cmdPtr->expectResponseList              = LE_DLS_LIST_INIT;
    cmdPtr->textSize                        = 0;
    cmdPtr->timeout                         = LE_ATDEFS_COMMAND_DEFAULT_TIMEOUT;
    cmdPtr->interfacePtr                    = NULL;
    cmdPtr->ref                             = le_ref_CreateRef(CmdRefMap, cmdPtr);
    cmdPtr->intermediateIndex               = 0;
    cmdPtr->responseList                    = LE_DLS_LIST_INIT;
    cmdPtr->link                            = LE_DLS_LINK_INIT;
    cmdPtr->sessionRef                      = le_atClient_GetClientSessionRef();

    return cmdPtr->ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the device where the AT command will be sent.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference is invalid, a fatal error occurs,
 *       the function won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetDevice
(
    le_atClient_CmdRef_t cmdRef,
        ///< [IN] AT Command

    le_atClient_DeviceRef_t devRef
        ///< [IN] Device where the AT command has to be sent
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);

    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdRef);
        return LE_BAD_PARAMETER;
    }

    DeviceContext_t* interfacePtr = le_ref_Lookup(DevicesRefMap, devRef);

    if (interfacePtr == NULL)
    {
        LE_ERROR("Invalid device");
        return LE_FAULT;
    }

    cmdPtr->interfacePtr = interfacePtr;

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete an AT command reference.
 *
 * @return
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference is invalid, a fatal error occurs,
 *       the function won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_Delete
(
    le_atClient_CmdRef_t cmdRef
        ///< [IN] AT Command
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdRef);
        return LE_BAD_PARAMETER;
    }

    le_mem_Release(cmdPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the AT command string to be sent.
 *
 * @return
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference is invalid, a fatal error occurs,
 *       the function won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetCommand
(
    le_atClient_CmdRef_t cmdRef,
        ///< [IN] AT Command

    const char* commandPtr
        ///< [IN] Set Command
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdRef);
        return LE_BAD_PARAMETER;
    }

    le_utf8_Copy(cmdPtr->cmd, commandPtr, sizeof(cmdPtr->cmd), NULL);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the waiting intermediate responses.
 * Several intermediate responses can be specified separated by a '|' character into the string
 * given in parameter.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference or set intermediate response is invalid, a fatal error occurs,
 *       the function won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetIntermediateResponse
(
    le_atClient_CmdRef_t cmdRef,
        ///< [IN] AT Command

    const char* intermediatePtr
        ///< [IN] Set Intermediate
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdRef);
        return LE_BAD_PARAMETER;
    }

    if (intermediatePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid intermediatePtr (%p) provided!", intermediatePtr);
        return LE_BAD_PARAMETER;
    }

    if (strcmp(intermediatePtr, "\0") != 0)
    {
        size_t len = strlen(intermediatePtr) + 1;
        char tmpIntermediate[len];
        memcpy(tmpIntermediate, intermediatePtr, len);
        char *savePtr = (char*) tmpIntermediate;
        char *interPtr = strtok_r((char*) tmpIntermediate, "|", &savePtr);

        while(interPtr != NULL)
        {
            RspString_t* newStringPtr = le_mem_ForceAlloc(RspStringPool);
            memset(newStringPtr, 0, sizeof(RspString_t));

            le_utf8_Copy(newStringPtr->line, interPtr, LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);

            newStringPtr->link = LE_DLS_LINK_INIT;

            le_dls_Queue(&(cmdPtr->ExpectintermediateResponseList), &(newStringPtr->link));

            interPtr = strtok_r(NULL, "|", &savePtr);
        }
    }
    else
    {
        RspString_t* newStringPtr = le_mem_ForceAlloc(RspStringPool);
        memset(newStringPtr, 0, sizeof(RspString_t));
        newStringPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(cmdPtr->ExpectintermediateResponseList), &(newStringPtr->link));

    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the final response(s) of the AT command execution.
 * Several final responses can be specified separated by a '|' character into the string given in
 * parameter.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference or set response is invalid, a fatal error occurs,
 *       the function won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetFinalResponse
(
    le_atClient_CmdRef_t cmdRef,
        ///< [IN] AT Command

    const char* responsePtr
        ///< [IN] Set Response
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdRef);
        return LE_BAD_PARAMETER;
    }

    if (responsePtr == NULL)
    {
        LE_KILL_CLIENT("Invalid responsePtr (%p) provided!", responsePtr);
        return LE_BAD_PARAMETER;
    }


    if (strcmp(responsePtr, "\0") != 0)
    {
        size_t len = strlen(responsePtr) + 1;
        char tmpResponse[len];
        memcpy(tmpResponse, responsePtr, len);
        char *savePtr = (char*) tmpResponse;
        char *respPtr = strtok_r((char*) tmpResponse, "|", &savePtr);

        while(respPtr != NULL)
        {
            RspString_t* newStringPtr = le_mem_ForceAlloc(RspStringPool);
            memset(newStringPtr,0,sizeof(RspString_t));

            le_utf8_Copy(newStringPtr->line,respPtr,LE_ATDEFS_RESPONSE_MAX_BYTES, NULL);

            newStringPtr->link = LE_DLS_LINK_INIT;

            le_dls_Queue(&(cmdPtr->expectResponseList),&(newStringPtr->link));

            respPtr = strtok_r(NULL, "|", &savePtr);
        }
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the text when the prompt is expected.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference is invalid, a fatal error occurs,
 *       the function won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetText
(
    le_atClient_CmdRef_t cmdRef,
        ///< [IN] AT Command

    const char* textPtr
        ///< [IN] The AT Data to send
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdRef);
        return LE_BAD_PARAMETER;
    }

    if(strlen(textPtr)>LE_ATDEFS_TEXT_MAX_LEN)
    {
        LE_ERROR("Text is too long! (%zd>%d)",strlen(textPtr),LE_ATDEFS_TEXT_MAX_LEN);
        return LE_FAULT;
    }

    memcpy(cmdPtr->text, textPtr, strlen(textPtr));
    cmdPtr->textSize = strlen(textPtr);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the timeout of the AT command execution.
 *
 * @return
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference is invalid, a fatal error occurs,
 *       the function won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetTimeout
(
    le_atClient_CmdRef_t cmdRef,
        ///< [IN] AT Command

    uint32_t timer
        ///< [IN] Set Timer
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdRef);
        return LE_BAD_PARAMETER;
    }

    cmdPtr->timeout = timer;
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send an AT Command and wait for response.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_TIMEOUT when a timeout occur
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference is invalid, a fatal error occurs,
 *       the function won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_Send
(
    le_atClient_CmdRef_t cmdRef
        ///< [IN] AT Command
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdRef);
        return LE_BAD_PARAMETER;
    }

    if (cmdPtr->interfacePtr == NULL)
    {
        LE_ERROR("no device set");
        return LE_FAULT;
    }

    if (le_dls_NumLinks(&cmdPtr->expectResponseList) == 0)
    {
        LE_ERROR("no final responses set");
        return LE_FAULT;
    }

    if (le_dls_NumLinks(&cmdPtr->ExpectintermediateResponseList) == 0)
    {
        if (le_atClient_SetIntermediateResponse(cmdRef,"") != LE_OK)
        {
            LE_ERROR("Can't set intermediate rsp");
            return LE_FAULT;
        }
    }

    cmdPtr->endSem = le_sem_Create("ResultSignal",0);
    le_dls_Queue(&cmdPtr->interfacePtr->atCommandList, &cmdPtr->link);

    ReleaseRspStringList(&cmdPtr->responseList);

    le_event_QueueFunctionToThread(cmdPtr->interfacePtr->threadRef,
                                                SendCommand,
                                                (void*) cmdPtr->interfacePtr,
                                                (void*) NULL);

    le_sem_Wait(cmdPtr->endSem);

    le_sem_Delete(cmdPtr->endSem);

    return cmdPtr->result;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the first intermediate response.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference is invalid, a fatal error occurs,
 *       the function won't return.
*/
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_GetFirstIntermediateResponse
(
    le_atClient_CmdRef_t cmdRef,
        ///< [IN] AT Command

    char* intermediateRspPtr,
        ///< [OUT] Get Next Line

    size_t intermediateRspNumElements
        ///< [IN]
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);

    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdRef);
        return LE_BAD_PARAMETER;
    }

    cmdPtr->responsesCount = le_dls_NumLinks(&cmdPtr->responseList);
    cmdPtr->intermediateIndex = 0;

    if (cmdPtr->responsesCount > 1)
    {
        char* firstLinePtr = GetIntermediateResponse(&cmdPtr->responseList,0);

        if (firstLinePtr)
        {
            snprintf(intermediateRspPtr, intermediateRspNumElements, "%s", firstLinePtr);
            return LE_OK;
        }
    }

    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the next intermediate response.
 *
 * @return
 *      - LE_NOT_FOUND when there are no further results.
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference is invalid, a fatal error occurs,
 *       the function won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_GetNextIntermediateResponse
(
    le_atClient_CmdRef_t cmdRef,
        ///< [IN] AT Command

    char* intermediateRspPtr,
        ///< [OUT] Get Next Line

    size_t intermediateRspNumElements
        ///< [IN]
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdRef);
        return LE_BAD_PARAMETER;
    }

    cmdPtr->intermediateIndex += 1;

    if (cmdPtr->intermediateIndex < cmdPtr->responsesCount-1)
    {
        char* firstLinePtr = GetIntermediateResponse(&cmdPtr->responseList,
                                                     cmdPtr->intermediateIndex);

        if (firstLinePtr)
        {
            snprintf(intermediateRspPtr, intermediateRspNumElements, "%s", firstLinePtr);
            return LE_OK;
        }
    }

    return LE_NOT_FOUND;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the final response
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_OK when function succeed
 *
 * @note If the AT Command reference is invalid, a fatal error occurs,
 *       the function won't return.
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_GetFinalResponse
(
    le_atClient_CmdRef_t cmdRef,
        ///< [IN] AT Command

    char* finalRspPtr,
        ///< [OUT] Get Final Line

    size_t finalRspNumElements
        ///< [IN]
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdRef);
        return LE_BAD_PARAMETER;
    }

    le_dls_Link_t* linkPtr;
    RspString_t* rspPtr;

    if ((linkPtr = le_dls_PeekTail(&cmdPtr->responseList)) != NULL)
    {
        rspPtr = CONTAINER_OF(linkPtr, RspString_t, link);
    }
    else
    {
        return LE_FAULT;
    }

    snprintf(finalRspPtr, finalRspNumElements, "%s", rspPtr->line);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to automatically set and send an AT Command.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_TIMEOUT when a timeout occur
 *      - LE_OK when function succeed
 *
 * @note This command creates a command reference when called
 *
 * @note In case of an Error the command reference will be deleted and though
 *       not usable. Make sure to test the return code and not use the reference
 *       in other functions.
 *
 * @note If the AT command is invalid, a fatal error occurs,
 *       the function won't return.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetCommandAndSend
(
    le_atClient_CmdRef_t* cmdRefPtr,
        ///< [OUT] Command reference

    le_atClient_DeviceRef_t devRef,
        ///< [IN] Device reference

    const char* commandPtr,
        ///< [IN] AT Command

    const char* interRespPtr,
        ///< [IN] Expected Intermediate Response

    const char* finalRespPtr,
        ///< [IN] Expected Final Response

    uint32_t timeout
        ///< [IN] Timeout
)
{
    if (cmdRefPtr  == NULL)
    {
        LE_KILL_CLIENT("cmdRefPtr is NULL.");
        return LE_FAULT;
    }

    le_result_t res = LE_FAULT;

    *cmdRefPtr = le_atClient_Create();
    LE_DEBUG("New command ref (%p) created",*cmdRefPtr);
    if(*cmdRefPtr == NULL)
    {
        return LE_FAULT;
    }

    res = le_atClient_SetCommand(*cmdRefPtr,commandPtr);
    if (res != LE_OK)
    {
        le_atClient_Delete(*cmdRefPtr);
        LE_ERROR("Failed to set the command !");
        return res;
    }

    res = le_atClient_SetDevice(*cmdRefPtr,devRef);
    if (res != LE_OK)
    {
        le_atClient_Delete(*cmdRefPtr);
        LE_ERROR("Failed to set the command !");
        return res;
    }

    res = le_atClient_SetIntermediateResponse(*cmdRefPtr,interRespPtr);
    if (res != LE_OK)
    {
        le_atClient_Delete(*cmdRefPtr);
        LE_ERROR("Failed to set intermediate response !");
        return res;
    }

    res = le_atClient_SetFinalResponse(*cmdRefPtr,finalRespPtr);
    if (res != LE_OK)
    {
        le_atClient_Delete(*cmdRefPtr);
        LE_ERROR("Failed to set final response !");
        return res;
    }

    if(timeout > 0)
    {
        res = le_atClient_SetTimeout(*cmdRefPtr, timeout);
        if (res != LE_OK)
        {
            le_atClient_Delete(*cmdRefPtr);
            LE_ERROR("Failed to send !");
            return res;
        }
    }

    res = le_atClient_Send(*cmdRefPtr);
    if (res != LE_OK)
    {
        le_atClient_Delete(*cmdRefPtr);
        LE_ERROR("Failed to send !");
        return res;
    }

    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * This event provides information on a subscribed unsolicited response when this unsolicited
 * response is received.
 *
 */
//--------------------------------------------------------------------------------------------------
le_atClient_UnsolicitedResponseHandlerRef_t le_atClient_AddUnsolicitedResponseHandler
(
    const char* unsolRsp,
        ///< [IN] Pattern to match

    le_atClient_DeviceRef_t devRef,
        ///< [IN] Device to listen

    le_atClient_UnsolicitedResponseHandlerFunc_t handlerPtr,
        ///< [IN]

    void* contextPtr,
        ///< [IN]

    uint32_t lineCount
)
{
    DeviceContext_t* interfacePtr = le_ref_Lookup(DevicesRefMap, devRef);

    if (interfacePtr == NULL)
    {
        LE_ERROR("Invalid device");
        return NULL;
    }

    Unsolicited_t* unsolicitedPtr = le_mem_ForceAlloc(UnsolicitedPool);

    memset(unsolicitedPtr, 0 ,sizeof(Unsolicited_t));
    le_utf8_Copy(unsolicitedPtr->unsolRsp, unsolRsp, LE_ATDEFS_UNSOLICITED_MAX_BYTES, NULL);
    unsolicitedPtr->lineCount    = lineCount;
    unsolicitedPtr->handlerPtr = handlerPtr;
    unsolicitedPtr->contextPtr = contextPtr;
    unsolicitedPtr->inProgress = false;
    unsolicitedPtr->ref = le_ref_CreateRef(UnsolRefMap, unsolicitedPtr);
    unsolicitedPtr->interfacePtr = interfacePtr;
    unsolicitedPtr->link = LE_DLS_LINK_INIT;
    unsolicitedPtr->sessionRef = le_atClient_GetClientSessionRef();

    le_dls_Queue(&interfacePtr->unsolicitedList, &unsolicitedPtr->link);

    return unsolicitedPtr->ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * Remove handler function for EVENT 'le_atClient_UnsolicitedResponse'
 */
//--------------------------------------------------------------------------------------------------
void le_atClient_RemoveUnsolicitedResponseHandler
(
    le_atClient_UnsolicitedResponseHandlerRef_t addHandlerRef
        ///< [IN]
)
{
    Unsolicited_t* unsolicitedPtr = le_ref_Lookup(UnsolRefMap, addHandlerRef);

    if (unsolicitedPtr)
    {
        le_event_QueueFunctionToThread(unsolicitedPtr->interfacePtr->threadRef,
                                   RemoveUnsolicited,
                                   (void*) unsolicitedPtr,
                                   (void*) NULL);

        le_ref_DeleteRef(UnsolRefMap, addHandlerRef);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Handler function to the close session service
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
    AtCmd_t *cmdPtr = NULL;
    DeviceContext_t *devPtr = NULL;
    Unsolicited_t *unsolPtr  = NULL;

    iter = le_ref_GetIterator(UnsolRefMap);
    while (LE_OK == le_ref_NextNode(iter))
    {
        unsolPtr = (Unsolicited_t *) le_ref_GetValue(iter);
        if (unsolPtr)
        {
            if (sessionRef == unsolPtr->sessionRef)
            {
                le_mem_Release(unsolPtr);
            }
        }
    }

    iter = le_ref_GetIterator(CmdRefMap);
    while (LE_OK == le_ref_NextNode(iter))
    {
        cmdPtr = (AtCmd_t *) le_ref_GetValue(iter);
        if (cmdPtr)
        {
            if (sessionRef == cmdPtr->sessionRef)
            {
                le_mem_Release(cmdPtr);
            }
        }
    }

    iter = le_ref_GetIterator(DevicesRefMap);
    while (LE_OK == le_ref_NextNode(iter))
    {
        devPtr = (DeviceContext_t *) le_ref_GetValue(iter);
        if (devPtr)
        {
            if (sessionRef == devPtr->sessionRef)
            {
                le_mem_Release(devPtr);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start a ATClient session on a specified device.
 *
 * @return reference on a device context
 */
//--------------------------------------------------------------------------------------------------
le_atClient_DeviceRef_t le_atClient_Start
(
    int32_t              fd          ///< The file descriptor
)
{
    char name[THREAD_NAME_MAX_LENGTH];
    static uint32_t threatCounter = 1;
    char errMsg[ERR_MSG_MAX] = {0};

    // check if the file descriptor is valid
    if (fcntl(fd, F_GETFD) == -1)
    {
        LE_ERROR("%s", strerror_r(errno, errMsg, ERR_MSG_MAX));
        return NULL;
    }

    DeviceContext_t* newInterfacePtr = le_mem_ForceAlloc(DevicesPool);

    memset(newInterfacePtr,0,sizeof(DeviceContext_t));

    LE_DEBUG("Create a new interface for '%d'", fd);
    newInterfacePtr->device.fd = fd;

    snprintf(name,THREAD_NAME_MAX_LENGTH,"atCommandClient-%d",threatCounter);
    newInterfacePtr->threadRef = le_thread_Create(name,DeviceThread,newInterfacePtr);

    memset(name,0,THREAD_NAME_MAX_LENGTH);
    snprintf(name,THREAD_NAME_MAX_LENGTH,"ItfWaitSemaphore-%d",threatCounter);
    newInterfacePtr->waitingSemaphore = le_sem_Create(name,0);

    newInterfacePtr->sessionRef = le_atClient_GetClientSessionRef();

    threatCounter++;

    le_thread_AddChildDestructor(newInterfacePtr->threadRef,
                                 DestroyDeviceThread,
                                 newInterfacePtr);

    le_thread_SetJoinable(newInterfacePtr->threadRef);

    le_thread_Start(newInterfacePtr->threadRef);
    le_sem_Wait(newInterfacePtr->waitingSemaphore);

    newInterfacePtr->ref = le_ref_CreateRef(DevicesRefMap, newInterfacePtr);
    return newInterfacePtr->ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the ATClient session on the specified device.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_Stop
(
    le_atClient_DeviceRef_t devRef
)
{
    DeviceContext_t* interfacePtr = le_ref_Lookup(DevicesRefMap, devRef);

    if (interfacePtr == NULL)
    {
        LE_ERROR("Invalid device");
        return LE_FAULT;
    }

    le_mem_Release(interfacePtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * The COMPONENT_INIT intialize the AT Client Component when Legato start
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    // Device pool allocation
    DevicesPool = le_mem_CreatePool("AtClientDevicesPool",sizeof(DeviceContext_t));
    le_mem_ExpandPool(DevicesPool,DEVICE_POOL_SIZE);
    DevicesRefMap = le_ref_CreateMap("DevicesRefMap", DEVICE_POOL_SIZE);
    le_mem_SetDestructor(DevicesPool,DevicesPoolDestructor);

    // AT commands pool allocation
    AtCommandPool = le_mem_CreatePool("AtCommandPool",sizeof(AtCmd_t));
    le_mem_ExpandPool(AtCommandPool, CMD_POOL_SIZE);
    le_mem_SetDestructor(AtCommandPool,AtCmdPoolDestructor);
    CmdRefMap = le_ref_CreateMap("CmdRefMap", CMD_POOL_SIZE);

    // Response pool allocation
    RspStringPool = le_mem_CreatePool("RspStringPool",sizeof(RspString_t));
    le_mem_ExpandPool(RspStringPool,RSP_POOL_SIZE);

    // Unsolicited pool allocation
    UnsolicitedPool = le_mem_CreatePool("AtUnsolicitedPool",sizeof(Unsolicited_t));
    le_mem_ExpandPool(UnsolicitedPool,UNSOLICITED_POOL_SIZE);
    le_mem_SetDestructor(UnsolicitedPool,UnsolicitedPoolDestructor);
    UnsolRefMap = le_ref_CreateMap("UnsolRefMap", UNSOLICITED_POOL_SIZE);

    // Add a handler to the close session service
    le_msg_AddServiceCloseHandler(
        le_atClient_GetServiceRef(), CloseSessionEventHandler, NULL);

    // Try to kick a couple of times before each timeout.
    le_clk_Time_t watchdogInterval = { .sec = MS_WDOG_INTERVAL };
    le_wdogChain_Init(1);
    le_wdogChain_MonitorEventLoop(0, watchdogInterval);
}
