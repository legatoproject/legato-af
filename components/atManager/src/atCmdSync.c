/**
 *
 * Copyright (C) Sierra Wireless, Inc. 2012. All rights reserved. Use of this work is subject to license.
 */


/** @file atcmdsync.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2012. All rights reserved. Use of this work is subject to license.
 */
#include "legato.h"

#include "../devices/adapter_layer/inc/le_da.h"
#include "../devices/uart/inc/le_uart.h"
#include "../inc/atMgr.h"

#include "../inc/atCmdSync.h"

#define DEFAULT_RESULT_POOL_SIZE    1
#define DEFAULT_SYNC_POOL_SIZE      1
#define DEFAULT_LINE_POOL_SIZE      1

//--------------------------------------------------------------------------------------------------
/**
 * One line returned by the modem
 */
//--------------------------------------------------------------------------------------------------
typedef struct atcmdsync_Line
{
    char            line[ATCMDSENDER_LINE];     ///< one line, send by the modem
    le_dls_Link_t   link;                       ///< used for (atcmdsync_Result_t)->lines
} atcmdsync_Line_t;

//--------------------------------------------------------------------------------------------------
/**
 * List of line returned by the modem
 */
//--------------------------------------------------------------------------------------------------
typedef struct atcmdsync_Result
{
    le_dls_List_t       lines;      ///< list of atcmdsync_Line_t
} atcmdsync_Result_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure to synchronize a sending to the modem.
 */
//--------------------------------------------------------------------------------------------------
typedef struct atcmdsync_Sync
{
    atmgr_Ref_t              interfaceRef;      ///< ATManager interface
    atcmdsync_Result_t      *resultPtr;         ///< Result of the Command
    atcmd_Ref_t              atCmdInProcessRef; ///< The command send
    le_sem_Ref_t             endSignal;         ///< Semaphore to make it all synchronous
    le_dls_Link_t            link;              ///< used for CommandList
} atcmdsync_Sync_t;

/* Pool reference for all internal structure */
static le_mem_PoolRef_t       ResultPoolRef;
static le_mem_PoolRef_t       SyncPoolRef;
static le_mem_PoolRef_t       LinePoolRef;

/*
 *Thread used for the EventIntermediateId (IntermediateHandler), and EventFinalId (FinalHandler)
 */
static le_thread_Ref_t        AtCmdThreadRef=NULL;
static le_event_Id_t          EventIntermediateId; // used to get intermediate pattern
static le_event_Id_t          EventFinalId;        // used to get final pattern

static le_dls_List_t          CommandList;    // List of all command in process


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a result
 *
 * @return a pointer to a atcmdsync_Result_t structure
 */
