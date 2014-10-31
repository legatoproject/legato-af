/** @file atmachinemanager.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "atMachineMgrItf.h"
#include "atMachineManager.h"
#include "atMachineParser.h"
#include "atMachineDevice.h"
#include "atMachineCommand.h"
#include "atMachineUnsolicited.h"

#define ONE_MSEC    1000

/*
 * ATManager state machine
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
 */
static void WaitingState(ATManagerStateMachineRef_t smRef,EIndicationATManager input);
static void SendingState(ATManagerStateMachineRef_t smRef,EIndicationATManager input);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to initialize the ATManager FSM
 *
 */
//--------------------------------------------------------------------------------------------------
static void InitializeState
(
    ATManagerStateMachineRef_t  smRef
)
{
    atmachineparser_InitializeState(&(smRef->curContext.atParser));
    smRef->curContext.atUnsolicitedList = LE_DLS_LIST_INIT;
    smRef->curContext.atCommandList = LE_DLS_LIST_INIT;
    smRef->curContext.atCommandTimer = le_timer_Create("AtManagerTimer");
    smRef->curContext.atParser.atManagerPtr = smRef;
    smRef->curState = WaitingState;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to check if the extra data should be report
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckUnsolicitedExtraData
(
    ATManagerStateMachineRef_t  smRef,
    char                       *unsolicitedPtr,
    size_t                      unsolicitedSize
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
            atmgr_UnsolResponse_t atResp;

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
    ATManagerStateMachineRef_t  smRef,
    char                       *unsolicitedPtr,
    size_t                      unsolicitedSize
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

        if ( strncmp(unsolicitedPtr,currUnsolicitedPtr->unsolRsp,currUnsolicitedPtrSize) == 0)
        {
            atmgr_UnsolResponse_t atResp;

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
    ATManagerStateMachineRef_t  smRef,
    char                       *unsolicitedPtr,
    size_t                      unsolicitedSize
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
    ATManagerStateMachineRef_t smRef
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
    ATManagerStateMachineRef_t smRef
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
    int fd  ///< File descriptor to read on
)
{
    int32_t size = 0;
    ATManagerStateMachine_t *atManagerRef = le_event_GetContextPtr();
    ATParserStateMachineRef_t  atParserRef = &(atManagerRef->curContext.atParser) ;

    LE_DEBUG("Start read");

    /* Read RX data on uart */
    size = atmachinedevice_Read(&(atManagerRef->curContext.atDevice),
                         (uint8_t *)(atParserRef->curContext.buffer + atParserRef->curContext.idx),
                         (ATFSMPARSER_BUFFER_MAX - atParserRef->curContext.idx));

    /* Start the parsing only if we have read some bytes */
    if (size > 0)
    {
        LE_DEBUG(">>> Read %d bytes (FillIndex=%d)", size,  atParserRef->curContext.idx);
        /* Increment AT command buffer index */
        atParserRef->curContext.endbuffer += size;
        LE_DEBUG("Increase Rx Buffer Index: FillIndex = %zd", atParserRef->curContext.endbuffer);

        atmachinedevice_PrintBuffer(atManagerRef->curContext.atDevice.name,
                             atParserRef->curContext.buffer,
                             atParserRef->curContext.endbuffer);

        /* Call the parser */
        atmachineparser_ReadBuffer(atParserRef);
        atmachineparser_ResetBuffer(atParserRef);

    }
    if (atParserRef->curContext.endbuffer > ATFSMPARSER_BUFFER_MAX)
    {
        LE_WARN("Rx Buffer Overflow (FillIndex = %d)!!!", atParserRef->curContext.idx);
    }

    LE_DEBUG("read finished");
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to save the line to process, and execute the ATManager FSM
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemanager_ProcessLine
(
    ATManagerStateMachineRef_t smRef,
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
 * This function is to resume the current ATManager
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemanager_Resume
(
    void *report    ///< Callback parameter
)
{
    char monitorName[64];
    le_event_FdMonitorRef_t fdMonitorRef;
    struct atmgr *interfacePtr = le_event_GetContextPtr();

    if (interfacePtr->atManager.curContext.atDevice.fdMonitor) {
        LE_WARN("Interface %s already started",interfacePtr->atManager.curContext.atDevice.name);
        le_sem_Post(interfacePtr->waitingSemaphore);
        return;
    }

    InitializeState(&(interfacePtr->atManager));

    interfacePtr->atManager.curContext.atDevice.handle =
                interfacePtr->atManager.curContext.atDevice.deviceItf.open(
                    interfacePtr->atManager.curContext.atDevice.path
                );
    LE_FATAL_IF(interfacePtr->atManager.curContext.atDevice.handle==-1,"Open device failed");

    // Create a File Descriptor Monitor object for the serial port's file descriptor.
    snprintf(monitorName,64,"%s-Monitor",interfacePtr->atManager.curContext.atDevice.name);
    monitorName[63]='\0';
    fdMonitorRef = le_event_CreateFdMonitor(monitorName,
                                            interfacePtr->atManager.curContext.atDevice.handle);
    // Register a read handler.
    interfacePtr->fdHandlerRef = le_event_SetFdHandler(
                                                        fdMonitorRef,
                                                        LE_EVENT_FD_READABLE,
                                                        RxNewData);
    interfacePtr->atManager.curContext.atDevice.fdMonitor = fdMonitorRef;

    le_event_SetFdHandlerContextPtr(interfacePtr->fdHandlerRef,&(interfacePtr->atManager));

    {
        char threadName[25];
        le_thread_GetName(le_thread_GetCurrent(),threadName,25);
        LE_DEBUG("Resume %s with handle(%d)(%p) [%s]",
                 threadName,
                 interfacePtr->atManager.curContext.atDevice.handle,
                 interfacePtr->atManager.curContext.atDevice.fdMonitor,
                 monitorName
                );
    }

    le_sem_Post(interfacePtr->waitingSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is to stop the current ATManager
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemanager_Suspend
(
    void *report    ///< Callback parameter
)
{
    struct atmgr *interfacePtr = le_event_GetContextPtr();

    if (interfacePtr->atManager.curContext.atDevice.fdMonitor==NULL) {
        LE_WARN("Interface %s already stoped",interfacePtr->atManager.curContext.atDevice.name);
        le_sem_Post(interfacePtr->waitingSemaphore);
        return;
    }

    {
        char threadName[25];
        le_thread_GetName(le_thread_GetCurrent(),threadName,25);
        LE_DEBUG("Suspend %s with handle(%d)(%p)",
                 threadName,
                 interfacePtr->atManager.curContext.atDevice.handle,
                 interfacePtr->atManager.curContext.atDevice.fdMonitor
        );
    }

    le_timer_SetHandler(interfacePtr->atManager.curContext.atCommandTimer,NULL);
    le_timer_Delete(interfacePtr->atManager.curContext.atCommandTimer);

    le_event_DeleteFdMonitor(interfacePtr->atManager.curContext.atDevice.fdMonitor);
    interfacePtr->atManager.curContext.atDevice.deviceItf.close(
                                    interfacePtr->atManager.curContext.atDevice.handle);

    interfacePtr->atManager.curContext.atDevice.fdMonitor = NULL;
    interfacePtr->atManager.curContext.atDevice.handle = 0;

    le_sem_Post(interfacePtr->waitingSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is to add unsolicited string into current atmanager
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemanager_AddUnsolicited
(
    void *report    ///< Callback parameter
)
{
    ATManagerStateMachine_t *atManagerRef = le_event_GetContextPtr();
    atUnsolicited_t* unsolicitedPtr = report;

    LE_DEBUG("Unsolicited ADD %p <%s>",
             unsolicitedPtr->unsolicitedReportId,
             unsolicitedPtr->unsolRsp);

    unsolicitedPtr->link = LE_DLS_LINK_INIT;
    le_mem_AddRef(unsolicitedPtr);
    le_dls_Queue(&(atManagerRef->curContext.atUnsolicitedList),&(unsolicitedPtr->link));

    le_mem_Release(report);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is to remove unsolicited string from current atmanager
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemanager_RemoveUnsolicited
(
    void *report    ///< Callback parameter
)
{
    ATManagerStateMachine_t *atManagerRef = le_event_GetContextPtr();
    atUnsolicited_t* unsolicitedPtr = report;

    LE_DEBUG("Unsolicited DEL %p <%s>",
             unsolicitedPtr->unsolicitedReportId,
             unsolicitedPtr->unsolRsp);

    le_dls_Link_t* linkPtr = le_dls_Peek(&(atManagerRef->curContext.atUnsolicitedList));
    /* Browse all the queue while the string is not found */
    while (linkPtr != NULL)
    {
        atUnsolicited_t *currUnsolicitedPtr = CONTAINER_OF(linkPtr,
                                                           atUnsolicited_t,
                                                           link);

        linkPtr = le_dls_PeekNext(&(atManagerRef->curContext.atUnsolicitedList), linkPtr);

        if (   ( unsolicitedPtr->unsolicitedReportId == currUnsolicitedPtr->unsolicitedReportId )
            && ( strcmp(unsolicitedPtr->unsolRsp,currUnsolicitedPtr->unsolRsp) == 0)
           )
        {
            LE_DEBUG("Unsolicited DEL %p <%s> DONE",
                     currUnsolicitedPtr->unsolicitedReportId,
                     currUnsolicitedPtr->unsolRsp);
            le_dls_Remove(&(atManagerRef->curContext.atUnsolicitedList),
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
void atmachinemanager_SendCommand
(
    void *report    ///< Callback parameter
)
{
    ATManagerStateMachine_t *atManagerRef = le_event_GetContextPtr();
    struct atcmd* atcommandPtr = report;

    if (atcommandPtr)
    {
        LE_DEBUG("Adding command(%d) '%s' in list",atcommandPtr->commandId,atcommandPtr->command);
        le_mem_AddRef(atcommandPtr);
        le_dls_Queue(&(atManagerRef->curContext.atCommandList),&(atcommandPtr->link));
        (atManagerRef->curState)(atManagerRef,EVENT_MANAGER_SENDCMD);
    }

    le_mem_Release(report);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is to cancel an AT command
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinemanager_CancelCommand
(
    void *report    ///< Callback parameter
)
{
    ATManagerStateMachine_t *atManagerRef = le_event_GetContextPtr();
    struct atcmd *atcommandPtr = report;

    if (atcommandPtr)
    {
        LE_DEBUG("Canceling command(%d) '%s'",atcommandPtr->commandId,atcommandPtr->command);
        // if it is in the list it means that it has not started
        if ( le_dls_IsInList(&(atManagerRef->curContext.atCommandList),&(atcommandPtr->link)) )
        {
            le_dls_Remove(&(atManagerRef->curContext.atCommandList),&(atcommandPtr->link));
            le_mem_Release(atcommandPtr);
        }
        else if (atManagerRef->curContext.atCommandInProgressRef == atcommandPtr)
        {
            (atManagerRef->curState)(atManagerRef,EVENT_MANAGER_CANCELCMD);
        }
        else
        {
            char cmd[ATCOMMAND_SIZE];
            atcmd_GetCommand(atcommandPtr,cmd,ATCOMMAND_SIZE);
            LE_WARN("Try to cancel a command '%s' that does not exist anymore",cmd);
        }
    }

    le_mem_Release(report);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is used to do a transition between two state with one event.
 *
 */
//--------------------------------------------------------------------------------------------------
static void updateTransition
(
    ATManagerStateMachineRef_t   smRef,
 EIndicationATManager         input,
 ATManagerStateProc_Func_t    newState
)
{
    smRef->prevState   = smRef->curState;
    smRef->curState    = newState;
    smRef->lastEvent   = input;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is a state of the ATManager FSM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void WaitingState
(
    ATManagerStateMachineRef_t smRef,
    EIndicationATManager input
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

            smRef->curContext.atCommandInProgressRef = CONTAINER_OF(linkPtr,struct atcmd,link);
            if (smRef->curContext.atCommandInProgressRef) {

                LE_DEBUG("Executing command(%d) '%s' from list",
                         smRef->curContext.atCommandInProgressRef->commandId,
                         smRef->curContext.atCommandInProgressRef->command);

                if (smRef->curContext.atCommandInProgressRef->timer>0) {
                    updateTransition(smRef,input,SendingState);
                    StartTimer(smRef);
                }
                atmachinecommand_Prepare(smRef->curContext.atCommandInProgressRef);

                atmachinedevice_Write(&(smRef->curContext.atDevice),
                               smRef->curContext.atCommandInProgressRef->command,
                               smRef->curContext.atCommandInProgressRef->commandSize);

                if (smRef->curContext.atCommandInProgressRef->timer==0) {
                    le_mem_Release(smRef->curContext.atCommandInProgressRef);
                    smRef->curContext.atCommandInProgressRef = NULL;
                }

                LE_DEBUG("There is still %zd waiting command",
                         le_dls_NumLinks(&(smRef->curContext.atCommandList)));
            } else {
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
 * This function is a state of the ATManager FSM.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendingState
(
    ATManagerStateMachineRef_t smRef,
    EIndicationATManager input
)
{
    switch (input)
    {
        case EVENT_MANAGER_SENDDATA:
        {
            // Send data
            atmachinedevice_Write(&(smRef->curContext.atDevice),
                           smRef->curContext.atCommandInProgressRef->data,
                           smRef->curContext.atCommandInProgressRef->dataSize);
            break;
        }
        case EVENT_MANAGER_PROCESSLINE:
        {
            CheckUnsolicited(smRef,
                                       smRef->curContext.atLine,
                                       strlen(smRef->curContext.atLine));

            if ( atmachinecommand_CheckFinal(smRef->curContext.atCommandInProgressRef,
                smRef->curContext.atLine,
                strlen(smRef->curContext.atLine)) )
            {
                StopTimer(smRef);

                le_mem_Release(smRef->curContext.atCommandInProgressRef);
                smRef->curContext.atCommandInProgressRef = NULL;

                updateTransition(smRef,input,WaitingState);
                // Send the next command
                (smRef->curState)(smRef,EVENT_MANAGER_SENDCMD);
                return;
            }

            atmachinecommand_CheckIntermediate(smRef->curContext.atCommandInProgressRef,
                                        smRef->curContext.atLine,
                                        strlen(smRef->curContext.atLine));
            break;
        }
        case EVENT_MANAGER_CANCELCMD:
        {
            StopTimer(smRef);
            le_mem_Release(smRef->curContext.atCommandInProgressRef);
            smRef->curContext.atCommandInProgressRef = NULL;
            updateTransition(smRef,input,WaitingState);
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
