/** @file le_dev.c
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "le_dev.h"

#include <string.h>
#include <termios.h>

#define     ONE_MSEC                            1000
#define     THREAD_NAME_MAX                     64
#define     LE_UART_WRITE_MAX_SZ                64
#define     DEFAULT_ATUNSOLICITED_POOL_SIZE     1
#define     DEFAULT_ATSTRING_POOL_SIZE          1

#ifndef     UINT32_MAX
#define     UINT32_MAX                          ((uint32_t)-1)
#endif

/*
 * atCommandClient state machine
 *
 * @verbatim
 *
 *        EVENT_MANAGER_SENDCMD
 *          & Command NULL                            EVENT_MANAGER_SENDDATA
 *            -----------                                 ------------
 *           |           |                               |            |
 *           \/          |     EVENT_MANAGER_SENDCMD     |            \/
 *    --------------    -       & Command not NULL        -    ----------------
 *   |              |   ---------------------------------->   |                |
 *   | WaitingState |                                         |  SendingState  |
 *   |              |   <----------------------------------   |                |
 *    --------------    -    EVENT_MANAGER_PROCESSLINE    -    ----------------
 *           /\          |     & Final pattern match     |            /\
 *           |           |                               |            |
 *            -----------                                 ------------
 *      EVENT_MANAGER_PROCESSLINE                    EVENT_MANAGER_PROCESSLINE
 *                                                      & Final pattern not match
 *
 * @endverbatim
 *
 *
 *
 *
 *
 * ATParser state machine
 *
 * @verbatim
 *
 *    ---------------                                           ---------------------
 *   |               |           EVENT_PARSER_CHAR             |                     |
 *   | StartingState |   ---------------------------------->   |  InitializingState  |
 *   |               |                                         |                     |
 *    ---------------                                           ---------------------
 *          |                                                            |
 *          |                                                            |
 *          |                                                            |
 *          |                     -----------------    EVENT_PARSER_CRLF |
 *          |                    |                 | <-------------------
 *           ---------------->   | ProcessingState | -----------------------
 *           EVENT_PARSER_CRLF   |                 | --------------------   |
 *                                -----------------                      |  |
 *                                    /\       /\      EVENT_PARSER_CRLF |  |
 *                                    |        |                         |  |
 *                                    |         -------------------------   |
 *                                     -------------------------------------
 *                                             EVENT_PARSER_PROMPT
 *
 * @verbatim
 *
 */
static void WaitingState(le_atClient_ManagerStateMachineRef_t smRef,EIndicationATCommandClient input);
static void SendingState(le_atClient_ManagerStateMachineRef_t smRef,EIndicationATCommandClient input);

static le_mem_PoolRef_t    AtCommandClientItfPool;
static le_mem_PoolRef_t    AtStringPool;

static void StartingState      (le_atClient_ParserStateMachineRef_t smRef,EIndicationATParser input);
static void InitializingState  (le_atClient_ParserStateMachineRef_t smRef,EIndicationATParser input);
static void ProcessingState    (le_atClient_ParserStateMachineRef_t smRef,EIndicationATParser input);

static void SendLine(le_atClient_ParserStateMachineRef_t smRef);
static void SendData(le_atClient_ParserStateMachineRef_t smRef);

static le_mem_PoolRef_t    AtUnsolicitedPool;

//--------------------------------------------------------------------------------------------------
/**
 * This struct is used the send a line when an unsolicited pattern matched.
 *
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char            line[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];
}
le_atClient_MgrUnsolResponse_t;

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the atCommandClient FSM
 *
 */
//--------------------------------------------------------------------------------------------------
static void InitializeState
(
    le_atClient_ManagerStateMachineRef_t  smRef
)
{
    le_dev_InitializeState(&(smRef->curContext.atParser));
    smRef->curContext.atUnsolicitedList = LE_DLS_LIST_INIT;
    smRef->curContext.atCommandList = LE_DLS_LIST_INIT;
    smRef->curContext.atCommandTimer = le_timer_Create("AtCommandClientTimer");
    smRef->curContext.atParser.atCommandClientPtr = smRef;
    smRef->curState = WaitingState;
}

