/** @file le_atClient.c
 *
 * Implementation of AT commands client API.
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
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
#include <termios.h>

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
    char            line[LE_ATCLIENT_CMD_RSP_MAX_BYTES];  ///< string value
    le_dls_Link_t   link;                                 ///< link for list
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
    le_atClient_UnsolicitedResponseHandlerFunc_t handlerPtr;        ///< Unsolicited handler
    void*         contextPtr;                                       ///< User context
    char          unsolRsp[LE_ATCLIENT_UNSOLICITED_MAX_BYTES];      ///< pattern to match
    char          unsolBuffer[LE_ATCLIENT_UNSOLICITED_MAX_BYTES];   ///< Unsolicited buffer
    uint32_t      lineCount;                                        ///< Unsolicited lines number
    uint32_t      lineCounter;                                      ///< Received line counter
    bool          inProgress;                                       ///< Reception in progress
    le_atClient_UnsolicitedResponseHandlerRef_t ref;                ///< Unsolicited reference
    DeviceContextPtr_t interfacePtr;                                ///< device context
    le_dls_Link_t link;                                             ///< link in Unsolicited List
}
Unsolicited_t;

//--------------------------------------------------------------------------------------------------
/**
 * device structure
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct Device
{
    char               path[LE_ATCLIENT_PATH_MAX_BYTES]; ///< Path of the device
    uint32_t           handle;                           ///< Handle of the device.
    le_fdMonitor_Ref_t fdMonitor;                        ///< fd event monitor associated to Handle
}
Device_t;

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
    le_atClient_DeviceRef_t ref;        ///< reference of the deivce context
}
DeviceContext_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure of an AT Command.
 */
