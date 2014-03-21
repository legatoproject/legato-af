/** @file atmachinecommand.c
 *
 * Copyright (C) Sierra Wireless, Inc. 2013. All rights reserved. Use of this work is subject to license.
 */

#include "atMachineCommand.h"
#include "atMachineManager.h"
#include "atMachineString.h"

#define DEFAULT_ATCOMMAND_POOL_SIZE     1
static le_mem_PoolRef_t  AtCommandPool;

#ifndef UINT32_MAX
    #define     UINT32_MAX      ((uint32_t)-1)
#endif
static uint32_t     IdCpt = 0;

//--------------------------------------------------------------------------------------------------
/**
 * This function is the destructor for ATCommand_t struct
 *
 */
//--------------------------------------------------------------------------------------------------
static void atCommandPoolDestructor(void *ptr)
{
    struct atcmd* oldPtr = ptr;

    atmachinestring_ReleaseFromList(&(oldPtr->intermediateResp));
    atmachinestring_ReleaseFromList(&(oldPtr->finaleResp));
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to initialize the atcommand
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinecommand_Init
(
    void
)
{
    AtCommandPool = le_mem_CreatePool("AtCommandPool",sizeof(struct atcmd));
    le_mem_ExpandPool(AtCommandPool,DEFAULT_ATCOMMAND_POOL_SIZE);

    le_mem_SetDestructor(AtCommandPool,atCommandPoolDestructor);
}

//--------------------------------------------------------------------------------------------------
/**
 * This function is used to check if the line should be report
 *
 */
//--------------------------------------------------------------------------------------------------
static void CheckIntermediateExtraData
(
    struct atcmd *atcommandPtr,
    char               *atLinePtr,
    size_t              atLineSize
)
{
    LE_DEBUG("Start checking intermediate extra data");

    if ( atcommandPtr->waitExtra )
    {
        atcmd_Response_t atResp;

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
    struct atcmd  *atcommandPtr,
    char                *atLinePtr,
    size_t               atLineSize,
    bool                 isFinal
)
{
    const le_dls_List_t *listPtr;
    le_event_Id_t  reportId;

    if (isFinal)
    {
        listPtr  = &(atcommandPtr->finaleResp);
        reportId = atcommandPtr->finalId;
    } else {
        listPtr  = &(atcommandPtr->intermediateResp);
        reportId = atcommandPtr->intermediateId;
    }


    le_dls_Link_t* linkPtr = le_dls_Peek(listPtr);
    /* Browse all the queue while the string is not found */
    while (linkPtr != NULL)
    {
        atmachinestring_t *currStringPtr = CONTAINER_OF(linkPtr,
                                                 atmachinestring_t,
                                                 link);

        if ( strncmp(atLinePtr,currStringPtr->line,strlen(currStringPtr->line)) == 0)
        {
            atcmd_Response_t atResp;

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
 * This function is used to check if the line match any of intermediate string of the command
 *
 */
//--------------------------------------------------------------------------------------------------
void atmachinecommand_CheckIntermediate
(
    atcmd_Ref_t  atCommandRef,
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
bool atmachinecommand_CheckFinal
(
    atcmd_Ref_t  atCommandRef,
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
void atmachinecommand_Prepare
(
    struct atcmd *atcommandPtr
)
{
    /* Convert end of string into carrier return */
    LE_FATAL_IF((atcommandPtr->commandSize > ATCOMMAND_SIZE),
                "command is too long(%zd): Max size=%d",
                atcommandPtr->commandSize , ATCOMMAND_SIZE);

    atcommandPtr->command[atcommandPtr->commandSize++]   = '\r';
    atcommandPtr->command[atcommandPtr->commandSize] = '\0';

    if ( atcommandPtr->dataSize && (atcommandPtr->dataSize <= ATCOMMAND_DATA_SIZE) ) {
        /* Convert end of string into ctrl-z code */
        atcommandPtr->data[atcommandPtr->dataSize++] = 0x1A;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to create a new AT command.
 *
 * @return pointer to the new AT Command
 */
//--------------------------------------------------------------------------------------------------
atcmd_Ref_t atcmd_Create
(
    void
)
{
    struct atcmd* newATCmdPtr = le_mem_ForceAlloc(AtCommandPool);

    newATCmdPtr->commandId          = (++IdCpt)%UINT32_MAX;
    memset(newATCmdPtr->command,0,sizeof(newATCmdPtr->command));
    newATCmdPtr->commandSize        = 0;
    memset(newATCmdPtr->data,0,sizeof(newATCmdPtr->data));
    newATCmdPtr->dataSize           = 0;
    newATCmdPtr->finaleResp         = LE_DLS_LIST_INIT;
    newATCmdPtr->finalId            = NULL;
    newATCmdPtr->intermediateResp   = LE_DLS_LIST_INIT;
    newATCmdPtr->intermediateId     = NULL;
    newATCmdPtr->link               = LE_DLS_LINK_INIT;
    newATCmdPtr->timer              = 0;
    newATCmdPtr->withExtra          = false;
    newATCmdPtr->waitExtra          = false;

    return newATCmdPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set when the final response must be sent with string matched.
 *
 */
//--------------------------------------------------------------------------------------------------
void atcmd_AddFinalResp
(
    struct atcmd  *atcommandPtr,  ///< AT Command
    le_event_Id_t        reportId,      ///< Event Id to report to
    const char         **listFinalPtr   ///< List of pattern to match (must finish with NULL)
)
{
    LE_FATAL_IF(atcommandPtr==NULL,"Cannot set AT Final Resp");
    if (reportId)
    {
        atcommandPtr->finalId = reportId;
        atmachinestring_AddInList(&(atcommandPtr->finaleResp),listFinalPtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set when the intermediate response must be sent with string
 * matched.
 *
 */
//--------------------------------------------------------------------------------------------------
void atcmd_AddIntermediateResp
(
    struct atcmd  *atcommandPtr,       ///< AT Command
    le_event_Id_t        reportId,           ///< Event Id to report to
    const char         **listIntermediatePtr ///< List of pattern to match (must finish with NULL)
)
{
    LE_FATAL_IF(atcommandPtr==NULL,"Cannot set AT intermediate Resp");
    if (reportId)
    {
        atcommandPtr->intermediateId = reportId;
        atmachinestring_AddInList(&(atcommandPtr->intermediateResp),listIntermediatePtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the AT Command to send
 *
 */
//--------------------------------------------------------------------------------------------------
void atcmd_AddCommand
(
    struct atcmd *atcommandPtr,  ///< AT Command
    const char         *commandPtr,    ///< the command to send
    bool                extraData      ///< Indicate if cmd has additionnal data without prefix.
)
{
    LE_FATAL_IF(atcommandPtr==NULL,"atcommandPtr is NULL : Cannot set AT Command");

    le_utf8_Copy((char*)atcommandPtr->command,
                 commandPtr,
                 ATCOMMAND_SIZE,
                 &(atcommandPtr->commandSize));

    atcommandPtr->withExtra = extraData;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to set the AT Data with prompt expected
 *
 */
//--------------------------------------------------------------------------------------------------
void atcmd_AddData
(
    struct atcmd *atcommandPtr,  ///< AT Command
    const char         *dataPtr,       ///< the data to send if expectPrompt is true
    uint32_t            dataSize      ///< Size of AtData to send
)
{
    LE_FATAL_IF(atcommandPtr==NULL,"atcommandPtr is NULL : Cannot set AT data");
    if (dataPtr) {
        LE_FATAL_IF( (dataSize>ATCOMMAND_DATA_SIZE),
                     "Data -%s- is too long! (%zd>%d)",dataPtr,strlen(dataPtr),ATCOMMAND_DATA_SIZE);

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
void atcmd_SetTimer
(
    struct atcmd      *atcommandPtr,  ///< AT Command
    uint32_t                 timer,         ///< the timer
    le_timer_ExpiryHandler_t handlerRef     ///< the handler to call if the timer expired
)
{
    LE_FATAL_IF(atcommandPtr==NULL,"atcommandPtr is NULL : Cannot set Timer");
    atcommandPtr->timer = timer;
    atcommandPtr->timerHandler = handlerRef;
}

//--------------------------------------------------------------------------------------------------
/**
 * This function must be called to get the ID of a command
 *
 */
//--------------------------------------------------------------------------------------------------
uint32_t atcmd_GetId
(
    struct atcmd      *atcommandPtr  ///< AT Command
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
le_result_t atcmd_GetCommand
(
    struct atcmd      *atcommandPtr,  ///< AT Command
    char                    *commandPtr,    ///< [IN/OUT] command buffer
    size_t                   commandSize    ///< [IN] size of commandPtr
)
{
    le_result_t result;
    LE_FATAL_IF(atcommandPtr==NULL,"atcommandPtr is NULL : Cannot Get command string");

    result = le_utf8_Copy(commandPtr,(const char*)(atcommandPtr->command),commandSize,NULL);

    if (result==LE_OK) {
        commandPtr[atcommandPtr->commandSize-1] = '\0'; // remove the CR char
    } else {
        commandPtr[commandSize] = '\0';
    }

    return result;
}