//--------------------------------------------------------------------------------------------------
static atcmdsync_Result_t* CreateResult()
{
    atcmdsync_Result_t* newPtr = le_mem_ForceAlloc(ResultPoolRef);

    newPtr->lines = LE_DLS_LIST_INIT;

    return newPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a atcmdsync_Sync_t
 *
 * @return a pointer to a atcmdsync_Sync_t structure
 */
//--------------------------------------------------------------------------------------------------
static atcmdsync_Sync_t* CreateCommandSync()
{
    atcmdsync_Sync_t* newPtr = le_mem_ForceAlloc(SyncPoolRef);

    newPtr->endSignal = le_sem_Create("ResultSignal",0);
    newPtr->link = LE_DLS_LINK_INIT;
    newPtr->atCmdInProcessRef = NULL;
    newPtr->resultPtr = CreateResult();

    return newPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a line
 *
 * @return a pointer to a atcmdsync_Result_t structure
 */
//--------------------------------------------------------------------------------------------------
static atcmdsync_Line_t* CreateLine
(
    const char* linePtr,
    size_t      lineSize
)
{
    LE_FATAL_IF(lineSize>=ATCMDSENDER_LINE,"line is too long, cannot create the structure");

    atcmdsync_Line_t* newPtr = le_mem_ForceAlloc(LinePoolRef);

    memcpy(newPtr->line,linePtr,lineSize);
    newPtr->line[lineSize] = '\0';

    newPtr->link = LE_DLS_LINK_INIT;

    return newPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function find the command associated to the response
 *
 */
//--------------------------------------------------------------------------------------------------
static atcmdsync_Sync_t* FindCommand(atcmd_Ref_t cmdRef)
{
    /* Init the current pointer with the first element */
    le_dls_Link_t* linkPtr = le_dls_Peek(&CommandList);

    while (linkPtr != NULL)
    {
        atcmdsync_Sync_t * currentPtr = CONTAINER_OF(linkPtr,atcmdsync_Sync_t,link);

        if (atcmd_GetId(currentPtr->atCmdInProcessRef)==atcmd_GetId(cmdRef)) {
            /* we have found the Command result so return the pointer */
            return currentPtr;
        }

        linkPtr = le_dls_PeekNext(&CommandList, linkPtr);
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called when debug
 *
 */
//--------------------------------------------------------------------------------------------------
void atcmdsync_Print(atcmdsync_Result_t* resultPtr)
{
    int i=0;

    le_dls_Link_t* linkPtr = le_dls_Peek(&(resultPtr->lines));

    /* Browse all the queue while the string is not found */
    while (linkPtr != NULL)
    {
        atcmdsync_Line_t *currLinePtr = CONTAINER_OF(linkPtr,
                                                       atcmdsync_Line_t,
                                                       link);
        LE_DEBUG("L%d: >%s<",
                 i++,
                 currLinePtr->line);

        linkPtr = le_dls_PeekNext(&(resultPtr->lines), linkPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is the handler for the timer of an AT Command.
 *
 */
//--------------------------------------------------------------------------------------------------
static void TimerHandler
(
    le_timer_Ref_t timerRef
)
{
    atcmd_Ref_t atcommandRef = le_timer_GetContextPtr(timerRef);

    atcmdsync_Sync_t* commandPtr = FindCommand(atcommandRef);
    if (commandPtr == NULL) {
        char command[ATCOMMAND_SIZE];
        atcmd_GetCommand(atcommandRef,command,ATCOMMAND_SIZE);
        LE_WARN("This command (%d)-%s- is not found",
                atcmd_GetId(atcommandRef),
                command);
    } else {
        atmgr_CancelCommandRequest(commandPtr->interfaceRef , commandPtr->atCmdInProcessRef);

        atcmdsync_Line_t* newLinePtr = CreateLine("TIMEOUT",sizeof("TIMEOUT"));

        le_dls_Queue(&(commandPtr->resultPtr->lines),&(newLinePtr->link));

        le_sem_Post(commandPtr->endSignal);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is the handler for intermediate line found of an AT Command.
 *
 */
//--------------------------------------------------------------------------------------------------
static void  IntermediateHandler(void* reportPtr) {
    atcmd_Response_t* responsePtr = reportPtr;

    char command[ATCOMMAND_SIZE];
    atcmd_GetCommand(responsePtr->fromWhoRef,command,ATCOMMAND_SIZE);

    LE_DEBUG("Handler Intermediate Response received for (%d)-%s-",
             atcmd_GetId(responsePtr->fromWhoRef),
             command);

    atcmdsync_Sync_t* commandPtr = FindCommand(responsePtr->fromWhoRef);
    if (commandPtr == NULL) {
        LE_WARN("This command (%d)-%s- is not found",
                atcmd_GetId(responsePtr->fromWhoRef),
                command);
    } else {
        atcmdsync_Line_t* newLinePtr = CreateLine(responsePtr->line,
                                                                strlen(responsePtr->line)+1);

        le_dls_Queue(&(commandPtr->resultPtr->lines),&(newLinePtr->link));
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is the handler for final line found of an AT Command.
 *
 *
 */
//--------------------------------------------------------------------------------------------------
static void FinalHandler(void* reportPtr) {
    atcmd_Response_t* responsePtr = reportPtr;

    char command[ATCOMMAND_SIZE];
    atcmd_GetCommand(responsePtr->fromWhoRef,command,ATCOMMAND_SIZE);

    LE_DEBUG("Handler Final Response received for (%d)-%s-",
             atcmd_GetId(responsePtr->fromWhoRef),
             command);

    atcmdsync_Sync_t* commandPtr = FindCommand(responsePtr->fromWhoRef);
    if (commandPtr == NULL) {
        LE_WARN("This command (%d)-%s- is not found",
                atcmd_GetId(responsePtr->fromWhoRef),
                command);
    } else {
        atcmdsync_Line_t* newLinePtr = CreateLine(responsePtr->line,
                                                                strlen(responsePtr->line)+1);

        le_dls_Queue(&(commandPtr->resultPtr->lines),&(newLinePtr->link));

        le_sem_Post(commandPtr->endSignal);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called when the last le_mem_Release is called
 *
 */
//--------------------------------------------------------------------------------------------------
static void ResultDestructor
(
    void* ptr   ///< atcmdsync_Result_t reference
)
{
    atcmdsync_Result_t* resultPtr = ptr;

    le_dls_Link_t* linkPtr;

    while ((linkPtr=le_dls_Pop(&(resultPtr->lines))) != NULL)
    {
        atcmdsync_Line_t * currentPtr = CONTAINER_OF(linkPtr,atcmdsync_Line_t,link);

        le_mem_Release(currentPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is called when the last le_mem_Release is called
 *
 */
//--------------------------------------------------------------------------------------------------
static void SyncDestructor
(
    void* ptr   ///< atcmdsync_Result_t reference
)
{
    atcmdsync_Sync_t* syncPtr = ptr;

    le_sem_Delete(syncPtr->endSignal);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the number of lines in response
 *
 */
//--------------------------------------------------------------------------------------------------
size_t atcmdsync_GetNumLines
(
    atcmdsync_Result_t *resultPtr
)
{
    LE_FATAL_IF(resultPtr==NULL, "bad parameter");

    return le_dls_NumLinks(&(resultPtr->lines));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get line in index
 *
 */
//--------------------------------------------------------------------------------------------------
char* atcmdsync_GetLine
(
    atcmdsync_Result_t *resultPtr,
    uint32_t            index
)
{
    le_dls_Link_t* linkPtr;
    LE_FATAL_IF(resultPtr==NULL, "bad parameter");

    uint32_t i;
    linkPtr = le_dls_Peek(&(resultPtr->lines));
    for(i=0;i<index;i++)
    {
        linkPtr = le_dls_PeekNext(&(resultPtr->lines), linkPtr);
    }
    if (linkPtr) {
        atcmdsync_Line_t *currLinePtr = CONTAINER_OF(linkPtr,
                                                     atcmdsync_Line_t,
                                                     link);

        return currLinePtr->line;
    } else {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the final lines
 * ie: sucess or error code
 *
 */
//--------------------------------------------------------------------------------------------------
char* atcmdsync_GetFinalLine
(
    atcmdsync_Result_t *resultPtr
)
{
    le_dls_Link_t* linkPtr;
    LE_FATAL_IF(resultPtr==NULL, "bad parameter");

    if ((linkPtr = le_dls_PeekTail(&(resultPtr->lines))) != NULL) {
        atcmdsync_Line_t *currLinePtr = CONTAINER_OF(linkPtr,
                                                     atcmdsync_Line_t,
                                                     link);

        return currLinePtr->line;
    } else {
        return NULL;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the intermediate event Id
 *
 * @return the event id
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t atcmdsync_GetIntermediateEventId
(
    void
)
{
    return EventIntermediateId;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the final event Id
 *
 * @return the event id
 */
//--------------------------------------------------------------------------------------------------
le_event_Id_t atcmdsync_GetFinalEventId
(
    void
)
{
    return EventFinalId;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the timer expiry handler
 *
 * @return le_timer_ExpiryHandler_t hander
 */
//--------------------------------------------------------------------------------------------------
le_timer_ExpiryHandler_t  atcmdsync_GetTimerExpiryHandler
(
    void
)
{
    return TimerHandler;
}
//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send an AT Command and wait for response.
 *
 * @return pointer to the response
 */
//--------------------------------------------------------------------------------------------------
atcmdsync_Result_t* atcmdsync_SendCommand
(
    atmgr_Ref_t  interfacePtr, ///< Interface where to send the sync command
    atcmd_Ref_t  atReqRef      ///< AT Request to execute
)
{
    atcmdsync_Sync_t* syncPtr = CreateCommandSync();
    atcmdsync_Result_t* resultPtr = syncPtr->resultPtr;

    le_mem_AddRef(atReqRef);
    syncPtr->atCmdInProcessRef = atReqRef;
    le_dls_Queue(&CommandList,&(syncPtr->link));

    syncPtr->interfaceRef = interfacePtr;

    atmgr_SendCommandRequest(interfacePtr , atReqRef);
    le_sem_Wait(syncPtr->endSignal);

    le_dls_Remove(&CommandList,&(syncPtr->link));

    LE_DEBUG("Command(%d)'s result",atcmd_GetId(atReqRef));
    atcmdsync_Print(resultPtr);

    le_mem_Release(syncPtr);
    le_mem_Release(atReqRef);

    return resultPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function Initialize atcmdsender.
 * Must be call only once
 *
 */
//--------------------------------------------------------------------------------------------------
static void Init
(
)
{
    ResultPoolRef = le_mem_CreatePool("ResultPool",sizeof(atcmdsync_Result_t));
    ResultPoolRef = le_mem_ExpandPool(ResultPoolRef,DEFAULT_RESULT_POOL_SIZE);
    le_mem_SetDestructor(ResultPoolRef,ResultDestructor);

    SyncPoolRef = le_mem_CreatePool("SyncPool",sizeof(atcmdsync_Sync_t));
    SyncPoolRef = le_mem_ExpandPool(SyncPoolRef,DEFAULT_SYNC_POOL_SIZE);
    le_mem_SetDestructor(SyncPoolRef,SyncDestructor);

    LinePoolRef = le_mem_CreatePool("LinePool",sizeof(atcmdsync_Line_t));
    LinePoolRef = le_mem_ExpandPool(LinePoolRef,DEFAULT_LINE_POOL_SIZE);

    EventIntermediateId   = le_event_CreateId("atcmdsenderInter",sizeof(atcmd_Response_t));
    EventFinalId          = le_event_CreateId("atcmdsenderfinal",sizeof(atcmd_Response_t));

    le_event_AddHandler("atcmdsync_FinalHandler",EventFinalId,FinalHandler);
    le_event_AddHandler("atcmdsync_IntermediateHandler",EventIntermediateId ,IntermediateHandler);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to run the atcmd tools thread
 *
 */
//--------------------------------------------------------------------------------------------------
static void* atcmdsync_thread(void* context)
{
    le_sem_Ref_t semPtr = context;
    LE_INFO("Start AT commands Sender tools");

    Init();

    le_sem_Post(semPtr);
    le_event_RunLoop();
    return NULL;
}

// This prototype is needed to register Modem Services as a component.
// todo: This is temporary until the build tools do this automatically
le_log_SessionRef_t log_RegComponent
(
    const char* componentNamePtr,       ///< [IN] A pointer to the component's name.
    le_log_Level_t** levelFilterPtr     ///< [OUT] Set to point to the component's level filter.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function Initialize the platform adapter layer.
 * Must be call only once
 *
 * @return LE_OK           The function succeeded.
 * @return LE_DUPLICATE    Module is already initialized
 */
//--------------------------------------------------------------------------------------------------
le_result_t atcmdsync_Init
(
)
{
    if ( atmgr_IsStarted() == false )
    {
        atmgr_Start();
    }

    if ( AtCmdThreadRef == NULL )
    {
        le_sem_Ref_t semPtr = le_sem_Create("atCmdSenderStartSem",0);

        AtCmdThreadRef = le_thread_Create("AtCmdSender",atcmdsync_thread,semPtr);
        le_thread_Start(AtCmdThreadRef);

        le_sem_Wait(semPtr);
        LE_INFO("AT commands Sender tools is started");
        le_sem_Delete(semPtr);
        return LE_OK;
    } else {
        return LE_DUPLICATE;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send a standard command
 * This is the default sending with intermediate pattern
 *
 * @return LE_NOT_POSSIBLE when pattern match : ERROR, +CME ERROR, +CMS ERROR
 * @return LE_TIMEOUT when pattern match : TIMEOUT
 * @return LE_OK when pattern match : OK
 */
//--------------------------------------------------------------------------------------------------
le_result_t atcmdsync_SendStandard
(
    atmgr_Ref_t              interfaceRef,   ///< [IN] Interface where to send the sync command
    const char              *commandPtr,     ///< [IN] the command string to execute
    atcmdsync_ResultRef_t   *responseRefPtr, ///< [IN/OUT] filled the response if not NULL
    const char             **intermediatePatternPtr, ///< [IN] intermediate pattern expected
    uint32_t                 timer           ///< [IN] the timer in seconds
)
{
    const char* finalRespOkPtr[] = { "OK" , NULL };
    const char* finalRespKoPtr[] = { "ERROR","+CME ERROR:","+CMS ERROR","TIMEOUT",NULL};

    atcmd_Ref_t atReqRef = atcmdsync_PrepareStandardCommand(commandPtr,
                                                                   intermediatePatternPtr,
                                                                   finalRespOkPtr,
                                                                   finalRespKoPtr,
                                                                   timer);

    atcmdsync_ResultRef_t  atRespRef = atcmdsync_SendCommand(interfaceRef,atReqRef);
    le_result_t result = atcmdsync_CheckCommandResult(atRespRef,
                                                      finalRespOkPtr,
                                                      finalRespKoPtr);

    if (responseRefPtr) {
        *responseRefPtr = atRespRef;
        le_mem_AddRef(atRespRef);
    }

    le_mem_Release(atReqRef);
    le_mem_Release(atRespRef);

    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a command with the good parameters
 *
 * @return reference on a new atcommand
 */
//--------------------------------------------------------------------------------------------------
atcmd_Ref_t atcmdsync_PrepareStandardCommand
(
    const char          *commandPtr,    ///< [IN] the command string to execute
    const char         **intermediatePatternPtr, ///< [IN] intermediate pattern expected
    const char         **finalSuccessPatternPtr, ///< [IN] final pattern that will succeed
    const char         **finalFailedPatternPtr, ///< [IN] final pattern that will failed
    uint32_t             timer         ///< [IN] the timer in seconds
)
{
    atcmd_Ref_t atReqRef = atcmd_Create();
    atcmd_AddCommand(atReqRef,commandPtr,false);
    atcmd_AddData(atReqRef,NULL,0);
    atcmd_SetTimer (atReqRef,timer,atcmdsync_GetTimerExpiryHandler());
    atcmd_AddIntermediateResp(atReqRef,atcmdsync_GetIntermediateEventId(),intermediatePatternPtr);
    atcmd_AddFinalResp(atReqRef,atcmdsync_GetFinalEventId(),finalSuccessPatternPtr);
    atcmd_AddFinalResp(atReqRef,atcmdsync_GetFinalEventId(),finalFailedPatternPtr);

    return atReqRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to check the result structure
 *
 * @return LE_NOT_POSSIBLE when finalFailedPatternPtr match
 * @return LE_TIMEOUT when a timeout occur
 * @return LE_OK when finalSuccessPattern match
 */
//--------------------------------------------------------------------------------------------------
le_result_t atcmdsync_CheckCommandResult
(
    atcmdsync_Result_t  *resultPtr,    ///< [IN] the command result to check
    const char         **finalSuccessPatternPtr, ///< [IN] final pattern that will succeed
    const char         **finalFailedPatternPtr ///< [IN] final pattern that will failed
)
{
    const char* finalLinePtr = atcmdsync_GetFinalLine(resultPtr);
    const char* tmp=NULL;
    uint32_t i=0;
    if (strcmp("TIMEOUT", finalLinePtr)==0) {
        LE_WARN("Modem failed with TIMEOUT");
        return LE_TIMEOUT;
    }
    for(i=0,tmp=finalFailedPatternPtr[0];tmp!=NULL;tmp=finalFailedPatternPtr[++i]) {
        if ( (strncmp(tmp, finalLinePtr,strlen(tmp)))==0)  {
            LE_WARN("Modem failed with '%s'",finalLinePtr);
            return LE_NOT_POSSIBLE;
        }
    }
    for(i=0,tmp=finalSuccessPatternPtr[0];tmp!=NULL;tmp=finalSuccessPatternPtr[++i]) {
            if (strncmp(tmp, finalLinePtr,strlen(tmp))==0)  {
                return LE_OK;
            }
    }

    LE_WARN("Modem failed with unspecified error '%s'",finalLinePtr);
    return LE_NOT_POSSIBLE;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to fill the buffer commandPtr with the command string
 *
 * if the buffer is too small, there is a LE_WARN
 */
//--------------------------------------------------------------------------------------------------
void atcmdsync_PrepareString
(
    char        *commandPtr,    ///< [OUT] the command result to check
    uint32_t     commandSize,   ///< [IN] the commandPtr buffer size
    char        *formatPtr,        ///< [IN] the format
    ...
)
{
    va_list aptr;
    int size;

    va_start(aptr, formatPtr);
    size = vsnprintf(commandPtr,commandSize, formatPtr, aptr);
    va_end(aptr);

    if ( size >= commandSize ) {
        LE_ERROR("string \"%s\" is too big, buffer(%d) should have %d bytes",
                 commandPtr,commandSize,size);
    }
}
