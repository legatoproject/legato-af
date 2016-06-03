/** @file le_atClient.c
 *
 * Copyright (C) Sierra Wireless Inc. Use of this work is subject to license.
 */

#include "legato.h"
#include "le_dev.h"
#include "le_basics.h"
#include <string.h>

#define LE_ATCLIENT_RESULT_POOL_SIZE        1
#define LE_ATCLIENT_SYNC_POOL_SIZE          1
#define LE_ATCLIENT_LINE_POOL_SIZE          1
#define LE_ATCLIENT_CMD_POOL_SIZE           1
#define DEFAULT_ATSTRING_POOL_SIZE          1

#define LE_ATCLIENT_CMD_MAX_BYTES           32
#define LE_ATCLIENT_INTER_MAX_BYTES         32
#define LE_ATCLIENT_RESP_MAX_BYTES          32
#define LE_ATCLIENT_DATA_MAX_BYTES          256
#define CMD_DEFAULT_TIMEOUT                 30000

#define AT_COMMAND                  "/dev/ttyAMA0"
#define AT_PPP                      "/dev/ttyACM0"

static uint32_t          IdCpt = 0;
static le_mem_PoolRef_t  AtCommandPool;
static le_mem_PoolRef_t  AtStringPool;
static le_ref_MapRef_t   CmdRefMap;

/*
 * Array of all Ports created.
 * This will correspond to LE_ATCLIENT_PORT_MAX threads created.
 */
static DevRef_t AllPorts[LE_ATCLIENT_PORT_MAX];
static bool     IsInitialized = false;

//--------------------------------------------------------------------------------------------------
/**
 * This will be the reference for the response of the modem.
 */
//--------------------------------------------------------------------------------------------------
typedef struct CmdSync_Result* le_atClient_CmdSyncResultRef_t;

//--------------------------------------------------------------------------------------------------
/**
 * One line returned by the modem.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    char            line[LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES];     ///< one line, send by the modem
    le_dls_Link_t   link;                                ///< used for (CmdSync_Result_t)->lines
}
CmdSync_Line_t;

//--------------------------------------------------------------------------------------------------
/**
 * List of line returned by the modem.
 */
//--------------------------------------------------------------------------------------------------
typedef struct CmdSync_Result
{
    le_dls_List_t       lines;      ///< list of CmdSync_Line_t
}
CmdSync_Result_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure to synchronize a sending to the modem.
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    DevRef_t                            interfaceRef;                ///< atCommandClient interface
    CmdSync_Result_t                   *resultPtr;                   ///< Result of the Command
    le_atClient_CmdRef_t                le_atClient_cmdInProcessRef; ///< The command send
    le_sem_Ref_t                        endSignal;                   ///< Semaphore to make it all synchronous
    le_dls_Link_t                       link;                        ///< used for CommandList
}
CmdSync_t;

//--------------------------------------------------------------------------------------------------
/**
 * Structure of an AT Command.
 */
//--------------------------------------------------------------------------------------------------
typedef struct AtCmd
{
    uint32_t                        commandId;  ///< Id for the command
    char                            cmd[LE_ATCLIENT_CMD_MAX_BYTES];
    le_dls_List_t                   intermediateExpectResponseList;   ///< List of string pattern for intermediate
    bool                            expInterRespIndicator;
    le_dls_List_t                   expectResponseList; ///< List of string pattern for final (ends the command)
    bool                            expRespIndicator;
    char                            data[LE_ATCLIENT_DATA_MAX_BYTES];
    size_t                          dataSize;   ///< sizeof data to send
    DevRef_t                        interface;
    uint32_t                        timeout;
    le_atClient_CmdRef_t            ref;
    le_atClient_CmdSyncResultRef_t  resultRefPtr;
    int                             lineCount;
}
AtCmd_t;


/* Pool reference for all internal structure */
static le_mem_PoolRef_t       ResultPoolRef;
static le_mem_PoolRef_t       SyncPoolRef;
static le_mem_PoolRef_t       LinePoolRef;

/*
 *Thread used for the EventIntermediateId (IntermediateHandler), and EventFinalId (FinalHandler)
 */
static le_thread_Ref_t        le_atClient_cmdThreadRef = NULL;
static le_event_Id_t          EventIntermediateId; // used to get intermediate pattern
static le_event_Id_t          EventFinalId;        // used to get final pattern
static le_dls_List_t          CommandList;         // List of all command in process

// This prototype is needed to register Modem Services as a component.
// todo: This is temporary until the build tools do this automatically
le_log_SessionRef_t log_RegComponent
(
    const char*      componentNamePtr,      ///< [IN] A pointer to the component's name.
    le_log_Level_t** levelFilterPtr         ///< [OUT] Set to point to the component's level filter.
);

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a result
 *
 * @return a pointer to a CmdSync_Result_t structure
 */