//--------------------------------------------------------------------------------------------------
typedef struct AtCmd
{
    char                   cmd[LE_ATCLIENT_CMD_MAX_BYTES];     ///< Command to send
    le_dls_List_t          ExpectintermediateResponseList;     ///< List of string pattern for
                                                               ///< intermediate response
    le_dls_List_t          expectResponseList;                 ///< List of string pattern for final
                                                               ///< response
    char                   text[LE_ATCLIENT_TEXT_MAX_BYTES+1]; ///< text to be sent after >
                                                               ///< +1 for ctrl-z
    size_t                 textSize;                           ///< size of text to send
    DeviceContext_t*       interfacePtr;                       ///< interface to send the command
    uint32_t               timeout;                            ///< command timeout (in ms)
    le_atClient_CmdRef_t   ref;                                ///< command reference
    le_dls_List_t          responseList;                       ///< Responses list
    uint32_t               intermediateIndex;                  ///< current index for intermediate
                                                               ///< reponses reading
    uint32_t               responsesCount;                     ///< responses count in responseList
    le_sem_Ref_t           endSem;                             ///< end treatment semaphore
    le_result_t            result;                             ///< result operation
    le_dls_Link_t          link;                               ///< link in AT commands list
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
                (stringSize < LE_ATCLIENT_UNSOLICITED_MAX_LEN-strlen(unsolPtr->unsolBuffer)) ?
                stringSize :
                LE_ATCLIENT_UNSOLICITED_MAX_LEN-strlen(unsolPtr->unsolBuffer);

            strncpy(unsolPtr->unsolBuffer+strlen(unsolPtr->unsolBuffer), unsolRspPtr, len);

            unsolPtr->inProgress = true;
        }

        if (unsolPtr->inProgress)
        {
            if ( (unsolPtr->lineCount - unsolPtr->lineCounter) == 1 )
            {
                unsolPtr->handlerPtr(unsolPtr->unsolBuffer, unsolPtr->contextPtr );
                memset(unsolPtr->unsolBuffer,0,LE_ATCLIENT_UNSOLICITED_MAX_BYTES);
                unsolPtr->lineCounter = 0;
                unsolPtr->inProgress = false;
            }
            else
            {
                if (LE_ATCLIENT_UNSOLICITED_MAX_LEN - strlen(unsolPtr->unsolBuffer) >= 2)
                {
                    snprintf( unsolPtr->unsolBuffer+strlen(unsolPtr->unsolBuffer),
                    LE_ATCLIENT_UNSOLICITED_MAX_BYTES,
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
 * This function must be called to print a buffer byte by byte
 *
 */
//--------------------------------------------------------------------------------------------------
static void PrintBuffer
(
    char     *namePtr,       ///< buffer name
    uint8_t  *bufferPtr,     ///< the buffer to print
    uint32_t  bufferSize     ///< Number of element to print
)
{
    uint32_t i;
    for(i=0;i<bufferSize;i++)
    {
        if (bufferPtr[i] == '\r' )
        {
            LE_DEBUG("'%s' -> [%d] '0x%.2x' '%s'",(namePtr)?namePtr:"no name",i,bufferPtr[i],"CR");
        }
        else if (bufferPtr[i] == '\n')
        {
            LE_DEBUG("'%s' -> [%d] '0x%.2x' '%s'",(namePtr)?namePtr:"no name",i,bufferPtr[i],"LF");
        }
        else if (bufferPtr[i] == 0x1A)
        {
            LE_DEBUG("'%s' -> [%d] '0x%.2x' '%s'",(namePtr)?namePtr:"no name",i,bufferPtr[i],"CTRL+Z");
        }
        else
        {
            LE_DEBUG("'%s' -> [%d] '0x%.2x' '%c'",(namePtr)?namePtr:"no name",i,bufferPtr[i],
                                                                                bufferPtr[i]);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called when we want to read on device (or port)
 *
 * @return byte number read
 */
//--------------------------------------------------------------------------------------------------
static int32_t DeviceRead
(
    Device_t  *devicePtr,    ///< device pointer
    uint8_t   *rxDataPtr,    ///< Buffer where to read
    uint32_t   size          ///< size of buffer
)
{
    int32_t status=1;
    int32_t amount = 0;

    while (status>0)
    {
        status = read(devicePtr->handle, rxDataPtr, size);

        if(status > 0)
        {
            size      -= status;
            rxDataPtr += status;
            amount    += status;
        }
    }

    LE_DEBUG("%s -> Read (%d) on %d",
             devicePtr->path,
             amount,devicePtr->handle);

    return amount;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to write on device (or port)
 *
 */
//--------------------------------------------------------------------------------------------------
static void  DeviceWrite
(
    Device_t *devicePtr,    ///< device pointer
    uint8_t  *rxDataPtr,    ///< Buffer to write
    uint32_t  bufLen        ///< size of buffer
)
{
    int32_t amount = 0;

    size_t size;
    size_t sizeToWrite;
    ssize_t sizeWritten;

    LE_FATAL_IF(devicePtr->handle==-1,"Write Handle error\n");

    for(size = 0; size < bufLen;)
    {
        sizeToWrite = bufLen - size;

        sizeWritten = write(devicePtr->handle, &rxDataPtr[size], sizeToWrite);

        if (sizeWritten < 0)
        {
            if ((errno != EINTR) && (errno != EAGAIN))
            {
                LE_ERROR("Cannot write on uart: %s",strerror(errno));
                return;
            }
        }
        else
        {
            size += sizeWritten;
        }
    }

    if(size > 0)
    {
        bufLen  -= size;
        amount  += size;
    }

    LE_DEBUG("%s -> write (%d) on %d",
             devicePtr->path,
             amount,devicePtr->handle);

    PrintBuffer(devicePtr->path,rxDataPtr,amount);
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

    int32_t size = 0;
    DeviceContext_t *interfacePtr = le_fdMonitor_GetContextPtr();

    LE_DEBUG("Start read");

    /* Read RX data on uart */
    size = DeviceRead(&interfacePtr->device,
                         (uint8_t *)(&interfacePtr->rxParser.rxData.buffer) +
                         interfacePtr->rxParser.rxData.idx,
                         (PARSER_BUFFER_MAX_BYTES - interfacePtr->rxParser.rxData.idx));

    /* Start the parsing only if we have read some bytes */
    if (size > 0)
    {
        interfacePtr->rxParser.rxData.endBuffer += size;

        PrintBuffer(interfacePtr->device.path,
                    interfacePtr->rxParser.rxData.buffer,
                    interfacePtr->rxParser.rxData.endBuffer);

        /* Call the parser */
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

    LE_DEBUG("Destroy thread for interface %s", interfacePtr->device.path);

    while ((linkPtr=le_dls_Peek(&interfacePtr->unsolicitedList)) != NULL)
    {
        Unsolicited_t *unsolPtr = CONTAINER_OF(linkPtr, Unsolicited_t, link);
        le_mem_Release(unsolPtr);
    }

    while ((linkPtr=le_dls_Peek(&interfacePtr->atCommandList)) != NULL)
    {
        AtCmd_t* atCmdPtr = CONTAINER_OF(linkPtr, AtCmd_t, link);
        le_mem_Release(atCmdPtr);
    }

    if (interfacePtr->device.fdMonitor)
    {
        le_fdMonitor_Delete(interfacePtr->device.fdMonitor);
    }

    if (interfacePtr->timerRef)
    {
        le_timer_Delete(interfacePtr->timerRef);
    }

    if (interfacePtr->waitingSemaphore)
    {
        le_sem_Delete(interfacePtr->waitingSemaphore);
    }

    if (interfacePtr->device.handle)
    {
        close(interfacePtr->device.handle);
    }

    le_ref_DeleteRef(DevicesRefMap, interfacePtr->ref);
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
    DeviceContext_t* newInterfacePtr = context;

    le_sem_Post(newInterfacePtr->waitingSemaphore);

    LE_DEBUG("Start thread for %s ", newInterfacePtr->device.path);

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
 * This function is used to check if the line match any of final string of the command
 *
 */
//--------------------------------------------------------------------------------------------------
static bool CheckResponse
(
    char*          receivedRspPtr,
    size_t         lineSize,
    le_dls_List_t* responseListPtr,
    le_dls_List_t* resultListPtr
)
{
    bool result = false;
    LE_DEBUG("Start checking response");

    if (lineSize == 0)
    {
        return false;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(responseListPtr);
    /* Browse all the queue while the string is not found */
    while (linkPtr != NULL)
    {
        RspString_t *currStringPtr = CONTAINER_OF(linkPtr,
                                                 RspString_t,
                                                 link);

        if ((strlen(currStringPtr->line) == 0) ||
           ((lineSize >= strlen(currStringPtr->line)) &&
           (strncmp(currStringPtr->line,receivedRspPtr,strlen(currStringPtr->line)) == 0)))
        {
            LE_DEBUG("rsp matched, size = %d", (int) lineSize);

            RspString_t* newStringPtr = le_mem_ForceAlloc(RspStringPool);
            memset(newStringPtr,0,sizeof(RspString_t));

            if(lineSize>LE_ATCLIENT_CMD_RSP_MAX_BYTES)
            {
                LE_ERROR("string too long");
                return false;
            }

            strncpy(newStringPtr->line,receivedRspPtr,lineSize);

            newStringPtr->link = LE_DLS_LINK_INIT;

            le_dls_Queue(resultListPtr,&(newStringPtr->link));

            return true;
        }

        linkPtr = le_dls_PeekNext(responseListPtr, linkPtr);
    }

    return false;
    LE_DEBUG("Stop checking response");
    return result;
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
            DeviceWrite(&(interfacePtr->device),
                           (uint8_t*) cmdPtr->text,
                           cmdPtr->textSize);

            // Send Ctrl-z
            uint8_t ctrlZ = 0x1A;
            DeviceWrite(&(interfacePtr->device),
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
                                    &(cmdPtr->expectResponseList), &(cmdPtr->responseList)))
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
                                    &(cmdPtr->ExpectintermediateResponseList),
                                    &(cmdPtr->responseList));
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

            DeviceWrite(&(interfacePtr->device),
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
 * This function is to start an AT command client session on a specified device
 *
 */
//--------------------------------------------------------------------------------------------------
static void StartClient
(
    void *param1Ptr,
    void *param2Ptr
)
{
    char monitorName[64];
    le_fdMonitor_Ref_t fdMonitorRef;
    DeviceContext_t *interfacePtr = param1Ptr;

    if (interfacePtr->device.fdMonitor)
    {
        LE_WARN("Interface %s already started",interfacePtr->device.path);
        le_sem_Post(interfacePtr->waitingSemaphore);
        return;
    }

    InitializeState(interfacePtr);

    interfacePtr->device.handle = open(interfacePtr->device.path, O_RDWR | O_NOCTTY | O_NONBLOCK);

    LE_FATAL_IF(interfacePtr->device.handle==-1,"Open device failed");

    uint32_t fd = interfacePtr->device.handle;

    struct termios term;
    bzero(&term, sizeof(term));

     // Default config
    tcgetattr(fd, &term);

    cfmakeraw(&term);
    term.c_oflag &= ~OCRNL;
    term.c_oflag &= ~ONLCR;
    term.c_oflag &= ~OPOST;
    tcsetattr(fd, TCSANOW, &term);
    tcflush(fd, TCIOFLUSH);

    // Create a File Descriptor Monitor object for the serial port's file descriptor.
    snprintf(monitorName,
             sizeof(monitorName),
             "Monitor-%d",
             interfacePtr->device.handle);

    fdMonitorRef = le_fdMonitor_Create(monitorName,
                                       interfacePtr->device.handle,
                                       RxNewData,
                                       POLLIN);

    interfacePtr->device.fdMonitor = fdMonitorRef;

    le_fdMonitor_SetContextPtr(fdMonitorRef, interfacePtr);

    if (le_log_GetFilterLevel() == LE_LOG_DEBUG)
    {
        char threadName[25];
        le_thread_GetName(le_thread_GetCurrent(), threadName, 25);
        LE_DEBUG("Resume %s with handle(%d)(%p) [%s]",
                 threadName,
                 interfacePtr->device.handle,
                 interfacePtr->device.fdMonitor,
                 monitorName
                );
    }

    le_sem_Post(interfacePtr->waitingSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is to stop an AT command client session on a specified device
 *
 */
//--------------------------------------------------------------------------------------------------
void StopClient
(
    void *param1Ptr,
    void *param2Ptr
)
{
    le_thread_Exit(NULL);
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

    le_event_QueueFunctionToThread(interfacePtr->threadRef,
                                   StopClient,
                                   (void*) interfacePtr,
                                   (void*) NULL);

    le_thread_Join(interfacePtr->threadRef,NULL);

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

    LE_DEBUG("Destroy unsolicited %s", unsolicitedPtr->unsolRsp);

    le_dls_Remove(&unsolicitedPtr->interfacePtr->unsolicitedList, &unsolicitedPtr->link);

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
    cmdPtr->timeout                         = LE_ATCLIENT_CMD_DEFAULT_TIMEOUT;
    cmdPtr->interfacePtr                    = NULL;
    cmdPtr->ref                             = le_ref_CreateRef(CmdRefMap, cmdPtr);
    cmdPtr->intermediateIndex               = 0;
    cmdPtr->responseList                    = LE_DLS_LIST_INIT;
    cmdPtr->link                            = LE_DLS_LINK_INIT;

    return cmdPtr->ref;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the devide where the AT command will be sent.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_OK when function succeed
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
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_FAULT;
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
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when the reference is invalid
 *      - LE_OK when function succeed
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
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_FAULT;
    }

    le_ref_DeleteRef(CmdRefMap, cmdRef);

    le_mem_Release(cmdPtr);

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the AT command string to be sent.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when the reference is invalid
 *      - LE_OK when function succeed
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
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_NOT_FOUND;
    }

    strncpy(cmdPtr->cmd, commandPtr, sizeof(cmdPtr->cmd));
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
 *      - LE_NOT_FOUND when the reference is invalid
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetIntermediateResponse
(
    le_atClient_CmdRef_t cmdRef,
        ///< [IN] AT Command

    const char* intermediatePtr
        ///< [IN] Set InterLE_NOT_FOUNDmediate
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_NOT_FOUND;
    }

    char *interPtr = strdup(intermediatePtr);

    if (strcmp(interPtr, "\0") != 0)
    {
        char *savePtr;
        interPtr = strtok_r(interPtr,"|", &savePtr);

        while(interPtr != NULL)
        {
            RspString_t* newStringPtr = le_mem_ForceAlloc(RspStringPool);
            memset(newStringPtr,0,sizeof(RspString_t));

            if(strlen(interPtr)>LE_ATCLIENT_CMD_RSP_MAX_BYTES)
            {
                LE_DEBUG("%s is too long (%zd): Max size %d",interPtr,strlen(interPtr),
                         LE_ATCLIENT_CMD_RSP_MAX_BYTES);
                return LE_FAULT;
            }

            strncpy(newStringPtr->line,interPtr,LE_ATCLIENT_CMD_RSP_MAX_BYTES);

            newStringPtr->link = LE_DLS_LINK_INIT;

            le_dls_Queue(&(cmdPtr->ExpectintermediateResponseList),&(newStringPtr->link));

            interPtr = strtok_r(NULL, "|", &savePtr);
        }
    }
    else
    {
        RspString_t* newStringPtr = le_mem_ForceAlloc(RspStringPool);
        memset(newStringPtr,0,sizeof(RspString_t));
        newStringPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(&(cmdPtr->ExpectintermediateResponseList),&(newStringPtr->link));

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
 *      - LE_NOT_FOUND when the reference is invalid
 *      - LE_OK when function succeed
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
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_NOT_FOUND;
    }

    char *respPtr = strdup(responsePtr);
    if (strcmp(respPtr, "\0") != 0)
    {
        char *savePtr;
        respPtr = strtok_r(respPtr,"|", &savePtr);

        while(respPtr != NULL)
        {
            RspString_t* newStringPtr = le_mem_ForceAlloc(RspStringPool);
            memset(newStringPtr,0,sizeof(RspString_t));

            if(strlen(respPtr)>LE_ATCLIENT_CMD_RSP_MAX_BYTES)
            {
                LE_DEBUG("%s is too long (%zd): Max size %d",respPtr,strlen(respPtr),
                            LE_ATCLIENT_CMD_RSP_MAX_BYTES);
                return LE_FAULT;
            }

            strncpy(newStringPtr->line,respPtr,LE_ATCLIENT_CMD_RSP_MAX_BYTES);

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
 *      - LE_NOT_FOUND when the reference is invalid
 *      - LE_OK when function succeed
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
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_NOT_FOUND;
    }

    if (textPtr)
    {
        if(strlen(textPtr)>LE_ATCLIENT_TEXT_MAX_LEN)
        {
            LE_ERROR("Text is too long! (%zd>%d)",strlen(textPtr),LE_ATCLIENT_TEXT_MAX_LEN);
            return LE_FAULT;
        }

        memcpy(cmdPtr->text, textPtr, strlen(textPtr));
        cmdPtr->textSize = strlen(textPtr);

        return LE_OK;
    }
    else
    {
        LE_DEBUG("No data to set");
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the timeout of the AT command execution.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when the reference is invalid
 *      - LE_OK when function succeed
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
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_NOT_FOUND;
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
 *      - LE_NOT_FOUND when the reference is invalid
 *      - LE_TIMEOUT when a timeout occur
 *      - LE_OK when function succeed
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
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_NOT_FOUND;
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
 * This function is used to get the first intermediate response
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when the reference is invalid
 *      - LE_OK when function succeed
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
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_FAULT;
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
        else
        {
            return LE_FAULT;
        }
    }

    return LE_NOT_FOUND;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the next intermediate responses
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when the reference is invalid
 *      - LE_OK when function succeed
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
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_FAULT;
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
        else
        {
            return LE_FAULT;
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
 *      - LE_NOT_FOUND when the reference is invalid
 *      - LE_OK when function succeed
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
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_FAULT;
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
 *      - LE_NOT_FOUND when the reference is invalid
 *      - LE_TIMEOUT when a timeout occur
 *      - LE_OK when function succeed
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
    le_result_t res = LE_FAULT;

    if (commandPtr == NULL)
    {
        LE_KILL_CLIENT("commandPtr is NULL !");
        return res;
    }

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
    strncpy(unsolicitedPtr->unsolRsp, unsolRsp, strlen(unsolRsp));
    unsolicitedPtr->lineCount    = lineCount;
    unsolicitedPtr->handlerPtr = handlerPtr;
    unsolicitedPtr->contextPtr = contextPtr;
    unsolicitedPtr->inProgress = false;
    unsolicitedPtr->ref = le_ref_CreateRef(UnsolRefMap, unsolicitedPtr);
    unsolicitedPtr->interfacePtr = interfacePtr;
    unsolicitedPtr->link = LE_DLS_LINK_INIT;

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

    if (unsolicitedPtr == NULL)
    {
        LE_ERROR("Invalid reference");
        return ;
    }

    le_event_QueueFunctionToThread(unsolicitedPtr->interfacePtr->threadRef,
                                   RemoveUnsolicited,
                                   (void*) unsolicitedPtr,
                                   (void*) NULL);
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
    const char* devicePathPtr
)
{
    char name[THREAD_NAME_MAX_LENGTH];
    static uint32_t threatCounter = 1;

    // Search if the device is already opened
    le_ref_IterRef_t iterRef = le_ref_GetIterator(DevicesRefMap);

     while (le_ref_NextNode(iterRef) == LE_OK)
     {
        DeviceContext_t* interfacePtr = (DeviceContext_t*) le_ref_GetValue(iterRef);

        if (le_path_IsEquivalent(devicePathPtr, interfacePtr->device.path, "/") == true)
        {
            le_mem_AddRef(interfacePtr);
            return interfacePtr->ref;
        }
    }

    DeviceContext_t* newInterfacePtr = le_mem_ForceAlloc(DevicesPool);

    memset(newInterfacePtr,0,sizeof(DeviceContext_t));

    le_utf8_Copy(newInterfacePtr->device.path,devicePathPtr,LE_ATCLIENT_PATH_MAX_BYTES,0);

    LE_DEBUG("Create a new interface for '%s'", devicePathPtr);

    snprintf(name,THREAD_NAME_MAX_LENGTH,"atCommandClient-%d",threatCounter);
    newInterfacePtr->threadRef = le_thread_Create(name,DeviceThread,newInterfacePtr);

    memset(name,0,THREAD_NAME_MAX_LENGTH);
    snprintf(name,THREAD_NAME_MAX_LENGTH,"ItfWaitSemaphore-%d",threatCounter);
    newInterfacePtr->waitingSemaphore = le_sem_Create(name,0);

    threatCounter++;

    le_thread_AddChildDestructor(newInterfacePtr->threadRef,
                                 DestroyDeviceThread,
                                 newInterfacePtr);

    le_thread_SetJoinable(newInterfacePtr->threadRef);

    le_thread_Start(newInterfacePtr->threadRef);
    le_sem_Wait(newInterfacePtr->waitingSemaphore);

    le_event_QueueFunctionToThread(newInterfacePtr->threadRef,
                                                StartClient,
                                                (void*) newInterfacePtr,
                                                (void*) NULL);

    le_sem_Wait(newInterfacePtr->waitingSemaphore);

    if (newInterfacePtr != NULL)
    {
        newInterfacePtr->ref = le_ref_CreateRef(DevicesRefMap, newInterfacePtr);
        return newInterfacePtr->ref;
    }
    else
    {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the ATClient session on the specified device.
 *
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
    DevicesPool = le_mem_CreatePool("DevicesPool",sizeof(DeviceContext_t));
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
}