static void le_uart_defaultConfig(int fd)
{
    struct termios term;
    bzero(&term, sizeof(term));
    cfmakeraw(&term);
    term.c_cflag |= CREAD;

    // Default config
    tcgetattr(fd, &term);
    term.c_cflag &= ~PARENB;
    term.c_cflag &= ~CRTSCTS;
    term.c_iflag &= ~(IXON | IXOFF);
    term.c_cflag |= CS8;
    term.c_cflag &= ~CSTOPB;
    cfsetspeed(&term, B115200);

    term.c_iflag &= ~ICRNL;
    term.c_iflag &= ~INLCR;
    term.c_iflag |= IGNBRK;

    term.c_oflag &= ~OCRNL;
    term.c_oflag &= ~ONLCR;
    term.c_oflag &= ~OPOST;

    term.c_lflag &= ~ICANON;
    term.c_lflag &= ~ISIG;
    term.c_lflag &= ~IEXTEN;
    term.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL|ECHOCTL|ECHOPRT|ECHOKE);

    tcsetattr(fd, TCSANOW, &term);


    tcflush(fd, TCIOFLUSH);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to check if the extra data should be report
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckUnsolicitedExtraData
(
    le_atClient_ManagerStateMachineRef_t  smRef,
    char                                 *unsolicitedPtr,
    size_t                                unsolicitedSize
)
{
    // check unsolicited and report to all mailbox
    LE_DEBUG("Start checking unsolicited extra data");

    le_dls_Link_t* linkPtr = le_dls_Peek(&(smRef->curContext.atUnsolicitedList));
    /* Browse all the queue while the string is not found */
    while (linkPtr != NULL)
    {
        atUnsolicited_t *currUnsolicitedPtr = CONTAINER_OF(linkPtr,
                                                           atUnsolicited_t,
                                                           link);

        if (currUnsolicitedPtr->waitForExtraData)
        {
            le_atClient_MgrUnsolResponse_t atResp;

            LE_FATAL_IF((sizeof(atResp.line)<unsolicitedSize-1),
                        "unsolicited response buffer is too small! resize it");

            memcpy(atResp.line,unsolicitedPtr,unsolicitedSize);
            atResp.line[unsolicitedSize] = '\0';

            LE_DEBUG("Report unsolicited extra data line <%s> ",atResp.line );
            le_event_Report(currUnsolicitedPtr->unsolicitedReportId,
                            &atResp,
                            sizeof(atResp));

            currUnsolicitedPtr->waitForExtraData = false;
        }


        linkPtr = le_dls_PeekNext(&(smRef->curContext.atUnsolicitedList), linkPtr);
    }

    LE_DEBUG("Stop checking unsolicited extra data");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to check unsolicited list
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckUnsolicitedList
(
    le_atClient_ManagerStateMachineRef_t  smRef,
    char                                 *unsolicitedPtr,
    size_t                                unsolicitedSize
)
{
    // check unsolicited and report to all mailbox
    LE_DEBUG("Start checking unsolilicted list");

    le_dls_Link_t* linkPtr = le_dls_Peek(&(smRef->curContext.atUnsolicitedList));
    /* Browse all the queue while the string is not found */
    while (linkPtr != NULL)
    {
        atUnsolicited_t *currUnsolicitedPtr = CONTAINER_OF(linkPtr,
                                                           atUnsolicited_t,
                                                           link);
        size_t currUnsolicitedPtrSize = strlen(currUnsolicitedPtr->unsolRsp);

        if (strncmp(unsolicitedPtr,currUnsolicitedPtr->unsolRsp,currUnsolicitedPtrSize) == 0)
        {
            le_atClient_MgrUnsolResponse_t atResp;

            LE_FATAL_IF((sizeof(atResp.line)<unsolicitedSize-1),
                        "unsolicited response buffer is too small! resize it");

            memcpy(atResp.line,unsolicitedPtr,unsolicitedSize);
            atResp.line[unsolicitedSize] = '\0';

            LE_DEBUG("Report unsolicited line <%s> ",atResp.line );
            le_event_Report(currUnsolicitedPtr->unsolicitedReportId,
                            &atResp,
                            sizeof(atResp));

            currUnsolicitedPtr->waitForExtraData = currUnsolicitedPtr->withExtraData;
        }


        linkPtr = le_dls_PeekNext(&(smRef->curContext.atUnsolicitedList), linkPtr);
    }

    LE_DEBUG("Stop checking unsolicited list");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to call-back all thread that are register on unsolicited
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckUnsolicited
(
    le_atClient_ManagerStateMachineRef_t  smRef,
    char                                 *unsolicitedPtr,
    size_t                                unsolicitedSize
)
{
    LE_DEBUG("Start checking unsolicited");

    CheckUnsolicitedExtraData(smRef,unsolicitedPtr,unsolicitedSize);
    CheckUnsolicitedList(smRef,unsolicitedPtr,unsolicitedSize);

    LE_DEBUG("Stop checking unsolicited");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start the timer of a command
 *
 */
//--------------------------------------------------------------------------------------------------
static void StartTimer
(
    le_atClient_ManagerStateMachineRef_t smRef
)
{
    le_clk_Time_t timer;

    le_timer_SetHandler(smRef->curContext.atCommandTimer,
                        smRef->curContext.atCommandInProgressRef->timerHandler );
    le_timer_SetContextPtr(smRef->curContext.atCommandTimer,
                           smRef->curContext.atCommandInProgressRef);

    timer.sec  = smRef->curContext.atCommandInProgressRef->timer/1000;
    timer.usec = (smRef->curContext.atCommandInProgressRef->timer%1000)*ONE_MSEC;
    le_timer_SetInterval(smRef->curContext.atCommandTimer,timer);

    le_timer_Start(smRef->curContext.atCommandTimer);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to stop the timer of a command
 *
 */
//--------------------------------------------------------------------------------------------------
static void StopTimer
(
    le_atClient_ManagerStateMachineRef_t smRef
)
{
    le_timer_Stop(smRef->curContext.atCommandTimer);
    le_timer_SetHandler(smRef->curContext.atCommandTimer,NULL);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function called when there is something to read on fd
 *
 */
//--------------------------------------------------------------------------------------------------
static void RxNewData
(
    int fd, ///< File descriptor to read on
    short events ///< Event reported on fd (expect only POLLIN)
)
{
    if (events & ~POLLIN)
    {
        LE_CRIT("Unexpected event(s) on fd %d (0x%hX).", fd, events);
    }

    int32_t size = 0;
    le_atClient_ManagerStateMachine_t *atCommandClientRef = le_fdMonitor_GetContextPtr();
//     le_atClient_ParserStateMachineRef_t  atParserRef = &(atCommandClientRef->curContext.atParser) ;
    le_atClient_ParserStateMachine_t*  atParserRef = &(atCommandClientRef->curContext.atParser) ;

    LE_DEBUG("Start read");

    /* Read RX data on uart */
    size = le_dev_Read(&(atCommandClientRef->curContext.le_atClient_device),
                         (uint8_t *)(atParserRef->curContext.buffer + atParserRef->curContext.idx),
                         (ATFSMPARSER_BUFFER_MAX - atParserRef->curContext.idx));

    /* Start the parsing only if we have read some bytes */
    if (size > 0)
    {
        LE_DEBUG(">>> Read %d bytes (FillIndex=%d)", size,  atParserRef->curContext.idx);
        /* Increment AT command buffer index */
        atParserRef->curContext.endbuffer += size;
        LE_DEBUG("Increase Rx Buffer Index: FillIndex = %zd", atParserRef->curContext.endbuffer);

        le_dev_PrintBuffer(atCommandClientRef->curContext.le_atClient_device.name,
                             atParserRef->curContext.buffer,
                             atParserRef->curContext.endbuffer);

        /* Call the parser */
        le_dev_ReadBuffer(atParserRef);
        le_dev_ResetBuffer(atParserRef);

    }
    if (atParserRef->curContext.endbuffer > ATFSMPARSER_BUFFER_MAX)
    {
        LE_WARN("Rx Buffer Overflow (FillIndex = %d)!!!", atParserRef->curContext.idx);
    }

    LE_DEBUG("read finished");
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is the destructor for le_atClient_mgr_t struct
 *
 */
//--------------------------------------------------------------------------------------------------
static void le_atClient_mgrItfPoolDestruct
(
    void* context   ///< le_atClient_mgr_t reference
)
{
    struct le_atClient_mgr* newInterfacePtr = context;

    LE_DEBUG("Destruct %s device",newInterfacePtr->atCommandClient.curContext.le_atClient_device.name);

    LE_DEBUG("Destruct Done");
    // No API to delete an event_Id, so nothing to do.
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create and start the atCommandClient + ATParser for the device.
 *
 */
//--------------------------------------------------------------------------------------------------
static void *InitThread
(
    void* context
)
{
    le_event_HandlerRef_t   handlerRef;
    struct le_atClient_mgr* newInterfacePtr = context;

    handlerRef = le_event_AddHandler("hdl_resumeInterface",
                                     newInterfacePtr->resumeInterfaceId,
                                     le_dev_Resume);
    le_event_SetContextPtr(handlerRef,&(newInterfacePtr->atCommandClient));

    handlerRef = le_event_AddHandler("hdl_SuspendInterface",
                                     newInterfacePtr->suspendInterfaceId,
                                     le_dev_Suspend);
    le_event_SetContextPtr(handlerRef,&(newInterfacePtr->atCommandClient));

    handlerRef = le_event_AddHandler("hdl_SubscribeUnsol",
                                     newInterfacePtr->subscribeUnsolicitedId,
                                     le_dev_AddUnsolicited);
    le_event_SetContextPtr(handlerRef,&(newInterfacePtr->atCommandClient));

    handlerRef = le_event_AddHandler("hdl_UnSubscribeUnsol",
                                     newInterfacePtr->unSubscribeUnsolicitedId,
                                     le_dev_RemoveUnsolicited);
    le_event_SetContextPtr(handlerRef,&(newInterfacePtr->atCommandClient));

    handlerRef = le_event_AddHandler("hdl_SendCommand",
                                     newInterfacePtr->sendCommandId,
                                     le_dev_SendCommand);
    le_event_SetContextPtr(handlerRef,&(newInterfacePtr->atCommandClient));

    handlerRef = le_event_AddHandler("hdl_CancelCommand",
                                     newInterfacePtr->cancelCommandId,
                                     le_dev_CancelCommand);
    le_event_SetContextPtr(handlerRef,&(newInterfacePtr->atCommandClient));

    le_sem_Post(newInterfacePtr->waitingSemaphore);

    {
        char threadName[25];
        le_thread_GetName(le_thread_GetCurrent(),threadName,25);
        LE_DEBUG("Start thread %s %p",threadName,le_thread_GetCurrent());
    }

    le_event_RunLoop();

    return NULL; // Should not happen
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is used to do a transition between two state with one event.
 *
 */
//--------------------------------------------------------------------------------------------------
static void updateTransitionParser
(
    le_atClient_ParserStateMachineRef_t   smRef,
    EIndicationATParser         input,
    ATParserStateProc_func_t    newState
)
{
    smRef->prevState   = smRef->curState;
    smRef->curState    = newState;
    smRef->lastEvent   = input;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to do a transition between two state with one event.
 *
 */
//--------------------------------------------------------------------------------------------------
static void updateTransitionManager
(
    le_atClient_ManagerStateMachineRef_t   smRef,
    EIndicationATCommandClient         input,
    ATCommandClientStateProc_Func_t    newState
)
{
    smRef->prevState   = smRef->curState;
    smRef->curState    = newState;
    smRef->lastEvent   = input;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the atCommandClient FSM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WaitingState
(
    le_atClient_ManagerStateMachineRef_t smRef,
    EIndicationATCommandClient input
)
{
    switch (input)
    {
        case EVENT_MANAGER_SENDCMD:
        {
            // Send at command
            le_dls_Link_t* linkPtr = le_dls_Pop(&(smRef->curContext.atCommandList));

            if (linkPtr==NULL) {
                LE_DEBUG("No more command to execute");
                return;
            }

            smRef->curContext.atCommandInProgressRef = CONTAINER_OF(linkPtr,struct le_atClient_cmd,link);
            if (smRef->curContext.atCommandInProgressRef)
            {
                LE_DEBUG("Executing command(%d) '%s' from list",
                         smRef->curContext.atCommandInProgressRef->commandId,
                         smRef->curContext.atCommandInProgressRef->command);

                if (smRef->curContext.atCommandInProgressRef->timer>0)
                {
                    updateTransitionManager(smRef,input,SendingState);
                    StartTimer(smRef);
                }
                le_dev_Prepare(smRef->curContext.atCommandInProgressRef);

                le_dev_Write(&(smRef->curContext.le_atClient_device),
                               smRef->curContext.atCommandInProgressRef->command,
                               smRef->curContext.atCommandInProgressRef->commandSize);

                if (smRef->curContext.atCommandInProgressRef->timer==0)
                {
                    le_mem_Release(smRef->curContext.atCommandInProgressRef);
                    smRef->curContext.atCommandInProgressRef = NULL;
                }

                LE_DEBUG("There is still %zd waiting command",
                         le_dls_NumLinks(&(smRef->curContext.atCommandList)));
            }
            else
            {
                LE_WARN("Try to send a command that is not initialized");
            }

            break;
        }
        case EVENT_MANAGER_PROCESSLINE:
        {
            CheckUnsolicited(smRef,
                                       smRef->curContext.atLine,
                                       strlen(smRef->curContext.atLine));
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
 * This function is a state of the atCommandClient FSM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendingState
(
    le_atClient_ManagerStateMachineRef_t smRef,
    EIndicationATCommandClient input
)
{
    switch (input)
    {
        case EVENT_MANAGER_SENDDATA:
        {
            // Send data
            le_dev_Write(&(smRef->curContext.le_atClient_device),
                           smRef->curContext.atCommandInProgressRef->data,
                           smRef->curContext.atCommandInProgressRef->dataSize);
            break;
        }
        case EVENT_MANAGER_PROCESSLINE:
        {
            CheckUnsolicited(smRef,
                             smRef->curContext.atLine,
                             strlen(smRef->curContext.atLine));

            if (le_dev_CheckFinal(smRef->curContext.atCommandInProgressRef,
                smRef->curContext.atLine,
                strlen(smRef->curContext.atLine)) )
            {
                StopTimer(smRef);

                le_mem_Release(smRef->curContext.atCommandInProgressRef);
                smRef->curContext.atCommandInProgressRef = NULL;

                updateTransitionManager(smRef,input,WaitingState);
                // Send the next command
                (smRef->curState)(smRef,EVENT_MANAGER_SENDCMD);
                return;
            }

            le_dev_CheckIntermediate(smRef->curContext.atCommandInProgressRef,
                                               smRef->curContext.atLine,
                                               strlen(smRef->curContext.atLine));
            break;
        }
        case EVENT_MANAGER_CANCELCMD:
        {
            StopTimer(smRef);
            le_mem_Release(smRef->curContext.atCommandInProgressRef);
            smRef->curContext.atCommandInProgressRef = NULL;
            updateTransitionManager(smRef,input,WaitingState);
            // Send the next command
            (smRef->curState)(smRef,EVENT_MANAGER_SENDCMD);
            return;
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
 * This function is a state of the ATParser FSM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void StartingState
(
    le_atClient_ParserStateMachineRef_t smRef,
    EIndicationATParser input
)
{
    switch (input)
    {
        case EVENT_PARSER_CRLF:
            smRef->curContext.idx_lastCRLF = smRef->curContext.idx;
            updateTransitionParser(smRef,input,ProcessingState);
            break;
        case EVENT_PARSER_CHAR:
            updateTransitionParser(smRef,input,InitializingState);
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the ATParser FSM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void InitializingState
(
    le_atClient_ParserStateMachineRef_t smRef,
    EIndicationATParser input
)
{
    switch (input)
    {
        case EVENT_PARSER_CRLF:
            smRef->curContext.idx_lastCRLF = smRef->curContext.idx;
            updateTransitionParser(smRef,input,ProcessingState);
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the ATParser FSM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void ProcessingState
(
    le_atClient_ParserStateMachineRef_t smRef,
    EIndicationATParser input
)
{
    switch (input)
    {
        case EVENT_PARSER_CRLF:
            SendLine(smRef);
            updateTransitionParser(smRef,input,ProcessingState);
            break;
        case EVENT_PARSER_PROMPT:
            SendData(smRef);
            updateTransitionParser(smRef,input,ProcessingState);
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the ATParser FSM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendData
(
    le_atClient_ParserStateMachineRef_t smRef)
{
    LE_DEBUG("SEND DATA");
    (smRef->atCommandClientPtr->curState)(smRef->atCommandClientPtr,EVENT_MANAGER_SENDDATA);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to send the Line found between to CRLF (\\r\\n)
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendLine
(
    le_atClient_ParserStateMachineRef_t smRef
)
{
    int32_t newCRLF = smRef->curContext.idx-2;
    size_t lineSize = newCRLF - smRef->curContext.idx_lastCRLF;

    LE_DEBUG("%d [%d] ... [%d]",
                smRef->curContext.idx,
                smRef->curContext.idx_lastCRLF,
                newCRLF
    );

    if (lineSize > 0)
    {
        le_dev_ProcessLine(smRef->atCommandClientPtr,
                                     (char*)&(smRef->curContext.buffer[smRef->curContext.idx_lastCRLF]),
                                     lineSize);

    }
    smRef->curContext.idx_lastCRLF = smRef->curContext.idx;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the next event to send to the ATParser FSM
 *
 */
//--------------------------------------------------------------------------------------------------
static bool GetNextEvent(
    le_atClient_ParserStateMachineRef_t smRef,
    EIndicationATParser   *evPtr
)
{
    int32_t idx = smRef->curContext.idx++;
    if (idx < smRef->curContext.endbuffer)
    {
        if (smRef->curContext.buffer[idx] == '\r')
        {
            idx = smRef->curContext.idx++;
            if (idx < smRef->curContext.endbuffer)
            {
                if ( smRef->curContext.buffer[idx] == '\n')
                {
                    *evPtr = EVENT_PARSER_CRLF;
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                smRef->curContext.idx--;
                return false;
            }
        }
        else if (smRef->curContext.buffer[idx] == '\n')
        {
            if ( idx-1 > 0 )
            {
                if (smRef->curContext.buffer[idx-1] == '\r')
                {
                    *evPtr = EVENT_PARSER_CRLF;
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
        else if (smRef->curContext.buffer[idx] == '>')
        {
            *evPtr = EVENT_PARSER_PROMPT;
            return true;
        }
        else
        {
            *evPtr = EVENT_PARSER_CHAR;
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
 * This function is used to check if the line should be report
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckIntermediateExtraData
(
    struct le_atClient_cmd *atcommandPtr,
    char               *atLinePtr,
    size_t              atLineSize
)
{
    LE_DEBUG("Start checking intermediate extra data");

    if(atcommandPtr->waitExtra)
    {
        le_atClient_CmdResponse_t atResp;

        atResp.fromWhoRef = atcommandPtr;
        LE_FATAL_IF((sizeof(atResp.line))<atLineSize-1,"response buffer is too small! resize it");

        memcpy(atResp.line,atLinePtr,atLineSize);
        atResp.line[atLineSize] = '\0';

        atcommandPtr->waitExtra = false;
        LE_DEBUG("Report extra data line <%s> ",atResp.line );
        le_event_Report(atcommandPtr->intermediateId,&atResp,sizeof(atResp));
    }

    LE_DEBUG("Stop checking intermediate extra data");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to check if the line match any of intermediate/final string of the command
 *
 */
//--------------------------------------------------------------------------------------------------
static bool CheckList
(
    struct le_atClient_cmd  *atcommandPtr,
    char                *atLinePtr,
    size_t               atLineSize,
    bool                 isFinal
)
{
    const le_dls_List_t *listPtr;
    le_event_Id_t  reportId;

    if(isFinal)
    {
        listPtr  = &(atcommandPtr->finaleResp);
        reportId = atcommandPtr->finalId;
    }
    else
    {
        listPtr  = &(atcommandPtr->intermediateResp);
        reportId = atcommandPtr->intermediateId;
    }

    le_dls_Link_t* linkPtr = le_dls_Peek(listPtr);
    /* Browse all the queue while the string is not found */
    while (linkPtr != NULL)
    {
        le_atClient_machinestring_t *currStringPtr = CONTAINER_OF(linkPtr,
                                                 le_atClient_machinestring_t,
                                                 link);

        if (strncmp(atLinePtr,currStringPtr->line,strlen(currStringPtr->line)) == 0)
        {
            le_atClient_CmdResponse_t atResp;

            atResp.fromWhoRef = atcommandPtr;
            LE_FATAL_IF((sizeof(atResp.line))<atLineSize-1,"response buffer is too small! resize it");

            memcpy(atResp.line,atLinePtr,atLineSize);
            atResp.line[atLineSize] = '\0';

            LE_DEBUG("Report line <%s> ",atResp.line );
            le_event_Report(reportId,
                            &atResp,
                            sizeof(atResp));
            return true;
        }

        linkPtr = le_dls_PeekNext(listPtr, linkPtr);
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get a le_atClient_mgr_t from the Pool.
 *
 *  @return pointer to le_atClient_mgr_t
 */
//--------------------------------------------------------------------------------------------------
static struct le_atClient_mgr* CreateInterface
(
)
{
    struct le_atClient_mgr* newInterfacePtr = le_mem_ForceAlloc(AtCommandClientItfPool);

    memset(&(newInterfacePtr->atCommandClient),0,sizeof(newInterfacePtr->atCommandClient));

    newInterfacePtr->resumeInterfaceId = le_event_CreateId("id_resumeInterface",0);

    newInterfacePtr->suspendInterfaceId = le_event_CreateId("id_suspendInterface",0);

    newInterfacePtr->subscribeUnsolicitedId = le_event_CreateIdWithRefCounting("id_SubscribeUnsol");

    newInterfacePtr->unSubscribeUnsolicitedId =
                                            le_event_CreateIdWithRefCounting("id_UnSubscribeUnsol");

    newInterfacePtr->sendCommandId = le_event_CreateIdWithRefCounting("id_SendCommand");

    newInterfacePtr->cancelCommandId = le_event_CreateIdWithRefCounting("id_CancelCommand");

    newInterfacePtr->waitingSemaphore = le_sem_Create("ItfWaitSemaphore",0);

    return newInterfacePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is used to initialize the atstring, the atcommandclientitf and the atunsolicited
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_Init
(
    void
)
{
    AtStringPool = le_mem_CreatePool("AtStringPool",sizeof(le_atClient_machinestring_t));
    le_mem_ExpandPool(AtStringPool,DEFAULT_ATSTRING_POOL_SIZE);

    AtCommandClientItfPool = le_mem_CreatePool("atcommandclientitfPool",sizeof(struct le_atClient_mgr));
    le_mem_SetDestructor(AtCommandClientItfPool,le_atClient_mgrItfPoolDestruct);

    AtUnsolicitedPool = le_mem_CreatePool("AtUnsolicitedPool",sizeof(atUnsolicited_t));
    le_mem_ExpandPool(AtUnsolicitedPool,DEFAULT_ATUNSOLICITED_POOL_SIZE);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to add new string into a le_dls_List_t.
 *
 * @note
 * pList must be finished by a NULL.
 *
 * @return nothing
 */
//--------------------------------------------------------------------------------------------------
void le_dev_AddInList
(
    le_dls_List_t *list,          ///< List of le_atClient_machinestring_t
    const char   **patternListPtr ///< List of pattern
)
{
    uint32_t i = 0;

    if (!patternListPtr)
    {
        return;
    }

    while(patternListPtr[i] != NULL)
    {
        le_atClient_machinestring_t* newStringPtr = le_mem_ForceAlloc(AtStringPool);

        LE_FATAL_IF(
            (strlen(patternListPtr[i])>LE_ATCLIENT_CMD_SIZE_MAX_BYTES),
                    "%s is too long (%zd): Max size %d",
                    patternListPtr[i],strlen(patternListPtr[i]),LE_ATCLIENT_CMD_SIZE_MAX_BYTES);

        strncpy(newStringPtr->line,patternListPtr[i],LE_ATCLIENT_CMD_SIZE_MAX_BYTES);
        newStringPtr->line[LE_ATCLIENT_CMD_SIZE_MAX_BYTES-1]='\0';

        newStringPtr->link = LE_DLS_LINK_INIT;
        le_dls_Queue(list,&(newStringPtr->link));
        i++;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to release all string list.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_ReleaseFromList
(
    le_dls_List_t *pList
)
{
    le_dls_Link_t* linkPtr;

    while ((linkPtr=le_dls_Pop(pList)) != NULL)
    {
        le_atClient_machinestring_t * currentPtr = CONTAINER_OF(linkPtr,le_atClient_machinestring_t,link);
        le_mem_Release(currentPtr);
    }
    LE_DEBUG("All string has been released");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to save the line to process, and execute the atCommandClient FSM
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_ProcessLine
(
    le_atClient_ManagerStateMachineRef_t smRef,
    char *  linePtr,
    size_t  lineSize
)
{
    LE_FATAL_IF((lineSize>ATPARSER_LINE_MAX-1),"ATLine is too small, need to increase the size");

    memcpy(smRef->curContext.atLine,linePtr,lineSize);
    smRef->curContext.atLine[lineSize] = '\0';

    LE_DEBUG("Processing line '%s'",smRef->curContext.atLine);

    (smRef->curState)(smRef,EVENT_MANAGER_PROCESSLINE);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is to resume the current atCommandClient
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_Resume
(
    void *report    ///< Callback parameter
)
{
    char monitorName[64];
    le_fdMonitor_Ref_t fdMonitorRef;
    struct le_atClient_mgr *interfacePtr = le_event_GetContextPtr();

    if (interfacePtr->atCommandClient.curContext.le_atClient_device.fdMonitor) {
        LE_WARN("Interface %s already started",interfacePtr->atCommandClient.curContext.le_atClient_device.name);
        le_sem_Post(interfacePtr->waitingSemaphore);
        return;
    }

    InitializeState(&(interfacePtr->atCommandClient));

    interfacePtr->atCommandClient.curContext.le_atClient_device.handle =
                    open(interfacePtr->atCommandClient.curContext.le_atClient_device.path, O_RDWR | O_NOCTTY | O_NONBLOCK);

    LE_FATAL_IF(interfacePtr->atCommandClient.curContext.le_atClient_device.handle==-1,"Open device failed");

    le_uart_defaultConfig(interfacePtr->atCommandClient.curContext.le_atClient_device.handle);

    // Create a File Descriptor Monitor object for the serial port's file descriptor.
    snprintf(monitorName,
             sizeof(monitorName),
             "%s-Monitor",
             interfacePtr->atCommandClient.curContext.le_atClient_device.name);

    fdMonitorRef = le_fdMonitor_Create(monitorName,
                                       interfacePtr->atCommandClient.curContext.le_atClient_device.handle,
                                       RxNewData,
                                       POLLIN);

    interfacePtr->atCommandClient.curContext.le_atClient_device.fdMonitor = fdMonitorRef;

    le_fdMonitor_SetContextPtr(fdMonitorRef, &(interfacePtr->atCommandClient));

    if (le_log_GetFilterLevel() == LE_LOG_DEBUG)
    {
        char threadName[25];
        le_thread_GetName(le_thread_GetCurrent(), threadName, 25);
        LE_DEBUG("Resume %s with handle(%d)(%p) [%s]",
                 threadName,
                 interfacePtr->atCommandClient.curContext.le_atClient_device.handle,
                 interfacePtr->atCommandClient.curContext.le_atClient_device.fdMonitor,
                 monitorName
                );
    }

    le_sem_Post(interfacePtr->waitingSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is to stop the current atCommandClient
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_Suspend
(
    void *report    ///< Callback parameter
)
{
    struct le_atClient_mgr *interfacePtr = le_event_GetContextPtr();

    if (interfacePtr->atCommandClient.curContext.le_atClient_device.fdMonitor==NULL)
    {
        LE_WARN("Interface %s already stoped",interfacePtr->atCommandClient.curContext.le_atClient_device.name);
        le_sem_Post(interfacePtr->waitingSemaphore);
        return;
    }

    {
        char threadName[25];
        le_thread_GetName(le_thread_GetCurrent(),threadName,25);
        LE_DEBUG("Suspend %s with handle(%d)(%p)",
                 threadName,
                 interfacePtr->atCommandClient.curContext.le_atClient_device.handle,
                 interfacePtr->atCommandClient.curContext.le_atClient_device.fdMonitor
        );
    }

    le_timer_SetHandler(interfacePtr->atCommandClient.curContext.atCommandTimer,NULL);
    le_timer_Delete(interfacePtr->atCommandClient.curContext.atCommandTimer);

    le_fdMonitor_Delete(interfacePtr->atCommandClient.curContext.le_atClient_device.fdMonitor);
    close(interfacePtr->atCommandClient.curContext.le_atClient_device.handle);

    interfacePtr->atCommandClient.curContext.le_atClient_device.fdMonitor = NULL;
    interfacePtr->atCommandClient.curContext.le_atClient_device.handle = 0;

    le_sem_Post(interfacePtr->waitingSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is to add unsolicited string into current atCommandClient
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_AddUnsolicited
(
    void *report    ///< Callback parameter
)
{
    le_atClient_ManagerStateMachine_t *atCommandClientRef = le_event_GetContextPtr();
    atUnsolicited_t* unsolicitedPtr = report;

    LE_DEBUG("Unsolicited ADD %p <%s>",
             unsolicitedPtr->unsolicitedReportId,
             unsolicitedPtr->unsolRsp);

    unsolicitedPtr->link = LE_DLS_LINK_INIT;
    le_mem_AddRef(unsolicitedPtr);
    le_dls_Queue(&(atCommandClientRef->curContext.atUnsolicitedList),&(unsolicitedPtr->link));

    le_mem_Release(report);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is to remove unsolicited string from current atCommandClient
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_RemoveUnsolicited
(
    void *report    ///< Callback parameter
)
{
    le_atClient_ManagerStateMachine_t *atCommandClientRef = le_event_GetContextPtr();
    atUnsolicited_t* unsolicitedPtr = report;

    LE_DEBUG("Unsolicited DEL %p <%s>",
             unsolicitedPtr->unsolicitedReportId,
             unsolicitedPtr->unsolRsp);

    le_dls_Link_t* linkPtr = le_dls_Peek(&(atCommandClientRef->curContext.atUnsolicitedList));
    /* Browse all the queue while the string is not found */
    while (linkPtr != NULL)
    {
        atUnsolicited_t *currUnsolicitedPtr = CONTAINER_OF(linkPtr,
                                                           atUnsolicited_t,
                                                           link);

        linkPtr = le_dls_PeekNext(&(atCommandClientRef->curContext.atUnsolicitedList), linkPtr);

        if (   ( unsolicitedPtr->unsolicitedReportId == currUnsolicitedPtr->unsolicitedReportId )
            && ( strcmp(unsolicitedPtr->unsolRsp,currUnsolicitedPtr->unsolRsp) == 0)
           )
        {
            LE_DEBUG("Unsolicited DEL %p <%s> DONE",
                     currUnsolicitedPtr->unsolicitedReportId,
                     currUnsolicitedPtr->unsolRsp);
            le_dls_Remove(&(atCommandClientRef->curContext.atUnsolicitedList),
                          &(currUnsolicitedPtr->link));
            le_mem_Release(currUnsolicitedPtr);
        }
    }

    le_mem_Release(report);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is to send a new AT command
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_SendCommand
(
    void *report    ///< Callback parameter
)
{
    le_atClient_ManagerStateMachine_t *atCommandClientRef = le_event_GetContextPtr();
    struct le_atClient_cmd* atcommandPtr = report;

    if (atcommandPtr)
    {
        LE_DEBUG("Adding command(%d) '%s' in list",atcommandPtr->commandId,atcommandPtr->command);
        le_mem_AddRef(atcommandPtr);
        le_dls_Queue(&(atCommandClientRef->curContext.atCommandList),&(atcommandPtr->link));
        (atCommandClientRef->curState)(atCommandClientRef,EVENT_MANAGER_SENDCMD);
    }

    le_mem_Release(report);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is to cancel an AT command
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_CancelCommand
(
    void *report    ///< Callback parameter
)
{
    le_atClient_ManagerStateMachine_t *atCommandClientRef = le_event_GetContextPtr();
    struct le_atClient_cmd *atcommandPtr = report;

    if (atcommandPtr)
    {
        LE_DEBUG("Canceling command(%d) '%s'",atcommandPtr->commandId,atcommandPtr->command);
        // if it is in the list it means that it has not started
        if ( le_dls_IsInList(&(atCommandClientRef->curContext.atCommandList),&(atcommandPtr->link)) )
        {
            le_dls_Remove(&(atCommandClientRef->curContext.atCommandList),&(atcommandPtr->link));
            le_mem_Release(atcommandPtr);
        }
        else if (atCommandClientRef->curContext.atCommandInProgressRef == atcommandPtr)
        {
            (atCommandClientRef->curState)(atCommandClientRef,EVENT_MANAGER_CANCELCMD);
        }
        else
        {
            char cmd[LE_ATCLIENT_CMD_SIZE_MAX_BYTES];
            le_dev_GetCommand(atcommandPtr,cmd,LE_ATCLIENT_CMD_SIZE_MAX_BYTES);
            LE_WARN("Try to cancel a command '%s' that does not exist anymore",cmd);
        }
    }

    le_mem_Release(report);
}





//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create an interface for the given device on a device.
 *
 * @return a reference on the atCommandClient of this device
 */
//--------------------------------------------------------------------------------------------------
DevRef_t le_dev_CreateInterface
(
    struct le_atClient_device* devicePtr    ///< Device
)
{
    char name[THREAD_NAME_MAX];
    le_thread_Ref_t newThreadPtr;

    struct le_atClient_mgr* newInterfacePtr = CreateInterface();

    LE_DEBUG("Create a new interface for '%s'",devicePtr->name);

    memcpy(&(newInterfacePtr->atCommandClient.curContext.le_atClient_device),
           devicePtr,
           sizeof(newInterfacePtr->atCommandClient.curContext.le_atClient_device));

    snprintf(name,THREAD_NAME_MAX,"atCommandClient-%s",devicePtr->name);
    newThreadPtr = le_thread_Create(name,InitThread,newInterfacePtr);

    le_thread_Start(newThreadPtr);
    le_sem_Wait(newInterfacePtr->waitingSemaphore);

    return newInterfacePtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to Cancel an AT Command.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atClient_mgr_CancelCommandRequest
(
    struct le_atClient_mgr      *devicePtr,         ///< one atCommandClient interface
    struct le_atClient_cmd      *atcommandToCancelRef     ///< AT Command Reference
)
{
    le_mem_AddRef(atcommandToCancelRef);
    le_event_ReportWithRefCounting(devicePtr->cancelCommandId,
                                   atcommandToCancelRef);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the ATParser FSM
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_InitializeState
(
    le_atClient_ParserStateMachineRef_t  smRef
)
{
    memset(smRef,0,sizeof(*smRef));
    smRef->curState = StartingState;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to read and send event to the ATParser FSM
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_ReadBuffer
(
    le_atClient_ParserStateMachineRef_t smRef
)
{
    EIndicationATParser event;
    for (;smRef->curContext.idx < smRef->curContext.endbuffer;)
    {
        if (GetNextEvent(smRef, &event))
        {
            (smRef->curState)(smRef,event);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete character that where already read.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_ResetBuffer
(
    le_atClient_ParserStateMachineRef_t smRef
)
{
    if (smRef->curState == ProcessingState)
    {
        size_t idx,sizeToCopy;
        sizeToCopy = smRef->curContext.endbuffer-smRef->curContext.idx_lastCRLF+2;
        LE_DEBUG("%d sizeToCopy %zd from %d",smRef->curContext.idx,sizeToCopy,smRef->curContext.idx_lastCRLF-2);
        for (idx = 0; idx < sizeToCopy; idx++) {
            smRef->curContext.buffer[idx] = smRef->curContext.buffer[idx+smRef->curContext.idx_lastCRLF-2];
        }
        smRef->curContext.idx_lastCRLF = 2;
        smRef->curContext.endbuffer = sizeToCopy;
        smRef->curContext.idx = smRef->curContext.endbuffer;
        LE_DEBUG("new idx %d, startLine %d",
                 smRef->curContext.idx,
                 smRef->curContext.idx_lastCRLF);
    }
    else
    {
        LE_DEBUG("Nothing should be copied in ATParser\n");
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * This function must be called when we want to read on device (or port)
 *
 * @return byte number read
 */
//--------------------------------------------------------------------------------------------------
int32_t le_dev_Read
(
    struct le_atClient_device  *devicePtr,    ///< device pointer
    uint8_t     *rxDataPtr,    ///< Buffer where to read
    uint32_t    size           ///< size of buffer
)
{
    int32_t status;
    int32_t r_amount = 0;

    status = read(devicePtr->handle, rxDataPtr, size);

    if(status > 0)
    {
        size      -= status;
        rxDataPtr += status;
        r_amount  += status;
    }

    LE_DEBUG("%s[%s] -> Read (%d) on %d",
             devicePtr->name,devicePtr->path,
             r_amount,devicePtr->handle);

    return r_amount;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to write on device (or port)
 *
 */
//--------------------------------------------------------------------------------------------------
void  le_dev_Write
(
    struct le_atClient_device *devicePtr,    ///< device pointer
    uint8_t         *rxDataPtr,    ///< Buffer to write
    uint32_t         buf_len           ///< size of buffer
)
{
    int32_t status;
    int32_t w_amount = 0;

    size_t size;
    size_t size_to_wr;
    ssize_t size_written;

    LE_FATAL_IF(devicePtr->handle==-1,"Write Handle error\n");

    for(size = 0; size < buf_len;)
    {
        size_to_wr = buf_len - size;
        if(size_to_wr > LE_UART_WRITE_MAX_SZ)
           size_to_wr = LE_UART_WRITE_MAX_SZ;

        size_written = write(devicePtr->handle, &rxDataPtr[size], size_to_wr);

        if (size_written==-1)
        {
            LE_WARN("Cannot write on uart: %s",strerror(errno));
        }

        LE_DEBUG("Uart Write: %zd", size_written);
        size += size_written;

        if(size_written != size_to_wr)
            break;
    }

    status = size;

    if(status > 0)
    {
        buf_len   -= status;
        w_amount  += status;
    }

    LE_DEBUG("%s[%s] -> write (%d) on %d",
             devicePtr->name,devicePtr->path,
             w_amount,devicePtr->handle);

    le_dev_PrintBuffer(devicePtr->name,rxDataPtr,w_amount);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to print a buffer byte by byte
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_PrintBuffer
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
            LE_DEBUG("'%s' -> [%d] '0x%.2x' '%c'",(namePtr)?namePtr:"no name",i,bufferPtr[i],bufferPtr[i]);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to check if the line match any of intermediate string of the command
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_CheckIntermediate
(
    le_atClient_CmdRef_t  atCommandRef,
    char           *atLinePtr,
    size_t          atLineSize
)
{
    LE_DEBUG("Start checking intermediate");

    CheckIntermediateExtraData(atCommandRef,atLinePtr,atLineSize);

    if (CheckList(atCommandRef,
                  atLinePtr,
                  atLineSize,
                  false))
    {
        atCommandRef->waitExtra = atCommandRef->withExtra;
    }

    LE_DEBUG("Stop checking intermediate");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to check if the line match any of final string of the command
 *
 */
//--------------------------------------------------------------------------------------------------
bool le_dev_CheckFinal
(
    le_atClient_CmdRef_t     atCommandRef,
    char           *atLinePtr,
    size_t          atLineSize
)
{
    bool result;
    LE_DEBUG("Start checking final");

    result = CheckList(atCommandRef,
                       atLinePtr,
                       atLineSize,
                       true);

    LE_DEBUG("Stop checking final");
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to prepare the atcommand:
 *  - adding CR at the end of the command
 *  - adding Ctrl-Z for data
 *
 */
//--------------------------------------------------------------------------------------------------
void le_dev_Prepare
(
    struct le_atClient_cmd *atcommandPtr
)
{
    /* Convert end of string into carrier return */
    LE_FATAL_IF((atcommandPtr->commandSize > LE_ATCLIENT_CMD_SIZE_MAX_BYTES),
                "command is too long(%zd): Max size=%d",
                atcommandPtr->commandSize , LE_ATCLIENT_CMD_SIZE_MAX_BYTES);

    atcommandPtr->command[atcommandPtr->commandSize++]   = '\r';
    atcommandPtr->command[atcommandPtr->commandSize] = '\0';

    if (atcommandPtr->dataSize && (atcommandPtr->dataSize <= LE_ATCLIENT_DATA_SIZE)) {
        /* Convert end of string into ctrl-z code */
        atcommandPtr->data[atcommandPtr->dataSize++] = 0x1A;
    }
}



//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the ID of a command
 *
 */
//--------------------------------------------------------------------------------------------------
uint32_t le_dev_GetId
(
    struct le_atClient_cmd      *atcommandPtr  ///< AT Command
)
{
    LE_FATAL_IF(atcommandPtr==NULL,"atcommandPtr is NULL : Cannot Get Id");

    return atcommandPtr->commandId;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the command string
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_dev_GetCommand
(
    struct le_atClient_cmd            *atcommandPtr,  ///< AT Command
    char                    *commandPtr,    ///< [IN/OUT] command buffer
    size_t                   commandSize    ///< [IN] size of commandPtr
)
{
    le_result_t result;
    LE_FATAL_IF(atcommandPtr==NULL,"atcommandPtr is NULL : Cannot Get command string");

    result = le_utf8_Copy(commandPtr,(const char*)(atcommandPtr->command),commandSize,NULL);

    if (result==LE_OK)
    {
        commandPtr[atcommandPtr->commandSize-1] = '\0'; // remove the CR char
    }
    else
    {
        commandPtr[commandSize] = '\0';
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create an atUnsolicited_t struct
 *
 * @return pointer on the structure
 */
//--------------------------------------------------------------------------------------------------
atUnsolicited_t* le_dev_Create
(
    void
)
{
    atUnsolicited_t* newUnsolicitedPtr = le_mem_ForceAlloc(AtUnsolicitedPool);

    memset(newUnsolicitedPtr->unsolRsp, 0 ,sizeof(newUnsolicitedPtr->unsolRsp));
    newUnsolicitedPtr->unsolicitedReportId = NULL,
    newUnsolicitedPtr->withExtraData    = false;
    newUnsolicitedPtr->waitForExtraData = false;
    newUnsolicitedPtr->link = LE_DLS_LINK_INIT;

    return newUnsolicitedPtr;
}