//--------------------------------------------------------------------------------------------------
static CmdSync_Result_t* CreateResult
(
    void
)
{
    CmdSync_Result_t* newPtr = le_mem_ForceAlloc(ResultPoolRef);

    newPtr->lines = LE_DLS_LIST_INIT;

    return newPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a CmdSync_t
 *
 * @return a pointer to a CmdSync_t structure
 */
//--------------------------------------------------------------------------------------------------
static CmdSync_t* CreateCommandSync
(
    void
)
{
    CmdSync_t* newPtr = le_mem_ForceAlloc(SyncPoolRef);

    newPtr->endSignal = le_sem_Create("ResultSignal",0);
    newPtr->link = LE_DLS_LINK_INIT;
    newPtr->le_atClient_cmdInProcessRef = NULL;
    newPtr->resultPtr = CreateResult();

    return newPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to Send an AT Command.
 *
 */
//--------------------------------------------------------------------------------------------------
static void SendCommandRequest
(
    struct le_atClient_mgr*      atcommandclientitfPtr,     ///< one atCommandClient interface
    struct le_atClient_cmd *     atcommandToSendPtr         ///< AT Command Reference
)
{
    le_mem_AddRef(atcommandToSendPtr);
    le_event_ReportWithRefCounting(atcommandclientitfPtr->sendCommandId,
                                   atcommandToSendPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a line
 *
 * @return a pointer to a CmdSync_Result_t structure
 */
//--------------------------------------------------------------------------------------------------
static CmdSync_Line_t* CreateLine
(
    const char* linePtr,
    size_t      lineSize
)
{
    LE_FATAL_IF(lineSize>=LE_ATCLIENT_RESPLINE_SIZE_MAX_BYTES,"line is too long, cannot create the structure");

    CmdSync_Line_t* newPtr = le_mem_ForceAlloc(LinePoolRef);

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
static CmdSync_t* FindCommand
(
    le_atClient_CmdRef_t cmdRef
)
{
    /* Init the current pointer with the first element */
    le_dls_Link_t* linkPtr = le_dls_Peek(&CommandList);

    while (linkPtr != NULL)
    {
        CmdSync_t * currentPtr = CONTAINER_OF(linkPtr,CmdSync_t,link);

        if (le_dev_GetId(currentPtr->le_atClient_cmdInProcessRef)==le_dev_GetId(cmdRef))
        {
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
static void CmdSync_Print
(
    CmdSync_Result_t* resultPtr
)
{
    int i=0;

    le_dls_Link_t* linkPtr = le_dls_Peek(&(resultPtr->lines));

    /* Browse all the queue while the string is not found */
    while (linkPtr != NULL)
    {
        CmdSync_Line_t *currLinePtr = CONTAINER_OF(linkPtr, CmdSync_Line_t, link);
        LE_DEBUG("L%d: >%s<", i++, currLinePtr->line);
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
    le_atClient_CmdRef_t atcommandRef = le_timer_GetContextPtr(timerRef);

    CmdSync_t* commandPtr = FindCommand(atcommandRef);

    if (commandPtr == NULL)
    {
        char command[LE_ATCLIENT_CMD_SIZE_MAX_BYTES];
        le_dev_GetCommand(atcommandRef,command,LE_ATCLIENT_CMD_SIZE_MAX_BYTES);
        LE_WARN("This command (%d)-%s- is not found", le_dev_GetId(atcommandRef), command);
    }
    else
    {
        CmdSync_Line_t* newLinePtr = CreateLine("TIMEOUT",sizeof("TIMEOUT"));
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
static void  IntermediateHandler(void* reportPtr)
{
    le_atClient_CmdResponse_t* responsePtr = reportPtr;

    char command[LE_ATCLIENT_CMD_SIZE_MAX_BYTES];
    le_dev_GetCommand(responsePtr->fromWhoRef,command,LE_ATCLIENT_CMD_SIZE_MAX_BYTES);

    LE_DEBUG("Handler Intermediate Response received for (%d)-%s-", le_dev_GetId(responsePtr->fromWhoRef), command);

    CmdSync_t* commandPtr = FindCommand(responsePtr->fromWhoRef);

    if (commandPtr == NULL)
    {
        LE_WARN("This command (%d)-%s- is not found", le_dev_GetId(responsePtr->fromWhoRef), command);
    }
    else
    {
        CmdSync_Line_t* newLinePtr = CreateLine(responsePtr->line, strlen(responsePtr->line)+1);
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
static void FinalHandler(void* reportPtr)
{
    le_atClient_CmdResponse_t* responsePtr = reportPtr;

    char command[LE_ATCLIENT_CMD_SIZE_MAX_BYTES];
    le_dev_GetCommand(responsePtr->fromWhoRef,command,LE_ATCLIENT_CMD_SIZE_MAX_BYTES);

    LE_DEBUG("Handler Final Response received for (%d)-%s-", le_dev_GetId(responsePtr->fromWhoRef), command);

    CmdSync_t* commandPtr = FindCommand(responsePtr->fromWhoRef);
    if (commandPtr == NULL)
    {
        LE_WARN("This command (%d)-%s- is not found", le_dev_GetId(responsePtr->fromWhoRef), command);
    }
    else
    {
        CmdSync_Line_t* newLinePtr = CreateLine(responsePtr->line, strlen(responsePtr->line)+1);
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
    void* ptr   ///< CmdSync_Result_t reference
)
{
    CmdSync_Result_t* resultPtr = ptr;

    le_dls_Link_t* linkPtr;

    while ((linkPtr=le_dls_Pop(&(resultPtr->lines))) != NULL)
    {
        CmdSync_Line_t * currentPtr = CONTAINER_OF(linkPtr,CmdSync_Line_t,link);
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
    void* ptr   ///< CmdSync_Result_t reference
)
{
    CmdSync_t* syncPtr = ptr;
    le_sem_Delete(syncPtr->endSignal);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function Initialize le_atClient_cmdsender.
 * Must be call only once
 *
 */
//--------------------------------------------------------------------------------------------------
static void Init_Memory
(
    void
)
{
    ResultPoolRef = le_mem_CreatePool("ResultPool",sizeof(CmdSync_Result_t));
    ResultPoolRef = le_mem_ExpandPool(ResultPoolRef,LE_ATCLIENT_RESULT_POOL_SIZE);
    le_mem_SetDestructor(ResultPoolRef,ResultDestructor);

    SyncPoolRef = le_mem_CreatePool("SyncPool",sizeof(CmdSync_t));
    SyncPoolRef = le_mem_ExpandPool(SyncPoolRef,LE_ATCLIENT_SYNC_POOL_SIZE);
    le_mem_SetDestructor(SyncPoolRef,SyncDestructor);

    LinePoolRef = le_mem_CreatePool("LinePool",sizeof(CmdSync_Line_t));
    LinePoolRef = le_mem_ExpandPool(LinePoolRef,LE_ATCLIENT_LINE_POOL_SIZE);

    EventIntermediateId   = le_event_CreateId("le_atClient_cmdsenderInter",sizeof(le_atClient_CmdResponse_t));
    EventFinalId          = le_event_CreateId("le_atClient_cmdsenderfinal",sizeof(le_atClient_CmdResponse_t));

    le_event_AddHandler("le_atClient_cmdsync_FinalHandler",EventFinalId,FinalHandler);
    le_event_AddHandler("le_atClient_cmdsync_IntermediateHandler",EventIntermediateId ,IntermediateHandler);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to run the le_atClient_cmd tools thread
 *
 */
//--------------------------------------------------------------------------------------------------
static void* CmdSync_Thread
(
    void* context
)
{
    le_sem_Ref_t semPtr = context;
    LE_INFO("Start AT commands Sender tools");

    Init_Memory();

    le_sem_Post(semPtr);
    le_event_RunLoop();

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function Initialize a Command Port for AT Command
 *
 */
//--------------------------------------------------------------------------------------------------
static void CreateAtPortCommand
(
    void
)
{
    struct le_atClient_device le_atClient_device;

    memset(&le_atClient_device,0,sizeof(le_atClient_device));

    le_utf8_Copy(le_atClient_device.name,"le_atClient_cmd",sizeof(le_atClient_device.name),0);
    le_utf8_Copy(le_atClient_device.path,AT_COMMAND,sizeof(le_atClient_device.path),0);

    AllPorts[LE_ATCLIENT_PORT_COMMAND] = le_dev_CreateInterface(&le_atClient_device);

    LE_FATAL_IF(AllPorts[LE_ATCLIENT_PORT_COMMAND]==NULL, "Could not create port for '%s'",AT_COMMAND);

    LE_DEBUG("Port %s [%s] is created","le_atClient_cmd",AT_COMMAND);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function Initialize a PPP Port for AT Point-to-Point-Protocol
 *
 */
//--------------------------------------------------------------------------------------------------
static void CreateAtPortPpp
(
    void
)
{
    struct le_atClient_device le_atClient_device;

    memset(&le_atClient_device,0,sizeof(le_atClient_device));

    le_utf8_Copy(le_atClient_device.name,"PPP",sizeof(le_atClient_device.name),0);
    le_utf8_Copy(le_atClient_device.path,AT_PPP,sizeof(le_atClient_device.path),0);

    AllPorts[LE_ATCLIENT_PORT_PPP] = le_dev_CreateInterface(&le_atClient_device);

    LE_FATAL_IF(AllPorts[LE_ATCLIENT_PORT_PPP]==NULL, "Could not create port for '%s'",AT_PPP);

    LE_DEBUG("Port %s [%s] is created","PPP",AT_PPP);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is the destructor for ATCommand_t struct
 *
 */
//--------------------------------------------------------------------------------------------------
static void ATCmdPoolDestructor
(
    void *ptr
)
{
    struct le_atClient_cmd* oldPtr = ptr;

    le_dev_ReleaseFromList(&(oldPtr->intermediateResp));
    le_dev_ReleaseFromList(&(oldPtr->finaleResp));
}


//--------------------------------------------------------------------------------------------------
/**
 * This function Initialize the platform adapter layer and the AT Commands.
 * Must be call only once
 *
 * @return LE_OK           The function succeeded.
 * @return LE_DUPLICATE    Module is already initialized
 */
//--------------------------------------------------------------------------------------------------
static le_result_t Init
(
    void
)
{
    AtCommandPool = le_mem_CreatePool("AtCommandPool",sizeof(struct le_atClient_cmd));
    le_mem_ExpandPool(AtCommandPool,LE_ATCLIENT_CMD_POOL_SIZE);

    le_mem_SetDestructor(AtCommandPool,ATCmdPoolDestructor);

    if (le_atClient_cmdThreadRef == NULL)
    {
        le_sem_Ref_t semPtr = le_sem_Create("le_atClient_cmdSenderStartSem",0);

        le_atClient_cmdThreadRef = le_thread_Create("le_atClient_cmdSender",CmdSync_Thread,semPtr);
        le_thread_Start(le_atClient_cmdThreadRef);

        le_sem_Wait(semPtr);
        LE_INFO("AT commands Sender tools is started");
        le_sem_Delete(semPtr);
        return LE_OK;
    }
    else
    {
        return LE_DUPLICATE;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a new AT command.
 *
 * @return pointer to the new AT Command
 */
//--------------------------------------------------------------------------------------------------
static le_atClient_CmdRef_t CreateCmd
(
    void
)
{
    struct le_atClient_cmd* newle_atClient_cmdPtr   = le_mem_ForceAlloc(AtCommandPool);

    newle_atClient_cmdPtr->commandId          = (++IdCpt)%UINT32_MAX;
    memset(newle_atClient_cmdPtr->command,0,sizeof(newle_atClient_cmdPtr->command));
    newle_atClient_cmdPtr->commandSize        = 0;
    memset(newle_atClient_cmdPtr->data,0,sizeof(newle_atClient_cmdPtr->data));
    newle_atClient_cmdPtr->dataSize           = 0;
    newle_atClient_cmdPtr->finaleResp         = LE_DLS_LIST_INIT;
    newle_atClient_cmdPtr->finalId            = NULL;
    newle_atClient_cmdPtr->intermediateResp   = LE_DLS_LIST_INIT;
    newle_atClient_cmdPtr->intermediateId     = NULL;
    newle_atClient_cmdPtr->link               = LE_DLS_LINK_INIT;
    newle_atClient_cmdPtr->timer              = 0;
    newle_atClient_cmdPtr->withExtra          = false;
    newle_atClient_cmdPtr->waitExtra          = false;

    return newle_atClient_cmdPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set when the final response must be sent with string matched.
 *
 */
//--------------------------------------------------------------------------------------------------
static void AddFinalResp
(
    struct le_atClient_cmd    *atcommandPtr,  ///< AT Command
    le_event_Id_t              reportId,      ///< Event Id to report to
    const char               **listFinalPtr   ///< List of pattern to match (must finish with NULL)
)
{
    LE_FATAL_IF(atcommandPtr==NULL,"Cannot set AT Final Resp");

    if (reportId)
    {
        atcommandPtr->finalId = reportId;
        le_dev_AddInList(&(atcommandPtr->finaleResp),listFinalPtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set when the intermediate response must be sent with string
 * matched.
 *
 */
//--------------------------------------------------------------------------------------------------
static void AddIntermediateResp
(
    struct le_atClient_cmd    *atcommandPtr,       ///< AT Command
    le_event_Id_t              reportId,           ///< Event Id to report to
    const char               **listIntermediatePtr ///< List of pattern to match (must finish with NULL)
)
{
    LE_FATAL_IF(atcommandPtr==NULL,"Cannot set AT intermediate Resp");

    if (reportId)
    {
        atcommandPtr->intermediateId = reportId;
        le_dev_AddInList(&(atcommandPtr->intermediateResp),listIntermediatePtr);
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the AT Command to send
 *
 */
//--------------------------------------------------------------------------------------------------
static void AddCommand
(
    struct le_atClient_cmd   *atcommandPtr,  ///< AT Command
    const char               *commandPtr,    ///< the command to send
    bool                      extraData      ///< Indicate if cmd has additionnal data without prefix.
)
{
    LE_FATAL_IF(atcommandPtr==NULL,"atcommandPtr is NULL : Cannot set AT Command");

    le_utf8_Copy((char*)atcommandPtr->command,
                 commandPtr,
                 LE_ATCLIENT_CMD_SIZE_MAX_BYTES,
                 &(atcommandPtr->commandSize));

    atcommandPtr->withExtra = extraData;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the AT Data with prompt expected
 *
 */
//--------------------------------------------------------------------------------------------------
static void AddData
(
    struct le_atClient_cmd   *atcommandPtr,  ///< AT Command
    const char               *dataPtr,       ///< the data to send if expectPrompt is true
    uint32_t                  dataSize      ///< Size of AtData to send
)
{
    LE_FATAL_IF(atcommandPtr==NULL,"atcommandPtr is NULL : Cannot set AT data");

    if (dataPtr)
    {
        LE_FATAL_IF((dataSize>LE_ATCLIENT_DATA_SIZE),
                    "Data -%s- is too long! (%zd>%d)",
                    dataPtr,
                    strlen(dataPtr),
                    LE_ATCLIENT_DATA_SIZE);

        memcpy(atcommandPtr->data, dataPtr, dataSize);
        atcommandPtr->dataSize = dataSize;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set timer when a command is finished
 *
 */
//--------------------------------------------------------------------------------------------------
static void SetTimer
(
    struct le_atClient_cmd      *atcommandPtr,  ///< AT Command
    uint32_t                     timer,         ///< the timer
    le_timer_ExpiryHandler_t     handlerRef     ///< the handler to call if the timer expired
)
{
    LE_FATAL_IF(atcommandPtr==NULL,"atcommandPtr is NULL : Cannot set Timer");
    atcommandPtr->timer = timer;
    atcommandPtr->timerHandler = handlerRef;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send an AT Command and wait for response.
 *
 * @return pointer to the response
 */
//--------------------------------------------------------------------------------------------------
static CmdSync_Result_t* SendCommand
(
    DevRef_t  interfacePtr, ///< Interface where to send the sync command
    le_atClient_CmdRef_t  atReqRef      ///< AT Request to execute
)
{
    CmdSync_t* syncPtr = CreateCommandSync();
    CmdSync_Result_t* resultPtr = syncPtr->resultPtr;

    le_mem_AddRef(atReqRef);
    syncPtr->le_atClient_cmdInProcessRef = atReqRef;
    le_dls_Queue(&CommandList,&(syncPtr->link));

    syncPtr->interfaceRef = interfacePtr;

    SendCommandRequest(interfacePtr, atReqRef);
    le_sem_Wait(syncPtr->endSignal);

    le_dls_Remove(&CommandList,&(syncPtr->link));

    LE_DEBUG("Command(%d)'s result",le_dev_GetId(atReqRef));
    CmdSync_Print(resultPtr);

    le_mem_Release(syncPtr);
    le_mem_Release(atReqRef);

    return resultPtr;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get line in index
 *
 */
//--------------------------------------------------------------------------------------------------
static char* GetLine
(
    CmdSync_Result_t *resultPtr,
    uint32_t                      index
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
    if (linkPtr)
    {
        CmdSync_Line_t *currLinePtr = CONTAINER_OF(linkPtr, CmdSync_Line_t, link);
        return currLinePtr->line;
    }
    else
    {
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
static char* GetFinalLine
(
    CmdSync_Result_t *resultPtr
)
{
    le_dls_Link_t* linkPtr;
    LE_FATAL_IF(resultPtr==NULL, "bad parameter");

    if ((linkPtr = le_dls_PeekTail(&(resultPtr->lines))) != NULL)
    {
        CmdSync_Line_t *currLinePtr = CONTAINER_OF(linkPtr, CmdSync_Line_t, link);
        return currLinePtr->line;
    }
    else
    {
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
static le_event_Id_t GetIntermediateEventId
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
static le_event_Id_t GetFinalEventId
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
static le_timer_ExpiryHandler_t GetTimerExpiryHandler
(
    void
)
{
    return TimerHandler;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to start atCommandClient on a device.
 *
 * @note
 * After this call, unsolicited pattern can be parse, AT Command can be sent
 * on configuration Port Handle.
 *
 */
//--------------------------------------------------------------------------------------------------
static void StartInterface
(
    le_atClient_Ports_t   devicePtr    ///< Device
)
{
    DevRef_t atcommandclientitfPtr = AllPorts[devicePtr];
    le_event_Report(atcommandclientitfPtr->resumeInterfaceId,NULL,0);
    le_sem_Wait(atcommandclientitfPtr->waitingSemaphore);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to Stop the atCommandClient on a device (not used for the moment).
 *
 */
//--------------------------------------------------------------------------------------------------
// static void StopInterface
// (
//     le_atClient_Ports_t   devicePtr    ///< Device
// )
// {
//     DevRef_t atcommandclientitfPtr = AllPorts[devicePtr];
//     le_event_Report(atcommandclientitfPtr->suspendInterfaceId,NULL,0);
//     le_sem_Wait(atcommandclientitfPtr->waitingSemaphore);
// }


//--------------------------------------------------------------------------------------------------
/**
 * This function Initialize the all ports that will be available.
 * Must be call only once
 *
 * @return LE_OK           The function succeeded.
 * @return LE_DUPLICATE    Module is already initialized
 */
//--------------------------------------------------------------------------------------------------
static le_result_t PortsInit
(
    void
)
{
    if ( IsInitialized )
    {
        return LE_DUPLICATE;
    }

    memset(AllPorts,0,sizeof(AllPorts));

    CreateAtPortCommand();
    CreateAtPortPpp();

    IsInitialized = true;

    return LE_OK;
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
    memset(cmdPtr->cmd,0,sizeof(cmdPtr->cmd));
    memset(cmdPtr->data,0,sizeof(cmdPtr->data));

    cmdPtr->commandId                       = (++IdCpt)%UINT32_MAX;
    cmdPtr->intermediateExpectResponseList  = LE_DLS_LIST_INIT;
    cmdPtr->expInterRespIndicator           = false;
    cmdPtr->expectResponseList              = LE_DLS_LIST_INIT;
    cmdPtr->expRespIndicator                = false;
    cmdPtr->dataSize                        = 0;
    cmdPtr->timeout                         = CMD_DEFAULT_TIMEOUT;
    cmdPtr->interface                       = AllPorts[LE_ATCLIENT_PORT_COMMAND];
    cmdPtr->resultRefPtr                    = NULL;
    cmdPtr->ref                             = le_ref_CreateRef(CmdRefMap, cmdPtr);
    cmdPtr->lineCount                       = 0;

    return cmdPtr->ref;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to delete an AT command reference.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
void le_atClient_Delete
(
    le_atClient_CmdRef_t     cmdRef       ///< [IN] AT Command
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return;
    }

    le_ref_DeleteRef(CmdRefMap, cmdRef);
    le_mem_Release(cmdPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the AT command string to be send.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetCommand
(
    le_atClient_CmdRef_t     cmdRef,      ///< [IN] AT Command
    const char*              commandPtr   ///< [IN] Set Command
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
 * Several intermediate responses can be specified separated by a '|' character into the string given in parameter.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetIntermediateResponse
(
    le_atClient_CmdRef_t     cmdRef,          ///< [IN] AT Command
    const char*              intermediatePtr  ///< [IN] Set Intermediate
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
            le_atClient_machinestring_t* newStringPtr = le_mem_ForceAlloc(AtStringPool);

            if(strlen(interPtr)>LE_ATCLIENT_CMD_SIZE_MAX_BYTES)
            {
                LE_DEBUG("%s is too long (%zd): Max size %d",interPtr,strlen(interPtr),LE_ATCLIENT_CMD_SIZE_MAX_BYTES);
                return LE_FAULT;
            }

            strncpy(newStringPtr->line,interPtr,LE_ATCLIENT_CMD_SIZE_MAX_BYTES);

            newStringPtr->link = LE_DLS_LINK_INIT;

            le_dls_Queue(&(cmdPtr->intermediateExpectResponseList),&(newStringPtr->link));

            interPtr = strtok_r(NULL, "|", &savePtr);
        }
        cmdPtr->expInterRespIndicator = true;
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the final response(s) of the AT command execution.
 * Several final responses can be specified separated by a '|' character into the string given in parameter.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetFinalResponse
(
    le_atClient_CmdRef_t     cmdRef,         ///< [IN] AT Command
    const char*              responsePtr     ///< [IN] Set Response
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
            le_atClient_machinestring_t* newStringPtr = le_mem_ForceAlloc(AtStringPool);

            if(strlen(respPtr)>LE_ATCLIENT_CMD_SIZE_MAX_BYTES)
            {
                LE_DEBUG("%s is too long (%zd): Max size %d",respPtr,strlen(respPtr),LE_ATCLIENT_CMD_SIZE_MAX_BYTES);
                return LE_FAULT;
            }

            strncpy(newStringPtr->line,respPtr,LE_ATCLIENT_CMD_SIZE_MAX_BYTES);

            newStringPtr->link = LE_DLS_LINK_INIT;

            le_dls_Queue(&(cmdPtr->expectResponseList),&(newStringPtr->link));

            respPtr = strtok_r(NULL, "|", &savePtr);
        }
        cmdPtr->expRespIndicator = true;
    }
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the data when the prompt is expected.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetData
(
    le_atClient_CmdRef_t     cmdRef,      ///< [IN] AT Command
    const char*              dataPtr,     ///< [IN] The AT Data to send
    uint32_t                 dataSize     ///< [IN] Size of AT Data to send
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_NOT_FOUND;
    }

    if (dataPtr)
    {
        if(dataSize>LE_ATCLIENT_DATA_SIZE)
        {
            LE_ERROR("Data -%s- is too long! (%zd>%d)",dataPtr,strlen(dataPtr),LE_ATCLIENT_DATA_SIZE);
            return LE_FAULT;
        }

        if (dataSize > LE_ATCLIENT_DATA_MAX_BYTES-1)
        {
            return LE_FAULT;
        }

        strncpy(cmdPtr->data, dataPtr, dataSize);
        cmdPtr->dataSize = dataSize;
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
 * This function must be called to set the timeout of the AT command in execution.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetTimeout
(
    le_atClient_CmdRef_t     cmdRef,        ///< [IN] AT Command
    uint32_t                 timer          ///< [IN] Set Timer
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
 * This function must be called to set the PPP port for the AT command execution.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetPort
(
    le_atClient_CmdRef_t    cmdRef,        ///< [IN] AT Command
    le_atClient_Ports_t     port           ///< [IN] Set port
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_NOT_FOUND;
    }

    cmdPtr->interface = AllPorts[port];
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to send an AT Command and wait for response.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_TIMEOUT when a timeout occur
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_Send
(
    le_atClient_CmdRef_t     cmdRef         ///< [IN] AT Command
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
        return LE_NOT_FOUND;
    }

    le_dls_Link_t* linkPtr;
    le_atClient_machinestring_t* nodePtr;
    le_atClient_CmdRef_t atReqRef;

    atReqRef = CreateCmd();
    AddCommand(atReqRef,cmdPtr->cmd,false);

    if (cmdPtr->dataSize)
    {
        AddData(atReqRef,cmdPtr->data,cmdPtr->dataSize);
    }
    else
    {
        AddData(atReqRef,NULL,0);
    }

    SetTimer(atReqRef,cmdPtr->timeout,GetTimerExpiryHandler());

    // Expected Intermediate Response
    if (cmdPtr->expInterRespIndicator)
    {
        int i=0, nbIntermediate = le_dls_NumLinks(&cmdPtr->intermediateExpectResponseList);
        char interRespPtr[nbIntermediate+1][64];
        memset(interRespPtr,0,64*(nbIntermediate+1));

        linkPtr = le_dls_Peek(&cmdPtr->intermediateExpectResponseList);
        if (linkPtr != NULL)
        {
            do
            {
                // Get the node from MsgList
                nodePtr = CONTAINER_OF(linkPtr, le_atClient_machinestring_t, link);

                if (nodePtr)
                {
                    strncpy(interRespPtr[i], nodePtr->line, strlen(nodePtr->line));
                }

                // Move to the next node.
                linkPtr = le_dls_PeekNext(&cmdPtr->intermediateExpectResponseList, linkPtr);

                i++;
            } while (linkPtr != NULL);
        }

        char* interRespTabPtr[nbIntermediate+1];
        memset(interRespTabPtr,0,(nbIntermediate+1)*sizeof(char*));
        for(i=0;i<nbIntermediate;i++)
        {
            interRespTabPtr[i] = &interRespPtr[i][0];
        }

        AddIntermediateResp(atReqRef,GetIntermediateEventId(),(const char **)interRespTabPtr);
    }
    else
    {
        const char* respPtr[] = {"", NULL};
        AddIntermediateResp(atReqRef,GetIntermediateEventId(),respPtr);
    }


    // Expected Final Response
    if (cmdPtr->expRespIndicator)
    {
        int i=0, nbResp = le_dls_NumLinks(&cmdPtr->expectResponseList);
        char respPtr[nbResp+1][64];
        memset(respPtr,0,64*(nbResp+1));
        char* respTabPtr[nbResp+1];
        memset(respTabPtr,0,(nbResp+1)*sizeof(char*));

        linkPtr = le_dls_Peek(&cmdPtr->expectResponseList);
        if (linkPtr != NULL)
        {
            do
            {
                // Get the node from MsgList
                nodePtr = CONTAINER_OF(linkPtr, le_atClient_machinestring_t, link);

                if (nodePtr)
                {
                    strncpy(respPtr[i], nodePtr->line, strlen(nodePtr->line));
                }

                // Move to the next node.
                linkPtr = le_dls_PeekNext(&cmdPtr->expectResponseList, linkPtr);

                i++;
            } while (linkPtr != NULL);
        }

        for(i=0;i<nbResp;i++)
        {
            respTabPtr[i] = &respPtr[i][0];
        }

        AddFinalResp(atReqRef,GetFinalEventId(),(const char **)respTabPtr);
    }
    else
    {
        const char* finalRespPtr[] = {"OK","ERROR","+CME ERROR:","+CMS ERROR","TIMEOUT", NULL};
        AddFinalResp(atReqRef,GetFinalEventId(),finalRespPtr);
    }

    cmdPtr->resultRefPtr = SendCommand(cmdPtr->interface,atReqRef);

    le_mem_Release(atReqRef);
    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the first intermediate response
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_GetFirstIntermediateResponse
(
    le_atClient_CmdRef_t        cmdRef,                         ///< [IN] AT Command
    char*                       intermediateRspPtr,             ///< [OUT] Get Next Line
    size_t                      intermediateRspNumElements      ///< [IN] Size of response
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
    }

    char* firstLinePtr = GetLine(cmdPtr->resultRefPtr,0);
    strncpy(intermediateRspPtr, firstLinePtr, intermediateRspNumElements);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the intermediate response
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_GetNextIntermediateResponse
(
    le_atClient_CmdRef_t        cmdRef,                         ///< [IN] AT Command
    char*                       intermediateRspPtr,             ///< [OUT] Get Next Line
    size_t                      intermediateRspNumElements      ///< [IN] Size of response
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
    }

    cmdPtr->lineCount += 1;

    char* nextLinePtr = GetLine(cmdPtr->resultRefPtr,
                                cmdPtr->lineCount);
    strncpy(intermediateRspPtr, nextLinePtr, intermediateRspNumElements);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function is used to get the final response
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_GetFinalResponse
(
    le_atClient_CmdRef_t          cmdRef,                   ///< [IN] AT Command
    char*                         finalRspPtr,              ///< [OUT] Get Final Line
    size_t                        finalRspNumElements       ///< [IN] Size of response
)
{
    AtCmd_t* cmdPtr = le_ref_Lookup(CmdRefMap, cmdRef);
    if (cmdPtr == NULL)
    {
        LE_KILL_CLIENT("Invalid reference (%p) provided!", cmdPtr);
    }

    char* finalLinePtr = GetFinalLine(cmdPtr->resultRefPtr);
    strncpy(finalRspPtr, finalLinePtr, finalRspNumElements);

    return LE_OK;
}


//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to automatically set and send an AT Command.
 *
 * @return
 *      - LE_FAULT when function failed
 *      - LE_NOT_FOUND when Invalid reference
 *      - LE_TIMEOUT when a timeout occur
 *      - LE_OK when function succeed
 */
//--------------------------------------------------------------------------------------------------
le_result_t le_atClient_SetCommandAndSend
(
    le_atClient_CmdRef_t*               cmdRefPtr,        ///< [OUT] Command reference
    const char*                         commandPtr,       ///< [IN] AT Command
    const char*                         interRespPtr,     ///< [IN] Expected Intermediate Response
    const char*                         finalRespPtr,     ///< [IN] Expected Final Response
    uint32_t                            timeout           ///< [IN] Timeout
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
 * This function must be called to Set an unsolicited pattern to match.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atClient_AddUnsolicitedResponseHandler
(
    le_event_Id_t               unsolicitedReportId,        ///< [IN] Event Id to report to
    char*                       unsolRspPtr,                ///< [IN] Pattern to match
    bool                        withExtraData               ///< [IN] Indicate if the unsolicited has more than one line
)
{
    DevRef_t atcommandclientitfPtr = AllPorts[LE_ATCLIENT_PORT_COMMAND];
    atUnsolicited_t* newUnsolicitedPtr = le_dev_Create();

    newUnsolicitedPtr->withExtraData = withExtraData;
    newUnsolicitedPtr->unsolicitedReportId = unsolicitedReportId;
    le_utf8_Copy(newUnsolicitedPtr->unsolRsp,unsolRspPtr,ATCOMMANDCLIENT_UNSOLICITED_SIZE,0);

    le_event_ReportWithRefCounting(atcommandclientitfPtr->subscribeUnsolicitedId,
                                   newUnsolicitedPtr);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to remove an unsolicited pattern to match.
 *
 */
//--------------------------------------------------------------------------------------------------
void le_atClient_RemoveUnsolicitedResponseHandler
(
    le_event_Id_t               unsolicitedReportId,        ///< [IN] Event Id to report to
    char*                       unsolRspPtr                 ///< [IN] Pattern to match
)
{
    DevRef_t atcommandclientitfPtr = AllPorts[LE_ATCLIENT_PORT_COMMAND];
    atUnsolicited_t* newUnsolicitedPtr = le_dev_Create();

    newUnsolicitedPtr->unsolicitedReportId = unsolicitedReportId;
    le_utf8_Copy(newUnsolicitedPtr->unsolRsp,unsolRspPtr,ATCOMMANDCLIENT_UNSOLICITED_SIZE,0);

    le_event_ReportWithRefCounting(atcommandclientitfPtr->unSubscribeUnsolicitedId,
                                   newUnsolicitedPtr);
}


//--------------------------------------------------------------------------------------------------
/**
 * The COMPONENT_INIT Intialize the AT Client Component when Legato start
 *
 */
//--------------------------------------------------------------------------------------------------
COMPONENT_INIT
{
    le_dev_Init();
    Init();
    PortsInit();

    CmdRefMap = le_ref_CreateMap("CmdRefMap", 300);
    AtStringPool = le_mem_CreatePool("AtStringPool",sizeof(le_atClient_machinestring_t));
    le_mem_ExpandPool(AtStringPool,DEFAULT_ATSTRING_POOL_SIZE);

    StartInterface(LE_ATCLIENT_PORT_COMMAND);
    StartInterface(LE_ATCLIENT_PORT_PPP);
}