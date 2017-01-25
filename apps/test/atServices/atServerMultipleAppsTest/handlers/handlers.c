/**
 * This module implements handlers functions for AT commands server API.
 *
 * Copyright (C) Sierra Wireless Inc.
 *
 */

#include "legato.h"
#include "interfaces.h"
#include "handlers.h"

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of parameters
 */
//--------------------------------------------------------------------------------------------------
#define PARAM_MAX 10

//------------------------------------------------------------------------------
/**
 * Uppercase a string
 */
//------------------------------------------------------------------------------
static char* Uppercase
(
    char* strPtr
)
{
    int i = 0;
    while (strPtr[i] != '\0')
    {
        strPtr[i] = toupper(strPtr[i]);
        i++;
    }

    return strPtr;
}

//--------------------------------------------------------------------------------------------------
/**
 * get command name's refrence
 */
//--------------------------------------------------------------------------------------------------
static le_atServer_CmdRef_t GetCmdRef
(
    atSession_t* atSession,
    const char* cmdNamePtr
)
{
    int i;
    char* cmdName;

    cmdName = Uppercase((char *)cmdNamePtr);

    for (i=0; i<atSession->cmdsCount; i++)
    {
        if ( ! (strcmp(atSession->atCmds[i].atCmdPtr, cmdName)) )
        {
            return atSession->atCmds[i].cmdRef;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
/**
 * generic command handler
 *
 */
//--------------------------------------------------------------------------------------------------
void GenericCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    char rsp[LE_ATDEFS_RESPONSE_MAX_BYTES];
    char atCommandName[LE_ATDEFS_COMMAND_MAX_BYTES];
    char param[LE_ATDEFS_PARAMETER_MAX_BYTES];
    int i = 0;

    LE_INFO("commandRef %p", commandRef);

    memset(atCommandName, 0, LE_ATDEFS_COMMAND_MAX_BYTES);

    // Get command name
    LE_ASSERT(le_atServer_GetCommandName(
        commandRef,atCommandName,LE_ATDEFS_COMMAND_MAX_BYTES) == LE_OK);

    LE_INFO("AT command name %s", atCommandName);

    memset(rsp, 0, LE_ATDEFS_RESPONSE_MAX_BYTES);
    snprintf(rsp, LE_ATDEFS_RESPONSE_MAX_BYTES, "%s TYPE: ", atCommandName+2);

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            LE_INFO("Type PARA");
            snprintf(rsp+strlen(rsp),
                LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "PARA");
        break;

        case LE_ATSERVER_TYPE_TEST:
            LE_INFO("Type TEST");
            snprintf(rsp+strlen(rsp),
                LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "TEST");
        break;

        case LE_ATSERVER_TYPE_READ:
            LE_INFO("Type READ");
            snprintf(rsp+strlen(rsp),
                LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "READ");
        break;

        case LE_ATSERVER_TYPE_ACT:
            LE_INFO("Type ACT");
            snprintf(rsp+strlen(rsp),
                LE_ATDEFS_RESPONSE_MAX_BYTES-strlen(rsp), "ACT");
        break;

        default:
            LE_ASSERT(0);
        break;
    }

    // Send the command type into an intermediate response
    LE_ASSERT(le_atServer_SendIntermediateResponse(commandRef, rsp) == LE_OK);

    // Send parameters into an intermediate response
    for (i = 0; i < parametersNumber && parametersNumber <= PARAM_MAX; i++)
    {
        memset(param,0,LE_ATDEFS_PARAMETER_MAX_BYTES);
        LE_ASSERT(
            le_atServer_GetParameter(commandRef,
                                    i,
                                    param,
                                    LE_ATDEFS_PARAMETER_MAX_BYTES)
            == LE_OK);


        memset(rsp,0,LE_ATDEFS_RESPONSE_MAX_BYTES);
        snprintf(rsp,LE_ATDEFS_RESPONSE_MAX_BYTES,
            "%s PARAM %d: %s", atCommandName+2, i, param);

        LE_ASSERT(
            le_atServer_SendIntermediateResponse(commandRef, rsp)
            == LE_OK);

        LE_INFO("param %d \"%s\"", i, param);
    }

    // Send Final response
    LE_ASSERT(
        le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "")
        == LE_OK);
}

//------------------------------------------------------------------------------
/**
 * AT command handler
 *
 */
//------------------------------------------------------------------------------
void AtCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    // Send Final response
    LE_ASSERT(
        le_atServer_SendFinalResponse(commandRef, LE_ATSERVER_OK, false, "")
        == LE_OK);

}

//------------------------------------------------------------------------------
/**
 * Delete command handler
 *
 */
//------------------------------------------------------------------------------
void DelCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    le_atServer_FinalRsp_t finalRsp = LE_ATSERVER_OK;
    le_atServer_CmdRef_t cmdRef;
    char param[LE_ATDEFS_PARAMETER_MAX_BYTES];
    int i = 0;
    atSession_t* atSession;

    atSession = (atSession_t *) contextPtr;

    switch (type)
    {
        case LE_ATSERVER_TYPE_PARA:
            for (i = 0;
                i < parametersNumber && parametersNumber <= PARAM_MAX;
                i++)
            {
                memset(param,0,LE_ATDEFS_PARAMETER_MAX_BYTES);
                // get the command to delete
                LE_ASSERT(
                le_atServer_GetParameter(commandRef,
                                        i,
                                        param,
                                        LE_ATDEFS_PARAMETER_MAX_BYTES)
                                    == LE_OK);
                // get its refrence
                cmdRef = GetCmdRef(atSession, param);
                LE_DEBUG("Deleting %p => %s", cmdRef, param);
                // delete the command
                LE_ASSERT(le_atServer_Delete(cmdRef) == LE_OK);
            }
            // send an OK final response
            finalRsp = LE_ATSERVER_OK;
            break;
        // this command doesn't support test and read send an ERROR final
        // reponse
        case LE_ATSERVER_TYPE_TEST:
        case LE_ATSERVER_TYPE_READ:
            finalRsp = LE_ATSERVER_ERROR;
            break;
        // an action command type to verify that AT+DEL command does exist
        // send an OK final response
        case LE_ATSERVER_TYPE_ACT:
            finalRsp = LE_ATSERVER_OK;
            break;

        default:
            LE_ASSERT(0);
        break;
    }
    // send final response
    LE_ASSERT(
        le_atServer_SendFinalResponse(
            commandRef, finalRsp, false, "")
        == LE_OK);
}

//------------------------------------------------------------------------------
/**
 * Close command handler
 *
 */
//------------------------------------------------------------------------------
void CloseCmdHandler
(
    le_atServer_CmdRef_t commandRef,
    le_atServer_Type_t type,
    uint32_t parametersNumber,
    void* contextPtr
)
{
    atSession_t* atSession;

    atSession = (atSession_t *) contextPtr;

    switch (type) {
    // this command doesn't accept parameter, test or read send an ERROR
    // final response
    case LE_ATSERVER_TYPE_PARA:
    case LE_ATSERVER_TYPE_TEST:
    case LE_ATSERVER_TYPE_READ:
        LE_ASSERT(
            le_atServer_SendFinalResponse(
                commandRef, LE_ATSERVER_ERROR, false, "")
            == LE_OK);

        break;
    // in case of an action command just close the session
    // we cannot send a response, the closing is in progress
    case LE_ATSERVER_TYPE_ACT:
        LE_ASSERT(le_atServer_Close(atSession->devRef) == LE_OK);
        break;

    default:
        LE_ASSERT(0);
        break;
    }
}
